// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "internal/iothubtransport_amqp_cbs_auth.h"
#include "internal/iothubtransport_amqp_device.h"
#include "internal/iothubtransport_amqp_telemetry_messenger.h"
#include "internal/iothubtransport_amqp_twin_messenger.h"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_STATE, DEVICE_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_AUTH_MODE, DEVICE_AUTH_MODE_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_SEND_STATUS, DEVICE_SEND_STATUS_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(D2C_EVENT_SEND_RESULT, D2C_EVENT_SEND_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_MESSAGE_DISPOSITION_RESULT, DEVICE_MESSAGE_DISPOSITION_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_TWIN_UPDATE_RESULT, DEVICE_TWIN_UPDATE_RESULT_STRINGS);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_TWIN_UPDATE_TYPE, DEVICE_TWIN_UPDATE_TYPE_STRINGS)

#define RESULT_OK                                  0
#define INDEFINITE_TIME                            ((time_t)-1)
#define DEFAULT_AUTH_STATE_CHANGED_TIMEOUT_SECS    60
#define DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS    60

static const char* DEVICE_OPTION_SAVED_AUTH_OPTIONS = "saved_device_auth_options";
static const char* DEVICE_OPTION_SAVED_MESSENGER_OPTIONS = "saved_device_messenger_options";

typedef struct DEVICE_INSTANCE_TAG
{
    AMQP_DEVICE_CONFIG* config;
    DEVICE_STATE state;

    SESSION_HANDLE session_handle;
    CBS_HANDLE cbs_handle;

    AUTHENTICATION_HANDLE authentication_handle;
    AUTHENTICATION_STATE auth_state;
    AUTHENTICATION_ERROR_CODE auth_error_code;
    time_t auth_state_last_changed_time;
    size_t auth_state_change_timeout_secs;

    TELEMETRY_MESSENGER_HANDLE messenger_handle;
    TELEMETRY_MESSENGER_STATE msgr_state;
    time_t msgr_state_last_changed_time;
    size_t msgr_state_change_timeout_secs;

    ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback;
    void* on_message_received_context;

    TWIN_MESSENGER_HANDLE twin_messenger_handle;
    TWIN_MESSENGER_STATE twin_msgr_state;
    time_t twin_msgr_state_last_changed_time;
    size_t twin_msgr_state_change_timeout_secs;
    DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK on_device_twin_update_received_callback;
    void* on_device_twin_update_received_context;

    TICK_COUNTER_HANDLE tick_counter_handle;
    tickcounter_ms_t last_stop_request_time;
    size_t stop_delay_ms;
} AMQP_DEVICE_INSTANCE;

typedef struct DEVICE_SEND_EVENT_TASK_TAG
{
    ON_DEVICE_D2C_EVENT_SEND_COMPLETE on_event_send_complete_callback;
    void* on_event_send_complete_context;
} DEVICE_SEND_EVENT_TASK;

typedef struct DEVICE_SEND_TWIN_UPDATE_CONTEXT_TAG
{
    DEVICE_SEND_TWIN_UPDATE_COMPLETE_CALLBACK on_send_twin_update_complete_callback;
    void* context;
} DEVICE_SEND_TWIN_UPDATE_CONTEXT;

typedef struct DEVICE_GET_TWIN_CONTEXT_TAG
{
    DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK on_get_twin_completed_callback;
    void* context;
} DEVICE_GET_TWIN_CONTEXT;

// Internal state control
static void update_state(AMQP_DEVICE_INSTANCE* instance, DEVICE_STATE new_state)
{
    if (new_state != instance->state)
    {
        DEVICE_STATE previous_state = instance->state;
        instance->state = new_state;

        if (instance->config->on_state_changed_callback != NULL)
        {
            instance->config->on_state_changed_callback(instance->config->on_state_changed_context, previous_state, new_state);
        }
    }
}

static int is_timeout_reached(time_t start_time, size_t timeout_in_secs, int *is_timed_out)
{
    int result;

    if (start_time == INDEFINITE_TIME)
    {
        LogError("Failed to verify timeout (start_time is INDEFINITE)");
        result = MU_FAILURE;
    }
    else
    {
        time_t current_time;

        if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            LogError("Failed to verify timeout (get_time failed)");
            result = MU_FAILURE;
        }
        else
        {
            if (get_difftime(current_time, start_time) >= timeout_in_secs)
            {
                *is_timed_out = 1;
            }
            else
            {
                *is_timed_out = 0;
            }

            result = RESULT_OK;
        }
    }

    return result;
}


//---------- Callback Handlers ----------//

static D2C_EVENT_SEND_RESULT get_d2c_event_send_result_from(TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT result)
{
    D2C_EVENT_SEND_RESULT d2c_esr;

    switch (result)
    {
        case TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK:
            d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_OK;
            break;
        case TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE:
            d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE;
            break;
        case TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING:
            d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING;
            break;
        case TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT:
            d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT;
            break;
        case TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED:
            d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED;
            break;
        default:
            // This is not expected. All states should be mapped.
            d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_UNKNOWN;
            break;
    };

    return d2c_esr;
}

static void on_event_send_complete_messenger_callback(IOTHUB_MESSAGE_LIST* iothub_message, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT ev_send_comp_result, void* context)
{
    DEVICE_SEND_EVENT_TASK* send_task = (DEVICE_SEND_EVENT_TASK*)context;
    if (iothub_message == NULL || send_task == NULL)
    {
        LogError("on_event_send_complete_messenger_callback was invoked, but either iothub_message (%p) or context (%p) are NULL", iothub_message, send_task);
    }
    else
    {
        D2C_EVENT_SEND_RESULT device_send_result = get_d2c_event_send_result_from(ev_send_comp_result);

        if (send_task->on_event_send_complete_callback != NULL)
        {
            send_task->on_event_send_complete_callback(iothub_message, device_send_result, send_task->on_event_send_complete_context);
        }
    }

    if (send_task)
    {
        free(send_task);
    }
}

static void on_authentication_error_callback(void* context, AUTHENTICATION_ERROR_CODE error_code)
{
    if (context == NULL)
    {
        LogError("on_authentication_error_callback was invoked with error %d, but context is NULL", error_code);
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)context;
        instance->auth_error_code = error_code;
    }
}

static void on_authentication_state_changed_callback(void* context, AUTHENTICATION_STATE previous_state, AUTHENTICATION_STATE new_state)
{
    if (context == NULL)
    {
        LogError("on_authentication_state_changed_callback was invoked with new_state %d, but context is NULL", new_state);
    }
    else if (new_state != previous_state)
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)context;
        instance->auth_state = new_state;

        if ((instance->auth_state_last_changed_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            LogError("Device '%s' failed to set time of last authentication state change (get_time failed)", instance->config->device_id);
        }
    }
}

static void on_messenger_state_changed_callback(void* context, TELEMETRY_MESSENGER_STATE previous_state, TELEMETRY_MESSENGER_STATE new_state)
{
    if (context == NULL)
    {
        LogError("on_messenger_state_changed_callback was invoked with new_state %d, but context is NULL", new_state);
    }
    else if (new_state != previous_state)
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)context;
        instance->msgr_state = new_state;

        if ((instance->msgr_state_last_changed_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            LogError("Device '%s' failed to set time of last messenger state change (get_time failed)", instance->config->device_id);
        }
    }
}

static DEVICE_TWIN_UPDATE_RESULT get_device_twin_update_result_from(TWIN_REPORT_STATE_RESULT result)
{
    DEVICE_TWIN_UPDATE_RESULT device_result;

    switch (result)
    {
    case TWIN_REPORT_STATE_RESULT_SUCCESS:
        device_result = DEVICE_TWIN_UPDATE_RESULT_OK;
        break;
    case TWIN_REPORT_STATE_RESULT_ERROR:
        device_result = DEVICE_TWIN_UPDATE_RESULT_ERROR;
        break;
    default:
        device_result = DEVICE_TWIN_UPDATE_RESULT_ERROR;
    };

    return device_result;
}

static void on_report_state_complete_callback(TWIN_REPORT_STATE_RESULT result, TWIN_REPORT_STATE_REASON reason, int status_code, const void* context)
{
    (void)reason;

    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else
    {
        DEVICE_SEND_TWIN_UPDATE_CONTEXT* twin_ctx = (DEVICE_SEND_TWIN_UPDATE_CONTEXT*)context;

        if (twin_ctx->on_send_twin_update_complete_callback != NULL)
        {
            DEVICE_TWIN_UPDATE_RESULT device_result;

            device_result = get_device_twin_update_result_from(result);

            twin_ctx->on_send_twin_update_complete_callback(device_result, status_code, twin_ctx->context);
        }

        free(twin_ctx);
    }
}

static void on_twin_state_update_callback(TWIN_UPDATE_TYPE update_type, const char* payload, size_t size, const void* context)
{
    if (payload == NULL || context == NULL)
    {
        LogError("Invalid argument (context=%p, payload=%p)", context, payload);
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)context;

        DEVICE_TWIN_UPDATE_TYPE device_update_type;

        if (update_type == TWIN_UPDATE_TYPE_COMPLETE)
        {
            device_update_type = DEVICE_TWIN_UPDATE_TYPE_COMPLETE;
        }
        else
        {
            device_update_type = DEVICE_TWIN_UPDATE_TYPE_PARTIAL;
        }

        instance->on_device_twin_update_received_callback(device_update_type, (const unsigned char*)payload, size, instance->on_device_twin_update_received_context);
    }
}

static void on_twin_messenger_state_changed_callback(void* context, TWIN_MESSENGER_STATE previous_state, TWIN_MESSENGER_STATE new_state)
{
    if (context != NULL && new_state != previous_state)
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)context;
        instance->twin_msgr_state = new_state;

        if ((instance->twin_msgr_state_last_changed_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            LogError("Failed setting time of last twin messenger state changed event");
        }
    }
}

static void on_get_twin_completed(TWIN_UPDATE_TYPE update_type, const char* payload, size_t size, const void* context)
{
    (void)update_type;

    if (context == NULL)
    {
        LogError("Invalid argument (context=%p, payload=%p)", context, payload);
    }
    else
    {
        DEVICE_GET_TWIN_CONTEXT* twin_ctx = (DEVICE_GET_TWIN_CONTEXT*)context;

        // get-twin-async always returns a complete twin json.
        twin_ctx->on_get_twin_completed_callback(DEVICE_TWIN_UPDATE_TYPE_COMPLETE, (const unsigned char*)payload, size, twin_ctx->context);
        free(twin_ctx);
    }
}

//---------- Message Dispostion ----------//

static DEVICE_MESSAGE_DISPOSITION_INFO* create_device_message_disposition_info_from(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* messenger_disposition_info)
{
    DEVICE_MESSAGE_DISPOSITION_INFO* device_disposition_info;

    if ((device_disposition_info = (DEVICE_MESSAGE_DISPOSITION_INFO*)malloc(sizeof(DEVICE_MESSAGE_DISPOSITION_INFO))) == NULL)
    {
        LogError("Failed creating DEVICE_MESSAGE_DISPOSITION_INFO (malloc failed)");
    }
    else if (mallocAndStrcpy_s(&device_disposition_info->source, messenger_disposition_info->source) != RESULT_OK)
    {
        LogError("Failed creating DEVICE_MESSAGE_DISPOSITION_INFO (mallocAndStrcpy_s failed)");
        free(device_disposition_info);
        device_disposition_info = NULL;
    }
    else
    {
        device_disposition_info->message_id = messenger_disposition_info->message_id;
    }

    return device_disposition_info;
}

static void destroy_device_disposition_info(DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info)
{
    free(disposition_info->source);
    free(disposition_info);
}

static TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* create_messenger_disposition_info(DEVICE_MESSAGE_DISPOSITION_INFO* device_disposition_info)
{
    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* messenger_disposition_info;

    if ((messenger_disposition_info = (TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO*)malloc(sizeof(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO))) == NULL)
    {
        LogError("Failed creating TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO (malloc failed)");
    }
    else if (mallocAndStrcpy_s(&messenger_disposition_info->source, device_disposition_info->source) != RESULT_OK)
    {
        LogError("Failed creating TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO (mallocAndStrcpy_s failed)");
        free(messenger_disposition_info);
        messenger_disposition_info = NULL;
    }
    else
    {
        messenger_disposition_info->message_id = (delivery_number)device_disposition_info->message_id;
    }

    return messenger_disposition_info;
}

static void destroy_messenger_disposition_info(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* messenger_disposition_info)
{
    free(messenger_disposition_info->source);
    free(messenger_disposition_info);
}

static TELEMETRY_MESSENGER_DISPOSITION_RESULT get_messenger_message_disposition_result_from(DEVICE_MESSAGE_DISPOSITION_RESULT device_disposition_result)
{
    TELEMETRY_MESSENGER_DISPOSITION_RESULT messenger_disposition_result;

    switch (device_disposition_result)
    {
        case DEVICE_MESSAGE_DISPOSITION_RESULT_NONE:
            messenger_disposition_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_NONE;
            break;
        case DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED:
            messenger_disposition_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED;
            break;
        case DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED:
            messenger_disposition_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED;
            break;
        case DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED:
            messenger_disposition_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED;
            break;
        default:
            LogError("Failed to get the corresponding TELEMETRY_MESSENGER_DISPOSITION_RESULT (%d is not supported)", device_disposition_result);
            messenger_disposition_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED;
            break;
    }

    return messenger_disposition_result;
}

static TELEMETRY_MESSENGER_DISPOSITION_RESULT on_messenger_message_received_callback(IOTHUB_MESSAGE_HANDLE iothub_message_handle, TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
{
    TELEMETRY_MESSENGER_DISPOSITION_RESULT msgr_disposition_result;

    if (iothub_message_handle == NULL || context == NULL)
    {
        LogError("Failed receiving incoming C2D message (message handle (%p) or context (%p) is NULL)", iothub_message_handle, context);
        msgr_disposition_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* device_instance = (AMQP_DEVICE_INSTANCE*)context;

        if (device_instance->on_message_received_callback == NULL)
        {
            LogError("Device '%s' failed receiving incoming C2D message (callback is NULL)", device_instance->config->device_id);
            msgr_disposition_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED;
        }
        else
        {
            DEVICE_MESSAGE_DISPOSITION_INFO* device_message_disposition_info;

            if ((device_message_disposition_info = create_device_message_disposition_info_from(disposition_info)) == NULL)
            {
                LogError("Device '%s' failed receiving incoming C2D message (failed creating DEVICE_MESSAGE_DISPOSITION_INFO)", device_instance->config->device_id);
                msgr_disposition_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED;
            }
            else
            {
                DEVICE_MESSAGE_DISPOSITION_RESULT device_disposition_result = device_instance->on_message_received_callback(iothub_message_handle, device_message_disposition_info, device_instance->on_message_received_context);

                msgr_disposition_result = get_messenger_message_disposition_result_from(device_disposition_result);

                destroy_device_disposition_info(device_message_disposition_info);
            }
        }
    }

    return msgr_disposition_result;
}


//---------- Configuration Helpers ----------//

static void destroy_device_config(AMQP_DEVICE_CONFIG* config)
{
    if (config != NULL)
    {
        free(config->iothub_host_fqdn);
        free(config);
    }
}

static AMQP_DEVICE_CONFIG* clone_device_config(AMQP_DEVICE_CONFIG *config)
{
    AMQP_DEVICE_CONFIG* new_config;

    if ((new_config = (AMQP_DEVICE_CONFIG*)malloc(sizeof(AMQP_DEVICE_CONFIG))) == NULL)
    {
        LogError("Failed copying the AMQP_DEVICE_CONFIG (malloc failed)");
    }
    else
    {
        int result;
        memset(new_config, 0, sizeof(AMQP_DEVICE_CONFIG));

        if (mallocAndStrcpy_s(&new_config->iothub_host_fqdn, config->iothub_host_fqdn) != RESULT_OK)
        {
            LogError("Failed copying the AMQP_DEVICE_CONFIG (failed copying iothub_host_fqdn)");
            result = MU_FAILURE;
        }
        else
        {
            new_config->authorization_module = config->authorization_module;
            new_config->authentication_mode = config->authentication_mode;
            new_config->on_state_changed_callback = config->on_state_changed_callback;
            new_config->on_state_changed_context = config->on_state_changed_context;
            new_config->device_id = IoTHubClient_Auth_Get_DeviceId(config->authorization_module);
            new_config->module_id = IoTHubClient_Auth_Get_ModuleId(config->authorization_module);
            new_config->prod_info_cb = config->prod_info_cb;
            new_config->prod_info_ctx = config->prod_info_ctx;
            result = RESULT_OK;
        }

        if (result != RESULT_OK)
        {
            destroy_device_config(new_config);
            new_config = NULL;
        }
    }

    return new_config;
}

static void set_authentication_config(AMQP_DEVICE_INSTANCE* device_instance, AUTHENTICATION_CONFIG* auth_config)
{
    AMQP_DEVICE_CONFIG *device_config = device_instance->config;

    auth_config->device_id = device_config->device_id;
    auth_config->module_id = device_config->module_id;
    auth_config->iothub_host_fqdn = device_config->iothub_host_fqdn;
    auth_config->on_error_callback = on_authentication_error_callback;
    auth_config->on_error_callback_context = device_instance;
    auth_config->on_state_changed_callback = on_authentication_state_changed_callback;
    auth_config->on_state_changed_callback_context = device_instance;
    auth_config->authorization_module = device_config->authorization_module;
}

// Create and Destroy Helpers
static void internal_destroy_device(AMQP_DEVICE_INSTANCE* instance)
{
    if (instance != NULL)
    {
        if (instance->messenger_handle != NULL)
        {
            telemetry_messenger_destroy(instance->messenger_handle);
        }

        if (instance->twin_messenger_handle != NULL)
        {
            twin_messenger_destroy(instance->twin_messenger_handle);
        }

        if (instance->authentication_handle != NULL)
        {
            authentication_destroy(instance->authentication_handle);
        }

        tickcounter_destroy(instance->tick_counter_handle);
        destroy_device_config(instance->config);
        free(instance);
    }
}

static int create_authentication_instance(AMQP_DEVICE_INSTANCE *instance)
{
    int result;
    AUTHENTICATION_CONFIG auth_config;

    set_authentication_config(instance, &auth_config);

    if ((instance->authentication_handle = authentication_create(&auth_config)) == NULL)
    {
        LogError("Failed creating the AUTHENTICATION_HANDLE (authentication_create failed)");
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

static int create_telemetry_messenger_instance(AMQP_DEVICE_INSTANCE* instance, pfTransport_GetOption_Product_Info_Callback prod_info_cb, void* prod_info_ctx)
{
    int result;

    TELEMETRY_MESSENGER_CONFIG messenger_config;
    messenger_config.device_id = instance->config->device_id;
    messenger_config.module_id = instance->config->module_id;
    messenger_config.iothub_host_fqdn = instance->config->iothub_host_fqdn;
    messenger_config.on_state_changed_callback = on_messenger_state_changed_callback;
    messenger_config.on_state_changed_context = instance;

    if ((instance->messenger_handle = telemetry_messenger_create(&messenger_config, prod_info_cb, prod_info_ctx)) == NULL)
    {
        LogError("Failed creating the TELEMETRY_MESSENGER_HANDLE (messenger_create failed)");
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

static int create_twin_messenger(AMQP_DEVICE_INSTANCE* instance)
{
    int result;
    TWIN_MESSENGER_CONFIG twin_msgr_config;

    twin_msgr_config.prod_info_cb = instance->config->prod_info_cb;
    twin_msgr_config.prod_info_ctx = instance->config->prod_info_ctx;
    twin_msgr_config.device_id = instance->config->device_id;
    twin_msgr_config.module_id = instance->config->module_id;
    twin_msgr_config.iothub_host_fqdn = instance->config->iothub_host_fqdn;
    twin_msgr_config.on_state_changed_callback = on_twin_messenger_state_changed_callback;
    twin_msgr_config.on_state_changed_context = (void*)instance;

    if ((instance->twin_messenger_handle = twin_messenger_create(&twin_msgr_config)) == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}


// ---------- Set/Retrieve Options Helpers ----------//

static void* device_clone_option(const char* name, const void* value)
{
    void* result;

    if (name == NULL || value == NULL)
    {
        LogError("Failed to clone device option (either name (%p) or value (%p) is NULL)", name, value);
        result = NULL;
    }
    else
    {
        if (strcmp(DEVICE_OPTION_SAVED_AUTH_OPTIONS, name) == 0 ||
            strcmp(DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, name) == 0)
        {
            if ((result = (void*)OptionHandler_Clone((OPTIONHANDLER_HANDLE)value)) == NULL)
            {
                LogError("Failed to clone device option (OptionHandler_Clone failed for option %s)", name);
            }
        }
        else
        {
            LogError("Failed to clone device option (option with name '%s' is not suppported)", name);
            result = NULL;
        }
    }

    return result;
}

static void device_destroy_option(const char* name, const void* value)
{
    if (name == NULL || value == NULL)
    {
        LogError("Failed to destroy device option (either name (%p) or value (%p) is NULL)", name, value);
    }
    else
    {
        if (strcmp(name, DEVICE_OPTION_SAVED_AUTH_OPTIONS) == 0 ||
            strcmp(name, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS) == 0)
        {
            OptionHandler_Destroy((OPTIONHANDLER_HANDLE)value);
        }
        else
        {
            LogError("Failed to clone device option (option with name '%s' is not suppported)", name);
        }
    }
}


//---------- Public APIs ----------//

AMQP_DEVICE_HANDLE amqp_device_create(AMQP_DEVICE_CONFIG *config)
{
    AMQP_DEVICE_INSTANCE *instance;

    if (config == NULL)
    {
        LogError("Failed creating the device instance (config is NULL)");
        instance = NULL;
    }
    else if (config->iothub_host_fqdn == NULL)
    {
        LogError("Failed creating the device instance (config->iothub_host_fqdn is NULL)");
        instance = NULL;
    }
    else if (config->on_state_changed_callback == NULL)
    {
        LogError("Failed creating the device instance (config->on_state_changed_callback is NULL)");
        instance = NULL;
    }
    else if (config->authorization_module == NULL)
    {
        LogError("Failed creating the device instance (config->authorization_module is NULL)");
        instance = NULL;
    }
    else if ((instance = (AMQP_DEVICE_INSTANCE*)malloc(sizeof(AMQP_DEVICE_INSTANCE))) == NULL)
    {
        LogError("Failed creating the device instance (malloc failed)");
    }
    else
    {
        int result;

        memset(instance, 0, sizeof(AMQP_DEVICE_INSTANCE));

        if ((instance->config = clone_device_config(config)) == NULL)
        {
            LogError("Failed creating the device instance for device '%s' (failed copying the configuration)", config->device_id);
            result = MU_FAILURE;
        }
        else if ((instance->tick_counter_handle = tickcounter_create()) == NULL)
        {
            LogError("Failed creating the internal tick counter for device '%s'", instance->config->device_id);
            result = MU_FAILURE;
        }
        else if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS &&
                 create_authentication_instance(instance) != RESULT_OK)
        {
            LogError("Failed creating the device instance for device '%s' (failed creating the authentication instance)", instance->config->device_id);
            result = MU_FAILURE;
        }
        else if (create_telemetry_messenger_instance(instance, config->prod_info_cb, config->prod_info_ctx) != RESULT_OK)
        {
            LogError("Failed creating the device instance for device '%s' (failed creating the messenger instance)", instance->config->device_id);
            result = MU_FAILURE;
        }
        else if (create_twin_messenger(instance) != RESULT_OK)
        {
            LogError("Failed creating the twin messenger for device '%s'", instance->config->device_id);
            result = MU_FAILURE;
        }
        else
        {
            instance->auth_state = AUTHENTICATION_STATE_STOPPED;
            instance->msgr_state = TELEMETRY_MESSENGER_STATE_STOPPED;
            instance->twin_msgr_state = TWIN_MESSENGER_STATE_STOPPED;
            instance->state = DEVICE_STATE_STOPPED;
            instance->auth_state_last_changed_time = INDEFINITE_TIME;
            instance->auth_state_change_timeout_secs = DEFAULT_AUTH_STATE_CHANGED_TIMEOUT_SECS;
            instance->msgr_state_last_changed_time = INDEFINITE_TIME;
            instance->msgr_state_change_timeout_secs = DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS;
            instance->twin_msgr_state_last_changed_time = INDEFINITE_TIME;
            instance->twin_msgr_state_change_timeout_secs = DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS;
            instance->stop_delay_ms = 0;
            instance->last_stop_request_time = 0;

            result = RESULT_OK;
        }

        if (result != RESULT_OK)
        {
            internal_destroy_device(instance);
            instance = NULL;
        }
    }

    return (AMQP_DEVICE_HANDLE)instance;
}

int amqp_device_start_async(AMQP_DEVICE_HANDLE handle, SESSION_HANDLE session_handle, CBS_HANDLE cbs_handle)
{
    int result;

    if (handle == NULL)
    {
        LogError("Failed starting device (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        if (instance->state != DEVICE_STATE_STOPPED)
        {
            LogError("Failed starting device (device is not stopped)");
            result = MU_FAILURE;
        }
        else if (session_handle == NULL)
        {
            LogError("Failed starting device (session_handle is NULL)");
            result = MU_FAILURE;
        }
        else if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS && cbs_handle == NULL)
        {
            LogError("Failed starting device (device using CBS authentication, but cbs_handle is NULL)");
            result = MU_FAILURE;
        }
        else
        {
            instance->session_handle = session_handle;
            instance->cbs_handle = cbs_handle;
	    instance->stop_delay_ms = 0;

            update_state(instance, DEVICE_STATE_STARTING);

            result = RESULT_OK;
        }
    }

    return result;
}

static int internal_device_stop(AMQP_DEVICE_INSTANCE* instance)
{
    int result;

    if (instance->msgr_state != TELEMETRY_MESSENGER_STATE_STOPPED &&
        instance->msgr_state != TELEMETRY_MESSENGER_STATE_STOPPING &&
        telemetry_messenger_stop(instance->messenger_handle) != RESULT_OK)
    {
        LogError("Failed stopping device '%s' (telemetry_messenger_stop failed)", instance->config->device_id);
        update_state(instance, DEVICE_STATE_ERROR_MSG);
        result = MU_FAILURE;
    }
    else if (instance->twin_msgr_state != TWIN_MESSENGER_STATE_STOPPED &&
        instance->twin_msgr_state != TWIN_MESSENGER_STATE_STOPPING &&
        twin_messenger_stop(instance->twin_messenger_handle) != RESULT_OK)
    {
        LogError("Failed stopping device '%s' (twin_messenger_stop failed)", instance->config->device_id);
        update_state(instance, DEVICE_STATE_ERROR_MSG);
        result = MU_FAILURE;
    }
    else if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS &&
        instance->auth_state != AUTHENTICATION_STATE_STOPPED &&
        authentication_stop(instance->authentication_handle) != RESULT_OK)
    {
        LogError("Failed stopping device '%s' (authentication_stop failed)", instance->config->device_id);
        update_state(instance, DEVICE_STATE_ERROR_AUTH);
        result = MU_FAILURE;
    }
    else
    {
        update_state(instance, DEVICE_STATE_STOPPED);
        result = RESULT_OK;
    }

    return result;
}

// @brief
//     stops a device instance (stops messenger and authentication) synchronously.
// @returns
//     0 if the function succeeds, non-zero otherwise.
int amqp_device_stop(AMQP_DEVICE_HANDLE handle)
{
    int result;

    if (handle == NULL)
    {
        LogError("Failed stopping device (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        if (instance->state == DEVICE_STATE_STOPPED || (instance->state == DEVICE_STATE_STOPPING && instance->stop_delay_ms == 0))
        {
            LogError("Failed stopping device '%s' (device is already stopped or stopping)", instance->config->device_id);
            result = MU_FAILURE;
        }
        else
        {
            update_state(instance, DEVICE_STATE_STOPPING);
            
            if (internal_device_stop(instance) != 0)
            {
                LogError("Failed stopping device '%s' (immediate stop failed)", instance->config->device_id);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
    }

    return result;
}

// @brief
//     stops a device instance (stops messenger and authentication) synchronously.
// @returns
//     0 if the function succeeds, non-zero otherwise.
int amqp_device_delayed_stop(AMQP_DEVICE_HANDLE handle, size_t delay_secs)
{
    int result;

    if (handle == NULL)
    {
        LogError("Failed stopping device (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        if (instance->state == DEVICE_STATE_STOPPED || instance->state == DEVICE_STATE_STOPPING)
        {
            LogError("Failed stopping device '%s' (device is already stopped or stopping)", instance->config->device_id);
            result = MU_FAILURE;
        }
        else if (tickcounter_get_current_ms(instance->tick_counter_handle, &instance->last_stop_request_time) != 0)
        {
            LogError("Failed stopping device '%s' (could not get tickcounter time)", instance->config->device_id);
            result = MU_FAILURE;
        }
        else
        {
            update_state(instance, DEVICE_STATE_STOPPING);
            instance->stop_delay_ms = delay_secs * 1000;
            result = RESULT_OK;
        }
    }

    return result;
}

void amqp_device_do_work(AMQP_DEVICE_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("Failed to perform amqp_device_do_work (handle is NULL)");
    }
    else
    {
        // Cranking the state monster:
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        if (instance->state == DEVICE_STATE_STOPPING)
        {
            tickcounter_ms_t current_time_ms;

            if (tickcounter_get_current_ms(instance->tick_counter_handle, &current_time_ms) != 0)
            {
                LogError("Failed stopping device '%s' (could not get tickcounter time)", instance->config->device_id);
                update_state(instance, DEVICE_STATE_ERROR_MSG);
            }
            else if ((current_time_ms - instance->last_stop_request_time) > instance->stop_delay_ms)
            {
                if (internal_device_stop(instance) != 0)
                {
                    LogError("Failed stopping device '%s'", instance->config->device_id);
                    update_state(instance, DEVICE_STATE_ERROR_MSG);
                }
            }
        }
        else if (instance->state == DEVICE_STATE_STARTING)
        {
            if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS)
            {
                if (instance->auth_state == AUTHENTICATION_STATE_STOPPED)
                {
                    if (authentication_start(instance->authentication_handle, instance->cbs_handle) != RESULT_OK)
                    {
                        LogError("Device '%s' failed to be authenticated (authentication_start failed)", instance->config->device_id);

                        update_state(instance, DEVICE_STATE_ERROR_AUTH);
                    }
                }
                else if (instance->auth_state == AUTHENTICATION_STATE_STARTING)
                {
                    int is_timed_out;
                    if (is_timeout_reached(instance->auth_state_last_changed_time, instance->auth_state_change_timeout_secs, &is_timed_out) != RESULT_OK)
                    {
                        LogError("Device '%s' failed verifying the timeout for authentication start (is_timeout_reached failed)", instance->config->device_id);
                        update_state(instance, DEVICE_STATE_ERROR_AUTH);
                    }
                    else if (is_timed_out == 1)
                    {
                        LogError("Device '%s' authentication did not complete starting within expected timeout (%lu)", instance->config->device_id, (unsigned long)instance->auth_state_change_timeout_secs);

                        update_state(instance, DEVICE_STATE_ERROR_AUTH_TIMEOUT);
                    }
                }
                else if (instance->auth_state == AUTHENTICATION_STATE_ERROR)
                {
                    if (instance->auth_error_code == AUTHENTICATION_ERROR_AUTH_FAILED)
                    {
                        update_state(instance, DEVICE_STATE_ERROR_AUTH);
                    }
                    else // DEVICE_STATE_ERROR_TIMEOUT
                    {
                        update_state(instance, DEVICE_STATE_ERROR_AUTH_TIMEOUT);
                    }
                }
                // There is no AUTHENTICATION_STATE_STOPPING
            }

            if (instance->config->authentication_mode == DEVICE_AUTH_MODE_X509 || instance->auth_state == AUTHENTICATION_STATE_STARTED)
            {
                size_t number_of_messengers_started = 0;

                if (instance->msgr_state == TELEMETRY_MESSENGER_STATE_STOPPED)
                {
                    if (telemetry_messenger_start(instance->messenger_handle, instance->session_handle) != RESULT_OK)
                    {
                        LogError("Device '%s' messenger failed to be started (messenger_start failed)", instance->config->device_id);

                        update_state(instance, DEVICE_STATE_ERROR_MSG);
                    }
                }
                else if (instance->msgr_state == TELEMETRY_MESSENGER_STATE_STARTING)
                {
                    int is_timed_out;
                    if (is_timeout_reached(instance->msgr_state_last_changed_time, instance->msgr_state_change_timeout_secs, &is_timed_out) != RESULT_OK)
                    {
                        LogError("Device '%s' failed verifying the timeout for messenger start (is_timeout_reached failed)", instance->config->device_id);

                        update_state(instance, DEVICE_STATE_ERROR_MSG);
                    }
                    else if (is_timed_out == 1)
                    {
                        LogError("Device '%s' messenger did not complete starting within expected timeout (%lu)", instance->config->device_id, (unsigned long)instance->msgr_state_change_timeout_secs);

                        update_state(instance, DEVICE_STATE_ERROR_MSG);
                    }
                }
                else if (instance->msgr_state == TELEMETRY_MESSENGER_STATE_ERROR)
                {
                    LogError("Device '%s' messenger failed to be started (messenger got into error state)", instance->config->device_id);

                    update_state(instance, DEVICE_STATE_ERROR_MSG);
                }
                else if (instance->msgr_state == TELEMETRY_MESSENGER_STATE_STARTED)
                {
                    number_of_messengers_started++;
                }

                if (instance->twin_msgr_state == TWIN_MESSENGER_STATE_STOPPED)
                {
                    if (twin_messenger_start(instance->twin_messenger_handle, instance->session_handle) != RESULT_OK)
                    {
                        LogError("Device '%s' twin messenger failed to be started (messenger_start failed)", instance->config->device_id);

                        update_state(instance, DEVICE_STATE_ERROR_MSG);
                    }
                }
                else if (instance->twin_msgr_state == TWIN_MESSENGER_STATE_STARTING)
                {
                    int is_timed_out;
                    if (is_timeout_reached(instance->twin_msgr_state_last_changed_time, instance->twin_msgr_state_change_timeout_secs, &is_timed_out) != RESULT_OK)
                    {
                        LogError("Device '%s' failed verifying the timeout for twin messenger start (is_timeout_reached failed)", instance->config->device_id);

                        update_state(instance, DEVICE_STATE_ERROR_MSG);
                    }
                    else if (is_timed_out == 1)
                    {
                        LogError("Device '%s' twin messenger did not complete starting within expected timeout (%lu)", instance->config->device_id, (unsigned long)instance->twin_msgr_state_change_timeout_secs);

                        update_state(instance, DEVICE_STATE_ERROR_MSG);
                    }
                }
                else if (instance->twin_msgr_state == TWIN_MESSENGER_STATE_ERROR)
                {
                    LogError("Device '%s' twin messenger failed to be started (messenger got into error state)", instance->config->device_id);

                    update_state(instance, DEVICE_STATE_ERROR_MSG);
                }
                else if (instance->twin_msgr_state == TWIN_MESSENGER_STATE_STARTED)
                {
                    number_of_messengers_started++;
                }

                if (number_of_messengers_started == 2)
                {
                    update_state(instance, DEVICE_STATE_STARTED);
                }
            }
        }
        else if (instance->state == DEVICE_STATE_STARTED)
        {
            if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS &&
                instance->auth_state != AUTHENTICATION_STATE_STARTED)
            {
                LogError("Device '%s' is started but authentication reported unexpected state %d", instance->config->device_id, instance->auth_state);

                if (instance->auth_state != AUTHENTICATION_STATE_ERROR)
                {
                    if (instance->auth_error_code == AUTHENTICATION_ERROR_AUTH_FAILED)
                    {
                        update_state(instance, DEVICE_STATE_ERROR_AUTH);
                    }
                    else // AUTHENTICATION_ERROR_AUTH_TIMEOUT
                    {
                        update_state(instance, DEVICE_STATE_ERROR_AUTH_TIMEOUT);
                    }
                }
                else
                {
                    update_state(instance, DEVICE_STATE_ERROR_AUTH);
                }
            }
            else
            {
                if (instance->msgr_state != TELEMETRY_MESSENGER_STATE_STARTED)
                {
                    LogError("Device '%s' is started but messenger reported unexpected state %d", instance->config->device_id, instance->msgr_state);
                    update_state(instance, DEVICE_STATE_ERROR_MSG);
                }

                if (instance->twin_msgr_state != TWIN_MESSENGER_STATE_STARTED)
                {
                    LogError("Device '%s' is started but TWIN messenger reported unexpected state %d", instance->config->device_id, instance->twin_msgr_state);
                    update_state(instance, DEVICE_STATE_ERROR_MSG);
                }
            }
        }

        // Invoking the do_works():
        if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS)
        {
            if (instance->auth_state != AUTHENTICATION_STATE_STOPPED && instance->auth_state != AUTHENTICATION_STATE_ERROR)
            {
                authentication_do_work(instance->authentication_handle);
            }
        }

        if (instance->msgr_state != TELEMETRY_MESSENGER_STATE_STOPPED && instance->msgr_state != TELEMETRY_MESSENGER_STATE_ERROR)
        {
            telemetry_messenger_do_work(instance->messenger_handle);
        }

        if (instance->twin_msgr_state != TWIN_MESSENGER_STATE_STOPPED && instance->twin_msgr_state != TWIN_MESSENGER_STATE_ERROR)
        {
            twin_messenger_do_work(instance->twin_messenger_handle);
        }
    }
}

void amqp_device_destroy(AMQP_DEVICE_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("Failed destroying device handle (handle is NULL)");
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;
        if (instance->state == DEVICE_STATE_STARTED || instance->state == DEVICE_STATE_STARTING)
        {
            (void)amqp_device_stop((AMQP_DEVICE_HANDLE)instance);
        }

        internal_destroy_device((AMQP_DEVICE_INSTANCE*)handle);
    }
}

int amqp_device_send_event_async(AMQP_DEVICE_HANDLE handle, IOTHUB_MESSAGE_LIST* message, ON_DEVICE_D2C_EVENT_SEND_COMPLETE on_device_d2c_event_send_complete_callback, void* context)
{
    int result;

    if (handle == NULL || message == NULL)
    {
        LogError("Failed sending event (either handle (%p) or message (%p) are NULL)", handle, message);
        result = MU_FAILURE;
    }
    else
    {
        DEVICE_SEND_EVENT_TASK* send_task;

        if ((send_task = (DEVICE_SEND_EVENT_TASK*)malloc(sizeof(DEVICE_SEND_EVENT_TASK))) == NULL)
        {
            LogError("Failed sending event (failed creating task to send event)");
            result = MU_FAILURE;
        }
        else
        {
            AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

            memset(send_task, 0, sizeof(DEVICE_SEND_EVENT_TASK));
            send_task->on_event_send_complete_callback = on_device_d2c_event_send_complete_callback;
            send_task->on_event_send_complete_context = context;

            if (telemetry_messenger_send_async(instance->messenger_handle, message, on_event_send_complete_messenger_callback, (void*)send_task) != RESULT_OK)
            {
                LogError("Failed sending event (telemetry_messenger_send_async failed)");
                free(send_task);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
    }

    return result;
}

int amqp_device_get_send_status(AMQP_DEVICE_HANDLE handle, DEVICE_SEND_STATUS *send_status)
{
    int result;


    if (handle == NULL || send_status == NULL)
    {
        LogError("Failed getting the device messenger send status (NULL parameter received; handle=%p, send_status=%p)", handle, send_status);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;
        TELEMETRY_MESSENGER_SEND_STATUS messenger_send_status;

        if (telemetry_messenger_get_send_status(instance->messenger_handle, &messenger_send_status) != RESULT_OK)
        {
            LogError("Failed getting the device messenger send status (telemetry_messenger_get_send_status failed)");
            result = MU_FAILURE;
        }
        else
        {
            if (messenger_send_status == TELEMETRY_MESSENGER_SEND_STATUS_IDLE)
            {
                *send_status = DEVICE_SEND_STATUS_IDLE;
            }
            else // i.e., messenger_send_status == TELEMETRY_MESSENGER_SEND_STATUS_BUSY
            {
                *send_status = DEVICE_SEND_STATUS_BUSY;
            }

            result = RESULT_OK;
        }
    }

    return result;
}

int amqp_device_subscribe_message(AMQP_DEVICE_HANDLE handle, ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback, void* context)
{
    int result;

    if (handle == NULL || on_message_received_callback == NULL || context == NULL)
    {
        LogError("Failed subscribing to C2D messages (either handle (%p), on_message_received_callback (%p) or context (%p) is NULL)",
            handle, on_message_received_callback, context);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        if (telemetry_messenger_subscribe_for_messages(instance->messenger_handle, on_messenger_message_received_callback, handle) != RESULT_OK)
        {
            LogError("Failed subscribing to C2D messages (telemetry_messenger_subscribe_for_messages failed)");
            result = MU_FAILURE;
        }
        else
        {
            instance->on_message_received_callback = on_message_received_callback;
            instance->on_message_received_context = context;

            result = RESULT_OK;
        }
    }

    return result;
}

int amqp_device_unsubscribe_message(AMQP_DEVICE_HANDLE handle)
{
    int result;

    if (handle == NULL)
    {
        LogError("Failed unsubscribing to C2D messages (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        if (telemetry_messenger_unsubscribe_for_messages(instance->messenger_handle) != RESULT_OK)
        {
            LogError("Failed unsubscribing to C2D messages (telemetry_messenger_unsubscribe_for_messages failed)");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }
    return result;
}

int amqp_device_send_message_disposition(AMQP_DEVICE_HANDLE device_handle, DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info, DEVICE_MESSAGE_DISPOSITION_RESULT disposition_result)
{
    int result;

    if (device_handle == NULL || disposition_info == NULL)
    {
        LogError("Failed sending message disposition (either device_handle (%p) or disposition_info (%p) are NULL)", device_handle, disposition_info);
        result = MU_FAILURE;
    }
    else if (disposition_info->source == NULL)
    {
        LogError("Failed sending message disposition (disposition_info->source is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* device = (AMQP_DEVICE_INSTANCE*)device_handle;
        TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* messenger_disposition_info;

        if ((messenger_disposition_info = create_messenger_disposition_info(disposition_info)) == NULL)
        {
            LogError("Failed sending message disposition (failed to create TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO)");
            result = MU_FAILURE;
        }
        else
        {
            TELEMETRY_MESSENGER_DISPOSITION_RESULT messenger_disposition_result = get_messenger_message_disposition_result_from(disposition_result);

            if (telemetry_messenger_send_message_disposition(device->messenger_handle, messenger_disposition_info, messenger_disposition_result) != RESULT_OK)
            {
                LogError("Failed sending message disposition (telemetry_messenger_send_message_disposition failed)");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }

            destroy_messenger_disposition_info(messenger_disposition_info);
        }
    }

    return result;
}

int amqp_device_set_retry_policy(AMQP_DEVICE_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY policy, size_t retry_timeout_limit_in_seconds)
{
    (void)retry_timeout_limit_in_seconds;
    (void)policy;
    int result;

    if (handle == NULL)
    {
        LogError("Failed setting retry policy (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        LogError("Failed setting retry policy (functionality not supported)");
        result = MU_FAILURE;
    }

    return result;
}

int amqp_device_set_option(AMQP_DEVICE_HANDLE handle, const char* name, void* value)
{
    int result;

    if (handle == NULL || name == NULL || value == NULL)
    {
        LogError("failed setting device option (one of the following are NULL: _handle=%p, name=%p, value=%p)",
            handle, name, value);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        if (strcmp(DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, name) == 0 ||
            strcmp(DEVICE_OPTION_SAS_TOKEN_REFRESH_TIME_SECS, name) == 0 ||
            strcmp(DEVICE_OPTION_SAS_TOKEN_LIFETIME_SECS, name) == 0)
        {
            if (instance->authentication_handle == NULL)
            {
                LogError("failed setting option for device '%s' (cannot set authentication option '%s'; not using CBS authentication)", instance->config->device_id, name);
                result = MU_FAILURE;
            }
            else if(authentication_set_option(instance->authentication_handle, name, value) != RESULT_OK)
            {
                LogError("failed setting option for device '%s' (failed setting authentication option '%s')", instance->config->device_id, name);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else if (strcmp(DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, name) == 0)
        {
            if (telemetry_messenger_set_option(instance->messenger_handle, TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, value) != RESULT_OK)
            {
                LogError("failed setting option for device '%s' (failed setting messenger option '%s')", instance->config->device_id, name);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else if (strcmp(DEVICE_OPTION_SAVED_AUTH_OPTIONS, name) == 0)
        {
            if (instance->authentication_handle == NULL)
            {
                LogError("failed setting option for device '%s' (cannot set authentication option '%s'; not using CBS authentication)", instance->config->device_id, name);
                result = MU_FAILURE;
            }
            else if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, instance->authentication_handle) != OPTIONHANDLER_OK)
            {
                LogError("failed setting option for device '%s' (OptionHandler_FeedOptions failed for authentication instance)", instance->config->device_id);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else if (strcmp(DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, name) == 0)
        {
            if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, instance->messenger_handle) != OPTIONHANDLER_OK)
            {
                LogError("failed setting option for device '%s' (OptionHandler_FeedOptions failed for messenger instance)", instance->config->device_id);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else if (strcmp(DEVICE_OPTION_SAVED_OPTIONS, name) == 0)
        {
            if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, handle) != OPTIONHANDLER_OK)
            {
                LogError("failed setting option for device '%s' (OptionHandler_FeedOptions failed)", instance->config->device_id);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else
        {
            LogError("failed setting option for device '%s' (option with name '%s' is not suppported)", instance->config->device_id, name);
            result = MU_FAILURE;
        }
    }

    return result;
}

OPTIONHANDLER_HANDLE amqp_device_retrieve_options(AMQP_DEVICE_HANDLE handle)
{
    OPTIONHANDLER_HANDLE result;

    if (handle == NULL)
    {
        LogError("Failed to retrieve options from device instance (handle is NULL)");
        result = NULL;
    }
    else
    {
        OPTIONHANDLER_HANDLE options = OptionHandler_Create(device_clone_option, device_destroy_option, (pfSetOption)amqp_device_set_option);

        if (options == NULL)
        {
            LogError("Failed to retrieve options from device instance (OptionHandler_Create failed)");
            result = NULL;
        }
        else
        {
            AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

            OPTIONHANDLER_HANDLE dependency_options = NULL;

            if (instance->authentication_handle != NULL &&
                (dependency_options = authentication_retrieve_options(instance->authentication_handle)) == NULL)
            {
                LogError("Failed to retrieve options from device '%s' (failed to retrieve options from authentication instance)", instance->config->device_id);
                result = NULL;
            }
            else if (instance->authentication_handle != NULL &&
                OptionHandler_AddOption(options, DEVICE_OPTION_SAVED_AUTH_OPTIONS, (const void*)dependency_options) != OPTIONHANDLER_OK)
            {
                LogError("Failed to retrieve options from device '%s' (OptionHandler_AddOption failed for option '%s')", instance->config->device_id, DEVICE_OPTION_SAVED_AUTH_OPTIONS);
                result = NULL;
            }
            else if ((dependency_options = telemetry_messenger_retrieve_options(instance->messenger_handle)) == NULL)
            {
                LogError("Failed to retrieve options from device '%s' (failed to retrieve options from messenger instance)", instance->config->device_id);
                result = NULL;
            }
            else if (OptionHandler_AddOption(options, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, (const void*)dependency_options) != OPTIONHANDLER_OK)
            {
                LogError("Failed to retrieve options from device '%s' (OptionHandler_AddOption failed for option '%s')", instance->config->device_id, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS);
                result = NULL;
            }
            else
            {
                result = options;
            }

            if (result == NULL)
            {
                OptionHandler_Destroy(options);
            }
        }
    }

    return result;
}

int amqp_device_send_twin_update_async(AMQP_DEVICE_HANDLE handle, CONSTBUFFER_HANDLE data, DEVICE_SEND_TWIN_UPDATE_COMPLETE_CALLBACK on_send_twin_update_complete_callback, void* context)
{
    int result;

    if (handle == NULL || data == NULL)
    {
        LogError("Invalid argument (handle=%p, data=%p)", handle, data);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;
        DEVICE_SEND_TWIN_UPDATE_CONTEXT* twin_ctx;

        if ((twin_ctx = (DEVICE_SEND_TWIN_UPDATE_CONTEXT*)malloc(sizeof(DEVICE_SEND_TWIN_UPDATE_CONTEXT))) == NULL)
        {
            LogError("Cannot send twin update (failed creating TWIN context)");
            result = MU_FAILURE;
        }
        else
        {
            twin_ctx->on_send_twin_update_complete_callback = on_send_twin_update_complete_callback;
            twin_ctx->context = context;

            if (twin_messenger_report_state_async(instance->twin_messenger_handle, data, on_report_state_complete_callback, (const void*)twin_ctx) != 0)
            {
                LogError("Cannot send twin update (failed creating TWIN messenger)");
                free(twin_ctx);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
    }

    return result;
}

int amqp_device_subscribe_for_twin_updates(AMQP_DEVICE_HANDLE handle, DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK on_device_twin_update_received_callback, void* context)
{
    int result;

    if (handle == NULL || on_device_twin_update_received_callback == NULL)
    {
        LogError("Invalid argument (handle=%p, on_device_twin_update_received_callback=%p)", handle, on_device_twin_update_received_callback);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK previous_callback = instance->on_device_twin_update_received_callback;
        void* previous_context = instance->on_device_twin_update_received_context;

        instance->on_device_twin_update_received_callback = on_device_twin_update_received_callback;
        instance->on_device_twin_update_received_context = context;

        if (twin_messenger_subscribe(instance->twin_messenger_handle, on_twin_state_update_callback, (void*)instance) != 0)
        {
            LogError("Failed subscribing for device twin updates");
            instance->on_device_twin_update_received_callback = previous_callback;
            instance->on_device_twin_update_received_context = previous_context;
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

int amqp_device_unsubscribe_for_twin_updates(AMQP_DEVICE_HANDLE handle)
{
    int result;


    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;

        if (twin_messenger_unsubscribe(instance->twin_messenger_handle) != 0)
        {
            LogError("Failed unsubscribing for device twin updates");
            result = MU_FAILURE;
        }
        else
        {
            instance->on_device_twin_update_received_callback = NULL;
            instance->on_device_twin_update_received_context = NULL;
            result = RESULT_OK;
        }
    }

    return result;
}

int amqp_device_get_twin_async(AMQP_DEVICE_HANDLE handle, DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK on_device_get_twin_completed_callback, void* context)
{
    int result;

    if (handle == NULL || on_device_get_twin_completed_callback == NULL)
    {
        LogError("Invalid argument (handle=%p, on_device_get_twin_completed_callback=%p)", handle, on_device_get_twin_completed_callback);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_DEVICE_INSTANCE* instance = (AMQP_DEVICE_INSTANCE*)handle;
        DEVICE_GET_TWIN_CONTEXT* twin_ctx;

        if ((twin_ctx = (DEVICE_GET_TWIN_CONTEXT*)malloc(sizeof(DEVICE_GET_TWIN_CONTEXT))) == NULL)
        {
            LogError("Cannot get device twin (failed creating TWIN context)");
            result = MU_FAILURE;
        }
        else
        {
            twin_ctx->on_get_twin_completed_callback = on_device_get_twin_completed_callback;
            twin_ctx->context = context;

            if (twin_messenger_get_twin_async(instance->twin_messenger_handle, on_get_twin_completed, twin_ctx) != 0)
            {
                LogError("Failed getting device twin");
                free(twin_ctx);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
    }

    return result;
}

DEVICE_MESSAGE_DISPOSITION_INFO* amqp_device_clone_message_disposition_info(DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info)
{
    DEVICE_MESSAGE_DISPOSITION_INFO* clone;

    if ((clone = calloc(1, sizeof(DEVICE_MESSAGE_DISPOSITION_INFO))) == NULL)
    {
        LogError("Failed cloning DEVICE_MESSAGE_DISPOSITION_INFO");
    }
    else if (mallocAndStrcpy_s(&clone->source, disposition_info->source) != 0)
    {
        LogError("Failed cloning DEVICE_MESSAGE_DISPOSITION_INFO->source");
        free(clone);
        clone = NULL;
    }
    else
    {
        clone->message_id = disposition_info->message_id;
    }

    return clone;
}

void amqp_device_destroy_message_disposition_info(DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info)
{
    if (disposition_info == NULL)
    {
        LogError("Argument is invalid (disposition_info is NULL)");
    }
    else
    {
        destroy_device_disposition_info(disposition_info);
    }
}
