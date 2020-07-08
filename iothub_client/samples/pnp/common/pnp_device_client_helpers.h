// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PNP_DEVICE_CLIENT_HELPERS_H
#define PNP_DEVICE_CLIENT_HELPERS_H

#include "iothub_device_client.h"

//
// PnPHelper_CreateDeviceClientHandle creates an IOTHUB_DEVICE_CLIENT_HANDLE that will be ready to interact with PnP.
// Beyond basic handle creation, it also sets the handle to the appropriate Model Id, sets up callback functions
// for Device Method and Device Twin callbacks (to process PnP Commands and Properties, respectively)
// as well as some other basic maintenence on the handle. 
//
IOTHUB_DEVICE_CLIENT_HANDLE PnPHelper_CreateDeviceClientHandle(const char* connectionString, const char* modelId, bool enableTracing, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback);

#endif /* PNP_DEVICE_CLIENT_HELPERS_H */
