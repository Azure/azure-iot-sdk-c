// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Helper routines for PnP based samples to interact with Device Provisioning Service

#ifndef PNP_DPS_LL_H
#define PNP_DPS_LL_H

#include "iothub_device_client_ll.h"
#include "pnp_sample_config.h"

#ifndef USE_PROV_MODULE_FULL
// DPS is not brought in all configurations of the SDK.  If you hit this error,
// rerun CMake with the flags <-Duse_prov_client=ON -Dhsm_type_symm_key=ON -Drun_e2e_tests=OFF>
#error "Missing CMake flag for DPS"
#endif

//
// PnP_CreateDeviceClientLLHandle_ViaDps is used to create a IOTHUB_DEVICE_CLIENT_LL_HANDLE, invoking the DPS client
// to retrieve the needed hub information.
//
// NOTE: This function will BLOCK waiting for DPS to finish provisioning.
//
IOTHUB_DEVICE_CLIENT_LL_HANDLE PnP_CreateDeviceClientLLHandle_ViaDps(const PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration);

#endif /* PNP_DPS_LL_H */
