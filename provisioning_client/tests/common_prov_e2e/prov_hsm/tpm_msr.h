// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TPM_MSR_H
#define TPM_MSR_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif /* __cplusplus */

typedef struct TPM_INFO_TAG* TPM_INFO_HANDLE;

extern TPM_INFO_HANDLE tpm_msr_create();
extern void tpm_msr_destroy(TPM_INFO_HANDLE handle);

extern int tpm_msr_get_ek(TPM_INFO_HANDLE handle, unsigned char* data_pos, size_t* key_len);
extern int tpm_msr_get_srk(TPM_INFO_HANDLE handle, unsigned char* data_pos, size_t* key_len);
extern int tpm_msr_import_key(TPM_INFO_HANDLE handle, const unsigned char* key, size_t key_len);
extern int tpm_msr_sign_data(TPM_INFO_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char* signed_value, size_t* signed_len);

#endif