// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_c_shared_utility/xlogging.h"

#include "hsm_client_tpm_abstract.h"
#include "hsm_client_x509_abstract.h"

static IOTHUB_SECURITY_TYPE g_security_type = IOTHUB_SECURITY_TYPE_UNKNOWN;

int iothub_security_init(IOTHUB_SECURITY_TYPE sec_type)
{
    int result;
    g_security_type = sec_type;
    SECURE_DEVICE_TYPE device_type = prov_dev_security_get_type();
    if (device_type == SECURE_DEVICE_TYPE_UNKNOWN)
    {
        result = prov_dev_security_init(g_security_type == IOTHUB_SECURITY_TYPE_SAS ? SECURE_DEVICE_TYPE_TPM : SECURE_DEVICE_TYPE_X509);
    }
    else
    {
        // Make sure that the types are compatible
        if (device_type == SECURE_DEVICE_TYPE_TPM)
        {
            if (g_security_type != IOTHUB_SECURITY_TYPE_SAS)
            {
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
        }
        else
        {
            if (g_security_type != IOTHUB_SECURITY_TYPE_X509)
            {
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
        }
    }
    if (result == 0)
    {
        if (g_security_type == IOTHUB_SECURITY_TYPE_X509)
        {
            result = hsm_init_x509_system();
        }
        else
        {
            result = hsm_client_tpm_init();
        }
    }
    return result;
}

void iothub_security_deinit()
{
    if (g_security_type == IOTHUB_SECURITY_TYPE_X509)
    {
        hsm_deinit_x509_system();
    }
    else
    {
        hsm_client_tpm_deinit();
    }
}

IOTHUB_SECURITY_TYPE iothub_security_type()
{
    return g_security_type;
}