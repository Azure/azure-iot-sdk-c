// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_PROV_TRANSPORT_MQTT_WS_CLIENT_H
#define IOTHUB_PROV_TRANSPORT_MQTT_WS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */

#include "umock_c/umock_c_prod.h"
#include "azure_macro_utils/macro_utils.h"
#include "iothub_prov_transport.h"

MOCKABLE_FUNCTION(, const IOTHUB_PROV_TRANSPORT_PROVIDER*, IoTHub_Prov_MQTT_WS_Protocol);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // IOTHUB_PROV_TRANSPORT_MQTT_WS_CLIENT_H
