// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_c_shared_utility/xlogging.h"

#include "hsm_client_data.h"

static SECURE_DEVICE_TYPE g_device_hsm_type = SECURE_DEVICE_TYPE_UNKNOWN;

static IOTHUB_SECURITY_TYPE get_iothub_security_type(SECURE_DEVICE_TYPE sec_type)
{
    IOTHUB_SECURITY_TYPE ret;

    switch (sec_type)
    {
#if defined(HSM_TYPE_SAS_TOKEN)  || defined(HSM_AUTH_TYPE_CUSTOM)
        case SECURE_DEVICE_TYPE_TPM:
            ret = IOTHUB_SECURITY_TYPE_SAS;
            break;
#endif

#if defined(HSM_TYPE_X509) || defined(HSM_AUTH_TYPE_CUSTOM)
        case SECURE_DEVICE_TYPE_X509:
            ret = IOTHUB_SECURITY_TYPE_X509;
            break;
#endif

#ifdef HSM_TYPE_HTTP_EDGE
        case SECURE_DEVICE_TYPE_HTTP_EDGE :
            ret = IOTHUB_SECURITY_TYPE_HTTP_EDGE;
            break;
#endif

        default:
            ret = IOTHUB_SECURITY_TYPE_UNKNOWN;
            break;
    }

    return ret;
}

int prov_dev_security_init(SECURE_DEVICE_TYPE hsm_type)
{
    int result;

    IOTHUB_SECURITY_TYPE security_type_from_caller = get_iothub_security_type(hsm_type);

    if (security_type_from_caller == IOTHUB_SECURITY_TYPE_UNKNOWN)
    {
        LogError("HSM type %d is not supported on this SDK build", hsm_type);
        result = __FAILURE__;
    }
    else
    {
        g_device_hsm_type = hsm_type;
        IOTHUB_SECURITY_TYPE security_type_from_iot = iothub_security_type();
        if (security_type_from_iot == IOTHUB_SECURITY_TYPE_UNKNOWN)
        {
            // Initialize iothub_security layer if not currently
            result = iothub_security_init(security_type_from_caller);
        }
        else if (security_type_from_iot != security_type_from_caller)
        {
            LogError("Security HSM from caller %d (which maps to security type %d) does not match already specified security type %d", hsm_type, security_type_from_caller, security_type_from_iot);
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

void prov_dev_security_deinit(void)
{
    deinitialize_hsm_system();
}

SECURE_DEVICE_TYPE prov_dev_security_get_type(void)
{
    return g_device_hsm_type;
}
