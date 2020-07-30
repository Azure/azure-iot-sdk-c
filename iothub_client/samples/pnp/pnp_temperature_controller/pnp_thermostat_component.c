// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Standard C header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// PnP helper routines
#include "pnp_protocol_helpers.h"
#include "pnp_thermostat_component.h"

// Core IoT SDK utilities
#include "azure_c_shared_utility/xlogging.h"

// The default temperature to use before any is set
#define DEFAULT_TEMPERATURE_VALUE 22

// Size of buffer to store ISO 8601 time.
#define TIME_BUFFER_SIZE 128

// Name of command this component supports to retrieve a report about the component.
static const char g_getMinMaxReport[] = "getMaxMinReport";

// Names of properties for desired/reporting
static const char g_targetTemperaturePropertyName[] = "targetTemperature";
static const char g_maxTempSinceLastRebootPropertyName[] = "maxTempSinceLastReboot";

// Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
static const char g_ISO8601Format[] = "%Y-%m-%dT%H:%M:%SZ";
// Format string for sending temperature telemetry
static const char g_temperatureTelemetryBodyFormat[] = "{\"temperature\":%.02f}";
// Format string for building getMaxMinReport response
static const char g_minMaxCommandResponseFormat[] = "{\"maxTemp\":%.2f,\"minTemp\":%.2f,\"avgTemp\":%.2f,\"startTime\":\"%s\",\"endTime\":\"%s\"}";
// Format string for sending maxTempSinceLastReboot property
static const char g_maxTempSinceLastRebootPropertyFormat[] = "%.2f";
// Format of the body when responding to a targetTemperature 
static const char g_targetTemperaturePropertyResponseFormat[] = "%.2f";

// Start time of the program, stored in ISO 8601 format string for UTC
char g_programStartTime[TIME_BUFFER_SIZE] = {0};


// Response description is an optional, human readable message including more information
// about the setting of the temperature.  Because we accept all temperature requests, we 
// always indicate a success.  An actual sensor could optionally return information about
// a temperature being out of range or a mechanical issue on the device on error cases.
static const char g_temperaturePropertyResponseDescription[] = "success";

//
// PNP_THERMOSTAT_COMPONENT simulates a thermostat component
// (as in thermostat1 or thermostat2 in the TemperatureController model).  We need separate data structures
// because the components can be independently controlled.
//
typedef struct PNP_THERMOSTAT_COMPONENT_TAG
{
    // Name of this component
    char componentName[PNP_MAXIMUM_COMPONENT_LENGTH + 1];
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


PNP_THERMOSTAT_COMPONENT_HANDLE PnP_ThermostatComponent_CreateHandle(const char* componentName)
{
    PNP_THERMOSTAT_COMPONENT* thermostatComponent;

    if (strlen(componentName) > PNP_MAXIMUM_COMPONENT_LENGTH)
    {
        LogError("componentName=%s is too long.  Maximum length is=%d", componentName, PNP_MAXIMUM_COMPONENT_LENGTH);
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
static bool BuildMaxMinCommandResponse(PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent, unsigned char** response, size_t* responseSize)
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
    else if ((responseBuilderSize = snprintf(NULL, 0, g_minMaxCommandResponseFormat, pnpThermostatComponent->minTemperature, pnpThermostatComponent->maxTemperature, 
                                             pnpThermostatComponent->allTemperatures / pnpThermostatComponent->numTemperatureUpdates, g_programStartTime, currentTime)) < 0)
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
    else if ((responseBuilderSize = snprintf((char*)responseBuilder, responseBuilderSize + 1, g_minMaxCommandResponseFormat, pnpThermostatComponent->minTemperature, pnpThermostatComponent->maxTemperature, 
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
        *response = responseBuilder;
        *responseSize = (size_t)responseBuilderSize;
    }
    else
    {
        free(responseBuilder);
    }

    return result;
}       

int PnP_ThermostatComponent_ProcessCommand(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, const char *pnpCommandName, JSON_Value* commandJsonValue, unsigned char** response, size_t* responseSize)
{
    PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent = (PNP_THERMOSTAT_COMPONENT*)pnpThermostatComponentHandle;
    const char* sinceStr;
    int result;

    *response = NULL;
    *responseSize = 0;

    if (strcmp(pnpCommandName, g_getMinMaxReport) != 0)
    {
        LogError("PnP command=%s is not supported on thermostat component", pnpCommandName);
        result = PNP_STATUS_NOT_FOUND;
    }
    // See caveats section in ../readme.md; we don't actually respect this sinceStr to keep the sample simple,
    // but want to demonstrate how to parse out in any case.
    else if ((sinceStr = json_value_get_string(commandJsonValue)) == NULL)
    {
        LogError("Cannot retrieve JSON string for command");
        result = PNP_STATUS_BAD_FORMAT;
    }
    else if (BuildMaxMinCommandResponse(pnpThermostatComponent, response, responseSize) == false)
    {
        LogError("Unable to build response for component=%s", pnpThermostatComponent->componentName);
        result = PNP_STATUS_INTERNAL_ERROR;
    }
    else
    {
        LogInfo("Returning success from command request for component=%s", pnpThermostatComponent->componentName);
        result = PNP_STATUS_SUCCESS;
    }

    return result;
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
static void SendTargetTemperatureResponse(PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient, int version)
{
    char targetTemperatureAsString[32];
    IOTHUB_CLIENT_RESULT iothubClientResult;
    STRING_HANDLE jsonToSend = NULL;

    if (snprintf(targetTemperatureAsString, sizeof(targetTemperatureAsString), g_targetTemperaturePropertyResponseFormat, pnpThermostatComponent->currentTemperature) < 0)
    {
        LogError("Unable to create target temperature string for reporting result");
    }
    else if ((jsonToSend = PnPHelper_CreateReportedPropertyWithStatus(pnpThermostatComponent->componentName, g_targetTemperaturePropertyName, targetTemperatureAsString, 
                                                                      PNP_STATUS_SUCCESS, g_temperaturePropertyResponseDescription, version)) == NULL)
    {
        LogError("Unable to build reported property response");
    }
    else
    {
        const char* jsonToSendStr = STRING_c_str(jsonToSend);
        size_t jsonToSendStrLen = strlen(jsonToSendStr);

        if ((iothubClientResult = IoTHubDeviceClient_SendReportedState(deviceClient, (const unsigned char*)jsonToSendStr, jsonToSendStrLen, NULL, NULL)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to send reported state, error=%d", iothubClientResult);
        }
        else
        {
            LogInfo("Sending acknowledgement of property to IoTHub for component=%s", pnpThermostatComponent->componentName);
        }
    }

    STRING_delete(jsonToSend);
}

void PnP_TempControlComponent_Report_MaxTempSinceLastReboot_Property(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient)
{
    PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent = (PNP_THERMOSTAT_COMPONENT*)pnpThermostatComponentHandle;
    char maximumTemperatureAsString[32];
    IOTHUB_CLIENT_RESULT iothubClientResult;
    STRING_HANDLE jsonToSend = NULL;

    if (snprintf(maximumTemperatureAsString, sizeof(maximumTemperatureAsString), g_maxTempSinceLastRebootPropertyFormat, pnpThermostatComponent->maxTemperature) < 0)
    {
        LogError("Unable to create max temp since last reboot string for reporting result");
    }
    else if ((jsonToSend = PnPHelper_CreateReportedProperty(pnpThermostatComponent->componentName, g_maxTempSinceLastRebootPropertyName, maximumTemperatureAsString)) == NULL)
    {
        LogError("Unable to build max temp since last reboot property");
    }
    else
    {
        const char* jsonToSendStr = STRING_c_str(jsonToSend);
        size_t jsonToSendStrLen = strlen(jsonToSendStr);

        if ((iothubClientResult = IoTHubDeviceClient_SendReportedState(deviceClient, (const unsigned char*)jsonToSendStr, jsonToSendStrLen, NULL, NULL)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to send reported state, error=%d", iothubClientResult);
        }
        else
        {
            LogInfo("Sending maximumTemperatureSinceLastReboot property to IoTHub for component=%s", pnpThermostatComponent->componentName);
        }
    }

    STRING_delete(jsonToSend);
}

void PnP_ThermostatComponent_ProcessPropertyUpdate(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient, const char* propertyName, JSON_Value* propertyValue, int version)
{
    PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent = (PNP_THERMOSTAT_COMPONENT*)pnpThermostatComponentHandle;

    if (strcmp(propertyName, g_targetTemperaturePropertyName) != 0)
    {
        LogError("Property=%s was requested to be changed but is not part of the thermostat interface definition", propertyName);
    }
    else if (json_value_get_type(propertyValue) != JSONNumber)
    {
        LogError("JSON field %s is not a number", g_targetTemperaturePropertyName);
    }
    else
    {
        double targetTemperature = json_value_get_number(propertyValue);

        LogInfo("Received targetTemperature=%f for component=%s", targetTemperature, pnpThermostatComponent->componentName);
        
        bool maxTempUpdated = false;
        UpdateTemperatureAndStatistics(pnpThermostatComponent, targetTemperature, &maxTempUpdated);

        // The device needs to let the service know that it has received the targetTemperature desired property.
        SendTargetTemperatureResponse(pnpThermostatComponent, deviceClient, version);
        
        if (maxTempUpdated)
        {
            // If the Maximum temperature has been updated, we also report this as a property.
            PnP_TempControlComponent_Report_MaxTempSinceLastReboot_Property(pnpThermostatComponent, deviceClient);
        }
    }
}

void PnP_ThermostatComponent_SendTelemetry(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient)
{
    PNP_THERMOSTAT_COMPONENT* pnpThermostatComponent = (PNP_THERMOSTAT_COMPONENT*)pnpThermostatComponentHandle;
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;

    char temperatureStringBuffer[32];

    if (snprintf(temperatureStringBuffer, sizeof(temperatureStringBuffer), g_temperatureTelemetryBodyFormat, pnpThermostatComponent->currentTemperature) < 0)
    {
        LogError("snprintf of current temperature telemetry failed");
    }
    else if ((messageHandle = PnPHelper_CreateTelemetryMessageHandle(pnpThermostatComponent->componentName, temperatureStringBuffer)) == NULL)
    {
        LogError("Unable to create telemetry message");
    }
    else if ((iothubResult = IoTHubDeviceClient_SendEventAsync(deviceClient, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send telemetry message, error=%d", iothubResult);
    }

    IoTHubMessage_Destroy(messageHandle);
}
