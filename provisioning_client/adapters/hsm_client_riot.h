// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HSM_CLIENT_RIOT_H
#define HSM_CLIENT_RIOT_H

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

MOCKABLE_FUNCTION(, HSM_CLIENT_HANDLE, hsm_client_riot_create);
MOCKABLE_FUNCTION(, void, hsm_client_riot_destroy, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_riot_get_certificate, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_riot_get_alias_key, HSM_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, char*, hsm_client_riot_get_signer_cert, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_riot_get_root_cert, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_riot_get_root_key, HSM_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, char*, hsm_client_riot_get_common_name, HSM_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, char*, hsm_client_riot_create_leaf_cert, HSM_CLIENT_HANDLE, handle, const char*, common_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HSM_CLIENT_RIOT
