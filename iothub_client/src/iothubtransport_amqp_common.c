// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_macro_utils/macro_utils.h"

#include "azure_uamqp_c/cbs.h"
#include "azure_uamqp_c/amqp_definitions.h"
#include "azure_uamqp_c/session.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/messaging.h"

#include "iothub_message.h"
#include "internal/iothub_message_private.h"
#include "iothub_client_options.h"
#include "internal/iothub_client_private.h"
#include "internal/iothubtransportamqp_methods.h"
#include "internal/iothub_client_retry_control.h"
#include "internal/iothubtransport_amqp_common.h"
#include "internal/iothubtransport_amqp_connection.h"
#include "internal/iothubtransport_amqp_device.h"
#include "internal/iothubtransport.h"
#include "iothub_client_version.h"
#include "internal/iothub_transport_ll_private.h"


#define RESULT_OK                                 0
#define INDEFINITE_TIME                           ((time_t)(-1))
#define DEFAULT_CBS_REQUEST_TIMEOUT_SECS          30
#define DEFAULT_DEVICE_STATE_CHANGE_TIMEOUT_SECS  60
#define DEFAULT_EVENT_SEND_TIMEOUT_SECS           300
#define MAX_NUMBER_OF_DEVICE_FAILURES             5
#define DEFAULT_SERVICE_KEEP_ALIVE_FREQ_SECS      240
#define DEFAULT_REMOTE_IDLE_PING_RATIO            0.50
#define DEFAULT_RETRY_POLICY                      IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
// DEFAULT_MAX_RETRY_TIME_IN_SECS = 0 means infinite retry.
#define DEFAULT_MAX_RETRY_TIME_IN_SECS            0
#define MAX_SERVICE_KEEP_ALIVE_RATIO              0.9
#define DEFAULT_DEVICE_STOP_DELAY                 10

// ---------- Data Definitions ---------- //

typedef enum AMQP_TRANSPORT_AUTHENTICATION_MODE_TAG
{
    AMQP_TRANSPORT_AUTHENTICATION_MODE_NOT_SET,
    AMQP_TRANSPORT_AUTHENTICATION_MODE_CBS,
    AMQP_TRANSPORT_AUTHENTICATION_MODE_X509
} AMQP_TRANSPORT_AUTHENTICATION_MODE;

/*
Definition of transport states:

AMQP_TRANSPORT_STATE_NOT_CONNECTED:                    Initial state when the transport is created.
AMQP_TRANSPORT_STATE_CONNECTING:                       First connection ever.
AMQP_TRANSPORT_STATE_CONNECTED:                        Transition from AMQP_TRANSPORT_STATE_CONNECTING or AMQP_TRANSPORT_STATE_RECONNECTING.
AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED:            When a failure occurred and the transport identifies a reconnection is needed.
AMQP_TRANSPORT_STATE_READY_FOR_RECONNECTION:           Transition from AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED after all prep is done (transient instances are destroyed, devices are stopped).
AMQP_TRANSPORT_STATE_RECONNECTING:                     Transition from AMQP_TRANSPORT_STATE_READY_FOR_RECONNECTION.
AMQP_TRANSPORT_STATE_NOT_CONNECTED_NO_MORE_RETRIES:    State reached if the maximum number/length of reconnections has been reached.
AMQP_TRANSPORT_STATE_BEING_DESTROYED:                  State set if IoTHubTransport_AMQP_Common_Destroy function is invoked.
*/

#define AMQP_TRANSPORT_STATE_STRINGS                    \
    AMQP_TRANSPORT_STATE_NOT_CONNECTED,                 \
    AMQP_TRANSPORT_STATE_CONNECTING,                    \
    AMQP_TRANSPORT_STATE_CONNECTED,                     \
    AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED,         \
    AMQP_TRANSPORT_STATE_READY_FOR_RECONNECTION,        \
    AMQP_TRANSPORT_STATE_RECONNECTING,                  \
    AMQP_TRANSPORT_STATE_NOT_CONNECTED_NO_MORE_RETRIES, \
    AMQP_TRANSPORT_STATE_BEING_DESTROYED

// Suppress unused function warning for AMQP_TRANSPORT_STATEstrings
#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
MU_DEFINE_LOCAL_ENUM(AMQP_TRANSPORT_STATE, AMQP_TRANSPORT_STATE_STRINGS);
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif

typedef struct AMQP_TRANSPORT_INSTANCE_TAG
{
    STRING_HANDLE iothub_host_fqdn;                                     // FQDN of the IoT Hub.
    XIO_HANDLE tls_io;                                                  // TSL I/O transport.
    AMQP_GET_IO_TRANSPORT underlying_io_transport_provider;             // Pointer to the function that creates the TLS I/O (internal use only).
    AMQP_CONNECTION_HANDLE amqp_connection;                             // Base amqp connection with service.
    AMQP_CONNECTION_STATE amqp_connection_state;                        // Current state of the amqp_connection.
    AMQP_TRANSPORT_AUTHENTICATION_MODE preferred_authentication_mode;   // Used to avoid registered devices using different authentication modes.
    SINGLYLINKEDLIST_HANDLE registered_devices;                         // List of devices currently registered in this transport.
    bool is_trace_on;                                                   // Turns logging on and off.
    OPTIONHANDLER_HANDLE saved_tls_options;                             // Here are the options from the xio layer if any is saved.
    AMQP_TRANSPORT_STATE state;                                         // Current state of the transport.
    RETRY_CONTROL_HANDLE connection_retry_control;                      // Controls when the re-connection attempt should occur.
    size_t svc2cl_keep_alive_timeout_secs;                       // Service to device keep alive frequency
    double cl2svc_keep_alive_send_ratio;                                    // Client to service keep alive frequency

    char* http_proxy_hostname;
    int http_proxy_port;
    char* http_proxy_username;
    char* http_proxy_password;

    size_t option_cbs_request_timeout_secs;                             // Device-specific option.
    size_t option_send_event_timeout_secs;                              // Device-specific option.

                                                                        // Auth module used to generating handle authorization
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;                   // with either SAS Token, x509 Certs, and Device SAS Token

    TRANSPORT_CALLBACKS_INFO transport_callbacks;
    void* transport_ctx;
} AMQP_TRANSPORT_INSTANCE;

typedef struct AMQP_TRANSPORT_DEVICE_INSTANCE_TAG
{
    STRING_HANDLE device_id;                                            // Identity of the device.
    AMQP_DEVICE_HANDLE device_handle;                                   // Logic unit that performs authentication, messaging, etc.
    AMQP_TRANSPORT_INSTANCE* transport_instance;                        // Saved reference to the transport the device is registered on.
    PDLIST_ENTRY waiting_to_send;                                       // List of events waiting to be sent to the iot hub (i.e., haven't been processed by the transport yet).
    DEVICE_STATE device_state;                                          // Current state of the device_handle instance.
    size_t number_of_previous_failures;                                 // Number of times the device has failed in sequence; this value is reset to 0 if device succeeds to authenticate, send and/or recv messages.
    size_t number_of_send_event_complete_failures;                      // Number of times on_event_send_complete was called in row with an error.
    time_t time_of_last_state_change;                                   // Time the device_handle last changed state; used to track timeouts of amqp_device_start_async and amqp_device_stop.
    unsigned int max_state_change_timeout_secs;                         // Maximum number of seconds allowed for device_handle to complete start and stop state changes.
    // the methods portion
    IOTHUBTRANSPORT_AMQP_METHODS_HANDLE methods_handle;                 // Handle to instance of module that deals with device methods for AMQP.
    // is subscription for methods needed?
    bool subscribe_methods_needed;                                       // Indicates if should subscribe for device methods.
    // is the transport subscribed for methods?
    bool subscribed_for_methods;                                         // Indicates if device is subscribed for device methods.

    TRANSPORT_CALLBACKS_INFO transport_callbacks;
    void* transport_ctx;
} AMQP_TRANSPORT_DEVICE_INSTANCE;

typedef DEVICE_MESSAGE_DISPOSITION_INFO MESSAGE_DISPOSITION_CONTEXT;

typedef struct AMQP_TRANSPORT_DEVICE_TWIN_CONTEXT_TAG
{
    uint32_t item_id;
    TRANSPORT_CALLBACKS_INFO transport_callbacks;
    void* transport_ctx;
} AMQP_TRANSPORT_DEVICE_TWIN_CONTEXT;

typedef struct AMQP_TRANSPORT_GET_TWIN_CONTEXT_TAG
{
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK on_get_twin_completed_callback;
    void* user_context;
} AMQP_TRANSPORT_GET_TWIN_CONTEXT;

// ---------- General Helpers ---------- //

static void free_proxy_data(AMQP_TRANSPORT_INSTANCE* amqp_transport_instance)
{
    if (amqp_transport_instance->http_proxy_hostname != NULL)
    {
        free(amqp_transport_instance->http_proxy_hostname);
        amqp_transport_instance->http_proxy_hostname = NULL;
    }

    if (amqp_transport_instance->http_proxy_username != NULL)
    {
        free(amqp_transport_instance->http_proxy_username);
        amqp_transport_instance->http_proxy_username = NULL;
    }

    if (amqp_transport_instance->http_proxy_password != NULL)
    {
        free(amqp_transport_instance->http_proxy_password);
        amqp_transport_instance->http_proxy_password = NULL;
    }
}

static STRING_HANDLE get_target_iothub_fqdn(const IOTHUBTRANSPORT_CONFIG* config)
{
    STRING_HANDLE fqdn;

    if (config->upperConfig->protocolGatewayHostName == NULL)
    {
        if ((fqdn = STRING_construct_sprintf("%s.%s", config->upperConfig->iotHubName, config->upperConfig->iotHubSuffix)) == NULL)
        {
            LogError("Failed to copy iotHubName and iotHubSuffix (STRING_construct_sprintf failed)");
        }
    }
    else if ((fqdn = STRING_construct(config->upperConfig->protocolGatewayHostName)) == NULL)
    {
        LogError("Failed to copy protocolGatewayHostName (STRING_construct failed)");
    }

    return fqdn;
}

static void update_state(AMQP_TRANSPORT_INSTANCE* transport_instance, AMQP_TRANSPORT_STATE new_state)
{
    transport_instance->state = new_state;
}

static void reset_retry_control(AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device)
{
    retry_control_reset(registered_device->transport_instance->connection_retry_control);
}

// ---------- Register/Unregister Helpers ---------- //

static void internal_destroy_amqp_device_instance(AMQP_TRANSPORT_DEVICE_INSTANCE *trdev_inst)
{
    if (trdev_inst->methods_handle != NULL)
    {
        iothubtransportamqp_methods_destroy(trdev_inst->methods_handle);
    }

    if (trdev_inst->device_handle != NULL)
    {
        amqp_device_destroy(trdev_inst->device_handle);
    }

    if (trdev_inst->device_id != NULL)
    {
        STRING_delete(trdev_inst->device_id);
    }

    free(trdev_inst);
}

static DEVICE_AUTH_MODE get_authentication_mode(const IOTHUB_DEVICE_CONFIG* device)
{
    DEVICE_AUTH_MODE result;

    if (device->deviceKey != NULL || device->deviceSasToken != NULL)
    {
        result = DEVICE_AUTH_MODE_CBS;
    }
    else if (IoTHubClient_Auth_Get_Credential_Type(device->authorization_module) == IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH)
    {
        result = DEVICE_AUTH_MODE_CBS;
    }
    else
    {
        result = DEVICE_AUTH_MODE_X509;
    }
    return result;
}

static void raise_connection_status_callback_retry_expired(const void* item, const void* action_context, bool* continue_processing)
{
    (void)action_context;

    AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)item;

    registered_device->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED, registered_device->transport_ctx);

    *continue_processing = true;
}

// @brief
//     Saves the new state, if it is different than the previous one.
static void on_device_state_changed_callback(void* context, DEVICE_STATE previous_state, DEVICE_STATE new_state)
{
    if (context != NULL && new_state != previous_state)
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)context;
        registered_device->device_state = new_state;
        registered_device->time_of_last_state_change = get_time(NULL);

        if (new_state == DEVICE_STATE_STARTED)
        {
            reset_retry_control(registered_device);

            registered_device->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, registered_device->transport_ctx);
        }
        else if (new_state == DEVICE_STATE_STOPPED)
        {
            if (registered_device->transport_instance->state == AMQP_TRANSPORT_STATE_CONNECTED ||
                registered_device->transport_instance->state == AMQP_TRANSPORT_STATE_BEING_DESTROYED)
            {
                registered_device->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, registered_device->transport_ctx);
            }
        }
        else if (new_state == DEVICE_STATE_ERROR_AUTH)
        {
            registered_device->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL, registered_device->transport_ctx);
        }
        else if (new_state == DEVICE_STATE_ERROR_AUTH_TIMEOUT || new_state == DEVICE_STATE_ERROR_MSG)
        {
            registered_device->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, registered_device->transport_ctx);
        }
    }
}

// @brief    Auxiliary function to be used to find a device in the registered_devices list.
// @returns  true if the device ids match, false otherwise.
static bool find_device_by_id_callback(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    bool result;

    if (match_context == NULL)
    {
        result = false;
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* device_instance = (AMQP_TRANSPORT_DEVICE_INSTANCE*)singlylinkedlist_item_get_value(list_item);

        if (device_instance == NULL ||
            device_instance->device_id == NULL ||
            STRING_c_str(device_instance->device_id) != match_context)
        {
            result = false;
        }
        else
        {
            result = true;
        }
    }

    return result;
}

// @brief       Verifies if a device is already registered within the transport that owns the list of registered devices.
// @remarks     Returns the correspoding LIST_ITEM_HANDLE in registered_devices, if found.
// @returns     true if the device is already in the list, false otherwise.
static bool is_device_registered_ex(SINGLYLINKEDLIST_HANDLE registered_devices, const char* device_id, LIST_ITEM_HANDLE *list_item)
{
    return ((*list_item = singlylinkedlist_find(registered_devices, find_device_by_id_callback, device_id)) != NULL ? 1 : 0);
}

// @brief       Verifies if a device is already registered within the transport that owns the list of registered devices.
// @returns     true if the device is already in the list, false otherwise.
static bool is_device_registered(AMQP_TRANSPORT_DEVICE_INSTANCE* amqp_device_instance)
{
    if (amqp_device_instance == NULL)
    {
        LogError("AMQP_TRANSPORT_DEVICE_INSTANCE is NULL");
        return false;
    }
    else
    {
        LIST_ITEM_HANDLE list_item;
        const char* device_id = STRING_c_str(amqp_device_instance->device_id);
        return is_device_registered_ex(amqp_device_instance->transport_instance->registered_devices, device_id, &list_item);
    }
}

static size_t get_number_of_registered_devices(AMQP_TRANSPORT_INSTANCE* transport)
{
    size_t result = 0;

    LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(transport->registered_devices);

    while (list_item != NULL)
    {
        result++;
        list_item = singlylinkedlist_get_next_item(list_item);
    }

    return result;
}


// ---------- Callbacks ---------- //


static DEVICE_MESSAGE_DISPOSITION_RESULT get_device_disposition_result_from(IOTHUBMESSAGE_DISPOSITION_RESULT iothubclient_disposition_result)
{
    DEVICE_MESSAGE_DISPOSITION_RESULT device_disposition_result;

    if (iothubclient_disposition_result == IOTHUBMESSAGE_ACCEPTED)
    {
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED;
    }
    else if (iothubclient_disposition_result == IOTHUBMESSAGE_ABANDONED)
    {
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED;
    }
    else if (iothubclient_disposition_result == IOTHUBMESSAGE_REJECTED)
    {
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED;
    }
    else
    {
        LogError("Failed getting corresponding DEVICE_MESSAGE_DISPOSITION_RESULT for IOTHUBMESSAGE_DISPOSITION_RESULT (%d is not supported)", iothubclient_disposition_result);
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED;
    }

    return device_disposition_result;
}

static DEVICE_MESSAGE_DISPOSITION_RESULT on_message_received(IOTHUB_MESSAGE_HANDLE message, DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
{
    DEVICE_MESSAGE_DISPOSITION_RESULT device_disposition_result;
    MESSAGE_DISPOSITION_CONTEXT* dispositionContextClone;

    if ((dispositionContextClone = amqp_device_clone_message_disposition_info(disposition_info)) == NULL)
    {
        LogError("Failed processing message received (failed creating message disposition context)");
        IoTHubMessage_Destroy(message);
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED;
    }
    else if (IoTHubMessage_SetDispositionContext(
        message, 
        (MESSAGE_DISPOSITION_CONTEXT_HANDLE)dispositionContextClone, 
        (MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION)amqp_device_destroy_message_disposition_info) != IOTHUB_MESSAGE_OK)
    {
        LogError("Failed setting disposition context in IOTHUB_MESSAGE_HANDLE");
        amqp_device_destroy_message_disposition_info(dispositionContextClone);
        IoTHubMessage_Destroy(message);
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED;
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* amqp_device_instance = (AMQP_TRANSPORT_DEVICE_INSTANCE*)context;

        if (amqp_device_instance->transport_callbacks.msg_cb(message, amqp_device_instance->transport_ctx) != true)
        {
            LogError("Failed processing message received (IoTHubClientCore_LL_MessageCallback failed)");
            // This will destroy the disposition context as well.
            IoTHubMessage_Destroy(message);
            device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED;
        }
        else
        {
            device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_NONE;
        }
    }

    return device_disposition_result;
}

static void on_methods_error(void* context)
{
    (void)context;
}

static void on_methods_unsubscribed(void* context)
{
    AMQP_TRANSPORT_DEVICE_INSTANCE* device_state = (AMQP_TRANSPORT_DEVICE_INSTANCE*)context;

    iothubtransportamqp_methods_unsubscribe(device_state->methods_handle);
    device_state->subscribed_for_methods = false;
}

static int on_method_request_received(void* context, const char* method_name, const unsigned char* request, size_t request_size, IOTHUBTRANSPORT_AMQP_METHOD_HANDLE method_handle)
{
    int result;
    AMQP_TRANSPORT_DEVICE_INSTANCE* device_state = (AMQP_TRANSPORT_DEVICE_INSTANCE*)context;

    if (device_state->transport_callbacks.method_complete_cb(method_name, request, request_size, (void*)method_handle, device_state->transport_ctx) != 0)
    {
        LogError("Failure: IoTHubClientCore_LL_DeviceMethodComplete");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

static int subscribe_methods(AMQP_TRANSPORT_DEVICE_INSTANCE* deviceState)
{
    int result;

    if (deviceState->subscribed_for_methods)
    {
        result = 0;
    }
    else
    {
        SESSION_HANDLE session_handle;

        if ((amqp_connection_get_session_handle(deviceState->transport_instance->amqp_connection, &session_handle)) != RESULT_OK)
        {
            LogError("Device '%s' failed subscribing for methods (failed getting session handle)", STRING_c_str(deviceState->device_id));
            result = MU_FAILURE;
        }
        else if (iothubtransportamqp_methods_subscribe(deviceState->methods_handle, session_handle, on_methods_error, deviceState, on_method_request_received, deviceState, on_methods_unsubscribed, deviceState) != 0)
        {
            LogError("Cannot subscribe for methods");
            result = MU_FAILURE;
        }
        else
        {
            deviceState->subscribed_for_methods = true;
            result = 0;
        }
    }

    return result;
}

static void on_device_send_twin_update_complete_callback(DEVICE_TWIN_UPDATE_RESULT result, int status_code, void* context)
{
    (void)result;

    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_TWIN_CONTEXT* dev_twin_ctx = (AMQP_TRANSPORT_DEVICE_TWIN_CONTEXT*)context;

        dev_twin_ctx->transport_callbacks.twin_rpt_state_complete_cb(dev_twin_ctx->item_id, status_code, dev_twin_ctx->transport_ctx);

        free(dev_twin_ctx);
    }
}

static void on_device_twin_update_received_callback(DEVICE_TWIN_UPDATE_TYPE update_type, const unsigned char* message, size_t length, void* context)
{
    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)context;

        registered_device->transport_instance->transport_callbacks.twin_retrieve_prop_complete_cb((update_type == DEVICE_TWIN_UPDATE_TYPE_COMPLETE ? DEVICE_TWIN_UPDATE_COMPLETE : DEVICE_TWIN_UPDATE_PARTIAL),
                message, length, registered_device->transport_instance->transport_ctx);
    }
}

static void on_device_get_twin_completed_callback(DEVICE_TWIN_UPDATE_TYPE update_type, const unsigned char* message, size_t length, void* context)
{
    (void)update_type;

    if (context == NULL)
    {
        LogError("Invalid argument (message=%p, context=%p)", message, context);
    }
    else
    {
        AMQP_TRANSPORT_GET_TWIN_CONTEXT* getTwinCtx = (AMQP_TRANSPORT_GET_TWIN_CONTEXT*)context;

        getTwinCtx->on_get_twin_completed_callback(DEVICE_TWIN_UPDATE_COMPLETE, message, length, getTwinCtx->user_context);

        free(getTwinCtx);
    }
}


// ---------- Underlying TLS I/O Helpers ---------- //

// @brief
//     Retrieves the options of the current underlying TLS I/O instance and saves in the transport instance.
// @remarks
//     This is used when the new underlying I/O transport (TLS I/O, or WebSockets, etc) needs to be recreated,
//     and the options previously set must persist.
//
//     If no TLS I/O instance was created yet, results in failure.
// @returns
//     0 if succeeds, non-zero otherwise.
static int save_underlying_io_transport_options(AMQP_TRANSPORT_INSTANCE* transport_instance)
{
    int result;

    if (transport_instance->tls_io == NULL)
    {
        LogError("failed saving underlying I/O transport options (tls_io instance is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        OPTIONHANDLER_HANDLE fresh_options;

        if ((fresh_options = xio_retrieveoptions(transport_instance->tls_io)) == NULL)
        {
            LogError("failed saving underlying I/O transport options (tls_io instance is NULL)");
            result = MU_FAILURE;
        }
        else
        {
            OPTIONHANDLER_HANDLE previous_options = transport_instance->saved_tls_options;
            transport_instance->saved_tls_options = fresh_options;

            if (previous_options != NULL)
            {
                OptionHandler_Destroy(previous_options);
            }

            result = RESULT_OK;
        }
    }

    return result;
}

static void destroy_underlying_io_transport_options(AMQP_TRANSPORT_INSTANCE* transport_instance)
{
    if (transport_instance->saved_tls_options != NULL)
    {
        OptionHandler_Destroy(transport_instance->saved_tls_options);
        transport_instance->saved_tls_options = NULL;
    }
}

// @brief
//     Applies TLS I/O options if previously saved to a new TLS I/O instance.
// @returns
//     0 if succeeds, non-zero otherwise.
static int restore_underlying_io_transport_options(AMQP_TRANSPORT_INSTANCE* transport_instance, XIO_HANDLE xio_handle)
{
    int result;

    if (transport_instance->saved_tls_options == NULL)
    {
        result = RESULT_OK;
    }
    else
    {
        if (OptionHandler_FeedOptions(transport_instance->saved_tls_options, xio_handle) != OPTIONHANDLER_OK)
        {
            LogError("Failed feeding existing options to new TLS instance.");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

// @brief    Destroys the XIO_HANDLE obtained with underlying_io_transport_provider(), saving its options beforehand.
static void destroy_underlying_io_transport(AMQP_TRANSPORT_INSTANCE* transport_instance)
{
    if (transport_instance->tls_io != NULL)
    {
        xio_destroy(transport_instance->tls_io);
        transport_instance->tls_io = NULL;
    }
}

// @brief    Invokes underlying_io_transport_provider() and retrieves a new XIO_HANDLE to use for I/O (TLS, or websockets, or w/e is supported).
// @param    xio_handle: if successfull, set with the new XIO_HANDLE acquired; not changed otherwise.
// @returns  0 if successfull, non-zero otherwise.
static int get_new_underlying_io_transport(AMQP_TRANSPORT_INSTANCE* transport_instance, XIO_HANDLE *xio_handle)
{
    int result;
    AMQP_TRANSPORT_PROXY_OPTIONS amqp_transport_proxy_options;

    amqp_transport_proxy_options.host_address = transport_instance->http_proxy_hostname;
    amqp_transport_proxy_options.port = transport_instance->http_proxy_port;
    amqp_transport_proxy_options.username = transport_instance->http_proxy_username;
    amqp_transport_proxy_options.password = transport_instance->http_proxy_password;

    if ((*xio_handle = transport_instance->underlying_io_transport_provider(STRING_c_str(transport_instance->iothub_host_fqdn), amqp_transport_proxy_options.host_address == NULL ? NULL : &amqp_transport_proxy_options)) == NULL)
    {
        LogError("Failed to obtain a TLS I/O transport layer (underlying_io_transport_provider() failed)");
        result = MU_FAILURE;
    }
    else
    {
        // If this is the HSM x509 ECC certificate
        if (transport_instance->authorization_module != NULL && IoTHubClient_Auth_Get_Credential_Type(transport_instance->authorization_module) == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
        {
            // Set the xio_handle
            if (IoTHubClient_Auth_Set_xio_Certificate(transport_instance->authorization_module, *xio_handle) != 0)
            {
                LogError("Unable to create the lower level TLS layer.");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else
        {
            result = RESULT_OK;
        }

        if (restore_underlying_io_transport_options(transport_instance, *xio_handle) != RESULT_OK)
        {
            /*pessimistically hope TLS will fail, be recreated and options re-given*/
            LogError("Failed to apply options previous saved to new underlying I/O transport instance.");
        }
    }

    return result;
}


// ---------- AMQP connection establishment/tear-down, connectry retry ---------- //

static void on_amqp_connection_state_changed(const void* context, AMQP_CONNECTION_STATE previous_state, AMQP_CONNECTION_STATE new_state)
{
    if (context != NULL && new_state != previous_state)
    {
        AMQP_TRANSPORT_INSTANCE* transport_instance = (AMQP_TRANSPORT_INSTANCE*)context;

        transport_instance->amqp_connection_state = new_state;

        if (new_state == AMQP_CONNECTION_STATE_ERROR)
        {
            LogError("Transport received an ERROR from the amqp_connection (state changed %s -> %s); it will be flagged for connection retry.", MU_ENUM_TO_STRING(AMQP_CONNECTION_STATE, previous_state), MU_ENUM_TO_STRING(AMQP_CONNECTION_STATE, new_state));
            transport_instance->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_NETWORK, transport_instance->transport_ctx);
            update_state(transport_instance, AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED);
        }
        else if (new_state == AMQP_CONNECTION_STATE_OPENED)
        {
            update_state(transport_instance, AMQP_TRANSPORT_STATE_CONNECTED);
        }
        else if (new_state == AMQP_CONNECTION_STATE_CLOSED && previous_state == AMQP_CONNECTION_STATE_OPENED && transport_instance->state != AMQP_TRANSPORT_STATE_BEING_DESTROYED)
        {
            LogError("amqp_connection was closed unexpectedly; connection retry will be triggered.");

            update_state(transport_instance, AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED);
        }
    }
}

static int establish_amqp_connection(AMQP_TRANSPORT_INSTANCE* transport_instance)
{
    int result;

    if (transport_instance->preferred_authentication_mode == AMQP_TRANSPORT_AUTHENTICATION_MODE_NOT_SET)
    {
        LogError("Failed establishing connection (transport doesn't have a preferred authentication mode set; unexpected!).");
        result = MU_FAILURE;
    }
    else if (transport_instance->tls_io == NULL &&
        get_new_underlying_io_transport(transport_instance, &transport_instance->tls_io) != RESULT_OK)
    {
        LogError("Failed establishing connection (failed to obtain a TLS I/O transport layer).");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_CONNECTION_CONFIG amqp_connection_config;
        amqp_connection_config.iothub_host_fqdn = STRING_c_str(transport_instance->iothub_host_fqdn);
        amqp_connection_config.underlying_io_transport = transport_instance->tls_io;
        amqp_connection_config.is_trace_on = transport_instance->is_trace_on;
        amqp_connection_config.on_state_changed_callback = on_amqp_connection_state_changed;
        amqp_connection_config.on_state_changed_context = transport_instance;
        amqp_connection_config.svc2cl_keep_alive_timeout_secs = transport_instance->svc2cl_keep_alive_timeout_secs;
        amqp_connection_config.cl2svc_keep_alive_send_ratio = transport_instance->cl2svc_keep_alive_send_ratio;

        if (transport_instance->preferred_authentication_mode == AMQP_TRANSPORT_AUTHENTICATION_MODE_CBS)
        {
            amqp_connection_config.create_sasl_io = true;
            amqp_connection_config.create_cbs_connection = true;
        }
        else if (transport_instance->preferred_authentication_mode == AMQP_TRANSPORT_AUTHENTICATION_MODE_X509)
        {
            amqp_connection_config.create_sasl_io = false;
            amqp_connection_config.create_cbs_connection = false;
        }
        // If new AMQP_TRANSPORT_AUTHENTICATION_MODE values are added, they need to be covered here.

        transport_instance->amqp_connection_state = AMQP_CONNECTION_STATE_CLOSED;

        if (transport_instance->state == AMQP_TRANSPORT_STATE_READY_FOR_RECONNECTION)
        {
            update_state(transport_instance, AMQP_TRANSPORT_STATE_RECONNECTING);
        }
        else
        {
            update_state(transport_instance, AMQP_TRANSPORT_STATE_CONNECTING);
        }

        if ((transport_instance->amqp_connection = amqp_connection_create(&amqp_connection_config)) == NULL)
        {
            LogError("Failed establishing connection (failed to create the amqp_connection instance).");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

static void prepare_device_for_connection_retry(AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device)
{
    iothubtransportamqp_methods_unsubscribe(registered_device->methods_handle);
    registered_device->subscribed_for_methods = 0;

    if (registered_device->device_state != DEVICE_STATE_STOPPED)
    {
        if (amqp_device_stop(registered_device->device_handle) != RESULT_OK)
        {
            LogError("Failed preparing device '%s' for connection retry (amqp_device_stop failed)", STRING_c_str(registered_device->device_id));
        }
    }

    registered_device->number_of_previous_failures = 0;
    registered_device->number_of_send_event_complete_failures = 0;
}

void prepare_for_connection_retry(AMQP_TRANSPORT_INSTANCE* transport_instance)
{
    LogInfo("Preparing transport for re-connection");

    if (save_underlying_io_transport_options(transport_instance) != RESULT_OK)
    {
        LogError("Failed saving TLS I/O options while preparing for connection retry; failure will be ignored");
    }

    LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(transport_instance->registered_devices);

    while (list_item != NULL)
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)singlylinkedlist_item_get_value(list_item);

        if (registered_device == NULL)
        {
            LogError("Failed preparing device for connection retry (singlylinkedlist_item_get_value failed)");
        }
        else
        {
            prepare_device_for_connection_retry(registered_device);
        }

        list_item = singlylinkedlist_get_next_item(list_item);
    }

    amqp_connection_destroy(transport_instance->amqp_connection);
    transport_instance->amqp_connection = NULL;
    transport_instance->amqp_connection_state = AMQP_CONNECTION_STATE_CLOSED;

    destroy_underlying_io_transport(transport_instance);

    update_state(transport_instance, AMQP_TRANSPORT_STATE_READY_FOR_RECONNECTION);
}


// @brief    Verifies if the credentials used by the device match the requirements and authentication mode currently supported by the transport.
// @returns  true if credentials are good, false otherwise.
static bool is_device_credential_acceptable(const IOTHUB_DEVICE_CONFIG* device_config, AMQP_TRANSPORT_AUTHENTICATION_MODE preferred_authentication_mode)
{
    bool result;

    if ((device_config->deviceSasToken != NULL) && (device_config->deviceKey != NULL))
    {
        LogError("Credential of device '%s' is not acceptable (must provide EITHER deviceSasToken OR deviceKey)", device_config->deviceId);
        result = false;
    }
    else if (preferred_authentication_mode == AMQP_TRANSPORT_AUTHENTICATION_MODE_NOT_SET)
    {
        result = true;
    }
    else if (preferred_authentication_mode == AMQP_TRANSPORT_AUTHENTICATION_MODE_X509 && (device_config->deviceKey != NULL || device_config->deviceSasToken != NULL))
    {
        LogError("Credential of device '%s' is not acceptable (transport is using X509 certificate authentication, but device config contains deviceKey or sasToken)", device_config->deviceId);
        result = false;
    }
    else if (preferred_authentication_mode != AMQP_TRANSPORT_AUTHENTICATION_MODE_X509 && (device_config->deviceKey == NULL && device_config->deviceSasToken == NULL))
    {
        LogError("Credential of device '%s' is not acceptable (transport is using CBS authentication, but device config does not contain deviceKey nor sasToken)", device_config->deviceId);
        result = false;
    }
    else
    {
        result = true;
    }

    return result;
}



//---------- DoWork Helpers ----------//

static IOTHUB_MESSAGE_LIST* get_next_event_to_send(AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device)
{
    IOTHUB_MESSAGE_LIST* message;

    if (!DList_IsListEmpty(registered_device->waiting_to_send))
    {
        PDLIST_ENTRY list_entry = registered_device->waiting_to_send->Flink;
        message = containingRecord(list_entry, IOTHUB_MESSAGE_LIST, entry);
        (void)DList_RemoveEntryList(list_entry);
    }
    else
    {
        message = NULL;
    }

    return message;
}

// @brief    "Parses" the D2C_EVENT_SEND_RESULT (from iothubtransport_amqp_device module) into a IOTHUB_CLIENT_CONFIRMATION_RESULT.
static IOTHUB_CLIENT_CONFIRMATION_RESULT get_iothub_client_confirmation_result_from(D2C_EVENT_SEND_RESULT result)
{
    IOTHUB_CLIENT_CONFIRMATION_RESULT iothub_send_result;

    switch (result)
    {
        case D2C_EVENT_SEND_COMPLETE_RESULT_OK:
            iothub_send_result = IOTHUB_CLIENT_CONFIRMATION_OK;
            break;
        case D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE:
        case D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING:
            iothub_send_result = IOTHUB_CLIENT_CONFIRMATION_ERROR;
            break;
        case D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT:
            iothub_send_result = IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT;
            break;
        case D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED:
            iothub_send_result = IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY;
            break;
        case D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_UNKNOWN:
        default:
            iothub_send_result = IOTHUB_CLIENT_CONFIRMATION_ERROR;
            break;
    }

    return iothub_send_result;
}

// @brief
//     Callback function for amqp_device_send_event_async.
static void on_event_send_complete(IOTHUB_MESSAGE_LIST* message, D2C_EVENT_SEND_RESULT result, void* context)
{
    AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)context;

    if (result != D2C_EVENT_SEND_COMPLETE_RESULT_OK && result != D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED)
    {
        registered_device->number_of_send_event_complete_failures++;
    }
    else
    {
        registered_device->number_of_send_event_complete_failures = 0;
    }

    if (message->callback != NULL)
    {
        IOTHUB_CLIENT_CONFIRMATION_RESULT iothub_send_result = get_iothub_client_confirmation_result_from(result);

        message->callback(iothub_send_result, message->context);
    }

    IoTHubMessage_Destroy(message->messageHandle);
    free(message);
}

// @brief
//     Gets events from wait to send list and sends to service in the order they were added.
// @returns
//     0 if all events could be sent to the next layer successfully, non-zero otherwise.
static int send_pending_events(AMQP_TRANSPORT_DEVICE_INSTANCE* device_state)
{
    int result;
    IOTHUB_MESSAGE_LIST* message;

    result = RESULT_OK;

    while ((message = get_next_event_to_send(device_state)) != NULL)
    {
        if (amqp_device_send_event_async(device_state->device_handle, message, on_event_send_complete, device_state) != RESULT_OK)
        {
            LogError("Device '%s' failed to send message (amqp_device_send_event_async failed)", STRING_c_str(device_state->device_id));
            result = MU_FAILURE;

            on_event_send_complete(message, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING, device_state);
            break;
        }
    }

    return result;
}

// @brief
//     Auxiliary function for the public DoWork API, performing DoWork activities (authenticate, messaging) for a specific device.
// @requires
//     The transport to have a valid instance of AMQP_CONNECTION (from which to obtain SESSION_HANDLE and CBS_HANDLE)
// @returns
//     0 if no errors occur, non-zero otherwise.
static int IoTHubTransport_AMQP_Common_Device_DoWork(AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device)
{
    int result;

    if (registered_device->device_state != DEVICE_STATE_STARTED)
    {
        if (registered_device->device_state == DEVICE_STATE_STOPPED)
        {
            SESSION_HANDLE session_handle;
            CBS_HANDLE cbs_handle = NULL;

            if (amqp_connection_get_session_handle(registered_device->transport_instance->amqp_connection, &session_handle) != RESULT_OK)
            {
                LogError("Failed performing DoWork for device '%s' (failed to get the amqp_connection session_handle)", STRING_c_str(registered_device->device_id));
                result = MU_FAILURE;
            }
            else if (registered_device->transport_instance->preferred_authentication_mode == AMQP_TRANSPORT_AUTHENTICATION_MODE_CBS &&
                amqp_connection_get_cbs_handle(registered_device->transport_instance->amqp_connection, &cbs_handle) != RESULT_OK)
            {
                LogError("Failed performing DoWork for device '%s' (failed to get the amqp_connection cbs_handle)", STRING_c_str(registered_device->device_id));
                result = MU_FAILURE;
            }
            else if (amqp_device_start_async(registered_device->device_handle, session_handle, cbs_handle) != RESULT_OK)
            {
                LogError("Failed performing DoWork for device '%s' (failed to start device)", STRING_c_str(registered_device->device_id));
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else if (registered_device->device_state == DEVICE_STATE_STARTING ||
                 registered_device->device_state == DEVICE_STATE_STOPPING)
        {
            bool is_timed_out;
            if (is_timeout_reached(registered_device->time_of_last_state_change, registered_device->max_state_change_timeout_secs, &is_timed_out) != RESULT_OK)
            {
                LogError("Failed performing DoWork for device '%s' (failed tracking timeout of device %d state)", STRING_c_str(registered_device->device_id), registered_device->device_state);
                registered_device->device_state = DEVICE_STATE_ERROR_AUTH; // if time could not be calculated, the worst must be assumed.
                result = MU_FAILURE;
            }
            else if (is_timed_out)
            {
                LogError("Failed performing DoWork for device '%s' (device failed to start or stop within expected timeout)", STRING_c_str(registered_device->device_id));
                registered_device->device_state = DEVICE_STATE_ERROR_AUTH; // this will cause device to be stopped bellow on the next call to this function.
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else // i.e., DEVICE_STATE_ERROR_AUTH || DEVICE_STATE_ERROR_AUTH_TIMEOUT || DEVICE_STATE_ERROR_MSG
        {
            registered_device->number_of_previous_failures++;

            if (registered_device->number_of_previous_failures >= MAX_NUMBER_OF_DEVICE_FAILURES)
            {
                LogError("Failed performing DoWork for device '%s' (device reported state %d; number of previous failures: %lu)",
                    STRING_c_str(registered_device->device_id), (int)registered_device->device_state, (unsigned long)registered_device->number_of_previous_failures);
                result = MU_FAILURE;
            }
            else if (amqp_device_delayed_stop(registered_device->device_handle, DEFAULT_DEVICE_STOP_DELAY) != RESULT_OK)
            {
                LogError("Failed to stop reset device '%s' (amqp_device_stop failed)", STRING_c_str(registered_device->device_id));
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
    }
    else if (registered_device->subscribe_methods_needed &&
        !registered_device->subscribed_for_methods &&
        subscribe_methods(registered_device) != RESULT_OK)
    {
        LogError("Failed performing DoWork for device '%s' (failed registering for device methods)", STRING_c_str(registered_device->device_id));
        registered_device->number_of_previous_failures++;
        result = MU_FAILURE;
    }
    else
    {
        if (send_pending_events(registered_device) != RESULT_OK)
        {
            LogError("Failed performing DoWork for device '%s' (failed sending pending events)", STRING_c_str(registered_device->device_id));
            registered_device->number_of_previous_failures++;
            result = MU_FAILURE;
        }
        else
        {
            registered_device->number_of_previous_failures = 0;
            result = RESULT_OK;
        }
    }

    // No harm in invoking this as API will simply exit if the state is not "started".
    amqp_device_do_work(registered_device->device_handle);

    return result;
}


//---------- SetOption-ish Helpers ----------//

// @brief
//     Gets all the device-specific options and replicates them into this new registered device.
// @returns
//     0 if the function succeeds, non-zero otherwise.
static int replicate_device_options_to(AMQP_TRANSPORT_DEVICE_INSTANCE* dev_instance, DEVICE_AUTH_MODE auth_mode)
{
    int result;

    if (amqp_device_set_option(
        dev_instance->device_handle,
        DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS,
        &dev_instance->transport_instance->option_send_event_timeout_secs) != RESULT_OK)
    {
        LogError("Failed to apply option DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS to device '%s' (amqp_device_set_option failed)", STRING_c_str(dev_instance->device_id));
        result = MU_FAILURE;
    }
    else if (auth_mode == DEVICE_AUTH_MODE_CBS)
    {
        if (amqp_device_set_option(
            dev_instance->device_handle,
            DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS,
            &dev_instance->transport_instance->option_cbs_request_timeout_secs) != RESULT_OK)
        {
            LogError("Failed to apply option DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS to device '%s' (amqp_device_set_option failed)", STRING_c_str(dev_instance->device_id));
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

// @brief
//     Translates from the option names supported by iothubtransport_amqp_common to the ones supported by iothubtransport_amqp_device.
static const char* get_device_option_name_from(const char* iothubclient_option_name)
{
    const char* device_option_name;

    if (strcmp(OPTION_CBS_REQUEST_TIMEOUT, iothubclient_option_name) == 0)
    {
        device_option_name = DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS;
    }
    else if (strcmp(OPTION_EVENT_SEND_TIMEOUT_SECS, iothubclient_option_name) == 0)
    {
        device_option_name = DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS;
    }
    else
    {
        device_option_name = NULL;
    }

    return device_option_name;
}

// @brief
//     Auxiliary function invoked by IoTHubTransport_AMQP_Common_SetOption to set an option on every registered device.
// @returns
//     0 if it succeeds, non-zero otherwise.
static int IoTHubTransport_AMQP_Common_Device_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, void* value)
{
    int result;
    const char* device_option;

    if ((device_option = get_device_option_name_from(option)) == NULL)
    {
        LogError("failed setting option '%s' to registered device (could not match name to options supported by device)", option);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_TRANSPORT_INSTANCE* instance = (AMQP_TRANSPORT_INSTANCE*)handle;
        result = RESULT_OK;

        LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(instance->registered_devices);

        while (list_item != NULL)
        {
            AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device;

            if ((registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)singlylinkedlist_item_get_value(list_item)) == NULL)
            {
                LogError("failed setting option '%s' to registered device (singlylinkedlist_item_get_value failed)", option);
                result = MU_FAILURE;
                break;
            }
            else if (amqp_device_set_option(registered_device->device_handle, device_option, value) != RESULT_OK)
            {
                LogError("failed setting option '%s' to registered device '%s' (amqp_device_set_option failed)",
                    option, STRING_c_str(registered_device->device_id));
                result = MU_FAILURE;
                break;
            }

            list_item = singlylinkedlist_get_next_item(list_item);
        }
    }

    return result;
}

static void internal_destroy_instance(AMQP_TRANSPORT_INSTANCE* instance)
{
    if (instance != NULL)
    {
        update_state(instance, AMQP_TRANSPORT_STATE_BEING_DESTROYED);

        if (instance->registered_devices != NULL)
        {
            LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(instance->registered_devices);

            while (list_item != NULL)
            {
                AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)singlylinkedlist_item_get_value(list_item);
                list_item = singlylinkedlist_get_next_item(list_item);
                IoTHubTransport_AMQP_Common_Unregister(registered_device);
            }

            singlylinkedlist_destroy(instance->registered_devices);
        }

        if (instance->amqp_connection != NULL)
        {
            amqp_connection_destroy(instance->amqp_connection);
        }

        destroy_underlying_io_transport(instance);
        destroy_underlying_io_transport_options(instance);
        retry_control_destroy(instance->connection_retry_control);

        STRING_delete(instance->iothub_host_fqdn);

        /* SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_043: [ `IoTHubTransport_AMQP_Common_Destroy` shall free the stored proxy options. ]*/
        free_proxy_data(instance);

        free(instance);
    }
}


// ---------- API functions ---------- //

TRANSPORT_LL_HANDLE IoTHubTransport_AMQP_Common_Create(const IOTHUBTRANSPORT_CONFIG* config, AMQP_GET_IO_TRANSPORT get_io_transport, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    TRANSPORT_LL_HANDLE result;

    if (config == NULL || config->upperConfig == NULL || get_io_transport == NULL || cb_info == NULL)
    {
        LogError("IoTHub AMQP client transport null configuration parameter (config=%p, get_io_transport=%p, cb_info=%p).", config, get_io_transport, cb_info);
        result = NULL;
    }
    else if (config->upperConfig->protocol == NULL)
    {
        LogError("Failed to create the AMQP transport common instance (NULL parameter received: protocol)");
        result = NULL;
    }
    else if (IoTHub_Transport_ValidateCallbacks(cb_info) != 0)
    {
        LogError("failure checking transport callbacks");
        result = NULL;
    }
    else
    {
        AMQP_TRANSPORT_INSTANCE* instance;

        if ((instance = (AMQP_TRANSPORT_INSTANCE*)malloc(sizeof(AMQP_TRANSPORT_INSTANCE))) == NULL)
        {
            LogError("Could not allocate AMQP transport state (malloc failed)");
            result = NULL;
        }
        else
        {
            memset(instance, 0, sizeof(AMQP_TRANSPORT_INSTANCE));
            instance->amqp_connection_state = AMQP_CONNECTION_STATE_CLOSED;
            instance->preferred_authentication_mode = AMQP_TRANSPORT_AUTHENTICATION_MODE_NOT_SET;
            instance->state = AMQP_TRANSPORT_STATE_NOT_CONNECTED;
            instance->authorization_module = config->auth_module_handle;

            if ((instance->connection_retry_control = retry_control_create(DEFAULT_RETRY_POLICY, DEFAULT_MAX_RETRY_TIME_IN_SECS)) == NULL)
            {
                LogError("Failed to create the connection retry control.");
                result = NULL;
            }
            else if ((instance->iothub_host_fqdn = get_target_iothub_fqdn(config)) == NULL)
            {
                LogError("Failed to obtain the iothub target fqdn.");
                result = NULL;
            }
            else if ((instance->registered_devices = singlylinkedlist_create()) == NULL)
            {
                LogError("Failed to initialize the internal list of registered devices (singlylinkedlist_create failed)");
                result = NULL;
            }
            else
            {
                instance->underlying_io_transport_provider = get_io_transport;
                instance->is_trace_on = false;
                instance->option_cbs_request_timeout_secs = DEFAULT_CBS_REQUEST_TIMEOUT_SECS;
                instance->option_send_event_timeout_secs = DEFAULT_EVENT_SEND_TIMEOUT_SECS;
                instance->svc2cl_keep_alive_timeout_secs = DEFAULT_SERVICE_KEEP_ALIVE_FREQ_SECS;
                instance->cl2svc_keep_alive_send_ratio = DEFAULT_REMOTE_IDLE_PING_RATIO;

                instance->transport_ctx = ctx;
                instance->transport_callbacks.msg_input_cb = cb_info->msg_input_cb;
                instance->transport_callbacks.msg_cb = cb_info->msg_cb;
                instance->transport_callbacks.connection_status_cb = cb_info->connection_status_cb;
                instance->transport_callbacks.send_complete_cb = cb_info->send_complete_cb;
                instance->transport_callbacks.prod_info_cb = cb_info->prod_info_cb;
                instance->transport_callbacks.twin_rpt_state_complete_cb = cb_info->twin_rpt_state_complete_cb;
                instance->transport_callbacks.twin_retrieve_prop_complete_cb = cb_info->twin_retrieve_prop_complete_cb;
                instance->transport_callbacks.method_complete_cb = cb_info->method_complete_cb;

                result = (TRANSPORT_LL_HANDLE)instance;
            }

            if (result == NULL)
            {
                internal_destroy_instance(instance);
            }
        }
    }

    return result;
}

IOTHUB_PROCESS_ITEM_RESULT IoTHubTransport_AMQP_Common_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
{
    IOTHUB_PROCESS_ITEM_RESULT result;

    if (handle == NULL || iothub_item == NULL)
    {
        LogError("Invalid argument (handle=%p, iothub_item=%p)", handle, iothub_item);
        result = IOTHUB_PROCESS_ERROR;
    }
    else
    {
        if (item_type == IOTHUB_TYPE_DEVICE_TWIN)
        {
            AMQP_TRANSPORT_DEVICE_TWIN_CONTEXT* dev_twin_ctx;

            if ((dev_twin_ctx = (AMQP_TRANSPORT_DEVICE_TWIN_CONTEXT*)malloc(sizeof(AMQP_TRANSPORT_DEVICE_TWIN_CONTEXT))) == NULL)
            {
                LogError("Failed allocating context for TWIN message");
                result = IOTHUB_PROCESS_ERROR;
            }
            else
            {
                AMQP_TRANSPORT_INSTANCE* transport_instance = (AMQP_TRANSPORT_INSTANCE*)handle;

                AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)iothub_item->device_twin->device_handle;

                dev_twin_ctx->transport_callbacks = transport_instance->transport_callbacks;
                dev_twin_ctx->transport_ctx = transport_instance->transport_ctx;
                dev_twin_ctx->item_id = iothub_item->device_twin->item_id;

                if (amqp_device_send_twin_update_async(
                    registered_device->device_handle,
                    iothub_item->device_twin->report_data_handle,
                    on_device_send_twin_update_complete_callback, (void*)dev_twin_ctx) != RESULT_OK)
                {
                    LogError("Failed sending TWIN update");
                    free(dev_twin_ctx);
                    result = IOTHUB_PROCESS_ERROR;
                }
                else
                {
                    result = IOTHUB_PROCESS_OK;
                }
            }
        }
        else
        {
            LogError("Item type not supported (%d)", item_type);
            result = IOTHUB_PROCESS_ERROR;
        }
    }

    return result;
}

void IoTHubTransport_AMQP_Common_DoWork(TRANSPORT_LL_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("IoTHubClient DoWork failed: transport handle parameter is NULL.");
    }
    else
    {
        AMQP_TRANSPORT_INSTANCE* transport_instance = (AMQP_TRANSPORT_INSTANCE*)handle;
        LIST_ITEM_HANDLE list_item;

        if (transport_instance->state == AMQP_TRANSPORT_STATE_NOT_CONNECTED_NO_MORE_RETRIES)
        {
            // Nothing to be done.
        }
        else if (transport_instance->state == AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED)
        {
            RETRY_ACTION retry_action;

            if (retry_control_should_retry(transport_instance->connection_retry_control, &retry_action) != RESULT_OK)
            {
                LogError("retry_control_should_retry() failed; assuming immediate connection retry for safety.");
                retry_action = RETRY_ACTION_RETRY_NOW;
            }

            if (retry_action == RETRY_ACTION_RETRY_NOW)
            {
                prepare_for_connection_retry(transport_instance);
            }
            else if (retry_action == RETRY_ACTION_STOP_RETRYING)
            {
                update_state(transport_instance, AMQP_TRANSPORT_STATE_NOT_CONNECTED_NO_MORE_RETRIES);

                (void)singlylinkedlist_foreach(transport_instance->registered_devices, raise_connection_status_callback_retry_expired, NULL);
            }
        }
        else
        {
            if ((list_item = singlylinkedlist_get_head_item(transport_instance->registered_devices)) != NULL)
            {
                // We need to check if there are devices, otherwise the amqp_connection won't be able to be created since
                // there is not a preferred authentication mode set yet on the transport.

                if (transport_instance->amqp_connection == NULL && establish_amqp_connection(transport_instance) != RESULT_OK)
                {
                    LogError("AMQP transport failed to establish connection with service.");

                    update_state(transport_instance, AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED);
                }
                else if (transport_instance->amqp_connection_state == AMQP_CONNECTION_STATE_OPENED)
                {
                    size_t number_of_devices = 0;
                    size_t number_of_faulty_devices = 0;

                    while (list_item != NULL)
                    {
                        AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device;

                        if ((registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)singlylinkedlist_item_get_value(list_item)) == NULL)
                        {
                            LogError("Transport had an unexpected failure during DoWork (failed to fetch a registered_devices list item value)");
                        }
                        else if (registered_device->number_of_send_event_complete_failures >= DEVICE_FAILURE_COUNT_RECONNECTION_THRESHOLD)
                        {
                            number_of_faulty_devices++;
                        }
                        else if (IoTHubTransport_AMQP_Common_Device_DoWork(registered_device) != RESULT_OK)
                        {
                            if (registered_device->number_of_previous_failures >= DEVICE_FAILURE_COUNT_RECONNECTION_THRESHOLD)
                            {
                                number_of_faulty_devices++;
                            }
                        }

                        list_item = singlylinkedlist_get_next_item(list_item);
                        number_of_devices++;
                    }

                    if (number_of_faulty_devices > 0 &&
                        ((float)number_of_faulty_devices/(float)number_of_devices) >= DEVICE_MULTIPLEXING_FAULTY_DEVICE_RATIO_RECONNECTION_THRESHOLD)
                    {
                        LogError("Reconnection required. %zd of %zd registered devices are failing.", number_of_faulty_devices, number_of_devices);

                        update_state(transport_instance, AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED);
                    }
                }
            }

            if (transport_instance->amqp_connection != NULL)
            {
                amqp_connection_do_work(transport_instance->amqp_connection);
            }
        }
    }
}

int IoTHubTransport_AMQP_Common_Subscribe(IOTHUB_DEVICE_HANDLE handle)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalid handle to IoTHubClient AMQP transport device handle.");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* amqp_device_instance = (AMQP_TRANSPORT_DEVICE_INSTANCE*)handle;

        if (!is_device_registered(amqp_device_instance))
        {
            LogError("Device '%s' failed subscribing to cloud-to-device messages (device is not registered)", STRING_c_str(amqp_device_instance->device_id));
            result = MU_FAILURE;
        }
        else if (amqp_device_subscribe_message(amqp_device_instance->device_handle, on_message_received, amqp_device_instance) != RESULT_OK)
        {
            LogError("Device '%s' failed subscribing to cloud-to-device messages (amqp_device_subscribe_message failed)", STRING_c_str(amqp_device_instance->device_id));
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

void IoTHubTransport_AMQP_Common_Unsubscribe(IOTHUB_DEVICE_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("Invalid handle to IoTHubClient AMQP transport device handle.");
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* amqp_device_instance = (AMQP_TRANSPORT_DEVICE_INSTANCE*)handle;

        if (!is_device_registered(amqp_device_instance))
        {
            LogError("Device '%s' failed unsubscribing to cloud-to-device messages (device is not registered)", STRING_c_str(amqp_device_instance->device_id));
        }
        else if (amqp_device_unsubscribe_message(amqp_device_instance->device_handle) != RESULT_OK)
        {
            LogError("Device '%s' failed unsubscribing to cloud-to-device messages (amqp_device_unsubscribe_message failed)", STRING_c_str(amqp_device_instance->device_id));
        }
    }
}

int IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_TRANSPORT_INSTANCE* transport = (AMQP_TRANSPORT_INSTANCE*)handle;

        if (get_number_of_registered_devices(transport) != 1)
        {
            LogError("Device Twin not supported on device multiplexing scenario");
            result = MU_FAILURE;
        }
        else
        {
            LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(transport->registered_devices);

            result = RESULT_OK;

            while (list_item != NULL)
            {
                AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device;

                if ((registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)singlylinkedlist_item_get_value(list_item)) == NULL)
                {
                    LogError("Failed retrieving registered device information");
                    result = MU_FAILURE;
                    break;
                }
                else if (amqp_device_subscribe_for_twin_updates(registered_device->device_handle, on_device_twin_update_received_callback, (void*)registered_device) != RESULT_OK)
                {
                    LogError("Failed subscribing for device Twin updates");
                    result = MU_FAILURE;
                    break;
                }

                list_item = singlylinkedlist_get_next_item(list_item);
            }
        }
    }

    return result;
}

void IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL");
    }
    else
    {
        AMQP_TRANSPORT_INSTANCE* transport = (AMQP_TRANSPORT_INSTANCE*)handle;

        if (get_number_of_registered_devices(transport) != 1)
        {
            LogError("Device Twin not supported on device multiplexing scenario");
        }
        else
        {
            LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(transport->registered_devices);

            while (list_item != NULL)
            {
                AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device;

                if ((registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)singlylinkedlist_item_get_value(list_item)) == NULL)
                {
                    LogError("Failed retrieving registered device information");
                    break;
                }
                else if (amqp_device_unsubscribe_for_twin_updates(registered_device->device_handle) != RESULT_OK)
                {
                    LogError("Failed unsubscribing for device Twin updates");
                    break;
                }

                list_item = singlylinkedlist_get_next_item(list_item);
            }
        }
    }
}

IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_GetTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
{
    (void)callbackContext;

    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL || completionCallback == NULL)
    {
        LogError("Invalid argument (handle=%p, completionCallback=%p)", handle, completionCallback);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)handle;

        if (get_number_of_registered_devices(registered_device->transport_instance) != 1)
        {
            LogError("Device Twin not supported on device multiplexing scenario");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            AMQP_TRANSPORT_GET_TWIN_CONTEXT* getTwinCtx;

            if ((getTwinCtx = malloc(sizeof(AMQP_TRANSPORT_GET_TWIN_CONTEXT))) == NULL)
            {
                LogError("Failed creating context for get twin");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                getTwinCtx->on_get_twin_completed_callback = completionCallback;
                getTwinCtx->user_context = callbackContext;

                if (amqp_device_get_twin_async(registered_device->device_handle, on_device_get_twin_completed_callback, (void*)getTwinCtx) != RESULT_OK)
                {
                    LogError("Failed subscribing for device Twin updates");
                    free(getTwinCtx);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
    }

    return result;
}

int IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    int result;

    if (handle == NULL)
    {
        LogError("NULL handle");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* device_state = (AMQP_TRANSPORT_DEVICE_INSTANCE*)handle;
        device_state->subscribe_methods_needed = true;
        device_state->subscribed_for_methods = false;
        result = 0;
    }

    return result;
}

void IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("NULL handle");
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* device_state = (AMQP_TRANSPORT_DEVICE_INSTANCE*)handle;

        if (device_state->subscribe_methods_needed)
        {
            device_state->subscribed_for_methods = false;
            device_state->subscribe_methods_needed = false;
            iothubtransportamqp_methods_unsubscribe(device_state->methods_handle);
        }
    }
}

int IoTHubTransport_AMQP_Common_DeviceMethod_Response(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    (void)response;
    (void)response_size;
    (void)status_response;
    (void)methodId;
    int result;
    AMQP_TRANSPORT_DEVICE_INSTANCE* device_state = (AMQP_TRANSPORT_DEVICE_INSTANCE*)handle;
    if (device_state != NULL)
    {
        IOTHUBTRANSPORT_AMQP_METHOD_HANDLE saved_handle = (IOTHUBTRANSPORT_AMQP_METHOD_HANDLE)methodId;
        if (iothubtransportamqp_methods_respond(saved_handle, response, response_size, status_response) != 0)
        {
            LogError("iothubtransportamqp_methods_respond failed");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        result = MU_FAILURE;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_GetSendStatus(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL || iotHubClientStatus == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("Failed retrieving the device send status (either handle (%p) or iotHubClientStatus (%p) are NULL)", handle, iotHubClientStatus);
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* amqp_device_state = (AMQP_TRANSPORT_DEVICE_INSTANCE*)handle;

        DEVICE_SEND_STATUS device_send_status;

        if (!DList_IsListEmpty(amqp_device_state->waiting_to_send))
        {
            *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_BUSY;
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            if (amqp_device_get_send_status(amqp_device_state->device_handle, &device_send_status) != RESULT_OK)
            {
                LogError("Failed retrieving the device send status (amqp_device_get_send_status failed)");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (device_send_status == DEVICE_SEND_STATUS_BUSY)
                {
                    *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_BUSY;
                }
                else // DEVICE_SEND_STATUS_IDLE
                {
                    *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_IDLE;
                }

                result = IOTHUB_CLIENT_OK;
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
{
    IOTHUB_CLIENT_RESULT result;

    if ((handle == NULL) || (option == NULL) || (value == NULL))
    {
        LogError("Invalid parameter (NULL) passed to AMQP transport SetOption (handle=%p, options=%p, value=%p)", handle, option, value);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        AMQP_TRANSPORT_INSTANCE* transport_instance = (AMQP_TRANSPORT_INSTANCE*)handle;
        bool is_device_specific_option;

        if (strcmp(OPTION_CBS_REQUEST_TIMEOUT, option) == 0)
        {
            is_device_specific_option = true;
            transport_instance->option_cbs_request_timeout_secs = *(size_t*)value;
        }
        else if (strcmp(OPTION_EVENT_SEND_TIMEOUT_SECS, option) == 0)
        {
            is_device_specific_option = true;
            transport_instance->option_send_event_timeout_secs = *(size_t*)value;
        }
        else
        {
            is_device_specific_option = false;
        }

        if (is_device_specific_option)
        {
            if (IoTHubTransport_AMQP_Common_Device_SetOption(handle, option, (void*)value) != RESULT_OK)
            {
                LogError("transport failed setting option '%s' (failed setting option on one or more registered devices)", option);
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else if (strcmp(OPTION_RETRY_INTERVAL_SEC, option) == 0)
        {
            if (retry_control_set_option(transport_instance->connection_retry_control, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, value) != 0)
            {
                LogError("Failure setting retry interval option");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else if (strcmp(OPTION_RETRY_MAX_DELAY_SECS, option) == 0)
        {
            if (retry_control_set_option(transport_instance->connection_retry_control, RETRY_CONTROL_OPTION_MAX_DELAY_IN_SECS, value) != 0)
            {
                LogError("Failure setting retry max delay option");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else if ((strcmp(OPTION_SERVICE_SIDE_KEEP_ALIVE_FREQ_SECS, option) == 0) || (strcmp(OPTION_C2D_KEEP_ALIVE_FREQ_SECS, option) == 0))
        {
            transport_instance->svc2cl_keep_alive_timeout_secs = *(size_t*)value;
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(OPTION_REMOTE_IDLE_TIMEOUT_RATIO, option) == 0)
        {

            if ((*(double*)value <= 0.0) || (*(double*)value >= MAX_SERVICE_KEEP_ALIVE_RATIO))
            {
                LogError("Invalid remote idle ratio %lf", *(double*) value);
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                transport_instance->cl2svc_keep_alive_send_ratio = *(double*)value; // override the default and set the user configured remote idle ratio value
                result = IOTHUB_CLIENT_OK;
            }

        }
        else if (strcmp(OPTION_LOG_TRACE, option) == 0)
        {
            transport_instance->is_trace_on = *((bool*)value);

            if (transport_instance->amqp_connection != NULL &&
                amqp_connection_set_logging(transport_instance->amqp_connection, transport_instance->is_trace_on) != RESULT_OK)
            {
                LogError("transport failed setting option '%s' (amqp_connection_set_logging failed)", option);
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else if (strcmp(OPTION_HTTP_PROXY, option) == 0)
        {
            HTTP_PROXY_OPTIONS* proxy_options = (HTTP_PROXY_OPTIONS*)value;

            if (transport_instance->tls_io != NULL)
            {
                LogError("Cannot set proxy option once the underlying IO is created");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (proxy_options->host_address == NULL)
            {
                LogError("NULL host_address in proxy options");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else if (((proxy_options->username == NULL) || (proxy_options->password == NULL)) &&
                (proxy_options->username != proxy_options->password))
            {
                LogError("Only one of username and password for proxy settings was NULL");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                char* copied_proxy_hostname = NULL;
                char* copied_proxy_username = NULL;
                char* copied_proxy_password = NULL;

                transport_instance->http_proxy_port = proxy_options->port;
                if (mallocAndStrcpy_s(&copied_proxy_hostname, proxy_options->host_address) != 0)
                {
                    LogError("Cannot copy HTTP proxy hostname");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if ((proxy_options->username != NULL) && (mallocAndStrcpy_s(&copied_proxy_username, proxy_options->username) != 0))
                {
                    free(copied_proxy_hostname);
                    LogError("Cannot copy HTTP proxy username");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if ((proxy_options->password != NULL) && (mallocAndStrcpy_s(&copied_proxy_password, proxy_options->password) != 0))
                {
                    if (copied_proxy_username != NULL)
                    {
                        free(copied_proxy_username);
                    }
                    free(copied_proxy_hostname);
                    LogError("Cannot copy HTTP proxy password");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    free_proxy_data(transport_instance);

                    transport_instance->http_proxy_hostname = copied_proxy_hostname;
                    transport_instance->http_proxy_username = copied_proxy_username;
                    transport_instance->http_proxy_password = copied_proxy_password;

                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else
        {
            result = IOTHUB_CLIENT_OK;

            if (strcmp(OPTION_X509_CERT, option) == 0 || strcmp(OPTION_X509_PRIVATE_KEY, option) == 0)
            {
                if (transport_instance->preferred_authentication_mode == AMQP_TRANSPORT_AUTHENTICATION_MODE_NOT_SET)
                {
                    transport_instance->preferred_authentication_mode = AMQP_TRANSPORT_AUTHENTICATION_MODE_X509;
                }
                else if (transport_instance->preferred_authentication_mode != AMQP_TRANSPORT_AUTHENTICATION_MODE_X509)
                {
                    LogError("transport failed setting option '%s' (preferred authentication method is not x509)", option);
                    result = IOTHUB_CLIENT_INVALID_ARG;
                }
                else
                {
                    IoTHubClient_Auth_Set_x509_Type(transport_instance->authorization_module, true);
                }
            }

            if (result != IOTHUB_CLIENT_INVALID_ARG)
            {
                if (transport_instance->tls_io == NULL &&
                    get_new_underlying_io_transport(transport_instance, &transport_instance->tls_io) != RESULT_OK)
                {
                    LogError("transport failed setting option '%s' (failed to obtain a TLS I/O transport).", option);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if (xio_setoption(transport_instance->tls_io, option, value) != RESULT_OK)
                {
                    LogError("transport failed setting option '%s' (xio_setoption failed)", option);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    if (save_underlying_io_transport_options(transport_instance) != RESULT_OK)
                    {
                        LogError("IoTHubTransport_AMQP_Common_SetOption failed to save underlying I/O options; failure will be ignored");
                    }

                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
    }

    return result;
}

IOTHUB_DEVICE_HANDLE IoTHubTransport_AMQP_Common_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend)
{
    IOTHUB_DEVICE_HANDLE result;

    if ((handle == NULL) || (device == NULL) || (waitingToSend == NULL))
    {
        LogError("invalid parameter TRANSPORT_LL_HANDLE handle=%p, const IOTHUB_DEVICE_CONFIG* device=%p, PDLIST_ENTRY waiting_to_send=%p",
            handle, device, waitingToSend);
        result = NULL;
    }
    else if (device->deviceId == NULL)
    {
        LogError("Transport failed to register device (device_id provided is NULL)");
        result = NULL;
    }
    else
    {
        LIST_ITEM_HANDLE list_item;
        AMQP_TRANSPORT_INSTANCE* transport_instance = (AMQP_TRANSPORT_INSTANCE*)handle;

        if (is_device_registered_ex(transport_instance->registered_devices, device->deviceId, &list_item))
        {
            LogError("IoTHubTransport_AMQP_Common_Register failed (device '%s' already registered on this transport instance)", device->deviceId);
            result = NULL;
        }
        else if (!is_device_credential_acceptable(device, transport_instance->preferred_authentication_mode))
        {
            LogError("Transport failed to register device '%s' (device credential was not accepted)", device->deviceId);
            result = NULL;
        }
        else
        {
            AMQP_TRANSPORT_DEVICE_INSTANCE* amqp_device_instance;

            if ((amqp_device_instance = (AMQP_TRANSPORT_DEVICE_INSTANCE*)malloc(sizeof(AMQP_TRANSPORT_DEVICE_INSTANCE))) == NULL)
            {
                LogError("Transport failed to register device '%s' (failed to create the device state instance; malloc failed)", device->deviceId);
                result = NULL;
            }
            else
            {
                memset(amqp_device_instance, 0, sizeof(AMQP_TRANSPORT_DEVICE_INSTANCE));

                amqp_device_instance->transport_instance = transport_instance;
                amqp_device_instance->waiting_to_send = waitingToSend;
                amqp_device_instance->device_state = DEVICE_STATE_STOPPED;
                amqp_device_instance->max_state_change_timeout_secs = DEFAULT_DEVICE_STATE_CHANGE_TIMEOUT_SECS;
                amqp_device_instance->subscribe_methods_needed = false;
                amqp_device_instance->subscribed_for_methods = false;
                amqp_device_instance->transport_ctx = transport_instance->transport_ctx;
                amqp_device_instance->transport_callbacks = transport_instance->transport_callbacks;

                if ((amqp_device_instance->device_id = STRING_construct(device->deviceId)) == NULL)
                {
                    LogError("Transport failed to register device '%s' (failed to copy the deviceId)", device->deviceId);
                    result = NULL;
                }
                else
                {
                    AMQP_DEVICE_CONFIG device_config;
                    (void)memset(&device_config, 0, sizeof(AMQP_DEVICE_CONFIG));
                    device_config.iothub_host_fqdn = (char*)STRING_c_str(transport_instance->iothub_host_fqdn);
                    device_config.authorization_module = device->authorization_module;

                    device_config.authentication_mode = get_authentication_mode(device);
                    device_config.on_state_changed_callback = on_device_state_changed_callback;
                    device_config.on_state_changed_context = amqp_device_instance;
                    device_config.prod_info_cb = transport_instance->transport_callbacks.prod_info_cb;
                    device_config.prod_info_ctx = transport_instance->transport_ctx;

                    if ((amqp_device_instance->device_handle = amqp_device_create(&device_config)) == NULL)
                    {
                        LogError("Transport failed to register device '%s' (failed to create the DEVICE_HANDLE instance)", device->deviceId);
                        result = NULL;
                    }
                    else
                    {
                        bool is_first_device_being_registered = (singlylinkedlist_get_head_item(transport_instance->registered_devices) == NULL);

                        amqp_device_instance->methods_handle = iothubtransportamqp_methods_create(STRING_c_str(transport_instance->iothub_host_fqdn), device->deviceId, device->moduleId);
                        if (amqp_device_instance->methods_handle == NULL)
                        {
                            LogError("Transport failed to register device '%s' (Cannot create the methods module)", device->deviceId);
                            result = NULL;
                        }
                        else
                        {
                            if (replicate_device_options_to(amqp_device_instance, device_config.authentication_mode) != RESULT_OK)
                            {
                                LogError("Transport failed to register device '%s' (failed to replicate options)", device->deviceId);
                                result = NULL;
                            }
                            else if (singlylinkedlist_add(transport_instance->registered_devices, amqp_device_instance) == NULL)
                            {
                                LogError("Transport failed to register device '%s' (singlylinkedlist_add failed)", device->deviceId);
                                result = NULL;
                            }
                            else
                            {
                                if (transport_instance->preferred_authentication_mode == AMQP_TRANSPORT_AUTHENTICATION_MODE_NOT_SET &&
                                    is_first_device_being_registered)
                                {
                                    if (device_config.authentication_mode == DEVICE_AUTH_MODE_CBS)
                                    {
                                        transport_instance->preferred_authentication_mode = AMQP_TRANSPORT_AUTHENTICATION_MODE_CBS;
                                    }
                                    else
                                    {
                                        transport_instance->preferred_authentication_mode = AMQP_TRANSPORT_AUTHENTICATION_MODE_X509;
                                    }
                                }

                                result = (IOTHUB_DEVICE_HANDLE)amqp_device_instance;
                            }
                        }
                    }
                }

                if (result == NULL)
                {
                    internal_destroy_amqp_device_instance(amqp_device_instance);
                }
            }
        }
    }

    return result;
}

void IoTHubTransport_AMQP_Common_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    if (deviceHandle == NULL)
    {
        LogError("Failed to unregister device (deviceHandle is NULL).");
    }
    else
    {
        AMQP_TRANSPORT_DEVICE_INSTANCE* registered_device = (AMQP_TRANSPORT_DEVICE_INSTANCE*)deviceHandle;
        const char* device_id;
        LIST_ITEM_HANDLE list_item;

        if ((device_id = STRING_c_str(registered_device->device_id)) == NULL)
        {
            LogError("Failed to unregister device (failed to get device id char ptr)");
        }
        else if (registered_device->transport_instance == NULL)
        {
            LogError("Failed to unregister device '%s' (deviceHandle does not have a transport state associated to).", device_id);
        }
        else if (!is_device_registered_ex(registered_device->transport_instance->registered_devices, device_id, &list_item))
        {
            LogError("Failed to unregister device '%s' (device is not registered within this transport).", device_id);
        }
        else
        {
            // Removing it first so the race hazard is reduced between this function and DoWork. Best would be to use locks.
            if (singlylinkedlist_remove(registered_device->transport_instance->registered_devices, list_item) != RESULT_OK)
            {
                LogError("Failed to unregister device '%s' (singlylinkedlist_remove failed).", device_id);
            }
            else
            {
                internal_destroy_amqp_device_instance(registered_device);
            }
        }
    }
}

void IoTHubTransport_AMQP_Common_Destroy(TRANSPORT_LL_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("Failed to destroy AMQP transport instance (handle is NULL)");
    }
    else
    {
        internal_destroy_instance((AMQP_TRANSPORT_INSTANCE*)handle);
    }
}

int IoTHubTransport_AMQP_Common_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    int result;

    if (handle == NULL)
    {
        LogError("Cannot set retry policy (transport handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        RETRY_CONTROL_HANDLE new_retry_control;

        if ((new_retry_control = retry_control_create(retryPolicy, (unsigned int)retryTimeoutLimitInSeconds)) == NULL)
        {
            LogError("Cannot set retry policy (retry_control_create failed)");
            result = MU_FAILURE;
        }
        else
        {
            AMQP_TRANSPORT_INSTANCE* transport_instance = (AMQP_TRANSPORT_INSTANCE*)handle;
            RETRY_CONTROL_HANDLE previous_retry_control = transport_instance->connection_retry_control;

            transport_instance->connection_retry_control = new_retry_control;

            retry_control_destroy(previous_retry_control);

            if (transport_instance->state == AMQP_TRANSPORT_STATE_NOT_CONNECTED_NO_MORE_RETRIES)
            {
                transport_instance->state = AMQP_TRANSPORT_STATE_RECONNECTION_REQUIRED;
            }

            result = RESULT_OK;
        }
    }

    return result;
}

STRING_HANDLE IoTHubTransport_AMQP_Common_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    STRING_HANDLE result;

    if (handle == NULL)
    {
        LogError("Cannot provide the target host name (transport handle is NULL).");

        result = NULL;
    }
    else if ((result = STRING_clone(((AMQP_TRANSPORT_INSTANCE*)(handle))->iothub_host_fqdn)) == NULL)
    {
        LogError("Cannot provide the target host name (STRING_clone failed).");
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_SendMessageDisposition(IOTHUB_DEVICE_HANDLE handle, IOTHUB_MESSAGE_HANDLE message_handle, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    IOTHUB_CLIENT_RESULT result;
    if (message_handle == NULL || handle == NULL)
    {
        LogError("Failed sending message disposition (handle=%p, message_handle=%p)", handle, message_handle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        MESSAGE_DISPOSITION_CONTEXT_HANDLE device_message_disposition_info;

        if (IoTHubMessage_GetDispositionContext(message_handle, &device_message_disposition_info) != IOTHUB_MESSAGE_OK ||
            device_message_disposition_info == NULL)
        {
            LogError("Invalid message handle (no disposition context found)");
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
        else
        {
            AMQP_TRANSPORT_DEVICE_INSTANCE* amqp_device_instance = (AMQP_TRANSPORT_DEVICE_INSTANCE*)handle;

            DEVICE_MESSAGE_DISPOSITION_RESULT device_disposition_result = get_device_disposition_result_from(disposition);

            if (amqp_device_send_message_disposition(amqp_device_instance->device_handle, (DEVICE_MESSAGE_DISPOSITION_INFO*)device_message_disposition_info, device_disposition_result) != RESULT_OK)
            {
                LogError("Device '%s' failed sending message disposition (amqp_device_send_message_disposition failed)", STRING_c_str(amqp_device_instance->device_id));
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
    }

    IoTHubMessage_Destroy(message_handle);

    return result;
}

int IoTHubTransport_AMQP_SetCallbackContext(TRANSPORT_LL_HANDLE handle, void* ctx)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid parameter specified handle: %p", handle);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_TRANSPORT_INSTANCE* instance = (AMQP_TRANSPORT_INSTANCE*)handle;
        instance->transport_ctx = ctx;
        result = 0;
    }
    return result;
}

int IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(TRANSPORT_LL_HANDLE handle, PLATFORM_INFO_OPTION* info)
{
    int result;

    if (handle == NULL || info == NULL)
    {
        LogError("Invalid parameter specified (handle: %p, info: %p)", handle, info);
        result = MU_FAILURE;
    }
    else
    {
        *info = PLATFORM_INFO_OPTION_RETRIEVE_SQM;
        result = 0;
    }

    return result;
}