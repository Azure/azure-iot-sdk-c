// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CUSTOM_HSM_X509_IMPL_H
#define CUSTOM_HSM_X509_IMPL_H

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

// x509 Functions
extern char* custom_hsm_get_certificate(DPS_CUSTOM_HSM_HANDLE handle);
extern char* custom_hsm_get_alias_key(DPS_CUSTOM_HSM_HANDLE handle);
extern char* custom_hsm_get_common_name(DPS_CUSTOM_HSM_HANDLE handle);

extern char* custom_hsm_get_signer_cert(DPS_CUSTOM_HSM_HANDLE handle);
extern char* custom_hsm_get_root_cert(DPS_CUSTOM_HSM_HANDLE handle);
extern char* custom_hsm_get_root_key(DPS_CUSTOM_HSM_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CUSTOM_HSM_X509_IMPL_H
