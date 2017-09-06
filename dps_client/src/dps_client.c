// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>

#include "parson.h"

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/sastoken.h" 
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/tickcounter.h"

#include "azure_hub_modules/dps_sec_client.h"
#include "azure_hub_modules/dps_client.h"

static const char* OPTION_LOG_TRACE = "logtrace";
static const char* OPTION_X509_CERT = "x509certificate";
static const char* OPTION_X509_PRIVATE_KEY = "x509privatekey";
static const char* OPTION_KEEP_ALIVE = "keepalive";

static const char* OPTION_PROXY_HOST = "proxy_address";
static const char* OPTION_PROXY_USERNAME = "proxy_username";
static const char* OPTION_PROXY_PASSWORD = "proxy_password";

static const char* JSON_NODE_STATUS = "status";
static const char* JSON_NODE_REG_STATUS = "registrationStatus";
static const char* JSON_NODE_AUTH_KEY = "authenticationKey";
static const char* JSON_NODE_DEVICE_ID = "deviceId";
static const char* JSON_NODE_KEY_NAME = "keyName";
static const char* JSON_NODE_OPERATION_ID = "operationId";
static const char* JSON_NODE_ASSIGNED_HUB = "assignedHub";
static const char* JSON_NODE_TPM_NODE = "tpm";
static const char* JSON_NODE_TRACKING_ID = "trackingId";

static const char* DPS_ASSIGNED_STATUS = "assigned";
static const char* DPS_ASSIGNING_STATUS = "assigning";
static const char* DPS_UNASSIGNED_STATUS = "unassigned";
static const char* DPS_FAILED_STATUS = "failed";
static const char* DPS_BLACKLISTED_STATUS = "blacklisted";

static const char* DPS_API_VERSION = "2017-08-31-preview";
static const char* SAS_TOKEN_SCOPE_FMT = "%s/registrations/%s";

static const char* DPS_CLIENT_VERSION = "1.1.01";

#define SAS_TOKEN_DEFAULT_LIFETIME  3600
#define EPOCH_TIME_T_VALUE          (time_t)0
#define MAX_AUTH_ATTEMPTS           3
#define DPS_GET_THROTTLE_TIME       2
#define DPS_DEFAULT_TIMEOUT         60

typedef enum DPS_CLIENT_STATE_TAG
{
    DPS_CLIENT_STATE_READY,

    DPS_CLIENT_STATE_REGISTER_SEND,
    DPS_CLIENT_STATE_REGISTER_SENT,
    DPS_CLIENT_STATE_REGISTER_RECV,

    DPS_CLIENT_STATE_STATUS_SEND,
    DPS_CLIENT_STATE_STATUS_SENT,
    DPS_CLIENT_STATE_STATUS_RECV,

    DPS_CLIENT_STATE_ERROR
} DPS_CLIENT_STATE;

typedef struct DPS_IOTHUB_REQ_INFO_TAG
{
    char* iothub_url;
    char* iothub_key;
    char* device_id;
} DPS_IOTHUB_REQ_INFO;

typedef struct DPS_INSTANCE_INFO_TAG
{
    DPS_CLIENT_REGISTER_DEVICE_CALLBACK register_callback;
    void* user_context;
    DPS_CLIENT_REGISTER_STATUS_CALLBACK register_status_cb;
    void* status_user_ctx;
    DPS_CLIENT_ON_ERROR_CALLBACK error_callback;
    DPS_ERROR error_reason;
    void* on_error_context;

    const DPS_TRANSPORT_PROVIDER* dps_transport_protocol;
    DPS_TRANSPORT_HANDLE transport_handle;

    TICK_COUNTER_HANDLE tick_counter;

    tickcounter_ms_t status_throttle;
    tickcounter_ms_t timeout_value;

    char* registration_id;

    DPS_SEC_HANDLE dps_sec_handle;

    bool is_connected;

    DPS_SEC_TYPE dps_hsm_type;

    DPS_IOTHUB_REQ_INFO iothub_info;

    DPS_CLIENT_STATE dps_state;

    size_t auth_attempts_made;

    char* scope_id;
} DPS_INSTANCE_INFO;

static char* dps_transport_challenge_callback(const unsigned char* nonce, size_t nonce_len, const char* key_name, void* user_ctx)
{
    char* result;
    if (user_ctx == NULL || nonce == NULL)
    {
        LogError("Bad argument user_ctx: %p, nonce: %p", nonce, nonce_len);
        result = NULL;
    }
    else
    {
        DPS_INSTANCE_INFO* dps_info = (DPS_INSTANCE_INFO*)user_ctx;
        char* token_scope;
        size_t token_scope_len;

        size_t sec_since_epoch = (size_t)(difftime(get_time(NULL), EPOCH_TIME_T_VALUE) + 0);
        size_t expiry_time = sec_since_epoch + SAS_TOKEN_DEFAULT_LIFETIME;

        // Construct Token scope
        token_scope_len = strlen(SAS_TOKEN_SCOPE_FMT) + strlen(dps_info->scope_id) + strlen(dps_info->registration_id);

        token_scope = malloc(token_scope_len + 1);
        if (token_scope == NULL)
        {
            LogError("Failure to allocate token scope");
            result = NULL;
        }
        else if (sprintf(token_scope, SAS_TOKEN_SCOPE_FMT, dps_info->scope_id, dps_info->registration_id) <= 0)
        {
            LogError("Failure to constructing token_scope");
            free(token_scope);
            result = NULL;
        }
        else
        {
            STRING_HANDLE encoded_token = URL_EncodeString(token_scope);
            if (encoded_token == NULL)
            {
                LogError("Failure to url encoding string");
                result = NULL;
            }
            else
            {
                if (dps_sec_import_key(dps_info->dps_sec_handle, nonce, nonce_len) != 0)
                {
                    LogError("Failure to import the dps key");
                    result = NULL;
                }
                else if ((result = dps_sec_construct_sas_token(dps_info->dps_sec_handle, STRING_c_str(encoded_token), key_name, expiry_time)) == NULL)
                {
                    LogError("Failure to import the dps key");
                    result = NULL;
                }
                STRING_delete(encoded_token);
            }
            free(token_scope);
        }
    }
    return result;
}

static DPS_TRANSPORT_STATUS retrieve_status_type(const char* dps_status)
{
    DPS_TRANSPORT_STATUS result;
    if (dps_status == NULL || strcmp(dps_status, DPS_UNASSIGNED_STATUS) == 0)
    {
        result = DPS_TRANSPORT_STATUS_UNASSIGNED;
    }
    else if (strcmp(dps_status, DPS_ASSIGNING_STATUS) == 0)
    {
        result = DPS_TRANSPORT_STATUS_ASSIGNING;
    }
    else if (strcmp(dps_status, DPS_ASSIGNED_STATUS) == 0)
    {
        result = DPS_TRANSPORT_STATUS_ASSIGNED;
    }
    else if (strcmp(dps_status, DPS_FAILED_STATUS) == 0)
    {
        result = DPS_TRANSPORT_STATUS_ERROR;
    }
    else if (strcmp(dps_status, DPS_BLACKLISTED_STATUS) == 0)
    {
        result = DPS_TRANSPORT_STATUS_BLACKLISTED;
    }
    else
    {
        result = DPS_TRANSPORT_STATUS_ERROR;
    }
    return result;
}

static char* retrieve_json_item(JSON_Object* json_object, const char* field_name)
{
    char* result;
    JSON_Value* json_field;
    if ((json_field = json_object_get_value(json_object, field_name)) == NULL)
    {
        LogError("failure retrieving json operation id");
        result = NULL;
    }
    else
    {
        const char* json_item = json_value_get_string(json_field);
        if (json_item != NULL)
        {
            if (mallocAndStrcpy_s(&result, json_item) != 0)
            {
                LogError("failure retrieving operation id");
                result = NULL;
            }
        }
        else
        {
            result = NULL;
        }
    }
    return result;
}

static DPS_JSON_INFO* dps_transport_process_json_reply(const char* json_document, void* user_ctx)
{
    DPS_JSON_INFO* result;
    JSON_Value* root_value;
    JSON_Object* json_object;
    if (user_ctx == NULL)
    {
        LogError("failure user_ctx is NULL");
        result = NULL;
    }
    else if ((root_value = json_parse_string(json_document)) == NULL)
    {
        LogError("failure calling json_parse_string");
        result = NULL;
    }
    else if ((json_object = json_value_get_object(root_value)) == NULL)
    {
        LogError("failure retrieving node root object");
        json_value_free(root_value);
        result = NULL;
    }
    else if ((result = malloc(sizeof(DPS_JSON_INFO))) == NULL)
    {
        LogError("failure allocating DPS_JSON_INFO");
        json_value_free(root_value);
    }
    else
    {
        memset(result, 0, sizeof(DPS_JSON_INFO));
        DPS_INSTANCE_INFO* dps_info = (DPS_INSTANCE_INFO*)user_ctx;
        JSON_Value* json_status = json_object_get_value(json_object, JSON_NODE_STATUS);

        // status can be NULL
        result->dps_status = retrieve_status_type(json_value_get_string(json_status) );
        switch (result->dps_status)
        {
            case DPS_TRANSPORT_STATUS_UNASSIGNED:
            {
                JSON_Value* auth_key;
                if ((auth_key = json_object_get_value(json_object, JSON_NODE_AUTH_KEY)) == NULL)
                {
                    LogError("failure retrieving json auth key value");
                    free(result);
                    result = NULL;
                }
                else
                {
                    const char* nonce_field = json_value_get_string(auth_key);
                    if ((result->authorization_key = Base64_Decoder(nonce_field)) == NULL)
                    {
                        LogError("failure creating buffer nonce field");
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        result->key_name = retrieve_json_item(json_object, JSON_NODE_KEY_NAME);
                        if (result->key_name == NULL)
                        {
                            LogError("failure retrieving keyname field");
                            BUFFER_delete(result->authorization_key);
                            free(result);
                            result = NULL;
                        }
                    }
                }
                break;
            }

            case DPS_TRANSPORT_STATUS_ASSIGNING:
                if ((result->operation_id = retrieve_json_item(json_object, JSON_NODE_OPERATION_ID)) == NULL)
                {
                    LogError("Failure: operation_id node is mising");
                    free(result);
                    result = NULL;
                }
                break;

            case DPS_TRANSPORT_STATUS_ASSIGNED:
            {
                JSON_Object* json_reg_status_node;
                if ((json_reg_status_node = json_object_get_object(json_object, JSON_NODE_REG_STATUS)) == NULL)
                {
                    LogError("failure retrieving json registration status node");
                    free(result);
                    result = NULL;
                }
                else
                {
                    if (dps_info->dps_hsm_type == DPS_SEC_TYPE_TPM)
                    {
                        JSON_Object* json_tpm_node;
                        JSON_Value* auth_key;

                        if ((json_tpm_node = json_object_get_object(json_reg_status_node, JSON_NODE_TPM_NODE)) == NULL)
                        {
                            LogError("failure retrieving tpm node json_tpm_node: %p, auth key: %p", json_tpm_node, result->authorization_key);
                            free(result);
                            result = NULL;
                        }
                        else if ((auth_key = json_object_get_value(json_tpm_node, JSON_NODE_AUTH_KEY)) == NULL)
                        {
                            LogError("failure retrieving json auth key value");
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            const char* nonce_field = json_value_get_string(auth_key);
                            if ((result->authorization_key = Base64_Decoder(nonce_field)) == NULL)
                            {
                                LogError("failure creating buffer nonce field");
                                free(result);
                                result = NULL;
                            }
                        }
                    }

                    if (result != NULL)
                    {
                        if (
                            ((result->iothub_uri = retrieve_json_item(json_reg_status_node, JSON_NODE_ASSIGNED_HUB)) == NULL) ||
                            ((result->device_id = retrieve_json_item(json_reg_status_node, JSON_NODE_DEVICE_ID)) == NULL)
                            )
                        {
                            LogError("failure retrieving json value assigned_hub: %p, device_id: %p", result->iothub_uri, result->device_id);
                            free(result->iothub_uri);
                            free(result->authorization_key);
                            free(result);
                            result = NULL;
                        }
                    }
                }
                break;
            }

            case DPS_TRANSPORT_STATUS_BLACKLISTED:
                LogError("The device is unauthorized with DPS");
                free(result);
                result = NULL;
                break;

            case DPS_TRANSPORT_STATUS_ERROR:
            {
                /*JSON_Object* json_reg_status_node;
                if ((json_reg_status_node = json_object_get_object(json_object, JSON_NODE_REG_STATUS)) == NULL)
                {
                    LogError("failure retrieving json registration status node");
                    free(result);
                    result = NULL;
                }
                else
                {
                    JSON_Object* json_tracking_id_node;
                    if ((json_tracking_id_node = json_object_get_value(json_reg_status_node, JSON_NODE_TRACKING_ID)) == NULL)
                    {
                        LogError("failure retrieving tracking Id node");
                        free(result);
                        result = NULL;
                    }
                    else
                    {

                    }
                }*/
                LogError("Unsuccessful json encountered: %s", json_document);
                break;
            }

            default:
                LogError("invalid json status specified %d", result->dps_status);
                break;
        }
        json_value_free(root_value);
    }
    return result;
}

static void on_transport_registration_data(DPS_TRANSPORT_RESULT transport_result, BUFFER_HANDLE iothub_key, const char* assigned_hub, const char* device_id, void* user_ctx)
{
    if (user_ctx == NULL)
    {
        LogError("user context was unexpectantly NULL");
    }
    else
    {
        DPS_INSTANCE_INFO* dps_info = (DPS_INSTANCE_INFO*)user_ctx;
        if (transport_result == DPS_TRANSPORT_RESULT_OK)
        {
            if (dps_info->dps_hsm_type == DPS_SEC_TYPE_TPM)
            {
                if (iothub_key == NULL)
                {
                    dps_info->dps_state = DPS_CLIENT_STATE_ERROR;
                    dps_info->error_reason = DPS_ERROR_PARSING;
                    LogError("invalid iothub device key");
                }
                else
                {
                    const unsigned char* key_value = BUFFER_u_char(iothub_key);
                    size_t key_len = BUFFER_length(iothub_key);

                    /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_028: [ dps_sec_import_key shall import the specified key into the tpm using secure_device_import_key secure enclave function. ] */
                    if (dps_sec_import_key(dps_info->dps_sec_handle, key_value, key_len) != 0)
                    {
                        dps_info->dps_state = DPS_CLIENT_STATE_ERROR;
                        dps_info->error_reason = DPS_ERROR_KEY_ERROR;
                        LogError("Failure to import the dps key");
                    }
                }
            }

            if (dps_info->dps_state != DPS_CLIENT_STATE_ERROR)
            {
                dps_info->register_callback(DPS_CLIENT_OK, assigned_hub, device_id, dps_info->user_context);
                dps_info->dps_state = DPS_CLIENT_STATE_READY;
            }
        }
        else if (transport_result == DPS_TRANSPORT_RESULT_UNAUTHORIZED)
        {
            dps_info->dps_state = DPS_CLIENT_STATE_ERROR;
            dps_info->error_reason = DPS_ERROR_DEV_AUTH_ERROR;
            LogError("DPS result is unauthorized");
        }
        else
        {
            dps_info->dps_state = DPS_CLIENT_STATE_ERROR;
            dps_info->error_reason = DPS_ERROR_TRANSPORT;
            LogError("Failure retrieving data from the dps service");
        }
    }
}

static void on_transport_status(DPS_TRANSPORT_STATUS transport_status, void* user_ctx)
{
    if (user_ctx == NULL)
    {
        LogError("user_ctx was unexpectatly NULL");
    }
    else
    {
        DPS_INSTANCE_INFO* dps_info = (DPS_INSTANCE_INFO*)user_ctx;
        switch (transport_status)
        {
            case DPS_TRANSPORT_STATUS_CONNECTED:
                dps_info->is_connected = true;
                if (dps_info->register_status_cb != NULL)
                {
                    dps_info->register_status_cb(DPS_REGISTRATION_STATUS_CONNECTED, dps_info->status_user_ctx);
                }
                break;
            case DPS_TRANSPORT_STATUS_AUTHENTICATED:
            case DPS_TRANSPORT_STATUS_ASSIGNING:
            case DPS_TRANSPORT_STATUS_UNASSIGNED:
                dps_info->dps_state = DPS_CLIENT_STATE_STATUS_SEND;
                if (transport_status == DPS_TRANSPORT_STATUS_UNASSIGNED)
                {
                    if (dps_info->register_status_cb != NULL)
                    {
                        dps_info->register_status_cb(DPS_REGISTRATION_STATUS_REGISTERING, dps_info->status_user_ctx);
                    }
                }
                else if (transport_status == DPS_TRANSPORT_STATUS_ASSIGNING)
                {
                    if (dps_info->register_status_cb != NULL)
                    {
                        dps_info->register_status_cb(DPS_REGISTRATION_STATUS_ASSIGNING, dps_info->status_user_ctx);
                    }
                }
                break;
        }
    }
}

static void cleanup_dps_info(DPS_INSTANCE_INFO* dps_info)
{
    free(dps_info->iothub_info.device_id);
    dps_info->iothub_info.device_id = NULL;
    free(dps_info->iothub_info.iothub_key);
    dps_info->iothub_info.iothub_key = NULL;
    free(dps_info->iothub_info.iothub_url);
    dps_info->iothub_info.iothub_url = NULL;
    dps_info->auth_attempts_made = 0;
}

static void destroy_dps_instance(DPS_INSTANCE_INFO* dps_info)
{
    dps_info->dps_transport_protocol->dps_transport_destroy(dps_info->transport_handle);
    cleanup_dps_info(dps_info);
    free(dps_info->scope_id);
    free(dps_info->registration_id);
    dps_sec_destroy(dps_info->dps_sec_handle);
    tickcounter_destroy(dps_info->tick_counter);
    free(dps_info);
}

DPS_LL_HANDLE DPS_LL_Create(const char* dps_uri, const char* scope_id, DPS_TRANSPORT_PROVIDER_FUNCTION dps_protocol, DPS_CLIENT_ON_ERROR_CALLBACK on_error_callback, void* user_context)
{
    DPS_INSTANCE_INFO* result;
    /* Codes_SRS_DPS_CLIENT_07_001: [If dps_uri is NULL DPS_LL_CreateFromUri shall return NULL.] */
    if (dps_uri == NULL || scope_id == NULL || dps_protocol == NULL)
    {
        LogError("Invalid parameter specified dps_uri: %p, scope_id: %p, dps_protocol: %p", dps_uri, scope_id, dps_protocol);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_DPS_CLIENT_07_002: [ DPS_LL_CreateFromUri shall allocate a DPS_LL_HANDLE and initialize all members. ] */
        result = (DPS_INSTANCE_INFO*)malloc(sizeof(DPS_INSTANCE_INFO) );
        if (result == NULL)
        {
            LogError("unable to allocate DPS Instance Info");
        }
        else
        {
            memset(result, 0, sizeof(DPS_INSTANCE_INFO) );

            /* Codes_SRS_DPS_CLIENT_07_028: [ DPS_CLIENT_STATE_READY is the initial state after the DPS object is created which will send a uhttp_client_open call to the DPS http endpoint. ] */
            result->dps_state = DPS_CLIENT_STATE_READY;
            result->dps_transport_protocol = dps_protocol();

            /* Codes_SRS_DPS_CLIENT_07_034: [ DPS_LL_Create shall construct a scope_id by base64 encoding the dps_uri. ] */
            if (mallocAndStrcpy_s(&result->scope_id, scope_id) != 0)
            {
                /* Codes_SRS_DPS_CLIENT_07_003: [ If any error is encountered, DPS_LL_CreateFromUri shall return NULL. ] */
                LogError("failed to construct scope_id");
                free(result);
                result = NULL;
            }
            else if ((result->dps_sec_handle = dps_sec_create()) == NULL)
            {
                /* Codes_SRS_DPS_CLIENT_07_003: [ If any error is encountered, DPS_LL_CreateFromUri shall return NULL. ] */
                LogError("failed calling dps_sec_create\r\n");
                destroy_dps_instance(result);
                result = NULL;
            }
            /* Codes_SRS_DPS_CLIENT_07_035: [ DPS_LL_Create shall store the registration_id from the security module. ] */
            else if ((result->registration_id = dps_sec_get_registration_id(result->dps_sec_handle)) == NULL)
            {
                /* Codes_SRS_DPS_CLIENT_07_003: [ If any error is encountered, DPS_LL_CreateFromUri shall return NULL. ] */
                LogError("failure: Unable to retrieve registration Id from device auth.");
                destroy_dps_instance(result);
                result = NULL;
            }
            else if ((result->tick_counter = tickcounter_create()) == NULL)
            {
                LogError("failure: allocating tickcounter");
                destroy_dps_instance(result);
                result = NULL;
            }
            else
            {
                DPS_HSM_TYPE hsm_type;
                if ((result->dps_hsm_type = dps_sec_get_type(result->dps_sec_handle)) == DPS_SEC_TYPE_TPM)
                {
                    hsm_type = DPS_HSM_TYPE_TPM;
                }
                else
                {
                    hsm_type = DPS_HSM_TYPE_X509;
                }

                if ((result->transport_handle = result->dps_transport_protocol->dps_transport_create(dps_uri, hsm_type, result->scope_id, result->registration_id, DPS_API_VERSION)) == NULL)
                {
                    /* Codes_SRS_DPS_CLIENT_07_003: [ If any error is encountered, DPS_LL_CreateFromUri shall return NULL. ] */
                    LogError("failed calling into transport create");
                    destroy_dps_instance(result);
                    result = NULL;
                }
                else
                {
                    /* Codes_SRS_DPS_CLIENT_07_004: [ on_error_callback shall be used to communicate error that occur during the registration with DPS. ] */
                    result->error_callback = on_error_callback;
                    result->on_error_context = user_context;
                }
            }
        }
    }
    return (DPS_LL_HANDLE)result;
}

void DPS_LL_Destroy(DPS_LL_HANDLE handle)
{
    /* Codes_SRS_DPS_CLIENT_07_005: [ If handle is NULL DPS_LL_Destroy shall do nothing. ] */
    if (handle != NULL)
    {
        handle->dps_transport_protocol->dps_transport_close(handle->transport_handle);
        /* Codes_SRS_DPS_CLIENT_07_006: [ DPS_LL_Destroy shall destroy resources associated with the IoTHub_dps_client ] */
        destroy_dps_instance(handle);
    }
}

DPS_RESULT DPS_LL_Register_Device(DPS_LL_HANDLE handle, DPS_CLIENT_REGISTER_DEVICE_CALLBACK register_callback, void* user_context, DPS_CLIENT_REGISTER_STATUS_CALLBACK reg_status_cb, void* status_ctx)
{
    DPS_RESULT result;
    /* Codes_SRS_DPS_CLIENT_07_007: [ If handle or register_callback is NULL, DPS_LL_Register_Device shall return DPS_CLIENT_INVALID_ARG. ] */
    if (handle == NULL || register_callback == NULL)
    {
        LogError("Invalid parameter specified handle: %p register_callback: %p", handle, register_callback);
        result = DPS_CLIENT_INVALID_ARG;
    }
    else
    {
        BUFFER_HANDLE ek_value = NULL;
        BUFFER_HANDLE srk_value = NULL;

        if (handle->dps_state != DPS_CLIENT_STATE_READY)
        {
            LogError("DPS state is invalid");
            result = DPS_CLIENT_ERROR;
        }
        else
        {
            if (handle->dps_hsm_type == DPS_SEC_TYPE_TPM)
            {
                if ((ek_value = dps_sec_get_endorsement_key(handle->dps_sec_handle)) == NULL)
                {
                    LogError("Could not get endorsement key from tpm");
                    result = DPS_CLIENT_ERROR;
                }
                else if ((srk_value = dps_sec_get_storage_key(handle->dps_sec_handle)) == NULL)
                {
                    LogError("Could not get storage root key from tpm");
                    result = DPS_CLIENT_ERROR;
                    BUFFER_delete(ek_value);
                }
                else
                {
                    result = DPS_CLIENT_OK;
                }
            }
            else
            {
                char* x509_cert;
                char* x509_private_key;
                if ((x509_cert = dps_sec_get_certificate(handle->dps_sec_handle)) == NULL)
                {
                    LogError("Could not get the x509 certificate");
                    result = DPS_CLIENT_ERROR;
                }
                else if ((x509_private_key = dps_sec_get_alias_key(handle->dps_sec_handle)) == NULL)
                {
                    LogError("Could not get the x509 alias key");
                    free(x509_cert);
                    result = DPS_CLIENT_ERROR;
                }
                else
                {
                    if (handle->dps_transport_protocol->dps_transport_x509_cert(handle->transport_handle, x509_cert, x509_private_key) != 0)
                    {
                        LogError("unable to set the x509 certificate information on transport");
                        result = DPS_CLIENT_ERROR;
                    }
                    else
                    {
                        result = DPS_CLIENT_OK;
                    }
                    free(x509_cert);
                    free(x509_private_key);
                }
            }
        }
        if (result == DPS_CLIENT_OK)
        {
            if (handle->dps_transport_protocol->dps_transport_open(handle->transport_handle, ek_value, srk_value, on_transport_registration_data, handle, on_transport_status, handle) != 0)
            {
                LogError("Failure establishing DPS connection");
                result = DPS_CLIENT_ERROR;
            }
            else
            {
                /* Codes_SRS_DPS_CLIENT_07_008: [ DPS_LL_Register_Device shall set the state to send the registration request to DPS on subsequent DoWork calls. ] */
                handle->register_callback = register_callback;
                handle->user_context = user_context;

                handle->register_status_cb = reg_status_cb;
                handle->status_user_ctx = status_ctx;
                handle->dps_state = DPS_CLIENT_STATE_REGISTER_SEND;
                /* Codes_SRS_DPS_CLIENT_07_009: [ Upon success DPS_LL_Register_Device shall return DPS_CLIENT_OK. ] */
                result = DPS_CLIENT_OK;
            }
            BUFFER_delete(ek_value);
            BUFFER_delete(srk_value);
        }
    }
    return result;
}

void DPS_LL_DoWork(DPS_LL_HANDLE handle)
{
    /* Codes_SRS_DPS_CLIENT_07_010: [ If handle is NULL, DPS_LL_DoWork shall do nothing. ] */
    if (handle != NULL)
    {
        DPS_INSTANCE_INFO* dps_info = (DPS_INSTANCE_INFO*)handle;
        /* Codes_SRS_DPS_CLIENT_07_011: [ DPS_LL_DoWork shall call the underlying http_client_dowork function ] */
        dps_info->dps_transport_protocol->dps_transport_dowork(dps_info->transport_handle);

        if (dps_info->is_connected || dps_info->dps_state == DPS_CLIENT_STATE_ERROR)
        {
            switch (dps_info->dps_state)
            {
                case DPS_CLIENT_STATE_REGISTER_SEND:
                    /* Codes_SRS_DPS_CLIENT_07_013: [ DPS_CLIENT_STATE_REGISTER_SEND which shall construct an initial call to the DPS service with endorsement information ] */
                    if (dps_info->dps_transport_protocol->dps_transport_register(dps_info->transport_handle, dps_transport_challenge_callback, dps_info, dps_transport_process_json_reply, dps_info) != 0)
                    {
                        LogError("Failure registering device");
                        dps_info->error_reason = DPS_ERROR_PARSING;
                        dps_info->dps_state = DPS_CLIENT_STATE_ERROR;
                    }
                    else
                    {
                        (void)tickcounter_get_current_ms(dps_info->tick_counter, &dps_info->timeout_value);
                        dps_info->dps_state = DPS_CLIENT_STATE_REGISTER_SENT;
                    }
                    break;

                case DPS_CLIENT_STATE_STATUS_SEND:
                {
                    tickcounter_ms_t current_time = 0;
                    (void)tickcounter_get_current_ms(dps_info->tick_counter, &current_time);

                    if (dps_info->status_throttle == 0 || (current_time - dps_info->status_throttle) / 1000 > DPS_GET_THROTTLE_TIME)
                    {
                        /* Codes_SRS_DPS_CLIENT_07_026: [ Upon receiving the reply of the DPS_CLIENT_STATE_URL_REQ_SEND message from DPS, iothub_dps_client shall process the the reply of the DPS_CLIENT_STATE_URL_REQ_SEND state ] */
                        if (dps_info->dps_transport_protocol->dps_transport_get_op_status(dps_info->transport_handle) != 0)
                        {
                            LogError("Failure sending operation status");
                            dps_info->error_reason = DPS_ERROR_PARSING;
                            dps_info->dps_state = DPS_CLIENT_STATE_ERROR;
                        }
                        else
                        {
                            dps_info->dps_state = DPS_CLIENT_STATE_STATUS_SENT;
                            (void)tickcounter_get_current_ms(dps_info->tick_counter, &dps_info->timeout_value);
                        }
                        dps_info->status_throttle = current_time;
                    }
                    break;
                }

                case DPS_CLIENT_STATE_REGISTER_SENT:
                case DPS_CLIENT_STATE_STATUS_SENT:
                {
                    tickcounter_ms_t current_time = 0;
                    (void)tickcounter_get_current_ms(dps_info->tick_counter, &current_time);
                    if ((current_time - dps_info->timeout_value) / 1000 > DPS_DEFAULT_TIMEOUT)
                    {
                        LogError("Timeout waiting for reply from DPS");
                        dps_info->error_reason = DPS_ERROR_TIMEOUT;
                        dps_info->dps_state = DPS_CLIENT_STATE_ERROR;
                    }
                    break;
                }

                case DPS_CLIENT_STATE_READY:
                    break;

                case DPS_CLIENT_STATE_ERROR:
                default:
                    /* Codes_SRS_DPS_CLIENT_07_004: [ on_error_callback shall be used to communicate error that occur during the registration with DPS. ] */
                    if (dps_info->error_callback)
                    {
                        dps_info->error_callback(dps_info->error_reason, dps_info->on_error_context);
                    }
                    cleanup_dps_info(dps_info);
                    dps_info->dps_state = DPS_CLIENT_STATE_READY;
                    break;
            }
        }
        else
        {
            // Check the connection 
            tickcounter_ms_t current_time = 0;
            (void)tickcounter_get_current_ms(dps_info->tick_counter, &current_time);
            if ((current_time - dps_info->timeout_value) / 1000 > DPS_DEFAULT_TIMEOUT)
            {
                LogError("Failure sending operation status");
                dps_info->error_reason = DPS_ERROR_TIMEOUT;
                dps_info->dps_state = DPS_CLIENT_STATE_ERROR;
            }
        }
    }
}

DPS_RESULT DPS_LL_SetOption(DPS_LL_HANDLE handle, const char* option_name, const void* value)
{
    DPS_RESULT result;
    if (handle == NULL || option_name == NULL)
    {
        LogError("Invalid parameter specified handle: %p option_name: %p", handle, option_name);
        result = DPS_CLIENT_INVALID_ARG;
    }
    else
    {
        if (strcmp(option_name, "TrustedCerts") == 0)
        {
            const char* cert_info = (const char*)value;
            if (handle->dps_transport_protocol->dps_transport_trusted_cert(handle->transport_handle, cert_info) != 0)
            {
                result = DPS_CLIENT_ERROR;
                LogError("failure allocating certificate");
            }
            else
            {
                result = DPS_CLIENT_OK;
            }
        }
        else if (strcmp(OPTION_LOG_TRACE, option_name) == 0)
        {
            bool log_trace = *((bool*)value);
            if (handle->dps_transport_protocol->dps_transport_set_trace(handle->transport_handle, log_trace) != 0)
            {
                result = DPS_CLIENT_ERROR;
                LogError("failure setting trace option");
            }
            else
            {
                result = DPS_CLIENT_OK;
            }
        }
        else if (strcmp(OPTION_HTTP_PROXY, option_name) == 0)
        {
            /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_001: [ If `option` is `proxy_data`, `value` shall be used as an `HTTP_PROXY_OPTIONS*`. ]*/
            HTTP_PROXY_OPTIONS* proxy_options = (HTTP_PROXY_OPTIONS*)value;

            if (handle->dps_transport_protocol->dps_transport_set_proxy(handle->transport_handle, proxy_options) != 0)
            {
                LogError("Failure setting proxy options");
                result = DPS_CLIENT_ERROR;
            }
            else
            {
                result = DPS_CLIENT_OK;
            }
        }
        else
        {
            result = DPS_CLIENT_OK;
        }
    }
    return result;
}

const char* DPS_LL_Get_Version_String(void)
{
    return DPS_CLIENT_VERSION;
}
