// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This header implements a simulated DeviceInformation interface defined by dtmi:azure:DeviceManagement:DeviceInformation;1.
// This would (if actually tied into the appropriate Operating System APIs) return information about the amount of memory,
// the OS and its version, and other information about the device itself.

// DeviceInfo only reports properties defining the device and does not accept requested properties or commands.

#ifndef PNP_DEVICEINFO_COMPONENT_H
#define PNP_DEVICEINFO_COMPONENT_H

#include "iothub_device_client.h"

//
// PnP_DeviceInfoComponent_Report_All_Properties sends properties corresponding to the DeviceInfo interface to the cloud.
//
void PnP_DeviceInfoComponent_Report_All_Properties(const char* componentName, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient);

#endif /* PNP_DEVICEINFO_COMPONENT_H */
