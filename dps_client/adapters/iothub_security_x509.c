// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "iothub_security_x509.h"
#include "dps_hsm_riot.h"

typedef struct SECURITY_DEVICE_INFO_TAG
{
    char* x509_certificates;
    char* x509_private_key;
} SECURITY_DEVICE_INFO;

static const X509_SECURITY_INTERFACE x509_security_interface =
{
    iothub_security_x509_create,
    iothub_security_x509_destroy,
    iothub_security_x509_get_certificate,
    iothub_security_x509_get_alias_key
};

int initialize_x509_system(void)
{
    return initialize_riot_system();
}

void deinitialize_x509_system(void)
{
    deinitialize_riot_system();
}

SECURITY_DEVICE_HANDLE iothub_security_x509_create()
{
    SECURITY_DEVICE_INFO* result;
    /* Codes_SRS_iothub_security_x509_07_001: [ On success iothub_security_x509_create shall allocate a new instance of the device auth interface. ] */
    result = malloc(sizeof(SECURITY_DEVICE_INFO));
    if (result == NULL)
    {
        /* Codes_SRS_IOTHUB_SECURITY_x509_07_006: [ If any failure is encountered iothub_security_x509_create shall return NULL ] */
        LogError("Failure: malloc SAS_DEVICE_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(SECURITY_DEVICE_INFO));
        /* Codes_SRS_IOTHUB_SECURITY_x509_07_002: [ iothub_security_x509_create shall call into the dps RIoT module to retrieve the DPS_SECURE_DEVICE_HANDLE. ] */
        DPS_SECURE_DEVICE_HANDLE riot_handle = dps_hsm_riot_create();
        if (riot_handle == NULL)
        {
            /* Codes_SRS_IOTHUB_SECURITY_x509_07_006: [ If any failure is encountered iothub_security_x509_create shall return NULL ] */
            LogError("failure creating x509 system");
            free(result);
            result = NULL;
        }
        else
        {
            /* Codes_SRS_IOTHUB_SECURITY_x509_07_003: [ iothub_security_x509_create shall cache the x509_certificate from the RIoT module. ] */
            if ((result->x509_certificates = dps_hsm_riot_get_certificate(riot_handle)) == NULL)
            {
                /* Codes_SRS_IOTHUB_SECURITY_x509_07_006: [ If any failure is encountered iothub_security_x509_create shall return NULL ] */
                LogError("failure retrieving x509 certificate");
                free(result);
                result = NULL;
            }
            /* Codes_SRS_IOTHUB_SECURITY_x509_07_004: [ iothub_security_x509_create shall cache the x509_alias_key from the RIoT module. ] */
            else if ((result->x509_private_key = dps_hsm_riot_get_alias_key(riot_handle)) == NULL)
            {
                /* Codes_SRS_IOTHUB_SECURITY_x509_07_006: [ If any failure is encountered iothub_security_x509_create shall return NULL ] */
                LogError("failure retrieving x509 private key");
                free(result->x509_certificates);
                free(result);
                result = NULL;
            }
            dps_hsm_riot_destroy(riot_handle);
        }
    }
    return result;
}

void iothub_security_x509_destroy(SECURITY_DEVICE_HANDLE handle)
{
    /* Codes_SRS_IOTHUB_SECURITY_x509_07_007: [ if handle is NULL, iothub_security_x509_destroy shall do nothing. ] */
    if (handle != NULL)
    {
        /* Codes_SRS_IOTHUB_SECURITY_x509_07_009: [ iothub_security_x509_destroy shall free all resources allocated in this module. ] */
        free(handle->x509_certificates);
        free(handle->x509_private_key);
        free(handle);
    }
}

const char* iothub_security_x509_get_certificate(SECURITY_DEVICE_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_IOTHUB_SECURITY_x509_07_010: [ if handle is NULL, iothub_security_x509_get_certificate shall return NULL. ] */
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_IOTHUB_SECURITY_x509_07_011: [ iothub_security_x509_get_certificate shall return the cached riot certificate. ] */
        result = handle->x509_certificates;
    }
    return result;
}

const char* iothub_security_x509_get_alias_key(SECURITY_DEVICE_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_IOTHUB_SECURITY_x509_07_014: [ if handle is NULL, secure_device_riot_get_alias_key shall return NULL. ] */
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_IOTHUB_SECURITY_x509_07_015: [ secure_device_riot_get_alias_key shall allocate a char* to return the alias certificate. ] */
        result = handle->x509_private_key;
    }
    return result;
}

const X509_SECURITY_INTERFACE* iothub_security_x509_interface()
{
    /* Codes_SRS_IOTHUB_SECURITY_x509_07_029: [ iothub_security_x509_interface shall return the X509_SECURITY_INTERFACE structure. ] */
    return &x509_security_interface;
}
