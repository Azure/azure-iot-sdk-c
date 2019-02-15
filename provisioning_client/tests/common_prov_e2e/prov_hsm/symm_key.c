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
    char* symm_key;
    char* reg_id;
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
        free(handle->reg_id);
        free(handle->symm_key);
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

int symm_key_set_symm_key(SYMM_KEY_INFO_HANDLE handle, const char* reg_name, const char* symm_key)
{
    int result;
    if (handle == NULL)
    {
        (void)printf("Invalid handle value specified\r\n");
        result = __LINE__;
    }
    else
    {
        // TODO: Malloc the symmetric key for the iothub 
        // The SDK will call free() this value
        size_t reg_len = strlen(reg_name);
        size_t symm_len = strlen(symm_key);
        if ((handle->reg_id = (char*)malloc(reg_len + 1)) == NULL)
        {
            (void)printf("Failure allocating registration name\r\n");
            result = __LINE__;
        }
        else if ((handle->symm_key = (char*)malloc(symm_len + 1)) == NULL)
        {
            (void)printf("Failure allocating symm key\r\n");
            free(handle->reg_id);
            result = __LINE__;
        }
        else
        {
            strcpy(handle->reg_id, reg_name);
            strcpy(handle->symm_key, symm_key);
            result = 0;
        }
    }
    return result;
}
