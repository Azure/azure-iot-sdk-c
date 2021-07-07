// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_c_shared_utility/xlogging.h"
#include "hsm_client_data.h"

#if defined(HSM_TYPE_SAS_TOKEN)  || defined(HSM_AUTH_TYPE_CUSTOM)
#include "hsm_client_tpm.h"
#endif

#if defined(HSM_TYPE_X509) || defined(HSM_AUTH_TYPE_CUSTOM)
#include "hsm_client_riot.h"
#endif

#if defined(HSM_TYPE_SYMM_KEY) || defined(HSM_AUTH_TYPE_CUSTOM)
#include "hsm_client_key.h"
#endif

int initialize_hsm_system(void)
{
    int result = 0;
#if defined(HSM_TYPE_X509) || defined(HSM_AUTH_TYPE_CUSTOM)
    // Initialize x509
    if ((result == 0) && (hsm_client_x509_init() != 0))
    {
        LogError("Failure initializing x509 system");
        result = __LINE__;
    }
#endif
#if defined(HSM_TYPE_SAS_TOKEN)  || defined(HSM_AUTH_TYPE_CUSTOM)
    if ((result == 0) && (hsm_client_tpm_init() != 0))
    {
        LogError("Failure initializing tpm system");
        result = __LINE__;
    }
#endif
#ifdef HSM_TYPE_HTTP_EDGE
    if ((result == 0) && (hsm_client_http_edge_init() != 0))
    {
        LogError("Failure initializing http edge system");
        result = __LINE__;
    }
#endif

    return result;
}

void deinitialize_hsm_system(void)
{
#ifdef HSM_TYPE_HTTP_EDGE
    hsm_client_http_edge_deinit();
#endif
#if defined(HSM_TYPE_X509) || defined(HSM_AUTH_TYPE_CUSTOM)
    hsm_client_x509_deinit();
#endif
#if defined(HSM_TYPE_SAS_TOKEN)  || defined(HSM_AUTH_TYPE_CUSTOM)
    hsm_client_tpm_deinit();
#endif
}
