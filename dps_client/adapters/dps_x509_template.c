// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "dps_x509_template.h"


typedef struct DPS_SECURE_DEVICE_INFO_TAG
{
    // Add the 
    int module_variables;
} DPS_SECURE_DEVICE_INFO;

static const SEC_X509_INTERFACE sec_x509_interface =
{
    secure_device_x509_create,
    secure_device_x509_destroy,
    secure_device_x509_get_certificate,
    secure_device_x509_get_alias_key,
    secure_device_x509_get_signer_cert,
    secure_device_x509_get_common_name
};


int initialize_x509_system(void)
{
    // Initialize x509 system if neccessary
    return 0;
}

void deinitialize_x509_system(void)
{
    // Deinitialize x509 system if neccessary
}

DPS_SECURE_DEVICE_HANDLE secure_device_x509_create()
{
    DPS_SECURE_DEVICE_INFO* result;
    result = malloc(sizeof(DPS_SECURE_DEVICE_INFO) );
    if (result == NULL)
    {
        LogError("Failure: malloc DPS_SECURE_DEVICE_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(DPS_SECURE_DEVICE_INFO));
    }
    return result;
}

void secure_device_x509_destroy(DPS_SECURE_DEVICE_HANDLE handle)
{
    if (handle != NULL)
    {
        free(handle);
    }
}

char* secure_device_x509_get_certificate(DPS_SECURE_DEVICE_HANDLE handle)
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
        result = NULL;
    }
    return result;
}

char* secure_device_x509_get_alias_key(DPS_SECURE_DEVICE_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = NULL;
    }
    return result;
}


char* secure_device_x509_get_signer_cert(DPS_SECURE_DEVICE_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = NULL;
    }
    return result;
}

char* secure_device_x509_get_common_name(DPS_SECURE_DEVICE_HANDLE handle)
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
        result = NULL;
    }
    return result;
}

const SEC_X509_INTERFACE* secure_device_x509_interface()
{
    return &sec_x509_interface;
}
