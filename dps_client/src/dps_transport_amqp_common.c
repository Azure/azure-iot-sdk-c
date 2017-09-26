// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_hub_modules/dps_transport_amqp_common.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_hub_modules/dps_sasl_tpm.h"
#include "azure_c_shared_utility/strings.h"

#define AMQP_MAX_SENDER_MSG_SIZE    UINT64_MAX
#define AMQP_MAX_RECV_MSG_SIZE      65536

static const char* HEADER_KEY_AUTHORIZATION = "Authorization";
static const char* SAS_TOKEN_KEY_NAME = "registration";
static const char* AMQP_ADDRESS_FMT = "amqps://%s/%s/registrations/%s";
static const char* KEY_NAME_VALUE = "registration";

static const char* AMQP_REGISTER_ME = "iotdps-register-me";
static const char* AMQP_WHO_AM_I = "iotdps-who-am-i";
static const char* AMQP_OPERATION_STATUS = "iotdps-operation-status";

static const char* AMQP_OP_TYPE_PROPERTY = "iotdps-operation-type";
static const char* AMQP_STATUS = "iotdps-status";
static const char* AMQP_OPERATION_ID = "iotdps-operation-id";
static const char* AMQP_IOTHUB_URI = "iotdps-assigned-hub";
static const char* AMQP_DEVICE_ID = "iotdps-device-id";
static const char* AMQP_AUTH_KEY = "iotdps-auth-key";

static const char* DPS_ASSIGNED_STATUS = "Assigned";
static const char* DPS_ASSIGNING_STATUS = "Assigning";
static const char* DPS_UNASSIGNED_STATUS = "Unassigned";

typedef enum AMQP_TRANSPORT_STATE_TAG
{
    AMQP_STATE_IDLE,
    AMQP_STATE_DISCONNECTED,
    AMQP_STATE_CONNECTING,
    AMQP_STATE_CONNECTED,
    AMQP_STATE_ERROR
} AMQP_TRANSPORT_STATE;

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

typedef struct DPS_TRANSPORT_AMQP_INFO_TAG
{
    DPS_TRANSPORT_REGISTER_DATA_CALLBACK register_data_cb;
    void* user_ctx;
    DPS_TRANSPORT_STATUS_CALLBACK status_cb;
    void* status_ctx;
    DPS_TRANSPORT_CHALLENGE_CALLBACK challenge_cb;
    void* challenge_ctx;
    DPS_TRANSPORT_JSON_PARSE json_parse_cb;
    void* json_ctx;

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

    DPS_HSM_TYPE hsm_type;

    DPS_AMQP_TRANSPORT_IO transport_io_cb;

    DPS_TRANSPORT_STATE transport_state;
    AMQP_TRANSPORT_STATE amqp_state;

    MESSAGE_SENDER_STATE msg_recv_state;
    MESSAGE_SENDER_STATE msg_send_state;

    SASL_MECHANISM_HANDLE tpm_sasl_handler;
    SASLCLIENTIO_CONFIG sasl_io_config;
    CONNECTION_HANDLE connection;
    XIO_HANDLE transport_io;
    XIO_HANDLE underlying_io;
    SESSION_HANDLE session;
    LINK_HANDLE sender_link;
    LINK_HANDLE receiver_link;
    MESSAGE_SENDER_HANDLE msg_sender;
    MESSAGE_RECEIVER_HANDLE msg_receiver;
} DPS_TRANSPORT_AMQP_INFO;

static char* on_sasl_tpm_challenge_cb(BUFFER_HANDLE data_handle, void* user_ctx)
{
    char* result;
    DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)user_ctx;
    if (amqp_info == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_042: [ If user_ctx is NULL, on_sasl_tpm_challenge_cb shall return NULL. ] */
        LogError("Bad argument user_ctx NULL");
        result = NULL;
    }
    else if (amqp_info->challenge_cb == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_043: [ If the challenge_cb function is NULL, on_sasl_tpm_challenge_cb shall return NULL. ] */
        LogError("challenge callback is NULL, device needs to register");
        result = NULL;
    }
    else
    {
        const unsigned char* data = BUFFER_u_char(data_handle);
        size_t length = BUFFER_length(data_handle);

        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_045: [ on_sasl_tpm_challenge_cb shall call the challenge_cb returning the resulting value. ] */
        result = amqp_info->challenge_cb(data, length, KEY_NAME_VALUE, amqp_info->challenge_ctx);
        if (result == NULL)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_044: [ If any failure is encountered on_sasl_tpm_challenge_cb shall return NULL. ] */
            LogError("Failure challenge_cb");
            amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            amqp_info->amqp_state = AMQP_STATE_ERROR;
        }
    }
    return result;
}

static void on_message_sender_state_changed_callback(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
{
    if (context == NULL)
    {
        LogError("on_message_sender_state_changed_callback was invoked with a NULL context");
    }
    else
    {
        if (new_state != previous_state)
        {
            DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)context;
            amqp_info->msg_send_state = new_state;
            switch (amqp_info->msg_send_state)
            {
                case MESSAGE_SENDER_STATE_IDLE:
                case MESSAGE_SENDER_STATE_OPENING:
                case MESSAGE_SENDER_STATE_CLOSING:
                    break;
                case MESSAGE_SENDER_STATE_OPEN:
                    if (amqp_info->msg_recv_state == MESSAGE_RECEIVER_STATE_OPEN)
                    {
                        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_052: [ Once the uamqp reciever and sender link are connected the amqp_state shall be set to AMQP_STATE_CONNECTED ] */
                        amqp_info->amqp_state = AMQP_STATE_CONNECTED;
                        if (amqp_info->status_cb != NULL)
                        {
                            amqp_info->status_cb(DPS_TRANSPORT_STATUS_CONNECTED, amqp_info->status_ctx);
                        }
                    }
                    break;
                case MESSAGE_SENDER_STATE_ERROR:
                    amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                    amqp_info->amqp_state = AMQP_STATE_ERROR;
                    break;
            }
        }
    }
}

static void on_message_receiver_state_changed_callback(const void* user_ctx, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
{
    if (user_ctx == NULL)
    {
        LogError("on_message_receiver_state_changed_callback was invoked with a NULL context");
    }
    else
    {
        if (new_state != previous_state)
        {
            DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)user_ctx;
            amqp_info->msg_recv_state = new_state;
            switch (amqp_info->msg_recv_state)
            {
            case MESSAGE_RECEIVER_STATE_IDLE:
            case MESSAGE_RECEIVER_STATE_OPENING:
            case MESSAGE_RECEIVER_STATE_CLOSING:
                break;
            case MESSAGE_RECEIVER_STATE_OPEN:
                if (amqp_info->msg_send_state == MESSAGE_SENDER_STATE_OPEN)
                {
                    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_052: [ Once the uamqp reciever and sender link are connected the amqp_state shall be set to AMQP_STATE_CONNECTED ] */
                    amqp_info->amqp_state = AMQP_STATE_CONNECTED;
                    if (amqp_info->status_cb != NULL)
                    {
                        amqp_info->status_cb(DPS_TRANSPORT_STATUS_CONNECTED, amqp_info->status_ctx);
                    }
                }
                break;
            case MESSAGE_RECEIVER_STATE_ERROR:
                amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                amqp_info->amqp_state = AMQP_STATE_ERROR;
                break;
            }
        }
    }
}

static void on_amqp_send_complete(void* user_ctx, MESSAGE_SEND_RESULT send_result)
{
    if (user_ctx == NULL)
    {
        LogError("on_amqp_send_complete was invoked with a NULL context");
    }
    else
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)user_ctx;
        if (send_result != MESSAGE_SEND_OK)
        {
            amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            amqp_info->amqp_state = AMQP_STATE_ERROR;
        }
    }
}

static STRING_HANDLE construct_link_address(DPS_TRANSPORT_AMQP_INFO* amqp_info)
{
    STRING_HANDLE result;
    result = STRING_construct_sprintf(AMQP_ADDRESS_FMT, amqp_info->hostname, amqp_info->scope_id, amqp_info->registration_id);
    if (result == NULL)
    {
        LogError("Failure constructing event address");
    }
    return result;
}

static AMQP_VALUE on_message_recv_callback(const void* user_ctx, MESSAGE_HANDLE message)
{
    AMQP_VALUE result;
    if (user_ctx == NULL)
    {
        LogError("on_message_recv_callback was invoked with a NULL context");
        result = messaging_delivery_rejected("SDK error", "user context NULL");
    }
    else
    {
        MESSAGE_BODY_TYPE body_type;
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)user_ctx;

        if (message_get_body_type(message, &body_type) != 0)
        {
            LogError("message body type has failed");
            amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            amqp_info->amqp_state = AMQP_STATE_ERROR;
        }
        else if (body_type != MESSAGE_BODY_TYPE_DATA)
        {
            LogError("invalid message type of %d", body_type);
            amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            amqp_info->amqp_state = AMQP_STATE_ERROR;
        }
        else
        {
            BINARY_DATA binary_data;
            if (amqp_info->payload_data != NULL)
            {
                free(amqp_info->payload_data);
                amqp_info->payload_data = NULL;
            }

            if (message_get_body_amqp_data_in_place(message, 0, &binary_data) != 0)
            {
                LogError("failure getting message from amqp body");
                amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                amqp_info->amqp_state = AMQP_STATE_ERROR;
            }
            else if ((amqp_info->payload_data = malloc(binary_data.length + 1)) == NULL)
            {
                LogError("failure allocating payload data");
                amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                amqp_info->amqp_state = AMQP_STATE_ERROR;
            }
            else
            {
                memset(amqp_info->payload_data, 0, binary_data.length + 1);
                memcpy(amqp_info->payload_data, binary_data.bytes, binary_data.length);
                if (amqp_info->transport_state == TRANSPORT_CLIENT_STATE_REG_SENT)
                {
                    amqp_info->transport_state = TRANSPORT_CLIENT_STATE_REG_RECV;
                }
                else
                {
                    amqp_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_RECV;
                }
            }
        }
        result = messaging_delivery_accepted();
    }
    return result;
}

static int set_amqp_msg_property(AMQP_VALUE uamqp_prop_map, const char* key, const char* value)
{
    int result;
    AMQP_VALUE uamqp_key;
    AMQP_VALUE uamqp_value;
    if ((uamqp_key = amqpvalue_create_string(key)) == NULL)
    {
        LogError("Failed to create property key string.");
        result = __FAILURE__;
    }
    else if ((uamqp_value = amqpvalue_create_string(value)) == NULL)
    {
        amqpvalue_destroy(uamqp_key);
        LogError("Failed to create property value string.");
        result = __FAILURE__;
    }
    else
    {
        if (amqpvalue_set_map_value(uamqp_prop_map, uamqp_key, uamqp_value) != 0)
        {
            LogError("Failed to set map value.");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
        amqpvalue_destroy(uamqp_key);
        amqpvalue_destroy(uamqp_value);
    }
    return result;
}

static int send_amqp_message(DPS_TRANSPORT_AMQP_INFO* amqp_info, const char* msg_type)
{
    int result;
    MESSAGE_HANDLE uamqp_message;
    if ((uamqp_message = message_create()) == NULL)
    {
        LogError("Failed allocating the uAMQP message.");
        result = __FAILURE__;
    }
    else
    {
        AMQP_VALUE uamqp_prop_map;

        BINARY_DATA binary_data;
        binary_data.bytes = NULL;
        binary_data.length = 0;

        if ((uamqp_prop_map = amqpvalue_create_map()) == NULL)
        {
            LogError("Failed to create uAMQP map for the properties.");
            result = __FAILURE__;
        }
        else if (message_add_body_amqp_data(uamqp_message, binary_data) != 0)
        {
            LogError("Failed to adding amqp msg body.");
            amqpvalue_destroy(uamqp_prop_map);
            result = __FAILURE__;
        }
        else if (set_amqp_msg_property(uamqp_prop_map, AMQP_OP_TYPE_PROPERTY, msg_type) != 0)
        {
            LogError("Failed to adding amqp type property to message.");
            result = __FAILURE__;
            message_destroy(uamqp_message);
            amqpvalue_destroy(uamqp_prop_map);
        }
        else
        {
            if (amqp_info->operation_id != NULL)
            {
                if (set_amqp_msg_property(uamqp_prop_map, AMQP_OPERATION_ID, amqp_info->operation_id) != 0)
                {
                    LogError("Failed to adding operation status property to message.");
                    result = __FAILURE__;
                    message_destroy(uamqp_message);
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
                if (message_set_application_properties(uamqp_message, uamqp_prop_map) != 0)
                {
                    LogError("Failed to set map value.");
                    result = __FAILURE__;
                }
                else if (messagesender_send(amqp_info->msg_sender, uamqp_message, on_amqp_send_complete, amqp_info) != 0)
                {
                    LogError("Failed to send message.");
                    result = __FAILURE__;
                }
                else
                {
                    result = 0;
                }
            }
        }
        amqpvalue_destroy(uamqp_prop_map);
        message_destroy(uamqp_message);
    }
    return result;
}

static int create_sender_link(DPS_TRANSPORT_AMQP_INFO* amqp_info)
{
    int result;
    AMQP_VALUE msg_source;
    AMQP_VALUE msg_target;
    STRING_HANDLE event_address;

    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_050: [ The reciever and sender endpoints addresses shall be constructed in the following manner: amqps://[hostname]/[scope_id]/registrations/[registration_id] ] */
    if ((event_address = construct_link_address(amqp_info)) == NULL)
    {
        LogError("Failure constructing amqp link address");
        result = __FAILURE__;
    }
    else
    {
        if ((msg_source = messaging_create_source("remote_endpoint")) == NULL)
        {
            LogError("Failure retrieving messaging create source");
            result = __FAILURE__;
        }
        else if ((msg_target = messaging_create_target(STRING_c_str(event_address))) == NULL)
        {
            LogError("Failure creating messaging target");
            amqpvalue_destroy(msg_source);
            result = __FAILURE__;
        }
        else if ((amqp_info->sender_link = link_create(amqp_info->session, "sender_link", role_sender, msg_source, msg_target)) == NULL)
        {
            LogError("Failure creating link");
            amqpvalue_destroy(msg_source);
            amqpvalue_destroy(msg_target);
            result = __FAILURE__;
        }
        else
        {
            if (link_set_max_message_size(amqp_info->sender_link, AMQP_MAX_SENDER_MSG_SIZE) != 0)
            {
                LogError("Failure setting sender link max size");
                link_destroy(amqp_info->sender_link);
                amqp_info->sender_link = NULL;
                result = __FAILURE__;
            }
            else if ((amqp_info->msg_sender = messagesender_create(amqp_info->sender_link, on_message_sender_state_changed_callback, amqp_info)) == NULL)
            {
                LogError("Failure creating message sender");
                link_destroy(amqp_info->sender_link);
                amqp_info->sender_link = NULL;
                result = __FAILURE__;
            }
            else if (messagesender_open(amqp_info->msg_sender) != 0)
            {
                LogError("Failure opening message sender");
                messagesender_destroy(amqp_info->msg_sender);
                amqp_info->msg_sender = NULL;
                link_destroy(amqp_info->sender_link);
                amqp_info->sender_link = NULL;
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
            amqpvalue_destroy(msg_target);
            amqpvalue_destroy(msg_source);
        }
        STRING_delete(event_address);
    }
    return result;
}

static int create_receiver_link(DPS_TRANSPORT_AMQP_INFO* amqp_info)
{
    int result;
    AMQP_VALUE msg_source;
    AMQP_VALUE msg_target;
    STRING_HANDLE event_address;

    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_050: [ The reciever and sender endpoints addresses shall be constructed in the following manner: amqps://[hostname]/[scope_id]/registrations/[registration_id] ] */
    if ((event_address = construct_link_address(amqp_info)) == NULL)
    {
        LogError("Failure constructing amqp link address");
        result = __FAILURE__;
    }
    else
    {
        if ((msg_source = messaging_create_source(STRING_c_str(event_address))) == NULL)
        {
            LogError("Failure retrieving messaging create source");
            result = __FAILURE__;
        }
        else if ((msg_target = messaging_create_target("local_endpoint")) == NULL)
        {
            LogError("Failure creating messaging target");
            amqpvalue_destroy(msg_source);
            result = __FAILURE__;
        }
        else if ((amqp_info->receiver_link = link_create(amqp_info->session, "recv_link", role_receiver, msg_source, msg_target)) == NULL)
        {
            LogError("Failure creating link");
            amqpvalue_destroy(msg_source);
            amqpvalue_destroy(msg_target);
            result = __FAILURE__;
        }
        else
        {
            link_set_rcv_settle_mode(amqp_info->receiver_link, receiver_settle_mode_first);
            if (link_set_max_message_size(amqp_info->receiver_link, AMQP_MAX_RECV_MSG_SIZE) != 0)
            {
                LogError("Failure setting max message size");
                link_destroy(amqp_info->receiver_link);
                amqp_info->receiver_link = NULL;
                result = __FAILURE__;
            }
            else if ((amqp_info->msg_receiver = messagereceiver_create(amqp_info->receiver_link, on_message_receiver_state_changed_callback, (void*)amqp_info)) == NULL)
            {
                LogError("Failure creating message receiver");
                link_destroy(amqp_info->receiver_link);
                amqp_info->receiver_link = NULL;
                result = __FAILURE__;
            }
            else if (messagereceiver_open(amqp_info->msg_receiver, on_message_recv_callback, (void*)amqp_info) != 0)
            {
                LogError("Failure opening message receiver");
                link_destroy(amqp_info->receiver_link);
                amqp_info->receiver_link = NULL;
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }

            amqpvalue_destroy(msg_source);
            amqpvalue_destroy(msg_target);
        }
        STRING_delete(event_address);
    }
    return result;
}

static int create_amqp_connection(DPS_TRANSPORT_AMQP_INFO* amqp_info)
{
    int result;
    HTTP_PROXY_OPTIONS* transport_proxy;
    DPS_TRANSPORT_IO_INFO* transport_io;
    SASL_MECHANISM_HANDLE* sasl_mechanism = NULL;

    if (amqp_info->proxy_option.host_address != NULL)
    {
        transport_proxy = &amqp_info->proxy_option;
    }
    else
    {
        transport_proxy = NULL;
    }

    if (amqp_info->hsm_type == DPS_HSM_TYPE_TPM)
    {
        sasl_mechanism = &amqp_info->tpm_sasl_handler;
    }

    transport_io = amqp_info->transport_io_cb(amqp_info->hostname, sasl_mechanism, transport_proxy);
    if (transport_io == NULL)
    {
        LogError("Failure calling transport_io callback");
        result = __FAILURE__;
    }
    else
    {
        if (transport_io->sasl_handle == NULL)
        {
            amqp_info->underlying_io = transport_io->transport_handle;
        }
        else
        {
            amqp_info->transport_io = transport_io->transport_handle;
            amqp_info->underlying_io = transport_io->sasl_handle;
        }

        bool ignore_check = true;
        (void)xio_setoption(amqp_info->underlying_io, "ignore_server_name_check", &ignore_check);

        if (amqp_info->hsm_type == DPS_HSM_TYPE_X509)
        {
            if (amqp_info->x509_cert != NULL && amqp_info->private_key != NULL)
            {
                if (xio_setoption(amqp_info->underlying_io, OPTION_X509_ECC_CERT, amqp_info->x509_cert) != 0)
                {
                    LogError("Failure setting x509 cert on xio");
                    result = __FAILURE__;
                    xio_destroy(amqp_info->underlying_io);
                }
                else if (xio_setoption(amqp_info->underlying_io, OPTION_X509_ECC_KEY, amqp_info->private_key) != 0)
                {
                    LogError("Failure setting x509 key on xio");
                    result = __FAILURE__;
                    xio_destroy(amqp_info->underlying_io);
                }
                else
                {
                    result = 0;
                }
            }
            else
            {
                LogError("x509 certificate is NULL");
                result = __FAILURE__;
                xio_destroy(amqp_info->underlying_io);
            }
        }
        else
        {
            result = 0;
        }

        if (result == 0)
        {
            if (amqp_info->certificate != NULL && xio_setoption(amqp_info->underlying_io, "TrustedCerts", amqp_info->certificate) != 0)
            {
                LogError("Failure setting trusted certs");
                result = __FAILURE__;
                xio_destroy(amqp_info->underlying_io);
                amqp_info->underlying_io = NULL;
            }
            else if ((amqp_info->connection = connection_create(amqp_info->underlying_io, amqp_info->hostname, "dps_connection", NULL, NULL)) == NULL)
            {
                LogError("failed creating amqp connection");
                xio_destroy(amqp_info->underlying_io);
                amqp_info->underlying_io = NULL;
                result = __FAILURE__;
            }
            else
            {
                connection_set_trace(amqp_info->connection, amqp_info->log_trace);
                result = 0;
            }
        }
        free(transport_io); 
    }
    return result;
}

static int create_connection(DPS_TRANSPORT_AMQP_INFO* amqp_info)
{
    int result;
    const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_interface;
    SASL_TPM_CONFIG_INFO sasl_config;
    sasl_config.registration_id = amqp_info->registration_id;
    sasl_config.storage_root_key = amqp_info->srk;
    sasl_config.endorsement_key = amqp_info->ek;
    sasl_config.dps_hostname = amqp_info->hostname;
    sasl_config.challenge_cb = on_sasl_tpm_challenge_cb;
    sasl_config.user_ctx = amqp_info;

    // Create TPM SASL handler
    if ((sasl_interface = dps_sasltpm_get_interface()) == NULL)
    {
        LogError("dps_sasltpm_get_interface was NULL");
        result = __FAILURE__;
    }
    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_048: [ If the hsm_type is DPS_HSM_TYPE_TPM create_connection shall create a tpm_saslmechanism. ] */
    else if (amqp_info->hsm_type == DPS_HSM_TYPE_TPM && (amqp_info->tpm_sasl_handler = saslmechanism_create(sasl_interface, &sasl_config)) == NULL)
    {
        LogError("failed creating tpm sasl mechanism");
        result = __FAILURE__;
    }
    else if (create_amqp_connection(amqp_info) != 0)
    {
        LogError("failed creating amqp connection");
        saslmechanism_destroy(amqp_info->tpm_sasl_handler);
        amqp_info->tpm_sasl_handler = NULL;
        result = __FAILURE__;
    }
    else
    {
        if ((amqp_info->session = session_create(amqp_info->connection, NULL, NULL)) == NULL)
        {
            LogError("failed creating amqp session");
            saslmechanism_destroy(amqp_info->tpm_sasl_handler);
            amqp_info->tpm_sasl_handler = NULL;
            result = __FAILURE__;
        }
        else
        {
            (void)session_set_incoming_window(amqp_info->session, 2147483647);
            (void)session_set_outgoing_window(amqp_info->session, 65536);

            if (create_receiver_link(amqp_info) != 0)
            {
                LogError("failure creating amqp receiver link");
                saslmechanism_destroy(amqp_info->tpm_sasl_handler);
                amqp_info->tpm_sasl_handler = NULL;
                session_destroy(amqp_info->session);
                amqp_info->session = NULL;
                result = __FAILURE__;
            }
            else if (create_sender_link(amqp_info) != 0)
            {
                LogError("failure creating amqp sender link");
                saslmechanism_destroy(amqp_info->tpm_sasl_handler);
                amqp_info->tpm_sasl_handler = NULL;
                session_destroy(amqp_info->session);
                amqp_info->session = NULL;
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
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

void cleanup_dps_amqp_data(DPS_TRANSPORT_AMQP_INFO* amqp_info)
{
    free(amqp_info->hostname);
    free(amqp_info->registration_id);
    free(amqp_info->operation_id);
    free(amqp_info->api_version);
    free(amqp_info->scope_id);
    free(amqp_info->certificate);
    free((char*)amqp_info->proxy_option.host_address);
    free((char*)amqp_info->proxy_option.username);
    free((char*)amqp_info->proxy_option.password);
    free(amqp_info->x509_cert);
    free(amqp_info->private_key);
    free(amqp_info->sas_token);
    free(amqp_info->payload_data);
    free(amqp_info);
}

DPS_TRANSPORT_HANDLE dps_transport_common_amqp_create(const char* uri, DPS_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* dps_api_version, DPS_AMQP_TRANSPORT_IO transport_io)
{
    DPS_TRANSPORT_AMQP_INFO* result;
    if (uri == NULL || scope_id == NULL || registration_id == NULL || dps_api_version == NULL || transport_io == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_001: [ If uri, scope_id, registration_id, dps_api_version, or transport_io is NULL, dps_transport_common_amqp_create shall return NULL. ] */
        LogError("Invalid parameter specified uri: %p, scope_id: %p, registration_id: %p, dps_api_version: %p, transport_io: %p", uri, scope_id, registration_id, dps_api_version, transport_io);
        result = NULL;
    }
    else
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_003: [ dps_transport_common_amqp_create shall allocate a DPS_TRANSPORT_AMQP_INFO structure ] */
        result = malloc(sizeof(DPS_TRANSPORT_AMQP_INFO));
        if (result == NULL)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_002: [ If any error is encountered, dps_transport_common_amqp_create shall return NULL. ] */
            LogError("Unable to allocate DPS_TRANSPORT_AMQP_INFO");
        }
        else
        {
            memset(result, 0, sizeof(DPS_TRANSPORT_AMQP_INFO));
            if (mallocAndStrcpy_s(&result->hostname, uri) != 0)
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_002: [ If any error is encountered, dps_transport_common_amqp_create shall return NULL. ] */
                LogError("Failure allocating hostname");
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->registration_id, registration_id) != 0)
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_002: [ If any error is encountered, dps_transport_common_amqp_create shall return NULL. ] */
                LogError("failure constructing registration Id");
                cleanup_dps_amqp_data(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->api_version, dps_api_version) != 0)
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_002: [ If any error is encountered, dps_transport_common_amqp_create shall return NULL. ] */
                LogError("Failure allocating dps_api_version");
                cleanup_dps_amqp_data(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->scope_id, scope_id) != 0)
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_002: [ If any error is encountered, dps_transport_common_amqp_create shall return NULL. ] */
                LogError("Failure allocating dps_api_version");
                cleanup_dps_amqp_data(result);
                result = NULL;
            }
            else
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_004: [ On success dps_transport_common_amqp_create shall allocate a new instance of DPS_TRANSPORT_HANDLE. ] */
                result->transport_io_cb = transport_io;
                result->hsm_type = type;
            }
        }
    }
    return result;
}

void dps_transport_common_amqp_destroy(DPS_TRANSPORT_HANDLE handle)
{
    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_005: [ If handle is NULL, dps_transport_common_amqp_destroy shall do nothing. ] */
    if (handle != NULL)
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_006: [ dps_transport_common_amqp_destroy shall free all resources used in this module. ] */
        cleanup_dps_amqp_data(amqp_info);
    }
}

int dps_transport_common_amqp_open(DPS_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
{
    int result;
    DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;
    if (amqp_info == NULL || data_callback == NULL || status_cb == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_007: [ If handle, data_callback, or status_cb is NULL, dps_transport_common_amqp_open shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, data_callback: %p, status_cb: %p", handle, data_callback, status_cb);
        result = __FAILURE__;
    }
    else if ( (amqp_info->hsm_type == DPS_HSM_TYPE_TPM) && (ek == NULL || srk == NULL) )
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_008: [ If hsm_type is DPS_HSM_TYPE_TPM and ek or srk is NULL, dps_transport_common_amqp_open shall return a non-zero value. ] */
        LogError("Invalid parameter specified ek: %p, srk: %p", ek, srk);
        result = __FAILURE__;
    }
    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_009: [ dps_transport_common_amqp_open shall clone the ek and srk values.] */
    else if (ek != NULL && (amqp_info->ek = BUFFER_clone(ek)) == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_041: [ If a failure is encountered, dps_transport_common_amqp_open shall return a non-zero value. ] */
        LogError("Unable to allocate endorsement key");
        result = __FAILURE__;
    }
    else if (srk != NULL && (amqp_info->srk = BUFFER_clone(srk)) == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_041: [ If a failure is encountered, dps_transport_common_amqp_open shall return a non-zero value. ] */
        LogError("Unable to allocate storage root key");
        BUFFER_delete(amqp_info->ek);
        amqp_info->ek = NULL;
        result = __FAILURE__;
    }
    else
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_010: [ When complete dps_transport_common_amqp_open shall send a status callback of DPS_TRANSPORT_STATUS_CONNECTED.] */
        amqp_info->register_data_cb = data_callback;
        amqp_info->user_ctx = user_ctx;
        amqp_info->status_cb = status_cb;
        amqp_info->status_ctx = status_ctx;
        amqp_info->status_cb(DPS_TRANSPORT_STATUS_CONNECTED, amqp_info->status_ctx);
        amqp_info->amqp_state = AMQP_STATE_DISCONNECTED;
        result = 0;
    }
    return result;
}

int dps_transport_common_amqp_close(DPS_TRANSPORT_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_011: [ If handle is NULL, dps_transport_common_amqp_close shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;
        BUFFER_delete(amqp_info->ek);
        amqp_info->ek = NULL;
        BUFFER_delete(amqp_info->srk);
        amqp_info->srk = NULL;

        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_012: [ dps_transport_common_amqp_close shall close all links and connection associated with amqp communication. ] */
        messagesender_close(amqp_info->msg_sender);
        messagereceiver_close(amqp_info->msg_receiver);

        messagesender_destroy(amqp_info->msg_sender);
        amqp_info->msg_sender = NULL;
        messagereceiver_destroy(amqp_info->msg_receiver);
        amqp_info->msg_receiver = NULL;

        // Link
        link_destroy(amqp_info->receiver_link);
        amqp_info->receiver_link = NULL;
        link_destroy(amqp_info->sender_link);
        amqp_info->sender_link = NULL;

        if (amqp_info->tpm_sasl_handler != NULL)
        {
            saslmechanism_destroy(amqp_info->tpm_sasl_handler);
            amqp_info->tpm_sasl_handler = NULL;
        }

        session_destroy(amqp_info->session);
        amqp_info->session = NULL;
        connection_destroy(amqp_info->connection);
        amqp_info->connection = NULL;
        xio_destroy(amqp_info->transport_io);
        amqp_info->transport_io = NULL;
        xio_destroy(amqp_info->underlying_io);
        amqp_info->underlying_io = NULL;

        amqp_info->amqp_state = AMQP_STATE_IDLE;
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_013: [ On success dps_transport_common_amqp_close shall return a zero value. ] */
        result = 0;
    }
    return result;
}

int dps_transport_common_amqp_register_device(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx, DPS_TRANSPORT_JSON_PARSE json_parse_cb, void* json_ctx)
{
    int result;
    DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;
    if (amqp_info == NULL || json_parse_cb == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_014: [ If handle is NULL, dps_transport_common_amqp_register_device shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, json_parse_cb: %p", handle, json_parse_cb);
        result = __FAILURE__;
    }
    else if (amqp_info->hsm_type == DPS_HSM_TYPE_TPM && reg_challenge_cb == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_015: [ If hsm_type is of type DPS_HSM_TYPE_TPM and reg_challenge_cb is NULL, dps_transport_common_amqp_register_device shall return a non-zero value. ] */
        LogError("Invalid parameter specified reg_challenge_cb: %p", reg_challenge_cb);
        result = __FAILURE__;
    }
    else if (amqp_info->transport_state == TRANSPORT_CLIENT_STATE_REG_SEND || amqp_info->operation_id != NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_061: [ If the transport_state is TRANSPORT_CLIENT_STATE_REG_SEND or the the operation_id is NULL, dps_transport_common_amqp_register_device shall return a non-zero value. ] */
        LogError("Failure: device is currently in the registration process");
        result = __FAILURE__;
    }
    else if (amqp_info->transport_state == TRANSPORT_CLIENT_STATE_ERROR)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_016: [ If the transport_state is set to TRANSPORT_CLIENT_STATE_ERROR shall, dps_transport_common_amqp_register_device shall return a non-zero value. ] */
        LogError("dps is in an error state, close the connection and try again.");
        result = __FAILURE__;
    }
    else
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_017: [ On success dps_transport_common_amqp_register_device shall return a zero value. ] */
        amqp_info->transport_state = TRANSPORT_CLIENT_STATE_REG_SEND;
        amqp_info->challenge_cb = reg_challenge_cb;
        amqp_info->challenge_ctx = user_ctx;
        amqp_info->json_parse_cb = json_parse_cb;
        amqp_info->json_ctx = json_ctx;

        result = 0;
    }
    return result;
}

int dps_transport_common_amqp_get_operation_status(DPS_TRANSPORT_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_018: [ If handle is NULL, dps_transport_common_amqp_get_operation_status shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;
        if (amqp_info->operation_id == NULL)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_019: [ If the operation_id is NULL, dps_transport_common_amqp_get_operation_status shall return a non-zero value. ] */
            LogError("operation_id was not previously set in the dps challenge method");
            result = __FAILURE__;
        }
        else if (amqp_info->transport_state == TRANSPORT_CLIENT_STATE_ERROR)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_020: [ If the transport_state is set to TRANSPORT_CLIENT_STATE_ERROR shall, dps_transport_common_amqp_get_operation_status shall return a non-zero value. ] */
            LogError("dps is in an error state, close the connection and try again.");
            result = __FAILURE__;
        }
        else
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_021: [ dps_transport_common_amqp_get_operation_status shall set the transport_state to TRANSPORT_CLIENT_STATE_STATUS_SEND. ] */
            amqp_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_SEND;
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_022: [ On success dps_transport_common_amqp_get_operation_status shall return a zero value. ] */
            result = 0;
        }
    }
    return result;
}

void dps_transport_common_amqp_dowork(DPS_TRANSPORT_HANDLE handle)
{
    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_046: [ If handle is NULL, dps_transport_common_amqp_dowork shall do nothing. ] */
    if (handle != NULL)
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;
        if (amqp_info->amqp_state == AMQP_STATE_DISCONNECTED)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_047: [ If the amqp_state is AMQP_STATE_DISCONNECTED dps_transport_common_amqp_dowork shall attempt to connect the amqp connections and links. ] */
            if (create_connection(amqp_info) != 0)
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_049: [ If any error is encountered dps_transport_common_amqp_dowork shall set the amqp_state to AMQP_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                LogError("unable to create amqp connection");
                amqp_info->amqp_state = AMQP_STATE_ERROR;
                amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            }
            else
            {
                amqp_info->amqp_state = AMQP_STATE_CONNECTING;
            }
        }
        else if (amqp_info->amqp_state != AMQP_STATE_IDLE)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_051: [ Once connected dps_transport_common_amqp_dowork shall call uamqp connection dowork function and check the `transport_state` ] */
            connection_dowork(amqp_info->connection);
            if (amqp_info->amqp_state == AMQP_STATE_CONNECTED || amqp_info->amqp_state == AMQP_STATE_ERROR)
            {
                switch (amqp_info->transport_state)
                {
                case TRANSPORT_CLIENT_STATE_REG_SEND:
                    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_053: [ When then transport_state is set to TRANSPORT_CLIENT_STATE_REG_SEND, dps_transport_common_amqp_dowork shall send a AMQP_REGISTER_ME message ] */
                    if (send_amqp_message(amqp_info, AMQP_REGISTER_ME) != 0)
                    {
                        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_049: [ If any error is encountered dps_transport_common_amqp_dowork shall set the amqp_state to AMQP_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                        amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                        amqp_info->amqp_state = AMQP_STATE_ERROR;
                    }
                    else
                    {
                        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_054: [ Upon successful sending of a TRANSPORT_CLIENT_STATE_REG_SEND message, dps_transport_common_amqp_dowork shall set the transport_state to TRANSPORT_CLIENT_STATE_REG_SENT ] */
                        amqp_info->transport_state = TRANSPORT_CLIENT_STATE_REG_SENT;
                    }
                    break;

                case TRANSPORT_CLIENT_STATE_REG_RECV:
                case TRANSPORT_CLIENT_STATE_STATUS_RECV:
                {
                    DPS_JSON_INFO* parse_info = amqp_info->json_parse_cb(amqp_info->payload_data, amqp_info->json_ctx);
                    if (parse_info == NULL)
                    {
                        LogError("Unable to process registration reply.");
                        amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                        amqp_info->amqp_state = AMQP_STATE_ERROR;
                    }
                    else
                    {
                        amqp_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                        switch (parse_info->dps_status)
                        {
                            case DPS_TRANSPORT_STATUS_UNASSIGNED:
                            case DPS_TRANSPORT_STATUS_ASSIGNING:
                                if (parse_info->operation_id == NULL)
                                {
                                    LogError("Failure operation Id invalid");
                                    amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                                    amqp_info->amqp_state = AMQP_STATE_ERROR;
                                }
                                else if (amqp_info->operation_id == NULL && mallocAndStrcpy_s(&amqp_info->operation_id, parse_info->operation_id) != 0)
                                {
                                    LogError("Failure copying operation Id");
                                    amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                                    amqp_info->amqp_state = AMQP_STATE_ERROR;
                                }
                                else
                                {
                                    if (amqp_info->status_cb != NULL)
                                    {
                                        amqp_info->status_cb(parse_info->dps_status, amqp_info->status_ctx);
                                    }
                                }
                                break;

                            case DPS_TRANSPORT_STATUS_ASSIGNED:
                                amqp_info->register_data_cb(DPS_TRANSPORT_RESULT_OK, parse_info->authorization_key, parse_info->iothub_uri, parse_info->device_id, amqp_info->user_ctx);
                                amqp_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                                break;
                            case DPS_TRANSPORT_STATUS_ERROR:
                            default:
                                LogError("Unable to process message reply.");
                                amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                                amqp_info->amqp_state = AMQP_STATE_ERROR;
                                break;

                        }
                        free_json_parse_info(parse_info);
                    }
                    break;
                }

                case TRANSPORT_CLIENT_STATE_STATUS_SEND:
                    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_055: [ When then transport_state is set to TRANSPORT_CLIENT_STATE_STATUS_SEND, dps_transport_common_amqp_dowork shall send a AMQP_OPERATION_STATUS message ] */
                    if (send_amqp_message(amqp_info, AMQP_OPERATION_STATUS) != 0)
                    {
                        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_049: [ If any error is encountered dps_transport_common_amqp_dowork shall set the amqp_state to AMQP_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
                        amqp_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                        amqp_info->amqp_state = AMQP_STATE_ERROR;
                    }
                    else
                    {
                        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_056: [ Upon successful sending of a AMQP_OPERATION_STATUS message, dps_transport_common_amqp_dowork shall set the transport_state to TRANSPORT_CLIENT_STATE_STATUS_SENT ] */
                        amqp_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_SENT;
                    }
                    break;

                case TRANSPORT_CLIENT_STATE_ERROR:
                    /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_057: [ If transport_state is set to TRANSPORT_CLIENT_STATE_ERROR, dps_transport_common_amqp_dowork shall call the register_data_cb function with DPS_TRANSPORT_RESULT_ERROR setting the transport_state to TRANSPORT_CLIENT_STATE_IDLE. ] */
                    amqp_info->register_data_cb(DPS_TRANSPORT_RESULT_ERROR, NULL, NULL, NULL, amqp_info->user_ctx);
                    amqp_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                    amqp_info->amqp_state = AMQP_STATE_IDLE;
                    break;
                case TRANSPORT_CLIENT_STATE_REG_SENT:
                case TRANSPORT_CLIENT_STATE_STATUS_SENT:
                    // Check timout
                    break;
                case TRANSPORT_CLIENT_STATE_IDLE:
                default:
                    break;
                }
            }
        }
    }
}

int dps_transport_common_amqp_set_trace(DPS_TRANSPORT_HANDLE handle, bool trace_on)
{
    int result;
    if (handle == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_023: [ If handle is NULL, dps_transport_common_amqp_set_trace shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_024: [ dps_transport_common_amqp_set_trace shall set the log_trace variable to trace_on. ]*/
        amqp_info->log_trace = trace_on;

        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_059: [ If the uamqp connection is not NULL, dps_transport_common_amqp_set_trace shall set the connection trace option on that connection. ] */
        if (amqp_info->connection != NULL)
        {
            connection_set_trace(amqp_info->connection, amqp_info->log_trace);
        }

        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_025: [ On success dps_transport_common_amqp_set_trace shall return a zero value. ] */
        result = 0;
    }
    return result;
}

int dps_transport_common_amqp_x509_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
{
    int result;
    if (handle == NULL || certificate == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_026: [ If handle or certificate is NULL, dps_transport_common_amqp_x509_cert shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, certificate: %p", handle, certificate);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;

        if (amqp_info->x509_cert != NULL)
        {
            free(amqp_info->x509_cert);
            amqp_info->x509_cert = NULL;
        }
        if (amqp_info->private_key != NULL)
        {
            free(amqp_info->private_key);
            amqp_info->private_key = NULL;
        }

        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_027: [ dps_transport_common_amqp_x509_cert shall copy the certificate and private_key values. ] */
        if (mallocAndStrcpy_s(&amqp_info->x509_cert, certificate) != 0)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_028: [ On any failure dps_transport_common_amqp_x509_cert, shall return a non-zero value. ] */
            result = __FAILURE__;
            LogError("failure allocating certificate");
        }
        else if (mallocAndStrcpy_s(&amqp_info->private_key, private_key) != 0)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_028: [ On any failure dps_transport_common_amqp_x509_cert, shall return a non-zero value. ] */
            LogError("failure allocating certificate");
            free(amqp_info->x509_cert);
            amqp_info->x509_cert = NULL;
            result = __FAILURE__;
        }
        else
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_029: [ On success dps_transport_common_amqp_x509_cert shall return a zero value. ] */
            result = 0;
        }
    }
    return result;
}

int dps_transport_common_amqp_set_trusted_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate)
{
    int result;
    if (handle == NULL || certificate == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_030: [ If handle or certificate is NULL, dps_transport_common_amqp_set_trusted_cert shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, certificate: %p", handle, certificate);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;

        if (amqp_info->certificate != NULL)
        {
            free(amqp_info->certificate);
            amqp_info->certificate = NULL;
        }

        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_031: [ dps_transport_common_amqp_set_trusted_cert shall copy the certificate value. ] */
        if (mallocAndStrcpy_s(&amqp_info->certificate, certificate) != 0)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_032: [ On any failure dps_transport_common_amqp_set_trusted_cert, shall return a non-zero value. ] */
            result = __FAILURE__;
            LogError("failure allocating certificate");
        }
        else
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_033: [ On success dps_transport_common_amqp_set_trusted_cert shall return a zero value. ] */
            result = 0;
        }
    }
    return result;
}

int dps_transport_common_amqp_set_proxy(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
{
    int result;
    if (handle == NULL || proxy_options == NULL)
    {
        /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_034: [ If handle or proxy_options is NULL, dps_transport_common_amqp_set_proxy shall return a non-zero value. ]*/
        LogError("Invalid parameter specified handle: %p, proxy_options: %p", handle, proxy_options);
        result = __FAILURE__;
    }
    else
    {
        DPS_TRANSPORT_AMQP_INFO* amqp_info = (DPS_TRANSPORT_AMQP_INFO*)handle;
        if (proxy_options->host_address == NULL)
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_035: [ If HTTP_PROXY_OPTIONS host_address is NULL, dps_transport_common_amqp_set_proxy shall return a non-zero value. ] */
            LogError("NULL host_address in proxy options");
            result = __FAILURE__;
        }
        else if (((proxy_options->username == NULL) || (proxy_options->password == NULL)) &&
            (proxy_options->username != proxy_options->password))
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_036: [ If HTTP_PROXY_OPTIONS password is not NULL and password is NULL, dps_transport_common_amqp_set_proxy shall return a non-zero value. ] */
            LogError("Only one of username and password for proxy settings was NULL");
            result = __FAILURE__;
        }
        else
        {
            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_037: [ If any of the host_addess, username, or password variables are non-NULL, dps_transport_common_amqp_set_proxy shall free the memory. ] */
            if (amqp_info->proxy_option.host_address != NULL)
            {
                free((char*)amqp_info->proxy_option.host_address);
                amqp_info->proxy_option.host_address = NULL;
            }
            if (amqp_info->proxy_option.username != NULL)
            {
                free((char*)amqp_info->proxy_option.username);
                amqp_info->proxy_option.username = NULL;
            }
            if (amqp_info->proxy_option.password != NULL)
            {
                free((char*)amqp_info->proxy_option.password);
                amqp_info->proxy_option.password = NULL;
            }

            /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_038: [ dps_transport_common_amqp_set_proxy shall copy the host_addess, username, or password variables ] */
            amqp_info->proxy_option.port = proxy_options->port;
            if (mallocAndStrcpy_s((char**)&amqp_info->proxy_option.host_address, proxy_options->host_address) != 0)
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_039: [ On any failure dps_transport_common_amqp_set_proxy, shall return a non-zero value. ] */
                LogError("Failure setting proxy_host name");
                result = __FAILURE__;
            }
            else if (proxy_options->username != NULL && mallocAndStrcpy_s((char**)&amqp_info->proxy_option.username, proxy_options->username) != 0)
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_039: [ On any failure dps_transport_common_amqp_set_proxy, shall return a non-zero value. ] */
                LogError("Failure setting proxy username");
                free((char*)amqp_info->proxy_option.host_address);
                amqp_info->proxy_option.host_address = NULL;
                result = __FAILURE__;
            }
            else if (proxy_options->password != NULL && mallocAndStrcpy_s((char**)&amqp_info->proxy_option.password, proxy_options->password) != 0)
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_039: [ On any failure dps_transport_common_amqp_set_proxy, shall return a non-zero value. ] */
                LogError("Failure setting proxy password");
                free((char*)amqp_info->proxy_option.host_address);
                amqp_info->proxy_option.host_address = NULL;
                free((char*)amqp_info->proxy_option.username);
                amqp_info->proxy_option.username = NULL;
                result = __FAILURE__;
            }
            else
            {
                /* Codes_DPS_TRANSPORT_AMQP_COMMON_07_040: [ On success dps_transport_common_amqp_set_proxy shall return a zero value. ] */
                result = 0;
            }
        }
    }
    return result;
}
