// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_SECURITY_X509_H
#define IOTHUB_SECURITY_X509_H

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

#include "azure_hub_modules/iothub_security_factory.h"

MOCKABLE_FUNCTION(, SECURITY_DEVICE_HANDLE, iothub_security_x509_create);
MOCKABLE_FUNCTION(, void, iothub_security_x509_destroy, SECURITY_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, const char*, iothub_security_x509_get_certificate, SECURITY_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, const char*, iothub_security_x509_get_alias_key, SECURITY_DEVICE_HANDLE, handle);

MOCKABLE_FUNCTION(, const X509_SECURITY_INTERFACE*, iothub_security_x509_interface);
MOCKABLE_FUNCTION(, int, initialize_x509_system);
MOCKABLE_FUNCTION(, void, deinitialize_x509_system);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // IOTHUB_SECURITY_X509_H
