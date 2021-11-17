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
#include "internal/iothub_internal_consts.h"
#include "internal/iothub_message_private.h"

#include "azure_umqtt_c/mqtt_client.h"

#include "iothub_message.h"
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
#define TWIN_REPORT_UPDATE_TIMEOUT_SECS           (60*5)

static const char TOPIC_DEVICE_TWIN_PREFIX[] = "$iothub/twin";
static const char TOPIC_DEVICE_METHOD_PREFIX[] = "$iothub/methods";

static const char* TOPIC_GET_DESIRED_STATE = "$iothub/twin/res/#";
static const char* TOPIC_NOTIFICATION_STATE = "$iothub/twin/PATCH/properties/desired/#";

static const char* TOPIC_DEVICE_MSG = "devices/%s/messages/devicebound/#";
static const char* TOPIC_DEVICE_EVENTS = "devices/%s/messages/events/";
static const char* TOPIC_MODULE_EVENTS = "devices/%s/modules/%s/messages/events/";

static const char* TOPIC_INPUT_QUEUE_NAME = "devices/%s/modules/%s/#";

static const char* TOPIC_DEVICE_METHOD_SUBSCRIBE = "$iothub/methods/POST/#";

static const char* PROPERTY_SEPARATOR = "&";
static const char PROPERTY_EQUALS = '=';
static const char TOPIC_SLASH = '/';
static const char* REPORTED_PROPERTIES_TOPIC = "$iothub/twin/PATCH/properties/reported/?$rid=%"PRIu16;
static const char* GET_PROPERTIES_TOPIC = "$iothub/twin/GET/?$rid=%"PRIu16;
static const char* DEVICE_METHOD_RESPONSE_TOPIC = "$iothub/methods/res/%d/?$rid=%s";

static const char SYS_TOPIC_STRING_FORMAT[] = "%s%%24.%s=%s";

static const char REQUEST_ID_PROPERTY[] = "?$rid=";
static size_t REQUEST_ID_PROPERTY_LEN = sizeof(REQUEST_ID_PROPERTY) - 1;

#define SYS_PROP_MESSAGE_ID "mid"
#define SYS_PROP_MESSAGE_CREATION_TIME_UTC "ctime"
#define SYS_PROP_USER_ID "uid"
#define SYS_PROP_CORRELATION_ID "cid"
#define SYS_PROP_CONTENT_TYPE "ct"
#define SYS_PROP_CONTENT_ENCODING "ce"
#define SYS_PROP_DIAGNOSTIC_ID "diagid"
#define SYS_PROP_DIAGNOSTIC_CONTEXT "diagctx"
#define SYS_PROP_CONNECTION_DEVICE_ID "cdid"
#define SYS_PROP_CONNECTION_MODULE_ID "cmid"
#define SYS_PROP_ON "on"
#define SYS_PROP_EXP "exp"
#define SYS_PROP_TO "to"

static const char* DIAGNOSTIC_CONTEXT_CREATION_TIME_UTC_PROPERTY = "creationtimeutc";
static const char DT_MODEL_ID_TOKEN[] = "model-id";
static const char DEFAULT_IOTHUB_PRODUCT_IDENTIFIER[] = CLIENT_DEVICE_TYPE_PREFIX "/" IOTHUB_SDK_VERSION;

#define TOLOWER(c) (((c>='A') && (c<='Z'))?c-'A'+'a':c)

#define UNSUBSCRIBE_FROM_TOPIC                  0x0000
#define SUBSCRIBE_GET_REPORTED_STATE_TOPIC      0x0001
#define SUBSCRIBE_NOTIFICATION_STATE_TOPIC      0x0002
#define SUBSCRIBE_TELEMETRY_TOPIC               0x0004
#define SUBSCRIBE_DEVICE_METHOD_TOPIC           0x0008
#define SUBSCRIBE_INPUT_QUEUE_TOPIC             0x0010
#define SUBSCRIBE_TOPIC_COUNT                   5

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(MQTT_CLIENT_EVENT_ERROR, MQTT_CLIENT_EVENT_ERROR_VALUES);

// "System" property that a given MQTT property maps to, which can be used when building IOTHUB_MESSAGE_HANDLE that
// we will pass into application callback.
typedef enum IOTHUB_SYSTEM_PROPERTY_TYPE_TAG
{
    // Property that the application custom defined and will go into the propertyMap of IOTHUB_MESSAGE_HANDLE
    IOTHUB_SYSTEM_PROPERTY_TYPE_APPLICATION_CUSTOM,
    // A "system" property we should silently ignore.  There are many %24.<property> that previous versions of the
    // SDK parsed out but did NOT add to the application custom list.  To maintain backward compat, and because
    // the %24 implies a system property, we will parse these out but otherwise ignore them.
    IOTHUB_SYSTEM_PROPERTY_TYPE_SILENTLY_IGNORE,
    // Properties from this point on map to system properties that have accessors in IOTHUB_MESSAGE_HANDLE
    IOTHUB_SYSTEM_PROPERTY_TYPE_MESSAGE_ID,
    IOTHUB_SYSTEM_PROPERTY_TYPE_CREATION_TIME,
    IOTHUB_SYSTEM_PROPERTY_TYPE_CONNECTION_DEVICE_ID,
    IOTHUB_SYSTEM_PROPERTY_TYPE_CONNECTION_MODULE_ID,
    IOTHUB_SYSTEM_PROPERTY_TYPE_CORRELATION_ID,
    IOTHUB_SYSTEM_PROPERTY_TYPE_MESSAGE_USER_ID,
    IOTHUB_SYSTEM_PROPERTY_TYPE_CONTENT_TYPE,
    IOTHUB_SYSTEM_PROPERTY_TYPE_CONTENT_ENCODING
} IOTHUB_SYSTEM_PROPERTY_TYPE;

typedef struct SYSTEM_PROPERTY_INFO_TAG
{
    const char* propName;
    IOTHUB_SYSTEM_PROPERTY_TYPE propertyType;
} SYSTEM_PROPERTY_INFO;


// Encoding of a $ followed by ., which is how MQTT "system" properties are sent to us
#define URL_ENCODED_PERCENT_SIGN_DOT "%24."
const size_t URL_ENCODED_PERCENT_SIGN_DOT_LEN = sizeof(URL_ENCODED_PERCENT_SIGN_DOT) / sizeof(URL_ENCODED_PERCENT_SIGN_DOT[0]) - 1;

// Helper to build up system properties, which MUST start with "%24." string
#define DEFINE_MQTT_SYSTEM_PROPERTY(token)  URL_ENCODED_PERCENT_SIGN_DOT token

static SYSTEM_PROPERTY_INFO sysPropList[] = {
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_MESSAGE_ID), IOTHUB_SYSTEM_PROPERTY_TYPE_MESSAGE_ID},
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_USER_ID), IOTHUB_SYSTEM_PROPERTY_TYPE_MESSAGE_USER_ID },
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_CORRELATION_ID), IOTHUB_SYSTEM_PROPERTY_TYPE_CORRELATION_ID },
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_CONTENT_TYPE), IOTHUB_SYSTEM_PROPERTY_TYPE_CONTENT_TYPE },
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_CONTENT_ENCODING), IOTHUB_SYSTEM_PROPERTY_TYPE_CONTENT_ENCODING },
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_CONNECTION_DEVICE_ID), IOTHUB_SYSTEM_PROPERTY_TYPE_CONNECTION_DEVICE_ID},
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_CONNECTION_MODULE_ID), IOTHUB_SYSTEM_PROPERTY_TYPE_CONNECTION_MODULE_ID },
    // "System" properties the SDK previously ignored and will continue to do so for compat.
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_MESSAGE_CREATION_TIME_UTC), IOTHUB_SYSTEM_PROPERTY_TYPE_CREATION_TIME},
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_ON), IOTHUB_SYSTEM_PROPERTY_TYPE_SILENTLY_IGNORE },
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_EXP), IOTHUB_SYSTEM_PROPERTY_TYPE_SILENTLY_IGNORE },
    { DEFINE_MQTT_SYSTEM_PROPERTY(SYS_PROP_TO), IOTHUB_SYSTEM_PROPERTY_TYPE_SILENTLY_IGNORE },
    // even though they don't start with %24, previous versions of SDK parsed and ignored these.  Keep same behavior.
    { "devices/", IOTHUB_SYSTEM_PROPERTY_TYPE_SILENTLY_IGNORE },
    { "iothub-operation", IOTHUB_SYSTEM_PROPERTY_TYPE_SILENTLY_IGNORE },
    { "iothub-ack" , IOTHUB_SYSTEM_PROPERTY_TYPE_SILENTLY_IGNORE }     
};

static const size_t sysPropListLength = sizeof(sysPropList) / sizeof(sysPropList[0]);

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
    // conn_attempted indicates whether a connection has *ever* been attempted on the lifetime
    // of this handle.  Even if a given xio transport is added/removed, this always stays true.
    bool conn_attempted;

    MQTT_GET_IO_TRANSPORT get_io_transport;

    // The current mqtt iothub implementation requires that the hub name and the domain suffix be passed as the first of a series of segments
    // passed through the username portion of the connection frame.
    // The second segment will contain the device id.  The two segments are delimited by a "/".
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
    bool isRetryExpiredCallbackCalled;
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
    bool isConnectUsernameSet;
    int disconnect_recv_flag;
} MQTTTRANSPORT_HANDLE_DATA, *PMQTTTRANSPORT_HANDLE_DATA;

typedef struct MQTT_DEVICE_TWIN_ITEM_TAG
{
    tickcounter_ms_t msgCreationTime;
    tickcounter_ms_t msgPublishTime;
    size_t retryCount;
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

typedef struct MESSAGE_DISPOSITION_CONTEXT_TAG
{
    uint16_t packet_id;
    QOS_VALUE qos_value;
} MESSAGE_DISPOSITION_CONTEXT;

//
// InternStrnicmp implements strnicmp.  strnicmp isn't available on all platforms.
//
static int InternStrnicmp(const char* s1, const char* s2, size_t n)
{
    int result;

    if (s1 == NULL)
    {
        result = -1;
    }
    else if (s2 == NULL)
    {
        result = 1;
    }
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

//
// freeProxyData free()'s and resets proxy related settings of the mqtt_transport_instance.
//
static void freeProxyData(MQTTTRANSPORT_HANDLE_DATA* transport_data)
{
    if (transport_data->http_proxy_hostname != NULL)
    {
        free(transport_data->http_proxy_hostname);
        transport_data->http_proxy_hostname = NULL;
    }

    if (transport_data->http_proxy_username != NULL)
    {
        free(transport_data->http_proxy_username);
        transport_data->http_proxy_username = NULL;
    }

    if (transport_data->http_proxy_password != NULL)
    {
        free(transport_data->http_proxy_password);
        transport_data->http_proxy_password = NULL;
    }
}

//
// DestroyXioTransport frees resources associated with MQTT handle and resets appropriate state
//
static void DestroyXioTransport(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    mqtt_client_clear_xio(transport_data->mqttClient);
    xio_destroy(transport_data->xioTransport);
    transport_data->xioTransport = NULL;
}

//
// setSavedTlsOptions saves off TLS specific options.  This is used
// so that during a disconnection, we have these values available for next reconnection.
//
static void setSavedTlsOptions(PMQTTTRANSPORT_HANDLE_DATA transport, OPTIONHANDLER_HANDLE new_options)
{
    if (transport->saved_tls_options != NULL)
    {
        OptionHandler_Destroy(transport->saved_tls_options);
    }
    transport->saved_tls_options = new_options;
}

//
// freeTransportHandleData free()'s 'the transport_data and all members that were allocated by it.
//
static void freeTransportHandleData(MQTTTRANSPORT_HANDLE_DATA* transport_data)
{
    if (transport_data->mqttClient != NULL)
    {
        mqtt_client_deinit(transport_data->mqttClient);
        transport_data->mqttClient = NULL;
    }

    if (transport_data->retry_control_handle != NULL)
    {
        retry_control_destroy(transport_data->retry_control_handle);
    }

    setSavedTlsOptions(transport_data, NULL);

    tickcounter_destroy(transport_data->msgTickCounter);

    freeProxyData(transport_data);

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

    DestroyXioTransport(transport_data);

    free(transport_data);
}

//
// getNextPacketId gets the next Packet Id to use and increments internal counter.
//
static uint16_t getNextPacketId(PMQTTTRANSPORT_HANDLE_DATA transport_data)
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
//
// retrieveMqttReturnCodes returns friendly representation of connection code for logging purposes.
//
static const char* retrieveMqttReturnCodes(CONNECT_RETURN_CODE rtn_code)
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

//
// retrieveDeviceMethodRidInfo parses an incoming MQTT topic for a device method and retrieves the request ID it specifies.
//
static int retrieveDeviceMethodRidInfo(const char* resp_topic, STRING_HANDLE method_name, STRING_HANDLE request_id)
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

//
// parseDeviceTwinTopicInfo parses information about a topic PUBLISH'd to this device/module.
//
static int parseDeviceTwinTopicInfo(const char* resp_topic, bool* patch_msg, size_t* request_id, int* status_code)
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
                    *patch_msg = false;
                }
                else if (token_count == 3)
                {
                    *status_code = (int)atol(STRING_c_str(token_value));
                }
                else if (token_count == 4)
                {
                    const char* request_id_string = STRING_c_str(token_value);
                    if (strncmp(request_id_string, REQUEST_ID_PROPERTY, REQUEST_ID_PROPERTY_LEN) != 0)
                    {
                        LogError("requestId does not begin with string format %s", REQUEST_ID_PROPERTY);
                        *request_id = 0;
                        result = MU_FAILURE;
                    }
                    else
                    {
                        *request_id = (size_t)atol(request_id_string + REQUEST_ID_PROPERTY_LEN);
                        result = 0;
                    }
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

//
// retrieveTopicType translates an MQTT topic PUBLISH'd to this device/module into what type (e.g. twin, method, etc.) it represents.
//
static int retrieveTopicType(PMQTTTRANSPORT_HANDLE_DATA transportData, const char* topicName, IOTHUB_IDENTITY_TYPE* type)
{
    int result;

    const char* mqtt_message_queue_topic;
    const char* input_queue_topic;

    if (InternStrnicmp(topicName, TOPIC_DEVICE_TWIN_PREFIX, sizeof(TOPIC_DEVICE_TWIN_PREFIX) - 1) == 0)
    {
        *type = IOTHUB_TYPE_DEVICE_TWIN;
        result = 0;
    }
    else if (InternStrnicmp(topicName, TOPIC_DEVICE_METHOD_PREFIX, sizeof(TOPIC_DEVICE_METHOD_PREFIX) - 1) == 0)
    {
        *type = IOTHUB_TYPE_DEVICE_METHODS;
        result = 0;
    }
    // mqtt_message_queue_topic contains additional "#" from subscribe, which we strip off on comparing incoming.
    else if (((mqtt_message_queue_topic = STRING_c_str(transportData->topic_MqttMessage)) != NULL) && (InternStrnicmp(topicName, mqtt_message_queue_topic, strlen(mqtt_message_queue_topic) - 1) == 0))
    {
        *type = IOTHUB_TYPE_TELEMETRY;
        result = 0;
    }
    // input_queue_topic contains additional "#" from subscribe, which we strip off on comparing incoming.
    else if (((input_queue_topic = STRING_c_str(transportData->topic_InputQueue)) != NULL) && (InternStrnicmp(topicName, input_queue_topic, strlen(input_queue_topic) - 1) == 0))
    {
        *type = IOTHUB_TYPE_EVENT_QUEUE;
        result = 0;
    }
    else
    {
        LogError("Topic %s does not match any client is subscribed to", topicName);
        result = MU_FAILURE;        
    }
    return result;
}

//
// notifyApplicationOfSendMessageComplete lets application know that messages in the iothubMsgList have completed (or should be considered failed) with confirmResult status.
//
static void notifyApplicationOfSendMessageComplete(IOTHUB_MESSAGE_LIST* iothubMsgList, PMQTTTRANSPORT_HANDLE_DATA transport_data, IOTHUB_CLIENT_CONFIRMATION_RESULT confirmResult)
{
    DLIST_ENTRY messageCompleted;
    DList_InitializeListHead(&messageCompleted);
    DList_InsertTailList(&messageCompleted, &(iothubMsgList->entry));
    transport_data->transport_callbacks.send_complete_cb(&messageCompleted, confirmResult, transport_data->transport_ctx);
}

//
// addUserPropertiesTouMqttMessage translates application properties in iothub_message_handle (set by the application with IoTHubMessage_SetProperty e.g.)
// into a representation in the MQTT TOPIC topic_string.
//
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

//
// addSystemPropertyToTopicString appends a given "system" property from iothub_message_handle (set by the application with APIs such as IoTHubMessage_SetMessageId,
// IoTHubMessage_SetContentTypeSystemProperty, etc.) onto the MQTT TOPIC topic_string.
//
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
        else if (STRING_sprintf(topic_string, SYS_TOPIC_STRING_FORMAT, index == 0 ? "" : PROPERTY_SEPARATOR, property_key, STRING_c_str(encoded_property_value)) != 0)
        {
            LogError("Failed setting %s.", property_key);
            result = MU_FAILURE;
        }
        STRING_delete(encoded_property_value);
    }
    else
    {
        if (STRING_sprintf(topic_string, SYS_TOPIC_STRING_FORMAT, index == 0 ? "" : PROPERTY_SEPARATOR, property_key, property_value) != 0)
        {
            LogError("Failed setting %s.", property_key);
            result = MU_FAILURE;
        }
    }
    return result;
}

//
// addSystemPropertyToTopicString appends all "system" property from iothub_message_handle (set by the application with APIs such as IoTHubMessage_SetMessageId,
// IoTHubMessage_SetContentTypeSystemProperty, etc.) onto the MQTT TOPIC topic_string.
//
static int addSystemPropertiesTouMqttMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, STRING_HANDLE topic_string, size_t* index_ptr, bool urlencode)
{
    int result = 0;
    size_t index = *index_ptr;

    bool is_security_msg = IoTHubMessage_IsSecurityMessage(iothub_message_handle);
    const char* correlation_id = IoTHubMessage_GetCorrelationId(iothub_message_handle);
    if (correlation_id != NULL)
    {
        result = addSystemPropertyToTopicString(topic_string, index, SYS_PROP_CORRELATION_ID, correlation_id, urlencode);
        index++;
    }
    if (result == 0)
    {
        const char* msg_id = IoTHubMessage_GetMessageId(iothub_message_handle);
        if (msg_id != NULL)
        {
            result = addSystemPropertyToTopicString(topic_string, index, SYS_PROP_MESSAGE_ID, msg_id, urlencode);
            index++;
        }
    }
    if (result == 0)
    {
        const char* content_type = IoTHubMessage_GetContentTypeSystemProperty(iothub_message_handle);
        if (content_type != NULL)
        {
            result = addSystemPropertyToTopicString(topic_string, index, SYS_PROP_CONTENT_TYPE, content_type, urlencode);
            index++;
        }
    }
    if (result == 0)
    {
        const char* content_encoding = IoTHubMessage_GetContentEncodingSystemProperty(iothub_message_handle);
        if (content_encoding != NULL)
        {
            // Security message require content encoding
            result = addSystemPropertyToTopicString(topic_string, index, SYS_PROP_CONTENT_ENCODING, content_encoding, is_security_msg ? true : urlencode);
            index++;
        }
    }

    if (result == 0)
    {
        const char* message_creation_time_utc = IoTHubMessage_GetMessageCreationTimeUtcSystemProperty(iothub_message_handle);
        if (message_creation_time_utc != NULL)
        {
            result = addSystemPropertyToTopicString(topic_string, index, SYS_PROP_MESSAGE_CREATION_TIME_UTC, message_creation_time_utc, urlencode);
            index++;
        }
    }

    if (result == 0)
    {
        if (is_security_msg)
        {
            // The Security interface Id value must be encoded
            if (addSystemPropertyToTopicString(topic_string, index++, SECURITY_INTERFACE_ID_MQTT, SECURITY_INTERFACE_ID_VALUE, true) != 0)
            {
                LogError("Failed setting Security interface id");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
    }

    if (result == 0)
    {
        const char* output_name = IoTHubMessage_GetOutputName(iothub_message_handle);
        if (output_name != NULL)
        {
            // Encode the output name if encoding is on
            if (addSystemPropertyToTopicString(topic_string, index++, SYS_PROP_ON, output_name, urlencode) != 0)
            {
                LogError("Failed setting output name");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
    }

    *index_ptr = index;
    return result;
}

//
// addDiagnosticPropertiesTouMqttMessage appends diagnostic data (as specified by IoTHubMessage_SetDiagnosticPropertyData) onto
// the MQTT topic topic_string.
//
static int addDiagnosticPropertiesTouMqttMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, STRING_HANDLE topic_string, size_t* index_ptr)
{
    int result = 0;
    size_t index = *index_ptr;

    const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* diagnosticData = IoTHubMessage_GetDiagnosticPropertyData(iothub_message_handle);
    if (diagnosticData != NULL)
    {
        const char* diag_id = diagnosticData->diagnosticId;
        const char* creation_time_utc = diagnosticData->diagnosticCreationTimeUtc;
        //diagid and creationtimeutc must be present/unpresent simultaneously
        if (diag_id != NULL && creation_time_utc != NULL)
        {
            if (STRING_sprintf(topic_string, SYS_TOPIC_STRING_FORMAT, index == 0 ? "" : PROPERTY_SEPARATOR, SYS_PROP_DIAGNOSTIC_ID, diag_id) != 0)
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
                        if (STRING_sprintf(topic_string, SYS_TOPIC_STRING_FORMAT, index == 0 ? "" : PROPERTY_SEPARATOR, SYS_PROP_DIAGNOSTIC_CONTEXT, encodedContextValueString) != 0)
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
            LogError("diagid and diagcreationtimeutc must be present simultaneously.");
            result = MU_FAILURE;
        }
    }
    return result;
}

//
// addPropertiesTouMqttMessage adds user, "system", and diagnostic messages onto MQTT topic string.  Note that "system" properties is a
// construct of the SDK and IoT Hub.  The MQTT protocol itself does not assign any significance to system and user properties (as opposed to AMQP).
// The IOTHUB_MESSAGE_HANDLE structure however does have well-known properties (e.g. IoTHubMessage_SetMessageId) that the SDK treats as system
// properties where we can automatically fill in the key value for in the key=value list.
//
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

    return result;
}

//
// publishTelemetryMsg invokes the umqtt layer to send a PUBLISH message.
//
static int publishTelemetryMsg(PMQTTTRANSPORT_HANDLE_DATA transport_data, MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry, const unsigned char* payload, size_t len)
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

//
// publishDeviceMethodResponseMsg invokes the umqtt to send a PUBLISH message that contains device method call results.
//
static int publishDeviceMethodResponseMsg(MQTTTRANSPORT_HANDLE_DATA* transport_data, int status_code, STRING_HANDLE request_id, const unsigned char* response, size_t response_size)
{
    int result;
    uint16_t packet_id = getNextPacketId(transport_data);

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

//
// destroyDeviceTwinGetMsg frees msg_entry and any data associated with it.
//
static void destroyDeviceTwinGetMsg(MQTT_DEVICE_TWIN_ITEM* msg_entry)
{
    free(msg_entry);
}

//
// createDeviceTwinMsg allocates and fills in structure for MQTT_DEVICE_TWIN_ITEM.
//
static MQTT_DEVICE_TWIN_ITEM* createDeviceTwinMsg(MQTTTRANSPORT_HANDLE_DATA* transport_data, DEVICE_TWIN_MSG_TYPE device_twin_msg_type, uint32_t iothub_msg_id)
{
    MQTT_DEVICE_TWIN_ITEM* result;
    tickcounter_ms_t current_time;

    if (tickcounter_get_current_ms(transport_data->msgTickCounter, &current_time) != 0)
    {
        LogError("Failed retrieving tickcounter info");
        result = NULL;
    }
    else if ((result = (MQTT_DEVICE_TWIN_ITEM*)malloc(sizeof(MQTT_DEVICE_TWIN_ITEM))) == NULL)
    {
        LogError("Failed allocating device twin data.");
        result = NULL;
    }
    else
    {
        memset(result, 0, sizeof(*result));
        result->msgCreationTime = current_time;
        result->packet_id = getNextPacketId(transport_data);
        result->iothub_msg_id = iothub_msg_id;
        result->device_twin_msg_type = device_twin_msg_type;
    }

    return result;
}

//
// publishDeviceTwinGetMsg invokes umqtt to PUBLISH a request to get the twin information.
//
static int publishDeviceTwinGetMsg(MQTTTRANSPORT_HANDLE_DATA* transport_data, MQTT_DEVICE_TWIN_ITEM* mqtt_info)
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

//
// sendPendingGetTwinRequests will send any queued up GetTwin requests during a DoWork loop.
//
static void sendPendingGetTwinRequests(PMQTTTRANSPORT_HANDLE_DATA transportData)
{
    PDLIST_ENTRY dev_twin_item = transportData->pending_get_twin_queue.Flink;

    while (dev_twin_item != &transportData->pending_get_twin_queue)
    {
        DLIST_ENTRY saveListEntry;
        saveListEntry.Flink = dev_twin_item->Flink;
        MQTT_DEVICE_TWIN_ITEM* msg_entry = containingRecord(dev_twin_item, MQTT_DEVICE_TWIN_ITEM, entry);
        (void)DList_RemoveEntryList(dev_twin_item);

        if (publishDeviceTwinGetMsg(transportData, msg_entry) != 0)
        {
            LogError("Failed sending pending get twin request");
            destroyDeviceTwinGetMsg(msg_entry);
        }
        else
        {
            transportData->device_twin_get_sent = true;
        }

        dev_twin_item = saveListEntry.Flink;
    }
}

//
// removeExpiredTwinRequestsFromList removes any requests that have timed out.
//
static void removeExpiredTwinRequestsFromList(PMQTTTRANSPORT_HANDLE_DATA transport_data, tickcounter_ms_t current_ms, DLIST_ENTRY* twin_list)
{
    PDLIST_ENTRY list_item = twin_list->Flink;

    while (list_item != twin_list)
    {
        DLIST_ENTRY next_list_item;
        next_list_item.Flink = list_item->Flink;
        MQTT_DEVICE_TWIN_ITEM* msg_entry = containingRecord(list_item, MQTT_DEVICE_TWIN_ITEM, entry);
        bool item_timed_out = false;

        if ((msg_entry->device_twin_msg_type == RETRIEVE_PROPERTIES) &&
            (((current_ms - msg_entry->msgCreationTime) / 1000) >= ON_DEMAND_GET_TWIN_REQUEST_TIMEOUT_SECS))
        {
            item_timed_out = true;
            if (msg_entry->userCallback != NULL)
            {
                msg_entry->userCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, msg_entry->userContext);
            }
        }
        else if ((msg_entry->device_twin_msg_type == REPORTED_STATE) &&
                 (((current_ms - msg_entry->msgCreationTime) / 1000) >= TWIN_REPORT_UPDATE_TIMEOUT_SECS))
        {
            item_timed_out = true;
            transport_data->transport_callbacks.twin_rpt_state_complete_cb(msg_entry->iothub_msg_id, STATUS_CODE_TIMEOUT_VALUE, transport_data->transport_ctx);
        }

        if (item_timed_out)
        {
            (void)DList_RemoveEntryList(list_item);
            destroyDeviceTwinGetMsg(msg_entry);
        }

        list_item = next_list_item.Flink;
    }

}

//
// removeExpiredTwinRequests removes any requests that have timed out, regardless of how the request invoked.
//
static void removeExpiredTwinRequests(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    tickcounter_ms_t current_ms;

    if (tickcounter_get_current_ms(transport_data->msgTickCounter, &current_ms) == 0)
    {
        removeExpiredTwinRequestsFromList(transport_data, current_ms, &transport_data->pending_get_twin_queue);
        removeExpiredTwinRequestsFromList(transport_data, current_ms, &transport_data->ack_waiting_queue);
    }
}

//
// publishDeviceTwinMsg invokes umqtt to PUBLISH a request for the twin.
//
static int publishDeviceTwinMsg(MQTTTRANSPORT_HANDLE_DATA* transport_data, IOTHUB_DEVICE_TWIN* device_twin_info, MQTT_DEVICE_TWIN_ITEM* mqtt_info)
{
    int result;

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

//
// changeStateToSubscribeIfAllowed attempts to transition the state machine to subscribe, if our
// current state will allow it.
// This function does NOT immediately send the SUBSCRIBE however, instead setting things up
// so the next time DoWork is invoked then the SUBSCRIBE will happen.
//
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

//
// subscribeToNotifyStateIfNeeded sets up to subscribe to the server.
//
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

//
// GetMqttPropertyType compares a specific property in a topic to whether its one that we know about
// (specifically whether IOTHUB_MESSAGE_HANDLE has an accessor for it).
//
static IOTHUB_SYSTEM_PROPERTY_TYPE GetMqttPropertyType(const char* propertyNameAndValue, size_t propertyNameLength)
{
    IOTHUB_SYSTEM_PROPERTY_TYPE result = IOTHUB_SYSTEM_PROPERTY_TYPE_APPLICATION_CUSTOM;
    size_t index = 0;

    for (index = 0; index < sysPropListLength; index++)
    {
        if (propertyNameLength == strlen(sysPropList[index].propName) &&
            memcmp(propertyNameAndValue, sysPropList[index].propName, propertyNameLength) == 0)
        {
            result = sysPropList[index].propertyType;
            break;
        }
    }

    return result;
}

//
// addInputNamePropertyToMsg translates the input name (embedded in the MQTT topic name) into the IoTHubMessage handle
// such that the application can call IoTHubMessage_GetInputName() to retrieve this.  This is only currently used
// in IoT Edge module to module message communication, so that this module receiving the message can know which module invoked in.
//
// For IoT Edge module to module communication, the incoming topic will be of the form devices/{deviceId}/modules/{moduleId}/inputs/{inputName}.
// When this function is called, the caller has already skipped past the devices/{deviceId}/modules prefix.  We would start at inputs/{inputName}.
// On return, we indicate where properties (if specified) start for this message.
//
static const char* addInputNamePropertyToMsg(IOTHUB_MESSAGE_HANDLE iotHubMessage, const char* propertiesStart)
{
    const char* result;
    const char* inputNameStart;
    const char* inputNameEnd;
    char* inputNameCopy = NULL;

    if (((inputNameStart = strchr(propertiesStart, TOPIC_SLASH)) == NULL) || (*(inputNameStart + 1) == '\0'))
    {
        LogError("Cannot find '/' to mark beginning of input name");
        result = NULL;
    }
    else
    {
        inputNameStart++;
        if ((inputNameEnd = strchr(inputNameStart, TOPIC_SLASH)) == NULL)
        {
            LogError("Cannot find '/' after input name");
            result = NULL;
        }
        else 
        {
            size_t inputNameLength = inputNameEnd - inputNameStart;
            if ((inputNameCopy = malloc(inputNameLength + 1)) == NULL)
            {
                LogError("Cannot allocate input name");
                result = NULL;
            }
            else
            {
                memcpy(inputNameCopy, inputNameStart, inputNameLength);
                inputNameCopy[inputNameLength] = '\0';

                if (IoTHubMessage_SetInputName(iotHubMessage, inputNameCopy) != IOTHUB_MESSAGE_OK)
                {
                    LogError("Failed adding input name to msg");
                    result = NULL;
                }
                else
                {
                    result = inputNameEnd + 1;
                }
            }
        }
    }

    free(inputNameCopy);
    return result;
}

//
// addSystemPropertyToMessageWithDecodeIfNeeded adds a "system" property from the incoming MQTT PUBLISH to the iotHubMessage 
// we will ultimately deliver to the application on its callback.
//
static int addSystemPropertyToMessage(IOTHUB_MESSAGE_HANDLE iotHubMessage, IOTHUB_SYSTEM_PROPERTY_TYPE propertyType, const char* propValue)
{
    int result;

    switch (propertyType)
    {
        case IOTHUB_SYSTEM_PROPERTY_TYPE_CREATION_TIME:
        {
            if (IoTHubMessage_SetMessageCreationTimeUtcSystemProperty(iotHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'CreationTimeUtc' property.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        break;

        case IOTHUB_SYSTEM_PROPERTY_TYPE_CORRELATION_ID:
        {
            if (IoTHubMessage_SetCorrelationId(iotHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'correlationId' property.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        break;

        case IOTHUB_SYSTEM_PROPERTY_TYPE_CONNECTION_DEVICE_ID:
        {
            if (IoTHubMessage_SetConnectionDeviceId(iotHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'connectionDeviceId' property.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        break;

        case IOTHUB_SYSTEM_PROPERTY_TYPE_CONNECTION_MODULE_ID:
        {
            if (IoTHubMessage_SetConnectionModuleId(iotHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'connectionModelId' property.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        break;

        case IOTHUB_SYSTEM_PROPERTY_TYPE_MESSAGE_ID:
        {
            if (IoTHubMessage_SetMessageId(iotHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'messageId' property.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        break;

        case  IOTHUB_SYSTEM_PROPERTY_TYPE_MESSAGE_USER_ID:
        {
            if (IoTHubMessage_SetMessageUserIdSystemProperty(iotHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'messageUserId' property.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        break;

        case IOTHUB_SYSTEM_PROPERTY_TYPE_CONTENT_TYPE:
        {
            if (IoTHubMessage_SetContentTypeSystemProperty(iotHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'contentType' property.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        break;

        case IOTHUB_SYSTEM_PROPERTY_TYPE_CONTENT_ENCODING:
        {
            if (IoTHubMessage_SetContentEncodingSystemProperty(iotHubMessage, propValue) != IOTHUB_MESSAGE_OK)
            {
                LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'contentEncoding' property.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        break;

        default:
        {
            // This is an internal error in code as we should never have an unmapped property at this stage.
            LogError("Unknown property type = %d to add to message", propertyType);
            result = MU_FAILURE;
        }
    }

    return result;
}


//
// findMessagePropertyStart takes an MQTT topic PUBLISH'd as input and returns where message properties, if set, begin.
//
static const char* findMessagePropertyStart(PMQTTTRANSPORT_HANDLE_DATA transportData, const char* topic_name, IOTHUB_IDENTITY_TYPE type)
{
    const char *propertiesStart;

    if (type == IOTHUB_TYPE_EVENT_QUEUE)
    {
        const char* input_queue_topic = STRING_c_str(transportData->topic_InputQueue);
        // Substract one from queue length to reflect this having an extra # we subscribed to
        propertiesStart = topic_name + (strlen(input_queue_topic) - 1);
    }
    else if (type == IOTHUB_TYPE_TELEMETRY)
    {
        const char* mqtt_message_queue_topic = STRING_c_str(transportData->topic_MqttMessage);
        // Substract one from queue length to reflect this having an extra # we subscribed to
        propertiesStart = topic_name + (strlen(mqtt_message_queue_topic) - 1);
    }
    else
    {
        LogError("ERROR: type %d is not expected", type);
        propertiesStart = NULL;
    }

    return propertiesStart;
}

//
// AddApplicationProperty adds the custom key/value property name from the incoming MQTT PUBLISH to the iotHubMessage
// we will ultimately deliver to the application on its callback.
//
static int addApplicationPropertyToMessage(MAP_HANDLE propertyMap, const char* propertyNameAndValue, size_t propertyNameLength, const char* propertyValue, bool auto_url_encode_decode)
{
    int result = 0;

    char* propertyNameCopy = NULL;

    // We need to make a copy of propertyName at this point; we don't own the buffer it's part of
    // so we can't just sneak a temporary '\0' in there.  We don't need to make a copy of propertyValue
    // since its part of an otherwise null terminated string.
    if ((propertyNameCopy = (char*)malloc(propertyNameLength + 1)) == NULL)
    {
        LogError("Failed allocating property information");
        result = MU_FAILURE;
    }
    else
    {
        memcpy(propertyNameCopy, propertyNameAndValue, propertyNameLength);
        propertyNameCopy[propertyNameLength] = '\0';
    
        if (auto_url_encode_decode)
        {
            STRING_HANDLE propName_decoded = URL_DecodeString(propertyNameCopy);
            STRING_HANDLE propValue_decoded = URL_DecodeString(propertyValue);
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
            else
            {
                result = 0;
            }
            STRING_delete(propValue_decoded);
            STRING_delete(propName_decoded);
        }
        else if (Map_AddOrUpdate(propertyMap, propertyNameCopy, propertyValue) != MAP_OK)
        {
            LogError("Map_AddOrUpdate failed.");
            result = MU_FAILURE;
        }
    }

    free(propertyNameCopy);
    return result;
}

//
// addSystemPropertyToMessageWithDecodeIfNeeded adds a "system" property from the incoming MQTT PUBLISH to the iotHubMessage 
// we will ultimately deliver to the application on its callback.  This function does the urlDecode, if needed, on the property value.
//
static int addSystemPropertyToMessageWithDecodeIfNeeded(IOTHUB_MESSAGE_HANDLE iotHubMessage, IOTHUB_SYSTEM_PROPERTY_TYPE propertyType, const char* propertyValue, bool auto_url_encode_decode)
{
    int result = 0;

    if (auto_url_encode_decode)
    {
        STRING_HANDLE propValue_decoded;
        if ((propValue_decoded = URL_DecodeString(propertyValue)) == NULL)
        {
            LogError("Failed to URL decode property value");
            result = MU_FAILURE;
        }
        else if (addSystemPropertyToMessage(iotHubMessage, propertyType, STRING_c_str(propValue_decoded)) != 0)
        {
            LogError("Unable to set message property");
            result = MU_FAILURE;
        }
        STRING_delete(propValue_decoded);
    }
    else
    {
        if (addSystemPropertyToMessage(iotHubMessage, propertyType, propertyValue) != 0)
        {
            LogError("Unable to set message property");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

//
// extractMqttProperties parses the MQTT topic PUBLISH'd to this device/module, retrieves properties and fills out the 
// IOTHUB_MESSAGE_HANDLE which will ultimately be delivered to the application callback.
//
static int extractMqttProperties(PMQTTTRANSPORT_HANDLE_DATA transportData, IOTHUB_MESSAGE_HANDLE iotHubMessage, const char* topic_name, IOTHUB_IDENTITY_TYPE type)
{
    int result;

    const char* propertiesStart;
    STRING_TOKENIZER_HANDLE tokenizer = NULL;
    STRING_HANDLE propertyToken = NULL;
    MAP_HANDLE propertyMap;

    if ((propertiesStart = findMessagePropertyStart(transportData, topic_name, type)) == NULL)
    {
        LogError("Cannot find start of properties");
        result = MU_FAILURE;
    }
    else if ((type == IOTHUB_TYPE_EVENT_QUEUE) && ((propertiesStart = addInputNamePropertyToMsg(iotHubMessage, propertiesStart)) == NULL))
    {
        LogError("failure adding input name to property.");
        result = MU_FAILURE;
    }
    else if (*propertiesStart == '\0')
    {
        // No properties were specified.  This is not an error.  We'll return success to caller but skip further processing.
        result = 0;
    }    
    else if ((tokenizer = STRING_TOKENIZER_create_from_char(propertiesStart)) == NULL)
    {
        LogError("failure allocating tokenizer");
        result = MU_FAILURE;
    }
    else if ((propertyToken = STRING_new()) == NULL)
    {
        LogError("Failure to allocate STRING_new.");
        result = MU_FAILURE;
    }
    else if ((propertyMap = IoTHubMessage_Properties(iotHubMessage)) == NULL)
    {
        LogError("Failure to retrieve IoTHubMessage_properties.");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;

        // Iterate through each "propertyKey1=propertyValue1" set, tokenizing off the '&' separating key/value pairs.
        while ((STRING_TOKENIZER_get_next_token(tokenizer, propertyToken, PROPERTY_SEPARATOR) == 0) && result == 0)
        {
            const char* propertyNameAndValue = STRING_c_str(propertyToken);
            const char* propertyValue;
                
            if (propertyNameAndValue == NULL || (*propertyNameAndValue == 0))
            {
                ;
            }
            else if (((propertyValue = strchr(propertyNameAndValue, PROPERTY_EQUALS)) == NULL) ||
                      (*(propertyValue + 1) == 0))
            {
                ;
            }
            else
            {
                size_t propertyNameLength = propertyValue - propertyNameAndValue;
                propertyValue++;

                // After this point, propertyValue is a \0 terminated string (because the STRING_Tokenizer call above made it so)
                // but propertyNameAndValue is NOT \0 terminated and requires its length passed with it.

                IOTHUB_SYSTEM_PROPERTY_TYPE propertyType = GetMqttPropertyType(propertyNameAndValue, propertyNameLength);

                if (propertyType == IOTHUB_SYSTEM_PROPERTY_TYPE_SILENTLY_IGNORE)
                {
                    // To maintain behavior with previous versions of SDKs, "system" properties that we recognize but 
                    // do not have accessors in IOTHUB_MESSAGE_HANDLE will be silently ignored.  The alternative would be adding
                    // them to the application's custom properties, which isn't right as they're not application defined.
                    ;
                }
                else if (propertyType == IOTHUB_SYSTEM_PROPERTY_TYPE_APPLICATION_CUSTOM)
                {
                    result = addApplicationPropertyToMessage(propertyMap, propertyNameAndValue, propertyNameLength, propertyValue, transportData->auto_url_encode_decode);
                }
                else
                {
                    result = addSystemPropertyToMessageWithDecodeIfNeeded(iotHubMessage, propertyType, propertyValue, transportData->auto_url_encode_decode);
                }
            }
        }
    }

    STRING_delete(propertyToken);
    STRING_TOKENIZER_destroy(tokenizer);
    return result;
}

//
// processTwinNotification processes device and module twin updates made by IoT Hub / IoT Edge.
//
static void processTwinNotification(PMQTTTRANSPORT_HANDLE_DATA transportData, MQTT_MESSAGE_HANDLE msgHandle, const char* topicName)
{
    size_t request_id;
    int status_code;
    bool notification_msg;

    if (parseDeviceTwinTopicInfo(topicName, &notification_msg, &request_id, &status_code) != 0)
    {
        LogError("Failure: parsing device topic info");
    }
    else
    {
        const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(msgHandle);
        if (payload == NULL)
        {
            LogError("Failure: invalid payload");
        }
        else if (notification_msg)
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
                        transportData->transport_callbacks.twin_rpt_state_complete_cb(msg_entry->iothub_msg_id, status_code, transportData->transport_ctx);
                        // Only after receiving device twin request should we start listening for patches.
                        (void)subscribeToNotifyStateIfNeeded(transportData);
                    }

                    destroyDeviceTwinGetMsg(msg_entry);
                    break;
                }
                dev_twin_item = saveListEntry.Flink;
            }
        }
    }
}

//
// processDeviceMethodNotification processes a device and module method invocations made by IoT Hub / IoT Edge.
//
static void processDeviceMethodNotification(PMQTTTRANSPORT_HANDLE_DATA transportData, MQTT_MESSAGE_HANDLE msgHandle, const char* topicName)
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
            else if (retrieveDeviceMethodRidInfo(topicName, method_name, dev_method_info->request_id) != 0)
            {
                LogError("Failure: retrieve device topic info");
                STRING_delete(dev_method_info->request_id);
                free(dev_method_info);
            }
            else
            {
                const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(msgHandle);
                if (payload == NULL ||
                    transportData->transport_callbacks.method_complete_cb(STRING_c_str(method_name), payload->message, payload->length, (void*)dev_method_info, transportData->transport_ctx) != 0)
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

static void destroyMessageDispositionContext(MESSAGE_DISPOSITION_CONTEXT* dispositionContext)
{
    free(dispositionContext);
}

static MESSAGE_DISPOSITION_CONTEXT* createMessageDispositionContext(MQTT_MESSAGE_HANDLE msgHandle)
{
    MESSAGE_DISPOSITION_CONTEXT* result = malloc(sizeof(MESSAGE_DISPOSITION_CONTEXT));

    if (result == NULL)
    {
        LogError("Failed allocating MESSAGE_DISPOSITION_CONTEXT");
    }
    else
    {
        result->packet_id = mqttmessage_getPacketId(msgHandle);
        result->qos_value = mqttmessage_getQosType(msgHandle);
    }

    return result;
}

//
// processIncomingMessageNotification processes both C2D messages and messages sent from one IoT Edge module into this module
//
static void processIncomingMessageNotification(PMQTTTRANSPORT_HANDLE_DATA transportData, MQTT_MESSAGE_HANDLE msgHandle, const char* topicName, IOTHUB_IDENTITY_TYPE type)
{
    IOTHUB_MESSAGE_HANDLE IoTHubMessage = NULL;
    const APP_PAYLOAD* appPayload = mqttmessage_getApplicationMsg(msgHandle);

    if (appPayload == NULL)
    {
        LogError("Failure: invalid appPayload.");
    }
    else if ((IoTHubMessage = IoTHubMessage_CreateFromByteArray(appPayload->message, appPayload->length)) == NULL)
    {
        LogError("Failure: IotHub Message creation has failed.");
    }
    else if (extractMqttProperties(transportData, IoTHubMessage, topicName, type) != 0)
    {
        LogError("failure extracting mqtt properties.");
        IoTHubMessage_Destroy(IoTHubMessage);
    }
    else
    {
        MESSAGE_DISPOSITION_CONTEXT* dispositionContext = createMessageDispositionContext(msgHandle);

        if (dispositionContext == NULL)
        {
            LogError("failed creating message disposition context");
            IoTHubMessage_Destroy(IoTHubMessage);
        }
        else if (IoTHubMessage_SetDispositionContext(IoTHubMessage, dispositionContext, destroyMessageDispositionContext) != IOTHUB_MESSAGE_OK)
        {
            LogError("Failed setting disposition context in IOTHUB_MESSAGE_HANDLE");
            destroyMessageDispositionContext(dispositionContext);
            IoTHubMessage_Destroy(IoTHubMessage);
        }
        else
        {
            if (type == IOTHUB_TYPE_EVENT_QUEUE)
            {
                if (!transportData->transport_callbacks.msg_input_cb(IoTHubMessage, transportData->transport_ctx))
                {
                    LogError("IoTHubClientCore_LL_MessageCallbackFromInput returned false");
                    // This will destroy the dispostion context;
                    IoTHubMessage_Destroy(IoTHubMessage);
                }
            }
            else
            {
                if (!transportData->transport_callbacks.msg_cb(IoTHubMessage, transportData->transport_ctx))
                {
                    // Cleanup of allocated memory happens at end of function as pointers are non-NULL.
                    LogError("IoTHubClientCore_LL_MessageCallback returned false");
                    // This will destroy the dispostion context;
                    IoTHubMessage_Destroy(IoTHubMessage);
                }
            }
        }
    }
}

//
// mqttNotificationCallback processes incoming PUBLISH messages sent from Hub (or IoT Edge) to this device.
// This function is invoked by umqtt.  It determines what topic the PUBLISH was directed at (e.g. Device Twin, Method, etc.),
// performs further parsing based on topic, and translates this call up to "iothub_client" layer for ultimate delivery to application callback.
//
static MQTT_CLIENT_ACK_OPTION mqttNotificationCallback(MQTT_MESSAGE_HANDLE msgHandle, void* callbackCtx)
{
    if (msgHandle != NULL && callbackCtx != NULL)
    {
        PMQTTTRANSPORT_HANDLE_DATA transportData = (PMQTTTRANSPORT_HANDLE_DATA)callbackCtx;
        IOTHUB_IDENTITY_TYPE type;

        const char* topicName = mqttmessage_getTopicName(msgHandle);
        if (topicName == NULL)
        {
            LogError("Failure: NULL topic name encountered");
        }
        else if (retrieveTopicType(transportData, topicName, &type) != 0)
        {
            LogError("Received unexpected topic.  Ignoring remainder of request");
        }
        else
        {
            if (type == IOTHUB_TYPE_DEVICE_TWIN)
            {
                processTwinNotification(transportData, msgHandle, topicName);
            }
            else if (type == IOTHUB_TYPE_DEVICE_METHODS)
            {
                processDeviceMethodNotification(transportData, msgHandle, topicName);
            }
            else
            {
                processIncomingMessageNotification(transportData, msgHandle, topicName, type);
            }
        }
    }

    return MQTT_CLIENT_ACK_NONE;
}

//
// mqttOperationCompleteCallback is invoked by umqtt when an operation initiated by the device completes.
// Examples of device initiated operations include PUBLISH, CONNECT, and SUBSCRIBE.
//
static void mqttOperationCompleteCallback(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_RESULT actionResult, const void* msgInfo, void* callbackCtx)
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
                            notifyApplicationOfSendMessageComplete(mqttMsgEntry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_OK);
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
                            transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL, transport_data->transport_ctx);
                        }
                        else if (connack->returnCode == CONN_REFUSED_UNACCEPTABLE_VERSION)
                        {
                            transport_data->isRecoverableError = false;
                        }
                        LogError("Connection Not Accepted: 0x%x: %s", connack->returnCode, retrieveMqttReturnCodes(connack->returnCode));
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
        setSavedTlsOptions(transport_data, options);
        DestroyXioTransport(transport_data);
    }
}

//
// processDisconnectCallback is a callback invoked by umqtt to signal that the disconnection has completed.
//
static void processDisconnectCallback(void* ctx)
{
    if (ctx != NULL)
    {
        int* disconn_recv = (int*)ctx;
        *disconn_recv = 1;
    }
}

//
// DisconnectFromClient will tear down the existing MQTT connection, trying to gracefully send an MQTT DISCONNECT (with a timeout),
// destroy the underlying xio for network communication, and update the transport_data state machine appropriately.
//
//NOTE: After a call to DisconnectFromClient, determine if appropriate to also call
//      transport_data->transport_callbacks.connection_status_cb().
static void DisconnectFromClient(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->currPacketState != DISCONNECT_TYPE)
    {
        if (!transport_data->isDestroyCalled)
        {
            OPTIONHANDLER_HANDLE options = xio_retrieveoptions(transport_data->xioTransport);
            setSavedTlsOptions(transport_data, options);
        }
        // Ensure the disconnect message is sent
        if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_CONNECTED)
        {
            transport_data->disconnect_recv_flag = 0;
            (void)mqtt_client_disconnect(transport_data->mqttClient, processDisconnectCallback, &transport_data->disconnect_recv_flag);
            size_t disconnect_ctr = 0;
            do
            {
                mqtt_client_dowork(transport_data->mqttClient);
                disconnect_ctr++;
                ThreadAPI_Sleep(50);
            } while ((disconnect_ctr < MAX_DISCONNECT_VALUE) && (transport_data->disconnect_recv_flag == 0));
        }
        DestroyXioTransport(transport_data);

        transport_data->device_twin_get_sent = false;
        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
        transport_data->currPacketState = DISCONNECT_TYPE;
    }
}

//
// processErrorCallback is invoked by umqtt when an error has occurred.
//
static void processErrorCallback(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_ERROR error, void* callbackCtx)
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
                transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE, transport_data->transport_ctx);
                break;
            }
            case MQTT_CLIENT_PARSE_ERROR:
            case MQTT_CLIENT_MEMORY_ERROR:
            case MQTT_CLIENT_UNKNOWN_ERROR:
            default:
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

//
// SubscribeToMqttProtocol determines which topics we should SUBSCRIBE to, based on existing state, and then
// invokes the underlying umqtt layer to send the SUBSCRIBE across the network.
//
static void SubscribeToMqttProtocol(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->topics_ToSubscribe != UNSUBSCRIBE_FROM_TOPIC)
    {
        uint32_t topic_subscription = 0;
        size_t subscribe_count = 0;
        uint16_t packet_id = getNextPacketId(transport_data);
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
            subscribe[subscribe_count].qosReturn = DELIVER_AT_LEAST_ONCE;
            topic_subscription |= SUBSCRIBE_INPUT_QUEUE_TOPIC;
            subscribe_count++;
        }

        if (subscribe_count != 0)
        {
            if (mqtt_client_subscribe(transport_data->mqttClient, packet_id, subscribe, subscribe_count) != 0)
            {
                LogError("Failure: mqtt_client_subscribe returned error.");
            }
            else
            {
                transport_data->topics_ToSubscribe &= ~topic_subscription;
                transport_data->currPacketState = SUBSCRIBE_TYPE;
            }
        }
        else
        {
            LogError("Failure: subscribe_topic is empty.");
        }
        transport_data->currPacketState = SUBSCRIBE_TYPE;
    }
    else
    {
        transport_data->currPacketState = PUBLISH_TYPE;
    }
}

//
// RetrieveMessagePayload translates the payload set by the application in messageHandle into payload/length for sending across the network.
//
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

//
// ProcessPendingTelemetryMessages examines each telemetry message the device/module has sent that hasn't yet been PUBACK'd.
// For each message, it might:
// * Ignore it, if its timeout has not yet been reached.
// * Attempt to retry PUBLISH the message, if has remaining retries left.
// * Stop attempting to send the message.  This will result in tearing down the underlying MQTT/TCP connection because it indicates
//   something is wrong.
//
static void ProcessPendingTelemetryMessages(PMQTTTRANSPORT_HANDLE_DATA transport_data)
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
                notifyApplicationOfSendMessageComplete(msg_detail_entry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT);
                (void)DList_RemoveEntryList(current_entry);
                free(msg_detail_entry);

                DisconnectFromClient(transport_data);
                transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, transport_data->transport_ctx);
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
                        notifyApplicationOfSendMessageComplete(msg_detail_entry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
                    }
                    else
                    {
                        if (publishTelemetryMsg(transport_data, msg_detail_entry, messagePayload, messageLength) != 0)
                        {
                            (void)DList_RemoveEntryList(current_entry);
                            notifyApplicationOfSendMessageComplete(msg_detail_entry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
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

//
// CreateTransportProviderIfNecessary will create the underlying xioTransport (which handles networking I/O) and
// set its options, assuming the xioTransport does not already exist.
//
static int CreateTransportProviderIfNecessary(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result;

    if (transport_data->xioTransport == NULL)
    {
        // construct address
        const char* hostAddress = STRING_c_str(transport_data->hostAddress);
        MQTT_TRANSPORT_PROXY_OPTIONS mqtt_proxy_options;

        mqtt_proxy_options.host_address = transport_data->http_proxy_hostname;
        mqtt_proxy_options.port = transport_data->http_proxy_port;
        mqtt_proxy_options.username = transport_data->http_proxy_username;
        mqtt_proxy_options.password = transport_data->http_proxy_password;

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
                    setSavedTlsOptions(transport_data, NULL);
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

//
// buildClientId creates the MQTT ClientId of this device or module.
//
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

//
// buildConfigForUsernameStep2IfNeeded builds the MQTT username.  IoT Hub uses the query string of the userName to optionally
// specify SDK information, product information optionally specified by the application, and optionally the PnP ModelId.
//
static int buildConfigForUsernameStep2IfNeeded(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result;

    if (!transport_data->isConnectUsernameSet)
    {
        STRING_HANDLE userName = NULL;
        STRING_HANDLE modelIdParameter = NULL;
        STRING_HANDLE urlEncodedModelId = NULL;
        const char* modelId = transport_data->transport_callbacks.get_model_id_cb(transport_data->transport_ctx);
        // TODO: The preview API version in SDK is only scoped to scenarios that require the modelId to be set.
        // https://github.com/Azure/azure-iot-sdk-c/issues/1547 tracks removing this once non-preview API versions support modelId.
        const char* apiVersion = IOTHUB_API_VERSION;
        const char* appSpecifiedProductInfo = transport_data->transport_callbacks.prod_info_cb(transport_data->transport_ctx);
        STRING_HANDLE productInfoEncoded = NULL;

        if ((productInfoEncoded = URL_EncodeString((appSpecifiedProductInfo != NULL) ? appSpecifiedProductInfo : DEFAULT_IOTHUB_PRODUCT_IDENTIFIER)) == NULL)
        {
            LogError("Unable to UrlEncode productInfo");
            result = MU_FAILURE;
        }
        else if ((userName = STRING_construct_sprintf("%s?api-version=%s&DeviceClientType=%s", STRING_c_str(transport_data->configPassedThroughUsername), apiVersion, STRING_c_str(productInfoEncoded))) == NULL)
        {
            LogError("Failed constructing string");
            result = MU_FAILURE;
        }
        else if (modelId != NULL)
        {
            if ((urlEncodedModelId = URL_EncodeString(modelId)) == NULL)
            {
                LogError("Failed to URL encode the modelID string");
                result = MU_FAILURE;
            }
            else if ((modelIdParameter = STRING_construct_sprintf("&%s=%s", DT_MODEL_ID_TOKEN, STRING_c_str(urlEncodedModelId))) == NULL)
            {
                LogError("Cannot build modelID string");
                result = MU_FAILURE;
            }
            else if (STRING_concat_with_STRING(userName, modelIdParameter) != 0)
            {
                LogError("Failed to set modelID parameter in connect");
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

        if (result == 0)
        {
            STRING_delete(transport_data->configPassedThroughUsername);
            transport_data->configPassedThroughUsername = userName;
            userName = NULL;
            // setting connect string is only allowed once in the lifetime of the device client.
            transport_data->isConnectUsernameSet = true;
        }

        STRING_delete(userName);
        STRING_delete(modelIdParameter);
        STRING_delete(urlEncodedModelId);
        STRING_delete(productInfoEncoded);
    }
    else
    {
        result = 0;
    }

    return result;
}

//
// SendMqttConnectMsg sends the MQTT CONNECT message across the network.  This function may also
// perform security functionality for building up the required token (optionally invoking into DPS if configured to do so)
//
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
        STRING_HANDLE clientId;
        if (buildConfigForUsernameStep2IfNeeded(transport_data) != 0)
        {
            LogError("Failed to add optional connect parameters.");
            result = MU_FAILURE;
        }
        else if ((clientId = buildClientId(STRING_c_str(transport_data->device_id), STRING_c_str(transport_data->module_id))) == NULL)
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

            if (CreateTransportProviderIfNecessary(transport_data) == 0)
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
                    transport_data->isRetryExpiredCallbackCalled = false;
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

//
// UpdateMqttConnectionStateIfNeeded is used for updating MQTT's underlying connection status during a DoWork loop.
// Among this function's responsibilities:
// * Attempt to establish an MQTT connection if one has not been already.
// * Retries failed connection, if in the correct state.
// * Processes deferred disconnect requests
// * Checks timeouts, for instance on connection establishment time as well as SaS token lifetime (if SAS used)
static int UpdateMqttConnectionStateIfNeeded(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result = 0;

    // Make sure we're not destroying the object
    if (!transport_data->isDestroyCalled)
    {
        RETRY_ACTION retry_action = RETRY_ACTION_RETRY_LATER;

        if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_NOT_CONNECTED && transport_data->isRecoverableError)
        {
            // Note: in case retry_control_should_retry fails, the reconnection shall be attempted anyway (defaulting to policy IOTHUB_CLIENT_RETRY_IMMEDIATE).
            if (!transport_data->conn_attempted || retry_control_should_retry(transport_data->retry_control_handle, &retry_action) != 0 || retry_action == RETRY_ACTION_RETRY_NOW)
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
                // send callback if retry expired
                if (!transport_data->isRetryExpiredCallbackCalled)
                {
                    transport_data->transport_callbacks.connection_status_cb(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED, transport_data->transport_ctx);
                    transport_data->isRetryExpiredCallbackCalled = true;
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
                    uint64_t sas_token_expiry = IoTHubClient_Auth_Get_SasToken_Expiry(transport_data->authorization_module);
                    if ((current_time - transport_data->mqtt_connect_time) / 1000 > (sas_token_expiry*SAS_REFRESH_MULTIPLIER))
                    {
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

// At handle creation time, we don't have all the fields required for building up the user name (e.g. productID)
// We build up as much of the string as we can at this point because we do not store upperConfig after initialization.
// In buildConfigForUsernameStep2IfNeeded, only immediately before we do CONNECT itself, do we complete building up this string.
static STRING_HANDLE buildConfigForUsernameStep1(const IOTHUB_CLIENT_CONFIG* upperConfig, const char* moduleId)
{
    if (moduleId == NULL)
    {
        return STRING_construct_sprintf("%s.%s/%s/", upperConfig->iotHubName, upperConfig->iotHubSuffix, upperConfig->deviceId);
    }
    else
    {
        return STRING_construct_sprintf("%s.%s/%s/%s/", upperConfig->iotHubName, upperConfig->iotHubSuffix, upperConfig->deviceId, moduleId);
    }
}

//
// buildMqttEventString creates the MQTT topic for this device (and optionally module) to PUBLISH telemetry to.
//
static STRING_HANDLE buildMqttEventString(const char* device_id, const char* module_id)
{
    if (module_id == NULL)
    {
        return STRING_construct_sprintf(TOPIC_DEVICE_EVENTS, device_id);
    }
    else
    {
        return STRING_construct_sprintf(TOPIC_MODULE_EVENTS, device_id, module_id);
    }
}

//
// buildDevicesAndModulesPath builds the path used when generating a SaS token for this request.
//
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

//
// buildTopicMqttMsg builds the MQTT topic that is used for C2D messages
//
static STRING_HANDLE buildTopicMqttMsg(const char* device_id)
{
    return STRING_construct_sprintf(TOPIC_DEVICE_MSG, device_id);
}

//
// checkModuleIdsEqual verifies that module Ids coming from different upper layers are equal.
//
static bool checkModuleIdsEqual(const char* transportModuleId, const char* deviceModuleId)
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

//
// InitializeTransportHandleData creates a MQTTTRANSPORT_HANDLE_DATA.
//
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
            freeTransportHandleData(state);
            state = NULL;
        }
        else if ((state->retry_control_handle = retry_control_create(DEFAULT_RETRY_POLICY, DEFAULT_RETRY_TIMEOUT_IN_SECONDS)) == NULL)
        {
            LogError("Failed creating default retry control");
            freeTransportHandleData(state);
            state = NULL;
        }
        else if ((state->device_id = STRING_construct(upperConfig->deviceId)) == NULL)
        {
            LogError("failure constructing device_id.");
            freeTransportHandleData(state);
            state = NULL;
        }
        else if ((moduleId != NULL) && ((state->module_id = STRING_construct(moduleId)) == NULL))
        {
            LogError("failure constructing module_id.");
            freeTransportHandleData(state);
            state = NULL;
        }
        else if ((state->devicesAndModulesPath = buildDevicesAndModulesPath(upperConfig, moduleId)) == NULL)
        {
            LogError("failure constructing devicesPath.");
            freeTransportHandleData(state);
            state = NULL;
        }
        else
        {
            if ((state->topic_MqttEvent = buildMqttEventString(upperConfig->deviceId, moduleId)) == NULL)
            {
                LogError("Could not create topic_MqttEvent for MQTT");
                freeTransportHandleData(state);
                state = NULL;
            }
            else
            {
                state->mqttClient = mqtt_client_init(mqttNotificationCallback, mqttOperationCompleteCallback, state, processErrorCallback, state);
                if (state->mqttClient == NULL)
                {
                    LogError("failure initializing mqtt client.");
                    freeTransportHandleData(state);
                    state = NULL;
                }
                else
                {
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
                        freeTransportHandleData(state);
                        state = NULL;
                    }
                    else if ((state->configPassedThroughUsername = buildConfigForUsernameStep1(upperConfig, moduleId)) == NULL)
                    {
                        freeTransportHandleData(state);
                        state = NULL;
                    }
                    else
                    {
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
                        state->isRetryExpiredCallbackCalled = false;
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
                        state->isConnectUsernameSet = false;
                        state->auto_url_encode_decode = false;
                        state->conn_attempted = false;
                    }
                }
            }
        }
    }
    return state;
}

//
// ProcessSubackDoWork processes state transitions responding to a SUBACK packet.
// This does NOT occur once we receive the SUBACK packet immediately; instead the work is
// deferred to the DoWork loop.
//
static void ProcessSubackDoWork(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if ((transport_data->topic_NotifyState != NULL || transport_data->topic_GetState != NULL) &&
        !transport_data->device_twin_get_sent)
    {
        MQTT_DEVICE_TWIN_ITEM* mqtt_info;

        if ((mqtt_info = createDeviceTwinMsg(transport_data, RETRIEVE_PROPERTIES, 0)) == NULL)
        {
            LogError("Failure: could not create message for twin get command");
        }
        else if (publishDeviceTwinGetMsg(transport_data, mqtt_info) == 0)
        {
            transport_data->device_twin_get_sent = true;
        }
        else
        {
            LogError("Failure: sending device twin get command.");
            destroyDeviceTwinGetMsg(mqtt_info);
        }
    }

    // Publish can be called now and in any event we need to transition out of this state.
    transport_data->currPacketState = PUBLISH_TYPE;
}

//
// ProcessPublishStateDoWork traverses all messages waiting to be sent and attempts to PUBLISH them.
//
static void ProcessPublishStateDoWork(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    PDLIST_ENTRY currentListEntry = transport_data->waitingToSend->Flink;
    while (currentListEntry != transport_data->waitingToSend)
    {
        IOTHUB_MESSAGE_LIST* iothubMsgList = containingRecord(currentListEntry, IOTHUB_MESSAGE_LIST, entry);
        DLIST_ENTRY savedFromCurrentListEntry;
        savedFromCurrentListEntry.Flink = currentListEntry->Flink;

        size_t messageLength;
        const unsigned char* messagePayload = NULL;
        if (!RetrieveMessagePayload(iothubMsgList->messageHandle, &messagePayload, &messageLength))
        {
            (void)(DList_RemoveEntryList(currentListEntry));
            notifyApplicationOfSendMessageComplete(iothubMsgList, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
            LogError("Failure result from IoTHubMessage_GetData");
        }
        else
        {
            MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry = (MQTT_MESSAGE_DETAILS_LIST*)malloc(sizeof(MQTT_MESSAGE_DETAILS_LIST));
            if (mqttMsgEntry == NULL)
            {
                LogError("Allocation Error: Failure allocating MQTT Message Detail List.");
            }
            else
            {
                mqttMsgEntry->retryCount = 0;
                mqttMsgEntry->iotHubMessageEntry = iothubMsgList;
                mqttMsgEntry->packet_id = getNextPacketId(transport_data);
                if (publishTelemetryMsg(transport_data, mqttMsgEntry, messagePayload, messageLength) != 0)
                {
                    (void)(DList_RemoveEntryList(currentListEntry));
                    notifyApplicationOfSendMessageComplete(iothubMsgList, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
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

    if (transport_data->twin_resp_sub_recv)
    {
        sendPendingGetTwinRequests(transport_data);
    }
}

TRANSPORT_LL_HANDLE IoTHubTransport_MQTT_Common_Create(const IOTHUBTRANSPORT_CONFIG* config, MQTT_GET_IO_TRANSPORT get_io_transport, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    PMQTTTRANSPORT_HANDLE_DATA result;
    size_t deviceIdSize;

    if (config == NULL || get_io_transport == NULL || cb_info == NULL)
    {
        LogError("Invalid Argument config: %p, get_io_transport: %p, cb_info: %p", config, get_io_transport, cb_info);
        result = NULL;
    }
    else if (config->auth_module_handle == NULL)
    {
        LogError("Invalid Argument: auth_module_handle is NULL)");
        result = NULL;
    }
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
    else if (config->waitingToSend == NULL)
    {
        LogError("Invalid Argument: waitingToSend is NULL)");
        result = NULL;
    }
    else if (((deviceIdSize = strlen(config->upperConfig->deviceId)) > 128U) || (deviceIdSize == 0))
    {
        LogError("Invalid Argument: DeviceId is of an invalid size");
        result = NULL;
    }
    else if ((config->upperConfig->deviceKey != NULL) && (strlen(config->upperConfig->deviceKey) == 0))
    {
        LogError("Invalid Argument: deviceKey is empty");
        result = NULL;
    }
    else if ((config->upperConfig->deviceSasToken != NULL) && (strlen(config->upperConfig->deviceSasToken) == 0))
    {
        LogError("Invalid Argument: deviceSasToken is empty");
        result = NULL;
    }
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
    return result;
}

void IoTHubTransport_MQTT_Common_Destroy(TRANSPORT_LL_HANDLE handle)
{
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
            notifyApplicationOfSendMessageComplete(mqttMsgEntry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY);
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

            destroyDeviceTwinGetMsg(mqtt_device_twin);
        }
        while (!DList_IsListEmpty(&transport_data->pending_get_twin_queue))
        {
            PDLIST_ENTRY currentEntry = DList_RemoveHeadList(&transport_data->pending_get_twin_queue);

            MQTT_DEVICE_TWIN_ITEM* mqtt_device_twin = containingRecord(currentEntry, MQTT_DEVICE_TWIN_ITEM, entry);

            mqtt_device_twin->userCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, mqtt_device_twin->userContext);

            destroyDeviceTwinGetMsg(mqtt_device_twin);
        }

        freeTransportHandleData(transport_data);
    }
}

int IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(TRANSPORT_LL_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data == NULL)
    {
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else
    {
        if (transport_data->topic_GetState == NULL)
        {
            transport_data->topic_GetState = STRING_construct(TOPIC_GET_DESIRED_STATE);
            if (transport_data->topic_GetState == NULL)
            {
                LogError("Failure: unable constructing reported state topic");
                result = MU_FAILURE;
            }
            else
            {
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
    if (transport_data != NULL)
    {
        if (transport_data->topic_GetState != NULL)
        {
            transport_data->topics_ToSubscribe &= ~SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
            STRING_delete(transport_data->topic_GetState);
            transport_data->topic_GetState = NULL;
        }
        if (transport_data->topic_NotifyState != NULL)
        {
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

    if (handle == NULL || completionCallback == NULL)
    {
        LogError("Invalid argument (handle=%p, completionCallback=%p)", handle, completionCallback);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
        MQTT_DEVICE_TWIN_ITEM* mqtt_info;

        if ((mqtt_info = createDeviceTwinMsg(transport_data, RETRIEVE_PROPERTIES, 0)) == NULL)
        {
            LogError("Failed creating the device twin get request message");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if (tickcounter_get_current_ms(transport_data->msgTickCounter, &mqtt_info->msgCreationTime) != 0)
        {
            LogError("Failed setting the get twin request enqueue time");
            destroyDeviceTwinGetMsg(mqtt_info);
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            mqtt_info->userCallback = completionCallback;
            mqtt_info->userContext = callbackContext;

            DList_InsertTailList(&transport_data->pending_get_twin_queue, &mqtt_info->entry);

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
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else
    {
        if (transport_data->topic_DeviceMethods == NULL)
        {
            transport_data->topic_DeviceMethods = STRING_construct(TOPIC_DEVICE_METHOD_SUBSCRIBE);
            if (transport_data->topic_DeviceMethods == NULL)
            {
                LogError("Failure: unable constructing device method subscribe topic");
                result = MU_FAILURE;
            }
            else
            {
                transport_data->topics_ToSubscribe |= SUBSCRIBE_DEVICE_METHOD_TOPIC;
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

void IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod(TRANSPORT_LL_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data != NULL)
    {
        if (transport_data->topic_DeviceMethods != NULL)
        {
            const char* unsubscribe[1];
            unsubscribe[0] = STRING_c_str(transport_data->topic_DeviceMethods);

            if (mqtt_client_unsubscribe(transport_data->mqttClient, getNextPacketId(transport_data), unsubscribe, 1) != 0)
            {
                LogError("Failure calling mqtt_client_unsubscribe");
            }

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
        DEVICE_METHOD_INFO* dev_method_info = (DEVICE_METHOD_INFO*)methodId;
        if (dev_method_info == NULL)
        {
            LogError("Failure: DEVICE_METHOD_INFO was NULL");
            result = MU_FAILURE;
        }
        else
        {
            if (publishDeviceMethodResponseMsg(transport_data, status, dev_method_info->request_id, response, respSize) != 0)
            {
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
        result = MU_FAILURE;
        LogError("Failure: invalid TRANSPORT_LL_HANDLE parameter specified");
    }
    return result;
}

int IoTHubTransport_MQTT_Common_Subscribe(TRANSPORT_LL_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data == NULL)
    {
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else if (transport_data->module_id != NULL)
    {
        // This very strongly points to an internal error.  This code path should never be reachable from the IoTHub module client API.
        LogError("Cannot specify modules for C2D style messages, per IoT Hub protocol limitations.");
        result = MU_FAILURE;
    }
    else
    {
        transport_data->topic_MqttMessage = buildTopicMqttMsg(STRING_c_str(transport_data->device_id));
        if (transport_data->topic_MqttMessage == NULL)
        {
            LogError("Failure constructing Message Topic");
            result = MU_FAILURE;
        }
        else
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_TELEMETRY_TOPIC;
            changeStateToSubscribeIfAllowed(transport_data);
            result = 0;
        }
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe(TRANSPORT_LL_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data != NULL)
    {
        const char* unsubscribe[1];
        unsubscribe[0] = STRING_c_str(transport_data->topic_MqttMessage);
        if (mqtt_client_unsubscribe(transport_data->mqttClient, getNextPacketId(transport_data), unsubscribe, 1) != 0)
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
                MQTT_DEVICE_TWIN_ITEM* mqtt_info = createDeviceTwinMsg(transport_data, REPORTED_STATE, iothub_item->device_twin->item_id);
                if (mqtt_info == NULL)
                {
                    result = IOTHUB_PROCESS_ERROR;
                }
                else
                {
                    DList_InsertTailList(&transport_data->ack_waiting_queue, &mqtt_info->entry);

                    if (publishDeviceTwinMsg(transport_data, iothub_item->device_twin, mqtt_info) != 0)
                    {
                        DList_RemoveEntryList(&mqtt_info->entry);

                        free(mqtt_info);
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
                result = IOTHUB_PROCESS_CONTINUE;
            }
        }
        else
        {
            result = IOTHUB_PROCESS_NOT_CONNECTED;
        }
    }
    return result;
}

void IoTHubTransport_MQTT_Common_DoWork(TRANSPORT_LL_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data != NULL)
    {
        if (UpdateMqttConnectionStateIfNeeded(transport_data) == 0)
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
                ProcessSubackDoWork(transport_data);
            }
            else if (transport_data->currPacketState == PUBLISH_TYPE)
            {
                ProcessPublishStateDoWork(transport_data);
            }
            mqtt_client_dowork(transport_data->mqttClient);
        }

        // Check the ack messages timeouts
        ProcessPendingTelemetryMessages(transport_data);
        removeExpiredTwinRequests(transport_data);
    }
}

int IoTHubTransport_MQTT_Common_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else
    {
        RETRY_CONTROL_HANDLE new_retry_control_handle;

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

            result = 0;
        }
    }

    return result;
}


IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_GetSendStatus(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL || iotHubClientStatus == NULL)
    {
        LogError("invalid argument.");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* handleData = (MQTTTRANSPORT_HANDLE_DATA*)handle;
        if (!DList_IsListEmpty(handleData->waitingToSend) || !DList_IsListEmpty(&(handleData->telemetry_waitingForAck)))
        {
            *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_BUSY;
        }
        else
        {
            *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_IDLE;
        }
        result = IOTHUB_CLIENT_OK;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
{
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
            int* keepAliveOption = (int*)value;
            if (*keepAliveOption != transport_data->keepAliveValue)
            {
                transport_data->keepAliveValue = (uint16_t)(*keepAliveOption);
                if (transport_data->mqttClientStatus != MQTT_CLIENT_STATUS_NOT_CONNECTED)
                {
                    DisconnectFromClient(transport_data);
                }
            }
            result = IOTHUB_CLIENT_OK;
        }
        else if ((strcmp(OPTION_X509_CERT, option) == 0) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509 && cred_type != IOTHUB_CREDENTIAL_TYPE_UNKNOWN))
        {
            LogError("x509certificate specified, but authentication method is not x509");
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
        else if ((strcmp(OPTION_X509_PRIVATE_KEY, option) == 0) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509 && cred_type != IOTHUB_CREDENTIAL_TYPE_UNKNOWN))
        {
            LogError("x509privatekey specified, but authentication method is not x509");
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
        else if (strcmp(OPTION_RETRY_INTERVAL_SEC, option) == 0)
        {
            if (retry_control_set_option(transport_data->retry_control_handle, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, value) != 0)
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
            if (retry_control_set_option(transport_data->retry_control_handle, RETRY_CONTROL_OPTION_MAX_DELAY_IN_SECS, value) != 0)
            {
                LogError("Failure setting retry max delay option");
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

            if (transport_data->xioTransport != NULL)
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

                transport_data->http_proxy_port = proxy_options->port;
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
                    freeProxyData(transport_data);

                    transport_data->http_proxy_hostname = copied_proxy_hostname;
                    transport_data->http_proxy_username = copied_proxy_username;
                    transport_data->http_proxy_password = copied_proxy_password;

                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else
        {
            if (((strcmp(OPTION_X509_CERT, option) == 0) || (strcmp(OPTION_X509_PRIVATE_KEY, option) == 0)) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509))
            {
                IoTHubClient_Auth_Set_x509_Type(transport_data->authorization_module, true);
            }

            if (CreateTransportProviderIfNecessary(transport_data) == 0)
            {
                if (xio_setoption(transport_data->xioTransport, option, value) == 0)
                {
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
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

TRANSPORT_LL_HANDLE IoTHubTransport_MQTT_Common_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend)
{
    TRANSPORT_LL_HANDLE result = NULL;

    if ((handle == NULL) || (device == NULL) || (waitingToSend == NULL))
    {
        LogError("IoTHubTransport_MQTT_Common_Register: handle, device or waitingToSend is NULL.");
        result = NULL;
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)handle;

        if (device->deviceId == NULL)
        {
            LogError("IoTHubTransport_MQTT_Common_Register: deviceId is NULL.");
            result = NULL;
        }
        else if ((device->deviceKey != NULL) && (device->deviceSasToken != NULL))
        {
            LogError("IoTHubTransport_MQTT_Common_Register: Both deviceKey and deviceSasToken are defined. Only one can be used.");
            result = NULL;
        }
        else
        {
            if (strcmp(STRING_c_str(transport_data->device_id), device->deviceId) != 0)
            {
                LogError("IoTHubTransport_MQTT_Common_Register: deviceId does not match.");
                result = NULL;
            }
            else if (!checkModuleIdsEqual(STRING_c_str(transport_data->module_id), device->moduleId))
            {
                LogError("IoTHubTransport_MQTT_Common_Register: moduleId does not match.");
                result = NULL;
            }
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
                    result = (TRANSPORT_LL_HANDLE)handle;
                }
            }
        }
    }

    return result;
}

void IoTHubTransport_MQTT_Common_Unregister(TRANSPORT_LL_HANDLE deviceHandle)
{
    if (deviceHandle != NULL)
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)deviceHandle;

        transport_data->isRegistered = false;
    }
}

STRING_HANDLE IoTHubTransport_MQTT_Common_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    STRING_HANDLE result;
    if (handle == NULL)
    {
        result = NULL;
    }
    else if ((result = STRING_clone(((MQTTTRANSPORT_HANDLE_DATA*)(handle))->hostAddress)) == NULL)
    {
        LogError("Cannot provide the target host name (STRING_clone failed).");
    }

    return result;
}

// Azure IoT Hub (and Edge) support the following responses for devicebound messages:
// ACCEPTED: message shall be dequeued by the hub and consider the delivery completed.
// ABANDONED: message shall remain in the hub queue, and re-delivered upon client re-connection.
// REJECTED: message shall be dequeued by the hub and not not re-delivered anymore.
// The MQTT protocol by design only supports acknowledging the receipt of a message (through PUBACK).
// To simulate the message responses above, this function behaves as follows:
// ACCEPTED: a PUBACK is sent to the Hub/Edge.
// ABANDONED: no response is sent to the Hub/Edge.
// REJECTED: a PUBACK is sent to the Hub/Edge (same as for ACCEPTED).
IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_SendMessageDisposition(IOTHUB_DEVICE_HANDLE handle, IOTHUB_MESSAGE_HANDLE messageHandle, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    IOTHUB_CLIENT_RESULT result;
    if (handle == NULL || messageHandle == NULL)
    {
        LogError("Invalid argument (handle=%p, messageHandle=%p", handle, messageHandle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        if (disposition == IOTHUBMESSAGE_ACCEPTED || disposition == IOTHUBMESSAGE_REJECTED)
        {
            MESSAGE_DISPOSITION_CONTEXT* msgDispCtx = NULL;

            if (IoTHubMessage_GetDispositionContext(messageHandle, &msgDispCtx) != IOTHUB_MESSAGE_OK ||
                msgDispCtx == NULL)
            {
                LogError("invalid message handle (no disposition context found)");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;

                if (mqtt_client_send_message_response(transport_data->mqttClient, msgDispCtx->packet_id, msgDispCtx->qos_value) != 0)
                {
                    LogError("Failed sending ACK for MQTT message (packet_id=%u)", msgDispCtx->packet_id);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }

        // This will also destroy the disposition context.
        IoTHubMessage_Destroy(messageHandle);
    }

    return result;
}


int IoTHubTransport_MQTT_Common_Subscribe_InputQueue(TRANSPORT_LL_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data == NULL)
    {
        LogError("Invalid handle parameter. NULL.");
        result = MU_FAILURE;
    }
    else if (transport_data->module_id == NULL)
    {
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
        transport_data->topics_ToSubscribe |= SUBSCRIBE_INPUT_QUEUE_TOPIC;
        changeStateToSubscribeIfAllowed(transport_data);
        result = 0;
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue(TRANSPORT_LL_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if ((transport_data != NULL) && (transport_data->topic_InputQueue != NULL))
    {
        const char* unsubscribe[1];
        unsubscribe[0] = STRING_c_str(transport_data->topic_InputQueue);
        if (mqtt_client_unsubscribe(transport_data->mqttClient, getNextPacketId(transport_data), unsubscribe, 1) != 0)
        {
            LogError("Failure calling mqtt_client_unsubscribe");
        }
        STRING_delete(transport_data->topic_InputQueue);
        transport_data->topic_InputQueue = NULL;
        transport_data->topics_ToSubscribe &= ~SUBSCRIBE_INPUT_QUEUE_TOPIC;
    }
    else
    {
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
