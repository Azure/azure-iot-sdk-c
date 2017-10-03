// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_HSM_RIOT_H
#define DPS_HSM_RIOT_H

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

#include "azure_hub_modules/secure_device_factory.h"

MOCKABLE_FUNCTION(, DPS_SECURE_DEVICE_HANDLE, dps_hsm_riot_create);
MOCKABLE_FUNCTION(, void, dps_hsm_riot_destroy, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_certificate, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_alias_key, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_signer_cert, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_root_cert, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_root_key, DPS_SECURE_DEVICE_HANDLE, handle);

MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_common_name, DPS_SECURE_DEVICE_HANDLE, handle);

MOCKABLE_FUNCTION(, const SEC_X509_INTERFACE*, dps_hsm_x509_interface);
MOCKABLE_FUNCTION(, int, initialize_riot_system);
MOCKABLE_FUNCTION(, void, deinitialize_riot_system);

MOCKABLE_FUNCTION(, char*, dps_hsm_riot_create_leaf_cert, DPS_SECURE_DEVICE_HANDLE, handle, const char*, common_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DPS_HSM_RIOT
