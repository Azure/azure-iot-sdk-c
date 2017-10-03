// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_SECURITY_FACTORY_H
#define IOTHUB_SECURITY_FACTORY_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/buffer_.h"

#define IOTHUB_SECURITY_TYPE_VALUES \
    IOTHUB_SECURITY_TYPE_UNKNOWN,   \
    IOTHUB_SECURITY_TYPE_SAS,       \
    IOTHUB_SECURITY_TYPE_X509

DEFINE_ENUM(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);

typedef struct SECURITY_DEVICE_INFO_TAG* SECURITY_DEVICE_HANDLE;

typedef SECURITY_DEVICE_HANDLE (*SECURE_DEVICE_CREATE)();
typedef void (*SECURE_DEVICE_DESTROY)(SECURITY_DEVICE_HANDLE handle);

typedef BUFFER_HANDLE (*SECURE_DEVICE_SIGN_DATA)(SECURITY_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len);

typedef const char* (*SECURE_DEVICE_GET_CERTIFICATE)(SECURITY_DEVICE_HANDLE handle);
typedef const char* (*SECURE_DEVICE_GET_ALIAS_KEY)(SECURITY_DEVICE_HANDLE handle);

typedef struct SAS_SECURITY_INTERFACE_TAG
{
    SECURE_DEVICE_CREATE secure_device_create;
    SECURE_DEVICE_DESTROY secure_device_destroy;

    SECURE_DEVICE_SIGN_DATA secure_device_sign_data;
} SAS_SECURITY_INTERFACE;

typedef struct X509_SECURITY_INTERFACE_TAG
{
    SECURE_DEVICE_CREATE secure_device_create;
    SECURE_DEVICE_DESTROY secure_device_destroy;

    SECURE_DEVICE_GET_CERTIFICATE secure_device_get_cert;
    SECURE_DEVICE_GET_ALIAS_KEY secure_device_get_ak;
} X509_SECURITY_INTERFACE;

MOCKABLE_FUNCTION(, int, iothub_security_init);
MOCKABLE_FUNCTION(, void, iothub_security_deinit);

MOCKABLE_FUNCTION(, const void*, iothub_security_interface);
MOCKABLE_FUNCTION(, IOTHUB_SECURITY_TYPE, iothub_security_type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // IOTHUB_SECURITY_FACTORY_H