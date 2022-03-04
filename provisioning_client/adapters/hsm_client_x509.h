// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HSM_CLIENT_X509_H
#define HSM_CLIENT_X509_H

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

MOCKABLE_FUNCTION(, HSM_CLIENT_HANDLE, hsm_client_x509_create);
MOCKABLE_FUNCTION(, void, hsm_client_x509_destroy, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_x509_get_certificate, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_x509_get_key, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_x509_get_common_name, HSM_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, int, hsm_client_x509_set_certificate, HSM_CLIENT_HANDLE, handle, const char *, certificate);
MOCKABLE_FUNCTION(, int, hsm_client_x509_set_key, HSM_CLIENT_HANDLE, handle, const char *, key);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HSM_CLIENT_X509_H
