// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HSM_CLIENT_TPM_H
#define HSM_CLIENT_TPM_H

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

MOCKABLE_FUNCTION(, HSM_CLIENT_HANDLE, hsm_client_tpm_create);
MOCKABLE_FUNCTION(, void, hsm_client_tpm_destroy, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, hsm_client_tpm_get_endorsement_key, HSM_CLIENT_HANDLE, handle, unsigned char**, key, size_t*, key_len);
MOCKABLE_FUNCTION(, int, hsm_client_tpm_get_storage_key, HSM_CLIENT_HANDLE, handle, unsigned char**, key, size_t*, key_len);
MOCKABLE_FUNCTION(, int, hsm_client_tpm_import_key, HSM_CLIENT_HANDLE, handle, const unsigned char*, key, size_t, key_len);
MOCKABLE_FUNCTION(, int, hsm_client_tpm_sign_data, HSM_CLIENT_HANDLE, handle, const unsigned char*, data, size_t, data_len, unsigned char**, signed_value, size_t*, signed_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HSM_CLIENT_TPM_H
