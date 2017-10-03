// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_TRANSPORT_MQTT_CLIENT_H
#define DPS_TRANSPORT_MQTT_CLIENT_H

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

const DPS_TRANSPORT_PROVIDER* DPS_MQTT_Protocol(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DPS_TRANSPORT_MQTT_CLIENT_H
