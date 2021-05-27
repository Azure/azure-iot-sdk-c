// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample implements a PnP based Temperature Control model.  It demonstrates a complex 
// model in that the model has properties, commands, and telemetry off of the root component
// as well as subcomponents.

// The DTDL for component is https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json

// Standard C header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IoTHub Device Client and IoT core utility related header files
#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothub_message.h"
#include "iothub_client_properties.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"

#include "pnp_sample_config.h"

// Headers that provide implementation for subcomponents (the two thermostat components and DeviceInfo)
#include "pnp_thermostat_component.h"
#include "pnp_deviceinfo_component.h"


#ifdef SET_TRUSTED_CERT_IN_SAMPLES
// For devices that do not have (or want) an OS level trusted certificate store,
// but instead bring in default trusted certificates from the Azure IoT C SDK.
#include "azure_c_shared_utility/shared_util_options.h"
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#ifdef USE_PROV_MODULE_FULL
#include "pnp_dps_ll.h"
#endif // USE_PROV_MODULE_FULL

// Environment variable used to specify how app connects to hub and the two possible values
static const char g_securityTypeEnvironmentVariable[] = "IOTHUB_DEVICE_SECURITY_TYPE";
static const char g_securityTypeConnectionStringValue[] = "connectionString";
static const char g_securityTypeDpsValue[] = "DPS";

// Environment variable used to specify this application's connection string
static const char g_connectionStringEnvironmentVariable[] = "IOTHUB_DEVICE_CONNECTION_STRING";

// Values of connection / security settings read from environment variables and/or DPS runtime
PNP_DEVICE_CONFIGURATION g_pnpDeviceConfiguration;

#ifdef USE_PROV_MODULE_FULL
// Environment variable used to specify this application's DPS id scope
static const char g_dpsIdScopeEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_ID_SCOPE";

// Environment variable used to specify this application's DPS device id
static const char g_dpsDeviceIdEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_DEVICE_ID";

// Environment variable used to specify this application's DPS device key
static const char g_dpsDeviceKeyEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_DEVICE_KEY";

// Environment variable used to optionally specify this application's DPS id scope
static const char g_dpsEndpointEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_ENDPOINT";

// Global provisioning endpoint for DPS if one is not specified via the environment
static const char g_dps_DefaultGlobalProvUri[] = "global.azure-devices-provisioning.net";
#endif

// Amount of time to sleep between polling hub, in milliseconds.  Set to wake up every 100 milliseconds.
static unsigned int g_sleepBetweenPollsMs = 100;

// Every time the main loop wakes up, on the g_sendTelemetryPollInterval(th) pass will send a telemetry message.
// So we will send telemetry every (g_sendTelemetryPollInterval * g_sleepBetweenPollsMs) milliseconds; 60 seconds as currently configured.
static const int g_sendTelemetryPollInterval = 600;

// Whether tracing at the IoTHub client is enabled or not. 
static bool g_hubClientTraceEnabled = true;

// DTMI indicating this device's ModelId.
static const char g_temperatureControllerModelId[] = "dtmi:com:example:TemperatureController;1";

static const char g_jsonContentType[] = "application/json";
static const char g_utf8EncodingType[] = "utf8";

// PNP_THERMOSTAT_COMPONENT_HANDLE represent the thermostat components that are sub-components of the temperature controller.
// Note that we do NOT have an analogous DeviceInfo component handle because there is only DeviceInfo subcomponent and its
// implementation is straightforward.
PNP_THERMOSTAT_COMPONENT_HANDLE g_thermostatHandle1;
PNP_THERMOSTAT_COMPONENT_HANDLE g_thermostatHandle2;

// Name of subcomponents that TemmperatureController implements.
static const char g_thermostatComponent1Name[] = "thermostat1";
static const size_t g_thermostatComponent1Size = sizeof(g_thermostatComponent1Name) - 1;
static const char g_thermostatComponent2Name[] = "thermostat2";
static const size_t g_thermostatComponent2Size = sizeof(g_thermostatComponent2Name) - 1;
static const char g_deviceInfoComponentName[] = "deviceInformation";

static const char* g_modeledComponents[] = {g_thermostatComponent1Name, g_thermostatComponent2Name, g_deviceInfoComponentName};
static const size_t g_numModeledComponents = sizeof(g_modeledComponents) / sizeof(g_modeledComponents[0]);

// Command implemented by the TemperatureControl component itself to implement reboot.
static const char g_rebootCommand[] = "reboot";

// An empty JSON body for PnP command responses
static const char g_JSONEmpty[] = "{}";
static const size_t g_JSONEmptySize = sizeof(g_JSONEmpty) - 1;

// Minimum value we will return for working set, + some random number
static const int g_workingSetMinimum = 1000;
// Random number for working set will range between the g_workingSetMinimum and (g_workingSetMinimum+g_workingSetRandomModulo)
static const int g_workingSetRandomModulo = 500;
// Format string for sending a telemetry message with the working set.
static const char g_workingSetTelemetryFormat[] = "{\"workingSet\":%d}";

// Name of the serial number property as defined in this component's DTML
static const char g_serialNumberPropertyName[] = "serialNumber";
// Value of the serial number.  NOTE: This must be a legal JSON string which requires value to be in "..."
static const char g_serialNumberPropertyValue[] = "\"serial-no-123-abc\"";

//
// AllocateDeviceClientHandle does the actual createHandle call, depending on the security type
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE AllocateDeviceClientHandle(const PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceHandle = NULL;

    if (pnpDeviceConfiguration->securityType == PNP_CONNECTION_SECURITY_TYPE_CONNECTION_STRING)
    {
        if ((deviceHandle = IoTHubDeviceClient_LL_CreateFromConnectionString(pnpDeviceConfiguration->u.connectionString, MQTT_Protocol)) == NULL)
        {
            LogError("Failure creating IotHub client.  Hint: Check your connection string");
        }
    }
#ifdef USE_PROV_MODULE_FULL
    else if ((deviceHandle = PnP_CreateDeviceClientLLHandle_ViaDps(pnpDeviceConfiguration)) == NULL)
    {
        LogError("Cannot retrieve IoT Hub connection information from DPS client");
    }
#endif /* USE_PROV_MODULE_FULL */

    return deviceHandle;
}

//
// TempControl_CreateDeviceClientLLHandle creates an IOTHUB_DEVICE_CLIENT_LL_HANDLE that will be ready to interact with PnP.
// Beyond basic handle creation, it also sets the handle to the appropriate ModelId, optionally sets up callback functions
// for Device Method and Device Twin callbacks (to process PnP Commands and Properties, respectively)
// as well as some other basic maintenence on the handle. 
//
// NOTE: When using DPS based authentication, this function can *block* until DPS responds to the request or timeout.
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE TempControl_CreateDeviceClientLLHandle(const PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;
    bool urlAutoEncodeDecode = true;
    int iothubInitResult;
    bool result;

    // Before invoking ANY IoT Hub or DPS functionality, IoTHub_Init must be invoked.
    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        LogError("Failure to initialize client, error=%d", iothubInitResult);
        result = false;
    }
    else if ((deviceHandle = AllocateDeviceClientHandle(pnpDeviceConfiguration)) == NULL)
    {
        LogError("Unable to allocate deviceHandle");
        result = false;
    }
    // Sets verbosity level
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_LOG_TRACE, &pnpDeviceConfiguration->enableTracing)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set logging option, error=%d", iothubResult);
        result = false;
    }
    // Sets the name of ModelId for this PnP device.
    // This *MUST* be set before the client is connected to IoTHub.  We do not automatically connect when the 
    // handle is created, but will implicitly connect to subscribe for device method and device twin callbacks below.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_MODEL_ID, pnpDeviceConfiguration->modelId)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the ModelID, error=%d", iothubResult);
        result = false;
    }
    // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set auto Url encode option, error=%d", iothubResult);
        result = false;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on systems without built in certificate stores.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the trusted cert, error=%d", iothubResult);
        result = false;
    }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    else
    {
        result = true;
    }

    if ((result == false) && (deviceHandle != NULL))
    {
        IoTHubDeviceClient_LL_Destroy(deviceHandle);
        deviceHandle = NULL;
    }

    if ((result == false) &&  (iothubInitResult == 0))
    {
        IoTHub_Deinit();
    }

    return deviceHandle;
}


//
// PnP_TempControlComponent_InvokeRebootCommand processes the reboot command on the root interface
//
static int PnP_TempControlComponent_InvokeRebootCommand(JSON_Value* rootValue)
{
    int result;

    if (json_value_get_type(rootValue) != JSONNumber)
    {
        LogError("Delay payload is not a number");
        result = 400;
    }
    else
    {
        // See caveats section in ../readme.md; we don't actually respect the delay value to keep the sample simple.
        int delayInSeconds = (int)json_value_get_number(rootValue);
        LogInfo("Temperature controller 'reboot' command invoked with delay=%d seconds", delayInSeconds);
        result = 200;
    }
    
    return result;
}

//
// SetEmptyCommandResponse sets the response to be an empty JSON.  IoT Hub wants
// legal JSON, regardless of error status, so if command implementation did not set this do so here.
//
static void SetEmptyCommandResponse(unsigned char** response, size_t* responseSize, int* result)
{
    if ((*response = calloc(1, g_JSONEmptySize)) == NULL)
    {
        LogError("Unable to allocate empty JSON response");
        *result = 500;
    }
    else
    {
        memcpy(*response, g_JSONEmpty, g_JSONEmptySize);
        *responseSize = g_JSONEmptySize;
        // We only overwrite the caller's result on error; otherwise leave as it was
    }
}

// Note this was previously part of helper lib, named PnP_CopyPayloadToString.  
// Don't think this makes sense in API though so would be in app code.
static char* CopyPayloadToString(const unsigned char* payload, size_t size)
{
    char* jsonStr;
    size_t sizeToAllocate = size + 1;

    if ((jsonStr = (char*)malloc(sizeToAllocate)) == NULL)
    {
        LogError("Unable to allocate %lu size buffer", (unsigned long)(sizeToAllocate));
    }
    else
    {
        memcpy(jsonStr, payload, size);
        jsonStr[size] = '\0';
    }

    return jsonStr;
}

//
// PnP_TempControlComponent_DeviceMethodCallback is invoked by IoT SDK when a device method arrives.
//
static int PnP_TempControlComponent_CommandCallback(const char* componentName, const char* commandName, const unsigned char* payload, size_t size, unsigned char** response, size_t* responseSize, void* userContextCallback)
{
    (void)userContextCallback;

    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    int result;
    // unsigned const char *componentName;
    // size_t componentNameSize;
    // const char *pnpCommandName;

    *response = NULL;
    *responseSize = 0;

    // Parse the methodName into its PnP (optional) componentName and pnpCommandName.

    if ((jsonStr = CopyPayloadToString(payload, size)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
        result = 500;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse twin JSON");
        result = 500;
    }
    else
    {
        if (componentName != NULL)
        {
            LogInfo("Received PnP command for component=%s, command=%s", componentName, commandName);
            if (strcmp(componentName, g_thermostatComponent1Name) == 0)
            {
                result = PnP_ThermostatComponent_ProcessCommand(g_thermostatHandle1, commandName, rootValue, response, responseSize);
            }
            else if (strcmp(componentName, g_thermostatComponent2Name) == 0)
            {
                result = PnP_ThermostatComponent_ProcessCommand(g_thermostatHandle2, commandName, rootValue, response, responseSize);
            }
            else
            {
                LogError("PnP component=%s is not supported by TemperatureController", componentName);
                result = 404;
            }
        }
        else
        {
            LogInfo("Received PnP command for TemperatureController component, command=%s", commandName);
            if (strcmp(commandName, g_rebootCommand) == 0)
            {
                result = PnP_TempControlComponent_InvokeRebootCommand(rootValue);
            }
            else
            {
                LogError("PnP command=s%s is not supported by TemperatureController", commandName);
                result = 404;
            }
        }
    }

    if (*response == NULL)
    {
        SetEmptyCommandResponse(response, responseSize, &result);
    }

    json_value_free(rootValue);
    free(jsonStr);

    return result;
}

int PnP_TempControlComponent_UpdatedPropertyCallback(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, 
    const unsigned char* payLoad,
    size_t payloadLength,
    void* userContextCallback)
{
#if 0
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = (IOTHUB_DEVICE_CLIENT_LL_HANDLE)userContextCallback;
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY property;
    IOTHUB_CLIENT_PROPERTY_CONTEXT_HANDLE propertyContext;
    int propertiesVersion;
    IOTHUB_CLIENT_RESULT clientResult;

// OPTION2 - preferred
    if ((clientResult = IoTHubClient_Deserialize_Properties_Begin(payloadType, payLoad, payloadLength, &propertyContext, &propertiesVersion)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubClient_Deserialize_Properties failed, error=%d", clientResult);
    }
    else
    {
        bool propertySpecified;
        while ((clientResult = IoTHubClient_Deserialize_Properties_GetNextProperty(propertyContext, g_modeledComponents, g_numModeledComponents, &property, &propertySpecified)) == IOTHUB_CLIENT_OK)
        {
            if (propertySpecified == false)
            {
                break;
            }

            if (property.propertyType == IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE)
            {
                // We don't process previously reported properties, so ignore.  There is a potential optimization
                // however where if a desired property is the same value and version of a reported, then 
                // it wouldn't be necessary to call IoTHubDeviceClient_LL_PnP_SendReportedProperties for it
                continue;
            }
            
            if (property.componentName == NULL) 
            {   
                LogError("Property=%s arrived for TemperatureControl component itself.  This does not support properties on it (all properties are on subcomponents)", property.componentName);
            }
            else if (strcmp(property.componentName, g_thermostatComponent1Name) == 0)
            {
                PnP_ThermostatComponent_ProcessPropertyUpdate(g_thermostatHandle1, deviceClient, property.propertyName, property.propertyValue, propertiesVersion);
            }
            else if (strcmp(property.componentName, g_thermostatComponent2Name) == 0)
            {
                PnP_ThermostatComponent_ProcessPropertyUpdate(g_thermostatHandle2, deviceClient, property.propertyName, property.propertyValue, propertiesVersion);
            }
            else
            {
                LogError("Component=%s is not implemented by the TemperatureController", property.componentName);
            }
            
            IoTHubClient_Deserialize_Properties_FreeProperty(&property);
        }
    }

    IoTHubClient_Deserialize_Properties_End(propertyContext);


// END OPTION2

// OPTION1 - not preferred
/*
    if ((clientResult = IoTHubClient_Deserialize_Properties(payloadType, payLoad, payloadLength, g_modeledComponents, g_numModeledComponents, &properties, &numProperties, &propertiesVersion)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubClient_Deserialize_Properties failed, error=%d", clientResult);
    }
    else
    {
        for (size_t i = 0; i < numProperties; i++) 
        {
            const IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property = &properties[i];

            if (property->propertyType == IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE)
            {
                // We don't process previously reported properties, so ignore.  There is a potential optimization
                // however where if a desired property is the same value and version of a reported, then 
                // it wouldn't be necessary to call IoTHubDeviceClient_LL_PnP_SendReportedProperties for it
                continue;
            }

            if (property->componentName == NULL) 
            {   
                LogError("Property=%s arrived for TemperatureControl component itself.  This does not support properties on it (all properties are on subcomponents)", property->componentName);
            }
            else if (strcmp(property->componentName, g_thermostatComponent1Name) == 0)
            {
                PnP_ThermostatComponent_ProcessPropertyUpdate(g_thermostatHandle1, deviceClient, property->propertyName, property->propertyValue, propertiesVersion);
            }
            else if (strcmp(property->componentName, g_thermostatComponent2Name) == 0)
            {
                PnP_ThermostatComponent_ProcessPropertyUpdate(g_thermostatHandle2, deviceClient, property->propertyName, property->propertyValue, propertiesVersion);
            }
            else
            {
                LogError("Component=%s is not implemented by the TemperatureController", property->componentName);
            }
        }
    }

    IoTHubClient_Deserialized_Properties_Destroy(properties, numProperties);
*/
// END OPTION1
#endif
    return 0;
}


//
// PnP_TempControlComponent_SendWorkingSet sends a PnP telemetry indicating the current working set of the device, in 
// the unit of kibibytes (https://en.wikipedia.org/wiki/Kibibyte).
// This is a random value between g_workingSetMinimum and (g_workingSetMinimum+g_workingSetRandomModulo).
//
void PnP_TempControlComponent_SendWorkingSet(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_MESSAGE_RESULT messageResult;
    IOTHUB_CLIENT_RESULT iothubResult;
    char workingSetTelemetryPayload[64];

    int workingSet = g_workingSetMinimum + (rand() % g_workingSetRandomModulo);

    if (snprintf(workingSetTelemetryPayload, sizeof(workingSetTelemetryPayload), g_workingSetTelemetryFormat, workingSet) < 0)
    {
        LogError("Unable to create a workingSet telemetry payload string");
    }
    else if ((messageHandle = IoTHubMessage_CreateFromString(workingSetTelemetryPayload)) == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
    }
    else if ((messageResult = IoTHubMessage_SetContentTypeSystemProperty(messageHandle, g_jsonContentType)) != IOTHUB_MESSAGE_OK)
    {
        LogError("IoTHubMessage_SetContentTypeSystemProperty failed, error=%d", messageResult);
    }
    else if ((messageResult = IoTHubMessage_SetContentEncodingSystemProperty(messageHandle, g_utf8EncodingType)) != IOTHUB_MESSAGE_OK)
    {
        LogError("IoTHubMessage_SetContentEncodingSystemProperty failed, error=%d", messageResult);
    }
    else if ((iothubResult = IoTHubDeviceClient_LL_SendTelemetryAsync(deviceClient, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send telemetry message, error=%d", iothubResult);
    }

    IoTHubMessage_Destroy(messageHandle);
}

//
// PnP_TempControlComponent_ReportSerialNumber_Property sends the "serialNumber" property to IoTHub
//
static void PnP_TempControlComponent_ReportSerialNumber_Property(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    IOTHUB_CLIENT_REPORTED_PROPERTY property = { IOTHUB_CLIENT_REPORTED_PROPERTY_VERSION_1, g_serialNumberPropertyName, g_serialNumberPropertyValue };
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength;

    // The first step of reporting properties is to serialize it into an IoT Hub friendly format.  You can do this by either
    // implementing the PnP convention for building up the correct JSON or more simply to use IoTHubClient_Serialize_ReportedProperties.
    if ((iothubClientResult = IoTHubClient_Serialize_ReportedProperties(&property, 1, NULL, &serializedProperties, &serializedPropertiesLength)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to serialize reported state, error=%d", iothubClientResult);
    }
    // The output of IoTHubClient_Serialize_ReportedProperties is sent to IoTHubDeviceClient_LL_SendPropertiesAsync to perform network I/O.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClient, serializedProperties, serializedPropertiesLength, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send reported state, error=%d", iothubClientResult);
    }

    free(serializedProperties);
}

//
// GetConnectionStringFromEnvironment retrieves the connection string based on environment variable
//
static bool GetConnectionStringFromEnvironment()
{
    bool result;

    if ((g_pnpDeviceConfiguration.u.connectionString = getenv(g_connectionStringEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_connectionStringEnvironmentVariable);
        result = false;
    }
    else
    {
        g_pnpDeviceConfiguration.securityType = PNP_CONNECTION_SECURITY_TYPE_CONNECTION_STRING;
        result = true;    
    }

    return result;
}

//
// GetDpsFromEnvironment retrieves DPS configuration for a symmetric key based connection
// from environment variables
//
static bool GetDpsFromEnvironment()
{
#ifndef USE_PROV_MODULE_FULL
    // Explain to user misconfiguration.  The "run_e2e_tests" must be set to OFF because otherwise
    // the e2e's test HSM layer and symmetric key logic will conflict.
    LogError("DPS based authentication was requested via environment variables, but DPS is not enabled.");
    LogError("DPS is an optional component of the Azure IoT C SDK.  It is enabled with symmetric keys at cmake time by");
    LogError("passing <-Duse_prov_client=ON -Dhsm_type_symm_key=ON -Drun_e2e_tests=OFF> to cmake's command line");
    return false;
#else
    bool result;

    if ((g_pnpDeviceConfiguration.u.dpsConnectionAuth.endpoint = getenv(g_dpsEndpointEnvironmentVariable)) == NULL)
    {
        // We will fall back to standard endpoint if one is not specified
        g_pnpDeviceConfiguration.u.dpsConnectionAuth.endpoint = g_dps_DefaultGlobalProvUri;
    }

    if ((g_pnpDeviceConfiguration.u.dpsConnectionAuth.idScope = getenv(g_dpsIdScopeEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsIdScopeEnvironmentVariable);
        result = false;
    }
    else if ((g_pnpDeviceConfiguration.u.dpsConnectionAuth.deviceId = getenv(g_dpsDeviceIdEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsDeviceIdEnvironmentVariable);
        result = false;
    }
    else if ((g_pnpDeviceConfiguration.u.dpsConnectionAuth.deviceKey = getenv(g_dpsDeviceKeyEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsDeviceKeyEnvironmentVariable);
        result = false;
    }
    else
    {
        g_pnpDeviceConfiguration.securityType = PNP_CONNECTION_SECURITY_TYPE_DPS;
        result = true;    
    }

    return result;
#endif // USE_PROV_MODULE_FULL
}


//
// GetConfigurationFromEnvironment reads how to connect to the IoT Hub (using 
// either a connection string or a DPS symmetric key) from the environment.
//
static bool GetConnectionSettingsFromEnvironment()
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
            result = GetConnectionStringFromEnvironment();
        }
        else if (strcmp(securityTypeString, g_securityTypeDpsValue) == 0)
        {
            result = GetDpsFromEnvironment();
        }
        else
        {
            LogError("Environment variable %s must be either %s or %s", g_securityTypeEnvironmentVariable, g_securityTypeConnectionStringValue, g_securityTypeDpsValue);
            result = false;
        }
    }

    return result;    
}

//
// CreateDeviceClientAndAllocateComponents allocates the IOTHUB_DEVICE_CLIENT_LL_HANDLE the application will use along with thermostat components
// 
static IOTHUB_DEVICE_CLIENT_LL_HANDLE CreateDeviceClientAndAllocateComponents(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;
    IOTHUB_CLIENT_RESULT clientResult;
    bool result;

    g_pnpDeviceConfiguration.enableTracing = g_hubClientTraceEnabled;
    g_pnpDeviceConfiguration.modelId = g_temperatureControllerModelId;

    if (GetConnectionSettingsFromEnvironment() == false)
    {
        LogError("Cannot read required environment variable(s)");
        result = false;
    }
    else if ((deviceClient = TempControl_CreateDeviceClientLLHandle(&g_pnpDeviceConfiguration)) == NULL)
    {
        LogError("Failure creating IotHub device client");
        result = false;
    }
    // Subscribes for incoming commands from IoT Hub and when they arrive, have them routed to PnP_TempControlComponent_CommandCallback.
    else if ((clientResult = IoTHubDeviceClient_LL_SubscribeToCommands(deviceClient, PnP_TempControlComponent_CommandCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubDeviceClient_LL_PnP_SetCommandCallback failed, result=%d", clientResult);
        result = false;
    }
    // Retrieves all properties and also subscribes to property updates, routing them all to PnP_TempControlComponent_UpdatedPropertyCallback.
    else if ((clientResult = IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync(deviceClient, PnP_TempControlComponent_UpdatedPropertyCallback, (void*)deviceClient)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubDeviceClient_PnP_SetPropertyCallback failed, result=%d", clientResult);
        result = false;
    }
    // PnP_ThermostatComponent_CreateHandle creates handles to process the subcomponents of Temperature Controller (namely
    // thermostat1 and thermostat2) that require state and need to process callbacks from IoT Hub.  The other component,
    // deviceInfo, is so simple (one time send of data) as to not need a handle.
    else if ((g_thermostatHandle1 = PnP_ThermostatComponent_CreateHandle(g_thermostatComponent1Name)) == NULL)
    {
        LogError("Unable to create component handle for %s", g_thermostatComponent1Name);
        result = false;
    }
    else if ((g_thermostatHandle2 = PnP_ThermostatComponent_CreateHandle(g_thermostatComponent2Name)) == NULL)
    {
        LogError("Unable to create component handle for %s", g_thermostatComponent2Name);
        result = false;
    }
    else
    {
        result = true;
    }

    if (result == false)
    {
        PnP_ThermostatComponent_Destroy(g_thermostatHandle2);
        PnP_ThermostatComponent_Destroy(g_thermostatHandle1);
        if (deviceClient != NULL)
        {
            IoTHubDeviceClient_LL_Destroy(deviceClient);
            IoTHub_Deinit();
            deviceClient = NULL;
        }
    }

    return deviceClient;
}

int main(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;

    if ((deviceClient = CreateDeviceClientAndAllocateComponents()) == NULL)
    {
        LogError("Failure creating IotHub device client");
    }
    else
    {
        LogInfo("Successfully created device client.  Hit Control-C to exit program\n");

        int numberOfIterations = 0;

        // During startup, send what DTDLv2 calls "read-only properties."  This are read-only from IoT Hub perspective.
        PnP_TempControlComponent_ReportSerialNumber_Property(deviceClient);
        PnP_DeviceInfoComponent_Report_All_Properties(g_deviceInfoComponentName, deviceClient);
        PnP_TempControlComponent_Report_MaxTempSinceLastReboot_Property(g_thermostatHandle1, deviceClient);
        PnP_TempControlComponent_Report_MaxTempSinceLastReboot_Property(g_thermostatHandle2, deviceClient);

        while (true)
        {
            // Wake up periodically to poll.  Even if we do not plan on sending telemetry, we still need to poll periodically in order to process
            // incoming requests from the server and to do connection keep alives.
            if ((numberOfIterations % g_sendTelemetryPollInterval) == 0)
            {
                PnP_TempControlComponent_SendWorkingSet(deviceClient);
                PnP_ThermostatComponent_SendTelemetry(g_thermostatHandle1, deviceClient);
                PnP_ThermostatComponent_SendTelemetry(g_thermostatHandle2, deviceClient);
            }

            IoTHubDeviceClient_LL_DoWork(deviceClient);
            ThreadAPI_Sleep(g_sleepBetweenPollsMs);
            numberOfIterations++;
        }

        // Free the memory allocated to track simulated thermostat.
        PnP_ThermostatComponent_Destroy(g_thermostatHandle2);
        PnP_ThermostatComponent_Destroy(g_thermostatHandle1);

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(deviceClient);
        // Free all the sdk subsystem
        IoTHub_Deinit();
    }

    return 0;
}
