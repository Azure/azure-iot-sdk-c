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
#include "azure_hub_modules/iothub_security_factory.h"

MOCKABLE_FUNCTION(, SECURITY_DEVICE_HANDLE, iothub_security_sas_create);
MOCKABLE_FUNCTION(, void, iothub_security_sas_destroy, SECURITY_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, BUFFER_HANDLE, iothub_security_sas_sign_data, SECURITY_DEVICE_HANDLE, handle, const unsigned char*, data, size_t, data_len);

MOCKABLE_FUNCTION(, const SAS_SECURITY_INTERFACE*, iothub_security_sas_interface);
MOCKABLE_FUNCTION(, int, initialize_sas_system);
MOCKABLE_FUNCTION(, void, deinitialize_sas_system);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DEVICE_AUTH
