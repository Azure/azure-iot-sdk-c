// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample implements a PnP based thermostat.  This demonstrates a *relatively* simple PnP device
// that does only acts as a thermostat and does not have additional components.  

// The DigitalTwin Definition Language document describing the component implemented in this sample
// is available at https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json.

// Standard C header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// IoTHub Device Client and IoT core utility related header files
#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_message.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "azure_c_shared_utility/shared_util_options.h"
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

// JSON parser library
#include "parson.h"

// Environment variable used to specify this application's connection string
static const char g_connectionStringEnvironmentVariable[] = "IOTHUB_DEVICE_CONNECTION_STRING";

// Amount of time to sleep between sending telemetry to IotHub, in milliseconds.  Set to 1 minute.
static unsigned int g_sleepBetweenTelemetrySends = 60 * 1000;

// Whether verbose tracing at the IoTHub client is enabled or not.
static bool g_hubClientTraceEnabled = true;

// This device's PnP ModelId.
static const char g_ThermostatModelId[] = "dtmi:com:example:Thermostat;1";

// JSON fields from desired property to retrieve.
static const char g_IoTHubTwinDesiredVersion[] = "$version";
static const char g_JSONTargetTemperature[] = "targetTemperature";

// Name of command this component supports to get report information
static const char g_getMinMaxReport[] = "getMaxMinReport";
// Return codes for device methods and desired property responses
static int g_statusSuccess = 200;
static int g_statusBadFormat = 400;
static int g_statusNotFoundStatus = 404;
static int g_statusInternalError = 500;

// The default temperature to use before any is set.
#define DEFAULT_TEMPERATURE_VALUE 22
// Current temperature of the thermostat.
static double g_currentTemperature = DEFAULT_TEMPERATURE_VALUE;
// Minimum temperature thermostat has been at during current execution run.
static double g_minTemperature = DEFAULT_TEMPERATURE_VALUE;
// Maximum temperature thermostat has been at during current execution run.
static double g_maxTemperature = DEFAULT_TEMPERATURE_VALUE;
// Number of times temperature has been updated, counting the initial setting as 1.  Used to determine average temperature.
static int g_numTemperatureUpdates = 1;
// Total of all temperature updates during current execution run.  Used to determine average temperature.
static double g_allTemperatures = DEFAULT_TEMPERATURE_VALUE;

// snprintf format for building getMaxMinReport
static const char g_minMaxCommandResponseFormat[] = "{\"maxTemp\":%.2f,\"minTemp\":%.2f,\"avgTemp\":%.2f,\"startTime\":\"%s\",\"endTime\":\"%s\"}";

// Format string for sending temperature telemetry
static const char g_temperatureTelemetryBodyFormat[] = "{\"temperature\":%.02f}";

// Format string to report the property maximum temperature since reboot.  This is a "read only" property from the
// service solution's perspective, which means we don't need to include any sort of status codes.
static const char g_maxTemperatureSinceRebootFormat[] = "{\"maxTempSinceLastReboot\":%.2f}";

// Format string to indicate the device received an update request for the temperature.  Because this is a "writeable"
// property from the service solution's perspective, we need to return a status code (HTTP status code style) and version
// for the solution to correlate the request and its status.
static const char g_targetTemperatureResponseFormat[] = "{\"targetTemperature\":{\"value\":%.2f,\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}}";

// Response description is an optional, human readable message including more information
// about the setting of the temperature.  Because we accept all temperature requests, we 
// always indicate a success.  An actual sensor could optionally return information about
// a temperature being out of range or a mechanical issue on the device on error cases.
static const char g_temperaturePropertyResponseDescription[] = "success";

// Size of buffer to store ISO 8601 time.
#define TIME_BUFFER_SIZE 128
// Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
static const char g_ISO8601Format[] = "%04d-%02d-%02dT%02d:%02d:%02dZ";
// Start time of the program, stored in ISO 8601 format string for UTC.
char g_ProgramStartTime[TIME_BUFFER_SIZE];


//
// CopyTwinPayloadToString takes the twin payload data, which arrives as a potentially non-NULL terminated string, and creates
// a new copy of the data with a NULL terminator.  The JSON parser this sample uses, parson, only operates over NULL terminated strings.
//
static char* CopyTwinPayloadToString(const unsigned char* payload, size_t size)
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
// BuildUtcTimeFromCurrentTime writes the current time, in ISO 8601 format, into the specified buffer
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
// BuildMaxMinCommandResponse builds the response to the command for getMaxMinReport
//
static bool BuildMaxMinCommandResponse(unsigned char** response, size_t* responseSize)
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
    else if ((responseBuilderSize = snprintf(NULL, 0, g_minMaxCommandResponseFormat, g_minTemperature, g_maxTemperature, g_allTemperatures / g_numTemperatureUpdates, g_ProgramStartTime, currentTime)) < 0)
    {
        LogError("snprintf to determine string length for command response failed");
        result = false;
    }
    // We MUST allocate the response buffer.  It is returned to the IoTHub SDK in the direct method callback and the SDK in turn sends this to the server.
    else if ((responseBuilder = calloc(1, responseBuilderSize + 1)) == NULL)
    {
        LogError("Unable to allocate %lu bytes", (unsigned long)(responseBuilderSize + 1));
        result = false;
    }
    else if ((responseBuilderSize = snprintf((char*)responseBuilder, responseBuilderSize + 1, g_minMaxCommandResponseFormat, g_minTemperature, g_maxTemperature, g_allTemperatures / g_numTemperatureUpdates, g_ProgramStartTime, currentTime)) < 0)
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
        *response = responseBuilder;
        *responseSize = (size_t)responseBuilderSize;
        LogInfo("Response=<%s>", (const char*)responseBuilder);
    }
    else
    {
        free(responseBuilder);
    }

    return result;
}       

//
// Thermostat_DeviceMethodCallback is invoked by IoT SDK when a device method arrives.
//
static int Thermostat_DeviceMethodCallback(const char* methodName, const unsigned char* payload, size_t size, unsigned char** response, size_t* responseSize, void* userContextCallback)
{
    (void)userContextCallback;

    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    const char* sinceStr;
    int result;

    LogInfo("Device method %s arrived", methodName);

    *response = NULL;
    *responseSize = 0;

    if (strcmp(methodName, g_getMinMaxReport) != 0)
    {
        LogError("Method name %s is not supported on this component", methodName);
        result = g_statusNotFoundStatus;
    }
    else if ((jsonStr = CopyTwinPayloadToString(payload, size)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
        result = g_statusInternalError;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse twin JSON");
        result = g_statusBadFormat;
    }
    // See caveats section in ../readme.md; we don't actually respect this sinceStr to keep the sample simple,
    // but want to demonstrate how to parse out in any case.
    else if ((sinceStr = json_value_get_string(rootValue)) == NULL)
    {
        LogError("Cannot retrieve since value");
        result = g_statusBadFormat;
    }
    else if (BuildMaxMinCommandResponse(response, responseSize) == false)
    {
        LogError("Unable to build response");
        result = g_statusInternalError;        
    }
    else
    {
        LogInfo("Returning success from command request");
        result = g_statusSuccess;
    }

    json_value_free(rootValue);
    free(jsonStr);

    return result;
}

//
// GetDesiredJson retrieves JSON_Object* in the JSON tree corresponding to the desired payload.
//
static JSON_Object* GetDesiredJson(DEVICE_TWIN_UPDATE_STATE updateState, JSON_Value* rootValue)
{
    JSON_Object* rootObject = NULL;
    JSON_Object* desiredObject;

    if ((rootObject = json_value_get_object(rootValue)) == NULL)
    {
        LogError("Unable to get root object of JSON");
        desiredObject = NULL;
    }
    else
    {
        if (updateState == DEVICE_TWIN_UPDATE_COMPLETE)
        {
            // For a complete update, the JSON from IoTHub will contain both "desired" and "reported" - the full twin.
            // We only care about "desired" in this sample, so just retrieve it.
            desiredObject = json_object_get_object(rootObject, "desired");
        }
        else
        {
            // For a patch update, IoTHub only sends "desired" portion of twin and does not explicitly put a "desired:" JSON envelope.
            // So here we simply need the root of the JSON itself.
            desiredObject = rootObject;
        }
    }

    return desiredObject;
}

//
// UpdateTemperatureAndStatistics updates the temperature and min/max/average values
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
// SendTargetTemperatureReport sends a PnP property indicating the device has received the desired targeted temperature
//
static void SendTargetTemperatureReport(IOTHUB_DEVICE_CLIENT_HANDLE deviceClient, double desiredTemp, int responseStatus, int version, const char* description)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    char targetTemperatureResponseProperty[256];

    if (snprintf(targetTemperatureResponseProperty, sizeof(targetTemperatureResponseProperty), g_targetTemperatureResponseFormat, desiredTemp, responseStatus, version, description) < 0)
    {
        LogError("snprintf building targetTemperature response failed");
    }
    else if ((iothubClientResult = IoTHubDeviceClient_SendReportedState(deviceClient, (const unsigned char*)targetTemperatureResponseProperty, strlen(targetTemperatureResponseProperty), NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send reported state for target temperature.  Error=%d", iothubClientResult);
    }
    else
    {
        LogInfo("Sending maxTempSinceReboot property");
    }
}

//
// SendMaxTemperatureSinceReboot reports a PnP property indicating the maximum temperature since the last reboot (simulated here by lifetime of executable)
//
static void SendMaxTemperatureSinceReboot(IOTHUB_DEVICE_CLIENT_HANDLE deviceClient)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    char maxTemperatureSinceRebootProperty[256];

    if (snprintf(maxTemperatureSinceRebootProperty, sizeof(maxTemperatureSinceRebootProperty), g_maxTemperatureSinceRebootFormat, g_maxTemperature) < 0)
    {
        LogError("snprintf building maxTemperature failed");
    }
    else if ((iothubClientResult = IoTHubDeviceClient_SendReportedState(deviceClient, (const unsigned char*)maxTemperatureSinceRebootProperty, strlen(maxTemperatureSinceRebootProperty), NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send reported state for maximum temperature.  Error=%d", iothubClientResult);
    }
    else
    {
        LogInfo("Sending maxTempSinceReboot property");
    }
}

//
// Thermostat_DeviceTwinCallback is invoked by the IoT SDK when a twin - either full twin or a PATCH update - arrives.
//
static void Thermostat_DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, void* userContextCallback)
{
    // The device handle associated with this request is passed as the context, since we will need to send reported events back.
    IOTHUB_DEVICE_CLIENT_HANDLE deviceClient = (IOTHUB_DEVICE_CLIENT_HANDLE)userContextCallback;

    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* desiredObject;
    JSON_Value* versionValue = NULL;
    JSON_Value* targetTemperatureValue = NULL;

    LogInfo("DeviceTwin callback invoked");

    if ((jsonStr = CopyTwinPayloadToString(payload, size)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse twin JSON");
    }
    else if ((desiredObject = GetDesiredJson(updateState, rootValue)) == NULL)
    {
        LogError("Cannot retrieve desired JSON object");
    }
    else if ((targetTemperatureValue = json_object_get_value(desiredObject, g_JSONTargetTemperature)) == NULL)
    {
        LogInfo("JSON property %s not specified.  This is NOT an error as the server doesn't need to set this, but there is no further action to take.", g_JSONTargetTemperature);
    }
    else if ((versionValue = json_object_get_value(desiredObject, g_IoTHubTwinDesiredVersion)) == NULL)
    {
        // The $version does need to be set in *any* legitimate twin desired document.  Its absence suggests 
        // something is fundamentally wrong with how we've received the twin and we should not proceed.
        LogError("Cannot retrieve field %s for twin.  The underlying IoTHub device twin protocol (NOT the service solution directly) should have specified this.", g_IoTHubTwinDesiredVersion);
    }
    else if (json_value_get_type(versionValue) != JSONNumber)
    {
        // The $version must be a number (and in practice an int) A non-numerical value indicates 
        // something is fundamentally wrong with how we've received the twin and we should not proceed.
        LogError("JSON field %s is not a number but must be", g_IoTHubTwinDesiredVersion);
    }
    else if (json_value_get_type(targetTemperatureValue) != JSONNumber)
    {
        LogError("JSON field %s is not a number", g_JSONTargetTemperature);
    }
    else
    {
        double targetTemperature = json_value_get_number(targetTemperatureValue);
        int version = (int)json_value_get_number(versionValue);

        LogInfo("Received targetTemperature = %f", targetTemperature);

        bool maxTempUpdated = false;
        UpdateTemperatureAndStatistics(targetTemperature, &maxTempUpdated);

        // The device needs to let the service know that it has received the targetTemperature desired property.
        SendTargetTemperatureReport(deviceClient, targetTemperature, g_statusSuccess, version, g_temperaturePropertyResponseDescription);

        if (maxTempUpdated)
        {
            // If the Maximum temperature has been updated, we also report this as a property.
            SendMaxTemperatureSinceReboot(deviceClient);
        }
    }

    json_value_free(rootValue);
    free(jsonStr);
}

//
// Thermostat_SendCurrentTemperature sends a PnP telemetry indicating the current temperature
//
void Thermostat_SendCurrentTemperature(IOTHUB_DEVICE_CLIENT_HANDLE deviceClient) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;

    char temperatureStringBuffer[32];

    if (snprintf(temperatureStringBuffer, sizeof(temperatureStringBuffer), g_temperatureTelemetryBodyFormat, g_currentTemperature) < 0)
    {
        LogError("snprintf of current temperature telemetry failed");
    }
    else if ((messageHandle = IoTHubMessage_CreateFromString(temperatureStringBuffer)) == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
    }
    else if ((iothubResult = IoTHubDeviceClient_SendEventAsync(deviceClient, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send telemetry message, error=%d", iothubResult);
    }

    IoTHubMessage_Destroy(messageHandle);
}

//
// CreateDeviceClientHandleForPnP creates a IOTHUB_DEVICE_CLIENT_HANDLE for this application, setting its
// ModelId along with various callbacks.
//
static IOTHUB_DEVICE_CLIENT_HANDLE CreateDeviceClientHandleForPnP(const char* connectionString)
{
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;
    bool urlAutoEncodeDecode = true;
    int iothubInitResult;
    bool result;

    // Before invoking ANY IoTHub Device SDK functionality, IoTHub_Init must be invoked.
    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        LogError("Failure to initialize client.  Error=%d", iothubInitResult);
        result = false;
    }
    // Create the deviceHandle itself.
    else if ((deviceHandle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, MQTT_Protocol)) == NULL)
    {
        LogError("Failure creating IotHub client.  Hint: Check your connection string");
        result = false;
    }
    // Sets verbosity level
    else if ((iothubResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_LOG_TRACE, &g_hubClientTraceEnabled)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set logging option, error=%d", iothubResult);
        result = false;
    }
    // Sets the name of ModelId for this PnP device.
    // This *MUST* be set before the client is connected to IoTHub.  We do not automatically connect when the 
    // handle is created, but will implicitly connect to subscribe for device method and device twin callbacks below.
    else if ((iothubResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_MODEL_ID, g_ThermostatModelId)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the ModelID, error=%d", iothubResult);
        result = false;
    }
    // Sets the callback function that processes incoming device methods, which is the channel PnP Commands are transferred over
    else if ((iothubResult = IoTHubDeviceClient_SetDeviceMethodCallback(deviceHandle, Thermostat_DeviceMethodCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set device method callback, error=%d", iothubResult);
        result = false;
    }
    // Sets the callback function that processes device twin changes from the IoTHub, which is the channel that PnP Properties are 
    // transferred over.  This will also automatically retrieve the full twin for the application. 
    else if ((iothubResult = IoTHubDeviceClient_SetDeviceTwinCallback(deviceHandle, Thermostat_DeviceTwinCallback, (void*)deviceHandle)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set device twin callback, error=%d", iothubResult);
        result = false;
    }
    // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically.
    else if ((iothubResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set auto Url encode option, error=%d", iothubResult);
        result = false;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on systems without built in certificate stores.
    else if ((iothubResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
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
        IoTHubDeviceClient_Destroy(deviceHandle);
        deviceHandle = NULL;
    }

    if ((result == false) &&  (iothubInitResult == 0))
    {
        IoTHub_Deinit();
    }

    return deviceHandle;
}


int main(void)
{
    IOTHUB_DEVICE_CLIENT_HANDLE deviceClient = NULL;
    const char* connectionString;

    if ((connectionString = getenv(g_connectionStringEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable %s", g_connectionStringEnvironmentVariable);
    }
    else if (BuildUtcTimeFromCurrentTime(g_ProgramStartTime, sizeof(g_ProgramStartTime)) == false)
    {
        LogError("Unable to output the program start time");
    }
    else if ((deviceClient = CreateDeviceClientHandleForPnP(connectionString)) == NULL)
    {
        LogError("Failed creating IotHub device");
    }
    else
    {
        LogInfo("Successfully created device client handle.  Hit Control-C to exit program\n");

        SendMaxTemperatureSinceReboot(deviceClient);

        while (true)
        {
            // Wake up periodically to send telemetry.
            // IOTHUB_DEVICE_CLIENT_HANDLE brings in the IoTHub device convenience layer, which means that the IoTHub SDK itself 
            // spins a worker thread to perform all operations.
            Thermostat_SendCurrentTemperature(deviceClient);
            ThreadAPI_Sleep(g_sleepBetweenTelemetrySends);
        }

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_Destroy(deviceClient);
        // Free all the IoT SDK subsystem
        IoTHub_Deinit();        
    }

    return 0;
}
