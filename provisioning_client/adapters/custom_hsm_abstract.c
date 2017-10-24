// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_hub_modules/secure_device_factory.h"
#include "azure_hub_modules/iothub_security_factory.h"
#include "azure_c_shared_utility/buffer_.h"

#include "custom_hsm_abstract.h"
#include "custom_hsm_tpm_impl.h"
#include "custom_hsm_x509_impl.h"

typedef struct HSM_SECURE_DEVICE_INFO_TAG
{
    int dev_info;
    DPS_CUSTOM_HSM_HANDLE custom_handle;
} HSM_SECURE_DEVICE_INFO;

static const SEC_X509_INTERFACE sec_x509_interface =
{
    dps_hsm_custom_create,
    dps_hsm_custom_destroy,
    dps_hsm_custom_get_certificate,
    dps_hsm_custom_get_alias_key,
    dps_hsm_custom_get_signer_cert,
    dps_hsm_custom_get_common_name
};

static const SEC_TPM_INTERFACE sec_tpm_interface =
{
    dps_hsm_custom_create,
    dps_hsm_custom_destroy,
    dps_hsm_custom_import_key,
    dps_hsm_custom_get_endorsement_key,
    dps_hsm_custom_get_storage_key,
    dps_hsm_custom_sign_data
};

static const SAS_SECURITY_INTERFACE sas_security_interface =
{
    iothub_hsm_custom_create,
    iothub_hsm_custom_destroy,
    iothub_hsm_custom_sign_data
};

static const X509_SECURITY_INTERFACE x509_security_interface =
{
    iothub_hsm_custom_create,
    iothub_hsm_custom_destroy,
    iothub_hsm_custom_get_certificate,
    iothub_hsm_custom_get_alias_key
};

int initialize_custom_hsm()
{
    return 0;
}

void deinitialize_custom_hsm()
{
}

static HSM_SECURE_DEVICE_INFO* create_custom_hsm()
{
    HSM_SECURE_DEVICE_INFO* result;
    result = malloc(sizeof(HSM_SECURE_DEVICE_INFO));
    if (result == NULL)
    {
        LogError("Failure: malloc SEC_DEVICE_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(HSM_SECURE_DEVICE_INFO));

        result->custom_handle = custom_hsm_create();
        if (result->custom_handle == NULL)
        {
            LogError("Failure initializing custom hsm.");
            free(result);
            result = NULL;
        }
    }
    return result;
}

static void destroy_custom_hsm(HSM_SECURE_DEVICE_INFO* hsm_info)
{
    custom_hsm_destroy(hsm_info->custom_handle);
    free(hsm_info);
}

static char* get_hsm_certificate_key(HSM_SECURE_DEVICE_INFO* hsm_info)
{
    char* result;
    result = custom_hsm_get_certificate(hsm_info->custom_handle);
    if (result == NULL)
    {
        LogError("Failure retrieving alias key from custom HSM.");
    }
    return result;
}

static char* get_hsm_alias_key(HSM_SECURE_DEVICE_INFO* hsm_info)
{
    char* result;
    result = custom_hsm_get_alias_key(hsm_info->custom_handle);
    if (result == NULL)
    {
        LogError("Failure retrieving alias key from custom HSM.");
    }
    return result;
}

static BUFFER_HANDLE get_hsm_signed_key(HSM_SECURE_DEVICE_INFO* hsm_info, const unsigned char* data, size_t data_len)
{
    BUFFER_HANDLE result;
    unsigned char* value;
    size_t value_len;
    if (custom_hsm_sign_key(hsm_info->custom_handle, data, data_len, &value, &value_len) != 0)
    {
        LogError("Failure signing key from custom hsm");
        result = NULL;
    }
    else
    {
        result = BUFFER_create(value, value_len);
        if (result == NULL)
        {
            LogError("Failure allocating sign data");
        }
        free(value);
    }
    return result;
}

DPS_SECURE_DEVICE_HANDLE dps_hsm_custom_create()
{
    return (DPS_SECURE_DEVICE_HANDLE)create_custom_hsm();
}

void dps_hsm_custom_destroy(DPS_SECURE_DEVICE_HANDLE handle)
{
    if (handle != NULL)
    {
        destroy_custom_hsm((HSM_SECURE_DEVICE_INFO*)handle);
    }
}

SECURITY_DEVICE_HANDLE iothub_hsm_custom_create()
{
    return (SECURITY_DEVICE_HANDLE)create_custom_hsm();
}

void iothub_hsm_custom_destroy(SECURITY_DEVICE_HANDLE handle)
{
    if (handle != NULL)
    {
        destroy_custom_hsm((HSM_SECURE_DEVICE_INFO*)handle);
    }
}

char* dps_hsm_custom_get_certificate(DPS_SECURE_DEVICE_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        // allocate certificate and return
        result = get_hsm_certificate_key((HSM_SECURE_DEVICE_INFO*)handle);
        if (result == NULL)
        {
            LogError("Failure retrieving certificate from custom HSM.");
        }
    }
    return result;
}

const char* iothub_hsm_custom_get_certificate(SECURITY_DEVICE_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = "";
    }
    return result;
}

char* dps_hsm_custom_get_alias_key(DPS_SECURE_DEVICE_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        // allocate certificate and return
        result = get_hsm_alias_key((HSM_SECURE_DEVICE_INFO*)handle);
        if (result == NULL)
        {
            LogError("Failure retrieving alias key from custom HSM.");
        }
    }
    return result;
}

const char* iothub_hsm_custom_get_alias_key(SECURITY_DEVICE_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = "";
    }
    return result;
}

char* dps_hsm_custom_get_signer_cert(DPS_SECURE_DEVICE_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_SECURE_DEVICE_INFO* hsm_info = (HSM_SECURE_DEVICE_INFO*)handle;
        result = custom_hsm_get_signer_cert(hsm_info->custom_handle);
        if (result == NULL)
        {
            LogError("Failure retrieving alias key from custom HSM.");
        }
    }
    return result;
}

char* dps_hsm_custom_get_common_name(DPS_SECURE_DEVICE_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        // Return the common name from the certificate
        HSM_SECURE_DEVICE_INFO* hsm_info = (HSM_SECURE_DEVICE_INFO*)handle;
        result = custom_hsm_get_common_name(hsm_info->custom_handle);
        if (result == NULL)
        {
            LogError("Failure retrieving common name from custom HSM.");
        }
    }
    return result;
}

// TPM Custom Information handling
BUFFER_HANDLE dps_hsm_custom_get_endorsement_key(DPS_SECURE_DEVICE_HANDLE handle)
{
    BUFFER_HANDLE result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        unsigned char* ek_value;
        size_t ek_len;
        HSM_SECURE_DEVICE_INFO* hsm_info = (HSM_SECURE_DEVICE_INFO*)handle;
        if (custom_hsm_get_endorsement_key(hsm_info->custom_handle, &ek_value, &ek_len) != 0)
        {
            LogError("Failure getting endorsement key from custom hsm");
            result = NULL;
        }
        else
        {
            // Allocate and return the endorsement key
            result = BUFFER_create(ek_value, ek_len);
            if (result == NULL)
            {
                LogError("Failure encoding Endorsement Key");
                result = NULL;
            }
            free(ek_value);
        }
    }
    return result;
}

BUFFER_HANDLE dps_hsm_custom_get_storage_key(DPS_SECURE_DEVICE_HANDLE handle)
{
    BUFFER_HANDLE result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        unsigned char* srk_value;
        size_t srk_len;
        HSM_SECURE_DEVICE_INFO* hsm_info = (HSM_SECURE_DEVICE_INFO*)handle;
        if (custom_hsm_get_storage_root_key(hsm_info->custom_handle, &srk_value, &srk_len) != 0)
        {
            LogError("Failure getting Storage Root key from custom hsm");
            result = NULL;
        }
        else
        {
            // Allocate and return the endorsement key
            result = BUFFER_create(srk_value, srk_len);
            if (result == NULL)
            {
                LogError("Failure encoding Storage Root Key");
                result = NULL;
            }
            free(srk_value);
        }
    }
    return result;
}

int dps_hsm_custom_import_key(DPS_SECURE_DEVICE_HANDLE handle, const unsigned char* key, size_t key_len)
{
    int result;
    if (handle == NULL || key == NULL || key_len == 0)
    {
        LogError("Invalid argument specified handle: %p, key: %p, key_len: %d", handle, key, key_len);
        result = __FAILURE__;
    }
    else
    {
        HSM_SECURE_DEVICE_INFO* hsm_info = (HSM_SECURE_DEVICE_INFO*)handle;
        if (custom_hsm_import_key(hsm_info->custom_handle, key, key_len) != 0)
        {
            LogError("Failure import Key from custom hsm");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

BUFFER_HANDLE dps_hsm_custom_sign_data(DPS_SECURE_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len)
{
    BUFFER_HANDLE result;
    if (handle == NULL || data == NULL || data_len == 0)
    {
        LogError("Invalid handle value specified handle: %p, data: %p", handle, data);
        result = NULL;
    }
    else
    {
        result = get_hsm_signed_key((HSM_SECURE_DEVICE_INFO*)handle, data, data_len);
        if (result == NULL)
        {
            LogError("Failure signing key from custom hsm");
            result = NULL;
        }
    }
    return result;
}

BUFFER_HANDLE iothub_hsm_custom_sign_data(SECURITY_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len)
{
    BUFFER_HANDLE result;
    if (handle == NULL || data == NULL || data_len == 0)
    {
        LogError("Invalid handle value specified handle: %p, data: %p", handle, data);
        result = NULL;
    }
    else
    {
        result = get_hsm_signed_key((HSM_SECURE_DEVICE_INFO*)handle, data, data_len);
        if (result == NULL)
        {
            LogError("Failure signing key from custom hsm");
            result = NULL;
        }
    }
    return result;
}

const SAS_SECURITY_INTERFACE* iothub_custom_hsm_sas_interface()
{
    return &sas_security_interface;
}

const X509_SECURITY_INTERFACE* iothub_custom_hsm_x509_interface()
{
    /* Codes_SRS_IOTHUB_SECURITY_x509_07_029: [ iothub_security_x509_interface shall return the X509_SECURITY_INTERFACE structure. ] */
    return &x509_security_interface;
}

const SEC_TPM_INTERFACE* dps_custom_hsm_tpm_interface()
{
    return &sec_tpm_interface;
}

const SEC_X509_INTERFACE* dps_custom_hsm_x509_interface()
{
    return &sec_x509_interface;
}
