// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "symm_key.h"


typedef struct SYMM_KEY_INFO_TAG
{
    char symm_key[64];
    char reg_id[128];
} SYMM_KEY_INFO;

void initialize_symm_key(void)
{
    // Generate crypto key
}

SYMM_KEY_INFO_HANDLE symm_key_info_create(void)
{
    SYMM_KEY_INFO* result;
    result = malloc(sizeof(SYMM_KEY_INFO));
    if (result == NULL)
    {
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

// Set-once: the HSM_CLIENT_KEY_INTERFACE contract treats a symmetric-key HSM
// instance as immutable after its credentials are injected. prov_auth_create
// (and iothub_auth_create) call this exactly once, immediately after
// hsm_client_key_create. Any subsequent attempt to re-key an existing instance
// is a misuse: callers must destroy the instance and create a new one. This
// guard enforces that locally so the test HSM mirrors the production spec
// ("HSM should not allow setting the key").
int symm_key_info_set(SYMM_KEY_INFO_HANDLE handle, const char* reg_name, const char* symm_key)
{
    int result;
    if (handle == NULL || reg_name == NULL || symm_key == NULL)
    {
        LogError("Invalid parameter specified handle: %p, reg_name: %p, symm_key: %p", handle, reg_name, symm_key);
        result = __LINE__;
    }
    else if (handle->symm_key[0] != '\0' || handle->reg_id[0] != '\0')
    {
        LogError("Refusing to re-key an already-initialized HSM instance; destroy and recreate instead");
        result = __LINE__;
    }
    else if (strlen(symm_key) >= sizeof(handle->symm_key))
    {
        LogError("Symmetric key too long (%zu >= %zu)", strlen(symm_key), sizeof(handle->symm_key));
        result = __LINE__;
    }
    else if (strlen(reg_name) >= sizeof(handle->reg_id))
    {
        LogError("Registration ID too long (%zu >= %zu)", strlen(reg_name), sizeof(handle->reg_id));
        result = __LINE__;
    }
    else
    {
        strcpy(handle->symm_key, symm_key);
        strcpy(handle->reg_id, reg_name);
        result = 0;
    }
    return result;
}
