// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Header file for sample for manipulating DigitalTwin Interface for device info.

#ifndef DIGITALTWIN_SAMPLE_SDK_INFO
#define DIGITALTWIN_SAMPLE_SDK_INFO

#include "digitaltwin_interface_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Creates a new DIGITALTWIN_INTERFACE_CLIENT_HANDLE for this interface.
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DigitalTwinSampleSdkInfo_CreateInterface(void);

// Closes down resources associated with sdk info interface.
void DigitalTwinSampleSdkInfo_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle);

#ifdef __cplusplus
}
#endif


#endif // DIGITALTWIN_SAMPLE_SDK_INFO
