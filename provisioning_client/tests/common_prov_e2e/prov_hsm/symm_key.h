// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SYMM_KEY_OPENSSL_H
#define SYMM_KEY_OPENSSL_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif /* __cplusplus */

    typedef struct SYMM_KEY_INFO_TAG* SYMM_KEY_INFO_HANDLE;

    extern void initialize_symm_key();

    extern SYMM_KEY_INFO_HANDLE symm_key_info_create();
    extern void symm_key_info_destroy(SYMM_KEY_INFO_HANDLE handle);
    extern const char* symm_key_info_get_key(SYMM_KEY_INFO_HANDLE handle);
    extern const char* symm_key_info_get_reg_id(SYMM_KEY_INFO_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // SYMM_KEY_OPENSSL_H
