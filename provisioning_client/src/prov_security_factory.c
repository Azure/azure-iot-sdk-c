// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_c_shared_utility/xlogging.h"

#include "hsm_client_data.h"

static SECURE_DEVICE_TYPE g_device_hsm_type = SECURE_DEVICE_TYPE_UNKNOWN;

int prov_dev_security_init(SECURE_DEVICE_TYPE hsm_type)
{
    int result;
    g_device_hsm_type = hsm_type;
    IOTHUB_SECURITY_TYPE security_type = iothub_security_type();
    if (security_type == IOTHUB_SECURITY_TYPE_UNKNOWN)
    {
        result = iothub_security_init(g_device_hsm_type == SECURE_DEVICE_TYPE_TPM ? IOTHUB_SECURITY_TYPE_SAS : IOTHUB_SECURITY_TYPE_X509);
    }
    else
    {
        if (security_type == IOTHUB_SECURITY_TYPE_SAS)
        {
            if (g_device_hsm_type != SECURE_DEVICE_TYPE_TPM)
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
            if (g_device_hsm_type != SECURE_DEVICE_TYPE_X509)
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
        result = initialize_hsm_system();
    }
    return result;
}

void prov_dev_security_deinit(void)
{
    deinitialize_hsm_system();
}

SECURE_DEVICE_TYPE prov_dev_security_get_type(void)
{
    return g_device_hsm_type;
}