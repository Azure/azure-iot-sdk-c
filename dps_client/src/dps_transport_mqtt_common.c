// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <inttypes.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_hub_modules/dps_transport_mqtt_common.h"
#include "azure_umqtt_c/mqtt_client.h"

#define AMQP_MAX_LINK_SIZE              UINT64_MAX
#define AMQP_MAX_RECV_LINK_SIZE         65536
#define DPS_SUBSCRIBE_TOPIC_COUNT       1

static const char* HEADER_KEY_AUTHORIZATION = "Authorization";
static const char* SAS_TOKEN_KEY_NAME = "registration";
static const char* MQTT_SUBSCRIBE_TOPIC = "$dps/registrations/res/#";
static const char* MQTT_USERNAME_FMT = "%s/registrations/%s/api-version=%s&DeviceClientType=%s";
static const char* CLIENT_VERSION = "123.123";

static const char* MQTT_REGISTER_MESSAGE_FMT = "$dps/registrations/POST/iotdps-register-me/?$rid=%d";
static const char* MQTT_STATUS_MESSAGE_FMT = "$dps/registrations/GET/iotdps-operation-status/?$rid=%d&operationId=%s";

static const char* DPS_ASSIGNED_STATUS = "Assigned";
static const char* DPS_ASSIGNING_STATUS = "Assigning";
static const char* DPS_UNASSIGNED_STATUS = "Unassigned";

typedef enum MQTT_TRANSPORT_STATE_TAG
{
    MQTT_STATE_IDLE,
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_CONNECTED,

    MQTT_STATE_SUBSCRIBING,
    MQTT_STATE_SUBSCRIBED,

    MQTT_STATE_ERROR
} MQTT_TRANSPORT_STATE;

typedef enum DPS_TRANSPORT_STATE_TAG
{
    TRANSPORT_CLIENT_STATE_IDLE,

    TRANSPORT_CLIENT_STATE_REG_SEND,
    TRANSPORT_CLIENT_STATE_REG_SENT,
    TRANSPORT_CLIENT_STATE_REG_RECV,

    TRANSPORT_CLIENT_STATE_STATUS_SEND,
    TRANSPORT_CLIENT_STATE_STATUS_SENT,
    TRANSPORT_CLIENT_STATE_STATUS_RECV,


    TRANSPORT_CLIENT_STATE_ERROR
} DPS_TRANSPORT_STATE;

typedef struct DPS_TRANSPORT_MQTT_INFO_TAG
{
    DPS_TRANSPORT_REGISTER_DATA_CALLBACK register_data_cb;
    void* user_ctx;
    DPS_TRANSPORT_STATUS_CALLBACK status_cb;
    void* status_ctx;
    DPS_TRANSPORT_CHALLENGE_CALLBACK challenge_cb;
    void* challenge_ctx;
    DPS_TRANSPORT_JSON_PARSE json_parse_cb;
    void* json_ctx;

    MQTT_CLIENT_HANDLE mqtt_client;

    char* hostname;

    HTTP_PROXY_OPTIONS proxy_option;

    char* x509_cert;
    char* private_key;

    char* certificate;

    BUFFER_HANDLE ek;
    BUFFER_HANDLE srk;
    char* registration_id;
    char* scope_id;
    char* sas_token;

    char* operation_id;

    char* api_version;
    char* payload_data;

    bool log_trace;

    uint16_t packet_id;

    DPS_HSM_TYPE hsm_type;

    DPS_MQTT_TRANSPORT_IO transport_io_cb;

    DPS_TRANSPORT_STATE transport_state;
    MQTT_TRANSPORT_STATE mqtt_state;

    XIO_HANDLE transport_io;
} DPS_TRANSPORT_MQTT_INFO;

static uint16_t get_next_packet_id(DPS_TRANSPORT_MQTT_INFO* mqtt_info)
{
    if (mqtt_info->packet_id + 1 >= USHRT_MAX)
    {
        mqtt_info->packet_id = 1;
    }
    else
    {
        mqtt_info->packet_id++;
    }
    return mqtt_info->packet_id;
}

static void mqtt_error_callback(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_ERROR error, void* user_ctx)
{
    (void)handle;
    if (user_ctx != NULL)
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)user_ctx;
        switch (error)
        {
            case MQTT_CLIENT_CONNECTION_ERROR:
            case MQTT_CLIENT_COMMUNICATION_ERROR:
                LogError("MQTT communication error");
                break;
            case MQTT_CLIENT_NO_PING_RESPONSE:
                LogError("Mqtt Ping Response was not encountered.  Reconnecting device...");
                break;

            case MQTT_CLIENT_PARSE_ERROR:
            case MQTT_CLIENT_MEMORY_ERROR:
            case MQTT_CLIENT_UNKNOWN_ERROR:
            default:
            {
                LogError("INTERNAL ERROR: unexpected error value received %d", error);
                break;
            }
        }
        mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
        mqtt_info->mqtt_state = MQTT_STATE_ERROR;
    }
    else
    {
        LogError("mqtt_error_callback was invoked with a NULL context");
    }
}

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

static void mqtt_operation_complete_callback(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_RESULT event_result, const void* msg_info, void* user_ctx)
{
    (void)handle;
    (void)msg_info;
    if (user_ctx != NULL)
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)user_ctx;
        switch (event_result)
        {
            case MQTT_CLIENT_ON_CONNACK:
            {
                const CONNECT_ACK* connack = (const CONNECT_ACK*)msg_info;
                if (connack != NULL)
                {
                    if (connack->returnCode == CONNECTION_ACCEPTED)
                    {
                        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_052: [ Once the mqtt CONNACK is recieved dps_transport_common_mqtt_dowork shall set mqtt_state to MQTT_STATE_CONNECTED ] */
                        mqtt_info->mqtt_state = MQTT_STATE_CONNECTED;
                    }
                    else
                    {
                        LogError("Connection Not Accepted: 0x%x: %s", connack->returnCode, retrieve_mqtt_return_codes(connack->returnCode));
                        mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                        mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                    }
                }
                else
                {
                    mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                    mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                    LogError("CONNECT_ACK packet is NULL");
                }
                break;
            }
            case MQTT_CLIENT_ON_SUBSCRIBE_ACK:
            {
                const SUBSCRIBE_ACK* suback = (const SUBSCRIBE_ACK*)msg_info;
                if (suback != NULL)
                {
                    size_t index = 0;
                    for (index = 0; index < suback->qosCount; index++)
                    {
                        if (suback->qosReturn[index] == DELIVER_FAILURE)
                        {
                            LogError("Subscribe delivery failure of subscribe %zu", index);
                            break;
                        }
                    }

                    if (index == suback->qosCount)
                    {
                        mqtt_info->mqtt_state = MQTT_STATE_SUBSCRIBED;
                    }
                    else
                    {
                        mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                        mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                    }
                }
                else
                {
                    mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                    mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                    LogError("SUBSCRIBE_ACK packet is NULL");
                }
                break;
            }
            case MQTT_CLIENT_ON_PUBLISH_ACK:
            case MQTT_CLIENT_ON_PUBLISH_COMP:
            case MQTT_CLIENT_ON_PUBLISH_RECV:
            case MQTT_CLIENT_ON_PUBLISH_REL:
            case MQTT_CLIENT_ON_DISCONNECT:
            case MQTT_CLIENT_ON_UNSUBSCRIBE_ACK:
                break;
            default:
                LogError("Unknown MQTT_CLIENT_EVENT_RESULT item %d");
                break;
        }
    }
    else
    {
        LogError("mqtt_operation_complete_callback was invoked with a NULL context");
    }
}

static void mqtt_notification_callback(MQTT_MESSAGE_HANDLE handle, void* user_ctx)
{
    if (user_ctx != NULL)
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)user_ctx;

        const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(handle);
        if (payload != NULL)
        {
            if (mqtt_info->payload_data != NULL)
            {
                free(mqtt_info->payload_data);
                mqtt_info->payload_data = NULL;
            }
            
            if ((mqtt_info->payload_data = malloc(payload->length + 1)) == NULL)
            {
                LogError("failure allocating payload data");
                mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                mqtt_info->mqtt_state = MQTT_STATE_ERROR;
            }
            else
            {
                memset(mqtt_info->payload_data, 0, payload->length + 1);
                memcpy(mqtt_info->payload_data, payload->message, payload->length);
                if (mqtt_info->transport_state == TRANSPORT_CLIENT_STATE_REG_SENT)
                {
                    mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_REG_RECV;
                }
                else
                {
                    mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_RECV;
                }
            }
        }
        else
        {
            LogError("failure NULL message encountered from umqtt");
            mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            mqtt_info->mqtt_state = MQTT_STATE_ERROR;
        }
    }
    else
    {
        LogError("mqtt_notification_callback was invoked with a NULL context");
    }
}

static int send_mqtt_message(DPS_TRANSPORT_MQTT_INFO* mqtt_info, const char* msg_topic)
{
    int result;
    MQTT_MESSAGE_HANDLE msg_handle = NULL;

    if ((msg_handle = mqttmessage_create(get_next_packet_id(mqtt_info), msg_topic, DELIVER_AT_MOST_ONCE, NULL, 0)) == NULL)
    {
        LogError("Failed creating mqtt message");
        result = __FAILURE__;
    }
    else
    {
        if (mqtt_client_publish(mqtt_info->mqtt_client, msg_handle) != 0)
        {
            LogError("Failed publishing client message");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
        mqttmessage_destroy(msg_handle);
    }
    return result;
}

static int send_register_message(DPS_TRANSPORT_MQTT_INFO* mqtt_info)
{
    int result;
    char* msg_topic;

    size_t length = strlen(MQTT_REGISTER_MESSAGE_FMT) + 8;
    if ((msg_topic = malloc(length + 1)) == NULL)
    {
        LogError("Failed allocating mqtt registration message");
        result = __FAILURE__;
    }
    else if (sprintf(msg_topic, MQTT_REGISTER_MESSAGE_FMT, mqtt_info->packet_id) <= 0)
    {
        LogError("Failed setting registration message");
        free(msg_topic);
        result = __FAILURE__;
    }
    else
    {
        result = send_mqtt_message(mqtt_info, msg_topic);
        free(msg_topic);
    }
    return result;
}

static int send_operation_status_message(DPS_TRANSPORT_MQTT_INFO* mqtt_info)
{
    int result;
    char* msg_topic;

    size_t length = strlen(MQTT_STATUS_MESSAGE_FMT) + strlen(mqtt_info->operation_id) + 8;
    if ((msg_topic = malloc(length + 1)) == NULL)
    {
        LogError("Failed allocating mqtt status message");
        result = __FAILURE__;
    }
    else if (sprintf(msg_topic, MQTT_STATUS_MESSAGE_FMT, mqtt_info->packet_id, mqtt_info->operation_id) <= 0)
    {
        LogError("Failed creating mqtt status message");
        free(msg_topic);
        result = __FAILURE__;
    }
    else
    {
        result = send_mqtt_message(mqtt_info, msg_topic);
        free(msg_topic);
    }
    return result;
}

static int subscribe_to_topic(DPS_TRANSPORT_MQTT_INFO* mqtt_info)
{
    int result;
    SUBSCRIBE_PAYLOAD subscribe[DPS_SUBSCRIBE_TOPIC_COUNT];
    subscribe[0].subscribeTopic = MQTT_SUBSCRIBE_TOPIC;
    subscribe[0].qosReturn = DELIVER_AT_LEAST_ONCE;

    if (mqtt_client_subscribe(mqtt_info->mqtt_client, get_next_packet_id(mqtt_info), subscribe, DPS_SUBSCRIBE_TOPIC_COUNT) != 0)
    {
        LogError("Failed subscribing to topic.");
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

static char* construct_dps_username(DPS_TRANSPORT_MQTT_INFO* mqtt_info)
{
    char* result;
    char* dps_name = NULL;
    size_t length;
    size_t hostname_len;

    // Extract dps name from hostname
    hostname_len = strlen(mqtt_info->hostname);
    for (size_t index = 0; index < hostname_len; index++)
    {
        if (mqtt_info->hostname[index] == '.')
        {
            if ((dps_name = malloc(index + 1)) == NULL)
            {
                LogError("Failure allocating dps name");
                result = NULL;
            }
            else
            {
                strncpy(dps_name, mqtt_info->hostname, index);
                dps_name[index] = '\0';
            }
            break;
        }
    }

    if (dps_name != NULL)
    {
        length = strlen(MQTT_USERNAME_FMT) + strlen(dps_name) + strlen(mqtt_info->registration_id) + strlen(mqtt_info->scope_id) + strlen(mqtt_info->api_version) + strlen(CLIENT_VERSION);
        if ((result = malloc(length + 1)) == NULL)
        {
            LogError("Failure allocating username");
            result = NULL;
        }
        else if (sprintf(result, MQTT_USERNAME_FMT, mqtt_info->scope_id, mqtt_info->registration_id, mqtt_info->api_version, CLIENT_VERSION) <= 0)
        {
            LogError("Failure creating mqtt username");
        }
        free(dps_name);
    }
    else
    {
        result = NULL;
    }
    return result;
}

static int create_connection(DPS_TRANSPORT_MQTT_INFO* mqtt_info)
{
    int result;
    HTTP_PROXY_OPTIONS* transport_proxy;
    MQTT_CLIENT_OPTIONS options = { 0 };
    char* username_info;

    if (mqtt_info->proxy_option.host_address != NULL)
    {
        transport_proxy = &mqtt_info->proxy_option;
    }
    else
    {
        transport_proxy = NULL;
    }

    if ((mqtt_info->transport_io = mqtt_info->transport_io_cb(mqtt_info->hostname, transport_proxy)) == NULL)
    {
        LogError("Failure calling transport_io callback");
        result = __FAILURE__;
    }
    else
    {
        bool ignore_check = true;
        (void)xio_setoption(mqtt_info->transport_io, "ignore_server_name_check", &ignore_check);

        if (mqtt_info->certificate != NULL && xio_setoption(mqtt_info->transport_io, "TrustedCerts", mqtt_info->certificate) != 0)
        {
            LogError("Failure setting trusted certs");
            result = __FAILURE__;
            xio_destroy(mqtt_info->transport_io);
        }
        else if ((username_info = construct_dps_username(mqtt_info)) == NULL)
        {
            LogError("Failure creating username info");
            xio_destroy(mqtt_info->transport_io);
            mqtt_info->transport_io = NULL;
            result = __FAILURE__;
        }
        else
        {
            (void)mqtt_client_set_trace(mqtt_info->mqtt_client, mqtt_info->log_trace, false);

            if (mqtt_info->hsm_type == DPS_HSM_TYPE_X509)
            {
                if (mqtt_info->x509_cert != NULL && mqtt_info->private_key != NULL)
                {
                    if (xio_setoption(mqtt_info->transport_io, OPTION_X509_ECC_CERT, mqtt_info->x509_cert) != 0)
                    {
                        LogError("Failure setting x509 cert on xio");
                        xio_destroy(mqtt_info->transport_io);
                        mqtt_info->transport_io = NULL;
                        result = __FAILURE__;
                    }
                    else if (xio_setoption(mqtt_info->transport_io, OPTION_X509_ECC_KEY, mqtt_info->private_key) != 0)
                    {
                        LogError("Failure setting x509 key on xio");
                        xio_destroy(mqtt_info->transport_io);
                        mqtt_info->transport_io = NULL;
                        result = __FAILURE__;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else
                {
                    LogError("x509 certificate is NULL");
                    xio_destroy(mqtt_info->transport_io);
                    mqtt_info->transport_io = NULL;
                    result = __FAILURE__;
                }
            }
            else
            {
                result = 0;
            }

            if (result == 0)
            {
                options.clientId = NULL;
                options.username = username_info;
                options.clientId = mqtt_info->registration_id;
                options.useCleanSession = 1;
                options.log_trace = mqtt_info->log_trace;
                options.qualityOfServiceValue = DELIVER_AT_LEAST_ONCE;
                if (mqtt_client_connect(mqtt_info->mqtt_client, mqtt_info->transport_io, &options) != 0)
                {
                    LogError("Failure connecting to mqtt server");
                    xio_destroy(mqtt_info->transport_io);
                    mqtt_info->transport_io = NULL;
                    result = __FAILURE__;
                }
                else
                {
                    result = 0;
                }
            }
            free(username_info);
        }
    }
    return result;
}

static void free_json_parse_info(DPS_JSON_INFO* parse_info)
{
    switch (parse_info->dps_status)
    {
        case DPS_TRANSPORT_STATUS_UNASSIGNED:
            BUFFER_delete(parse_info->authorization_key);
            free(parse_info->key_name);
            break;
        case DPS_TRANSPORT_STATUS_ASSIGNED:
            BUFFER_delete(parse_info->authorization_key);
            free(parse_info->iothub_uri);
            free(parse_info->device_id);
            break;
        case DPS_TRANSPORT_STATUS_ASSIGNING:
            free(parse_info->operation_id);
            break;
    }
    free(parse_info);
}

void cleanup_dps_mqtt_data(DPS_TRANSPORT_MQTT_INFO* mqtt_info)
{
    free(mqtt_info->hostname);
    free(mqtt_info->registration_id);
    free(mqtt_info->operation_id);
    free(mqtt_info->api_version);
    free(mqtt_info->scope_id);
    free(mqtt_info->certificate);
    free((char*)mqtt_info->proxy_option.host_address);
    free((char*)mqtt_info->proxy_option.username);
    free((char*)mqtt_info->proxy_option.password);
    free(mqtt_info->x509_cert);
    free(mqtt_info->private_key);
    free(mqtt_info->sas_token);
    free(mqtt_info->payload_data);
    free(mqtt_info);
}

DPS_TRANSPORT_HANDLE dps_transport_common_mqtt_create(const char* uri, DPS_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* dps_api_version, DPS_MQTT_TRANSPORT_IO transport_io)
{
    DPS_TRANSPORT_MQTT_INFO* result;
    if (uri == NULL || scope_id == NULL || registration_id == NULL || dps_api_version == NULL || transport_io == NULL)
    {
        /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_001: [ If uri, scope_id, registration_id, dps_api_version, or transport_io is NULL, dps_transport_common_mqtt_create shall return NULL. ] */
        LogError("Invalid parameter specified uri: %p, scope_id: %p, registration_id: %p, dps_api_version: %p, transport_io: %p", uri, scope_id, registration_id, dps_api_version, transport_io);
        result = NULL;
    }
    else if (type == DPS_HSM_TYPE_TPM)
    {
        /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_062: [ If DPS_HSM_TYPE is DPS_HSM_TYPE_TPM dps_transport_common_mqtt_create shall return NULL (currently TPM is not supported). ] */
        LogError("HSM type of TPM is not supported");
        result = NULL;
    }
    else
    {
        /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_003: [ dps_transport_common_mqtt_create shall allocate a DPS_TRANSPORT_MQTT_INFO and initialize the containing fields. ] */
        result = malloc(sizeof(DPS_TRANSPORT_MQTT_INFO));
        if (result == NULL)
        {
            /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_002: [ If any error is encountered, dps_transport_common_mqtt_create shall return NULL. ] */
            LogError("Unable to allocate DPS_TRANSPORT_MQTT_INFO");
        }
        else
        {
            memset(result, 0, sizeof(DPS_TRANSPORT_MQTT_INFO));
            if (mallocAndStrcpy_s(&result->hostname, uri) != 0)
            {
                /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_002: [ If any error is encountered, dps_transport_common_mqtt_create shall return NULL. ] */
                LogError("Failure allocating hostname");
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->registration_id, registration_id) != 0)
            {
                /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_002: [ If any error is encountered, dps_transport_common_mqtt_create shall return NULL. ] */
                LogError("failure constructing registration Id");
                cleanup_dps_mqtt_data(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->api_version, dps_api_version) != 0)
            {
                /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_002: [ If any error is encountered, dps_transport_common_mqtt_create shall return NULL. ] */
                LogError("Failure allocating dps_api_version");
                cleanup_dps_mqtt_data(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->scope_id, scope_id) != 0)
            {
                /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_002: [ If any error is encountered, dps_transport_common_mqtt_create shall return NULL. ] */
                LogError("Failure allocating dps_api_version");
                cleanup_dps_mqtt_data(result);
                result = NULL;
            }
            else if ((result->mqtt_client = mqtt_client_init(mqtt_notification_callback, mqtt_operation_complete_callback, result, mqtt_error_callback, result)) == NULL)
            {
                /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_002: [ If any error is encountered, dps_transport_common_mqtt_create shall return NULL. ] */
                LogError("Failed initializing mqtt client.");
                cleanup_dps_mqtt_data(result);
                result = NULL;
            }
            else
            {
                result->transport_io_cb = transport_io;
                result->hsm_type = type;
            }
        }
    }
    /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_004: [ On success dps_transport_common_mqtt_create shall return a new instance of DPS_TRANSPORT_HANDLE. ] */
    return result;
}

void dps_transport_common_mqtt_destroy(DPS_TRANSPORT_HANDLE handle)
{
    /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_005: [ If handle is NULL, dps_transport_common_mqtt_destroy shall do nothing. ] */
    if (handle != NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_006: [ dps_transport_common_mqtt_destroy shall free all resources used in this module. ] */
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;
        mqtt_client_deinit(mqtt_info->mqtt_client);
        mqtt_info->mqtt_client = NULL;
        cleanup_dps_mqtt_data(mqtt_info);
    }
}

int dps_transport_common_mqtt_open(DPS_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
{
    int result;
    DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;
    if (mqtt_info == NULL || data_callback == NULL || status_cb == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_007: [ If handle, data_callback, or status_cb is NULL, dps_transport_common_mqtt_open shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, data_callback: %p, status_cb: %p", handle, data_callback, status_cb);
        result = __FAILURE__;
    }
    else if ( (mqtt_info->hsm_type == DPS_HSM_TYPE_TPM) && (ek == NULL || srk == NULL) )
    {
        /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_008: [ If hsm_type is DPS_HSM_TYPE_TPM and ek or srk is NULL, dps_transport_common_mqtt_open shall return a non-zero value. ] */
        LogError("Invalid parameter specified ek: %p, srk: %p", ek, srk);
        result = __FAILURE__;
    }
    /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_009: [ dps_transport_common_mqtt_open shall clone the ek and srk values.] */
    else if (ek != NULL && (mqtt_info->ek = BUFFER_clone(ek)) == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_041: [ If a failure is encountered, dps_transport_common_mqtt_open shall return a non-zero value. ] */
        LogError("Unable to allocate endorsement key");
        result = __FAILURE__;
    }
    else if (srk != NULL && (mqtt_info->srk = BUFFER_clone(srk)) == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_041: [ If a failure is encountered, dps_transport_common_mqtt_open shall return a non-zero value. ] */
        LogError("Unable to allocate storage root key");
        BUFFER_delete(mqtt_info->ek);
        mqtt_info->ek = NULL;
        result = __FAILURE__;
    }
    else
    {
        mqtt_info->register_data_cb = data_callback;
        mqtt_info->user_ctx = user_ctx;
        mqtt_info->status_cb = status_cb;
        mqtt_info->status_ctx = status_ctx;
        mqtt_info->mqtt_state = MQTT_STATE_DISCONNECTED;
        result = 0;
    }
    return result;
}

int dps_transport_common_mqtt_close(DPS_TRANSPORT_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_011: [ If handle is NULL, dps_transport_common_mqtt_close shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;
        BUFFER_delete(mqtt_info->ek);
        mqtt_info->ek = NULL;
        BUFFER_delete(mqtt_info->srk);
        mqtt_info->srk = NULL;

        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_012: [ dps_transport_common_mqtt_close shall close all connection associated with mqtt communication. ] */
        if (mqtt_client_disconnect(mqtt_info->mqtt_client) == 0)
        {
            mqtt_client_dowork(mqtt_info->mqtt_client);
        }
        xio_destroy(mqtt_info->transport_io);
        mqtt_info->transport_io = NULL;

        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_013: [ On success dps_transport_common_mqtt_close shall return a zero value. ] */
        mqtt_info->mqtt_state = MQTT_STATE_IDLE;
        result = 0;
    }
    return result;
}

int dps_transport_common_mqtt_register_device(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx, DPS_TRANSPORT_JSON_PARSE json_parse_cb, void* json_ctx)
{
    int result;
    DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;
    if (mqtt_info == NULL || json_parse_cb == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_014: [ If handle is NULL, dps_transport_common_mqtt_register_device shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, json_parse_cb: %p", handle, json_parse_cb);
        result = __FAILURE__;
    }
    /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_015: [ If hsm_type is of type DPS_HSM_TYPE_TPM and reg_challenge_cb is NULL, dps_transport_common_mqtt_register_device shall return a non-zero value. ] */
    else if (mqtt_info->hsm_type == DPS_HSM_TYPE_TPM && reg_challenge_cb == NULL)
    {
        LogError("Invalid parameter specified reg_challenge_cb: %p", reg_challenge_cb);
        result = __FAILURE__;
    }
    /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_061: [ If the transport_state is TRANSPORT_CLIENT_STATE_REG_SEND or the the operation_id is NULL, dps_transport_common_mqtt_register_device shall return a non-zero value. ] */
    else if (mqtt_info->transport_state == TRANSPORT_CLIENT_STATE_REG_SEND || mqtt_info->operation_id != NULL)
    {
        LogError("Failure: device is currently in the registration process");
        result = __FAILURE__;
    }
    else if (mqtt_info->transport_state == TRANSPORT_CLIENT_STATE_ERROR)
    {
        LogError("dps is in an error state, close the connection and try again.");
        result = __FAILURE__;
    }
    else
    {
        mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_REG_SEND;
        mqtt_info->challenge_cb = reg_challenge_cb;
        mqtt_info->challenge_ctx = user_ctx;
        mqtt_info->json_parse_cb = json_parse_cb;
        mqtt_info->json_ctx = json_ctx;

        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_017: [ On success dps_transport_common_mqtt_register_device shall return a zero value. ] */
        result = 0;
    }
    return result;
}

int dps_transport_common_mqtt_get_operation_status(DPS_TRANSPORT_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_018: [ If handle is NULL, dps_transport_common_mqtt_get_operation_status shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;
        if (mqtt_info->operation_id == NULL)
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_019: [ If the operation_id is NULL, dps_transport_common_mqtt_get_operation_status shall return a non-zero value. ] */
            LogError("operation_id was not previously set in the dps challenge method");
            result = __FAILURE__;
        }
        else if (mqtt_info->transport_state == TRANSPORT_CLIENT_STATE_ERROR)
        {
            LogError("dps is in an error state, close the connection and try again.");
            result = __FAILURE__;
        }
        else
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_021: [ dps_transport_common_mqtt_get_operation_status shall set the transport_state to TRANSPORT_CLIENT_STATE_STATUS_SEND. ] */
            mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_SEND;
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_022: [ On success dps_transport_common_mqtt_get_operation_status shall return a zero value. ] */
            result = 0;
        }
    }
    return result;
}

void dps_transport_common_mqtt_dowork(DPS_TRANSPORT_HANDLE handle)
{
    /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_046: [ If handle is NULL, dps_transport_common_mqtt_dowork shall do nothing. ] */
    if (handle != NULL)
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;
        if (mqtt_info->mqtt_state == MQTT_STATE_DISCONNECTED)
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_047: [ If the mqtt_state is MQTT_STATE_DISCONNECTED dps_transport_common_mqtt_dowork shall attempt to connect the mqtt connections. ] */
            if (create_connection(mqtt_info) != 0)
            {
                /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_049: [ If any error is encountered dps_transport_common_mqtt_dowork shall set the mqtt_state to MQTT_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                LogError("unable to create amqp connection");
                mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            }
            else
            {
                mqtt_info->mqtt_state = MQTT_STATE_CONNECTING;
            }
        }
        else if (mqtt_info->mqtt_state == MQTT_STATE_CONNECTED)
        {
            mqtt_client_dowork(mqtt_info->mqtt_client);
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_050: [ When the mqtt_state is MQTT_STATE_CONNECTED, dps_transport_common_mqtt_dowork shall subscribe to the topic $dps/registrations/res/# ] */
            if (subscribe_to_topic(mqtt_info) != 0)
            {
                /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_049: [ If any error is encountered dps_transport_common_mqtt_dowork shall set the mqtt_state to MQTT_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                LogError("Failure subscribing to topic");
                mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            }
            else
            {
                mqtt_info->status_cb(DPS_TRANSPORT_STATUS_CONNECTED, mqtt_info->status_ctx);
                mqtt_info->mqtt_state = MQTT_STATE_SUBSCRIBING;
            }
        }
        else if (mqtt_info->mqtt_state != MQTT_STATE_IDLE)
        {
            mqtt_client_dowork(mqtt_info->mqtt_client);
            if (mqtt_info->mqtt_state == MQTT_STATE_SUBSCRIBED || mqtt_info->mqtt_state == MQTT_STATE_ERROR)
            {
                switch (mqtt_info->transport_state)
                {
                    case TRANSPORT_CLIENT_STATE_REG_SEND:
                        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_053: [ When then transport_state is set to TRANSPORT_CLIENT_STATE_REG_SEND, dps_transport_common_mqtt_dowork shall send a REGISTER_ME message ] */
                        if (send_register_message(mqtt_info) != 0)
                        {
                            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_049: [ If any error is encountered dps_transport_common_mqtt_dowork shall set the mqtt_state to MQTT_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                            mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                            mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                        }
                        else
                        {
                            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_054: [ Upon successful sending of a TRANSPORT_CLIENT_STATE_REG_SEND message, dps_transport_common_mqtt_dowork shall set the transport_state to TRANSPORT_CLIENT_STATE_REG_SENT ] */
                            mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_REG_SENT;
                        }
                        break;
                    case TRANSPORT_CLIENT_STATE_STATUS_SEND:
                        /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_055: [ When then transport_state is set to TRANSPORT_CLIENT_STATE_STATUS_SEND, dps_transport_common_mqtt_dowork shall send a AMQP_OPERATION_STATUS message ] */
                        if (send_operation_status_message(mqtt_info) != 0)
                        {
                            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_049: [ If any error is encountered dps_transport_common_mqtt_dowork shall set the mqtt_state to MQTT_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                            mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                            mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                        }
                        else
                        {
                            /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_056: [ Upon successful sending of a AMQP_OPERATION_STATUS message, dps_transport_common_mqtt_dowork shall set the transport_state to TRANSPORT_CLIENT_STATE_STATUS_SENT ] */
                            mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_SENT;
                        }
                        break;

                    case TRANSPORT_CLIENT_STATE_REG_RECV:
                    case TRANSPORT_CLIENT_STATE_STATUS_RECV:
                    {
                        DPS_JSON_INFO* parse_info = mqtt_info->json_parse_cb(mqtt_info->payload_data, mqtt_info->json_ctx);
                        if (parse_info == NULL)
                        {
                            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_049: [ If any error is encountered dps_transport_common_mqtt_dowork shall set the mqtt_state to MQTT_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                            LogError("Unable to process registration reply.");
                            mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                            mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                        }
                        else
                        {
                            switch (parse_info->dps_status)
                            {
                                case DPS_TRANSPORT_STATUS_UNASSIGNED:
                                case DPS_TRANSPORT_STATUS_ASSIGNING:
                                    if (parse_info->operation_id == NULL)
                                    {
                                        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_049: [ If any error is encountered dps_transport_common_mqtt_dowork shall set the mqtt_state to MQTT_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                                        LogError("Failure operation Id invalid");
                                        mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                                        mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                                    }
                                    else if (mqtt_info->operation_id == NULL && mallocAndStrcpy_s(&mqtt_info->operation_id, parse_info->operation_id) != 0)
                                    {
                                        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_049: [ If any error is encountered dps_transport_common_mqtt_dowork shall set the mqtt_state to MQTT_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                                        LogError("Failure copying operation Id");
                                        mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                                        mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                                    }
                                    else
                                    {
                                        if (mqtt_info->status_cb != NULL)
                                        {
                                            mqtt_info->status_cb(parse_info->dps_status, mqtt_info->status_ctx);
                                        }
                                    }
                                    break;
                                case DPS_TRANSPORT_STATUS_ASSIGNED:
                                    mqtt_info->register_data_cb(DPS_TRANSPORT_RESULT_OK, parse_info->authorization_key, parse_info->iothub_uri, parse_info->device_id, mqtt_info->user_ctx);
                                    mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                                    break;
                                case DPS_TRANSPORT_STATUS_ERROR:
                                default:
                                    /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_049: [ If any error is encountered dps_transport_common_mqtt_dowork shall set the mqtt_state to MQTT_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                                    LogError("Unable to process message reply.");
                                    mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                                    mqtt_info->mqtt_state = MQTT_STATE_ERROR;
                                    break;

                            }
                            free_json_parse_info(parse_info);
                        }
                        mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                        break;
                    }

                    case TRANSPORT_CLIENT_STATE_ERROR:
                        /* Codes_DPS_TRANSPORT_MQTT_COMMON_07_057: [ If transport_state is set to TRANSPORT_CLIENT_STATE_ERROR, dps_transport_common_mqtt_dowork shall call the register_data_cb function with DPS_TRANSPORT_RESULT_ERROR setting the transport_state to TRANSPORT_CLIENT_STATE_IDLE ] */
                        mqtt_info->register_data_cb(DPS_TRANSPORT_RESULT_ERROR, NULL, NULL, NULL, mqtt_info->user_ctx);
                        mqtt_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                        mqtt_info->mqtt_state = MQTT_STATE_IDLE;
                        break;
                    case TRANSPORT_CLIENT_STATE_REG_SENT:
                    case TRANSPORT_CLIENT_STATE_STATUS_SENT:
                        break;

                    case TRANSPORT_CLIENT_STATE_IDLE:
                    default:
                        break;
                }
            }
        }
    }
}

int dps_transport_common_mqtt_set_trace(DPS_TRANSPORT_HANDLE handle, bool trace_on)
{
    int result;
    if (handle == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_023: [ If handle is NULL, dps_transport_common_mqtt_set_trace shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_024: [ dps_transport_common_mqtt_set_trace shall set the log_trace variable to trace_on. ]*/
        mqtt_info->log_trace = trace_on;
        if (mqtt_info->mqtt_client != NULL)
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_059: [ If the umqtt connection is not NULL, dps_transport_common_mqtt_set_trace shall set the trace option on that connection. ] */
            mqtt_client_set_trace(mqtt_info->mqtt_client, mqtt_info->log_trace, false);
        }
        result = 0;
    }
    return result;
}

int dps_transport_common_mqtt_x509_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
{
    int result;
    if (handle == NULL || certificate == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_026: [ If handle or certificate is NULL, dps_transport_common_mqtt_x509_cert shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, certificate: %p", handle, certificate);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;

        if (mqtt_info->x509_cert != NULL)
        {
            free(mqtt_info->x509_cert);
            mqtt_info->x509_cert = NULL;
        }
        if (mqtt_info->private_key != NULL)
        {
            free(mqtt_info->private_key);
            mqtt_info->private_key = NULL;
        }

        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_027: [ dps_transport_common_mqtt_x509_cert shall copy the certificate and private_key values. ] */
        if (mallocAndStrcpy_s(&mqtt_info->x509_cert, certificate) != 0)
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_028: [ On any failure dps_transport_common_mqtt_x509_cert, shall return a non-zero value. ] */
            result = __FAILURE__;
            LogError("failure allocating certificate");
        }
        else if (mallocAndStrcpy_s(&mqtt_info->private_key, private_key) != 0)
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_028: [ On any failure dps_transport_common_mqtt_x509_cert, shall return a non-zero value. ] */
            LogError("failure allocating certificate");
            free(mqtt_info->x509_cert);
            mqtt_info->x509_cert = NULL;
            result = __FAILURE__;
        }
        else
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_029: [ On success dps_transport_common_mqtt_x509_cert shall return a zero value. ] */
            result = 0;
        }
    }
    return result;
}

int dps_transport_common_mqtt_set_trusted_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate)
{
    int result;
    if (handle == NULL || certificate == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_030: [ If handle or certificate is NULL, dps_transport_common_mqtt_set_trusted_cert shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, certificate: %p", handle, certificate);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;

        if (mqtt_info->certificate != NULL)
        {
            free(mqtt_info->certificate);
            mqtt_info->certificate = NULL;
        }

        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_031: [ dps_transport_common_mqtt_set_trusted_cert shall copy the certificate value. ] */
        if (mallocAndStrcpy_s(&mqtt_info->certificate, certificate) != 0)
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_032: [ On any failure dps_transport_common_mqtt_set_trusted_cert, shall return a non-zero value. ] */
            result = __FAILURE__;
            LogError("failure allocating certificate");
        }
        else
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_033: [ On success dps_transport_common_mqtt_set_trusted_cert shall return a zero value. ] */
            result = 0;
        }
    }
    return result;
}

int dps_transport_common_mqtt_set_proxy(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
{
    int result;
    if (handle == NULL || proxy_options == NULL)
    {
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_034: [ If handle or proxy_options is NULL, dps_transport_common_mqtt_set_proxy shall return a non-zero value. ]*/
        LogError("Invalid parameter specified handle: %p, proxy_options: %p", handle, proxy_options);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_MQTT_INFO* mqtt_info = (DPS_TRANSPORT_MQTT_INFO*)handle;
        if (proxy_options->host_address == NULL)
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_035: [ If HTTP_PROXY_OPTIONS host_address is NULL, dps_transport_common_mqtt_set_proxy shall return a non-zero value. ] */
            LogError("NULL host_address in proxy options");
            result = __FAILURE__;
        }
        /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_036: [ If HTTP_PROXY_OPTIONS password is not NULL and username is NULL, dps_transport_common_mqtt_set_proxy shall return a non-zero value. ] */
        else if (((proxy_options->username == NULL) || (proxy_options->password == NULL)) &&
            (proxy_options->username != proxy_options->password))
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_039: [ On any failure dps_transport_common_mqtt_set_proxy, shall return a non-zero value. ] */
            LogError("Only one of username and password for proxy settings was NULL");
            result = __FAILURE__;
        }
        else
        {
            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_037: [ If any of the host_addess, username, or password variables are non-NULL, dps_transport_common_mqtt_set_proxy shall free the memory. ] */
            if (mqtt_info->proxy_option.host_address != NULL)
            {
                free((char*)mqtt_info->proxy_option.host_address);
                mqtt_info->proxy_option.host_address = NULL;
            }
            if (mqtt_info->proxy_option.username != NULL)
            {
                free((char*)mqtt_info->proxy_option.username);
                mqtt_info->proxy_option.username = NULL;
            }
            if (mqtt_info->proxy_option.password != NULL)
            {
                free((char*)mqtt_info->proxy_option.password);
                mqtt_info->proxy_option.password = NULL;
            }

            /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_038: [ dps_transport_common_mqtt_set_proxy shall copy the host_addess, username, or password variables ] */
            mqtt_info->proxy_option.port = proxy_options->port;
            if (mallocAndStrcpy_s((char**)&mqtt_info->proxy_option.host_address, proxy_options->host_address) != 0)
            {
                /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_039: [ On any failure dps_transport_common_mqtt_set_proxy, shall return a non-zero value. ] */
                LogError("Failure setting proxy_host name");
                result = __FAILURE__;
            }
            else if (proxy_options->username != NULL && mallocAndStrcpy_s((char**)&mqtt_info->proxy_option.username, proxy_options->username) != 0)
            {
                /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_039: [ On any failure dps_transport_common_mqtt_set_proxy, shall return a non-zero value. ] */
                LogError("Failure setting proxy username");
                free((char*)mqtt_info->proxy_option.host_address);
                mqtt_info->proxy_option.host_address = NULL;
                result = __FAILURE__;
            }
            else if (proxy_options->password != NULL && mallocAndStrcpy_s((char**)&mqtt_info->proxy_option.password, proxy_options->password) != 0)
            {
                /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_039: [ On any failure dps_transport_common_mqtt_set_proxy, shall return a non-zero value. ] */
                LogError("Failure setting proxy password");
                free((char*)mqtt_info->proxy_option.host_address);
                mqtt_info->proxy_option.host_address = NULL;
                free((char*)mqtt_info->proxy_option.username);
                mqtt_info->proxy_option.username = NULL;
                result = __FAILURE__;
            }
            else
            {
                /* Tests_DPS_TRANSPORT_MQTT_COMMON_07_040: [ On success dps_transport_common_mqtt_set_proxy shall return a zero value. ] */
                result = 0;
            }
        }
    }
    return result;
}
