// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PNP_DEVICE_CLIENT_HELPERS_H
#define PNP_DEVICE_CLIENT_HELPERS_H

#include "iothub_device_client.h"

//
// PnPHelper_CreateDeviceClientHandle creates an IOTHUB_DEVICE_CLIENT_HANDLE that will be ready to interact with PnP.
// Beyond basic handle creation, it also sets the handle to the appropriate ModelId, optionally sets up callback functions
// for Device Method and Device Twin callbacks (to process PnP Commands and Properties, respectively)
// as well as some other basic maintenence on the handle. 
//
// If the application's model does not implement PnP commands, the application should set deviceMethodCallback to NULL.
// Anologously, if there are no desired PnP properties, deviceTwinCallback should be NULL.  Not subscribing for these
// callbacks if they are not needed by the model will save bandwidth and RAM.
//
IOTHUB_DEVICE_CLIENT_HANDLE PnPHelper_CreateDeviceClientHandle(const char* connectionString, const char* modelId, bool enableTracing, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback);

#endif /* PNP_DEVICE_CLIENT_HELPERS_H */
