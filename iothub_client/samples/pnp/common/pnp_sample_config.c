// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This file implements the logic to retrieve the PnP sample's configuration via the environment.

#include <stdlib.h>

#include "azure_c_shared_utility/xlogging.h"
#include "pnp_sample_config.h"

// Environment variable used to specify how app connects to hub and the two possible values.
static const char g_securityTypeEnvironmentVariable[] = "IOTHUB_DEVICE_SECURITY_TYPE";
static const char g_securityTypeConnectionStringValue[] = "connectionString";
static const char g_securityTypeDpsValue[] = "DPS";

// Environment variable used to specify this application's connection string.
static const char g_connectionStringEnvironmentVariable[] = "IOTHUB_DEVICE_CONNECTION_STRING";

#ifdef USE_PROV_MODULE_FULL
// Environment variable used to specify this application's DPS id scope.
static const char g_dpsIdScopeEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_ID_SCOPE";

// Environment variable used to specify this application's DPS device id.
static const char g_dpsDeviceIdEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_DEVICE_ID";

// Environment variable used to specify this application's DPS device key.
static const char g_dpsDeviceKeyEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_DEVICE_KEY";

// Environment variable used to optionally specify this application's DPS id scope.
static const char g_dpsEndpointEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_ENDPOINT";

// Global provisioning endpoint for DPS if one is not specified via the environment.
static const char g_dps_DefaultGlobalProvUri[] = "global.azure-devices-provisioning.net";
#endif

//
// GetConnectionStringFromEnvironment retrieves the connection string based on environment variable.
//
static bool GetConnectionStringFromEnvironment(PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration)
{
    bool result;

    if ((pnpDeviceConfiguration->u.connectionString = getenv(g_connectionStringEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_connectionStringEnvironmentVariable);
        result = false;
    }
    else
    {
        pnpDeviceConfiguration->securityType = PNP_CONNECTION_SECURITY_TYPE_CONNECTION_STRING;
        result = true;    
    }

    return result;
}

//
// GetDpsFromEnvironment retrieves DPS configuration for a symmetric key based connection
// from environment variables.
//
static bool GetDpsFromEnvironment(PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration)
{
#ifndef USE_PROV_MODULE_FULL
    // Explain to user misconfiguration.  The "run_e2e_tests" must be set to OFF because otherwise
    // the e2e's test HSM layer and symmetric key logic will conflict.
    (void)pnpDeviceConfiguration;
    LogError("DPS based authentication was requested via environment variables, but DPS is not enabled.");
    LogError("DPS is an optional component of the Azure IoT C SDK.  It is enabled with symmetric keys at CMake time by");
    LogError("passing <-Duse_prov_client=ON -Dhsm_type_symm_key=ON -Drun_e2e_tests=OFF> to CMake's command line");
    return false;
#else
    bool result;

    if ((pnpDeviceConfiguration->u.dpsConnectionAuth.endpoint = getenv(g_dpsEndpointEnvironmentVariable)) == NULL)
    {
        // We will fall back to standard endpoint if one is not specified
        pnpDeviceConfiguration->u.dpsConnectionAuth.endpoint = g_dps_DefaultGlobalProvUri;
    }

    if ((pnpDeviceConfiguration->u.dpsConnectionAuth.idScope = getenv(g_dpsIdScopeEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsIdScopeEnvironmentVariable);
        result = false;
    }
    else if ((pnpDeviceConfiguration->u.dpsConnectionAuth.deviceId = getenv(g_dpsDeviceIdEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsDeviceIdEnvironmentVariable);
        result = false;
    }
    else if ((pnpDeviceConfiguration->u.dpsConnectionAuth.deviceKey = getenv(g_dpsDeviceKeyEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsDeviceKeyEnvironmentVariable);
        result = false;
    }
    else
    {
        pnpDeviceConfiguration->securityType = PNP_CONNECTION_SECURITY_TYPE_DPS;
        result = true;    
    }

    return result;
#endif // USE_PROV_MODULE_FULL
}

bool GetConnectionSettingsFromEnvironment(PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration)
{
    const char* securityTypeString;
    bool result;

    if ((securityTypeString = getenv(g_securityTypeEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_securityTypeEnvironmentVariable);
        result = false;
    }
    else
    {
        if (strcmp(securityTypeString, g_securityTypeConnectionStringValue) == 0)
        {
            result = GetConnectionStringFromEnvironment(pnpDeviceConfiguration);
        }
        else if (strcmp(securityTypeString, g_securityTypeDpsValue) == 0)
        {
            result = GetDpsFromEnvironment(pnpDeviceConfiguration);
        }
        else
        {
            LogError("Environment variable %s must be either %s or %s", g_securityTypeEnvironmentVariable, g_securityTypeConnectionStringValue, g_securityTypeDpsValue);
            result = false;
        }
    }

    return result;    
}
