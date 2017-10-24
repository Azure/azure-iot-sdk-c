// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CUSTOM_HSM_TPM_IMPL_H
#define CUSTOM_HSM_TPM_IMPL_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */

typedef void* DPS_CUSTOM_HSM_HANDLE;

extern DPS_CUSTOM_HSM_HANDLE custom_hsm_create();
extern void custom_hsm_destroy(DPS_CUSTOM_HSM_HANDLE handle);

extern int initialize_hsm_system();
extern void deinitialize_hsm_system();

extern int custom_hsm_get_endorsement_key(DPS_CUSTOM_HSM_HANDLE handle, unsigned char** key, size_t* key_len);
extern int custom_hsm_get_storage_root_key(DPS_CUSTOM_HSM_HANDLE handle, unsigned char** key, size_t* key_len);
extern int custom_hsm_import_key(DPS_CUSTOM_HSM_HANDLE handle, const unsigned char* key, size_t key_len);
extern int custom_hsm_sign_key(DPS_CUSTOM_HSM_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char** signed_value, size_t* signed_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CUSTOM_HSM_TPM_IMPL_H
