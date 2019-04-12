// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HSM_CLIENT_HTTP_EDGE_H
#define HSM_CLIENT_HTTP_EDGE_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */

#include "umock_c/umock_c_prod.h"
#include "azure_macro_utils/macro_utils.h"
#include "hsm_client_data.h"

MOCKABLE_FUNCTION(, HSM_CLIENT_HANDLE, hsm_client_http_edge_create);
MOCKABLE_FUNCTION(, void, hsm_client_http_edge_destroy, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, hsm_client_http_edge_sign_data, HSM_CLIENT_HANDLE, handle, const unsigned char*, data, size_t, data_len, unsigned char**, signed_value, size_t*, signed_len);
MOCKABLE_FUNCTION(, char*, hsm_client_http_edge_get_trust_bundle, HSM_CLIENT_HANDLE, handle);

// NOTE: HSM_HTTP_EDGE_SIGNING_CONTEXT is in header file *only* for unit testing.  Other components should not consume directly.
typedef struct HSM_HTTP_WORKLOAD_CONTEXT_TAG
{
    bool continue_running;
    BUFFER_HANDLE http_response;
} HSM_HTTP_WORKLOAD_CONTEXT;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HSM_CLIENT_HTTP_EDGE_H
