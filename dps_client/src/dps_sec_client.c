// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_hub_modules/base32.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/sha.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/sastoken.h"

#include "azure_hub_modules/dps_sec_client.h"
#include "azure_hub_modules/secure_device_factory.h"

typedef struct DPS_SEC_INFO_TAG
{
    DPS_SECURE_DEVICE_HANDLE sec_dev_handle;

    DPS_SEC_TYPE sec_type;

    char* registration_id;

    DPS_SECURE_DEVICE_CREATE secure_device_create;
    DPS_SECURE_DEVICE_DESTROY secure_device_destroy;

    DPS_SECURE_DEVICE_IMPORT_KEY secure_device_import_key;
    DPS_SECURE_DEVICE_GET_EK secure_device_get_ek;
    DPS_SECURE_DEVICE_GET_SRK secure_device_get_srk;
    DPS_SECURE_DEVICE_SIGN_DATA secure_device_sign_data;

    DPS_SECURE_DEVICE_GET_CERTIFICATE secure_device_get_cert;
    DPS_SECURE_DEVICE_GET_ALIAS_KEY secure_device_get_ak;
    DPS_SECURE_DEVICE_GET_SIGNER_CERT secure_device_get_signer_cert;
    DPS_SECURE_DEVICE_GET_ROOT_CERT secure_device_get_root_cert;
    DPS_SECURE_DEVICE_GET_ROOT_KEY secure_device_get_root_key;
    DPS_SECURE_DEVICE_GET_COMMON_NAME secure_device_get_common_name;
} DPS_SEC_INFO;

static char* encode_value(const uint8_t* msg_digest, size_t digest_len)
{
    char* result;

    char* encoded_hash = Base32_Encode_Bytes(msg_digest, digest_len);
    if (encoded_hash == NULL)
    {
        LogError("Failure encoded Base32");
        result = NULL;
    }
    else
    {
        size_t encode_len = strlen(encoded_hash);
        char* iterator = encoded_hash + encode_len - 1;

        while (iterator != encoded_hash)
        {
            if (*iterator != '=')
            {
                *(iterator+1) = '\0';
                break;
            }
            iterator--;
        }

        if (mallocAndStrcpy_s(&result, encoded_hash) != 0)
        {
            LogError("Failure allocating encoded base32 result");
            result = NULL;
        }
        free(encoded_hash);
    }
    return result;
}

static int load_registration_id(DPS_SEC_INFO* handle)
{
    int result;
    if (handle->sec_type == DPS_SEC_TYPE_TPM)
    {
        SHA256Context sha_ctx;
        STRING_HANDLE encoded_hash;
        uint8_t msg_digest[SHA256HashSize];

        BUFFER_HANDLE endorsement_key = handle->secure_device_get_ek(handle->sec_dev_handle);
        if (endorsement_key == NULL)
        {
            LogError("Failed getting device reg id");
            encoded_hash = NULL;
            result = __FAILURE__;
        }
        else
        {
            if (SHA256Reset(&sha_ctx) != 0)
            {
                LogError("Failed sha256 reset");
                encoded_hash = NULL;
                result = __FAILURE__;
            }
            else if (SHA256Input(&sha_ctx, BUFFER_u_char(endorsement_key), (unsigned int)BUFFER_length(endorsement_key)) != 0)
            {
                LogError("Failed SHA256Input");
                encoded_hash = NULL;
                result = __FAILURE__;
            }
            else if (SHA256Result(&sha_ctx, msg_digest) != 0)
            {
                LogError("Failed SHA256Result");
                encoded_hash = NULL;
                result = __FAILURE__;
            }
            else
            {
                handle->registration_id = encode_value(msg_digest, SHA256HashSize);
                if (handle->registration_id == NULL)
                {
                    LogError("Failed allocating registration Id");
                    result = __FAILURE__;
                }
                else
                {
                    result = 0;
                }
            }
            BUFFER_delete(endorsement_key);
        }
    }
    else
    {
        handle->registration_id = handle->secure_device_get_common_name(handle->sec_dev_handle);
        if (handle->registration_id == NULL)
        {
            LogError("Failed getting common name from certificate");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

DPS_SEC_HANDLE dps_sec_create()
{
    DPS_SEC_INFO* result;
    /* Codes_SRS_DPS_SEC_CLIENT_07_001: [ dps_sec_create shall allocate the DPS_SEC_INFO. ] */
    if ((result = (DPS_SEC_INFO*)malloc(sizeof(DPS_SEC_INFO))) == NULL)
    {
        LogError("Failed allocating DPS_SEC_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(DPS_SEC_INFO) );
        if (dps_secure_device_type() == SECURE_DEVICE_TYPE_TPM)
        {
            /* Codes_SRS_DPS_SEC_CLIENT_07_003: [ dps_sec_create shall validate the specified secure enclave interface to ensure. ] */
            result->sec_type = DPS_SEC_TYPE_TPM;
            const SEC_TPM_INTERFACE* tpm_interface = dps_secure_device_interface();
            if ( ( (result->secure_device_create = tpm_interface->secure_device_create) == NULL) ||
                ((result->secure_device_destroy = tpm_interface->secure_device_destroy) == NULL) ||
                ((result->secure_device_import_key = tpm_interface->secure_device_import_key) == NULL) ||
                ((result->secure_device_get_ek = tpm_interface->secure_device_get_ek) == NULL) ||
                ((result->secure_device_get_srk = tpm_interface->secure_device_get_srk) == NULL) ||
                ((result->secure_device_sign_data = tpm_interface->secure_device_sign_data) == NULL)
                )
            {
                /* Codes_SRS_DPS_SEC_CLIENT_07_002: [ If any failure is encountered dps_sec_create shall return NULL ] */
                LogError("Invalid secure device interface");
                free(result);
                result = NULL;
            }
        }
        else
        {
            /* Codes_SRS_DPS_SEC_CLIENT_07_003: [ dps_sec_create shall validate the specified secure enclave interface to ensure. ] */
            result->sec_type = DPS_SEC_TYPE_X509;
            const SEC_X509_INTERFACE* riot_interface = dps_secure_device_interface();
            if ( ( (result->secure_device_create = riot_interface->secure_device_create) == NULL) ||
                ((result->secure_device_destroy = riot_interface->secure_device_destroy) == NULL) ||
                ((result->secure_device_get_cert = riot_interface->secure_device_get_cert) == NULL) ||
                ((result->secure_device_get_signer_cert = riot_interface->secure_device_get_signer_cert) == NULL) ||
                ((result->secure_device_get_root_cert = riot_interface->secure_device_get_root_cert) == NULL) ||
                ((result->secure_device_get_root_key = riot_interface->secure_device_get_root_key) == NULL) ||
                ((result->secure_device_get_common_name = riot_interface->secure_device_get_common_name) == NULL) ||
                ((result->secure_device_get_ak = riot_interface->secure_device_get_ak) == NULL)
                )
            {
                LogError("Invalid handle parameter: DEVICE_AUTH_CREATION_INFO is NULL");
                free(result);
                result = NULL;
            }
        }

        if (result != NULL)
        {
            /* Codes_SRS_DPS_SEC_CLIENT_07_004: [ dps_sec_create shall call secure_device_create on the secure enclave interface. ] */
            if ((result->sec_dev_handle = result->secure_device_create()) == NULL)
            {
                LogError("failed create device auth module.");
                free(result);
                result = NULL;
            }
        }

    }
    return result;
}

void dps_sec_destroy(DPS_SEC_HANDLE handle)
{
    /* Codes_SRS_DPS_SEC_CLIENT_07_005: [ if handle is NULL, dps_sec_destroy shall do nothing. ] */
    if (handle != NULL)
    {
        /* Codes_SRS_DPS_SEC_CLIENT_07_007: [ dps_sec_destroy shall free all resources allocated in this module. ] */
        free(handle->registration_id);
        handle->secure_device_destroy(handle->sec_dev_handle);
        /* Codes_SRS_DPS_SEC_CLIENT_07_006: [ dps_sec_destroy shall free the DPS_SEC_HANDLE instance. ] */
        free(handle);
    }
}

DPS_SEC_TYPE dps_sec_get_type(DPS_SEC_HANDLE handle)
{
    DPS_SEC_TYPE result;
    /* Codes_SRS_DPS_SEC_CLIENT_07_008: [ if handle is NULL dps_sec_get_type shall return DPS_SEC_TYPE_UNKNOWN ] */
    if (handle == NULL)
    {
        LogError("Invalid handle specified");
        result = DPS_SEC_TYPE_UNKNOWN;
    }
    else
    {
        /* Codes_SRS_DPS_SEC_CLIENT_07_009: [ dps_sec_get_type shall return the DPS_SEC_TYPE of the underlying secure enclave. ] */
        result = handle->sec_type;
    }
    return result;
}

char* dps_sec_get_registration_id(DPS_SEC_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_DPS_SEC_CLIENT_07_010: [ if handle is NULL dps_sec_get_registration_id shall return NULL. ] */
        LogError("Invalid handle parameter");
        result = NULL;
    }
    else
    {
        int load_result = 0;
        if (handle->registration_id == NULL)
        {
            /* Codes_SRS_DPS_SEC_CLIENT_07_011: [ dps_sec_get_registration_id shall load the registration id if it has not been previously loaded. ] */
            load_result = load_registration_id(handle);
        }

        if (load_result != 0)
        {
            /* Codes_SRS_DPS_SEC_CLIENT_07_012: [ If a failure is encountered, dps_sec_get_registration_id shall return NULL. ] */
            LogError("Failed loading registration key");
            result = NULL;
        }
        /* Codes_SRS_DPS_SEC_CLIENT_07_013: [ Upon success dps_sec_get_registration_id shall return the registration id of the secure enclave. ] */
        else if (mallocAndStrcpy_s(&result, handle->registration_id) != 0)
        {
            /* Codes_SRS_DPS_SEC_CLIENT_07_012: [ If a failure is encountered, dps_sec_get_registration_id shall return NULL. ] */
            LogError("Failed allocating registration key");
            result = NULL;
        }
    }
    return result;
}

BUFFER_HANDLE dps_sec_get_endorsement_key(DPS_SEC_HANDLE handle)
{
    BUFFER_HANDLE result;
    if (handle == NULL)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_021: [ If handle is NULL dps_sec_get_endorsement_key shall return NULL. ] */
        LogError("Invalid handle parameter");
        result = NULL;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_TPM)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_023: [ If the sec_type is DPS_SEC_TYPE_X509, dps_sec_get_endorsement_key shall return NULL. ] */
        LogError("Invalid type for operation");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_022: [ dps_sec_get_endorsement_key shall return the endorsement key returned by the secure_device_get_ek secure enclave function. ] */
        result = handle->secure_device_get_ek(handle->sec_dev_handle);
    }
    return result;
}

BUFFER_HANDLE dps_sec_get_storage_key(DPS_SEC_HANDLE handle)
{
    BUFFER_HANDLE result;
    /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_024: [ If handle is NULL dps_sec_get_storage_key shall return NULL. ] */
    if (handle == NULL)
    {
        LogError("Invalid handle parameter");
        result = NULL;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_TPM)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_026: [ If the sec_type is DPS_SEC_TYPE_X509, dps_sec_get_storage_key shall return NULL. ] */
        LogError("Invalid type for operation");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_025: [ dps_sec_get_storage_key shall return the endorsement key returned by the secure_device_get_srk secure enclave function. ] */
        result = handle->secure_device_get_srk(handle->sec_dev_handle);
    }
    return result;
}

int dps_sec_import_key(DPS_SEC_HANDLE handle, const unsigned char* key_value, size_t key_len)
{
    int result;
    if (handle == NULL || key_value == NULL || key_len == 0)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_027: [ If handle or key are NULL dps_sec_import_key shall return a non-zero value. ] */
        LogError("Invalid handle parameter");
        result = __FAILURE__;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_TPM)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_029: [ If the sec_type is not SECURE_ENCLAVE_TYPE_TPM, dps_sec_import_key shall return NULL. ] */
        LogError("Invalid type for operation");
        result = __FAILURE__;
    }
    else
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_028: [ dps_sec_import_key shall import the specified key into the tpm using secure_device_import_key secure enclave function. ] */
        if (handle->secure_device_import_key(handle->sec_dev_handle, key_value, key_len) != 0)
        {
            /* SRS_SECURE_ENCLAVE_CLIENT_07_040: [ If secure_device_import_key returns an error dps_sec_import_key shall return a non-zero value. ]*/
            LogError("failure importing key into tpm");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

char* dps_sec_construct_sas_token(DPS_SEC_HANDLE handle, const char* token_scope, const char* key_name, size_t expiry_time)
{
    char* result;
    char expire_token[64] = { 0 };

    if (handle == NULL || token_scope == NULL || key_name == NULL)
    {
        LogError("Invalid handle parameter handle: %p, token_scope: %p, key_name: %p", handle, token_scope, key_name);
        result = NULL;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_TPM)
    {
        LogError("Invalid type for operation");
        result = NULL;
    }
    else if (size_tToString(expire_token, sizeof(expire_token), expiry_time) != 0)
    {
        result = NULL;
        LogError("Failure creating expire token");
    }
    else
    {
        size_t len = strlen(token_scope) + strlen(expire_token) + 3;
        char* payload = malloc(len + 1);
        if (payload == NULL)
        {
            result = NULL;
            LogError("Failure allocating payload for sas token.");
        }
        else
        {
            BUFFER_HANDLE hash_handle;
            (void)sprintf(payload, "%s\n%s", token_scope, expire_token);

            /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_031: [ dps_sec_get_certificate shall import the specified cert into the client using secure_device_get_cert secure enclave function. ] */
            if ((hash_handle = handle->secure_device_sign_data(handle->sec_dev_handle, (const unsigned char*)payload, strlen(payload))) != NULL)
            {
                STRING_HANDLE urlEncodedSignature;
                STRING_HANDLE base64Signature;
                STRING_HANDLE sas_token_handle;
                if ((base64Signature = Base64_Encoder(hash_handle)) == NULL)
                {
                    result = NULL;
                    LogError("Failure constructing base64 encoding.");
                }
                else if ((urlEncodedSignature = URL_Encode(base64Signature)) == NULL)
                {
                    result = NULL;
                    LogError("Failure constructing url Signature.");
                    STRING_delete(base64Signature);
                }
                else
                {
                    sas_token_handle = STRING_construct_sprintf("SharedAccessSignature sr=%s&sig=%s&se=%s&skn=", token_scope, STRING_c_str(urlEncodedSignature), expire_token);
                    if (sas_token_handle == NULL)
                    {
                        result = NULL;
                        LogError("Failure constructing url Signature.");
                    }
                    else
                    {
                        const char* temp_sas_token = STRING_c_str(sas_token_handle);
                        if (mallocAndStrcpy_s(&result, temp_sas_token) != 0)
                        {
                            LogError("Failure allocating and copying string.");
                            result = NULL;
                        }
                        STRING_delete(sas_token_handle);
                    }
                    STRING_delete(base64Signature);
                    STRING_delete(urlEncodedSignature);
                }
                BUFFER_delete(hash_handle);
            }
            else
            {
                result = NULL;
                LogError("Failure generate sas token.");
            }
            free(payload);
        }
    }
    return result;
}

char* dps_sec_get_certificate(DPS_SEC_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_030: [ If handle or key are NULL dps_sec_get_certificate shall return a non-zero value. ] */
        LogError("Invalid handle parameter");
        result = NULL;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_X509)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_032: [ If the sec_type is not DPS_SEC_TYPE_X509, dps_sec_get_certificate shall return NULL. ] */
        LogError("Invalid type for operation");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_031: [ dps_sec_get_certificate shall import the specified cert into the client using secure_device_get_cert secure enclave function. ] */
        result = handle->secure_device_get_cert(handle->sec_dev_handle);
    }
    return result;
}

char* dps_sec_get_alias_key(DPS_SEC_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_033: [ If handle or key are NULL dps_sec_get_alias_key shall return a non-zero value. ] */
        LogError("Invalid handle parameter");
        result = NULL;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_X509)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_035: [ If the sec_type is not DPS_SEC_TYPE_X509, dps_sec_get_alias_key shall return NULL. ] */
        LogError("Invalid type for operation");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_034: [ dps_sec_get_alias_key shall import the specified alias key into the client using secure_device_get_ak secure enclave function. ] */
        result = handle->secure_device_get_ak(handle->sec_dev_handle);
    }
    return result;
}

char* dps_sec_get_signer_cert(DPS_SEC_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_036: [ If handle or key are NULL dps_sec_get_signer_cert shall return a non-zero value. ] */
        LogError("Invalid handle parameter");
        result = NULL;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_X509)
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_038: [ If the sec_type is not DPS_SEC_TYPE_X509, dps_sec_get_signer_cert shall return NULL. ] */
        LogError("Invalid type for operation");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_SECURE_ENCLAVE_CLIENT_07_037: [ dps_sec_get_signer_cert shall import the specified signer cert into the client using secure_device_get_signer_cert secure enclave function. ] */
        result = handle->secure_device_get_signer_cert(handle->sec_dev_handle);
    }
    return result;
}

char* dps_sec_get_root_cert(DPS_SEC_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle parameter");
        result = NULL;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_X509)
    {
        LogError("Invalid type for operation");
        result = NULL;
    }
    else
    {
        result = handle->secure_device_get_root_cert(handle->sec_dev_handle);
    }
    return result;
}

char* dps_sec_get_root_key(DPS_SEC_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle parameter");
        result = NULL;
    }
    else if (handle->sec_type != DPS_SEC_TYPE_X509)
    {
        LogError("Invalid type for operation");
        result = NULL;
    }
    else
    {
        result = handle->secure_device_get_root_key(handle->sec_dev_handle);
    }
    return result;
}
