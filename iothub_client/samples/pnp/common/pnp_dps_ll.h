// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PNP_DPS_LL_H
#define PNP_DPS_LL_H

#include "iothub_device_client.h"

#ifndef USE_PROV_MODULE_FULL
#error "Missing cmake flag for DPS"
#endif

//
// PnP_CreateDeviceClientHandle_ViaDps is used to create a IOTHUB_DEVICE_CLIENT_HANDLE, invoking the DPS client
// to retrieve the needed hub information.
//
// Applications should NOT invoke this function directly but instead should use PnP_CreateDeviceClientHandle.
//
IOTHUB_DEVICE_CLIENT_HANDLE PnP_CreateDeviceClientHandle_ViaDps(const PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration);

#endif /* PNP_DPS_LL_H */
