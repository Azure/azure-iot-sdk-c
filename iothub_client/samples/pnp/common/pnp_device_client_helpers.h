// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PNP_DEVICE_CLIENT_HELPERS_H
#define PNP_DEVICE_CLIENT_HELPERS_H

#include "iothub_device_client.h"

//
// Whether we're using a connection string or DPS provisioning for device credentials
//
typedef enum PNP_CONNECTION_SECURITY_TYPE_TAG
{
    PNP_CONNECTION_SECURITY_TYPE_CONNECTION_STRING,
    PNP_CONNECTION_SECURITY_TYPE_DPS
} PNP_CONNECTION_SECURITY_TYPE;

#ifdef USE_PROV_MODULE
//
// DPS functionality is only available if the cmake flag -Duse_prov_client=ON was
// enabled when building the Azure IoT C SDK.
//
// PNP_DPS_SYMMETRIC_KEY specifies the values the authentication material needed
// to authenticate when using DPS symmetric keys.
// 
typedef struct PNP_DPS_SYMMETRIC_KEY_TAG
{
    char* idScope;
    char* deviceId;
    char* deviceKey;
} PNP_DPS_SYMMETRIC_KEY;

//
// PNP_DPS_CONFIGURATION is the configuration for the IoTHub retrieved by the
// DPS client at runtime (NOT pre-configured environmnent)
typedef struct PNP_DPS_CONFIGURATION_TAG
{
    char* iothubUri;
    char* deviceId;
} PNP_DPS_CONFIGURATION;

#endif /* USE_PROV_MODULE */

//
// PNP_HELPER_DEVICE_CONFIGURATION is used to setup the IOTHUB_DEVICE_CLIENT_HANDLE
//
typedef struct PNP_DEVICE_CONFIGURATION_TAG
{
    // Whether we're using connection string or DPS
    PNP_CONNECTION_SECURITY_TYPE securityType;
    // The connection string or DPS security information
    union {
        char* connectionString;
#ifdef USE_PROV_MODULE
        PNP_DPS_CONFIGURATION dpsConfiguration;
#endif
    } u;
    // ModelId of this PnP device
    const char* modelId;
    // Whether more verbose tracing is enabled at IoT Hub DeviceClient layer
    bool enableTracing;
    // Callback for IoT Hub device methods, which is the mechanism PnP commands use
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback;
    // Callback for IoT Hub device twin notifications, which is the mechanism PnP properties from service use
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback;
} PNP_DEVICE_CONFIGURATION;

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
IOTHUB_DEVICE_CLIENT_HANDLE PnPHelper_CreateDeviceClientHandle(const PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration);
//const char* connectionString, const char* modelId, bool enableTracing, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback);

#endif /* PNP_DEVICE_CLIENT_HELPERS_H */
