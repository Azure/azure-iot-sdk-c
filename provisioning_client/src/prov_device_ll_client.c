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

#include "azure_prov_client/internal/prov_auth_client.h"
#include "azure_prov_client/internal/prov_transport_private.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_client_const.h"

static const char* OPTION_LOG_TRACE = "logtrace";

static const char* JSON_NODE_STATUS = "status";
static const char* JSON_NODE_REG_STATUS = "registrationState";
static const char* JSON_NODE_AUTH_KEY = "authenticationKey";
static const char* JSON_NODE_DEVICE_ID = "deviceId";
static const char* JSON_NODE_KEY_NAME = "keyName";
static const char* JSON_NODE_OPERATION_ID = "operationId";
static const char* JSON_NODE_ASSIGNED_HUB = "assignedHub";
static const char* JSON_NODE_TPM_NODE = "tpm";
static const char* JSON_NODE_TRACKING_ID = "trackingId";

static const char* PROV_FAILED_STATUS = "failed";
static const char* PROV_BLACKLISTED_STATUS = "blacklisted";

static const char* SAS_TOKEN_SCOPE_FMT = "%s/registrations/%s";

#define SAS_TOKEN_DEFAULT_LIFETIME  3600
#define EPOCH_TIME_T_VALUE          (time_t)0
#define MAX_AUTH_ATTEMPTS           3
#define PROV_GET_THROTTLE_TIME      2
#define PROV_DEFAULT_TIMEOUT        60

typedef enum CLIENT_STATE_TAG
{
    CLIENT_STATE_READY,

    CLIENT_STATE_REGISTER_SEND,
    CLIENT_STATE_REGISTER_SENT,
    CLIENT_STATE_REGISTER_RECV,

    CLIENT_STATE_STATUS_SEND,
    CLIENT_STATE_STATUS_SENT,
    CLIENT_STATE_STATUS_RECV,

    CLIENT_STATE_ERROR
} CLIENT_STATE;

typedef struct IOTHUB_REQ_INFO_TAG
{
    char* iothub_url;
    char* iothub_key;
    char* device_id;
} IOTHUB_REQ_INFO;

typedef struct PROV_INSTANCE_INFO_TAG
{
    PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK register_callback;
    void* user_context;
    PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK register_status_cb;
    void* status_user_ctx;
    PROV_DEVICE_RESULT error_reason;

    const PROV_DEVICE_TRANSPORT_PROVIDER* prov_transport_protocol;
    PROV_DEVICE_TRANSPORT_HANDLE transport_handle;

    TICK_COUNTER_HANDLE tick_counter;

    tickcounter_ms_t status_throttle;
    tickcounter_ms_t timeout_value;

    char* registration_id;
    bool user_supplied_reg_id;

    PROV_AUTH_HANDLE prov_auth_handle;

    bool is_connected;

    PROV_AUTH_TYPE hsm_type;

    IOTHUB_REQ_INFO iothub_info;

    CLIENT_STATE prov_state;

    size_t auth_attempts_made;

    char* scope_id;
} PROV_INSTANCE_INFO;

static char* prov_transport_challenge_callback(const unsigned char* nonce, size_t nonce_len, const char* key_name, void* user_ctx)
{
    char* result;
    if (user_ctx == NULL || nonce == NULL)
    {
        LogError("Bad argument user_ctx: %p, nonce: %p", nonce, nonce_len);
        result = NULL;
    }
    else
    {
        PROV_INSTANCE_INFO* prov_info = (PROV_INSTANCE_INFO*)user_ctx;
        char* token_scope;
        size_t token_scope_len;

        size_t sec_since_epoch = (size_t)(difftime(get_time(NULL), EPOCH_TIME_T_VALUE) + 0);
        size_t expiry_time = sec_since_epoch + SAS_TOKEN_DEFAULT_LIFETIME;

        // Construct Token scope
        token_scope_len = strlen(SAS_TOKEN_SCOPE_FMT) + strlen(prov_info->scope_id) + strlen(prov_info->registration_id);

        token_scope = malloc(token_scope_len + 1);
        if (token_scope == NULL)
        {
            LogError("Failure to allocate token scope");
            result = NULL;
        }
        else if (sprintf(token_scope, SAS_TOKEN_SCOPE_FMT, prov_info->scope_id, prov_info->registration_id) <= 0)
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
                if (prov_auth_import_key(prov_info->prov_auth_handle, nonce, nonce_len) != 0)
                {
                    LogError("Failure to import the provisioning key");
                    result = NULL;
                }
                else if ((result = prov_auth_construct_sas_token(prov_info->prov_auth_handle, STRING_c_str(encoded_token), key_name, expiry_time)) == NULL)
                {
                    LogError("Failure to import the provisioning key");
                    result = NULL;
                }
                STRING_delete(encoded_token);
            }
            free(token_scope);
        }
    }
    return result;
}

static PROV_DEVICE_TRANSPORT_STATUS retrieve_status_type(const char* prov_status)
{
    PROV_DEVICE_TRANSPORT_STATUS result;
    if (prov_status == NULL || strcmp(prov_status, PROV_UNASSIGNED_STATUS) == 0)
    {
        result = PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED;
    }
    else if (strcmp(prov_status, PROV_ASSIGNING_STATUS) == 0)
    {
        result = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
    }
    else if (strcmp(prov_status, PROV_ASSIGNED_STATUS) == 0)
    {
        result = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED;
    }
    else if (strcmp(prov_status, PROV_FAILED_STATUS) == 0)
    {
        result = PROV_DEVICE_TRANSPORT_STATUS_ERROR;
    }
    else if (strcmp(prov_status, PROV_BLACKLISTED_STATUS) == 0)
    {
        result = PROV_DEVICE_TRANSPORT_STATUS_BLACKLISTED;
    }
    else
    {
        result = PROV_DEVICE_TRANSPORT_STATUS_ERROR;
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

static PROV_JSON_INFO* prov_transport_process_json_reply(const char* json_document, void* user_ctx)
{
    PROV_JSON_INFO* result;
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
    else if ((result = malloc(sizeof(PROV_JSON_INFO))) == NULL)
    {
        LogError("failure allocating PROV_JSON_INFO");
        json_value_free(root_value);
    }
    else
    {
        memset(result, 0, sizeof(PROV_JSON_INFO));
        PROV_INSTANCE_INFO* prov_info = (PROV_INSTANCE_INFO*)user_ctx;
        JSON_Value* json_status = json_object_get_value(json_object, JSON_NODE_STATUS);

        // status can be NULL
        result->prov_status = retrieve_status_type(json_value_get_string(json_status) );
        switch (result->prov_status)
        {
            case PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED:
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

            case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING:
                if ((result->operation_id = retrieve_json_item(json_object, JSON_NODE_OPERATION_ID)) == NULL)
                {
                    LogError("Failure: operation_id node is mising");
                    free(result);
                    result = NULL;
                }
                break;

            case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED:
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
                    if (prov_info->hsm_type == PROV_AUTH_TYPE_TPM)
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

            case PROV_DEVICE_TRANSPORT_STATUS_BLACKLISTED:
                LogError("The device is unauthorized with service");
                free(result);
                result = NULL;
                break;

            case PROV_DEVICE_TRANSPORT_STATUS_ERROR:
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
                free(result);
                result = NULL;
                break;
            }

            default:
                LogError("invalid json status specified %d", result->prov_status);
                free(result);
                result = NULL;
                break;
        }
        json_value_free(root_value);
    }
    return result;
}

static void on_transport_registration_data(PROV_DEVICE_TRANSPORT_RESULT transport_result, BUFFER_HANDLE iothub_key, const char* assigned_hub, const char* device_id, void* user_ctx)
{
    if (user_ctx == NULL)
    {
        LogError("user context was unexpectantly NULL");
    }
    else
    {
        PROV_INSTANCE_INFO* prov_info = (PROV_INSTANCE_INFO*)user_ctx;
        if (transport_result == PROV_DEVICE_TRANSPORT_RESULT_OK)
        {
            if (prov_info->hsm_type == PROV_AUTH_TYPE_TPM)
            {
                if (iothub_key == NULL)
                {
                    prov_info->prov_state = CLIENT_STATE_ERROR;
                    prov_info->error_reason = PROV_DEVICE_RESULT_PARSING;
                    LogError("invalid iothub device key");
                }
                else
                {
                    const unsigned char* key_value = BUFFER_u_char(iothub_key);
                    size_t key_len = BUFFER_length(iothub_key);

                    /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_028: [ prov_auth_import_key shall import the specified key into the tpm using secure_device_import_key secure enclave function. ] */
                    if (prov_auth_import_key(prov_info->prov_auth_handle, key_value, key_len) != 0)
                    {
                        prov_info->prov_state = CLIENT_STATE_ERROR;
                        prov_info->error_reason = PROV_DEVICE_RESULT_KEY_ERROR;
                        LogError("Failure to import the provisioning key");
                    }
                }
            }

            if (prov_info->prov_state != CLIENT_STATE_ERROR)
            {
                prov_info->register_callback(PROV_DEVICE_RESULT_OK, assigned_hub, device_id, prov_info->user_context);
                prov_info->prov_state = CLIENT_STATE_READY;
            }
        }
        else if (transport_result == PROV_DEVICE_TRANSPORT_RESULT_UNAUTHORIZED)
        {
            prov_info->prov_state = CLIENT_STATE_ERROR;
            prov_info->error_reason = PROV_DEVICE_RESULT_DEV_AUTH_ERROR;
            LogError("provisioning result is unauthorized");
        }
        else
        {
            prov_info->prov_state = CLIENT_STATE_ERROR;
            prov_info->error_reason = PROV_DEVICE_RESULT_TRANSPORT;
            LogError("Failure retrieving data from the provisioning service");
        }
    }
}

static void on_transport_status(PROV_DEVICE_TRANSPORT_STATUS transport_status, void* user_ctx)
{
    if (user_ctx == NULL)
    {
        LogError("user_ctx was unexpectatly NULL");
    }
    else
    {
        PROV_INSTANCE_INFO* prov_info = (PROV_INSTANCE_INFO*)user_ctx;
        switch (transport_status)
        {
            case PROV_DEVICE_TRANSPORT_STATUS_CONNECTED:
                prov_info->is_connected = true;
                if (prov_info->register_status_cb != NULL)
                {
                    prov_info->register_status_cb(PROV_DEVICE_REG_STATUS_CONNECTED, prov_info->status_user_ctx);
                }
                break;
            case PROV_DEVICE_TRANSPORT_STATUS_AUTHENTICATED:
            case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING:
            case PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED:
                prov_info->prov_state = CLIENT_STATE_STATUS_SEND;
                if (transport_status == PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED)
                {
                    if (prov_info->register_status_cb != NULL)
                    {
                        prov_info->register_status_cb(PROV_DEVICE_REG_STATUS_REGISTERING, prov_info->status_user_ctx);
                    }
                }
                else if (transport_status == PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING)
                {
                    if (prov_info->register_status_cb != NULL)
                    {
                        prov_info->register_status_cb(PROV_DEVICE_REG_STATUS_ASSIGNING, prov_info->status_user_ctx);
                    }
                }
                break;
            default:
                LogError("Unknown status encountered");
                break;
        }
    }
}

static void cleanup_prov_info(PROV_INSTANCE_INFO* prov_info)
{
    free(prov_info->iothub_info.device_id);
    prov_info->iothub_info.device_id = NULL;
    free(prov_info->iothub_info.iothub_key);
    prov_info->iothub_info.iothub_key = NULL;
    free(prov_info->iothub_info.iothub_url);
    prov_info->iothub_info.iothub_url = NULL;
    prov_info->auth_attempts_made = 0;
}

static void destroy_instance(PROV_INSTANCE_INFO* prov_info)
{
    prov_info->prov_transport_protocol->prov_transport_destroy(prov_info->transport_handle);
    cleanup_prov_info(prov_info);
    free(prov_info->scope_id);
    free(prov_info->registration_id);
    prov_auth_destroy(prov_info->prov_auth_handle);
    tickcounter_destroy(prov_info->tick_counter);
    free(prov_info);
}

PROV_DEVICE_LL_HANDLE Prov_Device_LL_Create(const char* uri, const char* id_scope, PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION protocol)
{
    PROV_INSTANCE_INFO* result;
    /* Codes_SRS_PROV_CLIENT_07_001: [If uri is NULL Prov_Device_LL_CreateFromUri shall return NULL.] */
    if (uri == NULL || id_scope == NULL || protocol == NULL)
    {
        LogError("Invalid parameter specified uri: %p, id_scope: %p, protocol: %p", uri, id_scope, protocol);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_PROV_CLIENT_07_002: [ Prov_Device_LL_CreateFromUri shall allocate a PROV_DEVICE_LL_HANDLE and initialize all members. ] */
        result = (PROV_INSTANCE_INFO*)malloc(sizeof(PROV_INSTANCE_INFO) );
        if (result == NULL)
        {
            LogError("unable to allocate Instance Info");
        }
        else
        {
            memset(result, 0, sizeof(PROV_INSTANCE_INFO) );

            /* Codes_SRS_PROV_CLIENT_07_028: [ CLIENT_STATE_READY is the initial state after the object is created which will send a uhttp_client_open call to the http endpoint. ] */
            result->prov_state = CLIENT_STATE_READY;
            result->prov_transport_protocol = protocol();

            /* Codes_SRS_PROV_CLIENT_07_034: [ Prov_Device_LL_Create shall construct a id_scope by base64 encoding the uri. ] */
            if (mallocAndStrcpy_s(&result->scope_id, id_scope) != 0)
            {
                /* Codes_SRS_PROV_CLIENT_07_003: [ If any error is encountered, Prov_Device_LL_CreateFromUri shall return NULL. ] */
                LogError("failed to construct id_scope");
                free(result);
                result = NULL;
            }
            else if ((result->prov_auth_handle = prov_auth_create()) == NULL)
            {
                /* Codes_SRS_PROV_CLIENT_07_003: [ If any error is encountered, Prov_Device_LL_CreateFromUri shall return NULL. ] */
                LogError("failed calling prov_auth_create\r\n");
                destroy_instance(result);
                result = NULL;
            }
            else if ((result->tick_counter = tickcounter_create()) == NULL)
            {
                LogError("failure: allocating tickcounter");
                destroy_instance(result);
                result = NULL;
            }
            else
            {
                TRANSPORT_HSM_TYPE hsm_type;
                if ((result->hsm_type = prov_auth_get_type(result->prov_auth_handle)) == PROV_AUTH_TYPE_TPM)
                {
                    hsm_type = TRANSPORT_HSM_TYPE_TPM;
                }
                else
                {
                    hsm_type = TRANSPORT_HSM_TYPE_X509;
                }

                if ((result->transport_handle = result->prov_transport_protocol->prov_transport_create(uri, hsm_type, result->scope_id, PROV_API_VERSION)) == NULL)
                {
                    /* Codes_SRS_PROV_CLIENT_07_003: [ If any error is encountered, Prov_Device_LL_CreateFromUri shall return NULL. ] */
                    LogError("failed calling into transport create");
                    destroy_instance(result);
                    result = NULL;
                }
            }
        }
    }
    return (PROV_DEVICE_LL_HANDLE)result;
}

void Prov_Device_LL_Destroy(PROV_DEVICE_LL_HANDLE handle)
{
    /* Codes_SRS_PROV_CLIENT_07_005: [ If handle is NULL Prov_Device_LL_Destroy shall do nothing. ] */
    if (handle != NULL)
    {
        handle->prov_transport_protocol->prov_transport_close(handle->transport_handle);
        /* Codes_SRS_PROV_CLIENT_07_006: [ Prov_Device_LL_Destroy shall destroy resources associated with the IoTHub_client ] */
        destroy_instance(handle);
    }
}

PROV_DEVICE_RESULT Prov_Device_LL_Register_Device(PROV_DEVICE_LL_HANDLE handle, PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK register_callback, void* user_context, PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK reg_status_cb, void* status_ctx)
{
    PROV_DEVICE_RESULT result;
    /* Codes_SRS_PROV_CLIENT_07_007: [ If handle or register_callback is NULL, Prov_Device_LL_Register_Device shall return PROV_CLIENT_INVALID_ARG. ] */
    if (handle == NULL || register_callback == NULL)
    {
        LogError("Invalid parameter specified handle: %p register_callback: %p", handle, register_callback);
        result = PROV_DEVICE_RESULT_INVALID_ARG;
    }
    /* Codes_SRS_PROV_CLIENT_07_035: [ Prov_Device_LL_Create shall store the registration_id from the security module. ] */
    else if (handle->registration_id == NULL && (handle->registration_id = prov_auth_get_registration_id(handle->prov_auth_handle)) == NULL)
    {
        /* Codes_SRS_PROV_CLIENT_07_003: [ If any error is encountered, Prov_Device_LL_CreateFromUri shall return NULL. ] */
        LogError("failure: Unable to retrieve registration Id from device auth.");
        result = PROV_DEVICE_RESULT_ERROR;
    }
    else
    {
        BUFFER_HANDLE ek_value = NULL;
        BUFFER_HANDLE srk_value = NULL;

        if (handle->prov_state != CLIENT_STATE_READY)
        {
            LogError("state is invalid");
            if (!handle->user_supplied_reg_id)
            {
                free(handle->registration_id);
                handle->registration_id = NULL;
            }
            result = PROV_DEVICE_RESULT_ERROR;
        }
        else
        {
            if (handle->hsm_type == PROV_AUTH_TYPE_TPM)
            {
                if ((ek_value = prov_auth_get_endorsement_key(handle->prov_auth_handle)) == NULL)
                {
                    LogError("Could not get endorsement key from tpm");
                    if (!handle->user_supplied_reg_id)
                    {
                        free(handle->registration_id);
                        handle->registration_id = NULL;
                    }
                    result = PROV_DEVICE_RESULT_ERROR;
                }
                else if ((srk_value = prov_auth_get_storage_key(handle->prov_auth_handle)) == NULL)
                {
                    LogError("Could not get storage root key from tpm");
                    if (!handle->user_supplied_reg_id)
                    {
                        free(handle->registration_id);
                        handle->registration_id = NULL;
                    }
                    result = PROV_DEVICE_RESULT_ERROR;
                    BUFFER_delete(ek_value);
                }
                else
                {
                    result = PROV_DEVICE_RESULT_OK;
                }
            }
            else
            {
                char* x509_cert;
                char* x509_private_key;
                if ((x509_cert = prov_auth_get_certificate(handle->prov_auth_handle)) == NULL)
                {
                    LogError("Could not get the x509 certificate");
                    if (!handle->user_supplied_reg_id)
                    {
                        free(handle->registration_id);
                        handle->registration_id = NULL;
                    }
                    result = PROV_DEVICE_RESULT_ERROR;
                }
                else if ((x509_private_key = prov_auth_get_alias_key(handle->prov_auth_handle)) == NULL)
                {
                    LogError("Could not get the x509 alias key");
                    if (!handle->user_supplied_reg_id)
                    {
                        free(handle->registration_id);
                        handle->registration_id = NULL;
                    }
                    free(x509_cert);
                    result = PROV_DEVICE_RESULT_ERROR;
                }
                else
                {
                    if (handle->prov_transport_protocol->prov_transport_x509_cert(handle->transport_handle, x509_cert, x509_private_key) != 0)
                    {
                        LogError("unable to set the x509 certificate information on transport");
                        if (!handle->user_supplied_reg_id)
                        {
                            free(handle->registration_id);
                            handle->registration_id = NULL;
                        }
                        result = PROV_DEVICE_RESULT_ERROR;
                    }
                    else
                    {
                        result = PROV_DEVICE_RESULT_OK;
                    }
                    free(x509_cert);
                    free(x509_private_key);
                }
            }
        }
        if (result == PROV_DEVICE_RESULT_OK)
        {
            /* Codes_SRS_PROV_CLIENT_07_008: [ Prov_Device_LL_Register_Device shall set the state to send the registration request to on subsequent DoWork calls. ] */
            handle->register_callback = register_callback;
            handle->user_context = user_context;

            handle->register_status_cb = reg_status_cb;
            handle->status_user_ctx = status_ctx;

            if (handle->prov_transport_protocol->prov_transport_open(handle->transport_handle, handle->registration_id, ek_value, srk_value, on_transport_registration_data, handle, on_transport_status, handle) != 0)
            {
                LogError("Failure establishing  connection");
                if (!handle->user_supplied_reg_id)
                {
                    free(handle->registration_id);
                    handle->registration_id = NULL;
                }
                handle->register_callback = NULL;
                handle->user_context = NULL;

                handle->register_status_cb = NULL;
                handle->status_user_ctx = NULL;
                result = PROV_DEVICE_RESULT_ERROR;
            }
            else
            {
                handle->prov_state = CLIENT_STATE_REGISTER_SEND;
                /* Codes_SRS_PROV_CLIENT_07_009: [ Upon success Prov_Device_LL_Register_Device shall return PROV_CLIENT_OK. ] */
                result = PROV_DEVICE_RESULT_OK;
            }
            BUFFER_delete(ek_value);
            BUFFER_delete(srk_value);
        }
    }
    return result;
}

void Prov_Device_LL_DoWork(PROV_DEVICE_LL_HANDLE handle)
{
    /* Codes_SRS_PROV_CLIENT_07_010: [ If handle is NULL, Prov_Device_LL_DoWork shall do nothing. ] */
    if (handle != NULL)
    {
        PROV_INSTANCE_INFO* prov_info = (PROV_INSTANCE_INFO*)handle;
        /* Codes_SRS_PROV_CLIENT_07_011: [ Prov_Device_LL_DoWork shall call the underlying http_client_dowork function ] */
        prov_info->prov_transport_protocol->prov_transport_dowork(prov_info->transport_handle);

        if (prov_info->is_connected || prov_info->prov_state == CLIENT_STATE_ERROR)
        {
            switch (prov_info->prov_state)
            {
                case CLIENT_STATE_REGISTER_SEND:
                    /* Codes_SRS_PROV_CLIENT_07_013: [ CLIENT_STATE_REGISTER_SEND which shall construct an initial call to the service with endorsement information ] */
                    if (prov_info->prov_transport_protocol->prov_transport_register(prov_info->transport_handle, prov_transport_challenge_callback, prov_info, prov_transport_process_json_reply, prov_info) != 0)
                    {
                        LogError("Failure registering device");
                        prov_info->error_reason = PROV_DEVICE_RESULT_PARSING;
                        prov_info->prov_state = CLIENT_STATE_ERROR;
                    }
                    else
                    {
                        (void)tickcounter_get_current_ms(prov_info->tick_counter, &prov_info->timeout_value);
                        prov_info->prov_state = CLIENT_STATE_REGISTER_SENT;
                    }
                    break;

                case CLIENT_STATE_STATUS_SEND:
                {
                    tickcounter_ms_t current_time = 0;
                    (void)tickcounter_get_current_ms(prov_info->tick_counter, &current_time);

                    if (prov_info->status_throttle == 0 || (current_time - prov_info->status_throttle) / 1000 > PROV_GET_THROTTLE_TIME)
                    {
                        /* Codes_SRS_PROV_CLIENT_07_026: [ Upon receiving the reply of the CLIENT_STATE_URL_REQ_SEND message from  iothub_client shall process the the reply of the CLIENT_STATE_URL_REQ_SEND state ] */
                        if (prov_info->prov_transport_protocol->prov_transport_get_op_status(prov_info->transport_handle) != 0)
                        {
                            LogError("Failure sending operation status");
                            prov_info->error_reason = PROV_DEVICE_RESULT_PARSING;
                            prov_info->prov_state = CLIENT_STATE_ERROR;
                        }
                        else
                        {
                            prov_info->prov_state = CLIENT_STATE_STATUS_SENT;
                            (void)tickcounter_get_current_ms(prov_info->tick_counter, &prov_info->timeout_value);
                        }
                        prov_info->status_throttle = current_time;
                    }
                    break;
                }

                case CLIENT_STATE_REGISTER_SENT:
                case CLIENT_STATE_STATUS_SENT:
                {
                    tickcounter_ms_t current_time = 0;
                    (void)tickcounter_get_current_ms(prov_info->tick_counter, &current_time);
                    if ((current_time - prov_info->timeout_value) / 1000 > PROV_DEFAULT_TIMEOUT)
                    {
                        LogError("Timeout waiting for reply");
                        prov_info->error_reason = PROV_DEVICE_RESULT_TIMEOUT;
                        prov_info->prov_state = CLIENT_STATE_ERROR;
                    }
                    break;
                }

                case CLIENT_STATE_READY:
                    break;

                case CLIENT_STATE_ERROR:
                default:
                    prov_info->register_callback(prov_info->error_reason, NULL, NULL, prov_info->user_context);
                    cleanup_prov_info(prov_info);
                    prov_info->prov_state = CLIENT_STATE_READY;
                    break;
            }
        }
        else
        {
            // Check the connection 
            tickcounter_ms_t current_time = 0;
            (void)tickcounter_get_current_ms(prov_info->tick_counter, &current_time);
            if ((current_time - prov_info->timeout_value) / 1000 > PROV_DEFAULT_TIMEOUT)
            {
                LogError("Timed out connecting to provisioning service");
                prov_info->error_reason = PROV_DEVICE_RESULT_TIMEOUT;
                prov_info->prov_state = CLIENT_STATE_ERROR;
            }
        }
    }
}

PROV_DEVICE_RESULT Prov_Device_LL_SetOption(PROV_DEVICE_LL_HANDLE handle, const char* option_name, const void* value)
{
    PROV_DEVICE_RESULT result;
    if (handle == NULL || option_name == NULL)
    {
        LogError("Invalid parameter specified handle: %p option_name: %p", handle, option_name);
        result = PROV_DEVICE_RESULT_INVALID_ARG;
    }
    else
    {
        if (strcmp(option_name, OPTION_TRUSTED_CERT) == 0)
        {
            const char* cert_info = (const char*)value;
            if (handle->prov_transport_protocol->prov_transport_trusted_cert(handle->transport_handle, cert_info) != 0)
            {
                result = PROV_DEVICE_RESULT_ERROR;
                LogError("failure allocating certificate");
            }
            else
            {
                result = PROV_DEVICE_RESULT_OK;
            }
        }
        else if (strcmp(OPTION_LOG_TRACE, option_name) == 0)
        {
            bool log_trace = *((bool*)value);
            if (handle->prov_transport_protocol->prov_transport_set_trace(handle->transport_handle, log_trace) != 0)
            {
                result = PROV_DEVICE_RESULT_ERROR;
                LogError("failure setting trace option");
            }
            else
            {
                result = PROV_DEVICE_RESULT_OK;
            }
        }
        else if (strcmp(OPTION_HTTP_PROXY, option_name) == 0)
        {
            /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_001: [ If `option` is `proxy_data`, `value` shall be used as an `HTTP_PROXY_OPTIONS*`. ]*/
            HTTP_PROXY_OPTIONS* proxy_options = (HTTP_PROXY_OPTIONS*)value;

            if (handle->prov_transport_protocol->prov_transport_set_proxy(handle->transport_handle, proxy_options) != 0)
            {
                LogError("setting proxy options");
                result = PROV_DEVICE_RESULT_ERROR;
            }
            else
            {
                result = PROV_DEVICE_RESULT_OK;
            }
        }
        else if (strcmp(PROV_REGISTRATION_ID, option_name) == 0)
        {
            if (handle->prov_state != CLIENT_STATE_READY)
            {
                LogError("registration id cannot be set after registration has begun");
                result = PROV_DEVICE_RESULT_ERROR;
            }
            else if (value == NULL)
            {
                LogError("value must be set to the correct registration id");
                result = PROV_DEVICE_RESULT_ERROR;
            }
            else
            {
                if (handle->registration_id != NULL)
                {
                    free(handle->registration_id);
                }
                
                if (mallocAndStrcpy_s(&handle->registration_id, (const char*)value) != 0)
                {
                    LogError("Failure allocating setting registration id");
                    result = PROV_DEVICE_RESULT_ERROR;
                }
                else if (prov_auth_set_registration_id(handle->prov_auth_handle, handle->registration_id) != 0)
                {
                    LogError("Failure setting registration id");
                    free(handle->registration_id);
                    result = PROV_DEVICE_RESULT_ERROR;
                }
                else
                {
                    handle->user_supplied_reg_id = true;
                    result = PROV_DEVICE_RESULT_OK;
                }
            }
        }
        else
        {
            result = PROV_DEVICE_RESULT_OK;
        }
    }
    return result;
}

const char* Prov_Device_LL_GetVersionString(void)
{
    return PROV_DEVICE_CLIENT_VERSION;
}
