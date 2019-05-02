// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <ctype.h>

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/urlencode.h"

#include "internal/iothub_client_private.h"
#include "internal/iothub_client_retry_control.h"
#include "internal/iothub_transport_ll_private.h"
#include "internal/iothubtransport_mqtt_common.h"
#include "internal/iothubtransport.h"

#include "azure_umqtt_c/mqtt_client.h"

#include "iothub_client_options.h"
#include "iothub_client_version.h"

#include <stdarg.h>
#include <stdio.h>

#include <limits.h>
#include <inttypes.h>

#define SAS_REFRESH_MULTIPLIER              .8
#define EPOCH_TIME_T_VALUE                  0
#define DEFAULT_MQTT_KEEPALIVE              4*60 // 4 min
#define DEFAULT_CONNACK_TIMEOUT             30 // 30 seconds
#define BUILD_CONFIG_USERNAME               24
#define SAS_TOKEN_DEFAULT_LEN               10
#define RESEND_TIMEOUT_VALUE_MIN            1*60
#define MAX_SEND_RECOUNT_LIMIT              2
#define DEFAULT_CONNECTION_INTERVAL         30
#define FAILED_CONN_BACKOFF_VALUE           5
#define STATUS_CODE_FAILURE_VALUE           500
#define STATUS_CODE_TIMEOUT_VALUE           408

#define DEFAULT_RETRY_POLICY                IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define DEFAULT_RETRY_TIMEOUT_IN_SECONDS    0
#define MAX_DISCONNECT_VALUE                50

#define ON_DEMAND_GET_TWIN_REQUEST_TIMEOUT_SECS    60

static const char TOPIC_DEVICE_TWIN_PREFIX[] = "$iothub/twin";
static const char TOPIC_DEVICE_METHOD_PREFIX[] = "$iothub/methods";

static const char* TOPIC_GET_DESIRED_STATE = "$iothub/twin/res/#";
static const char* TOPIC_NOTIFICATION_STATE = "$iothub/twin/PATCH/properties/desired/#";

static const char* TOPIC_DEVICE_MSG = "devices/%s/messages/devicebound/#";
static const char* TOPIC_DEVICE_MODULE_MSG = "devices/%s/modules/%s/messages/devicebound/#";
static const char* TOPIC_DEVICE_DEVICE = "devices/%s/messages/events/";
static const char* TOPIC_DEVICE_DEVICE_MODULE = "devices/%s/modules/%s/messages/events/";

static const char* TOPIC_INPUT_QUEUE_NAME = "devices/%s/modules/%s/#";

static const char* TOPIC_DEVICE_METHOD_SUBSCRIBE = "$iothub/methods/POST/#";

static const char* IOTHUB_API_VERSION = "2017-11-08-preview";

static const char* PROPERTY_SEPARATOR = "&";
static const char* REPORTED_PROPERTIES_TOPIC = "$iothub/twin/PATCH/properties/reported/?$rid=%"PRIu16;
static const char* GET_PROPERTIES_TOPIC = "$iothub/twin/GET/?$rid=%"PRIu16;
static const char* DEVICE_METHOD_RESPONSE_TOPIC = "$iothub/methods/res/%d/?$rid=%s";

static const char* REQUEST_ID_PROPERTY = "?$rid=";

static const char* MESSAGE_ID_PROPERTY = "mid";
static const char* CORRELATION_ID_PROPERTY = "cid";
static const char* CONTENT_TYPE_PROPERTY = "ct";
static const char* CONTENT_ENCODING_PROPERTY = "ce";
static const char* DIAGNOSTIC_ID_PROPERTY = "diagid";
static const char* DIAGNOSTIC_CONTEXT_PROPERTY = "diagctx";
static const char* CONNECTION_DEVICE_ID = "cdid";
static const char* CONNECTION_MODULE_ID_PROPERTY = "cmid";

static const char* DIAGNOSTIC_CONTEXT_CREATION_TIME_UTC_PROPERTY = "creationtimeutc";

#define TOLOWER(c) (((c>='A') && (c<='Z'))?c-'A'+'a':c)

#define UNSUBSCRIBE_FROM_TOPIC                  0x0000
#define SUBSCRIBE_GET_REPORTED_STATE_TOPIC      0x0001
#define SUBSCRIBE_NOTIFICATION_STATE_TOPIC      0x0002
#define SUBSCRIBE_TELEMETRY_TOPIC               0x0004
#define SUBSCRIBE_DEVICE_METHOD_TOPIC           0x0008
#define SUBSCRIBE_INPUT_QUEUE_TOPIC             0x0010
#define SUBSCRIBE_TOPIC_COUNT                   5

MU_DEFINE_ENUM_STRINGS(MQTT_CLIENT_EVENT_ERROR, MQTT_CLIENT_EVENT_ERROR_VALUES)

typedef struct SYSTEM_PROPERTY_INFO_TAG
{
    const char* propName;
    size_t propLength;
} SYSTEM_PROPERTY_INFO;

static SYSTEM_PROPERTY_INFO sysPropList[] = {
    { "%24.exp", 7 },
    { "%24.mid", 7 },
    { "%24.uid", 7 },
    { "%24.to", 6 },
    { "%24.cid", 7 },
    { "%24.ct", 6 },
    { "%24.ce", 6 },
    { "devices/", 8 },
    { "iothub-operation", 16 },
    { "iothub-ack", 10 },
    { "%24.on", 6 },
    { "%24.cdid", 8 },
    { "%24.cmid", 8 }
};

static const int slashes_to_reach_input_name = 5;

typedef enum DEVICE_TWIN_MSG_TYPE_TAG
{
    REPORTED_STATE,
    RETRIEVE_PROPERTIES
} DEVICE_TWIN_MSG_TYPE;

typedef enum MQTT_TRANSPORT_CREDENTIAL_TYPE_TAG
{
    CREDENTIAL_NOT_BUILD,
    X509,
    SAS_TOKEN_FROM_USER,
    DEVICE_KEY,
} MQTT_TRANSPORT_CREDENTIAL_TYPE;

typedef enum MQTT_CLIENT_STATUS_TAG
{
    MQTT_CLIENT_STATUS_NOT_CONNECTED,
    MQTT_CLIENT_STATUS_CONNECTING,
    MQTT_CLIENT_STATUS_CONNECTED,
    MQTT_CLIENT_STATUS_PENDING_CLOSE,
    MQTT_CLIENT_STATUS_EXECUTE_DISCONNECT
} MQTT_CLIENT_STATUS;

typedef struct MQTTTRANSPORT_HANDLE_DATA_TAG
{
    // Topic control
    STRING_HANDLE topic_MqttEvent;
    STRING_HANDLE topic_MqttMessage;
    STRING_HANDLE topic_GetState;
    STRING_HANDLE topic_NotifyState;
    STRING_HANDLE topic_InputQueue;

    STRING_HANDLE topic_DeviceMethods;

    uint32_t topics_ToSubscribe;

    // Connection related constants
    STRING_HANDLE hostAddress;
    STRING_HANDLE device_id;
    STRING_HANDLE module_id;
    STRING_HANDLE devicesAndModulesPath;
    int portNum;
    bool conn_attempted;

    MQTT_GET_IO_TRANSPORT get_io_transport;

    // The current mqtt iothub implementation requires that the hub name and the domain suffix be passed as the first of a series of segments
    // passed through the username portion of the connection frame.
    // The second segment will contain the device id.  The two segments are delemited by a "/".
    // The first segment can be a maximum 256 characters.
    // The second segment can be a maximum 128 characters.
    // With the / delimeter you have 384 chars (Plus a terminator of 0).
    STRING_HANDLE configPassedThroughUsername;

    // Protocol
    MQTT_CLIENT_HANDLE mqttClient;
    XIO_HANDLE xioTransport;

    // Session - connection
    uint16_t packetId;
    uint16_t twin_resp_packet_id;

    // Connection state control
    bool isRegistered;
    MQTT_CLIENT_STATUS mqttClientStatus;
    bool isDestroyCalled;
    bool isRetryExpiredCallbackSet;
    bool device_twin_get_sent;
    bool twin_resp_sub_recv;
    bool isRecoverableError;
    uint16_t keepAliveValue;
    uint16_t connect_timeout_in_sec;
    tickcounter_ms_t mqtt_connect_time;
    size_t connectFailCount;
    tickcounter_ms_t connectTick;
    bool log_trace;
    bool raw_trace;
    TICK_COUNTER_HANDLE msgTickCounter;
    OPTIONHANDLER_HANDLE saved_tls_options; // Here are the options from the xio layer if any is saved.

    // Internal lists for message tracking
    PDLIST_ENTRY waitingToSend;
    DLIST_ENTRY ack_waiting_queue;

    DLIST_ENTRY pending_get_twin_queue;

    // Message tracking
    CONTROL_PACKET_TYPE currPacketState;

    // Telemetry specific
    DLIST_ENTRY telemetry_waitingForAck;
    bool auto_url_encode_decode;

    // Controls frequency of reconnection logic.
    RETRY_CONTROL_HANDLE retry_control_handle;

    // Auth module used to generating handle authorization
    // with either SAS Token, x509 Certs, and Device SAS Token
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;

    TRANSPORT_CALLBACKS_INFO transport_callbacks;
    void* transport_ctx;

    char* http_proxy_hostname;
    int http_proxy_port;
    char* http_proxy_username;
    char* http_proxy_password;
    bool isProductInfoSet;
    int disconnect_recv_flag;
} MQTTTRANSPORT_HANDLE_DATA, *PMQTTTRANSPORT_HANDLE_DATA;

typedef struct MQTT_DEVICE_TWIN_ITEM_TAG
{
    tickcounter_ms_t msgEnqueueTime;
    tickcounter_ms_t msgPublishTime;
    size_t retryCount;
    IOTHUB_IDENTITY_TYPE iothub_type;
    uint16_t packet_id;
    uint32_t iothub_msg_id;
    IOTHUB_DEVICE_TWIN* device_twin_data;
    DEVICE_TWIN_MSG_TYPE device_twin_msg_type;
    DLIST_ENTRY entry;
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK userCallback;
    void* userContext;
} MQTT_DEVICE_TWIN_ITEM;

typedef struct MQTT_MESSAGE_DETAILS_LIST_TAG
{
    tickcounter_ms_t msgPublishTime;
    size_t retryCount;
    IOTHUB_MESSAGE_LIST* iotHubMessageEntry;
    void* context;
    uint16_t packet_id;
    DLIST_ENTRY entry;
} MQTT_MESSAGE_DETAILS_LIST, *PMQTT_MESSAGE_DETAILS_LIST;

typedef struct DEVICE_METHOD_INFO_TAG
{
    STRING_HANDLE request_id;
} DEVICE_METHOD_INFO;

static void free_proxy_data(MQTTTRANSPORT_HANDLE_DATA* mqtt_transport_instance)
{
    if (mqtt_transport_instance->http_proxy_hostname != NULL)
    {
        free(mqtt_transport_instance->http_proxy_hostname);
        mqtt_transport_instance->http_proxy_hostname = NULL;
    }

    if (mqtt_transport_instance->http_proxy_username != NULL)
    {
        free(mqtt_transport_instance->http_proxy_username);
        mqtt_transport_instance->http_proxy_username = NULL;
    }

    if (mqtt_transport_instance->http_proxy_password != NULL)
    {
        free(mqtt_transport_instance->http_proxy_password);
        mqtt_transport_instance->http_proxy_password = NULL;
    }
}

static void set_saved_tls_options(PMQTTTRANSPORT_HANDLE_DATA transport, OPTIONHANDLER_HANDLE new_options)
{
    if (transport->saved_tls_options != NULL)
    {
        OptionHandler_Destroy(transport->saved_tls_options);
    }
    transport->saved_tls_options = new_options;
}

static void free_transport_handle_data(MQTTTRANSPORT_HANDLE_DATA* transport_data)
{
    if (transport_data->mqttClient != NULL)
    {
        mqtt_client_deinit(transport_data->mqttClient);
    }

    if (transport_data->retry_control_handle != NULL)
    {
        retry_control_destroy(transport_data->retry_control_handle);
    }

    set_saved_tls_options(transport_data, NULL);

    tickcounter_destroy(transport_data->msgTickCounter);

    free_proxy_data(transport_data);

    STRING_delete(transport_data->devicesAndModulesPath);
    STRING_delete(transport_data->topic_MqttEvent);
    STRING_delete(transport_data->topic_MqttMessage);
    STRING_delete(transport_data->device_id);
    STRING_delete(transport_data->module_id);
    STRING_delete(transport_data->hostAddress);
    STRING_delete(transport_data->configPassedThroughUsername);
    STRING_delete(transport_data->topic_GetState);
    STRING_delete(transport_data->topic_NotifyState);
    STRING_delete(transport_data->topic_DeviceMethods);
    STRING_delete(transport_data->topic_InputQueue);

    free(transport_data);
}

int IoTHubTransport_MQTT_Common_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    int result;

    if (handle == NULL)
    {
        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_25_041: [**If any handle is NULL then IoTHubTransport_MQTT_Common_SetRetryPolicy shall return resultant line.] */
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else
    {
        RETRY_CONTROL_HANDLE new_retry_control_handle;

        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_006: [ IoTHubTransport_MQTT_Common_SetRetryPolicy shall set the retry logic by calling retry_control_create() with retry policy and retryTimeout as parameters]
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_009: [ If retry_control_create() fails then IoTHubTransport_MQTT_Common_SetRetryPolicy shall revert to previous retry policy and return non-zero value ]
        if ((new_retry_control_handle = retry_control_create(retryPolicy, (unsigned int)retryTimeoutLimitInSeconds)) == NULL)
        {
            LogError("Failed creating new retry control handle");
            result = MU_FAILURE;
        }
        else
        {
            PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
            RETRY_CONTROL_HANDLE previous_retry_control_handle = transport_data->retry_control_handle;

            transport_data->retry_control_handle = new_retry_control_handle;
            retry_control_destroy(previous_retry_control_handle);

            /*Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_25_045: [**If retry logic for specified parameters of retry policy and retryTimeoutLimitInSeconds is created successfully then IoTHubTransport_MQTT_Common_SetRetryPolicy shall return 0]*/
            result = 0;
        }
    }

    return result;
}

static uint16_t get_next_packet_id(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->packetId + 1 >= USHRT_MAX)
    {
        transport_data->packetId = 1;
    }
    else
    {
        transport_data->packetId++;
    }
    return transport_data->packetId;
}

#ifndef NO_LOGGING
static const char* retrieve_mqtt_return_codes(CONNECT_RETURN_CODE rtn_code)
{
    switch (rtn_code)
    {
        case CONNECTION_ACCEPTED:
            return "Accepted";
        case CONN_REFUSED_UNACCEPTABLE_VERSION:
            return "Unacceptable Version";
        case CONN_REFUSED_ID_REJECTED:
            return "Id Rejected";
        case CONN_REFUSED_SERVER_UNAVAIL:
            return "Server Unavailable";
        case CONN_REFUSED_BAD_USERNAME_PASSWORD:
            return "Bad Username/Password";
        case CONN_REFUSED_NOT_AUTHORIZED:
            return "Not Authorized";
        case CONN_REFUSED_UNKNOWN:
        default:
            return "Unknown";
    }
}
#endif // NO_LOGGING

static int retrieve_device_method_rid_info(const char* resp_topic, STRING_HANDLE method_name, STRING_HANDLE request_id)
{
    int result;
    STRING_TOKENIZER_HANDLE token_handle = STRING_TOKENIZER_create_from_char(resp_topic);
    if (token_handle == NULL)
    {
        LogError("Failed creating token from device twin topic.");
        result = MU_FAILURE;
    }
    else
    {
        STRING_HANDLE token_value;
        if ((token_value = STRING_new()) == NULL)
        {
            LogError("Failed allocating new string .");
            result = MU_FAILURE;
        }
        else
        {
            size_t token_index = 0;
            size_t request_id_length = strlen(REQUEST_ID_PROPERTY);
            result = MU_FAILURE;
            while (STRING_TOKENIZER_get_next_token(token_handle, token_value, "/") == 0)
            {
                if (token_index == 3)
                {
                    if (STRING_concat_with_STRING(method_name, token_value) != 0)
                    {
                        LogError("Failed STRING_concat_with_STRING.");
                        result = MU_FAILURE;
                        break;
                    }
                }
                else if (token_index == 4)
                {
                    if (STRING_length(token_value) >= request_id_length)
                    {
                        const char* request_id_value = STRING_c_str(token_value);
                        if (memcmp(request_id_value, REQUEST_ID_PROPERTY, request_id_length) == 0)
                        {
                            if (STRING_concat(request_id, request_id_value + request_id_length) != 0)
                            {
                                LogError("Failed STRING_concat failed.");
                                result = MU_FAILURE;
                            }
                            else
                            {
                                result = 0;
                            }
                            break;
                        }
                    }
                }
                token_index++;
            }
            STRING_delete(token_value);
        }
        STRING_TOKENIZER_destroy(token_handle);
    }
    return result;
}

static int parse_device_twin_topic_info(const char* resp_topic, bool* patch_msg, size_t* request_id, int* status_code)
{
    int result;
    STRING_TOKENIZER_HANDLE token_handle = STRING_TOKENIZER_create_from_char(resp_topic);
    if (token_handle == NULL)
    {
        LogError("Failed creating token from device twin topic.");
        result = MU_FAILURE;
        *status_code = 0;
        *request_id = 0;
        *patch_msg = false;
    }
    else
    {
        STRING_HANDLE token_value;
        if ((token_value = STRING_new()) == NULL)
        {
            LogError("Failed allocating new string .");
            result = MU_FAILURE;
            *status_code = 0;
            *request_id = 0;
            *patch_msg = false;
        }
        else
        {
            result = MU_FAILURE;
            size_t token_count = 0;
            while (STRING_TOKENIZER_get_next_token(token_handle, token_value, "/") == 0)
            {
                if (token_count == 2)
                {
                    if (strcmp(STRING_c_str(token_value), "PATCH") == 0)
                    {
                        *patch_msg = true;
                        *status_code = 0;
                        *request_id = 0;
                        result = 0;
                        break;
                    }
                    else
                    {
                        *patch_msg = false;
                    }
                }
                else if (token_count == 3)
                {
                    *status_code = (int)atol(STRING_c_str(token_value));
                    if (STRING_TOKENIZER_get_next_token(token_handle, token_value, "/?$rid=") == 0)
                    {
                        *request_id = (size_t)atol(STRING_c_str(token_value));
                    }
                    *patch_msg = false;
                    result = 0;
                    break;
                }
                token_count++;
            }
            STRING_delete(token_value);
        }
        STRING_TOKENIZER_destroy(token_handle);
    }
    return result;
}

static int InternStrnicmp(const char* s1, const char* s2, size_t n)
{
    int result;

    if (s1 == NULL) result = -1;
    else if (s2 == NULL) result = 1;
    else
    {
        result = 0;

        while (n-- && result == 0)
        {
            if (*s1 == 0) result = -1;
            else if (*s2 == 0) result = 1;
            else
            {

                result = TOLOWER(*s1) - TOLOWER(*s2);
                ++s1;
                ++s2;
            }
        }
    }
    return result;
}

static IOTHUB_IDENTITY_TYPE retrieve_topic_type(const char* topic_resp, const char* input_queue)
{
    IOTHUB_IDENTITY_TYPE type;
    if (InternStrnicmp(topic_resp, TOPIC_DEVICE_TWIN_PREFIX, sizeof(TOPIC_DEVICE_TWIN_PREFIX) - 1) == 0)
    {
        type = IOTHUB_TYPE_DEVICE_TWIN;
    }
    else if (InternStrnicmp(topic_resp, TOPIC_DEVICE_METHOD_PREFIX, sizeof(TOPIC_DEVICE_METHOD_PREFIX) - 1) == 0)
    {
        type = IOTHUB_TYPE_DEVICE_METHODS;
    }
    // input_queue contains additional "#" from subscribe, which we strip off on comparing incoming.
    else if ((input_queue != NULL) && InternStrnicmp(topic_resp, input_queue, strlen(input_queue) - 1) == 0)
    {
        type = IOTHUB_TYPE_EVENT_QUEUE;
    }
    else
    {
        type = IOTHUB_TYPE_TELEMETRY;
    }
    return type;

}

static void sendMsgComplete(IOTHUB_MESSAGE_LIST* iothubMsgList, PMQTTTRANSPORT_HANDLE_DATA transport_data, IOTHUB_CLIENT_CONFIRMATION_RESULT confirmResult)
{
    DLIST_ENTRY messageCompleted;
    DList_InitializeListHead(&messageCompleted);
    DList_InsertTailList(&messageCompleted, &(iothubMsgList->entry));
    transport_data->transport_callbacks.send_complete_cb(&messageCompleted, confirmResult, transport_data->transport_ctx);
}

static int addUserPropertiesTouMqttMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, STRING_HANDLE topic_string, size_t* index_ptr, bool urlencode)
{
    int result = 0;
    const char* const* propertyKeys;
    const char* const* propertyValues;
    size_t propertyCount;
    size_t index = *index_ptr;
    MAP_HANDLE properties_map = IoTHubMessage_Properties(iothub_message_handle);
    if (properties_map != NULL)
    {
        if (Map_GetInternals(properties_map, &propertyKeys, &propertyValues, &propertyCount) != MAP_OK)
        {
            LogError("Failed to get the internals of the property map.");
            result = MU_FAILURE;
        }
        else
        {
            if (propertyCount != 0)
            {
                for (index = 0; index < propertyCount && result == 0; index++)
                {
                    if (urlencode)
                    {
                        STRING_HANDLE property_key = URL_EncodeString(propertyKeys[index]);
                        STRING_HANDLE property_value = URL_EncodeString(propertyValues[index]);
                        if ((property_key == NULL) || (property_value == NULL))
                        {
                            LogError("Failed URL Encoding properties");
                            result = MU_FAILURE;
                        }
                        else if (STRING_sprintf(topic_string, "%s=%s%s", STRING_c_str(property_key), STRING_c_str(property_value), propertyCount - 1 == index ? "" : PROPERTY_SEPARATOR) != 0)
                        {
                            LogError("Failed constructing property string.");
                            result = MU_FAILURE;
                        }
                        STRING_delete(property_key);
                        STRING_delete(property_value);
                    }
                    else
                    {
                        if (STRING_sprintf(topic_string, "%s=%s%s", propertyKeys[index], propertyValues[index], propertyCount - 1 == index ? "" : PROPERTY_SEPARATOR) != 0)
                        {
                            LogError("Failed constructing property string.");
                            result = MU_FAILURE;
                        }
                    }
                }
            }
        }
    }
    *index_ptr = index;
    return result;
}

static int addSystemPropertyToTopicString(STRING_HANDLE topic_string, size_t index, const char* property_key, const char* property_value, bool urlencode)
{
    int result = 0;

    if (urlencode)
    {
        STRING_HANDLE encoded_property_value = URL_EncodeString(property_value);
        if (encoded_property_value == NULL)
        {
            LogError("Failed URL encoding %s.", property_key);
            result = MU_FAILURE;
        }
        else if (STRING_sprintf(topic_string, "%s%%24.%s=%s", index == 0 ? "" : PROPERTY_SEPARATOR, property_key, STRING_c_str(encoded_property_value)) != 0)
        {
            LogError("Failed setting %s.", property_key);
            result = MU_FAILURE;
        }
        STRING_delete(encoded_property_value);
    }
    else
    {
        if (STRING_sprintf(topic_string, "%s%%24.%s=%s", index == 0 ? "" : PROPERTY_SEPARATOR, property_key, property_value) != 0)
        {
            LogError("Failed setting %s.", property_key);
            result = MU_FAILURE;
        }
    }
    return result;
}

static int addSystemPropertiesTouMqttMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, STRING_HANDLE topic_string, size_t* index_ptr, bool urlencode)
{
    (void)urlencode;
    int result = 0;
    size_t index = *index_ptr;

    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_052: [ IoTHubTransport_MQTT_Common_DoWork shall check for the CorrelationId property and if found add the value as a system property in the format of $.cid=<id> ] */
    const char* correlation_id = IoTHubMessage_GetCorrelationId(iothub_message_handle);
    if (correlation_id != NULL)
    {
        result = addSystemPropertyToTopicString(topic_string, index, CORRELATION_ID_PROPERTY, correlation_id, urlencode);
        index++;
    }
    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_053: [ IoTHubTransport_MQTT_Common_DoWork shall check for the MessageId property and if found add the value as a system property in the format of $.mid=<id> ] */
    if (result == 0)
    {
        const char* msg_id = IoTHubMessage_GetMessageId(iothub_message_handle);
        if (msg_id != NULL)
        {
            result = addSystemPropertyToTopicString(topic_string, index, MESSAGE_ID_PROPERTY, msg_id, urlencode);
            index++;
        }
    }
    // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_010: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the ContentType property and if found add the `value` as a system property in the format of `$.ct=<value>` ]
    if (result == 0)
    {
        const char* content_type = IoTHubMessage_GetContentTypeSystemProperty(iothub_message_handle);
        if (content_type != NULL)
        {
            result = addSystemPropertyToTopicString(topic_string, index, CONTENT_TYPE_PROPERTY, content_type, urlencode);
            index++;
        }
    }
    // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_011: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the ContentEncoding property and if found add the `value` as a system property in the format of `$.ce=<value>` ]
    if (result == 0)
    {
        const char* content_encoding = IoTHubMessage_GetContentEncodingSystemProperty(iothub_message_handle);
        if (content_encoding != NULL)
        {
            result = addSystemPropertyToTopicString(topic_string, index, CONTENT_ENCODING_PROPERTY, content_encoding, urlencode);
            index++;
        }
    }
    *index_ptr = index;
    return result;
}

static int addDiagnosticPropertiesTouMqttMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, STRING_HANDLE topic_string, size_t* index_ptr)
{
    int result = 0;
    size_t index = *index_ptr;

    // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_014: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the diagnostic properties including diagid and diagCreationTimeUtc and if found both add them as system property in the format of `$.diagid` and `$.diagctx` respectively]
    const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* diagnosticData = IoTHubMessage_GetDiagnosticPropertyData(iothub_message_handle);
    if (diagnosticData != NULL)
    {
        const char* diag_id = diagnosticData->diagnosticId;
        const char* creation_time_utc = diagnosticData->diagnosticCreationTimeUtc;
        //diagid and creationtimeutc must be present/unpresent simultaneously
        if (diag_id != NULL && creation_time_utc != NULL)
        {
            if (STRING_sprintf(topic_string, "%s%%24.%s=%s", index == 0 ? "" : PROPERTY_SEPARATOR, DIAGNOSTIC_ID_PROPERTY, diag_id) != 0)
            {
                LogError("Failed setting diagnostic id");
                result = MU_FAILURE;
            }
            index++;

            if (result == 0)
            {
                //construct diagnostic context, it should be urlencode(key1=value1,key2=value2)
                STRING_HANDLE diagContextHandle = STRING_construct_sprintf("%s=%s", DIAGNOSTIC_CONTEXT_CREATION_TIME_UTC_PROPERTY, creation_time_utc);
                if (diagContextHandle == NULL)
                {
                    LogError("Failed constructing diagnostic context");
                    result = MU_FAILURE;
                }
                else
                {
                    //Add other diagnostic context properties here if have more
                    STRING_HANDLE encodedContextValueHandle = URL_Encode(diagContextHandle);
                    const char* encodedContextValueString = NULL;
                    if (encodedContextValueHandle != NULL &&
                        (encodedContextValueString = STRING_c_str(encodedContextValueHandle)) != NULL)
                    {
                        if (STRING_sprintf(topic_string, "%s%%24.%s=%s", index == 0 ? "" : PROPERTY_SEPARATOR, DIAGNOSTIC_CONTEXT_PROPERTY, encodedContextValueString) != 0)
                        {
                            LogError("Failed setting diagnostic context");
                            result = MU_FAILURE;
                        }
                        STRING_delete(encodedContextValueHandle);
                        encodedContextValueHandle = NULL;
                    }
                    else
                    {
                        LogError("Failed encoding diagnostic context value");
                        result = MU_FAILURE;
                    }
                    STRING_delete(diagContextHandle);
                    diagContextHandle = NULL;
                    index++;
                }
            }
        }
        else if (diag_id != NULL || creation_time_utc != NULL)
        {
            // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_015: [ `IoTHubTransport_MQTT_Common_DoWork` shall check whether diagid and diagCreationTimeUtc be present simultaneously, treat as error if not]
            LogError("diagid and diagcreationtimeutc must be present simultaneously.");
            result = MU_FAILURE;
        }
    }
    return result;
}


static STRING_HANDLE addPropertiesTouMqttMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, const char* eventTopic, bool urlencode)
{
    size_t index = 0;
    STRING_HANDLE result = STRING_construct(eventTopic);
    if (result == NULL)
    {
        LogError("Failed to create event topic string handle");
    }
    else if (addUserPropertiesTouMqttMessage(iothub_message_handle, result, &index, urlencode) != 0)
    {
        LogError("Failed adding Properties to uMQTT Message");
        STRING_delete(result);
        result = NULL;
    }
    else if (addSystemPropertiesTouMqttMessage(iothub_message_handle, result, &index, urlencode) != 0)
    {
        LogError("Failed adding System Properties to uMQTT Message");
        STRING_delete(result);
        result = NULL;
    }
    else if (addDiagnosticPropertiesTouMqttMessage(iothub_message_handle, result, &index) != 0)
    {
        LogError("Failed adding Diagnostic Properties to uMQTT Message");
        STRING_delete(result);
        result = NULL;
    }

    // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_060: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the OutputName property and if found add the value as a system property in the format of $.on=<value> ]
    if (result != NULL)
    {
        const char* output_name = IoTHubMessage_GetOutputName(iothub_message_handle);
        if (output_name != NULL)
        {
            if (STRING_sprintf(result, "%s%%24.on=%s/", index == 0 ? "" : PROPERTY_SEPARATOR, output_name) != 0)
            {
                LogError("Failed setting output name.");
                STRING_delete(result);
                result = NULL;
            }
            index++;
        }
    }

    return result;
}

static int publish_mqtt_telemetry_msg(PMQTTTRANSPORT_HANDLE_DATA transport_data, MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry, const unsigned char* payload, size_t len)
{
    int result;
    STRING_HANDLE msgTopic = addPropertiesTouMqttMessage(mqttMsgEntry->iotHubMessageEntry->messageHandle, STRING_c_str(transport_data->topic_MqttEvent), transport_data->auto_url_encode_decode);
    if (msgTopic == NULL)
    {
        LogError("Failed adding properties to mqtt message");
        result = MU_FAILURE;
    }
    else
    {
        MQTT_MESSAGE_HANDLE mqttMsg = mqttmessage_create_in_place(mqttMsgEntry->packet_id, STRING_c_str(msgTopic), DELIVER_AT_LEAST_ONCE, payload, len);
        if (mqttMsg == NULL)
        {
            LogError("Failed creating mqtt message");
            result = MU_FAILURE;
        }
        else
        {
            if (tickcounter_get_current_ms(transport_data->msgTickCounter, &mqttMsgEntry->msgPublishTime) != 0)
            {
                LogError("Failed retrieving tickcounter info");
                result = MU_FAILURE;
            }
            else
            {
                if (mqtt_client_publish(transport_data->mqttClient, mqttMsg) != 0)
                {
                    LogError("Failed attempting to publish mqtt message");
                    result = MU_FAILURE;
                }
                else
                {
                    mqttMsgEntry->retryCount++;
                    result = 0;
                }
            }
            mqttmessage_destroy(mqttMsg);
        }
        STRING_delete(msgTopic);
    }
    return result;
}

static int publish_device_method_message(MQTTTRANSPORT_HANDLE_DATA* transport_data, int status_code, STRING_HANDLE request_id, const unsigned char* response, size_t response_size)
{
    int result;
    uint16_t packet_id = get_next_packet_id(transport_data);

    STRING_HANDLE msg_topic = STRING_construct_sprintf(DEVICE_METHOD_RESPONSE_TOPIC, status_code, STRING_c_str(request_id));
    if (msg_topic == NULL)
    {
        LogError("Failed constructing message topic.");
        result = MU_FAILURE;
    }
    else
    {
        MQTT_MESSAGE_HANDLE mqtt_get_msg = mqttmessage_create_in_place(packet_id, STRING_c_str(msg_topic), DELIVER_AT_MOST_ONCE, response, response_size);
        if (mqtt_get_msg == NULL)
        {
            LogError("Failed constructing mqtt message.");
            result = MU_FAILURE;
        }
        else
        {
            if (mqtt_client_publish(transport_data->mqttClient, mqtt_get_msg) != 0)
            {
                LogError("Failed publishing to mqtt client.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
            mqttmessage_destroy(mqtt_get_msg);
        }
        STRING_delete(msg_topic);
    }
    return result;
}


static void destroy_device_twin_get_message(MQTT_DEVICE_TWIN_ITEM* msg_entry)
{
    free(msg_entry);
}

static MQTT_DEVICE_TWIN_ITEM* create_device_twin_get_message(MQTTTRANSPORT_HANDLE_DATA* transport_data)
{
    MQTT_DEVICE_TWIN_ITEM* result;

    if ((result = (MQTT_DEVICE_TWIN_ITEM*)malloc(sizeof(MQTT_DEVICE_TWIN_ITEM))) == NULL)
    {
        LogError("Failed allocating device twin data.");
    }
    else
    {
        result->packet_id = get_next_packet_id(transport_data);
        result->iothub_msg_id = 0;
        result->device_twin_msg_type = RETRIEVE_PROPERTIES;
        result->retryCount = 0;
        result->msgPublishTime = 0;
        result->msgEnqueueTime = 0;
        result->iothub_type = IOTHUB_TYPE_DEVICE_TWIN;
        result->device_twin_data = NULL;
        result->userCallback = NULL;
        result->userContext = NULL;
    }

    return result;
}

static int publish_device_twin_get_message(MQTTTRANSPORT_HANDLE_DATA* transport_data, MQTT_DEVICE_TWIN_ITEM* mqtt_info)
{
    int result;

    STRING_HANDLE msg_topic = STRING_construct_sprintf(GET_PROPERTIES_TOPIC, mqtt_info->packet_id);
    if (msg_topic == NULL)
    {
        LogError("Failed constructing get Prop topic.");
        result = MU_FAILURE;
    }
    else
    {
        MQTT_MESSAGE_HANDLE mqtt_get_msg = mqttmessage_create(mqtt_info->packet_id, STRING_c_str(msg_topic), DELIVER_AT_MOST_ONCE, NULL, 0);
        if (mqtt_get_msg == NULL)
        {
            LogError("Failed constructing mqtt message.");
            result = MU_FAILURE;
        }
        else
        {
            if (mqtt_client_publish(transport_data->mqttClient, mqtt_get_msg) != 0)
            {
                LogError("Failed publishing to mqtt client.");
                result = MU_FAILURE;
            }
            else
            {
                DList_InsertTailList(&transport_data->ack_waiting_queue, &mqtt_info->entry);
                result = 0;
            }
            mqttmessage_destroy(mqtt_get_msg);
        }
        STRING_delete(msg_topic);
    }

    return result;
}

static void sendPendingGetTwinRequests(PMQTTTRANSPORT_HANDLE_DATA transportData)
{
    PDLIST_ENTRY dev_twin_item = transportData->pending_get_twin_queue.Flink;

    while (dev_twin_item != &transportData->pending_get_twin_queue)
    {
        DLIST_ENTRY saveListEntry;
        saveListEntry.Flink = dev_twin_item->Flink;
        MQTT_DEVICE_TWIN_ITEM* msg_entry = containingRecord(dev_twin_item, MQTT_DEVICE_TWIN_ITEM, entry);
        (void)DList_RemoveEntryList(dev_twin_item);

        if (publish_device_twin_get_message(transportData, msg_entry) != 0)
        {
            LogError("Failed sending pending get twin request");
            destroy_device_twin_get_message(msg_entry);
        }
        else
        {
            transportData->device_twin_get_sent = true;
        }

        dev_twin_item = saveListEntry.Flink;
    }
}

static void removeExpiredPendingGetTwinRequests(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    tickcounter_ms_t current_ms;

    if (tickcounter_get_current_ms(transport_data->msgTickCounter, &current_ms) == 0)
    {
        PDLIST_ENTRY listItem = transport_data->pending_get_twin_queue.Flink;

        while (listItem != &transport_data->pending_get_twin_queue)
        {
            DLIST_ENTRY nextListItem;
            nextListItem.Flink = listItem->Flink;
            MQTT_DEVICE_TWIN_ITEM* msg_entry = containingRecord(listItem, MQTT_DEVICE_TWIN_ITEM, entry);

            if (((current_ms - msg_entry->msgEnqueueTime) / 1000) >= ON_DEMAND_GET_TWIN_REQUEST_TIMEOUT_SECS)
            {
                (void)DList_RemoveEntryList(listItem);
                msg_entry->userCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, msg_entry->userContext);
                destroy_device_twin_get_message(msg_entry);
            }

            listItem = nextListItem.Flink;
        }
    }
}

static void removeExpiredGetTwinRequestsPendingAck(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    tickcounter_ms_t current_ms;

    if (tickcounter_get_current_ms(transport_data->msgTickCounter, &current_ms) == 0)
    {
        PDLIST_ENTRY listItem = transport_data->ack_waiting_queue.Flink;

        while (listItem != &transport_data->ack_waiting_queue)
        {
            DLIST_ENTRY nextListItem;
            nextListItem.Flink = listItem->Flink;
            MQTT_DEVICE_TWIN_ITEM* msg_entry = containingRecord(listItem, MQTT_DEVICE_TWIN_ITEM, entry);

            // Check if it is a on-demand get-twin request.
            if (msg_entry->device_twin_msg_type == RETRIEVE_PROPERTIES && msg_entry->userCallback != NULL)
            {
                if (((current_ms - msg_entry->msgEnqueueTime) / 1000) >= ON_DEMAND_GET_TWIN_REQUEST_TIMEOUT_SECS)
                {
                    (void)DList_RemoveEntryList(listItem);
                    msg_entry->userCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, msg_entry->userContext);
                    destroy_device_twin_get_message(msg_entry);
                }
            }

            listItem = nextListItem.Flink;
        }
    }
}

static int publish_device_twin_message(MQTTTRANSPORT_HANDLE_DATA* transport_data, IOTHUB_DEVICE_TWIN* device_twin_info, MQTT_DEVICE_TWIN_ITEM* mqtt_info)
{
    int result;
    mqtt_info->packet_id = get_next_packet_id(transport_data);
    mqtt_info->device_twin_msg_type = REPORTED_STATE;

    STRING_HANDLE msgTopic = STRING_construct_sprintf(REPORTED_PROPERTIES_TOPIC, mqtt_info->packet_id);
    if (msgTopic == NULL)
    {
        LogError("Failed constructing reported prop topic.");
        result = MU_FAILURE;
    }
    else
    {
        const CONSTBUFFER* data_buff;
        if ((data_buff = CONSTBUFFER_GetContent(device_twin_info->report_data_handle)) == NULL)
        {
            LogError("Failed retrieving buffer content");
            result = MU_FAILURE;
        }
        else
        {
            MQTT_MESSAGE_HANDLE mqtt_rpt_msg = mqttmessage_create_in_place(mqtt_info->packet_id, STRING_c_str(msgTopic), DELIVER_AT_MOST_ONCE, data_buff->buffer, data_buff->size);
            if (mqtt_rpt_msg == NULL)
            {
                LogError("Failed creating mqtt message");
                result = MU_FAILURE;
            }
            else
            {
                if (tickcounter_get_current_ms(transport_data->msgTickCounter, &mqtt_info->msgPublishTime) != 0)
                {
                    LogError("Failed retrieving tickcounter info");
                    result = MU_FAILURE;
                }
                else
                {
                    if (mqtt_client_publish(transport_data->mqttClient, mqtt_rpt_msg) != 0)
                    {
                        LogError("Failed publishing mqtt message");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        mqtt_info->retryCount++;
                        result = 0;
                    }
                }
                mqttmessage_destroy(mqtt_rpt_msg);
            }
        }
        STRING_delete(msgTopic);
    }
    return result;
}

static void changeStateToSubscribeIfAllowed(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->currPacketState != CONNACK_TYPE &&
        transport_data->currPacketState != CONNECT_TYPE &&
        transport_data->currPacketState != DISCONNECT_TYPE &&
        transport_data->currPacketState != PACKET_TYPE_ERROR)
    {
        transport_data->currPacketState = SUBSCRIBE_TYPE;
    }
}

static int subscribeToNotifyStateIfNeeded(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result;

    if (transport_data->topic_NotifyState == NULL)
    {
        transport_data->topic_NotifyState = STRING_construct(TOPIC_NOTIFICATION_STATE);
        if (transport_data->topic_NotifyState == NULL)
        {
            LogError("Failure: unable constructing notify state topic");
            result = MU_FAILURE;
        }
        else
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
            result = 0;
        }
    }
    else
    {
        result = 0;
    }

    if (result == 0)
    {
        changeStateToSubscribeIfAllowed(transport_data);
    }

    return result;
}


static bool isSystemProperty(const char* tokenData)
{
    bool result = false;
    size_t propCount = sizeof(sysPropList) / sizeof(sysPropList[0]);
    size_t index = 0;
    for (index = 0; index < propCount; index++)
    {
        if (memcmp(tokenData, sysPropList[index].propName, sysPropList[index].propLength) == 0)
        {
            result = true;
            break;
        }
    }
    return result;
}

// Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_061: [ If the message is sent to an input queue, `IoTHubTransport_MQTT_Common_DoWork` shall parse out to the input queue name and store it in the message with IoTHubMessage_SetInputName ]
// Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_062: [ If IoTHubTransport_MQTT_Common_DoWork receives a malformatted inputQueue, it shall fail ]
static int addInputNamePropertyToMessage(IOTHUB_MESSAGE_HANDLE IoTHubMessage, const char* topic_name)
{
    int result = MU_FAILURE;
    int number_tokens_read = 0;

    STRING_TOKENIZER_HANDLE token_handle = STRING_TOKENIZER_create_from_char(topic_name);
    if (token_handle == NULL)
    {
        LogError("STRING_TOKENIZER_create_from_char failed\n");
        result = MU_FAILURE;
    }
    else
    {
        STRING_HANDLE token_value;
        if ((token_value = STRING_new()) == NULL)
        {
            LogError("Failed allocating token_value");
        }
        else
        {
            while (STRING_TOKENIZER_get_next_token(token_handle, token_value, "/") == 0)
            {
                number_tokens_read++;
                if (number_tokens_read == (slashes_to_reach_input_name + 1))
                {
                    if ((IOTHUB_MESSAGE_OK != IoTHubMessage_SetInputName(IoTHubMessage, STRING_c_str(token_value))))
                    {
                        LogError("Failed adding input name to msg");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                    break;
                }
            }
        }
        STRING_delete(token_value);

        if (number_tokens_read != (slashes_to_reach_input_name + 1))
        {
            LogError("Not enough '/' to contain input name.  Got %d, need at least %d", number_tokens_read, (slashes_to_reach_input_name + 1));
            result = MU_FAILURE;
        }
        STRING_TOKENIZER_destroy(token_handle);
    }

    return result;
}

static int setMqttMessagePropertyIfPossible(IOTHUB_MESSAGE_HANDLE IoTHubMessage, const char* propName, const char* propValue, size_t nameLen)
{
    // Not finding a system property to map to isn't an error.
    int result = 0;

    if (nameLen > 4)
    {
        if (strcmp((const char*)&propName[nameLen - 4], CONNECTION_DEVICE_ID) == 0)
        {
            // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_063: [ If type is IOTHUB_TYPE_TELEMETRY and the system property `$.cdid` is defined, its value shall be set on the IOTHUB_MESSAGE_HANDLE's ConnectionDeviceId property ]
            if (IoTHubMessage_SetConnectionDeviceId(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'messageId' property.");
                result = MU_FAILURE;
            }
            return result;
        }
        if (strcmp((const char*)&propName[nameLen - 4], CONNECTION_MODULE_ID_PROPERTY) == 0)
        {
            // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_064: [ If type is IOTHUB_TYPE_TELEMETRY and the system property `$.cmid` is defined, its value shall be set on the IOTHUB_MESSAGE_HANDLE's ConnectionModuleId property ]
            if (IoTHubMessage_SetConnectionModuleId(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'correlationId' property.");
                result = MU_FAILURE;
            }
            return result;
        }
    }
    if (nameLen > 3)
    {
        if (strcmp((const char*)&propName[nameLen - 3], MESSAGE_ID_PROPERTY) == 0)
        {
            if (IoTHubMessage_SetMessageId(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'messageId' property.");
                result = MU_FAILURE;
            }
            return result;
        }
        else if (strcmp((const char*)&propName[nameLen - 3], CORRELATION_ID_PROPERTY) == 0)
        {
            if (IoTHubMessage_SetCorrelationId(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'correlationId' property.");
                result = MU_FAILURE;
            }
            return result;
        }
    }

    if (nameLen > 2)
    {
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_012: [ If type is IOTHUB_TYPE_TELEMETRY and the system property `$.ct` is defined, its value shall be set on the IOTHUB_MESSAGE_HANDLE's ContentType property ]
        if (strcmp((const char*)&propName[nameLen - 2], CONTENT_TYPE_PROPERTY) == 0)
        {
            if (IoTHubMessage_SetContentTypeSystemProperty(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'customContentType' property.");
                result = MU_FAILURE;
            }
            return result;
        }
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_013: [ If type is IOTHUB_TYPE_TELEMETRY and the system property `$.ce` is defined, its value shall be set on the IOTHUB_MESSAGE_HANDLE's ContentEncoding property ]
        else if (strcmp((const char*)&propName[nameLen - 2], CONTENT_ENCODING_PROPERTY) == 0)
        {
            if (IoTHubMessage_SetContentEncodingSystemProperty(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'contentEncoding' property.");
                result = MU_FAILURE;
            }
            return result;
        }
    }

    return result;
}

static int extractMqttProperties(IOTHUB_MESSAGE_HANDLE IoTHubMessage, const char* topic_name, bool urldecode)
{
    int result;
    STRING_TOKENIZER_HANDLE token = STRING_TOKENIZER_create_from_char(topic_name);
    if (token != NULL)
    {
        MAP_HANDLE propertyMap = IoTHubMessage_Properties(IoTHubMessage);
        if (propertyMap == NULL)
        {
            LogError("Failure to retrieve IoTHubMessage_properties.");
            result = MU_FAILURE;
        }
        else
        {
            STRING_HANDLE output = STRING_new();
            if (output == NULL)
            {
                LogError("Failure to allocate STRING_new.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;

                while (STRING_TOKENIZER_get_next_token(token, output, PROPERTY_SEPARATOR) == 0 && result == 0)
                {
                    const char* tokenData = STRING_c_str(output);
                    size_t tokenLen = strlen(tokenData);
                    if (tokenData == NULL || tokenLen == 0)
                    {
                        break;
                    }
                    else
                    {
                        if (isSystemProperty(tokenData))
                        {
                            const char* iterator = tokenData;
                            while (iterator != NULL && *iterator != '\0' && result == 0)
                            {
                                if (*iterator == '=')
                                {
                                    char* propName = NULL;
                                    char* propValue = NULL;
                                    size_t nameLen = iterator - tokenData;
                                    size_t valLen = tokenLen - (nameLen + 1) + 1;

                                    if ((propName = malloc(nameLen + 1)) == NULL || (propValue = malloc(valLen + 1)) == NULL)
                                    {
                                        LogError("Failed allocating property name (%p) and/or value (%p)", propName, propValue);
                                        free(propName);
                                        result = MU_FAILURE;
                                    }
                                    else
                                    {
                                        memcpy(propName, tokenData, nameLen);
                                        propName[nameLen] = '\0';

                                        memcpy(propValue, iterator + 1, valLen);
                                        propValue[valLen] = '\0';

                                        if (urldecode)
                                        {
                                            STRING_HANDLE propValue_decoded;
                                            if ((propValue_decoded = URL_DecodeString(propValue)) == NULL)
                                            {
                                                LogError("Failed to URL decode property value");
                                                result = MU_FAILURE;
                                            }
                                            else if (setMqttMessagePropertyIfPossible(IoTHubMessage, propName, STRING_c_str(propValue_decoded), nameLen) != 0)
                                            {
                                                LogError("Unable to set message property");
                                                result = MU_FAILURE;
                                            }
                                            STRING_delete(propValue_decoded);
                                        }
                                        else
                                        {
                                            if (setMqttMessagePropertyIfPossible(IoTHubMessage, propName, propValue, nameLen) != 0)
                                            {
                                                LogError("Unable to set message property");
                                                result = MU_FAILURE;
                                            }
                                        }
                                        free(propName);
                                        free(propValue);
                                    }
                                    break;
                                }
                                iterator++;
                            }
                        }
                        else //User Properties
                        {
                            const char* iterator = tokenData;
                            while (iterator != NULL && *iterator != '\0' && result == 0)
                            {
                                if (*iterator == '=')
                                {
                                    char* propName = NULL;
                                    char* propValue = NULL;
                                    size_t nameLen = iterator - tokenData;
                                    size_t valLen = tokenLen - (nameLen + 1) + 1;

                                    if ((propName = (char*)malloc(nameLen + 1)) == NULL || (propValue = (char*)malloc(valLen + 1)) == NULL)
                                    {
                                        free(propName);
                                        LogError("Failed allocating property information");
                                        result = MU_FAILURE;
                                    }
                                    else
                                    {
                                        memcpy(propName, tokenData, nameLen);
                                        propName[nameLen] = '\0';

                                        memcpy(propValue, iterator + 1, valLen);
                                        propValue[valLen] = '\0';

                                        if (urldecode)
                                        {
                                            STRING_HANDLE propName_decoded = URL_DecodeString(propName);
                                            STRING_HANDLE propValue_decoded = URL_DecodeString(propValue);
                                            if (propName_decoded == NULL || propValue_decoded == NULL)
                                            {
                                                LogError("Failed to URL decode property");
                                                result = MU_FAILURE;
                                            }
                                            else if (Map_AddOrUpdate(propertyMap, STRING_c_str(propName_decoded), STRING_c_str(propValue_decoded)) != MAP_OK)
                                            {
                                                LogError("Map_AddOrUpdate failed.");
                                                result = MU_FAILURE;
                                            }
                                            STRING_delete(propName_decoded);
                                            STRING_delete(propValue_decoded);
                                        }
                                        else
                                        {
                                            if (Map_AddOrUpdate(propertyMap, propName, propValue) != MAP_OK)
                                            {
                                                LogError("Map_AddOrUpdate failed.");
                                                result = MU_FAILURE;
                                            }
                                        }
                                        free(propName);
                                        free(propValue);
                                    }
                                    break;
                                }
                                iterator++;
                            }
                        }
                    }
                }
                STRING_delete(output);
            }
        }
        STRING_TOKENIZER_destroy(token);
    }
    else
    {
        LogError("Unable to create Tokenizer object.");
        result = MU_FAILURE;
    }
    return result;
}

static void mqtt_notification_callback(MQTT_MESSAGE_HANDLE msgHandle, void* callbackCtx)
{
    /* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_051: [ If msgHandle or callbackCtx is NULL, mqtt_notification_callback shall do nothing. ] */
    if (msgHandle != NULL && callbackCtx != NULL)
    {
        /* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_052: [ mqtt_notification_callback shall extract the topic Name from the MQTT_MESSAGE_HANDLE. ] */
        const char* topic_resp = mqttmessage_getTopicName(msgHandle);
        if (topic_resp == NULL)
        {
            LogError("Failure: NULL topic name encountered");
        }
        else
        {
            PMQTTTRANSPORT_HANDLE_DATA transportData = (PMQTTTRANSPORT_HANDLE_DATA)callbackCtx;

            IOTHUB_IDENTITY_TYPE type = retrieve_topic_type(topic_resp, STRING_c_str(transportData->topic_InputQueue));
            if (type == IOTHUB_TYPE_DEVICE_TWIN)
            {
                size_t request_id;
                int status_code;
                bool notification_msg;
                if (parse_device_twin_topic_info(topic_resp, &notification_msg, &request_id, &status_code) != 0)
                {
                    LogError("Failure: parsing device topic info");
                }
                else
                {
                    const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(msgHandle);
                    if (notification_msg)
                    {
                        transportData->transport_callbacks.twin_retrieve_prop_complete_cb(DEVICE_TWIN_UPDATE_PARTIAL, payload->message, payload->length, transportData->transport_ctx);
                    }
                    else
                    {
                        PDLIST_ENTRY dev_twin_item = transportData->ack_waiting_queue.Flink;
                        while (dev_twin_item != &transportData->ack_waiting_queue)
                        {
                            DLIST_ENTRY saveListEntry;
                            saveListEntry.Flink = dev_twin_item->Flink;
                            MQTT_DEVICE_TWIN_ITEM* msg_entry = containingRecord(dev_twin_item, MQTT_DEVICE_TWIN_ITEM, entry);
                            if (request_id == msg_entry->packet_id)
                            {
                                (void)DList_RemoveEntryList(dev_twin_item);
                                if (msg_entry->device_twin_msg_type == RETRIEVE_PROPERTIES)
                                {
                                    if (msg_entry->userCallback == NULL)
                                    {
                                        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ If type is IOTHUB_TYPE_DEVICE_TWIN, then on success if msg_type is RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClientCore_LL_RetrievePropertyComplete... ] */
                                        transportData->transport_callbacks.twin_retrieve_prop_complete_cb(DEVICE_TWIN_UPDATE_COMPLETE, payload->message, payload->length, transportData->transport_ctx);
                                        // Only after receiving device twin request should we start listening for patches.
                                        (void)subscribeToNotifyStateIfNeeded(transportData);
                                    }
                                    else
                                    {
                                        // This is a on-demand get twin request.
                                        msg_entry->userCallback(DEVICE_TWIN_UPDATE_COMPLETE, payload->message, payload->length, msg_entry->userContext);
                                    }
                                }
                                else
                                {
                                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_055: [ if device_twin_msg_type is not RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClientCore_LL_ReportedStateComplete ] */
                                    transportData->transport_callbacks.twin_rpt_state_complete_cb(msg_entry->iothub_msg_id, status_code, transportData->transport_ctx);
                                    // Only after receiving device twin request should we start listening for patches.
                                    (void)subscribeToNotifyStateIfNeeded(transportData);
                                }

                                destroy_device_twin_get_message(msg_entry);
                                break;
                            }
                            dev_twin_item = saveListEntry.Flink;
                        }
                    }
                }
            }
            else if (type == IOTHUB_TYPE_DEVICE_METHODS)
            {
                STRING_HANDLE method_name = STRING_new();
                if (method_name == NULL)
                {
                    LogError("Failure: allocating method_name string value");
                }
                else
                {
                    DEVICE_METHOD_INFO* dev_method_info = malloc(sizeof(DEVICE_METHOD_INFO));
                    if (dev_method_info == NULL)
                    {
                        LogError("Failure: allocating DEVICE_METHOD_INFO object");
                    }
                    else
                    {
                        dev_method_info->request_id = STRING_new();
                        if (dev_method_info->request_id == NULL)
                        {
                            LogError("Failure constructing request_id string");
                            free(dev_method_info);
                        }
                        else if (retrieve_device_method_rid_info(topic_resp, method_name, dev_method_info->request_id) != 0)
                        {
                            LogError("Failure: retrieve device topic info");
                            STRING_delete(dev_method_info->request_id);
                            free(dev_method_info);
                        }
                        else
                        {
                            /* CodesSRS_IOTHUB_MQTT_TRANSPORT_07_053: [ If type is IOTHUB_TYPE_DEVICE_METHODS, then on success mqtt_notification_callback shall call IoTHubClientCore_LL_DeviceMethodComplete. ] */
                            const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(msgHandle);
                            if (transportData->transport_callbacks.method_complete_cb(STRING_c_str(method_name), payload->message, payload->length, (void*)dev_method_info, transportData->transport_ctx) != 0)
                            {
                                LogError("Failure: IoTHubClientCore_LL_DeviceMethodComplete");
                                STRING_delete(dev_method_info->request_id);
                                free(dev_method_info);
                            }
                        }
                    }
                    STRING_delete(method_name);
                }
            }
            else
            {
                const APP_PAYLOAD* appPayload = mqttmessage_getApplicationMsg(msgHandle);
                IOTHUB_MESSAGE_HANDLE IoTHubMessage = IoTHubMessage_CreateFromByteArray(appPayload->message, appPayload->length);
                if (IoTHubMessage == NULL)
                {
                    LogError("Failure: IotHub Message creation has failed.");
                }
                else
                {
                    if ((type == IOTHUB_TYPE_EVENT_QUEUE) && (addInputNamePropertyToMessage(IoTHubMessage, topic_resp) != 0))
                    {
                        LogError("failure adding input name to property.");
                    }
                    // Will need to update this when the service has messages that can be rejected
                    else if (extractMqttProperties(IoTHubMessage, topic_resp, transportData->auto_url_encode_decode) != 0)
                    {
                        LogError("failure extracting mqtt properties.");
                    }
                    else
                    {
                        MESSAGE_CALLBACK_INFO* messageData = (MESSAGE_CALLBACK_INFO*)malloc(sizeof(MESSAGE_CALLBACK_INFO));
                        if (messageData == NULL)
                        {
                            LogError("malloc failed");
                        }
                        else
                        {
                            messageData->messageHandle = IoTHubMessage;
                            messageData->transportContext = NULL;

                            if (type == IOTHUB_TYPE_EVENT_QUEUE)
                            {
                                // Codes_SRS_IOTHUB_MQTT_TRANSPORT_31_065: [ If type is IOTHUB_TYPE_TELEMETRY and sent to an input queue, then on success `mqtt_notification_callback` shall call `IoTHubClient_LL_MessageCallback`. ]
                                if (!transportData->transport_callbacks.msg_input_cb(messageData, transportData->transport_ctx))
                                {
                                    LogError("IoTHubClientCore_LL_MessageCallbackreturned false");

                                    IoTHubMessage_Destroy(IoTHubMessage);
                                    free(messageData);
                                }
                            }
                            else
                            {
                                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_056: [ If type is IOTHUB_TYPE_TELEMETRY, then on success mqtt_notification_callback shall call IoTHubClientCore_LL_MessageCallback. ] */
                                if (!transportData->transport_callbacks.msg_cb(messageData, transportData->transport_ctx))
                                {
                                    LogError("IoTHubClientCore_LL_MessageCallback returned false");
                                    IoTHubMessage_Destroy(IoTHubMessage);
                                    free(messageData);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static void mqtt_operation_complete_callback(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_RESULT actionResult, const void* msgInfo, void* callbackCtx)
{
    (void)handle;
    if (callbackCtx != NULL)
    {
        PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)callbackCtx;

        switch (actionResult)
        {
            case MQTT_CLIENT_ON_PUBLISH_ACK:
            case MQTT_CLIENT_ON_PUBLISH_COMP:
            {
                const PUBLISH_ACK* puback = (const PUBLISH_ACK*)msgInfo;
                if (puback != NULL)
                {
                    PDLIST_ENTRY currentListEntry = transport_data->telemetry_waitingForAck.Flink;
                    while (currentListEntry != &transport_data->telemetry_waitingForAck)
                    {
                        MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry = containingRecord(currentListEntry, MQTT_MESSAGE_DETAILS_LIST, entry);
                        DLIST_ENTRY saveListEntry;
                        saveListEntry.Flink = currentListEntry->Flink;

                        if (puback->packetId == mqttMsgEntry->packet_id)
                        {
                            (void)DList_RemoveEntryList(currentListEntry); //First remove the item from Waiting for Ack List.
                            sendMsgComplete(mqttMsgEntry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_OK);
                            free(mqttMsgEntry);
                        }
                        currentListEntry = saveListEntry.Flink;
                    }
                }
                else
                {
                    LogError("Failure: MQTT_CLIENT_ON_PUBLISH_ACK publish_ack structure NULL.");
                }
                break;
            }
            case MQTT_CLIENT_ON_CONNACK:
            {
                const CONNECT_ACK* connack = (const CONNECT_ACK*)msgInfo;
                if (connack != NULL)
                {
                    if (connack->returnCode == CONNECTION_ACCEPTED)
                    {
                        // The connect packet has been acked
                        transport_data->currPacketState = CONNACK_TYPE;
                        transport_data->isRecoverableError = true;
                        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_CONNECTED;

                        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_008: [ Upon successful connection the retry control shall be reset using retry_control_reset() ]
                        retry_control_reset(transport_data->retry_control_handle);

                        transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, transport_data->transport_ctx);
                    }
                    else
                    {
                        if (connack->returnCode == CONN_REFUSED_SERVER_UNAVAIL)
                        {
                            transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED, transport_data->transport_ctx);
                        }
                        else if (connack->returnCode == CONN_REFUSED_BAD_USERNAME_PASSWORD || connack->returnCode == CONN_REFUSED_ID_REJECTED)
                        {
                            transport_data->isRecoverableError = false;
                            transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL, transport_data->transport_ctx);
                        }
                        else if (connack->returnCode == CONN_REFUSED_NOT_AUTHORIZED)
                        {
                            transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED, transport_data->transport_ctx);
                        }
                        else if (connack->returnCode == CONN_REFUSED_UNACCEPTABLE_VERSION)
                        {
                            transport_data->isRecoverableError = false;
                        }
                        LogError("Connection Not Accepted: 0x%x: %s", connack->returnCode, retrieve_mqtt_return_codes(connack->returnCode));
                        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_PENDING_CLOSE;
                        transport_data->currPacketState = PACKET_TYPE_ERROR;
                    }
                }
                else
                {
                    LogError("MQTT_CLIENT_ON_CONNACK CONNACK parameter is NULL.");
                }
                break;
            }
            case MQTT_CLIENT_ON_SUBSCRIBE_ACK:
            {
                const SUBSCRIBE_ACK* suback = (const SUBSCRIBE_ACK*)msgInfo;
                if (suback != NULL)
                {
                    size_t index = 0;
                    for (index = 0; index < suback->qosCount; index++)
                    {
                        if (suback->qosReturn[index] == DELIVER_FAILURE)
                        {
                            LogError("Subscribe delivery failure of subscribe %lu", (unsigned long)index);
                        }
                    }
                    // The subscribed packet has been acked
                    transport_data->currPacketState = SUBACK_TYPE;

                    // Is this a twin message
                    if (suback->packetId == transport_data->twin_resp_packet_id)
                    {
                        transport_data->twin_resp_sub_recv = true;
                    }
                }
                else
                {
                    LogError("Failure: MQTT_CLIENT_ON_SUBSCRIBE_ACK SUBSCRIBE_ACK parameter is NULL.");
                }
                break;
            }
            case MQTT_CLIENT_ON_PUBLISH_RECV:
            case MQTT_CLIENT_ON_PUBLISH_REL:
            {
                // Currently not used
                break;
            }
            case MQTT_CLIENT_ON_DISCONNECT:
            {
                // Close the client so we can reconnect again
                transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
                break;
            }
            case MQTT_CLIENT_ON_UNSUBSCRIBE_ACK:
            case MQTT_CLIENT_ON_PING_RESPONSE:
            default:
            {
                break;
            }
        }
    }
}

// Prior to creating a new connection, if we have an existing xioTransport that has been connected before
// we need to clear it now or else cached settings (especially TLS when communicating with HTTP proxies)
// will break reconnection attempt.
static void ResetConnectionIfNecessary(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->xioTransport != NULL && transport_data->conn_attempted)
    {
        OPTIONHANDLER_HANDLE options = xio_retrieveoptions(transport_data->xioTransport);
        set_saved_tls_options(transport_data, options);

        xio_destroy(transport_data->xioTransport);
        transport_data->xioTransport = NULL;
    }
}

static void mqtt_disconnect_cb(void* ctx)
{
    if (ctx != NULL)
    {
        int* disconn_recv = (int*)ctx;
        *disconn_recv = 1;
    }
}

static void DisconnectFromClient(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->currPacketState != DISCONNECT_TYPE)
    {
        if (!transport_data->isDestroyCalled)
        {
            OPTIONHANDLER_HANDLE options = xio_retrieveoptions(transport_data->xioTransport);
            set_saved_tls_options(transport_data, options);
        }
        // Ensure the disconnect message is sent
        if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_CONNECTED)
        {
            transport_data->disconnect_recv_flag = 0;
            (void)mqtt_client_disconnect(transport_data->mqttClient, mqtt_disconnect_cb, &transport_data->disconnect_recv_flag);
            size_t disconnect_ctr = 0;
            do
            {
                mqtt_client_dowork(transport_data->mqttClient);
                disconnect_ctr++;
                ThreadAPI_Sleep(50);
            } while ((disconnect_ctr < MAX_DISCONNECT_VALUE) && (transport_data->disconnect_recv_flag == 0));
        }
        xio_destroy(transport_data->xioTransport);
        transport_data->xioTransport = NULL;

        transport_data->device_twin_get_sent = false;
        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
        transport_data->currPacketState = DISCONNECT_TYPE;
    }
}

static void mqtt_error_callback(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_ERROR error, void* callbackCtx)
{
    (void)handle;
    if (callbackCtx != NULL)
    {
        PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)callbackCtx;
        switch (error)
        {
            case MQTT_CLIENT_CONNECTION_ERROR:
            {
                transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_NETWORK, transport_data->transport_ctx);
                break;
            }
            case MQTT_CLIENT_COMMUNICATION_ERROR:
            {
                transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, transport_data->transport_ctx);
                break;
            }
            case MQTT_CLIENT_NO_PING_RESPONSE:
            {
                LogError("Mqtt Ping Response was not encountered.  Reconnecting device...");
                break;
            }
            case MQTT_CLIENT_PARSE_ERROR:
            case MQTT_CLIENT_MEMORY_ERROR:
            case MQTT_CLIENT_UNKNOWN_ERROR:
            {
                LogError("INTERNAL ERROR: unexpected error value received %s", MU_ENUM_TO_STRING(MQTT_CLIENT_EVENT_ERROR, error));
                break;
            }
        }
        if (transport_data->mqttClientStatus != MQTT_CLIENT_STATUS_PENDING_CLOSE)
        {
            // We have encountered an mqtt protocol error in an non-closing state
            // The best course of action is to execute a shutdown of the mqtt/tls/socket
            // layer and then attempt to reconnect
            transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_EXECUTE_DISCONNECT;
        }
        transport_data->currPacketState = PACKET_TYPE_ERROR;
        transport_data->device_twin_get_sent = false;
        if (transport_data->topic_MqttMessage != NULL)
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_TELEMETRY_TOPIC;
        }
        if (transport_data->topic_GetState != NULL)
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
        }
        if (transport_data->topic_NotifyState != NULL)
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
        }
        if (transport_data->topic_DeviceMethods != NULL)
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_DEVICE_METHOD_TOPIC;
        }
        if (transport_data->topic_InputQueue != NULL)
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_INPUT_QUEUE_TOPIC;
        }
    }
    else
    {
        LogError("Failure: mqtt called back with null context.");
    }
}

static void SubscribeToMqttProtocol(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->topics_ToSubscribe != UNSUBSCRIBE_FROM_TOPIC)
    {
        uint32_t topic_subscription = 0;
        size_t subscribe_count = 0;
        uint16_t packet_id = get_next_packet_id(transport_data);
        SUBSCRIBE_PAYLOAD subscribe[SUBSCRIBE_TOPIC_COUNT];
        if ((transport_data->topic_MqttMessage != NULL) && (SUBSCRIBE_TELEMETRY_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_MqttMessage);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_LEAST_ONCE;
            topic_subscription |= SUBSCRIBE_TELEMETRY_TOPIC;
            subscribe_count++;
        }
        if ((transport_data->topic_GetState != NULL) && (SUBSCRIBE_GET_REPORTED_STATE_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_GetState);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_MOST_ONCE;
            topic_subscription |= SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
            subscribe_count++;
            transport_data->twin_resp_packet_id = packet_id;
        }
        if ((transport_data->topic_NotifyState != NULL) && (SUBSCRIBE_NOTIFICATION_STATE_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_NotifyState);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_MOST_ONCE;
            topic_subscription |= SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
            subscribe_count++;
        }
        if ((transport_data->topic_DeviceMethods != NULL) && (SUBSCRIBE_DEVICE_METHOD_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_DeviceMethods);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_MOST_ONCE;
            topic_subscription |= SUBSCRIBE_DEVICE_METHOD_TOPIC;
            subscribe_count++;
        }
        if ((transport_data->topic_InputQueue != NULL) && (SUBSCRIBE_INPUT_QUEUE_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_InputQueue);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_MOST_ONCE;
            topic_subscription |= SUBSCRIBE_INPUT_QUEUE_TOPIC;
            subscribe_count++;
        }

        if (subscribe_count != 0)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_016: [IoTHubTransport_MQTT_Common_Subscribe shall call mqtt_client_subscribe to subscribe to the Message Topic.] */
            if (mqtt_client_subscribe(transport_data->mqttClient, packet_id, subscribe, subscribe_count) != 0)
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_017: [Upon failure IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
                LogError("Failure: mqtt_client_subscribe returned error.");
            }
            else
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_018: [On success IoTHubTransport_MQTT_Common_Subscribe shall return 0.] */
                transport_data->topics_ToSubscribe &= ~topic_subscription;
                transport_data->currPacketState = SUBSCRIBE_TYPE;
            }
        }
        else
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_017: [Upon failure IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
            LogError("Failure: subscribe_topic is empty.");
        }
        transport_data->currPacketState = SUBSCRIBE_TYPE;
    }
    else
    {
        transport_data->currPacketState = PUBLISH_TYPE;
    }
}

static bool RetrieveMessagePayload(IOTHUB_MESSAGE_HANDLE messageHandle, const unsigned char** payload, size_t* length)
{
    bool result;
    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(messageHandle);
    if (contentType == IOTHUBMESSAGE_BYTEARRAY)
    {
        if (IoTHubMessage_GetByteArray(messageHandle, &(*payload), length) != IOTHUB_MESSAGE_OK)
        {
            LogError("Failure result from IoTHubMessage_GetByteArray");
            result = false;
            *length = 0;
        }
        else
        {
            result = true;
        }
    }
    else if (contentType == IOTHUBMESSAGE_STRING)
    {
        *payload = (const unsigned char*)IoTHubMessage_GetString(messageHandle);
        if (*payload == NULL)
        {
            LogError("Failure result from IoTHubMessage_GetString");
            result = false;
            *length = 0;
        }
        else
        {
            *length = strlen((const char*)*payload);
            result = true;
        }
    }
    else
    {
        result = false;
        *length = 0;
    }
    return result;
}

static void process_queued_ack_messages(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    PDLIST_ENTRY current_entry = transport_data->telemetry_waitingForAck.Flink;
    tickcounter_ms_t current_ms;
    (void)tickcounter_get_current_ms(transport_data->msgTickCounter, &current_ms);
    while (current_entry != &transport_data->telemetry_waitingForAck)
    {
        MQTT_MESSAGE_DETAILS_LIST* msg_detail_entry = containingRecord(current_entry, MQTT_MESSAGE_DETAILS_LIST, entry);
        DLIST_ENTRY nextListEntry;
        nextListEntry.Flink = current_entry->Flink;

        if (((current_ms - msg_detail_entry->msgPublishTime) / 1000) > RESEND_TIMEOUT_VALUE_MIN)
        {
            if (msg_detail_entry->retryCount >= MAX_SEND_RECOUNT_LIMIT)
            {
                sendMsgComplete(msg_detail_entry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT);
                (void)DList_RemoveEntryList(current_entry);
                free(msg_detail_entry);

                DisconnectFromClient(transport_data);
            }
            else
            {
                // Ensure that the packet state is PUBLISH_TYPE and then attempt to send the message
                // again
                if (transport_data->currPacketState == PUBLISH_TYPE)
                {
                    size_t messageLength;
                    const unsigned char* messagePayload = NULL;
                    if (!RetrieveMessagePayload(msg_detail_entry->iotHubMessageEntry->messageHandle, &messagePayload, &messageLength))
                    {
                        (void)DList_RemoveEntryList(current_entry);
                        sendMsgComplete(msg_detail_entry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
                    }
                    else
                    {
                        if (publish_mqtt_telemetry_msg(transport_data, msg_detail_entry, messagePayload, messageLength) != 0)
                        {
                            (void)DList_RemoveEntryList(current_entry);
                            sendMsgComplete(msg_detail_entry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
                            free(msg_detail_entry);
                        }
                    }
                }
                else
                {
                    msg_detail_entry->retryCount++;
                    msg_detail_entry->msgPublishTime = current_ms;
                }
            }
        }
        current_entry = nextListEntry.Flink;
    }
}

static int GetTransportProviderIfNecessary(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result;

    if (transport_data->xioTransport == NULL)
    {
        // construct address
        const char* hostAddress = STRING_c_str(transport_data->hostAddress);
        MQTT_TRANSPORT_PROXY_OPTIONS mqtt_proxy_options;

        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_011: [ If no `proxy_data` option has been set, NULL shall be passed as the argument `mqtt_transport_proxy_options` when calling the function `get_io_transport` passed in `IoTHubTransport_MQTT_Common__Create`. ]*/
        mqtt_proxy_options.host_address = transport_data->http_proxy_hostname;
        mqtt_proxy_options.port = transport_data->http_proxy_port;
        mqtt_proxy_options.username = transport_data->http_proxy_username;
        mqtt_proxy_options.password = transport_data->http_proxy_password;

        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_010: [ If the `proxy_data` option has been set, the proxy options shall be filled in the argument `mqtt_transport_proxy_options` when calling the function `get_io_transport` passed in `IoTHubTransport_MQTT_Common__Create` to obtain the underlying IO handle. ]*/
        transport_data->xioTransport = transport_data->get_io_transport(hostAddress, (transport_data->http_proxy_hostname == NULL) ? NULL : &mqtt_proxy_options);
        if (transport_data->xioTransport == NULL)
        {
            LogError("Unable to create the lower level TLS layer.");
            result = MU_FAILURE;
        }
        else
        {
            if (transport_data->saved_tls_options != NULL)
            {
                if (OptionHandler_FeedOptions(transport_data->saved_tls_options, transport_data->xioTransport) != OPTIONHANDLER_OK)
                {
                    LogError("Failed feeding existing options to new TLS instance.");
                    result = MU_FAILURE;
                }
                else
                {
                    // The tlsio has the options, so our copy can be deleted
                    set_saved_tls_options(transport_data, NULL);
                    result = 0;
                }
            }
            else if (IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module) == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
            {
                if (IoTHubClient_Auth_Set_xio_Certificate(transport_data->authorization_module, transport_data->xioTransport) != 0)
                {
                    LogError("Unable to create the lower level TLS layer.");
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
            }
            else
            {
                result = 0;
            }
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

static STRING_HANDLE buildClientId(const char* device_id, const char* module_id)
{
    if (module_id == NULL)
    {
        return STRING_construct_sprintf("%s", device_id);
    }
    else
    {
        return STRING_construct_sprintf("%s/%s", device_id, module_id);
    }
}

static int SendMqttConnectMsg(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result;

    char* sasToken = NULL;
    result = 0;

    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module);
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY || cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH)
    {
        sasToken = IoTHubClient_Auth_Get_SasToken(transport_data->authorization_module, STRING_c_str(transport_data->devicesAndModulesPath), 0, NULL);
        if (sasToken == NULL)
        {
            LogError("failure getting sas token from IoTHubClient_Auth_Get_SasToken.");
            result = MU_FAILURE;
        }
    }
    else if (cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
    {
        SAS_TOKEN_STATUS token_status = IoTHubClient_Auth_Is_SasToken_Valid(transport_data->authorization_module);
        if (token_status == SAS_TOKEN_STATUS_INVALID)
        {
            transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN, transport_data->transport_ctx);
            result = MU_FAILURE;
        }
        else if (token_status == SAS_TOKEN_STATUS_FAILED)
        {
            transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL, transport_data->transport_ctx);
            result = MU_FAILURE;
        }
        else
        {
            sasToken = IoTHubClient_Auth_Get_SasToken(transport_data->authorization_module, NULL, 0, NULL);
            if (sasToken == NULL)
            {
                LogError("failure getting sas Token.");
                result = MU_FAILURE;
            }
        }
    }

    if (result == 0)
    {
        if (!transport_data->isProductInfoSet)
        {
            // This requires the iothubClientHandle, which sadly the MQTT transport only gets on DoWork, so this code still needs to remain here.
            // The correct place for this would be in the Create method, but we don't get the client handle there.
            // Also, when device multiplexing is used, the customer creates the transport directly and explicitly, when the client is still not created.
            // This will be a major hurdle when we add device multiplexing to MQTT transport.

            STRING_HANDLE clone;
            const char* product_info = transport_data->transport_callbacks.prod_info_cb(transport_data->transport_ctx);
            if (product_info == NULL)
            {
                clone = STRING_construct_sprintf("%s%%2F%s", CLIENT_DEVICE_TYPE_PREFIX, IOTHUB_SDK_VERSION);
            }
            else
            {
                clone = URL_EncodeString(product_info);
            }

            if (clone == NULL)
            {
                LogError("Failed obtaining the product info");
            }
            else
            {
                if (STRING_concat_with_STRING(transport_data->configPassedThroughUsername, clone) != 0)
                {
                    LogError("Failed concatenating the product info");
                }
                else
                {
                    transport_data->isProductInfoSet = true;
                }

                STRING_delete(clone);
            }
        }

        STRING_HANDLE clientId;

        clientId = buildClientId(STRING_c_str(transport_data->device_id), STRING_c_str(transport_data->module_id));
        if (NULL == clientId)
        {
            LogError("Unable to allocate clientId");
            result = MU_FAILURE;
        }
        else
        {
            MQTT_CLIENT_OPTIONS options = { 0 };
            options.clientId = (char*)STRING_c_str(clientId);
            options.willMessage = NULL;
            options.username = (char*)STRING_c_str(transport_data->configPassedThroughUsername);
            if (sasToken != NULL)
            {
                options.password = sasToken;
            }
            options.keepAliveInterval = transport_data->keepAliveValue;
            options.useCleanSession = false;
            options.qualityOfServiceValue = DELIVER_AT_LEAST_ONCE;

            if (GetTransportProviderIfNecessary(transport_data) == 0)
            {
                transport_data->conn_attempted = true;
                if (mqtt_client_connect(transport_data->mqttClient, transport_data->xioTransport, &options) != 0)
                {
                    LogError("failure connecting to address %s.", STRING_c_str(transport_data->hostAddress));
                    result = MU_FAILURE;
                }
                else
                {
                    transport_data->currPacketState = CONNECT_TYPE;
                    transport_data->isRetryExpiredCallbackSet = false;
                    (void)tickcounter_get_current_ms(transport_data->msgTickCounter, &transport_data->mqtt_connect_time);
                    result = 0;
                }
            }
            else
            {
                result = MU_FAILURE;
            }

            if (sasToken != NULL)
            {
                free(sasToken);
            }
            STRING_delete(clientId);
        }

    }
    return result;
}

static int InitializeConnection(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result = 0;

    // Make sure we're not destroying the object
    if (!transport_data->isDestroyCalled)
    {
        RETRY_ACTION retry_action = RETRY_ACTION_RETRY_LATER;

        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_007: [ IoTHubTransport_MQTT_Common_DoWork shall try to reconnect according to the current retry policy set ]
        if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_NOT_CONNECTED && transport_data->isRecoverableError)
        {
            // Note: in case retry_control_should_retry fails, the reconnection shall be attempted anyway (defaulting to policy IOTHUB_CLIENT_RETRY_IMMEDIATE).
            if (retry_control_should_retry(transport_data->retry_control_handle, &retry_action) != 0 || retry_action == RETRY_ACTION_RETRY_NOW)
            {
                if (tickcounter_get_current_ms(transport_data->msgTickCounter, &transport_data->connectTick) != 0)
                {
                    transport_data->connectFailCount++;
                    result = MU_FAILURE;
                }
                else
                {
                    ResetConnectionIfNecessary(transport_data);

                    if (SendMqttConnectMsg(transport_data) != 0)
                    {
                        transport_data->connectFailCount++;
                        result = MU_FAILURE;
                    }
                    else
                    {
                        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_CONNECTING;
                        transport_data->connectFailCount = 0;
                        result = 0;
                    }
                }
            }   
            else if (retry_action == RETRY_ACTION_STOP_RETRYING)
            {
                // Set callback if retry expired
                if (!transport_data->isRetryExpiredCallbackSet)
                {
                    transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED, transport_data->transport_ctx);
                    transport_data->isRetryExpiredCallbackSet = true;
                }
                result = MU_FAILURE;
            }
            else if (retry_action == RETRY_ACTION_RETRY_LATER)
            {
                result = MU_FAILURE;
            }
        }
        else if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_EXECUTE_DISCONNECT)
        {
            // Need to disconnect from client
            DisconnectFromClient(transport_data);
            result = 0;
        }
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_001: [ IoTHubTransport_MQTT_Common_DoWork shall trigger reconnection if the mqtt_client_connect does not complete within `keepalive` seconds]
        else if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_CONNECTING)
        {
            tickcounter_ms_t current_time;
            if (tickcounter_get_current_ms(transport_data->msgTickCounter, &current_time) != 0)
            {
                LogError("failed verifying MQTT_CLIENT_STATUS_CONNECTING timeout");
                result = MU_FAILURE;
            }
            else if ((current_time - transport_data->mqtt_connect_time) / 1000 > transport_data->connect_timeout_in_sec)
            {
                LogError("mqtt_client timed out waiting for CONNACK");
                transport_data->currPacketState = PACKET_TYPE_ERROR;
                DisconnectFromClient(transport_data);
                result = MU_FAILURE;
            }
        }
        else if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_CONNECTED)
        {
            // We are connected and not being closed, so does SAS need to reconnect?
            tickcounter_ms_t current_time;
            if (tickcounter_get_current_ms(transport_data->msgTickCounter, &current_time) != 0)
            {
                transport_data->connectFailCount++;
                result = MU_FAILURE;
            }
            else
            {
                IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module);
                // If the credential type is not an x509 certificate then we shall renew the Sas_Token
                if (cred_type != IOTHUB_CREDENTIAL_TYPE_X509 && cred_type != IOTHUB_CREDENTIAL_TYPE_X509_ECC)
                {
                    size_t sas_token_expiry = IoTHubClient_Auth_Get_SasToken_Expiry(transport_data->authorization_module);
                    if ((current_time - transport_data->mqtt_connect_time) / 1000 > (sas_token_expiry*SAS_REFRESH_MULTIPLIER))
                    {
                        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_058: [ If the sas token has timed out IoTHubTransport_MQTT_Common_DoWork shall disconnect from the mqtt client and destroy the transport information and wait for reconnect. ] */
                        DisconnectFromClient(transport_data);

                        transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN, transport_data->transport_ctx);
                        transport_data->currPacketState = UNKNOWN_TYPE;
                        if (transport_data->topic_MqttMessage != NULL)
                        {
                            transport_data->topics_ToSubscribe |= SUBSCRIBE_TELEMETRY_TOPIC;
                        }
                        if (transport_data->topic_GetState != NULL)
                        {
                            transport_data->topics_ToSubscribe |= SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
                        }
                        if (transport_data->topic_NotifyState != NULL)
                        {
                            transport_data->topics_ToSubscribe |= SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
                        }
                        if (transport_data->topic_DeviceMethods != NULL)
                        {
                            transport_data->topics_ToSubscribe |= SUBSCRIBE_DEVICE_METHOD_TOPIC;
                        }
                        if (transport_data->topic_InputQueue != NULL)
                        {
                            transport_data->topics_ToSubscribe |= SUBSCRIBE_INPUT_QUEUE_TOPIC;
                        }
                    }
                }
            }
        }
    }
    return result;
}

static STRING_HANDLE buildConfigForUsername(const IOTHUB_CLIENT_CONFIG* upperConfig, const char* moduleId)
{
    if (moduleId == NULL)
    {
        return STRING_construct_sprintf("%s.%s/%s/?api-version=%s&DeviceClientType=", upperConfig->iotHubName, upperConfig->iotHubSuffix, upperConfig->deviceId, IOTHUB_API_VERSION);
    }
    else
    {
        return STRING_construct_sprintf("%s.%s/%s/%s/?api-version=%s&DeviceClientType=", upperConfig->iotHubName, upperConfig->iotHubSuffix, upperConfig->deviceId, moduleId, IOTHUB_API_VERSION);
    }
}

static STRING_HANDLE buildMqttEventString(const char* device_id, const char* module_id)
{
    if (module_id == NULL)
    {
        return STRING_construct_sprintf(TOPIC_DEVICE_DEVICE, device_id);
    }
    else
    {
        return STRING_construct_sprintf(TOPIC_DEVICE_DEVICE_MODULE, device_id, module_id);
    }
}

static STRING_HANDLE buildDevicesAndModulesPath(const IOTHUB_CLIENT_CONFIG* upperConfig, const char* moduleId)
{
    if (moduleId == NULL)
    {
        return STRING_construct_sprintf("%s.%s/devices/%s", upperConfig->iotHubName, upperConfig->iotHubSuffix, upperConfig->deviceId);
    }
    else
    {
        return STRING_construct_sprintf("%s.%s/devices/%s/modules/%s", upperConfig->iotHubName, upperConfig->iotHubSuffix, upperConfig->deviceId, moduleId);
    }
}

static PMQTTTRANSPORT_HANDLE_DATA InitializeTransportHandleData(const IOTHUB_CLIENT_CONFIG* upperConfig, PDLIST_ENTRY waitingToSend, IOTHUB_AUTHORIZATION_HANDLE auth_module, const char* moduleId)
{
    PMQTTTRANSPORT_HANDLE_DATA state = (PMQTTTRANSPORT_HANDLE_DATA)malloc(sizeof(MQTTTRANSPORT_HANDLE_DATA));
    if (state == NULL)
    {
        LogError("Could not create MQTT transport state. Memory allocation failed.");
    }
    else
    {
        memset(state, 0, sizeof(MQTTTRANSPORT_HANDLE_DATA));
        if ((state->msgTickCounter = tickcounter_create()) == NULL)
        {
            LogError("Invalid Argument: iotHubName is empty");
            free_transport_handle_data(state);
            state = NULL;
        }
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_005: [ MQTT transport shall use EXPONENTIAL_WITH_BACK_OFF as default retry policy ]
        else if ((state->retry_control_handle = retry_control_create(DEFAULT_RETRY_POLICY, DEFAULT_RETRY_TIMEOUT_IN_SECONDS)) == NULL)
        {
            LogError("Failed creating default retry control");
            free_transport_handle_data(state);
            state = NULL;
        }
        else if ((state->device_id = STRING_construct(upperConfig->deviceId)) == NULL)
        {
            LogError("failure constructing device_id.");
            free_transport_handle_data(state);
            state = NULL;
        }
        else if ((moduleId != NULL) && ((state->module_id = STRING_construct(moduleId)) == NULL))
        {
            LogError("failure constructing module_id.");
            free_transport_handle_data(state);
            state = NULL;
        }
        else if ((state->devicesAndModulesPath = buildDevicesAndModulesPath(upperConfig, moduleId)) == NULL)
        {
            LogError("failure constructing devicesPath.");
            free_transport_handle_data(state);
            state = NULL;
        }
        else
        {
            if ((state->topic_MqttEvent = buildMqttEventString(upperConfig->deviceId, moduleId)) == NULL)
            {
                LogError("Could not create topic_MqttEvent for MQTT");
                free_transport_handle_data(state);
                state = NULL;
            }
            else
            {
                state->mqttClient = mqtt_client_init(mqtt_notification_callback, mqtt_operation_complete_callback, state, mqtt_error_callback, state);
                if (state->mqttClient == NULL)
                {
                    LogError("failure initializing mqtt client.");
                    free_transport_handle_data(state);
                    state = NULL;
                }
                else
                {
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_008: [If the upperConfig contains a valid protocolGatewayHostName value the this shall be used for the hostname, otherwise the hostname shall be constructed using the iothubname and iothubSuffix.] */
                    if (upperConfig->protocolGatewayHostName == NULL)
                    {
                        state->hostAddress = STRING_construct_sprintf("%s.%s", upperConfig->iotHubName, upperConfig->iotHubSuffix);
                    }
                    else
                    {
                        state->hostAddress = STRING_construct(upperConfig->protocolGatewayHostName);
                    }

                    if (state->hostAddress == NULL)
                    {
                        LogError("failure constructing host address.");
                        free_transport_handle_data(state);
                        state = NULL;
                    }
                    else if ((state->configPassedThroughUsername = buildConfigForUsername(upperConfig, moduleId)) == NULL)
                    {
                        free_transport_handle_data(state);
                        state = NULL;
                    }
                    else
                    {
                        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_010: [IoTHubTransport_MQTT_Common_Create shall allocate memory to save its internal state where all topics, hostname, device_id, device_key, sasTokenSr and client handle shall be saved.] */
                        DList_InitializeListHead(&(state->telemetry_waitingForAck));
                        DList_InitializeListHead(&(state->ack_waiting_queue));
                        DList_InitializeListHead(&(state->pending_get_twin_queue));
                        state->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
                        state->isRecoverableError = true;
                        state->packetId = 1;
                        state->waitingToSend = waitingToSend;
                        state->currPacketState = CONNECT_TYPE;
                        state->keepAliveValue = DEFAULT_MQTT_KEEPALIVE;
                        state->connect_timeout_in_sec = DEFAULT_CONNACK_TIMEOUT;
                        state->topics_ToSubscribe = UNSUBSCRIBE_FROM_TOPIC;
                        srand((unsigned int)get_time(NULL));
                        state->authorization_module = auth_module;

                        state->isDestroyCalled = false;
                        state->isRetryExpiredCallbackSet = false;
                        state->isRegistered = false;
                        state->device_twin_get_sent = false;
                        state->xioTransport = NULL;
                        state->portNum = 0;
                        state->connectFailCount = 0;
                        state->connectTick = 0;
                        state->topic_MqttMessage = NULL;
                        state->topic_GetState = NULL;
                        state->topic_NotifyState = NULL;
                        state->topic_DeviceMethods = NULL;
                        state->topic_InputQueue = NULL;
                        state->log_trace = state->raw_trace = false;
                        state->isProductInfoSet = false;
                        state->auto_url_encode_decode = false;
                        state->conn_attempted = false;
                    }
                }
            }
        }
    }
    return state;
}

TRANSPORT_LL_HANDLE IoTHubTransport_MQTT_Common_Create(const IOTHUBTRANSPORT_CONFIG* config, MQTT_GET_IO_TRANSPORT get_io_transport, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    PMQTTTRANSPORT_HANDLE_DATA result;
    size_t deviceIdSize;

    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_001: [If parameter config is NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_041: [ if get_io_transport is NULL then IoTHubTransport_MQTT_Common_Create shall return NULL. ] */
    if (config == NULL || get_io_transport == NULL || cb_info == NULL)
    {
        LogError("Invalid Argument config: %p, get_io_transport: %p, cb_info: %p", config, get_io_transport, cb_info);
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [If the parameter config's variables upperConfig, auth_module_handle or waitingToSend are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (config->auth_module_handle == NULL)
    {
        LogError("Invalid Argument: auth_module_handle is NULL)");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [If the parameter config's variables upperConfig or waitingToSend are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, both deviceKey and deviceSasToken, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_03_003: [If both deviceKey & deviceSasToken fields are NOT NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (config->upperConfig == NULL ||
        config->upperConfig->protocol == NULL ||
        config->upperConfig->deviceId == NULL ||
        ((config->upperConfig->deviceKey != NULL) && (config->upperConfig->deviceSasToken != NULL)) ||
        config->upperConfig->iotHubName == NULL ||
        config->upperConfig->iotHubSuffix == NULL)
    {
        LogError("Invalid Argument: upperConfig structure contains an invalid parameter");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [If the parameter config's variables upperConfig, auth_module_handle or waitingToSend are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (config->waitingToSend == NULL)
    {
        LogError("Invalid Argument: waitingToSend is NULL)");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_006: [If the upperConfig's variables deviceId is an empty strings or length is greater then 128 then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (((deviceIdSize = strlen(config->upperConfig->deviceId)) > 128U) || (deviceIdSize == 0))
    {
        LogError("Invalid Argument: DeviceId is of an invalid size");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, both deviceKey and deviceSasToken, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if ((config->upperConfig->deviceKey != NULL) && (strlen(config->upperConfig->deviceKey) == 0))
    {
        LogError("Invalid Argument: deviceKey is empty");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, both deviceKey and deviceSasToken, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if ((config->upperConfig->deviceSasToken != NULL) && (strlen(config->upperConfig->deviceSasToken) == 0))
    {
        LogError("Invalid Argument: deviceSasToken is empty");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, deviceKey, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (strlen(config->upperConfig->iotHubName) == 0)
    {
        LogError("Invalid Argument: iotHubName is empty");
        result = NULL;
    }
    else if (IoTHub_Transport_ValidateCallbacks(cb_info) != 0)
    {
        LogError("failure checking transport callbacks");
        result = NULL;
    }
    else
    {
        result = InitializeTransportHandleData(config->upperConfig, config->waitingToSend, config->auth_module_handle, config->moduleId);
        if (result != NULL)
        {
            result->get_io_transport = get_io_transport;
            result->http_proxy_hostname = NULL;
            result->http_proxy_username = NULL;
            result->http_proxy_password = NULL;

            result->transport_ctx = ctx;
            memcpy(&result->transport_callbacks, cb_info, sizeof(TRANSPORT_CALLBACKS_INFO));
        }
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_009: [If any error is encountered then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_011: [On Success IoTHubTransport_MQTT_Common_Create shall return a non-NULL value.] */
    return result;
}

void IoTHubTransport_MQTT_Common_Destroy(TRANSPORT_LL_HANDLE handle)
{
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_012: [IoTHubTransport_MQTT_Common_Destroy shall do nothing if parameter handle is NULL.] */
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data != NULL)
    {
        transport_data->isDestroyCalled = true;

        DisconnectFromClient(transport_data);

        //Empty the Waiting for Ack Messages.
        while (!DList_IsListEmpty(&transport_data->telemetry_waitingForAck))
        {
            PDLIST_ENTRY currentEntry = DList_RemoveHeadList(&transport_data->telemetry_waitingForAck);
            MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry = containingRecord(currentEntry, MQTT_MESSAGE_DETAILS_LIST, entry);
            sendMsgComplete(mqttMsgEntry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY);
            free(mqttMsgEntry);
        }
        while (!DList_IsListEmpty(&transport_data->ack_waiting_queue))
        {
            PDLIST_ENTRY currentEntry = DList_RemoveHeadList(&transport_data->ack_waiting_queue);
            MQTT_DEVICE_TWIN_ITEM* mqtt_device_twin = containingRecord(currentEntry, MQTT_DEVICE_TWIN_ITEM, entry);

            if (mqtt_device_twin->userCallback == NULL)
            {
                transport_data->transport_callbacks.twin_rpt_state_complete_cb(mqtt_device_twin->iothub_msg_id, STATUS_CODE_TIMEOUT_VALUE, transport_data->transport_ctx);
            }
            else
            {
                mqtt_device_twin->userCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, mqtt_device_twin->userContext);
            }

            destroy_device_twin_get_message(mqtt_device_twin);
        }
        while (!DList_IsListEmpty(&transport_data->pending_get_twin_queue))
        {
            PDLIST_ENTRY currentEntry = DList_RemoveHeadList(&transport_data->pending_get_twin_queue);

            MQTT_DEVICE_TWIN_ITEM* mqtt_device_twin = containingRecord(currentEntry, MQTT_DEVICE_TWIN_ITEM, entry);

            mqtt_device_twin->userCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, mqtt_device_twin->userContext);

            destroy_device_twin_get_message(mqtt_device_twin);
        }

        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_014: [IoTHubTransport_MQTT_Common_Destroy shall free all the resources currently in use.] */
        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_012: [ `IoTHubTransport_MQTT_Common_Destroy` shall free the stored proxy options. ]*/
        free_transport_handle_data(transport_data);
    }
}

int IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(TRANSPORT_LL_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data == NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_042: [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else
    {
        if (transport_data->topic_GetState == NULL)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_044: [IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall construct the get state topic string and the notify state topic string.] */
            transport_data->topic_GetState = STRING_construct(TOPIC_GET_DESIRED_STATE);
            if (transport_data->topic_GetState == NULL)
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_046: [Upon failure IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return a non-zero value.] */
                LogError("Failure: unable constructing reported state topic");
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_047: [On success IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return 0.] */
                transport_data->topics_ToSubscribe |= SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
                result = 0;
            }
        }
        else
        {
            result = 0;
        }
        if (result == 0)
        {
            changeStateToSubscribeIfAllowed(transport_data);
        }
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(TRANSPORT_LL_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_048: [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall do nothing.] */
    if (transport_data != NULL)
    {
        if (transport_data->topic_GetState != NULL)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_049: [If subscribe_state is set to IOTHUB_DEVICE_TWIN_DESIRED_STATE then IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall unsubscribe from the topic_GetState to the mqtt client.] */
            transport_data->topics_ToSubscribe &= ~SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
            STRING_delete(transport_data->topic_GetState);
            transport_data->topic_GetState = NULL;
        }
        if (transport_data->topic_NotifyState != NULL)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_050: [If subscribe_state is set to IOTHUB_DEVICE_TWIN_NOTIFICATION_STATE then IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall unsubscribe from the topic_NotifyState to the mqtt client.] */
            transport_data->topics_ToSubscribe &= ~SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
            STRING_delete(transport_data->topic_NotifyState);
            transport_data->topic_NotifyState = NULL;
        }
    }
    else
    {
        LogError("Invalid argument to unsubscribe (handle is NULL).");
    }
}

IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_GetTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
{
    IOTHUB_CLIENT_RESULT result;

    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_09_001: [ If `handle` or `completionCallback` are `NULL` than `IoTHubTransport_MQTT_Common_GetTwinAsync` shall return IOTHUB_CLIENT_INVALID_ARG. ]
    if (handle == NULL || completionCallback == NULL)
    {
        LogError("Invalid argument (handle=%p, completionCallback=%p)", handle, completionCallback);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
        MQTT_DEVICE_TWIN_ITEM* mqtt_info;

        if ((mqtt_info = create_device_twin_get_message(transport_data)) == NULL)
        {
            LogError("Failed creating the device twin get request message");
            // Codes_SRS_IOTHUB_MQTT_TRANSPORT_09_003: [ If any failure occurs, IoTHubTransport_MQTT_Common_GetTwinAsync shall return IOTHUB_CLIENT_ERROR ]
            result = IOTHUB_CLIENT_ERROR;
        }
        else if (tickcounter_get_current_ms(transport_data->msgTickCounter, &mqtt_info->msgEnqueueTime) != 0)
        {
            LogError("Failed setting the get twin request enqueue time");
            destroy_device_twin_get_message(mqtt_info);
            // Codes_SRS_IOTHUB_MQTT_TRANSPORT_09_003: [ If any failure occurs, IoTHubTransport_MQTT_Common_GetTwinAsync shall return IOTHUB_CLIENT_ERROR ]
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            mqtt_info->userCallback = completionCallback;
            mqtt_info->userContext = callbackContext;

            // Codes_SRS_IOTHUB_MQTT_TRANSPORT_09_002: [ The request shall be queued to be sent when the transport is connected, through DoWork ]
            DList_InsertTailList(&transport_data->pending_get_twin_queue, &mqtt_info->entry);

            // Codes_SRS_IOTHUB_MQTT_TRANSPORT_09_004: [ If no failure occurs, IoTHubTransport_MQTT_Common_GetTwinAsync shall return IOTHUB_CLIENT_OK ]
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}

int IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;

    if (transport_data == NULL)
    {
        /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_001 : [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return a non - zero value.]*/
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else
    {
        if (transport_data->topic_DeviceMethods == NULL)
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_004 : [IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall construct the DEVICE_METHOD topic string for subscribe.]*/
            transport_data->topic_DeviceMethods = STRING_construct(TOPIC_DEVICE_METHOD_SUBSCRIBE);
            if (transport_data->topic_DeviceMethods == NULL)
            {
                /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_006 : [Upon failure IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return a non - zero value.]*/
                LogError("Failure: unable constructing device method subscribe topic");
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_003 : [IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall set the signaling flag for DEVICE_METHOD topic for the receiver's topic list. ]*/
                transport_data->topics_ToSubscribe |= SUBSCRIBE_DEVICE_METHOD_TOPIC;
                result = 0;
            }
        }
        else
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_002 : [If the MQTT transport has been previously subscribed to DEVICE_METHOD topic IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall do nothing and return 0.]*/
            result = 0;
        }

        if (result == 0)
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_005 : [IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall schedule the send of the subscription.]*/
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_007 : [On success IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return 0.]*/
            changeStateToSubscribeIfAllowed(transport_data);
        }
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod(TRANSPORT_LL_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_008 : [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall do nothing and return.]*/
    if (transport_data != NULL)
    {
        /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_009 : [If the MQTT transport has not been subscribed to DEVICE_METHOD topic IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall do nothing and return.]*/
        if (transport_data->topic_DeviceMethods != NULL)
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_010 : [IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall construct the DEVICE_METHOD topic string for unsubscribe.]*/
            const char* unsubscribe[1];
            unsubscribe[0] = STRING_c_str(transport_data->topic_DeviceMethods);

            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_011 : [IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall send the unsubscribe.]*/
            if (mqtt_client_unsubscribe(transport_data->mqttClient, get_next_packet_id(transport_data), unsubscribe, 1) != 0)
            {
                LogError("Failure calling mqtt_client_unsubscribe");
            }

            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_012 : [IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall removes the signaling flag for DEVICE_METHOD topic from the receiver's topic list. ]*/
            STRING_delete(transport_data->topic_DeviceMethods);
            transport_data->topic_DeviceMethods = NULL;
            transport_data->topics_ToSubscribe &= ~SUBSCRIBE_DEVICE_METHOD_TOPIC;
        }
    }
    else
    {
        LogError("Invalid argument to unsubscribe (NULL).");
    }
}

int IoTHubTransport_MQTT_Common_DeviceMethod_Response(TRANSPORT_LL_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t respSize, int status)
{
    int result;
    MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)handle;
    if (transport_data != NULL)
    {
        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_042: [ IoTHubTransport_MQTT_Common_DeviceMethod_Response shall publish an mqtt message for the device method response. ] */
        DEVICE_METHOD_INFO* dev_method_info = (DEVICE_METHOD_INFO*)methodId;
        if (dev_method_info == NULL)
        {
            LogError("Failure: DEVICE_METHOD_INFO was NULL");
            result = MU_FAILURE;
        }
        else
        {
            if (publish_device_method_message(transport_data, status, dev_method_info->request_id, response, respSize) != 0)
            {
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_051: [ If any error is encountered, IoTHubTransport_MQTT_Common_DeviceMethod_Response shall return a non-zero value. ] */
                LogError("Failure: publishing device method response");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
            STRING_delete(dev_method_info->request_id);
            free(dev_method_info);
        }
    }
    else
    {
        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_041: [ If the parameter handle is NULL than IoTHubTransport_MQTT_Common_DeviceMethod_Response shall return a non-zero value. ] */
        result = MU_FAILURE;
        LogError("Failure: invalid TRANSPORT_LL_HANDLE parameter specified");
    }
    return result;
}

static STRING_HANDLE buildTopicMqttMessage(const char* device_id, const char* module_id)
{
    if (module_id == NULL)
    {
        return STRING_construct_sprintf(TOPIC_DEVICE_MSG, device_id);
    }
    else
    {
        return STRING_construct_sprintf(TOPIC_DEVICE_MODULE_MSG, device_id, module_id);
    }
}

int IoTHubTransport_MQTT_Common_Subscribe(TRANSPORT_LL_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data == NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_015: [If parameter handle is NULL than IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else
    {
        /* Code_SRS_IOTHUB_MQTT_TRANSPORT_07_016: [IoTHubTransport_MQTT_Common_Subscribe shall set a flag to enable mqtt_client_subscribe to be called to subscribe to the Message Topic.] */
        transport_data->topic_MqttMessage = buildTopicMqttMessage(STRING_c_str(transport_data->device_id), STRING_c_str(transport_data->module_id));
        if (transport_data->topic_MqttMessage == NULL)
        {
            LogError("Failure constructing Message Topic");
            result = MU_FAILURE;
        }
        else
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_TELEMETRY_TOPIC;
            /* Code_SRS_IOTHUB_MQTT_TRANSPORT_07_035: [If current packet state is not CONNACT, DISCONNECT_TYPE, or PACKET_TYPE_ERROR then IoTHubTransport_MQTT_Common_Subscribe shall set the packet state to SUBSCRIBE_TYPE.]*/
            changeStateToSubscribeIfAllowed(transport_data);
            result = 0;
        }
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe(TRANSPORT_LL_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_019: [If parameter handle is NULL then IoTHubTransport_MQTT_Common_Unsubscribe shall do nothing.] */
    if (transport_data != NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_020: [IoTHubTransport_MQTT_Common_Unsubscribe shall call mqtt_client_unsubscribe to unsubscribe the mqtt message topic.] */
        const char* unsubscribe[1];
        unsubscribe[0] = STRING_c_str(transport_data->topic_MqttMessage);
        if (mqtt_client_unsubscribe(transport_data->mqttClient, get_next_packet_id(transport_data), unsubscribe, 1) != 0)
        {
            LogError("Failure calling mqtt_client_unsubscribe");
        }
        STRING_delete(transport_data->topic_MqttMessage);
        transport_data->topic_MqttMessage = NULL;
        transport_data->topics_ToSubscribe &= ~SUBSCRIBE_TELEMETRY_TOPIC;
    }
    else
    {
        LogError("Invalid argument to unsubscribe (NULL).");
    }
}

IOTHUB_PROCESS_ITEM_RESULT IoTHubTransport_MQTT_Common_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
{
    IOTHUB_PROCESS_ITEM_RESULT result;
    /* Codes_SRS_IOTHUBCLIENT_LL_07_001: [ If handle or iothub_item are NULL then IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR. ]*/
    if (handle == NULL || iothub_item == NULL)
    {
        LogError("Invalid handle parameter iothub_item=%p", iothub_item);
        result = IOTHUB_PROCESS_ERROR;
    }
    else
    {
        PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;

        if (transport_data->currPacketState == PUBLISH_TYPE)
        {
            // Ensure the reported property suback has been received
            if (item_type == IOTHUB_TYPE_DEVICE_TWIN && transport_data->twin_resp_sub_recv)
            {
                MQTT_DEVICE_TWIN_ITEM* mqtt_info = (MQTT_DEVICE_TWIN_ITEM*)malloc(sizeof(MQTT_DEVICE_TWIN_ITEM));
                if (mqtt_info == NULL)
                {
                    /* Codes_SRS_IOTHUBCLIENT_LL_07_004: [ If any errors are encountered IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR. ]*/
                    result = IOTHUB_PROCESS_ERROR;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_07_003: [ IoTHubTransport_MQTT_Common_ProcessItem shall publish a message to the mqtt protocol with the message topic for the message type.]*/
                    mqtt_info->iothub_type = item_type;
                    mqtt_info->iothub_msg_id = iothub_item->device_twin->item_id;
                    mqtt_info->retryCount = 0;
                    mqtt_info->userCallback = NULL;
                    mqtt_info->userContext = NULL;
                    mqtt_info->msgEnqueueTime = 0;

                    /* Codes_SRS_IOTHUBCLIENT_LL_07_005: [ If successful IoTHubTransport_MQTT_Common_ProcessItem shall add mqtt info structure acknowledgement queue. ] */
                    DList_InsertTailList(&transport_data->ack_waiting_queue, &mqtt_info->entry);

                    if (publish_device_twin_message(transport_data, iothub_item->device_twin, mqtt_info) != 0)
                    {
                        DList_RemoveEntryList(&mqtt_info->entry);

                        free(mqtt_info);
                        /* Codes_SRS_IOTHUBCLIENT_LL_07_004: [ If any errors are encountered IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR. ]*/
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
                /* Codes_SRS_IOTHUBCLIENT_LL_07_006: [ If the item_type is not a supported type IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_CONTINUE. ]*/
                result = IOTHUB_PROCESS_CONTINUE;
            }
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_07_002: [ If the mqtt is not ready to publish messages IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_NOT_CONNECTED. ] */
            result = IOTHUB_PROCESS_NOT_CONNECTED;
        }
    }
    return result;
}

/* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ IoTHubTransport_MQTT_Common_DoWork shall subscribe to the Notification and get_state Topics if they are defined. ] */
void IoTHubTransport_MQTT_Common_DoWork(TRANSPORT_LL_HANDLE handle)
{
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_026: [IoTHubTransport_MQTT_Common_DoWork shall do nothing if parameter handle and/or iotHubClientHandle is NULL.] */
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data != NULL)
    {
        if (InitializeConnection(transport_data) == 0)
        {
            if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_PENDING_CLOSE)
            {
                mqtt_client_disconnect(transport_data->mqttClient, NULL, NULL);
                transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
            }
            else if (transport_data->currPacketState == CONNACK_TYPE || transport_data->currPacketState == SUBSCRIBE_TYPE)
            {
                SubscribeToMqttProtocol(transport_data);
            }
            else if (transport_data->currPacketState == SUBACK_TYPE)
            {
                if ((transport_data->topic_NotifyState != NULL || transport_data->topic_GetState != NULL) &&
                    !transport_data->device_twin_get_sent)
                {
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_055: [ IoTHubTransport_MQTT_Common_DoWork shall send a device twin get property message upon successfully retrieving a SUBACK on device twin topics. ] */
                    MQTT_DEVICE_TWIN_ITEM* mqtt_info;

                    if ((mqtt_info = create_device_twin_get_message(transport_data)) == NULL)
                    {
                        LogError("Failure: could not create message for twin get command");
                    }
                    else if (publish_device_twin_get_message(transport_data, mqtt_info) == 0)
                    {
                        transport_data->device_twin_get_sent = true;
                    }
                    else
                    {
                        LogError("Failure: sending device twin get property command.");
                        destroy_device_twin_get_message(mqtt_info);
                    }
                }
                // Publish can be called now
                transport_data->currPacketState = PUBLISH_TYPE;
            }
            else if (transport_data->currPacketState == PUBLISH_TYPE)
            {
                PDLIST_ENTRY currentListEntry = transport_data->waitingToSend->Flink;
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_027: [IoTHubTransport_MQTT_Common_DoWork shall inspect the "waitingToSend" DLIST passed in config structure.] */
                while (currentListEntry != transport_data->waitingToSend)
                {
                    IOTHUB_MESSAGE_LIST* iothubMsgList = containingRecord(currentListEntry, IOTHUB_MESSAGE_LIST, entry);
                    DLIST_ENTRY savedFromCurrentListEntry;
                    savedFromCurrentListEntry.Flink = currentListEntry->Flink;

                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_027: [IoTHubTransport_MQTT_Common_DoWork shall inspect the "waitingToSend" DLIST passed in config structure.] */
                    size_t messageLength;
                    const unsigned char* messagePayload = NULL;
                    if (!RetrieveMessagePayload(iothubMsgList->messageHandle, &messagePayload, &messageLength))
                    {
                        (void)(DList_RemoveEntryList(currentListEntry));
                        sendMsgComplete(iothubMsgList, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
                        LogError("Failure result from IoTHubMessage_GetData");
                    }
                    else
                    {
                        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_029: [IoTHubTransport_MQTT_Common_DoWork shall create a MQTT_MESSAGE_HANDLE and pass this to a call to mqtt_client_publish.] */
                        MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry = (MQTT_MESSAGE_DETAILS_LIST*)malloc(sizeof(MQTT_MESSAGE_DETAILS_LIST));
                        if (mqttMsgEntry == NULL)
                        {
                            LogError("Allocation Error: Failure allocating MQTT Message Detail List.");
                        }
                        else
                        {
                            mqttMsgEntry->retryCount = 0;
                            mqttMsgEntry->iotHubMessageEntry = iothubMsgList;
                            mqttMsgEntry->packet_id = get_next_packet_id(transport_data);
                            if (publish_mqtt_telemetry_msg(transport_data, mqttMsgEntry, messagePayload, messageLength) != 0)
                            {
                                (void)(DList_RemoveEntryList(currentListEntry));
                                sendMsgComplete(iothubMsgList, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
                                free(mqttMsgEntry);
                            }
                            else
                            {
                                // Remove the message from the waiting queue ...
                                (void)(DList_RemoveEntryList(currentListEntry));
                                // and add it to the ack queue
                                DList_InsertTailList(&(transport_data->telemetry_waitingForAck), &(mqttMsgEntry->entry));
                            }
                        }
                    }
                    currentListEntry = savedFromCurrentListEntry.Flink;
                }

                sendPendingGetTwinRequests(transport_data);
            }
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_030: [IoTHubTransport_MQTT_Common_DoWork shall call mqtt_client_dowork everytime it is called if it is connected.] */
            mqtt_client_dowork(transport_data->mqttClient);
        }

        // Check the ack messages timeouts
        process_queued_ack_messages(transport_data);
        removeExpiredPendingGetTwinRequests(transport_data);
        removeExpiredGetTwinRequestsPendingAck(transport_data);
    }
}

IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_GetSendStatus(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL || iotHubClientStatus == NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_023: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter.] */
        LogError("invalid arument.");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* handleData = (MQTTTRANSPORT_HANDLE_DATA*)handle;
        if (!DList_IsListEmpty(handleData->waitingToSend) || !DList_IsListEmpty(&(handleData->telemetry_waitingForAck)))
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_025: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_BUSY if there are currently event items to be sent or being sent.] */
            *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_BUSY;
        }
        else
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_024: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_IDLE if there are currently no event items to be sent or being sent.] */
            *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_IDLE;
        }
        result = IOTHUB_CLIENT_OK;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
{
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_021: [If any parameter is NULL then IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
    IOTHUB_CLIENT_RESULT result;
    if (
        (handle == NULL) ||
        (option == NULL) ||
        (value == NULL)
        )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid parameter (NULL) passed to IoTHubTransport_MQTT_Common_SetOption.");
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)handle;

        IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module);

        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_031: [If the option parameter is set to "logtrace" then the value shall be a bool_ptr and the value will determine if the mqtt client log is on or off.] */
        if (strcmp(OPTION_LOG_TRACE, option) == 0)
        {
            transport_data->log_trace = *((bool*)value);
            mqtt_client_set_trace(transport_data->mqttClient, transport_data->log_trace, transport_data->raw_trace);
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp("rawlogtrace", option) == 0)
        {
            transport_data->raw_trace = *((bool*)value);
            mqtt_client_set_trace(transport_data->mqttClient, transport_data->log_trace, transport_data->raw_trace);
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(OPTION_AUTO_URL_ENCODE_DECODE, option) == 0)
        {
            transport_data->auto_url_encode_decode = *((bool*)value);
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(OPTION_CONNECTION_TIMEOUT, option) == 0)
        {
            int* connection_time = (int*)value;
            if (*connection_time != transport_data->connect_timeout_in_sec)
            {
                transport_data->connect_timeout_in_sec = (uint16_t)(*connection_time);
            }
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(OPTION_KEEP_ALIVE, option) == 0)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_036: [If the option parameter is set to "keepalive" then the value shall be a int_ptr and the value will determine the mqtt keepalive time that is set for pings.] */
            int* keepAliveOption = (int*)value;
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_037 : [If the option parameter is set to supplied int_ptr keepalive is the same value as the existing keepalive then IoTHubTransport_MQTT_Common_SetOption shall do nothing.] */
            if (*keepAliveOption != transport_data->keepAliveValue)
            {
                transport_data->keepAliveValue = (uint16_t)(*keepAliveOption);
                if (transport_data->mqttClientStatus != MQTT_CLIENT_STATUS_NOT_CONNECTED)
                {
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_038: [If the client is connected when the keepalive is set then IoTHubTransport_MQTT_Common_SetOption shall disconnect and reconnect with the specified keepalive value.] */
                    DisconnectFromClient(transport_data);
                }
            }
            result = IOTHUB_CLIENT_OK;
        }
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_039: [If the option parameter is set to "x509certificate" then the value shall be a const char of the certificate to be used for x509.] */
        else if ((strcmp(OPTION_X509_CERT, option) == 0) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509 && cred_type != IOTHUB_CREDENTIAL_TYPE_UNKNOWN))
        {
            LogError("x509certificate specified, but authentication method is not x509");
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_040: [If the option parameter is set to "x509privatekey" then the value shall be a const char of the RSA Private Key to be used for x509.] */
        else if ((strcmp(OPTION_X509_PRIVATE_KEY, option) == 0) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509 && cred_type != IOTHUB_CREDENTIAL_TYPE_UNKNOWN))
        {
            LogError("x509privatekey specified, but authentication method is not x509");
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
        else if (strcmp(OPTION_HTTP_PROXY, option) == 0)
        {
            /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_001: [ If `option` is `proxy_data`, `value` shall be used as an `HTTP_PROXY_OPTIONS*`. ]*/
            HTTP_PROXY_OPTIONS* proxy_options = (HTTP_PROXY_OPTIONS*)value;

            if (transport_data->xioTransport != NULL)
            {
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_007: [ If the underlying IO has already been created, then `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
                LogError("Cannot set proxy option once the underlying IO is created");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (proxy_options->host_address == NULL)
            {
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_003: [ If `host_address` is NULL, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
                LogError("NULL host_address in proxy options");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_006: [ If only one of `username` and `password` is NULL, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
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

                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_002: [ The fields `host_address`, `port`, `username` and `password` shall be saved for later used (needed when creating the underlying IO to be used by the transport). ]*/
                transport_data->http_proxy_port = proxy_options->port;
                if (mallocAndStrcpy_s(&copied_proxy_hostname, proxy_options->host_address) != 0)
                {
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
                    LogError("Cannot copy HTTP proxy hostname");
                    result = IOTHUB_CLIENT_ERROR;
                }
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_005: [ `username` and `password` shall be allowed to be NULL. ]*/
                else if ((proxy_options->username != NULL) && (mallocAndStrcpy_s(&copied_proxy_username, proxy_options->username) != 0))
                {
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
                    free(copied_proxy_hostname);
                    LogError("Cannot copy HTTP proxy username");
                    result = IOTHUB_CLIENT_ERROR;
                }
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_005: [ `username` and `password` shall be allowed to be NULL. ]*/
                else if ((proxy_options->password != NULL) && (mallocAndStrcpy_s(&copied_proxy_password, proxy_options->password) != 0))
                {
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
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
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_009: [ When setting the proxy options succeeds any previously saved proxy options shall be freed. ]*/
                    free_proxy_data(transport_data);

                    transport_data->http_proxy_hostname = copied_proxy_hostname;
                    transport_data->http_proxy_username = copied_proxy_username;
                    transport_data->http_proxy_password = copied_proxy_password;

                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_008: [ If setting the `proxy_data` option suceeds, `IoTHubTransport_MQTT_Common_SetOption` shall return `IOTHUB_CLIENT_OK` ]*/
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_039: [If the option parameter is set to "x509certificate" then the value shall be a const char of the certificate to be used for x509.] */
            if (((strcmp(OPTION_X509_CERT, option) == 0) || (strcmp(OPTION_X509_PRIVATE_KEY, option) == 0)) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509))
            {
                IoTHubClient_Auth_Set_x509_Type(transport_data->authorization_module, true);
            }

            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_032: [IoTHubTransport_MQTT_Common_SetOption shall pass down the option to xio_setoption if the option parameter is not a known option string for the MQTT transport.] */
            if (GetTransportProviderIfNecessary(transport_data) == 0)
            {
                if (xio_setoption(transport_data->xioTransport, option, value) == 0)
                {
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_132: [IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG xio_setoption fails] */
                    result = IOTHUB_CLIENT_INVALID_ARG;
                }
            }
            else
            {
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }
    return result;
}

static bool check_module_ids_equal(const char* transportModuleId, const char* deviceModuleId)
{
    if ((transportModuleId != NULL) && (deviceModuleId == NULL))
    {
        return false;
    }
    else if ((transportModuleId == NULL) && (deviceModuleId != NULL))
    {
        return false;
    }
    else if ((transportModuleId == NULL) && (deviceModuleId == NULL))
    {
        return true;
    }
    else
    {
        return (0 == strcmp(transportModuleId, deviceModuleId));
    }
}

TRANSPORT_LL_HANDLE IoTHubTransport_MQTT_Common_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend)
{
    TRANSPORT_LL_HANDLE result = NULL;

    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_001: [ IoTHubTransport_MQTT_Common_Register shall return NULL if the TRANSPORT_LL_HANDLE is NULL.]
    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_002: [ IoTHubTransport_MQTT_Common_Register shall return NULL if device or waitingToSend are NULL.]
    if ((handle == NULL) || (device == NULL) || (waitingToSend == NULL))
    {
        LogError("IoTHubTransport_MQTT_Common_Register: handle, device or waitingToSend is NULL.");
        result = NULL;
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)handle;

        // Codes_SRS_IOTHUB_MQTT_TRANSPORT_03_001: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceId, or both deviceKey and deviceSasToken are NULL.]
        if (device->deviceId == NULL)
        {
            LogError("IoTHubTransport_MQTT_Common_Register: deviceId is NULL.");
            result = NULL;
        }
        // Codes_SRS_IOTHUB_MQTT_TRANSPORT_03_002: [ IoTHubTransport_MQTT_Common_Register shall return NULL if both deviceKey and deviceSasToken are provided.]
        else if ((device->deviceKey != NULL) && (device->deviceSasToken != NULL))
        {
            LogError("IoTHubTransport_MQTT_Common_Register: Both deviceKey and deviceSasToken are defined. Only one can be used.");
            result = NULL;
        }
        else
        {
            // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_003: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceId or deviceKey do not match the deviceId and deviceKey passed in during IoTHubTransport_MQTT_Common_Create.]
            if (strcmp(STRING_c_str(transport_data->device_id), device->deviceId) != 0)
            {
                LogError("IoTHubTransport_MQTT_Common_Register: deviceId does not match.");
                result = NULL;
            }
            else if (!check_module_ids_equal(STRING_c_str(transport_data->module_id), device->moduleId))
            {
                LogError("IoTHubTransport_MQTT_Common_Register: moduleId does not match.");
                result = NULL;
            }
            // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_43_001: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceKey is NULL when credential type is IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY ]
            else if (IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module) == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY &&
                ((device->deviceKey == NULL) || (strcmp(IoTHubClient_Auth_Get_DeviceKey(transport_data->authorization_module), device->deviceKey) != 0)))
            {
                LogError("IoTHubTransport_MQTT_Common_Register: deviceKey does not match.");
                result = NULL;
            }
            else
            {
                if (transport_data->isRegistered == true)
                {
                    LogError("Transport already has device registered by id: [%s]", device->deviceId);
                    result = NULL;
                }
                else
                {
                    transport_data->isRegistered = true;
                    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_004: [ IoTHubTransport_MQTT_Common_Register shall return the TRANSPORT_LL_HANDLE as the TRANSPORT_LL_HANDLE. ]
                    result = (TRANSPORT_LL_HANDLE)handle;
                }
            }
        }
    }

    return result;
}

void IoTHubTransport_MQTT_Common_Unregister(TRANSPORT_LL_HANDLE deviceHandle)
{
    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_005: [ If deviceHandle is NULL `IoTHubTransport_MQTT_Common_Unregister` shall do nothing. ]
    if (deviceHandle != NULL)
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)deviceHandle;

        transport_data->isRegistered = false;
    }
}

STRING_HANDLE IoTHubTransport_MQTT_Common_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    STRING_HANDLE result;
    /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_02_001: [ If handle is NULL then IoTHubTransport_MQTT_Common_GetHostname shall fail and return NULL. ]*/
    if (handle == NULL)
    {
        result = NULL;
    }
    /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_02_002: [ Otherwise IoTHubTransport_MQTT_Common_GetHostname shall return a non-NULL STRING_HANDLE containg the hostname. ]*/
    else if ((result = STRING_clone(((MQTTTRANSPORT_HANDLE_DATA*)(handle))->hostAddress)) == NULL)
    {
        LogError("Cannot provide the target host name (STRING_clone failed).");
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_SendMessageDisposition(MESSAGE_CALLBACK_INFO* message_data, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    (void)disposition;

    IOTHUB_CLIENT_RESULT result;
    if (message_data)
    {
        if (message_data->messageHandle)
        {
            IoTHubMessage_Destroy(message_data->messageHandle);
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_10_002: [If any of the messageData fields are NULL, IoTHubTransport_MQTT_Common_SendMessageDisposition shall fail and return IOTHUB_CLIENT_ERROR. ]*/
            LogError("message handle is NULL");
            result = IOTHUB_CLIENT_ERROR;
        }
        free(message_data);
    }
    else
    {
        /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_10_001: [If messageData is NULL, IoTHubTransport_MQTT_Common_SendMessageDisposition shall fail and return IOTHUB_CLIENT_ERROR. ]*/
        LogError("message_data is NULL");
        result = IOTHUB_CLIENT_ERROR;
    }
    return result;
}


int IoTHubTransport_MQTT_Common_Subscribe_InputQueue(TRANSPORT_LL_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data == NULL)
    {
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_066: [ If parameter handle is NULL than IoTHubTransport_MQTT_Common_Subscribe_InputQueue shall return a non-zero value.]
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else if (transport_data->module_id == NULL)
    {
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_073: [ If module ID is not set on the transpont, IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue shall fail.]
        LogError("ModuleID must be specified for input queues. NULL.");
        result = MU_FAILURE;
    }
    else if ((transport_data->topic_InputQueue == NULL) &&
        (transport_data->topic_InputQueue = STRING_construct_sprintf(TOPIC_INPUT_QUEUE_NAME, STRING_c_str(transport_data->device_id), STRING_c_str(transport_data->module_id))) == NULL)
    {
        LogError("Failure constructing Message Topic");
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_067: [ IoTHubTransport_MQTT_Common_Subscribe_InputQueue shall set a flag to enable mqtt_client_subscribe to be called to subscribe to the input queue Message Topic.]
        transport_data->topics_ToSubscribe |= SUBSCRIBE_INPUT_QUEUE_TOPIC;
        changeStateToSubscribeIfAllowed(transport_data);
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_070: [ On success IoTHubTransport_MQTT_Common_Subscribe_InputQueue shall return 0.]
        result = 0;
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue(TRANSPORT_LL_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if ((transport_data != NULL) && (transport_data->topic_InputQueue != NULL))
    {
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_072: [ IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue shall call mqtt_client_unsubscribe to unsubscribe the mqtt input queue message topic.]
        const char* unsubscribe[1];
        unsubscribe[0] = STRING_c_str(transport_data->topic_InputQueue);
        if (mqtt_client_unsubscribe(transport_data->mqttClient, get_next_packet_id(transport_data), unsubscribe, 1) != 0)
        {
            LogError("Failure calling mqtt_client_unsubscribe");
        }
        STRING_delete(transport_data->topic_InputQueue);
        transport_data->topic_InputQueue = NULL;
        transport_data->topics_ToSubscribe &= ~SUBSCRIBE_INPUT_QUEUE_TOPIC;
    }
    else
    {
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_31_071: [ If parameter handle is NULL then IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue shall do nothing.]
        LogError("Invalid argument to unsubscribe input queue (NULL).");
    }
}

int IoTHubTransport_MQTT_SetCallbackContext(TRANSPORT_LL_HANDLE handle, void* ctx)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid parameter specified handle: %p", handle);
        result = MU_FAILURE;
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)handle;
        transport_data->transport_ctx = ctx;
        result = 0;
    }
    return result;
}

int IoTHubTransport_MQTT_GetSupportedPlatformInfo(TRANSPORT_LL_HANDLE handle, PLATFORM_INFO_OPTION* info)
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