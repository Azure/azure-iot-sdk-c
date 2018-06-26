// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_c_shared_utility/xlogging.h"

#include "hsm_client_data.h"

static IOTHUB_SECURITY_TYPE g_security_type = IOTHUB_SECURITY_TYPE_UNKNOWN;

static SECURE_DEVICE_TYPE get_secure_device_type(IOTHUB_SECURITY_TYPE sec_type)
{
    SECURE_DEVICE_TYPE ret;

    switch (sec_type)
    {
        case IOTHUB_SECURITY_TYPE_SAS:
            ret = SECURE_DEVICE_TYPE_TPM;
            break;

        case IOTHUB_SECURITY_TYPE_X509:
            ret = SECURE_DEVICE_TYPE_X509;
            break;

        case IOTHUB_SECURITY_TYPE_HTTP_EDGE:
            ret = SECURE_DEVICE_TYPE_HTTP_EDGE;
            break;

        default:
            ret = SECURE_DEVICE_TYPE_UNKNOWN;
            break;
    }

    return ret;
}

int iothub_security_init(IOTHUB_SECURITY_TYPE sec_type)
{
    int result;
    g_security_type = sec_type;
    SECURE_DEVICE_TYPE device_type = prov_dev_security_get_type();
    if (device_type == SECURE_DEVICE_TYPE_UNKNOWN)
    {
        result = prov_dev_security_init(get_secure_device_type(g_security_type));
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
        else if (device_type == SECURE_DEVICE_TYPE_X509)
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
#ifdef HSM_TYPE_HTTP_EDGE
        else if (device_type == SECURE_DEVICE_TYPE_HTTP_EDGE)
        {
            if (g_security_type != IOTHUB_SECURITY_TYPE_HTTP_EDGE)
            {
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
        }
#endif
        else
        {
            result = __FAILURE__;
        }
    }
    if (result == 0)
    {
        result = initialize_hsm_system();
    }
    return result;
}

void iothub_security_deinit()
{
    deinitialize_hsm_system();
}

IOTHUB_SECURITY_TYPE iothub_security_type()
{
    return g_security_type;
}
