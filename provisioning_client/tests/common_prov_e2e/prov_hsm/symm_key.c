// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/gb_rand.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "symm_key.h"


typedef struct SYMM_KEY_INFO_TAG
{
    char symm_key[64];
    char reg_id[128];
} SYMM_KEY_INFO;

void initialize_symm_key()
{
    // Generate crypto key
}

SYMM_KEY_INFO_HANDLE symm_key_info_create()
{
    SYMM_KEY_INFO* result;
    /* Codes_SRS_HSM_CLIENT_TPM_07_002: [ On success hsm_client_tpm_create shall allocate a new instance of the secure device tpm interface. ] */
    result = malloc(sizeof(SYMM_KEY_INFO));
    if (result == NULL)
    {
        /* Codes_SRS_HSM_CLIENT_TPM_07_001: [ If any failure is encountered hsm_client_tpm_create shall return NULL ] */
        printf("Failure: malloc SYMM_KEY_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(SYMM_KEY_INFO));
    }
    return result;
}

void symm_key_info_destroy(SYMM_KEY_INFO_HANDLE handle)
{
    if (handle != NULL)
    {
        free(handle);
    }
}

const char* symm_key_info_get_key(SYMM_KEY_INFO_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = handle->symm_key;
    }
    return result;
}

const char* symm_key_info_get_reg_id(SYMM_KEY_INFO_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = handle->reg_id;
    }
    return result;
}
