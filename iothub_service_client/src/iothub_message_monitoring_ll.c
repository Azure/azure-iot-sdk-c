// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <ctype.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/sastoken.h"

#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_plain.h"
#include "azure_uamqp_c/cbs.h"

#include "iothub_message_monitoring_ll.h"
#include "iothub_sc_version.h"
#include "uamqp_messaging.h"

#define INDEFINITE_TIME                       ((time_t)-1)
#define DEFAULT_IOTHUB_AMQP_PORT              5671
#define DEFAULT_LINK_TARGET_NAME              "svc-cl-d2c-msg-mon"
#define DEFAULT_LINK_NAME                     "svc-cl-rcv-link"
#define DEFAULT_CONNECTION_CONTAINER_ID       "svc-cl-msg-mon"
#define DEFAULT_SESSION_INCOMING_WINDOW       2147483647
#define DEFAULT_SESSION_OUTGOING_WINDOW       (255 * 1024)
#define DEFAULT_SAS_TOKEN_DURATION_IN_SECS    (365 * 24 * 60 * 60)

typedef struct CALLBACK_DATA_TAG
{
    IOTHUB_MESSAGE_MONITORING_STATE_CHANGED_CALLBACK openCompleteCompleteCallback;
    void* openUserContext;
    void* sendUserContext;
    void* feedbackUserContext;
} CALLBACK_DATA;

typedef struct IOTHUB_MESSAGE_MONITORING_LL_TAG
{
    char* hostname;
    char* iothubName;
    char* iothubSuffix;
    char* sharedAccessKey;
    char* keyName;

    MESSAGE_RECEIVER_HANDLE message_receiver;
    CONNECTION_HANDLE connection;
    SESSION_HANDLE session;
    LINK_HANDLE receiver_link;
    SASL_MECHANISM_HANDLE sasl_mechanism_handle;
    XIO_HANDLE sasl_io;
    XIO_HANDLE tls_io;
    MESSAGE_RECEIVER_STATE message_receiver_state;

    IOTHUB_MESSAGE_RECEIVED_CALLBACK message_received_callback;
    void* message_received_context;
    IOTHUB_MESSAGE_MONITORING_STATE_CHANGED_CALLBACK on_state_changed_callback;
    void* on_state_changed_context;

    IOTHUB_MESSAGE_MONITORING_STATE state;
    bool enableLogging;
} IOTHUB_MESSAGE_MONITORING_LL;

static size_t get_sas_token_expiry_time(size_t duration_in_seconds)
{
    size_t result;
    time_t current_time;

    if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
    {
        LogError("Failed getting the current local time");
        result = 0;
    }
    else
    {
        result = (int)get_difftime(current_time, (time_t)0) + duration_in_seconds;
    }

    return result;
}

static char* createSasToken(IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle)
{
    char* result;
    STRING_HANDLE hostName;

    if ((hostName = STRING_construct(messageMonitoringHandle->hostname)) == NULL)
    {
        LogError("STRING_construct failed for hostName");
        result = NULL;
    }
    else
    {
        STRING_HANDLE sharedAccessKey;

        if ((sharedAccessKey = STRING_construct(messageMonitoringHandle->sharedAccessKey)) == NULL)
        {
            LogError("STRING_construct failed for sharedAccessKey");
            result = NULL;
        }
        else
        {
            STRING_HANDLE keyName;
        
            if ((keyName = STRING_construct(messageMonitoringHandle->keyName)) == NULL)
            {
                LogError("STRING_construct failed for keyName");
                result = NULL;
            }
            else
            {
                size_t expiry_time;

                if ((expiry_time = get_sas_token_expiry_time(DEFAULT_SAS_TOKEN_DURATION_IN_SECS)) == 0)
                {
                    LogError("Failed defining SAS token expiry time");
                    result = NULL;
                }
                else
                {
                    STRING_HANDLE sasHandle = SASToken_Create(sharedAccessKey, hostName, keyName, expiry_time);

                    if (sasHandle == NULL)
                    {
                        LogError("SASToken_Create failed");
                        result = NULL;
                    }
                    else
                    {
                        const char* sas_charptr;

                        if ((sas_charptr = (const char*)STRING_c_str(sasHandle)) == NULL)
                        {
                            LogError("STRING_c_str returned NULL");
                            result = NULL;
                        }
                        else if (mallocAndStrcpy_s(&result, sas_charptr) != 0)
                        {
                            LogError("mallocAndStrcpy_s failed for sharedAccessToken");
                            result = NULL;
                        }

                        STRING_delete(sasHandle);
                    }
                }

                STRING_delete(keyName);
            }

            STRING_delete(sharedAccessKey);
        }

        STRING_delete(hostName);
    }

    return result;
}

static char* createAuthCid(IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle)
{
    char* result;
    const char* AMQP_SEND_AUTHCID_FMT = "iothubowner@sas.root.%s";
    size_t authCidLen = strlen(AMQP_SEND_AUTHCID_FMT) + strlen(messageMonitoringHandle->iothubName);

    if ((result = (char*)malloc(authCidLen + 1)) == NULL)
    {
        LogError("Malloc failed for authCid.");
    }
    else if ((snprintf(result, authCidLen + 1, AMQP_SEND_AUTHCID_FMT, messageMonitoringHandle->iothubName)) < 0)
    {
        LogError("sprintf_s failed for authCid.");
        free(result);
        result = NULL;
    }

    return result;
}

static int attachServiceClientTypeToLink(LINK_HANDLE link)
{
    fields attach_properties;
    AMQP_VALUE serviceClientTypeKeyName;
    AMQP_VALUE serviceClientTypeValue;
    int result;

    if ((attach_properties = amqpvalue_create_map()) == NULL)
    {
        LogError("Failed to create the map for service client type.");
        result = __FAILURE__;
    }
    else
    {
        if ((serviceClientTypeKeyName = amqpvalue_create_symbol("com.microsoft:client-version")) == NULL)
        {
            LogError("Failed to create the key name for the service client type.");
            result = __FAILURE__;
        }
        else
        {
            if ((serviceClientTypeValue = amqpvalue_create_string(IOTHUB_SERVICE_CLIENT_TYPE_PREFIX IOTHUB_SERVICE_CLIENT_BACKSLASH IOTHUB_SERVICE_CLIENT_VERSION)) == NULL)
            {
                LogError("Failed to create the key value for the service client type.");
                result = __FAILURE__;
            }
            else
            {
                if ((result = amqpvalue_set_map_value(attach_properties, serviceClientTypeKeyName, serviceClientTypeValue)) != 0)
                {
                    LogError("Failed to set the property map for the service client type.  Error code is: %d", result);
                }
                else if ((result = link_set_attach_properties(link, attach_properties)) != 0)
                {
                    LogError("Unable to attach the service client type to the link properties. Error code is: %d", result);
                }
                else
                {
                    result = 0;
                }

                amqpvalue_destroy(serviceClientTypeValue);
            }

            amqpvalue_destroy(serviceClientTypeKeyName);
        }

        amqpvalue_destroy(attach_properties);
    }
    return result;
}

void IoTHubMessageMonitoring_LL_Close(IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle)
{
    if (messageMonitoringHandle == NULL)
    {
        LogError("Input parameter cannot be NULL");
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING_LL* msgMon = (IOTHUB_MESSAGE_MONITORING_LL*)messageMonitoringHandle;

        if (msgMon->message_receiver != NULL)
        {
            messagereceiver_destroy(msgMon->message_receiver);
            msgMon->message_receiver = NULL;
        }

        if (msgMon->receiver_link != NULL)
        {
            link_destroy(msgMon->receiver_link);
            msgMon->receiver_link = NULL;
        }

        if (msgMon->session != NULL)
        {
            session_destroy(msgMon->session);
            msgMon->session = NULL;
        }

        if (msgMon->connection != NULL)
        {
            connection_destroy(msgMon->connection);
            msgMon->connection = NULL;
        }

        if (msgMon->sasl_io != NULL)
        {
            xio_destroy(msgMon->sasl_io);
            msgMon->sasl_io = NULL;
        }

        if (msgMon->sasl_mechanism_handle != NULL)
        {
            saslmechanism_destroy(msgMon->sasl_mechanism_handle);
            msgMon->sasl_mechanism_handle = NULL;
        }

        if (msgMon->tls_io != NULL)
        {
            xio_destroy(msgMon->tls_io);
            msgMon->tls_io = NULL;
        }

        msgMon->on_state_changed_callback = NULL;
        msgMon->on_state_changed_context = NULL;
    }
}

static void on_messagereceiver_state_changed(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
{
    if (context != NULL && new_state != previous_state)
    {
        IOTHUB_MESSAGE_MONITORING_LL* msgMon = (IOTHUB_MESSAGE_MONITORING_LL*)context;

        msgMon->message_receiver_state = new_state;

        if (new_state == MESSAGE_RECEIVER_STATE_IDLE)
        {
            IOTHUB_MESSAGE_MONITORING_STATE previous_internal_state = msgMon->state;
            IOTHUB_MESSAGE_MONITORING_STATE_CHANGED_CALLBACK on_state_changed_callback = msgMon->on_state_changed_callback;
            void* on_state_changed_context = msgMon->on_state_changed_context;

            IoTHubMessageMonitoring_LL_Close(msgMon);

            msgMon->state = IOTHUB_MESSAGE_MONITORING_STATE_CLOSED;

            if (on_state_changed_callback != NULL)
            {
                on_state_changed_callback(on_state_changed_context, msgMon->state, previous_internal_state);
            }
        }
        else if (new_state == MESSAGE_RECEIVER_STATE_OPEN)
        {
            IOTHUB_MESSAGE_MONITORING_STATE previous_internal_state = msgMon->state;
            
            msgMon->state = IOTHUB_MESSAGE_MONITORING_STATE_OPEN;

            if (msgMon->on_state_changed_callback != NULL)
            {
                msgMon->on_state_changed_callback(msgMon->on_state_changed_context, msgMon->state, previous_internal_state);
            }
        }
        else if (new_state == MESSAGE_RECEIVER_STATE_ERROR)
        {
            IOTHUB_MESSAGE_MONITORING_STATE previous_internal_state = msgMon->state;

            msgMon->state = IOTHUB_MESSAGE_MONITORING_STATE_ERROR;

            if (msgMon->on_state_changed_callback != NULL)
            {
                msgMon->on_state_changed_callback(msgMon->on_state_changed_context, msgMon->state, previous_internal_state);
            }
        }
    }
}

void IoTHubMessageMonitoring_LL_Destroy(IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle)
{
    if (messageMonitoringHandle == NULL)
    {
        LogError("Invalid argument (messageMonitoringHandle=NULL)");
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING_LL* msgMon = (IOTHUB_MESSAGE_MONITORING_LL*)messageMonitoringHandle;


        // TODO: stop or destroy components.

        if (msgMon->hostname != NULL)
        {
            free(msgMon->hostname);
        }
        
        if (msgMon->iothubName != NULL)
        {
            free(msgMon->iothubName);
        }

        if (msgMon->iothubSuffix != NULL)
        {
            free(msgMon->iothubSuffix);
        }

        if (msgMon->sharedAccessKey != NULL)
        {
            free(msgMon->sharedAccessKey);
        }

        if (msgMon->keyName != NULL)
        {
            free(msgMon->keyName);
        }

        free(msgMon);
    }
}

IOTHUB_MESSAGE_MONITORING_LL_HANDLE IoTHubMessageMonitoring_LL_Create(IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientHandle)
{
    IOTHUB_MESSAGE_MONITORING_LL_HANDLE result;

    if (serviceClientHandle == NULL)
    {
        LogError("serviceClientHandle input parameter cannot be NULL");
        result = NULL;
    }
    else
    {
        IOTHUB_SERVICE_CLIENT_AUTH* serviceClientAuth = (IOTHUB_SERVICE_CLIENT_AUTH*)serviceClientHandle;

        if (serviceClientAuth->hostname == NULL ||
            serviceClientAuth->iothubName == NULL ||
            serviceClientAuth->iothubSuffix == NULL ||
            serviceClientAuth->keyName == NULL ||
            serviceClientAuth->sharedAccessKey == NULL)
        {
            LogError("Invalid argument (hostname=%p, iothubName=%p, iothubSuffix=%p, keyName=%p, sharedAccessKey=%p)", 
                serviceClientAuth->hostname, serviceClientAuth->iothubName, serviceClientAuth->iothubSuffix, serviceClientAuth->keyName, serviceClientAuth->sharedAccessKey);
            result = NULL;
        }
        else
        {
            if ((result = (IOTHUB_MESSAGE_MONITORING_LL*)malloc(sizeof(IOTHUB_MESSAGE_MONITORING_LL))) == NULL)
            {
                LogError("Malloc failed for IOTHUB_MESSAGE_MONITORING_LL");
            }
            else
            {
                memset(result, 0, sizeof(IOTHUB_MESSAGE_MONITORING_LL));

                if (mallocAndStrcpy_s(&result->hostname, serviceClientAuth->hostname) != 0)
                {
                    LogError("mallocAndStrcpy_s failed for hostName");
                    IoTHubMessageMonitoring_LL_Destroy(result);
                    result = NULL;
                }
                else if (mallocAndStrcpy_s(&result->iothubName, serviceClientAuth->iothubName) != 0)
                {
                    LogError("mallocAndStrcpy_s failed for iothubName");
                    IoTHubMessageMonitoring_LL_Destroy(result);
                    result = NULL;
                }
                else if (mallocAndStrcpy_s(&result->iothubSuffix, serviceClientAuth->iothubSuffix) != 0)
                {
                    LogError("mallocAndStrcpy_s failed for iothubSuffix");
                    IoTHubMessageMonitoring_LL_Destroy(result);
                    result = NULL;
                }
                else if (mallocAndStrcpy_s(&result->sharedAccessKey, serviceClientAuth->sharedAccessKey) != 0)
                {
                    LogError("mallocAndStrcpy_s failed for sharedAccessKey");
                    IoTHubMessageMonitoring_LL_Destroy(result);
                    result = NULL;
                }
                else if (mallocAndStrcpy_s(&result->keyName, serviceClientAuth->keyName) != 0)
                {
                    LogError("mallocAndStrcpy_s failed for keyName");
                    IoTHubMessageMonitoring_LL_Destroy(result);
                    result = NULL;
                }
                else
                {
                    result->message_receiver_state = MESSAGE_RECEIVER_STATE_IDLE;
                }
            }
        }
    }

    return result;
}

static char* createLinkSourceAddress(IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle, const char* consumerGroup, size_t partitionNumber)
{
    (void)partitionNumber;
    char* result;
    const char* defaultConsumerGroup = "$Default";
    const char* AMQP_RECEIVE_TARGET_ADDRESS_FMT = "amqps://%s/ConsumerGroups/%s/Partitions/%u";

    size_t addressLen;
    
    if (consumerGroup == NULL)
    {
        consumerGroup = defaultConsumerGroup;
    }
    
    addressLen = strlen(AMQP_RECEIVE_TARGET_ADDRESS_FMT) + strlen(messageMonitoringHandle->hostname) + strlen(consumerGroup) + 10; // 10 = space for partitionNumber.

    if ((result = (char*)malloc(addressLen + 1)) == NULL)
    {
        LogError("Malloc failed for receiveTargetAddress");
    }
    else if ((snprintf(result, addressLen + 1, AMQP_RECEIVE_TARGET_ADDRESS_FMT, messageMonitoringHandle->hostname, consumerGroup, partitionNumber)) < 0)
    {
        LogError("sprintf_s failed for receiveTargetAddress.");
        free(result);
        result = NULL;
    }

    return result;
}

static AMQP_VALUE on_message_received(const void* context, MESSAGE_HANDLE message)
{
    AMQP_VALUE result;

    if (context == NULL || message == NULL)
    {
        LogError("Invalid argument (context=%p, message=%p)", context, message);
        result = messaging_delivery_rejected("API error", "context and/or message is NULL");
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING_LL* msgMon = (IOTHUB_MESSAGE_MONITORING_LL*)context;

        if (msgMon->message_received_callback == NULL)
        {
            result = messaging_delivery_released();
        }
        else
        {
            IOTHUB_MESSAGE_HANDLE iothub_message;
            
            if (message_create_IoTHubMessage_from_uamqp_message(message, &iothub_message) != 0)
            {
                LogError("Failed parsing uamqp message into IoTHub Message");
                result = messaging_delivery_released();
            }
            else
            {
                msgMon->message_received_callback(msgMon->message_received_context, iothub_message);
                IoTHubMessage_Destroy(iothub_message);
                result = messaging_delivery_accepted();
            }
        }
    }

    return result;
}

IOTHUB_MESSAGE_MONITORING_RESULT IoTHubMessageMonitoring_LL_Open(
    IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle,
    const char* consumerGroup,
    size_t partitionNumber,
    IOTHUB_MESSAGE_FILTER* filter,
    IOTHUB_MESSAGE_MONITORING_STATE_CHANGED_CALLBACK stateChangedCallback, 
    void* userContext)
{
    (void)filter;

    IOTHUB_MESSAGE_MONITORING_RESULT result;

    if (messageMonitoringHandle == NULL || stateChangedCallback == NULL)
    {
        LogError("Invalid argument (messageMonitoringHandle=%p, stateChangedCallback=%p)", messageMonitoringHandle, stateChangedCallback);
        result = IOTHUB_MESSAGE_MONITORING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING_LL* msgMon = (IOTHUB_MESSAGE_MONITORING_LL*)messageMonitoringHandle;

        if (msgMon->message_receiver != NULL)
        {
            LogError("Messaging is already opened");
            result = IOTHUB_MESSAGE_MONITORING_ERROR;
        }
        else
        {
            const IO_INTERFACE_DESCRIPTION* tlsio_interface;

            if ((tlsio_interface = platform_get_default_tlsio()) == NULL)
            {
                LogError("Could not get default TLS IO interface.");
                result = IOTHUB_MESSAGE_MONITORING_ERROR;
            }
            else
            {
                TLSIO_CONFIG tls_io_config;

                memset(&tls_io_config, 0, sizeof(TLSIO_CONFIG));

                tls_io_config.hostname = msgMon->hostname;
                tls_io_config.port = DEFAULT_IOTHUB_AMQP_PORT;

                if ((msgMon->tls_io = xio_create(tlsio_interface, &tls_io_config)) == NULL)
                {
                    LogError("Could not create TLS IO.");
                    result = IOTHUB_MESSAGE_MONITORING_ERROR;
                }
                else
                {
                    SASL_PLAIN_CONFIG sasl_plain_config;

                    memset(&sasl_plain_config, 0, sizeof(SASL_PLAIN_CONFIG));

                    if ((sasl_plain_config.authcid = createAuthCid(messageMonitoringHandle)) == NULL)
                    {
                        LogError("Could not create authCid");
                        IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                        result = IOTHUB_MESSAGE_MONITORING_ERROR;
                    }
                    else if ((sasl_plain_config.passwd = createSasToken(messageMonitoringHandle)) == NULL)
                    {
                        LogError("Could not create sasToken");
                        free((char*)sasl_plain_config.authcid);
                        IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                        result = IOTHUB_MESSAGE_MONITORING_ERROR;
                    }
                    else
                    {
                        const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_mechanism_interface;

                        if ((sasl_mechanism_interface = saslplain_get_interface()) == NULL)
                        {
                            LogError("Could not get SASL plain mechanism interface.");
                            IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                            result = IOTHUB_MESSAGE_MONITORING_ERROR;
                        }
                        else if ((msgMon->sasl_mechanism_handle = saslmechanism_create(sasl_mechanism_interface, &sasl_plain_config)) == NULL)
                        {
                            LogError("Could not create SASL plain mechanism.");
                            IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                            result = IOTHUB_MESSAGE_MONITORING_ERROR;
                        }
                        else
                        {
                            SASLCLIENTIO_CONFIG sasl_io_config;
                            sasl_io_config.sasl_mechanism = msgMon->sasl_mechanism_handle;
                            sasl_io_config.underlying_io = msgMon->tls_io;

                            const IO_INTERFACE_DESCRIPTION* saslclientio_interface;

                            if ((saslclientio_interface = saslclientio_get_interface_description()) == NULL)
                            {
                                LogError("Could not create get SASL IO interface description.");
                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                            }
                            else if ((msgMon->sasl_io = xio_create(saslclientio_interface, &sasl_io_config)) == NULL)
                            {
                                LogError("Could not create SASL IO.");
                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                            }
                            else if ((msgMon->connection = connection_create(msgMon->sasl_io, msgMon->hostname, DEFAULT_CONNECTION_CONTAINER_ID, NULL, NULL)) == NULL)
                            {
                                LogError("Could not create connection.");
                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                            }
                            else if ((msgMon->session = session_create(msgMon->connection, NULL, NULL)) == NULL)
                            {
                                LogError("Could not create session.");
                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                            }
                            else if (session_set_incoming_window(msgMon->session, DEFAULT_SESSION_INCOMING_WINDOW) != 0)
                            {
                                LogError("Could not set incoming window.");
                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                            }
                            else if (session_set_outgoing_window(msgMon->session, DEFAULT_SESSION_OUTGOING_WINDOW) != 0)
                            {
                                LogError("Could not set outgoing window.");
                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                            }
                            else
                            {
                                char* link_source_address;

                                if ((link_source_address = createLinkSourceAddress(messageMonitoringHandle, consumerGroup, partitionNumber)) == NULL)
                                {
                                    LogError("Could not create link source address");
                                    IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                    result = IOTHUB_MESSAGE_MONITORING_ERROR;
                                }
                                else
                                {
                                    AMQP_VALUE linkSource;

                                    if ((linkSource = messaging_create_source(link_source_address)) == NULL)
                                    {
                                        LogError("Could not create source for link.");
                                        IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                        result = IOTHUB_MESSAGE_MONITORING_ERROR;
                                    }
                                    else
                                    {
                                        AMQP_VALUE linkTarget;
                                
                                        if ((linkTarget = messaging_create_target(DEFAULT_LINK_TARGET_NAME)) == NULL)
                                        {
                                            LogError("Could not create target for link.");
                                            IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                            result = IOTHUB_MESSAGE_MONITORING_ERROR;
                                        }
                                        else
                                        {
                                            if ((msgMon->receiver_link = link_create(msgMon->session, DEFAULT_LINK_NAME, role_receiver, linkSource, linkTarget)) == NULL)
                                            {
                                                LogError("Could not create link.");
                                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                                            }
                                            else if (attachServiceClientTypeToLink(msgMon->receiver_link) != 0)
                                            {
                                                LogError("Could not set client version info.");
                                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                                            }
                                            else if (link_set_rcv_settle_mode(msgMon->receiver_link, receiver_settle_mode_first) != 0)
                                            {
                                                LogError("Could not set the receiver settle mode.");
                                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                                            }
                                            else if ((msgMon->message_receiver = messagereceiver_create(msgMon->receiver_link, on_messagereceiver_state_changed, messageMonitoringHandle)) == NULL)
                                            {
                                                LogError("Could not create message receiver.");
                                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                                            }
                                            else if (messagereceiver_open(msgMon->message_receiver, on_message_received, msgMon) != 0)
                                            {
                                                LogError("Could not open message receiver.");
                                                IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle);
                                                result = IOTHUB_MESSAGE_MONITORING_ERROR;
                                            }
                                            else
                                            {
                                                connection_set_trace(msgMon->connection, msgMon->enableLogging);
                                                messagereceiver_set_trace(msgMon->message_receiver, msgMon->enableLogging);

                                                msgMon->on_state_changed_callback = stateChangedCallback;
                                                msgMon->on_state_changed_context = userContext;
                                                result = IOTHUB_MESSAGE_MONITORING_OK;
                                            }

                                            amqpvalue_destroy(linkTarget);
                                        }

                                        amqpvalue_destroy(linkSource);
                                    }

                                    free(link_source_address);
                                }
                            }
                        }

                        free((char*)sasl_plain_config.authcid);
                        free((char*)sasl_plain_config.passwd);
                    }
                }
            }
        }
    }

    return result;
}

IOTHUB_MESSAGE_MONITORING_RESULT IoTHubMessageMonitoring_LL_SetMessageCallback(IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle, IOTHUB_MESSAGE_RECEIVED_CALLBACK messageReceivedCallback, void* userContextCallback)
{
    IOTHUB_MESSAGE_MONITORING_RESULT result;

    if (messageMonitoringHandle == NULL)
    {
        LogError("Invalid argument (messageMonitoringHandle=%p)", messageMonitoringHandle);
        result = IOTHUB_MESSAGE_MONITORING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING_LL* msgMon = (IOTHUB_MESSAGE_MONITORING_LL*)messageMonitoringHandle;

        msgMon->message_received_callback = messageReceivedCallback;
        msgMon->message_received_context = userContextCallback;

        result = IOTHUB_MESSAGE_MONITORING_OK;
    }
    return result;
}

void IoTHubMessageMonitoring_LL_DoWork(IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle)
{
    if (messageMonitoringHandle != NULL)
    {
        IOTHUB_MESSAGE_MONITORING_LL* msgMon = (IOTHUB_MESSAGE_MONITORING_LL*)messageMonitoringHandle;

        if (msgMon->connection != NULL)
        {
            connection_dowork(msgMon->connection);
        }
    }
}

IOTHUB_MESSAGE_MONITORING_RESULT IoTHubMessageMonitoring_LL_SetOption(IOTHUB_MESSAGE_MONITORING_LL_HANDLE messageMonitoringHandle, const char* name, const void* value)
{
    IOTHUB_MESSAGE_MONITORING_RESULT result;

    if (messageMonitoringHandle == NULL || name == NULL)
    {
        LogError("Invalid argument (messageMonitoringHandle=%p, name=%p)", messageMonitoringHandle, name);
        result = IOTHUB_MESSAGE_MONITORING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING_LL* msgMon = (IOTHUB_MESSAGE_MONITORING_LL*)messageMonitoringHandle;

        if (strcmp(name, OPTION_ENABLE_LOGGING) == 0)
        {
            msgMon->enableLogging = *((bool*)value);

            result = IOTHUB_MESSAGE_MONITORING_OK;
        }
        else
        {
            LogError("Invalid option %s", name);
            result = IOTHUB_MESSAGE_MONITORING_INVALID_ARG;
        }
    }

    return result;
}