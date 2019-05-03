// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HSM_CLIENT_KEY_H
#define HSM_CLIENT_KEY_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "umock_c/umock_c_prod.h"
#include "hsm_client_data.h"

MOCKABLE_FUNCTION(, HSM_CLIENT_HANDLE, hsm_client_key_create);
MOCKABLE_FUNCTION(, void, hsm_client_key_destroy, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_get_symmetric_key, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_get_registration_name, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, hsm_client_set_key_info, HSM_CLIENT_HANDLE, handle, const char*, reg_name, const char*, symm_key);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HSM_CLIENT_RIOT
