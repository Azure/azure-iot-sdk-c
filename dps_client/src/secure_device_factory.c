// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_hub_modules/secure_device_factory.h"

#ifdef DPS_X509_TYPE
    #ifdef DPS_CUSTOM_HSM
        #include "custom_hsm_abstract.h"
    #else
        #include "dps_hsm_riot.h"
    #endif
#else
    #ifdef DPS_CUSTOM_HSM
        #include "custom_hsm_abstract.h"
    #else
        #include "dps_hsm_tpm.h"
    #endif
#endif

int dps_secure_device_init(void)
{
    int result;
#ifdef DPS_X509_TYPE
    #ifdef DPS_CUSTOM_HSM
        result = initialize_custom_hsm();
    #else
        result = initialize_riot_system();
    #endif
#else
    #ifdef DPS_CUSTOM_HSM
        result = initialize_custom_hsm();
    #else
        result = initialize_tpm_system();
    #endif
#endif
    return result;
}

void dps_secure_device_deinit(void)
{
#ifdef DPS_X509_TYPE
    #ifdef DPS_CUSTOM_HSM
        deinitialize_custom_hsm();
    #else
        deinitialize_riot_system();
    #endif
#else
    #ifdef DPS_CUSTOM_HSM
        deinitialize_custom_hsm();
    #else
        deinitialize_tpm_system();
    #endif
#endif
}

const void* dps_secure_device_interface(void)
{
#ifdef DPS_X509_TYPE
    #ifdef DPS_CUSTOM_HSM
        return dps_custom_hsm_x509_interface();
    #else
        return dps_hsm_x509_interface();
    #endif
#else
    #ifdef DPS_CUSTOM_HSM
        return dps_custom_hsm_tpm_interface();
    #else
        return dps_hsm_tpm_interface();
    #endif
#endif
}

SECURE_DEVICE_TYPE dps_secure_device_type(void)
{
#ifdef DPS_X509_TYPE
    return SECURE_DEVICE_TYPE_X509;
#else
    return SECURE_DEVICE_TYPE_TPM;
#endif
}