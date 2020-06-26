// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PNP_DEVICE_CLIENT_HELPERS_H
#define PNP_DEVICE_CLIENT_HELPERS_H

#include "iothub.h"
#include "iothub_device_client.h"

IOTHUB_DEVICE_CLIENT_HANDLE PnPHelper_CreateDeviceClient(const char* connectionString, const char* modelId, bool enableTracing, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback);

#endif /* PNP_DEVICE_CLIENT_HELPERS_H */
