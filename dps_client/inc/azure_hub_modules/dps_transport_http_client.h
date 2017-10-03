// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_TRANSPORT_HTTP_CLIENT_H
#define DPS_TRANSPORT_HTTP_CLIENT_H

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
#include "azure_hub_modules/dps_transport.h"

typedef struct DPS_TRANSPORT_HTTP_INFO_TAG* DPS_TRANSPORT_HTTP_HANDLE;

const DPS_TRANSPORT_PROVIDER* DPS_HTTP_Protocol(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DPS_TRANSPORT_HTTP_CLIENT_H
