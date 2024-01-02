// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Standard C header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <math.h>

// PnP routines
#include "pnp_thermostat_component.h"
#include "iothub_client_properties.h"

// Core IoT SDK utilities
#include "azure_c_shared_utility/xlogging.h"

#include "pnp_status_values.h"

// Size of buffer to store ISO 8601 time.
#define TIME_BUFFER_SIZE 128

// Size of buffer to store current temperature telemetry.
#define CURRENT_TEMPERATURE_BUFFER_SIZE  32

// Size of buffer to store the maximum temp since reboot property.
#define MAX_TEMPERATURE_SINCE_REBOOT_BUFFER_SIZE 32

// Maximum component size.  This comes from the spec https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/dtdlv2.md#component.
#define MAX_COMPONENT_NAME_LENGTH 64

// Names of properties for desired/reporting.
static const char g_targetTemperaturePropertyName[] = "targetTemperature";
static const char g_maxTempSinceLastRebootPropertyName[] = "maxTempSinceLastReboot";

// Name of command this component supports to retrieve a report about the component.
static const char g_getMaxMinReportCommandName[] = "getMaxMinReport";

// The default temperature to use before any is set
#define DEFAULT_TEMPERATURE_VALUE 22

// Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
static const char g_ISO8601Format[] = "%Y-%m-%dT%H:%M:%SZ";

// Format string for sending temperature telemetry.
static const char g_temperatureTelemetryBodyFormat[] = "{\"temperature\":%.02f}";

// Format string for building getMaxMinReport response.
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

// Start time of the program, stored in ISO 8601 format string for UTC
char g_programStartTime[TIME_BUFFER_SIZE] = {0};

//
// PNP_THERMOSTAT_COMPONENT simulates a thermostat component
// (as in thermostat1 or thermostat2 in the TemperatureController model).  We need separate data structures
// because the components can be independently controlled.
//
typedef struct PNP_THERMOSTAT_COMPONENT_TAG
{
    // Name of this component
    char componentName[MAX_COMPONENT_NAME_LENGTH + 1];
    // Current temperature of this thermostat component
    double currentTemperature;
    // Minimum temperature this thermostat has been at during current execution run of this thermostat component
    double minTemperature;
    // Maximum temperature thermostat has been at during current execution run of this thermostat component
    double maxTemperature;
    // Number of times temperature has been updated, counting the initial setting as 1.  Used to determine average temperature of this thermostat component
    int numTemperatureUpdates;
    // Total of all temperature updates during current execution run.  Used to determine average temperature of this thermostat component
    double allTemperatures;
}
PNP_THERMOSTAT_COMPONENT;

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

PNP_THERMOSTAT_COMPONENT_HANDLE PnP_ThermostatComponent_CreateHandle(const char* componentName)
{
    PNP_THERMOSTAT_COMPONENT* thermostatComponent;

    if (strlen(componentName) > MAX_COMPONENT_NAME_LENGTH)
    {
        LogError("componentName %s is too long.  Maximum length is %d", componentName, MAX_COMPONENT_NAME_LENGTH);
        thermostatComponent = NULL;
    }
    // On initial invocation, store the UTC time into g_programStartTime global.
    else if ((g_programStartTime[0] == 0) && (BuildUtcTimeFromCurrentTime(g_programStartTime, sizeof(g_programStartTime)) == false))
    {
        LogError("Unable to store program start time");
        thermostatComponent = NULL;
    }
    else if ((thermostatComponent = (PNP_THERMOSTAT_COMPONENT*)calloc(1, sizeof(PNP_THERMOSTAT_COMPONENT))) == NULL)
    {
        LogError("Unable to allocate thermostat");
    }
    else
    {
        strcpy(thermostatComponent->componentName, componentName);
        thermostatComponent->currentTemperature = DEFAULT_TEMPERATURE_VALUE;
        thermostatComponent->maxTemperature = DEFAULT_TEMPERATURE_VALUE;
        thermostatComponent->minTemperature = DEFAULT_TEMPERATURE_VALUE;
        thermostatComponent->numTemperatureUpdates = 1;
        thermostatComponent->allTemperatures = DEFAULT_TEMPERATURE_VALUE;
    }

    return (PNP_THERMOSTAT_COMPONENT_HANDLE)thermostatComponent;
}

void PnP_ThermostatComponent_Destroy(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle)
{
    if (pnpThermostatComponentHandle != NULL)
    {
        free(pnpThermostatComponentHandle);
    }
}

//
// BuildMaxMinCommandResponse builds the response to the command for getMaxMinReport
//
static bool BuildMaxMinCommandResponse(PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent, IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse)
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
    else if ((responseBuilderSize = snprintf(NULL, 0, g_maxMinCommandResponseFormat, pnpThermostatComponent->maxTemperature, pnpThermostatComponent->minTemperature,
                                             pnpThermostatComponent->allTemperatures / pnpThermostatComponent->numTemperatureUpdates, g_programStartTime, currentTime)) < 0)
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
    else if ((responseBuilderSize = snprintf((char*)responseBuilder, (size_t)responseBuilderSize + 1, g_maxMinCommandResponseFormat, pnpThermostatComponent->maxTemperature, pnpThermostatComponent->minTemperature,
                                              pnpThermostatComponent->allTemperatures / pnpThermostatComponent->numTemperatureUpdates, g_programStartTime, currentTime)) < 0)
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
    }
    else
    {
        free(responseBuilder);
    }

    return result;
}       

void PnP_ThermostatComponent_ProcessCommand(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, const char *pnpCommandName, JSON_Value* commandJsonValue, IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse)
{
    PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent = (PNP_THERMOSTAT_COMPONENT*)pnpThermostatComponentHandle;
    const char* sinceStr;

    if (strcmp(pnpCommandName, g_getMaxMinReportCommandName) != 0)
    {
        LogError("Command %s is not supported on thermostat component", pnpCommandName);
        commandResponse->statusCode = PNP_STATUS_NOT_FOUND;
    }
    // See caveats section in ../readme.md; we don't actually respect this sinceStr to keep the sample simple,
    // but want to demonstrate how to parse out in any case.
    else if ((sinceStr = json_value_get_string(commandJsonValue)) == NULL)
    {
        LogError("Cannot retrieve JSON string for command");
        commandResponse->statusCode = PNP_STATUS_BAD_FORMAT;
    }
    else if (BuildMaxMinCommandResponse(pnpThermostatComponent, commandResponse) == false)
    {
        LogError("Unable to build response for component %s", pnpThermostatComponent->componentName);
        commandResponse->statusCode = PNP_STATUS_INTERNAL_ERROR;
    }
    else
    {
        LogInfo("Returning success from command request for component %s", pnpThermostatComponent->componentName);
        commandResponse->statusCode = PNP_STATUS_SUCCESS;
    }
}

//
// UpdateTemperatureAndStatistics updates the temperature and min/max/average values
//
static void UpdateTemperatureAndStatistics(PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent, double desiredTemp, bool* maxTempUpdated)
{
    if (desiredTemp > pnpThermostatComponent->maxTemperature)
    {
        pnpThermostatComponent->maxTemperature = desiredTemp;
        *maxTempUpdated = true;
    }
    else 
    {
        *maxTempUpdated = false;

        if (desiredTemp < pnpThermostatComponent->minTemperature)
        {
            pnpThermostatComponent->minTemperature = desiredTemp;
            *maxTempUpdated = false;
        }
    }

    pnpThermostatComponent->numTemperatureUpdates++;
    pnpThermostatComponent->allTemperatures += desiredTemp;
    pnpThermostatComponent->currentTemperature = desiredTemp;
}

//
// SendTargetTemperatureResponse sends a PnP property indicating the device has received the desired targeted temperature
//
static void SendTargetTemperatureResponse(PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent, IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient, const char* desiredTemperatureString, int responseStatus, int version, const char* description)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;

    // IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE in this case specifies the response to a desired temperature.
    IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE temperatureProperty;
    memset(&temperatureProperty, 0, sizeof(temperatureProperty));
    // Specify the structure version (not to be confused with the $version on IoT Hub) to protect back-compat in case the structure adds fields.
    temperatureProperty.structVersion= IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1;
    // This represents the version of the request from IoT Hub.  It needs to be returned so service applications can determine
    // what current version of the writable property the device is currently using, as the server may update the property even when the device
    // is offline.
    temperatureProperty.ackVersion = version;
    // Result of request, which maps to HTTP status code.
    temperatureProperty.result = responseStatus;
    temperatureProperty.name = g_targetTemperaturePropertyName;
    temperatureProperty.value = desiredTemperatureString;
    temperatureProperty.description = description;

    unsigned char* propertySerialized = NULL;
    size_t propertySerializedLength;

    // The first step of reporting properties is to serialize IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE into JSON for sending.
    if ((iothubClientResult = IoTHubClient_Properties_Serializer_CreateWritableResponse(&temperatureProperty, 1, pnpThermostatComponent->componentName, &propertySerialized, &propertySerializedLength)) != IOTHUB_CLIENT_OK)
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
        LogInfo("Sending acknowledgement of property to IoTHub for component %s", pnpThermostatComponent->componentName);
    }
    IoTHubClient_Properties_Serializer_Destroy(propertySerialized);
}

void PnP_TempControlComponent_Report_MaxTempSinceLastReboot_Property(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient)
{
    PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent = (PNP_THERMOSTAT_COMPONENT*)pnpThermostatComponentHandle;
    char maximumTemperatureAsString[MAX_TEMPERATURE_SINCE_REBOOT_BUFFER_SIZE];
    IOTHUB_CLIENT_RESULT iothubClientResult;

    if (snprintf(maximumTemperatureAsString, sizeof(maximumTemperatureAsString), g_maxTempSinceLastRebootPropertyFormat, pnpThermostatComponent->maxTemperature) < 0)
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

        // The first step of reporting properties is to serialize IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE into JSON for sending.
        if ((iothubClientResult = IoTHubClient_Properties_Serializer_CreateReported(&maxTempProperty, 1, pnpThermostatComponent->componentName, &propertySerialized, &propertySerializedLength)) != IOTHUB_CLIENT_OK)
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
            LogInfo("Sending %s property to IoTHub for component %s", g_maxTempSinceLastRebootPropertyName, pnpThermostatComponent->componentName);
        }
        IoTHubClient_Properties_Serializer_Destroy(propertySerialized);
    }
}

void PnP_ThermostatComponent_ProcessPropertyUpdate(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient, const char* propertyName, const char* propertyValue, int version)
{
    PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent = (PNP_THERMOSTAT_COMPONENT*)pnpThermostatComponentHandle;

    if (strcmp(propertyName, g_targetTemperaturePropertyName) != 0)
    {
        LogError("Property %s was requested to be changed but is not part of the thermostat interface definition", propertyName);
    }
    else
    {
        char* next;
        double targetTemperature = strtod(propertyValue, &next);
        if ((propertyValue == next) || (targetTemperature == HUGE_VAL) || (targetTemperature == (-1*HUGE_VAL)))
        {
            LogError("Property %s is not a valid number", propertyValue);
            SendTargetTemperatureResponse(pnpThermostatComponent, deviceClient, propertyValue, PNP_STATUS_BAD_FORMAT, version, g_temperaturePropertyResponseDescriptionNotInt);
        }
        else
        {
            LogInfo("Received targetTemperature %f for component %s", targetTemperature, pnpThermostatComponent->componentName);
            
            bool maxTempUpdated = false;
            UpdateTemperatureAndStatistics(pnpThermostatComponent, targetTemperature, &maxTempUpdated);

            // The device needs to let the service know that it has received the targetTemperature desired property.
            SendTargetTemperatureResponse(pnpThermostatComponent, deviceClient, propertyValue, PNP_STATUS_SUCCESS, version, NULL);
            
            if (maxTempUpdated)
            {
                // If the maximum temperature has been updated, we also report this as a property.
                PnP_TempControlComponent_Report_MaxTempSinceLastReboot_Property(pnpThermostatComponent, deviceClient);
            }
        }
    }
}

void PnP_ThermostatComponent_SendCurrentTemperature(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient)
{
    PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent = (PNP_THERMOSTAT_COMPONENT*)pnpThermostatComponentHandle;
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_MESSAGE_RESULT messageResult;
    IOTHUB_CLIENT_RESULT iothubClientResult;

    char temperatureStringBuffer[CURRENT_TEMPERATURE_BUFFER_SIZE];

    // Create the telemetry message body to send.
    if (snprintf(temperatureStringBuffer, sizeof(temperatureStringBuffer), g_temperatureTelemetryBodyFormat, pnpThermostatComponent->currentTemperature) < 0)
    {
        LogError("snprintf of current temperature telemetry failed");
    }
    // Create the message handle and specify its metadata.
    else if ((messageHandle = IoTHubMessage_CreateFromString(temperatureStringBuffer)) == NULL)
    {
        LogError("IoTHubMessage_PnP_CreateFromString failed");
    }
    else if ((messageResult = IoTHubMessage_SetContentTypeSystemProperty(messageHandle, g_jsonContentType)) != IOTHUB_MESSAGE_OK)
    {
        LogError("IoTHubMessage_SetContentTypeSystemProperty failed, error=%d", messageResult);
    }
    else if ((messageResult = IoTHubMessage_SetContentEncodingSystemProperty(messageHandle, g_utf8EncodingType)) != IOTHUB_MESSAGE_OK)
    {
        LogError("IoTHubMessage_SetContentEncodingSystemProperty failed, error=%d", messageResult);
    }
    else if ((messageResult = IoTHubMessage_SetComponentName(messageHandle, pnpThermostatComponent->componentName)) != IOTHUB_MESSAGE_OK)
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
