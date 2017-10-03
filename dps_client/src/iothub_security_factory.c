// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_hub_modules/iothub_security_factory.h"

#ifdef DPS_X509_TYPE
    #ifdef DPS_CUSTOM_HSM
        #include "custom_hsm_abstract.h"
    #else
        #include "iothub_security_x509.h"
    #endif
#else
    #ifdef DPS_CUSTOM_HSM
        #include "custom_hsm_abstract.h"
    #else
        #include "iothub_security_sas.h"
    #endif
#endif

int iothub_security_init()
{
    int result;
#ifdef DPS_X509_TYPE
    #ifdef DPS_CUSTOM_HSM
        result = initialize_custom_hsm();
    #else
        result = initialize_x509_system();
    #endif
#else
    #ifdef DPS_CUSTOM_HSM
        result = initialize_custom_hsm();
    #else
        result = initialize_sas_system();
    #endif
#endif
    return result;
}

void iothub_security_deinit()
{
#ifdef DPS_X509_TYPE
    #ifdef DPS_CUSTOM_HSM
        deinitialize_custom_hsm();
    #else
        deinitialize_x509_system();
    #endif
#else
    #ifdef DPS_CUSTOM_HSM
        deinitialize_custom_hsm();
    #else
        deinitialize_sas_system();
    #endif
#endif
}

const void* iothub_security_interface()
{
#ifdef DPS_X509_TYPE
    #ifdef DPS_CUSTOM_HSM
        return iothub_custom_hsm_x509_interface();
    #else
        return iothub_security_x509_interface();
    #endif
#else
    #ifdef DPS_CUSTOM_HSM
        return iothub_custom_hsm_sas_interface();
    #else
        return iothub_security_sas_interface();
    #endif
#endif
}

IOTHUB_SECURITY_TYPE iothub_security_type()
{
#ifdef DPS_X509_TYPE
    return IOTHUB_SECURITY_TYPE_X509;
#else
    return IOTHUB_SECURITY_TYPE_SAS;
#endif
}