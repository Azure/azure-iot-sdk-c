// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample implements an IoT Plug and Play based thermostat.  This demonstrates a *relatively* simple PnP device
// that only acts as a thermostat and does not have additional components.  

// The Digital Twin Definition Language (DTDLv2) document describing the device implemented in this sample
// is available at https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json.

// Standard C header files.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <math.h>

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

// Size of buffer to store ISO 8601 time.
#define TIME_BUFFER_SIZE 128
// Size of buffer to store current temperature telemetry.
#define CURRENT_TEMPERATURE_BUFFER_SIZE  32
// Size of buffer to store the maximum temp since reboot property.
#define MAX_TEMPERATURE_SINCE_REBOOT_BUFFER_SIZE 32

// Amount of time to sleep between polling hub, in milliseconds.  Set to wake up every 100 milliseconds.
static unsigned int g_sleepBetweenPollsMs = 100;

// Every time the main loop wakes up, on the g_sendTelemetryPollInterval(th) pass will send a telemetry message.
// So we will send telemetry every (g_sendTelemetryPollInterval * g_sleepBetweenPollsMs) milliseconds; 60 seconds as currently configured.
static const int g_sendTelemetryPollInterval = 600;

// Whether verbose tracing at the IoTHub client is enabled or not.
static bool g_hubClientTraceEnabled = true;

// DTMI indicating this device's model identifier.
static const char g_ThermostatModelId[] = "dtmi:com:example:Thermostat;1";

// Names of properties for desired/reporting
static const char g_targetTemperaturePropertyName[] = "targetTemperature";
static const char g_maxTempSinceLastRebootPropertyName[] = "maxTempSinceLastReboot";

// Name of command this component supports to get report information
static const char g_getMaxMinReportCommandName[] = "getMaxMinReport";

// An empty JSON body for PnP command responses.
static const char g_JSONEmpty[] = "{}";
static const size_t g_JSONEmptySize = sizeof(g_JSONEmpty) - 1;

// The default temperature to use before any is set.
#define DEFAULT_TEMPERATURE_VALUE 22

// Current temperature of the thermostat.
static double g_currentTemperature = DEFAULT_TEMPERATURE_VALUE;

// Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
static const char g_ISO8601Format[] = "%Y-%m-%dT%H:%M:%SZ";

// Format string for sending temperature telemetry.
static const char g_temperatureTelemetryBodyFormat[] = "{\"temperature\":%.02f}";

// Format string for building getMaxMinReport.
static const char g_maxMinCommandResponseFormat[] = "{\"maxTemp\":%.2f,\"minTemp\":%.2f,\"avgTemp\":%.2f,\"startTime\":\"%s\",\"endTime\":\"%s\"}";

// Format string for sending maxTempSinceLastReboot property.
static const char g_maxTempSinceLastRebootPropertyFormat[] = "%.2f";

// Metadata to add to telemetry messages.
static const char g_jsonContentType[] = "application/json";
static const char g_utf8EncodingType[] = "utf8";

// Response description is an optional, human readable message including more information
// about the setting of the temperature.  On success cases, this sample does not 
// send a description to save bandwidth but on error cases we'll provide some hints.
static const char g_temperaturePropertyResponseDescriptionNotInt[] = "desired temperature is not a number";

// Minimum temperature thermostat has been at during current execution run.
static double g_minTemperature = DEFAULT_TEMPERATURE_VALUE;

// Maximum temperature thermostat has been at during current execution run.
static double g_maxTemperature = DEFAULT_TEMPERATURE_VALUE;

// Number of times temperature has been updated, counting the initial setting as 1.  Used to determine average temperature.
static int g_numTemperatureUpdates = 1;

// Total of all temperature updates during current execution run.  Used to determine average temperature.
static double g_allTemperatures = DEFAULT_TEMPERATURE_VALUE;

// Start time of the program, stored in ISO 8601 format string for UTC.
static char g_ProgramStartTime[TIME_BUFFER_SIZE];

// Values of connection / security settings read from environment variables and/or DPS runtime.
PNP_DEVICE_CONFIGURATION g_pnpDeviceConfiguration;

//
// CopyPayloadToString creates a null-terminated string from the payload buffer.
// payload is not guaranteed to be null-terminated by the IoT Hub device SDK.
//
static char* CopyPayloadToString(const unsigned char* payload, size_t size)
{
    char* jsonStr;

    if ((jsonStr = (char*)malloc(size+1)) == NULL)
    {
        LogError("Unable to allocate %lu size buffer", (unsigned long)(size+1));
    }
    else
    {
        memcpy(jsonStr, payload, size);
        jsonStr[size] = '\0';
    }

    return jsonStr;
}

//
// BuildUtcTimeFromCurrentTime writes the current time, in ISO 8601 format, into the specified buffer.
//
static bool BuildUtcTimeFromCurrentTime(char* utcTimeBuffer, size_t utcTimeBufferSize)
{
    bool result;
    time_t currentTime;
    struct tm * currentTimeTm;

    time(&currentTime);
    currentTimeTm = gmtime(&currentTime);

    if (strftime(utcTimeBuffer, utcTimeBufferSize, g_ISO8601Format, currentTimeTm) == 0)
    {
        LogError("snprintf on UTC time failed");
        result = false;
    }
    else
    {
        result = true;
    }

    return result;
}

//
// BuildMaxMinCommandResponse builds the response to the command for getMaxMinReport.
//
static bool BuildMaxMinCommandResponse(IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse)
{
    int responseBuilderSize = 0;
    unsigned char* responseBuilder = NULL;
    bool result;
    char currentTime[TIME_BUFFER_SIZE];

    if (BuildUtcTimeFromCurrentTime(currentTime, sizeof(currentTime)) == false)
    {
        LogError("Unable to output the current time");
        result = false;
    }
    else if ((responseBuilderSize = snprintf(NULL, 0, g_maxMinCommandResponseFormat, g_maxTemperature, g_minTemperature, g_allTemperatures / g_numTemperatureUpdates, g_ProgramStartTime, currentTime)) < 0)
    {
        LogError("snprintf to determine string length for command response failed");
        result = false;
    }
    // We must allocate the response buffer.  It is returned to the IoTHub SDK in the command callback and the SDK in turn sends this to the server.  
    // The SDK takes responsibility for the buffer and will free it.
    else if ((responseBuilder = calloc(1, (size_t)responseBuilderSize + 1)) == NULL)
    {
        LogError("Unable to allocate %lu bytes", (unsigned long)(responseBuilderSize + 1));
        result = false;
    }
    else if ((responseBuilderSize = snprintf((char*)responseBuilder, (size_t)responseBuilderSize + 1, g_maxMinCommandResponseFormat, g_maxTemperature, g_minTemperature, g_allTemperatures / g_numTemperatureUpdates, g_ProgramStartTime, currentTime)) < 0)
    {
        LogError("snprintf to output buffer for command response");
        result = false;
    }
    else
    {
        result = true;
    }

    if (result == true)
    {
        commandResponse->payload = responseBuilder;
        commandResponse->payloadLength = (size_t)responseBuilderSize;
        LogInfo("Response=<%s>", (const char*)responseBuilder);
    }
    else
    {
        free(responseBuilder);
    }

    return result;
}       

//
// SetEmptyCommandResponse sets the response to be an empty JSON.  IoT Hub needs
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
// Thermostat_CommandCallback is invoked by IoT SDK when a command arrives.
//
static void Thermostat_CommandCallback(const IOTHUB_CLIENT_COMMAND_REQUEST* commandRequest, IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse, void* userContextCallback)
{
    (void)userContextCallback;

    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    const char* sinceStr;

    LogInfo("Device command %s arrived", commandRequest->commandName);

    if (commandRequest->componentName != NULL)
    {
        LogError("This model only supports root components, but component %s was specified in command", commandRequest->componentName);
        commandResponse->statusCode = PNP_STATUS_NOT_FOUND;
    }
    else if (strcmp(commandRequest->commandName, g_getMaxMinReportCommandName) != 0)
    {
        LogError("Command name %s is not supported on this component", commandRequest->commandName);
        commandResponse->statusCode = PNP_STATUS_NOT_FOUND;
    }
    // Because the payload isn't null-terminated, create one here so parson can process it.
    else if ((jsonStr = CopyPayloadToString(commandRequest->payload, commandRequest->payloadLength)) == NULL)
    {
        LogError("Unable to allocate buffer");
        commandResponse->statusCode = PNP_STATUS_INTERNAL_ERROR;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse JSON");
        commandResponse->statusCode = PNP_STATUS_BAD_FORMAT;
    }
    // See caveats section in ../readme.md; we don't actually respect this sinceStr to keep the sample simple.
    else if ((sinceStr = json_value_get_string(rootValue)) == NULL)
    {
        LogError("Cannot retrieve 'since' value");
        commandResponse->statusCode = PNP_STATUS_BAD_FORMAT;
    }
    else if (BuildMaxMinCommandResponse(commandResponse) == false)
    {
        LogError("Unable to build response");
        commandResponse->statusCode = PNP_STATUS_INTERNAL_ERROR;
    }
    else
    {
        LogInfo("Returning success from command request");
        commandResponse->statusCode = PNP_STATUS_SUCCESS;
    }

    if (commandResponse->payload == NULL)
    {
        SetEmptyCommandResponse(commandResponse);
    }

    json_value_free(rootValue);
    free(jsonStr);
}

//
// UpdateTemperatureAndStatistics updates the temperature and min/max/average values.
//
static void UpdateTemperatureAndStatistics(double desiredTemp, bool* maxTempUpdated)
{
    if (desiredTemp > g_maxTemperature)
    {
        g_maxTemperature = desiredTemp;
        *maxTempUpdated = true;
    }
    else if (desiredTemp < g_minTemperature)
    {
        g_minTemperature = desiredTemp;
    }

    g_numTemperatureUpdates++;
    g_allTemperatures += desiredTemp;
    g_currentTemperature = desiredTemp;
}

//
// SendTargetTemperatureResponse sends a PnP property indicating the device has received the desired targeted temperature.
//
static void SendTargetTemperatureResponse(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient, const char* desiredTemperatureString, int responseStatus, int version, const char* description)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;

    // IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE specifies the response to a desired temperature.
    IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE desiredTemperatureResponse;
    memset(&desiredTemperatureResponse, 0, sizeof(desiredTemperatureResponse));
    // Specify the structure version (not to be confused with the $version on IoT Hub) to protect back-compat in case the structure adds fields.
    desiredTemperatureResponse.structVersion = IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1;
    // This represents the version of the request from IoT Hub.  It needs to be returned so service applications can determine
    // what current version of the writable property the device is currently using, as the server may update the property even when the device
    // is offline.
    desiredTemperatureResponse.ackVersion = version;
    // Result of request, which maps to HTTP status code.
    desiredTemperatureResponse.result = responseStatus;
    desiredTemperatureResponse.name = g_targetTemperaturePropertyName;
    desiredTemperatureResponse.value = desiredTemperatureString;
    desiredTemperatureResponse.description = description;

    unsigned char* propertySerialized = NULL;
    size_t propertySerializedLength;

    // The first step of reporting properties is to serialize IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE into JSON for sending.
    if ((iothubClientResult = IoTHubClient_Properties_Serializer_CreateWritableResponse(&desiredTemperatureResponse, 1, NULL, &propertySerialized, &propertySerializedLength)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to serialize updated property, error=%d", iothubClientResult);
    }
    // The output of IoTHubClient_Properties_Serializer_CreateWritableResponse is sent to IoTHubDeviceClient_LL_SendPropertiesAsync to perform network I/O.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClient, propertySerialized, propertySerializedLength, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send updated property, error=%d", iothubClientResult);
    }
    else
    {
        LogInfo("Sending acknowledgement of property to IoTHub");
    }
    IoTHubClient_Properties_Serializer_Destroy(propertySerialized);
}

//
// SendMaxTemperatureSinceReboot reports a PnP property indicating the maximum temperature since the last reboot.
//
static void SendMaxTemperatureSinceReboot(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    char maximumTemperatureAsString[MAX_TEMPERATURE_SINCE_REBOOT_BUFFER_SIZE];

    if (snprintf(maximumTemperatureAsString, sizeof(maximumTemperatureAsString), g_maxTempSinceLastRebootPropertyFormat, g_maxTemperature) < 0)
    {
        LogError("Unable to create max temp since last reboot string for reporting result");
    }
    else
    {
        IOTHUB_CLIENT_PROPERTY_REPORTED maxTempProperty;
        maxTempProperty.structVersion = IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1;
        maxTempProperty.name = g_maxTempSinceLastRebootPropertyName;
        maxTempProperty.value =  maximumTemperatureAsString;

        unsigned char* propertySerialized = NULL;
        size_t propertySerializedLength;

        // The first step of reporting properties is to serialize IOTHUB_CLIENT_PROPERTY_REPORTED into JSON for sending.
        if ((iothubClientResult = IoTHubClient_Properties_Serializer_CreateReported(&maxTempProperty, 1, NULL, &propertySerialized, &propertySerializedLength)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to serialize reported state, error=%d", iothubClientResult);
        }
        // The output of IoTHubClient_Properties_Serializer_CreateReported is sent to IoTHubDeviceClient_LL_SendPropertiesAsync to perform network I/O.
        else if ((iothubClientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClient, propertySerialized, propertySerializedLength,  NULL, NULL)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to send reported state, error=%d", iothubClientResult);
        }
        else
        {
            LogInfo("Sending %s property to IoTHub for component", g_maxTempSinceLastRebootPropertyName);
        }
        IoTHubClient_Properties_Serializer_Destroy(propertySerialized);
    }
}

//
// Thermostat_ProcessTargetTemperature processes a writable update for desired temperature property.
//
static void Thermostat_ProcessTargetTemperature(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient, IOTHUB_CLIENT_PROPERTY_PARSED* property, int propertiesVersion)
{
    char* next;
    double targetTemperature = strtod(property->value.str, &next);
    if ((property->value.str == next) || (targetTemperature == HUGE_VAL) || (targetTemperature == (-1*HUGE_VAL)))
    {
        LogError("Property %s is not a valid number", property->value.str);
        SendTargetTemperatureResponse(deviceClient, property->value.str, PNP_STATUS_BAD_FORMAT, propertiesVersion, g_temperaturePropertyResponseDescriptionNotInt);
    }
    else
    {
        LogInfo("Received targetTemperature = %f", targetTemperature);
        
        bool maxTempUpdated = false;
        UpdateTemperatureAndStatistics(targetTemperature, &maxTempUpdated);
        
        // The device needs to let the service know that it has received the targetTemperature desired property.
        SendTargetTemperatureResponse(deviceClient, property->value.str, PNP_STATUS_SUCCESS, propertiesVersion, NULL);
        
        if (maxTempUpdated)
        {
            // If the maximum temperature has been updated, we also report this as a property.
            SendMaxTemperatureSinceReboot(deviceClient);
        }
    }
}

//
// Thermostat_PropertiesCallback is invoked when properties arrive from the server.
//
static void Thermostat_PropertiesCallback(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,  const unsigned char* payload, size_t payloadLength, void* userContextCallback)
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
                // We are iterating over a property that the device has previously sent to IoT Hub.
                //
                // There are scenarios where a device may use this, such as knowing whether the
                // given property has changed on the device and needs to be re-reported to IoT Hub.
                //
                // This sample doesn't do anything with this, so we'll continue on when we hit reported properties.
                continue;
            }

            // Process IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE propertyType, which means IoT Hub is configuring a property
            // on this device.
            //
            // If we receive a property the model does not support, log the condition locally but do not report this
            // back to IoT Hub.
            if (property.componentName != NULL)
            {   
                LogError("Property %s arrived for non-root component %s.  This model does not support such properties", property.name, property.componentName);
            }
            else if (strcmp(property.name, g_targetTemperaturePropertyName) == 0)
            {
                Thermostat_ProcessTargetTemperature(deviceClient, &property, propertiesVersion);
            }
            else
            {
                LogError("Property %s is not part of the thermostat model and will be ignored", property.name);
            }
            
            IoTHubClient_Properties_DeserializerProperty_Destroy(&property);
        }
    }

    IoTHubClient_Properties_Deserializer_Destroy(propertiesReader);
}

//
// Thermostat_SendCurrentTemperature sends a telemetry message indicating the current temperature.
//
void Thermostat_SendCurrentTemperature(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_MESSAGE_RESULT messageResult;
    IOTHUB_CLIENT_RESULT iothubClientResult;

    char temperatureStringBuffer[CURRENT_TEMPERATURE_BUFFER_SIZE];

    // Create the telemetry message body to send.
    if (snprintf(temperatureStringBuffer, sizeof(temperatureStringBuffer), g_temperatureTelemetryBodyFormat, g_currentTemperature) < 0)
    {
        LogError("snprintf of current temperature telemetry failed");
    }
    // Create the message handle and specify its metadata.
    else if ((messageHandle = IoTHubMessage_CreateFromString(temperatureStringBuffer)) == NULL)
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
            LogError("Failure creating IotHub client.  Hint: Check your connection string");
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

    // Before invoking any IoTHub Device SDK functionality, IoTHub_Init must be invoked.
    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        LogError("Failure to initialize client, error=%d", iothubInitResult);
        result = false;
    }
    // Create the deviceClient.
    else if ((deviceClient = CreateDeviceClientLLHandle()) == NULL)
    {
        LogError("Failure creating IotHub client.  Hint: Check your connection string or DPS configuration");
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
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_MODEL_ID, g_ThermostatModelId)) != IOTHUB_CLIENT_OK)
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
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SubscribeToCommands(deviceClient, Thermostat_CommandCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to subscribe for commands, error=%d", iothubClientResult);
        result = false;
    }
    // Retrieve all properties for the device and also subscribe for any future writable property update changes.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync(deviceClient, Thermostat_PropertiesCallback, (void*)deviceClient)) != IOTHUB_CLIENT_OK)
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
    }

    if ((result == false) &&  (iothubInitResult == 0))
    {
        IoTHub_Deinit();
    }

    return deviceClient;
}

int main(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;

    g_pnpDeviceConfiguration.modelId = g_ThermostatModelId;
    g_pnpDeviceConfiguration.enableTracing = g_hubClientTraceEnabled;

    // First determine the IoT Hub / credentials / device to use.
    if (GetConnectionSettingsFromEnvironment(&g_pnpDeviceConfiguration) == false)
    {
        LogError("Cannot read required environment variable(s)");
    }
    else if (BuildUtcTimeFromCurrentTime(g_ProgramStartTime, sizeof(g_ProgramStartTime)) == false)
    {
        LogError("Unable to output the program start time");
    }
    // Create a handle to device client handle.  Note that this call may block
    // for extended periods of time when using DPS.
    else if ((deviceClient = CreateAndConfigureDeviceClientHandleForPnP()) == NULL)
    {
        LogError("Failed creating IotHub device client");
    }
    else
    {
        LogInfo("Successfully created device client handle.  Hit Control-C to exit program\n");

        int numberOfIterations = 0;
        // During startup, send what DTDLv2 calls "read-only properties" to indicate initial device state.
        SendMaxTemperatureSinceReboot(deviceClient);

        while (true)
        {
            // Wake up periodically to poll.  Even if we do not plan on sending telemetry, we still need to poll periodically in order to process
            // incoming requests from the server and to do connection keep alives.
            if ((numberOfIterations % g_sendTelemetryPollInterval) == 0)
            {
                Thermostat_SendCurrentTemperature(deviceClient);
            }

            IoTHubDeviceClient_LL_DoWork(deviceClient);
            ThreadAPI_Sleep(g_sleepBetweenPollsMs);
            numberOfIterations++;
        }

        // The remainder of the code is used for cleaning up our allocated resources. It won't be executed in this 
        // sample (because the loop above is infinite and // is only broken out of by Control-C of the program), but 
        // it is included for reference.

        // Clean up the IoT Hub SDK handle.
        IoTHubDeviceClient_LL_Destroy(deviceClient);
        // Free all IoT Hub SDK subsystem.
        IoTHub_Deinit();        
    }

    return 0;
}
