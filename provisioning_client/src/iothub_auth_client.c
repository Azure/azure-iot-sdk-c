// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/sha.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "azure_prov_client/iothub_auth_client.h"
#include "azure_prov_client/iothub_security_factory.h"
#include "hsm_client_data.h"
#include "hsm_client_x509_abstract.h"
#include "hsm_client_tpm_abstract.h"

typedef struct IOTHUB_SECURITY_INFO_TAG
{
    DEVICE_AUTH_TYPE cred_type;

    HSM_CLIENT_HANDLE hsm_client_handle;

    HSM_CLIENT_CREATE hsm_client_create;
    HSM_CLIENT_DESTROY hsm_client_destroy;

    HSM_CLIENT_SIGN_DATA hsm_client_sign_data;

    HSM_CLIENT_GET_CERTIFICATE hsm_client_get_cert;
    HSM_CLIENT_GET_ALIAS_KEY hsm_client_get_alias_key;

    char* sas_token;
    char* x509_certificate;
    char* x509_alias_key;
} IOTHUB_SECURITY_INFO;

#define HMAC_LENGTH                 32

IOTHUB_SECURITY_HANDLE iothub_device_auth_create()
{
    IOTHUB_SECURITY_INFO* result;
    if ((result = (IOTHUB_SECURITY_INFO*)malloc(sizeof(IOTHUB_SECURITY_INFO))) == NULL)
    {
        /* Codes_IOTHUB_DEV_AUTH_07_001: [ if any failure is encountered iothub_device_auth_create shall return NULL. ] */
        LogError("Failed allocating IOTHUB_SECURITY_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(IOTHUB_SECURITY_INFO) );
        if (iothub_security_type() == IOTHUB_SECURITY_TYPE_SAS)
        {
            result->cred_type = AUTH_TYPE_SAS;
            const HSM_CLIENT_TPM_INTERFACE* tpm_interface = hsm_client_tpm_interface();
            if (((result->hsm_client_create = tpm_interface->hsm_client_tpm_create) == NULL) ||
                ((result->hsm_client_destroy = tpm_interface->hsm_client_tpm_destroy) == NULL) ||
                ((result->hsm_client_sign_data = tpm_interface->hsm_client_sign_data) == NULL)
                )
            {
                /* Codes_IOTHUB_DEV_AUTH_07_034: [ if any of the iothub_security_interface function are NULL iothub_device_auth_create shall return NULL. ] */
                LogError("Invalid secure device interface");
                free(result);
                result = NULL;
            }
        }
        else
        {
            result->cred_type = AUTH_TYPE_X509;
            const HSM_CLIENT_X509_INTERFACE* x509_interface = hsm_client_x509_interface();
            if (((result->hsm_client_create = x509_interface->hsm_client_x509_create) == NULL) ||
                ((result->hsm_client_destroy = x509_interface->hsm_client_x509_destroy) == NULL) ||
                ((result->hsm_client_get_cert = x509_interface->hsm_client_get_cert) == NULL) ||
                ((result->hsm_client_get_alias_key = x509_interface->hsm_client_get_key) == NULL)
                )
            {
                /* Codes_IOTHUB_DEV_AUTH_07_034: [ if any of the iothub_security_interface function are NULL iothub_device_auth_create shall return NULL. ] */
                LogError("Invalid handle parameter: DEVICE_AUTH_CREATION_INFO is NULL");
                free(result);
                result = NULL;
            }
        }

        if (result != NULL)
        {
            /* Codes_IOTHUB_DEV_AUTH_07_025: [ iothub_device_auth_create shall call the concrete_iothub_device_auth_create function associated with the interface_desc. ] */
            /* Codes_IOTHUB_DEV_AUTH_07_026: [ if concrete_iothub_device_auth_create fails iothub_device_auth_create shall return NULL. ] */
            if ((result->hsm_client_handle = result->hsm_client_create()) == NULL)
            {
                /* Codes_IOTHUB_DEV_AUTH_07_002: [ iothub_device_auth_create shall allocate the IOTHUB_SECURITY_INFO and shall fail if the allocation fails. ]*/
                LogError("failed create device auth module.");
                free(result);
                result = NULL;
            }
        }
    }
    /* Codes_IOTHUB_DEV_AUTH_07_003: [ If the function succeeds iothub_device_auth_create shall return a IOTHUB_SECURITY_HANDLE. ] */
    return result;
}

void iothub_device_auth_destroy(IOTHUB_SECURITY_HANDLE handle)
{
    /* Codes_IOTHUB_DEV_AUTH_07_006: [ If the argument handle is NULL, iothub_device_auth_destroy shall do nothing ] */
    if (handle != NULL)
    {
        /* Codes_IOTHUB_DEV_AUTH_07_005: [ iothub_device_auth_destroy shall call the concrete_iothub_device_auth_destroy function associated with the XDA_INTERFACE_DESCRIPTION. ] */
        free(handle->x509_certificate);
        free(handle->x509_alias_key);
        free(handle->sas_token);
        handle->hsm_client_destroy(handle->hsm_client_handle);
        /* Codes_IOTHUB_DEV_AUTH_07_004: [ iothub_device_auth_destroy shall free all resources associated with the IOTHUB_SECURITY_HANDLE handle ] */
        free(handle);
    }
}

DEVICE_AUTH_TYPE iothub_device_auth_get_type(IOTHUB_SECURITY_HANDLE handle)
{
    DEVICE_AUTH_TYPE result;
    /* Codes_IOTHUB_DEV_AUTH_07_007: [ If the argument handle is NULL, iothub_device_auth_get_auth_type shall return AUTH_TYPE_UNKNOWN. ] */
    if (handle == NULL)
    {
        LogError("Invalid handle specified");
        result = AUTH_TYPE_UNKNOWN;
    }
    else
    {
        /* Codes_IOTHUB_DEV_AUTH_07_008: [ iothub_device_auth_get_auth_type shall call concrete_iothub_device_auth_type function associated with the XDA_INTERFACE_DESCRIPTION. ] */
        /* Codes_IOTHUB_DEV_AUTH_07_009: [ iothub_device_auth_get_auth_type shall return the DEVICE_AUTH_TYPE returned by the concrete_iothub_device_auth_type function. ] */
        result = handle->cred_type;
    }
    return result;
}

CREDENTIAL_RESULT* iothub_device_auth_generate_credentials(IOTHUB_SECURITY_HANDLE handle, const DEVICE_AUTH_CREDENTIAL_INFO* dev_auth_cred)
{
    CREDENTIAL_RESULT* result;
    /* Codes_IOTHUB_DEV_AUTH_07_010: [ If the argument handle or dev_auth_cred is NULL, iothub_device_auth_generate_credentials shall return a NULL value. ] */
    if (handle == NULL)
    {
        LogError("Invalid handle parameter: handle");
        result = NULL;
    }
    else
    {
        if (handle->cred_type == AUTH_TYPE_SAS)
        {
            if (handle->sas_token != NULL)
            {
                free(handle->sas_token);
                handle->sas_token = NULL;
            }
            char expire_token[64] = { 0 };
            if (dev_auth_cred == NULL)
            {
                LogError("Invalid handle parameter: dev_auth_cred");
                result = NULL;
            }
            else if (dev_auth_cred->dev_auth_type != AUTH_TYPE_SAS)
            {
                LogError("Invalid handle parameter: dev_auth_cred.dev_auth_type");
                result = NULL;
            }
            else if (size_tToString(expire_token, sizeof(expire_token), (size_t)dev_auth_cred->sas_info.expiry_seconds) != 0)
            {
                result = NULL;
                LogError("Failure creating expire token");
            }
            else
            {
                size_t len = strlen(dev_auth_cred->sas_info.token_scope)+strlen(expire_token)+3;
                char* payload = malloc(len+1);
                if (payload == NULL)
                {
                    result = NULL;
                    LogError("Failure allocating payload.");
                }
                else
                {
                    unsigned char* data_value;
                    size_t data_len;

                    size_t total_len = sprintf(payload, "%s\n%s", dev_auth_cred->sas_info.token_scope, expire_token);
                    if (total_len <= 0)
                    {
                        result = NULL;
                        LogError("Failure constructing hash payload.");
                    }
                    /* Codes_IOTHUB_DEV_AUTH_07_035: [ For tpm type iothub_device_auth_generate_credentials shall call the concrete_dev_auth_sign_data function to hash the data. ] */
                    else if (handle->hsm_client_sign_data(handle->hsm_client_handle, (const unsigned char*)payload, strlen(payload), &data_value, &data_len) == 0)
                    {
                        STRING_HANDLE urlEncodedSignature;
                        STRING_HANDLE base64Signature;
                        STRING_HANDLE sas_token_handle;
                        if ((base64Signature = Base64_Encode_Bytes(data_value, data_len)) == NULL)
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
                            sas_token_handle = STRING_construct_sprintf("SharedAccessSignature sr=%s&sig=%s&se=%s&skn=", dev_auth_cred->sas_info.token_scope, STRING_c_str(urlEncodedSignature), expire_token);
                            if (sas_token_handle == NULL)
                            {
                                result = NULL;
                                LogError("Failure constructing url Signature.");
                            }
                            else if ((result = malloc(sizeof(CREDENTIAL_RESULT))) == NULL)
                            {
                                STRING_delete(sas_token_handle);
                                LogError("Failure allocating credential result.");
                            }
                            else
                            {
                                const char* temp_sas_token = STRING_c_str(sas_token_handle);
                                if (mallocAndStrcpy_s(&handle->sas_token, temp_sas_token) != 0)
                                {
                                    free(result);
                                    result = NULL;
                                    LogError("Failure allocating and copying string.");
                                }
                                else
                                {
                                    result->auth_cred_result.sas_result.sas_token = handle->sas_token;
                                }
                                STRING_delete(sas_token_handle);
                            }
                            STRING_delete(base64Signature);
                            STRING_delete(urlEncodedSignature);
                        }
                        free(data_value);
                    }
                    else
                    {
                        result = NULL;
                        LogError("Failure generate hash from tpm.");
                    }
                    free(payload);
                }
            }
        }
        else
        {
            if (handle->x509_certificate != NULL)
            {
                free(handle->x509_certificate);
                handle->x509_certificate = NULL;
            }
            if (handle->x509_alias_key != NULL)
            {
                free(handle->x509_alias_key);
                handle->x509_alias_key = NULL;
            }

            if ((result = malloc(sizeof(CREDENTIAL_RESULT))) == NULL)
            {
                LogError("Failure allocating credential result.");
            }
            else if ((handle->x509_certificate = handle->hsm_client_get_cert(handle->hsm_client_handle)) == NULL)
            {
                LogError("Failure allocating device credential result.");
                free(result);
                result = NULL;
            }
            else if ((handle->x509_alias_key = handle->hsm_client_get_alias_key(handle->hsm_client_handle)) == NULL)
            {
                LogError("Failure allocating device credential result.");
                free(handle->x509_certificate);
                handle->x509_certificate = NULL;
                free(result);
                result = NULL;
            }
            else
            {
                result->auth_cred_result.x509_result.x509_cert = handle->x509_certificate;
                result->auth_cred_result.x509_result.x509_alias_key = handle->x509_alias_key;
            }
        }
    }
    return result;
}
