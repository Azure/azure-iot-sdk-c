// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Header file for sample for manipulating DigitalTwin Interface for device info.

#ifndef DIGITALTWIN_SAMPLE_DEVICE_INFO
#define DIGITALTWIN_SAMPLE_DEVICE_INFO

#include "digitaltwin_interface_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Creates a new DIGITALTWIN_INTERFACE_CLIENT_HANDLE for this interface.
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DigitalTwinSampleDeviceInfo_CreateInterface(void);
// Closes down resources associated with device info interface.
void DigitalTwinSampleDeviceInfo_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle);

#ifdef __cplusplus
}
#endif


#endif // DIGITALTWIN_SAMPLE_DEVICE_INFO
