// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "hsm_client_x509.h"
#include "hsm_client_data.h"

typedef struct HSM_CLIENT_X509_INFO_TAG
{
    const char* x509certificate;
    const char* x509key;
} HSM_CLIENT_X509_INFO;

static HSM_CLIENT_X509_INFO hsm_singleton;

HSM_CLIENT_HANDLE hsm_client_x509_create(void)
{
    HSM_CLIENT_X509_INFO* result;
    result = &hsm_singleton;
    return result;
}

void hsm_client_x509_destroy(HSM_CLIENT_HANDLE handle)
{
    (void)handle;
    // No-op.
}

int hsm_client_x509_set_certificate(HSM_CLIENT_HANDLE handle, const char* certificate)
{
    int result;
    if (handle == NULL || certificate == NULL)
    {
        LogError("Invalid parameter handle: %p, certificate: %p", handle, certificate);
        result = MU_FAILURE;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;
        if (x509_client->x509certificate != NULL)
        {
            LogError("Certificate has been previously set and cannot be changed");
            result = MU_FAILURE;
        }
        else if (mallocAndStrcpy_s((char **)&(x509_client->x509certificate), certificate) != 0)
        {
            LogError("Failed allocating certificate key");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

int hsm_client_x509_set_key(HSM_CLIENT_HANDLE handle, const char* key)
{
    int result;
    if (handle == NULL || key == NULL)
    {
        LogError("Invalid parameter handle: %p, key: %p", handle, key);
        result = MU_FAILURE;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;
        if (x509_client->x509key != NULL)
        {
            LogError("Certificate key has been previously set and cannot be changed");
            result = MU_FAILURE;
        }
        else if (mallocAndStrcpy_s((char **)&x509_client->x509key, key) != 0)
        {
            LogError("Failed allocating cert key");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

char* hsm_client_x509_get_certificate(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid client handle value specified.");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        if (x509_client->x509certificate == NULL)
        {
            LogError("Client X509 certificate was not configured.");
            result = NULL;
        }
        else
        {
            // Create a copy. Lifetime is managed by the DPS or Hub clients.
            if (mallocAndStrcpy_s(&result, x509_client->x509certificate) != 0)
            {
                LogError("Failure allocating x509certificate.");
                result = NULL;
            }
        }
    }

    return result;
}

char* hsm_client_x509_get_key(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid client handle value specified.");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        if (x509_client->x509key == NULL)
        {
            LogError("Client X509 key was not configured.");
            result = NULL;
        }
        else
        {
            // Create a copy. Lifetime is managed by the DPS or Hub clients.
            if (mallocAndStrcpy_s(&result, x509_client->x509key) != 0)
            {
                LogError("Failure allocating x509 key.");
                result = NULL;
            }
        }
    }
    
    return result;
}

char* hsm_client_x509_get_common_name(HSM_CLIENT_HANDLE handle)
{
    (void)handle;
    // Provisioning Device Client should never call this function.
    LogError("Registration ID was not configured.");
    return NULL;
}

static const HSM_CLIENT_X509_INTERFACE x509_interface =
{
    hsm_client_x509_create,
    hsm_client_x509_destroy,
    hsm_client_x509_get_certificate,
    hsm_client_x509_get_key,
    hsm_client_x509_get_common_name
};

const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface(void)
{
    return &x509_interface;
}

int hsm_client_x509_init(void)
{
    memset(&hsm_singleton, 0, sizeof(HSM_CLIENT_X509_INFO));
    return 0;
}

void hsm_client_x509_deinit(void)
{
    if (hsm_singleton.x509certificate != NULL)
    {
        free((char*)hsm_singleton.x509certificate);
        hsm_singleton.x509certificate = NULL;
    }   

    if (hsm_singleton.x509key != NULL)
    {
        free((char*)hsm_singleton.x509key);
        hsm_singleton.x509key = NULL;
    }
}
