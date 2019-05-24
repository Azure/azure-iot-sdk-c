// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_PROV_TRANSPORT_H
#define IOTHUB_PROV_TRANSPORT_H

#include "internal/iothub_transport_ll_private.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C"
{
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

    struct IOTHUB_PROV_TRANSPORT_PROVIDER_TAG;
    typedef struct IOTHUB_PROV_TRANSPORT_PROVIDER_TAG IOTHUB_PROV_TRANSPORT_PROVIDER;

    typedef const IOTHUB_PROV_TRANSPORT_PROVIDER*(*IOTHUB_PROV_CLIENT_TRANSPORT_PROVIDER)(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // IOTHUB_PROV_TRANSPORT_H
