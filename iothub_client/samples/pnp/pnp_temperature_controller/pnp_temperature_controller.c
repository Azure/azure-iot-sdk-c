// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample implements a PnP based Temperature Control model.  It demonstrates a complex 
// model in that the model has properties, commands, and telemetry off of the root component
// as well as subcomponents.

// The Digital Twin Definition Language (DTDLv2) document describing the device implemented in this sample
// is available at https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json

// Standard C header files.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// JSON parser library.
#include "parson.h"

// IoT Hub device client and IoT core utility related header files.
#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothub_message.h"
#include "iothub_client_properties.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "iothubtransportmqtt.h"

#include "pnp_status_values.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
// For devices that do not have (or want) an OS level trusted certificate store,
// but instead bring in default trusted certificates from the Azure IoT C SDK.
#include "azure_c_shared_utility/shared_util_options.h"
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#include "pnp_sample_config.h"

#ifdef USE_PROV_MODULE_FULL
#include "pnp_dps_ll.h"
#endif // USE_PROV_MODULE_FULL

// Headers that provide implementation for subcomponents (the two thermostat components and DeviceInfo)
#include "pnp_thermostat_component.h"
#include "pnp_deviceinfo_component.h"

#define CURRENT_WORKING_SET_BUFFER_SIZE 64

// Amount of time to sleep between polling hub, in milliseconds.  Set to wake up every 100 milliseconds.
static unsigned int g_sleepBetweenPollsMs = 100;

// Every time the main loop wakes up, on the g_sendTelemetryPollInterval(th) pass will send a telemetry message.
// So we will send telemetry every (g_sendTelemetryPollInterval * g_sleepBetweenPollsMs) milliseconds; 60 seconds as currently configured.
static const int g_sendTelemetryPollInterval = 600;

// Whether tracing at the IoT Hub client is enabled or not.
static bool g_hubClientTraceEnabled = true;

// DTMI indicating this device's model identifier.
static const char g_temperatureControllerModelId[] = "dtmi:com:example:TemperatureController;1";

// Metadata to add to telemetry messages.
static const char g_jsonContentType[] = "application/json";
static const char g_utf8EncodingType[] = "utf8";

// PNP_THERMOSTAT_COMPONENT_HANDLE represent the thermostat components that are sub-components of the temperature controller.
// Note that we do NOT have an analogous DeviceInfo component handle because there is only DeviceInfo subcomponent and its
// implementation is straightforward.
PNP_THERMOSTAT_COMPONENT_HANDLE g_thermostatHandle1;
PNP_THERMOSTAT_COMPONENT_HANDLE g_thermostatHandle2;

// Name of subcomponents that TemmperatureController implements.
static const char g_thermostatComponent1Name[] = "thermostat1";
static const char g_thermostatComponent2Name[] = "thermostat2";
static const char g_deviceInfoComponentName[] = "deviceInformation";

// Command implemented by the TemperatureControl component itself to implement reboot.
static const char g_rebootCommand[] = "reboot";

// An empty JSON body for PnP command responses.
static const char g_JSONEmpty[] = "{}";
static const size_t g_JSONEmptySize = sizeof(g_JSONEmpty) - 1;

// Minimum value we will return for working set, + some random number.
static const int g_workingSetMinimum = 1000;
// Random number for working set will range between the g_workingSetMinimum and (g_workingSetMinimum+g_workingSetRandomModulo).
static const int g_workingSetRandomModulo = 500;
// Format string for sending a telemetry message with the working set.
static const char g_workingSetTelemetryFormat[] = "{\"workingSet\":%d}";

// Name of the serial number property.
static const char g_serialNumberPropertyName[] = "serialNumber";
// Value of the serial number.  NOTE: This must be a legal JSON string which requires value to be in "...".
static const char g_serialNumberPropertyValue[] = "\"serial-no-123-abc\"";

// Values of connection / security settings read from environment variables and/or DPS runtime.
PNP_DEVICE_CONFIGURATION g_pnpDeviceConfiguration;

//
// CopyPayloadToString creates a null-terminated string from the payload buffer.
// payload is not guaranteed to be null-terminated by the IoT Hub device SDK.
//
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
// SetEmptyCommandResponse sets the response to be an empty JSON.  IoT Hub wants
// legal JSON, regardless of error status, so if command implementation did not set this do so here.
//
static void SetEmptyCommandResponse(IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse)
{
    if ((commandResponse->payload = calloc(1, g_JSONEmptySize)) == NULL)
    {
        LogError("Unable to allocate empty JSON response");
        commandResponse->statusCode = PNP_STATUS_INTERNAL_ERROR;
    }
    else
    {
        memcpy(commandResponse->payload, g_JSONEmpty, g_JSONEmptySize);
        commandResponse->payloadLength = g_JSONEmptySize;
        // We only overwrite the caller's result on error; otherwise leave as it was
    }
}

//
// PnP_TempControlComponent_InvokeRebootCommand processes the reboot command on the root component.
//
static void PnP_TempControlComponent_InvokeRebootCommand(JSON_Value* rootValue, IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse)
{
    if (json_value_get_type(rootValue) != JSONNumber)
    {
        LogError("Delay payload is not a number");
        commandResponse->statusCode = PNP_STATUS_BAD_FORMAT;
    }
    else
    {
        // See caveats section in ../readme.md; we don't actually respect the delay value to keep the sample simple.
        int delayInSeconds = (int)json_value_get_number(rootValue);
        LogInfo("Temperature controller 'reboot' command invoked with delay %d seconds", delayInSeconds);
        commandResponse->statusCode = PNP_STATUS_SUCCESS;
    }
}

//
// PnP_TempControlComponent_CommandCallback is invoked by IoT SDK when a command arrives.
//
static void PnP_TempControlComponent_CommandCallback(const IOTHUB_CLIENT_COMMAND_REQUEST* commandRequest, IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse, void* userContextCallback)
{
    (void)userContextCallback;

    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;

    LogInfo("Device command %s arrived for component %s", commandRequest->commandName, (commandRequest->componentName == NULL) ? "" : commandRequest->componentName);

    // Because the payload isn't null-terminated, create one here so parson can process it.
    if ((jsonStr = CopyPayloadToString(commandRequest->payload, commandRequest->payloadLength)) == NULL)
    {
        LogError("Unable to allocate buffer");
        commandResponse->statusCode = PNP_STATUS_INTERNAL_ERROR;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse JSON");
        commandResponse->statusCode = PNP_STATUS_INTERNAL_ERROR;
    }
    else
    {
        // Route the incoming command to the appropriate component to process it.
        if (commandRequest->componentName != NULL)
        {
            if (strcmp(commandRequest->componentName, g_thermostatComponent1Name) == 0)
            {
                PnP_ThermostatComponent_ProcessCommand(g_thermostatHandle1, commandRequest->commandName, rootValue, commandResponse);
            }
            else if (strcmp(commandRequest->componentName, g_thermostatComponent2Name) == 0)
            {
                PnP_ThermostatComponent_ProcessCommand(g_thermostatHandle2, commandRequest->commandName, rootValue, commandResponse);
            }
            else
            {
                LogError("Component %s is not supported by TemperatureController", commandRequest->componentName);
                commandResponse->statusCode = PNP_STATUS_NOT_FOUND;
            }
        }
        else
        {
            // Command has arrived on the root component
            if (strcmp(commandRequest->commandName, g_rebootCommand) == 0)
            {
                PnP_TempControlComponent_InvokeRebootCommand(rootValue, commandResponse);
            }
            else
            {
                LogError("Command %s is not supported by TemperatureController", commandRequest->commandName);
                commandResponse->statusCode= PNP_STATUS_NOT_FOUND;
            }
        }
    }

    if (commandResponse->payload == NULL)
    {
        SetEmptyCommandResponse(commandResponse);
    }

    json_value_free(rootValue);
    free(jsonStr);
}

//
// PnP_TempControlComponent_UpdatedPropertyCallback is invoked when properties arrive from the server.
//
static void PnP_TempControlComponent_UpdatedPropertyCallback(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, 
    const unsigned char* payload,
    size_t payloadLength,
    void* userContextCallback)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = (IOTHUB_DEVICE_CLIENT_LL_HANDLE)userContextCallback;
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE propertiesReader = NULL;
    IOTHUB_CLIENT_PROPERTY_PARSED property;
    int propertiesVersion;
    IOTHUB_CLIENT_RESULT clientResult;

    // The properties arrive as a raw JSON buffer (which is not null-terminated).  IoTHubClient_Properties_Deserializer_Create parses 
    // this into a more convenient form to allow property-by-property enumeration over the updated properties.
    if ((clientResult = IoTHubClient_Properties_Deserializer_Create(payloadType, payload, payloadLength, &propertiesReader)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubClient_Deserialize_Properties failed, error=%d", clientResult);
    }
    else if ((clientResult = IoTHubClient_Properties_Deserializer_GetVersion(propertiesReader, &propertiesVersion)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubClient_Properties_Deserializer_GetVersion failed, error=%d", clientResult);
    }
    else
    {
        bool propertySpecified;
        property.structVersion = IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1;

        while ((clientResult = IoTHubClient_Properties_Deserializer_GetNext(propertiesReader, &property, &propertySpecified)) == IOTHUB_CLIENT_OK)
        {
            if (propertySpecified == false)
            {
                break;
            }

            if (property.propertyType == IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_CLIENT)
            {
                // We are iterating over a property that the device has previously sent to IoT Hub; 
                // this shows what IoT Hub has recorded the reported property as.
                //
                // There are scenarios where a device may use this, such as knowing whether the
                // given property has changed on the device and needs to be re-reported.
                //
                // This sample doesn't do anything with this, so we'll continue on when we hit reported properties.
                continue;
            }

            // Process IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE propertyType, which means IoT Hub is configuring a property
            // on this device.
            //
            // If we receive a component or property the model does not support, log the condition locally but do not report this
            // back to IoT Hub.
            if (property.componentName == NULL) 
            {   
                LogError("Property %s arrived for TemperatureControl component itself.  This does not support properties on it (all properties are on subcomponents)", property.componentName);
            }
            else if (strcmp(property.componentName, g_thermostatComponent1Name) == 0)
            {
                PnP_ThermostatComponent_ProcessPropertyUpdate(g_thermostatHandle1, deviceClient, property.name, property.value.str, propertiesVersion);
            }
            else if (strcmp(property.componentName, g_thermostatComponent2Name) == 0)
            {
                PnP_ThermostatComponent_ProcessPropertyUpdate(g_thermostatHandle2, deviceClient, property.name, property.value.str, propertiesVersion);
            }
            else
            {
                LogError("Component %s is not implemented by the TemperatureController", property.componentName);
            }
            
            IoTHubClient_Properties_DeserializerProperty_Destroy(&property);
        }
    }

    IoTHubClient_Properties_Deserializer_Destroy(propertiesReader);
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
    IOTHUB_CLIENT_RESULT iothubClientResult;
    char workingSetTelemetryPayload[CURRENT_WORKING_SET_BUFFER_SIZE];

    int workingSet = g_workingSetMinimum + (rand() % g_workingSetRandomModulo);

    // Create the telemetry message body to send.
    if (snprintf(workingSetTelemetryPayload, sizeof(workingSetTelemetryPayload), g_workingSetTelemetryFormat, workingSet) < 0)
    {
        LogError("Unable to create a workingSet telemetry payload string");
    }
    // Create the message handle and specify its metadata.
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
    // Send the telemetry message.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SendTelemetryAsync(deviceClient, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send telemetry message, error=%d", iothubClientResult);
    }

    IoTHubMessage_Destroy(messageHandle);
}

//
// PnP_TempControlComponent_ReportSerialNumber_Property sends the "serialNumber" property to IoT Hub.
//
static void PnP_TempControlComponent_ReportSerialNumber_Property(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient)
{
    IOTHUB_CLIENT_RESULT clientResult;
    IOTHUB_CLIENT_PROPERTY_REPORTED property = { IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, g_serialNumberPropertyName, g_serialNumberPropertyValue };
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength;

    // The first step of reporting properties is to serialize it into an IoT Hub friendly format.  You can do this by either
    // implementing the PnP convention for building up the correct JSON or more simply to use IoTHubClient_Properties_Serializer_CreateReported.
    if ((clientResult = IoTHubClient_Properties_Serializer_CreateReported(&property, 1, NULL, &serializedProperties, &serializedPropertiesLength)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to serialize reported state, error=%d", clientResult);
    }
    // The output of IoTHubClient_Properties_Serializer_CreateReported is sent to IoTHubDeviceClient_LL_SendPropertiesAsync to perform network I/O.
    else if ((clientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClient, serializedProperties, serializedPropertiesLength, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send reported state, error=%d", clientResult);
    }

    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

//
// CreateDeviceClientLLHandle creates the IOTHUB_DEVICE_CLIENT_LL_HANDLE based on environment configuration.
// If PNP_CONNECTION_SECURITY_TYPE_DPS is used, the call will block until DPS provisions the device.
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE CreateDeviceClientLLHandle(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;

    if (g_pnpDeviceConfiguration.securityType == PNP_CONNECTION_SECURITY_TYPE_CONNECTION_STRING)
    {
        if ((deviceClient = IoTHubDeviceClient_LL_CreateFromConnectionString(g_pnpDeviceConfiguration.u.connectionString, MQTT_Protocol)) == NULL)
        {
            LogError("Failure creating Iot Hub client.  Hint: Check your connection string");
        }
    }
#ifdef USE_PROV_MODULE_FULL
    else if ((deviceClient = PnP_CreateDeviceClientLLHandle_ViaDps(&g_pnpDeviceConfiguration)) == NULL)
    {
        LogError("Cannot retrieve IoT Hub connection information from DPS client");
    }
#endif /* USE_PROV_MODULE_FULL */

    return deviceClient;
}

//
// CreateAndConfigureDeviceClientHandleForPnP creates a IOTHUB_DEVICE_CLIENT_LL_HANDLE for this application, setting its
// ModelId along with various callbacks.
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE CreateAndConfigureDeviceClientHandleForPnP(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;
    IOTHUB_CLIENT_RESULT iothubClientResult;
    bool urlAutoEncodeDecode = true;
    int iothubInitResult;
    bool result;

    // Before invoking any IoT Hub Device SDK functionality, IoTHub_Init must be invoked.
    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        LogError("Failure to initialize client, error=%d", iothubInitResult);
        result = false;
    }
    // Create the deviceClient.
    else if ((deviceClient = CreateDeviceClientLLHandle()) == NULL)
    {
        LogError("Failure creating Iot Hub client.  Hint: Check your connection string or DPS configuration");
        result = false;
    }
    // Sets verbosity level.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_LOG_TRACE, &g_hubClientTraceEnabled)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set logging option, error=%d", iothubClientResult);
        result = false;
    }
    // Sets the name of ModelId for this PnP device.
    // This *MUST* be set before the client is connected to IoTHub.  We do not automatically connect when the 
    // handle is created, but will implicitly connect to subscribe for command and property callbacks below.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_MODEL_ID, g_temperatureControllerModelId)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the ModelID, error=%d", iothubClientResult);
        result = false;
    }
    // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically for telemetry message properties.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set auto Url encode option, error=%d", iothubClientResult);
        result = false;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on systems without built in certificate stores.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the trusted cert, error=%d", iothubClientResult);
        result = false;
    }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    // Sets the callback function that processes incoming commands.  Note that this will implicitly initiate a connection to IoT Hub.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SubscribeToCommands(deviceClient, PnP_TempControlComponent_CommandCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to subscribe for commands, error=%d", iothubClientResult);
        result = false;
    }
    // Retrieve all properties for the device and also subscribe for any future writable property update changes.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync(deviceClient, PnP_TempControlComponent_UpdatedPropertyCallback, (void*)deviceClient)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to subscribe and get properties, error=%d", iothubClientResult);
        result = false;
    }
    else
    {
        result = true;
    }

    if (result == false)
    {
        IoTHubDeviceClient_LL_Destroy(deviceClient);
        deviceClient = NULL;

        if (iothubInitResult == 0)
        {
            IoTHub_Deinit();
        }
    }

    return deviceClient;
}

//
// AllocateThermostatComponents allocates subcomponents of this module.  These are implemented in separate .c 
// files and the thermostat components have handles to simulate how a more complex PnP device might be composed.
// 
static bool AllocateThermostatComponents(void)
{
    bool result;

    // PnP_ThermostatComponent_CreateHandle creates handles to process the subcomponents of Temperature Controller (namely
    // thermostat1 and thermostat2) that require state and need to process callbacks from IoT Hub.  The other component,
    // deviceInfo, is so simple (one time send of data) as to not need a handle.
    if ((g_thermostatHandle1 = PnP_ThermostatComponent_CreateHandle(g_thermostatComponent1Name)) == NULL)
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
        PnP_ThermostatComponent_Destroy(g_thermostatHandle1);
        PnP_ThermostatComponent_Destroy(g_thermostatHandle2);
    }

    return result;
}

int main(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;

    g_pnpDeviceConfiguration.modelId = g_temperatureControllerModelId;
    g_pnpDeviceConfiguration.enableTracing = g_hubClientTraceEnabled;

    // First determine the IoT Hub / credentials / device to use.
    if (GetConnectionSettingsFromEnvironment(&g_pnpDeviceConfiguration) == false)
    {
        LogError("Cannot read required environment variable(s)");
    }
    // Creates the thermostat subcomponents defined by this model.  Since everything
    // is simulated, this setup stage just creates simulated objects in memory.
    else if (AllocateThermostatComponents() == false)
    {
        LogError("Failure allocating thermostat components");
    }
    // Create a handle to device client handle.  Note that this call may block
    // for extended periods of time when using DPS.
    else if ((deviceClient = CreateAndConfigureDeviceClientHandleForPnP()) == NULL)
    {
        LogError("Failure creating Iot Hub device client");
        PnP_ThermostatComponent_Destroy(g_thermostatHandle1);
        PnP_ThermostatComponent_Destroy(g_thermostatHandle2);
    }
    else
    {
        LogInfo("Successfully created device client.  Hit Control-C to exit program\n");

        int numberOfIterations = 0;

        // During startup, send what DTDLv2 calls "read-only properties" to indicate initial device state.
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
                PnP_ThermostatComponent_SendCurrentTemperature(g_thermostatHandle1, deviceClient);
                PnP_ThermostatComponent_SendCurrentTemperature(g_thermostatHandle2, deviceClient);
            }

            IoTHubDeviceClient_LL_DoWork(deviceClient);
            ThreadAPI_Sleep(g_sleepBetweenPollsMs);
            numberOfIterations++;
        }

        // The remainder of the code is used for cleaning up our allocated resources. It won't be executed in this 
        // sample (because the loop above is infinite and is only broken out of by Control-C of the program), but 
        // it is included for reference.

        // Free the memory allocated to track simulated thermostat.
        PnP_ThermostatComponent_Destroy(g_thermostatHandle1);
        PnP_ThermostatComponent_Destroy(g_thermostatHandle2);

        // Clean up the IoT Hub SDK handle.
        IoTHubDeviceClient_LL_Destroy(deviceClient);
        // Free all IoT Hub subsystem.
        IoTHub_Deinit();
    }

    return 0;
}
