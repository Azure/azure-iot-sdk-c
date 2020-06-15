// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Implements a sample DigitalTwin interface as defined by "dtmi:my_company:com:EnvironmentalSensor;1"
// This demonstrates the core DigitalTwin concepts of telemetry, commands, and properties.
// Because this sample is designed to be run on any device, all of the sample data and command 
// processing is expressed simply with random numbers and LogInfo() calls.

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "digitaltwin_sample_environmental_sensor.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

//
// TODO`s: Configure core settings of your Digital Twin sample interface.
// The ComponentName should match what the model named the "dtmi:my_company:com:EnvironmentalSensor;1"
// Many sample models use "sensor" as this name, so this sample uses this as the default.

static const char DigitalTwinSampleEnvironmentalSensor_ComponentName[] = "sensor";

// END TODO section
//

//  
//  Telemetry names for this interface.
//
static const char* DigitalTwinSampleEnvironmentalSensor_TemperatureTelemetry = "temp";
static const char* DigitalTwinSampleEnvironmentalSensor_HumidityTelemetry = "humidity";


//
//  Property names and data for DigitalTwin read-only properties for this interface.
//
// digitaltwinSample_DeviceState* represents the environmental sensor's read-only property, whether its online or not.
static const char digitaltwinSample_DeviceStateProperty[] = "state";
static const unsigned char digitaltwinSample_DeviceStateData[] = "true";
static const int digitaltwinSample_DeviceStateDataLen = sizeof(digitaltwinSample_DeviceStateData) - 1;

//
//  Callback command names for this interface.
//
static const char digitaltwinSample_EnvironmentalSensorCommandBlink[] = "blink";
static const char digitaltwinSample_EnvironmentalSensorCommandTurnOn[] =  "turnOn";
static const char digitaltwinSample_EnvironmentalSensorCommandTurnOff[] =  "turnOff";

//
// Command status codes
//
static const int commandStatusProcessing = 102;
static const int commandStatusSuccess = 200;
static const int commandStatusPending = 202;
static const int commandStatusNotPresent = 404;
static const int commandStatusFailure = 500;


//
// What we respond to various commands with.  Must be valid JSON.
//
static const unsigned char digitaltwinSample_EnviromentalSensor_BlinkResponse[] = "{ \"description\": \"leds blinking\" }";

static const unsigned char digitaltwinSample_EnviromentalSensor_OutOfMemory[] = "\"Out of memory\"";
static const unsigned char digitaltwinSample_EnviromentalSensor_NotImplemented[] = "\"Requested command not implemented on this interface\"";


//
// Property names that are updatebale from the server application/operator.
//

static const char digitaltwinSample_EnvironmentalSensorPropertyCustomerName[] = "name";
static const char digitaltwinSample_EnvironmentalSensorPropertyBrightness[] = "brightness";

//
// Application state associated with the particular interface.  In particular it contains 
// the DIGITALTWIN_INTERFACE_CLIENT_HANDLE used for responses in callbacks along with properties set
// and representations of the property update and command callbacks invoked on given interface
//
typedef struct DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE_TAG
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandle;
    int brightness;
    char* customerName;
    int numTimesBlinkCommandCalled;
} DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE;

// State for interface.  For simplicity we set this as a global and set during DigitalTwin_InterfaceClient_Create, but  
// callbacks of this interface don't reference it directly but instead use userContextCallback passed to them.
static DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE digitaltwinSample_EnvironmentalSensorState;

// DigitalTwinSampleEnvironmentalSensor_SetCommandResponse is a helper that fills out a DIGITALTWIN_CLIENT_COMMAND_RESPONSE
static int DigitalTwinSampleEnvironmentalSensor_SetCommandResponse(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, const unsigned char* responseData, int status)
{
    size_t responseLen = strlen((const char *)responseData);
    memset(dtCommandResponse, 0, sizeof(*dtCommandResponse));
    dtCommandResponse->version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;
    int result;

    // Allocate a copy of the response data to return to the invoker.  The DigitalTwin layer that invoked the application callback
    // takes responsibility for freeing this data.
    if (mallocAndStrcpy_s((char**)&dtCommandResponse->responseData, (const char *)responseData) != 0)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Unable to allocate response data");
        dtCommandResponse->status = commandStatusFailure;
        result = MU_FAILURE;
    }
    else
    {
        dtCommandResponse->responseDataLen = responseLen;
        dtCommandResponse->status = status;
        result = 0;
    }

    return result;
}

static int DigitalTwinSampleEnvironmentalSensor_SetCommandResponseEmptyBody(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, int status)
{
    memset(dtCommandResponse, 0, sizeof(*dtCommandResponse));
    dtCommandResponse->version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;
    dtCommandResponse->status = status;
    return 0;
}

// Implement the callback to process the command "blink".  Information pertaining to the request is specified in DIGITALTWIN_CLIENT_COMMAND_REQUEST,
// and the callback fills out data it wishes to return to the caller on the service in DIGITALTWIN_CLIENT_COMMAND_RESPONSE.
static void DigitalTwinSampleEnvironmentalSensor_BlinkCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* commandCallbackContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE* sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)commandCallbackContext;

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Blink command invoked.  It has been invoked %d times previously", sensorState->numTimesBlinkCommandCalled);
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Blink data=<%.*s>", (int)dtCommandRequest->requestDataLen, dtCommandRequest->requestData);

    sensorState->numTimesBlinkCommandCalled++;

    (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponse(dtCommandResponse, digitaltwinSample_EnviromentalSensor_BlinkResponse, commandStatusSuccess);
}

// Implement the callback to process the command "turnOn".
static void DigitalTwinSampleEnvironmentalSensor_TurnOnLightCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* commandCallbackContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE* sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)commandCallbackContext;
    (void)sensorState; // Sensor state not used in this sample

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Turn on light command invoked");
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Turn on light data=<%.*s>", (int)dtCommandRequest->requestDataLen, dtCommandRequest->requestData);

    (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponseEmptyBody(dtCommandResponse, commandStatusSuccess);
}

// Implement the callback to process the command "turnOff".
static void DigitalTwinSampleEnvironmentalSensor_TurnOffLightCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* commandCallbackContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE* sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)commandCallbackContext;
    (void)sensorState; // Sensor state not used in this sample

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Turn off light command invoked");
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Turn off light data=<%.*s>", (int)dtCommandRequest->requestDataLen, dtCommandRequest->requestData);

    (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponseEmptyBody(dtCommandResponse, commandStatusSuccess);
}

// DigitalTwinSampleEnvironmentalSensor_PropertyCallback is invoked when a property is updated (or failed) going to server.
// In this sample, we route ALL property callbacks to this function and just have the userContextCallback set
// to the propertyName.  Product code will potentially have context stored in this userContextCallback.
static void DigitalTwinSampleEnvironmentalSensor_PropertyCallback(DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContextCallback)
{
    if (dtReportedStatus == DIGITALTWIN_CLIENT_OK)
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Updating property=<%s> succeeded", (const char*)userContextCallback);
    }
    else
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Updating property property=<%s> failed, error=<%d>", (const char*)userContextCallback, dtReportedStatus);
    }
}

// Processes a property update, which the server initiated, for customer name.
static void DigitalTwinSampleEnvironmentalSensor_CustomerNameCallback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* propertyCallbackContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE *sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)propertyCallbackContext;
    DIGITALTWIN_CLIENT_RESULT result;

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: CustomerName property invoked...");
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: CustomerName data=<%.*s>", (int)dtClientPropertyUpdate->propertyDesiredLen, dtClientPropertyUpdate->propertyDesired);

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;

    // Version of this structure for C SDK.
    propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
    propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;

    free(sensorState->customerName);
    if ((sensorState->customerName = (char*)malloc(dtClientPropertyUpdate->propertyDesiredLen+1)) == NULL)
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Out of memory updating CustomerName...");

        // Indicates failure
        propertyResponse.statusCode = 500;
        // Optional additional human readable information about status.
        propertyResponse.statusDescription = "Out of memory";
    }
    else
    {
        strncpy(sensorState->customerName, (char*)dtClientPropertyUpdate->propertyDesired, dtClientPropertyUpdate->propertyDesiredLen);
        sensorState->customerName[dtClientPropertyUpdate->propertyDesiredLen] = 0;
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: CustomerName sucessfully updated...");
        
        // Indicates success
        propertyResponse.statusCode = 200;
        // Optional additional human readable information about status.
        propertyResponse.statusDescription = "Property Updated Successfully";
    }

    //
    // DigitalTwin_InterfaceClient_ReportPropertyAsync takes the DIGITALTWIN_CLIENT_PROPERTY_RESPONSE and returns information back to service.
    //
    result = DigitalTwin_InterfaceClient_ReportPropertyAsync(sensorState->interfaceClientHandle, digitaltwinSample_EnvironmentalSensorPropertyCustomerName,
                                                             dtClientPropertyUpdate->propertyDesired, dtClientPropertyUpdate->propertyDesiredLen, &propertyResponse,
                                                             DigitalTwinSampleEnvironmentalSensor_PropertyCallback, (void*)digitaltwinSample_EnvironmentalSensorPropertyCustomerName);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_ReportPropertyAsync for CustomerName failed, error=<%d>", result);
    }
    else
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Successfully queued Property update for CustomerName");
    }
}

// Processes a property update, which the server initiated, for brightness.
static int DigitalTwinSampleEnvironmentalSensor_ParseBrightness(const char* propertyDesired, int *brightness)
{
    int result;

    char* next;
    *brightness = (int)strtol(propertyDesired, &next, 0);
    if ((propertyDesired == next) || ((((*brightness) == INT_MAX) || ((*brightness) == INT_MIN)) && (errno != 0)))
    {
        LogError("Could not parse data=<%s> specified", propertyDesired);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

// Process a property update for bright level.
static void DigitalTwinSampleEnvironmentalSensor_BrightnessCallback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* propertyCallbackContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE *sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)propertyCallbackContext;
    DIGITALTWIN_CLIENT_RESULT result;

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Brightness property invoked...");
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Brightness data=<%.*s>", (int)dtClientPropertyUpdate->propertyDesiredLen, dtClientPropertyUpdate->propertyDesired);

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    int brightness;

    // Version of this structure for C SDK.
    propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
    propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;

    if (DigitalTwinSampleEnvironmentalSensor_ParseBrightness((const char*)dtClientPropertyUpdate->propertyDesired, &brightness) != 0)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Invalid brightness data=<%.*s> specified", (int)dtClientPropertyUpdate->propertyDesiredLen, dtClientPropertyUpdate->propertyDesired);

        // Indicates failure
        propertyResponse.statusCode = 500;
        // Optional additional human readable information about status.
        propertyResponse.statusDescription = "Invalid brightness setting";
    }
    else
    {
        sensorState->brightness = brightness;
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Brightness successfully updated to %d...", sensorState->brightness);
        
        // Indicates success
        propertyResponse.statusCode = 200;
        // Optional additional human readable information about status.
        propertyResponse.statusDescription = "Brightness updated";
    }

    //
    // DigitalTwin_InterfaceClient_ReportPropertyAsync takes the DIGITALTWIN_CLIENT_PROPERTY_RESPONSE and returns information back to service.
    //
    result = DigitalTwin_InterfaceClient_ReportPropertyAsync(sensorState->interfaceClientHandle, digitaltwinSample_EnvironmentalSensorPropertyBrightness,
                                                             dtClientPropertyUpdate->propertyDesired, dtClientPropertyUpdate->propertyDesiredLen, &propertyResponse,
                                                             DigitalTwinSampleEnvironmentalSensor_PropertyCallback, (void*)digitaltwinSample_EnvironmentalSensorPropertyBrightness);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_ReportPropertyAsync for Brightness failed, error=<%d>", result);
    }
    else
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Successfully queued Property update for Brightness");
    }    
}

// Sends a reported property for device state of this simulated device.
static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleEnvironmentalSensor_ReportDeviceStateAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    DIGITALTWIN_CLIENT_RESULT result;

    result = DigitalTwin_InterfaceClient_ReportPropertyAsync(interfaceHandle, digitaltwinSample_DeviceStateProperty, 
                                                             digitaltwinSample_DeviceStateData, digitaltwinSample_DeviceStateDataLen, NULL,
                                                             DigitalTwinSampleEnvironmentalSensor_PropertyCallback, (void*)digitaltwinSample_DeviceStateProperty);

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Reporting property=<%s> failed, error=<%d>", digitaltwinSample_DeviceStateProperty, result);
    }
    else
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Queued async report read only property for %s", digitaltwinSample_DeviceStateProperty);
    }

    return result;
}

// DigitalTwinSampleEnvironmentalSensor_InterfaceRegisteredCallback is invoked when this interface
// is successfully or unsuccessfully registered with the service, and also when the interface is deleted.
static void DigitalTwinSampleEnvironmentalSensor_InterfaceRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE *sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)userInterfaceContext;
    if (dtInterfaceStatus == DIGITALTWIN_CLIENT_OK)
    {
        // Once the interface is registered, send our reported properties to the service.  
        // It *IS* safe to invoke most DigitalTwin API calls from a callback thread like this, though it 
        // is NOT safe to create/destroy/register interfaces now.
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Interface successfully registered.");
        DigitalTwinSampleEnvironmentalSensor_ReportDeviceStateAsync(sensorState->interfaceClientHandle);
    }
    else if (dtInterfaceStatus == DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING)
    {
        // Once an interface is marked as unregistered, it cannot be used for any DigitalTwin SDK calls.
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Interface received unregistering callback.");
    }
    else 
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Interface received failed, status=<%d>.", dtInterfaceStatus);
    }
}


// DigitalTwinSample_ProcessCommandUpdate receives commands from the server.  This implementation acts as a simple dispatcher
// to the functions to perform the actual processing.
void DigitalTwinSample_ProcessCommandUpdate(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* commandCallbackContext)
{
    if (strcmp(dtCommandRequest->commandName, digitaltwinSample_EnvironmentalSensorCommandBlink) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_BlinkCallback(dtCommandRequest, dtCommandResponse, commandCallbackContext);
    }
    else if (strcmp(dtCommandRequest->commandName, digitaltwinSample_EnvironmentalSensorCommandTurnOn) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_TurnOnLightCallback(dtCommandRequest, dtCommandResponse, commandCallbackContext);
    }
    else if (strcmp(dtCommandRequest->commandName, digitaltwinSample_EnvironmentalSensorCommandTurnOff) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_TurnOffLightCallback(dtCommandRequest, dtCommandResponse, commandCallbackContext);
    }
    else
    {
        // If the command is not implemented by this interface, by convention we return a 404 error to server.
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Command name <%s> is not associated with this interface", dtCommandRequest->commandName);
        (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponse(dtCommandResponse, digitaltwinSample_EnviromentalSensor_NotImplemented, commandStatusNotPresent);
    }
}

// DigitalTwinSampleEnvironmentalSensor_ProcessPropertyUpdate receives updated properties from the server.  This implementation
// acts as a simple dispatcher to the functions to perform the actual processing.
static void DigitalTwinSampleEnvironmentalSensor_ProcessPropertyUpdate(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* propertyCallbackContext)
{
    if (strcmp(dtClientPropertyUpdate->propertyName, digitaltwinSample_EnvironmentalSensorPropertyCustomerName) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_CustomerNameCallback(dtClientPropertyUpdate, propertyCallbackContext);
    }
    else if (strcmp(dtClientPropertyUpdate->propertyName, digitaltwinSample_EnvironmentalSensorPropertyBrightness) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_BrightnessCallback(dtClientPropertyUpdate, propertyCallbackContext);
    }
    else if (strcmp(dtClientPropertyUpdate->propertyName, digitaltwinSample_DeviceStateProperty) == 0)
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Property name <%s>, last reported value=<%.*s>", 
            dtClientPropertyUpdate->propertyName, (int)dtClientPropertyUpdate->propertyReportedLen, dtClientPropertyUpdate->propertyReported);
    }
    else
    {
        // If the property is not implemented by this interface, presently we only record a log message but do not have a mechanism to report back to the service
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Property name <%s> is not associated with this interface", dtClientPropertyUpdate->propertyName);
    }
}


//
// DigitalTwinSampleEnvironmentalSensor_CreateInterface is the initial entry point into the DigitalTwin Sample Environmental Sensor interface.
// It simply creates a DIGITALTWIN_INTERFACE_CLIENT_HANDLE that is mapped to the environmental sensor component.
// This call is synchronous, as simply creating an interface only performs initial allocations.
//
// NOTE: The actual registration of this interface is left to the caller, which may register 
// multiple interfaces on one DIGITALTWIN_DEVICE_CLIENT_HANDLE.
//
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DigitalTwinSampleEnvironmentalSensor_CreateInterface(void)
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    memset(&digitaltwinSample_EnvironmentalSensorState, 0, sizeof(digitaltwinSample_EnvironmentalSensorState));
    
    if ((result =  DigitalTwin_InterfaceClient_Create(DigitalTwinSampleEnvironmentalSensor_ComponentName, DigitalTwinSampleEnvironmentalSensor_InterfaceRegisteredCallback, (void*)&digitaltwinSample_EnvironmentalSensorState, &interfaceHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Unable to allocate interface client handle for componentName=<%s>, error=<%d>", DigitalTwinSampleEnvironmentalSensor_ComponentName, result);
        interfaceHandle = NULL;
    }
    else if ((result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(interfaceHandle, DigitalTwinSampleEnvironmentalSensor_ProcessPropertyUpdate, (void*)&digitaltwinSample_EnvironmentalSensorState)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback failed. error=<%d>", result);
        DigitalTwinSampleEnvironmentalSensor_Close(interfaceHandle);
        interfaceHandle = NULL;
    }
    else if ((result = DigitalTwin_InterfaceClient_SetCommandsCallback(interfaceHandle, DigitalTwinSample_ProcessCommandUpdate, (void*)&digitaltwinSample_EnvironmentalSensorState)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallback failed. error=<%d>", result);
        DigitalTwinSampleEnvironmentalSensor_Close(interfaceHandle);
        interfaceHandle = NULL;
    }
    else
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE.  componentName=<%s>, handle=<%p>", DigitalTwinSampleEnvironmentalSensor_ComponentName, interfaceHandle);
        digitaltwinSample_EnvironmentalSensorState.interfaceClientHandle = interfaceHandle;
    }

    return interfaceHandle;
}


// DigitalTwinSampleEnvironmentalSensor_TelemetryCallback is invoked when a DigitalTwin telemetry message
// is either successfully delivered to the service or else fails.  For this sample, the userContextCallback
// is simply a string pointing to the name of the message sent.  More complex scenarios may include
// more detailed state information as part of this callback.
static void DigitalTwinSampleEnvironmentalSensor_TelemetryCallback(DIGITALTWIN_CLIENT_RESULT dtTelemetryStatus, void* userContextCallback)
{
    (void)userContextCallback;

    if (dtTelemetryStatus == DIGITALTWIN_CLIENT_OK)
    {
        // This tends to overwhelm the logging on output based on how frequently this function is invoked, so removing by default.
        // LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin successfully delivered telemetry message for <%s>", (const char*)userContextCallback);
    }
    else
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin failed delivered telemetry message, error=<%d>", dtTelemetryStatus);
    }
}

//
// DigitalTwinSampleEnvironmentalSensor_SendTelemetryMessagesAsync is periodically invoked by the caller to
// send telemetry containing the current temperature and humidity (in both cases random numbers
// so this sample will work on platforms without these sensors).
//
DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleEnvironmentalSensor_SendTelemetryMessagesAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    DIGITALTWIN_CLIENT_RESULT result;

    float currentTemperature = 20.0f + ((float)rand() / RAND_MAX) * 15.0f;
    float currentHumidity = 60.0f + ((float)rand() / RAND_MAX) * 20.0f;

    char currentMessage[128];

    sprintf(currentMessage, "{\"%s\":%.3f, \"%s\":%.3f}", 
            DigitalTwinSampleEnvironmentalSensor_TemperatureTelemetry, currentTemperature,
            DigitalTwinSampleEnvironmentalSensor_HumidityTelemetry, currentHumidity);

    if ((result = DigitalTwin_InterfaceClient_SendTelemetryAsync(interfaceHandle, (unsigned char*)currentMessage, strlen(currentMessage),
                                                                 DigitalTwinSampleEnvironmentalSensor_TelemetryCallback, NULL)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_SendTelemetryAsync failed for sending.");
    }

    return result;
}

//
// DigitalTwinSampleEnvironmentalSensor_Close is invoked when the sample device is shutting down.
//
void DigitalTwinSampleEnvironmentalSensor_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    // On shutdown, in general the first call made should be to DigitalTwin_InterfaceClient_Destroy.
    // This will block if there are any active callbacks in this interface, and then
    // mark the underlying handle such that no future callbacks shall come to it.
    DigitalTwin_InterfaceClient_Destroy(interfaceHandle);

    // After DigitalTwin_InterfaceClient_Destroy returns, it is safe to assume
    // no more callbacks shall arrive for this interface and it is OK to free
    // resources callbacks otherwise may have needed.
    free(digitaltwinSample_EnvironmentalSensorState.customerName);
    digitaltwinSample_EnvironmentalSensorState.customerName = NULL;
}

