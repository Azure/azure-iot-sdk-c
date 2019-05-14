// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "hsm_client_data.h"
#include "hsm_client_key.h"

typedef struct HSM_CLIENT_KEY_INFO_TAG
{
    char* symmetrical_key;
    char* registration_name;
} HSM_CLIENT_KEY_INFO;

HSM_CLIENT_HANDLE hsm_client_key_create(void)
{
    HSM_CLIENT_KEY_INFO* result;
    result = malloc(sizeof(HSM_CLIENT_KEY_INFO) );
    if (result == NULL)
    {
        LogError("Failure: malloc HSM_CLIENT_KEY_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(HSM_CLIENT_KEY_INFO));
    }
    return result;
}

void hsm_client_key_destroy(HSM_CLIENT_HANDLE handle)
{
    if (handle != NULL)
    {
        HSM_CLIENT_KEY_INFO* key_client = (HSM_CLIENT_KEY_INFO*)handle;
        if (key_client->symmetrical_key != NULL)
        {
            free(key_client->symmetrical_key);
            key_client->symmetrical_key = NULL;
        }
        if (key_client->registration_name != NULL)
        {
            free(key_client->registration_name);
            key_client->registration_name = NULL;
        }
        free(key_client);
    }
}

char* hsm_client_get_symmetric_key(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_HSM_CLIENT_RIOT_07_010: [ if handle is NULL, hsm_client_riot_get_certificate shall return NULL. ] */
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_KEY_INFO* key_client = (HSM_CLIENT_KEY_INFO*)handle;
        if (mallocAndStrcpy_s(&result, key_client->symmetrical_key) != 0)
        {
            LogError("Failed to allocate symmetrical_key.");
            result = NULL;
        }
    }
    return result;
}

char* hsm_client_get_registration_name(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_HSM_CLIENT_RIOT_07_026: [ if handle is NULL, hsm_client_riot_get_common_name shall return NULL. ] */
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_KEY_INFO* key_client = (HSM_CLIENT_KEY_INFO*)handle;
        if (mallocAndStrcpy_s(&result, key_client->registration_name) != 0)
        {
            LogError("Failure allocating registration_name");
            result = NULL;
        }
    }
    return result;
}

int hsm_client_set_key_info(HSM_CLIENT_HANDLE handle, const char* reg_name, const char* symm_key)
{
    int result;
    if (handle == NULL || reg_name == NULL || symm_key == NULL)
    {
        LogError("Invalid parameter specified handle: %p, reg_name: %p, symm_key: %p", handle, reg_name, symm_key);
        result = MU_FAILURE;
    }
    else
    {
        HSM_CLIENT_KEY_INFO* key_client = (HSM_CLIENT_KEY_INFO*)handle;

        char* temp_reg_name;
        char* temp_key;
        if (mallocAndStrcpy_s(&temp_reg_name, reg_name) != 0)
        {
            LogError("Failure allocating registration name");
            result = MU_FAILURE;
        }
        else if (mallocAndStrcpy_s(&temp_key, symm_key) != 0)
        {
            LogError("Failure allocating symmetric key");
            free(temp_reg_name);
            result = MU_FAILURE;
        }
        else
        {
            if (key_client->symmetrical_key != NULL)
            {
                free(key_client->symmetrical_key);
            }
            if (key_client->registration_name != NULL)
            {
                free(key_client->registration_name);
            }
            key_client->symmetrical_key = temp_key;
            key_client->registration_name = temp_reg_name;
            result = 0;
        }
    }
    return result;
}

static const HSM_CLIENT_KEY_INTERFACE key_interface =
{
    hsm_client_key_create,
    hsm_client_key_destroy,
    hsm_client_get_symmetric_key,
    hsm_client_get_registration_name,
    hsm_client_set_key_info
};

const HSM_CLIENT_KEY_INTERFACE* hsm_client_key_interface(void)
{
    return &key_interface;
}
