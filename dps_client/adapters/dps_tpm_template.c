// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/buffer_.h"

#include "azure_hub_modules/secure_device_factory.h"
#include "azure_hub_modules/secure_device_tpm.h"

typedef struct DPS_SECURE_DEVICE_INFO_TAG
{
    int dev_info;
} DPS_SECURE_DEVICE_INFO;

static const SEC_TPM_INTERFACE sec_tpm_interface =
{
    dps_tpm_create,
    dps_tpm_destroy,
    dps_tpm_import_key,
    dps_tpm_get_endorsement_key,
    dps_tpm_get_storage_key,
    dps_tpm_sign_data,
};

DPS_SECURE_DEVICE_HANDLE dps_tpm_create()
{
    SEC_DEVICE_INFO* result;
    result = malloc(sizeof(SEC_DEVICE_INFO) );
    if (result == NULL)
    {
        LogError("Failure: malloc SEC_DEVICE_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(SEC_DEVICE_INFO));

        // Initialize SEC_DEVICE_INFO function with anything
        // That is necessary
    }
    return (DPS_SECURE_DEVICE_HANDLE)result;
}

void dps_tpm_destroy(DPS_SECURE_DEVICE_HANDLE handle)
{
    if (handle != NULL)
    {
        free(handle); 
    }
}

BUFFER_HANDLE dps_tpm_get_endorsement_key(DPS_SECURE_DEVICE_HANDLE handle)
{
    BUFFER_HANDLE result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        // Allocate and return the endorsement key
        result = BUFFER_create(0x12, 1);
        if (result == NULL)
        {
            LogError("Failure encoding Endorsement Key");
            result = NULL;
        }
    }
    return result;
}

BUFFER_HANDLE dps_tpm_get_storage_key(DPS_SECURE_DEVICE_HANDLE handle)
{
    BUFFER_HANDLE result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        // Allocate and return the storage root key
        result = BUFFER_create(0x12, 1);
        if (result == NULL)
        {
            LogError("Failure encoding storage root key");
            result = NULL;
        }
    }
    return result;
}

int dps_tpm_import_key(DPS_SECURE_DEVICE_HANDLE handle, const unsigned char* key, size_t key_len)
{
    int result;
    if (handle == NULL || key == NULL || key_len == 0)
    {
        LogError("Invalid argument specified handle: %p, key: %p, key_len: %d", handle, key, key_len);
        result = __FAILURE__;
    }
    else
    {
        // Import the key into the HSM for later hashing of data
    }
    return result;
}

BUFFER_HANDLE dps_tpm_sign_data(DPS_SECURE_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len)
{
    BUFFER_HANDLE result;
    if (handle == NULL || data == NULL || data_len == 0)
    {
        LogError("Invalid handle value specified handle: %p, data: %p", handle, data);
        result = NULL;
    }
    else
    {
        BYTE data_signature[1024] = { 0 };
        data_signature[0] = 0x12;
        size_t sign_len = 1;

        // Hash the data and return it it in a BUFFER_HANDLE
        result = BUFFER_create(data_signature, sign_len);
        if (result == NULL)
        {
            LogError("Failure allocating sign data");
        }
    }
    return result;
}

const SEC_TPM_INTERFACE* dps_tpm_interface()
{
    return &sec_tpm_interface;
}
