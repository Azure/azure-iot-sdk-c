// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DEVICE_AUTH_H
#define DEVICE_AUTH_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_hub_modules/secure_device_factory.h"

MOCKABLE_FUNCTION(, DPS_SECURE_DEVICE_HANDLE, dps_hsm_tpm_create);
MOCKABLE_FUNCTION(, void, dps_hsm_tpm_destroy, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, BUFFER_HANDLE, dps_hsm_tpm_get_endorsement_key, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, BUFFER_HANDLE, dps_hsm_tpm_get_storage_key, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_hsm_tpm_import_key, DPS_SECURE_DEVICE_HANDLE, handle, const unsigned char*, key, size_t, key_len);
MOCKABLE_FUNCTION(, BUFFER_HANDLE, dps_hsm_tpm_sign_data, DPS_SECURE_DEVICE_HANDLE, handle, const unsigned char*, data, size_t, data_len);

MOCKABLE_FUNCTION(, const SEC_TPM_INTERFACE*, dps_hsm_tpm_interface);
MOCKABLE_FUNCTION(, int, initialize_tpm_system);
MOCKABLE_FUNCTION(, void, deinitialize_tpm_system);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DEVICE_AUTH
