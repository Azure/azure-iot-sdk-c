// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SECURE_DEVICE_FACTORY_H
#define SECURE_DEVICE_FACTORY_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/buffer_.h"

#define SECURE_DEVICE_TYPE_VALUES \
    SECURE_DEVICE_TYPE_UNKNOWN,   \
    SECURE_DEVICE_TYPE_TPM,       \
    SECURE_DEVICE_TYPE_X509

DEFINE_ENUM(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE_VALUES);

typedef struct DPS_SECURE_DEVICE_INFO_TAG* DPS_SECURE_DEVICE_HANDLE;

typedef DPS_SECURE_DEVICE_HANDLE (*DPS_SECURE_DEVICE_CREATE)();
typedef void (*DPS_SECURE_DEVICE_DESTROY)(DPS_SECURE_DEVICE_HANDLE handle);
typedef int (*DPS_SECURE_DEVICE_IMPORT_KEY)(DPS_SECURE_DEVICE_HANDLE handle, const unsigned char* key, size_t key_len);
typedef BUFFER_HANDLE (*DPS_SECURE_DEVICE_GET_EK)(DPS_SECURE_DEVICE_HANDLE handle);
typedef BUFFER_HANDLE (*DPS_SECURE_DEVICE_GET_SRK)(DPS_SECURE_DEVICE_HANDLE handle);
typedef BUFFER_HANDLE (*DPS_SECURE_DEVICE_SIGN_DATA)(DPS_SECURE_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len);

typedef char* (*DPS_SECURE_DEVICE_GET_CERTIFICATE)(DPS_SECURE_DEVICE_HANDLE handle);
typedef char* (*DPS_SECURE_DEVICE_GET_ALIAS_KEY)(DPS_SECURE_DEVICE_HANDLE handle);
typedef char* (*DPS_SECURE_DEVICE_GET_SIGNER_CERT)(DPS_SECURE_DEVICE_HANDLE handle);
typedef char* (*DPS_SECURE_DEVICE_GET_ROOT_CERT)(DPS_SECURE_DEVICE_HANDLE handle);
typedef char* (*DPS_SECURE_DEVICE_GET_COMMON_NAME)(DPS_SECURE_DEVICE_HANDLE handle);
typedef char* (*DPS_SECURE_DEVICE_GET_ROOT_KEY)(DPS_SECURE_DEVICE_HANDLE handle);

typedef struct SEC_TPM_INTERFACE_TAG
{
    DPS_SECURE_DEVICE_CREATE secure_device_create;
    DPS_SECURE_DEVICE_DESTROY secure_device_destroy;
    DPS_SECURE_DEVICE_IMPORT_KEY secure_device_import_key;
    DPS_SECURE_DEVICE_GET_EK secure_device_get_ek;
    DPS_SECURE_DEVICE_GET_SRK secure_device_get_srk;
    DPS_SECURE_DEVICE_SIGN_DATA secure_device_sign_data;
} SEC_TPM_INTERFACE;

typedef struct SEC_X509_INTERFACE_TAG
{
    DPS_SECURE_DEVICE_CREATE secure_device_create;
    DPS_SECURE_DEVICE_DESTROY secure_device_destroy;

    DPS_SECURE_DEVICE_GET_CERTIFICATE secure_device_get_cert;
    DPS_SECURE_DEVICE_GET_ALIAS_KEY secure_device_get_ak;
    DPS_SECURE_DEVICE_GET_SIGNER_CERT secure_device_get_signer_cert;
    DPS_SECURE_DEVICE_GET_ROOT_CERT secure_device_get_root_cert;
    DPS_SECURE_DEVICE_GET_ROOT_KEY secure_device_get_root_key;
    DPS_SECURE_DEVICE_GET_COMMON_NAME secure_device_get_common_name;
} SEC_X509_INTERFACE;

MOCKABLE_FUNCTION(, int, dps_secure_device_init);
MOCKABLE_FUNCTION(, void, dps_secure_device_deinit);

MOCKABLE_FUNCTION(, const void*, dps_secure_device_interface);
MOCKABLE_FUNCTION(, SECURE_DEVICE_TYPE, dps_secure_device_type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // SECURE_DEVICE_FACTORY_H