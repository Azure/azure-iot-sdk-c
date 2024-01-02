// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PNP_SAMPLE_CONFIG_H
#define PNP_SAMPLE_CONFIG_H

#include <stdbool.h>

//
// Whether we're using a connection string or DPS provisioning for device credentials
//
typedef enum PNP_CONNECTION_SECURITY_TYPE_TAG
{
    PNP_CONNECTION_SECURITY_TYPE_CONNECTION_STRING,
    PNP_CONNECTION_SECURITY_TYPE_DPS
} PNP_CONNECTION_SECURITY_TYPE;

#ifdef USE_PROV_MODULE_FULL
//
// PNP_DPS_CONNECTION_AUTH is used to configure the DPS device client
//
typedef struct PNP_DPS_CONNECTION_AUTH_TAG
{
    const char* endpoint;
    const char* idScope;
    const char* deviceId;
    const char* deviceKey;
} PNP_DPS_CONNECTION_AUTH;
#endif /* USE_PROV_MODULE_FULL */

//
// PNP_DEVICE_CONFIGURATION is used to setup the IOTHUB_DEVICE_CLIENT_LL_HANDLE
//
typedef struct PNP_DEVICE_CONFIGURATION_TAG
{
    // Whether we're using connection string or DPS
    PNP_CONNECTION_SECURITY_TYPE securityType;
    // The connection string or DPS security information
    union {
        char* connectionString;
#ifdef USE_PROV_MODULE_FULL
        PNP_DPS_CONNECTION_AUTH dpsConnectionAuth;
#endif
    } u;
    // ModelId of this PnP device
    const char* modelId;
    // Whether more verbose tracing is enabled for the IoT Hub client
    bool enableTracing;
} PNP_DEVICE_CONFIGURATION;

//
// GetConfigurationFromEnvironment determines how to connect to the IoT Hub (using 
// either a connection string or a DPS symmetric key) from the environment.
//
bool GetConnectionSettingsFromEnvironment(PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration);

#endif /* PNP_SAMPLE_CONFIG_H */
