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

MOCKABLE_FUNCTION(, STRING_HANDLE, Base32_Encoder, BUFFER_HANDLE, input);
MOCKABLE_FUNCTION(, char*, Base32_Encode_Bytes, const unsigned char*, source, size_t, size);
MOCKABLE_FUNCTION(, BUFFER_HANDLE, Base32_Decoder, const char*, source);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_H */
