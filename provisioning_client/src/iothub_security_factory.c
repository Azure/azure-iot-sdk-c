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
#if defined(HSM_TYPE_SAS_TOKEN)  || defined(HSM_AUTH_TYPE_CUSTOM)
        case IOTHUB_SECURITY_TYPE_SAS:
            ret = SECURE_DEVICE_TYPE_TPM;
            break;
#endif

#if defined(HSM_TYPE_X509) || defined(HSM_AUTH_TYPE_CUSTOM)
        case IOTHUB_SECURITY_TYPE_X509:
            ret = SECURE_DEVICE_TYPE_X509;
            break;
#endif

#ifdef HSM_TYPE_HTTP_EDGE
        case IOTHUB_SECURITY_TYPE_HTTP_EDGE:
            ret = SECURE_DEVICE_TYPE_HTTP_EDGE;
            break;
#endif

        default:
            ret = SECURE_DEVICE_TYPE_UNKNOWN;
            break;
    }

    return ret;
}

int iothub_security_init(IOTHUB_SECURITY_TYPE sec_type)
{
    int result;

    SECURE_DEVICE_TYPE secure_device_type_from_caller = get_secure_device_type(sec_type);

    if (secure_device_type_from_caller == SECURE_DEVICE_TYPE_UNKNOWN)
    {
        LogError("Security type %d is not supported in this SDK build", sec_type);
        result = __FAILURE__;
    }
    else
    {
        g_security_type = sec_type;
        SECURE_DEVICE_TYPE security_device_type_from_prov = prov_dev_security_get_type();
        if (security_device_type_from_prov == SECURE_DEVICE_TYPE_UNKNOWN)
        {
            result = prov_dev_security_init(secure_device_type_from_caller);
        }
        else if (secure_device_type_from_caller != security_device_type_from_prov)
        {
            LogError("Security type from caller %d (which maps to security device type %d) does not match already specified security device type %d", sec_type, secure_device_type_from_caller, security_device_type_from_prov);
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }

        if (result == 0)
        {
            result = initialize_hsm_system();
        }
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
