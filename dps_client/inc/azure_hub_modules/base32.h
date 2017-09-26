// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BASE32_H
#define BASE32_H

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

#include "azure_c_shared_utility/umock_c_prod.h"

/**
* @brief Encodes the BUFFER_HANDLE to a base 32 STRING_HANDLE
*
* @param    input   BUFFER_HANDLE of the unsigned char* to be encoded
*
* @return   A base32 encoded STRING_HANDLE that will need to be deallocated
*/
MOCKABLE_FUNCTION(, STRING_HANDLE, Base32_Encode, BUFFER_HANDLE, input);

/**
* @brief    Encodes the BUFFER_HANDLE to a base 32 char*
*
* @param    source   An unsigned char* to be encoded
* @param    size     The lenght in bytes of the source variable
*
* @return   A base32 encoded string that will need to be deallocated
*/
MOCKABLE_FUNCTION(, char*, Base32_Encode_Bytes, const unsigned char*, source, size_t, size);

/**
* @brief    Decodes a base32 encoded STRING_HANDLE to a BUFFER_HANDLE
*
* @param    handle   STRING_HANDLE of a base32 encode string
*
* @return   A BUFFER_HANDLE of the result of decoding the handle
*/
MOCKABLE_FUNCTION(, BUFFER_HANDLE, Base32_Decode, STRING_HANDLE, handle);

/**
* @brief    Decodes a base32 encoded char* to a BUFFER_HANDLE
*
* @param    source   char* of a base32 encode string
*
* @return   A BUFFER_HANDLE of the result of decoding the source
*/
MOCKABLE_FUNCTION(, BUFFER_HANDLE, Base32_Decode_String, const char*, source);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_H */
