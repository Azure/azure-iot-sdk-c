// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROV_TRANSPORT_UTIL_H
#define PROV_TRANSPORT_UTIL_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#else
#include <stdint.h>
#endif /* __cplusplus */

uint32_t parse_retry_after_value(const char* retry_after);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROV_TRANSPORT_UTIL
