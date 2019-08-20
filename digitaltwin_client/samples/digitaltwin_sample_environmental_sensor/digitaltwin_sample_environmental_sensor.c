// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Implements a sample DigitalTwin interface that integrates with an environmental sensor (for reporting 
// temperature and humidity over time).  It also has basic commands and properties it can process,
// such as setting brightness of a light and blinking LEDs.  Because this sample is designed
// to be run anywhere, all of the sameple data and command processing is expressed simply with 
// random numbers and LogInfo() calls.

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "digitaltwin_sample_environmental_sensor.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

//
// TODO`s: Configure core settings of your Digital Twin sample interface
//

// TODO: Fill in DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INTERFACE_ID. E.g. 
#define DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INTERFACE_ID "urn:YOUR_COMPANY_NAME_HERE:EnvironmentalSensor:1"

//
// END TODO section
//


// DigitalTwin interface name from service perspective.
static const char DigitalTwinSampleEnvironmentalSensor_InterfaceId[] = DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INTERFACE_ID;
static const char DigitalTwinSampleEnvironmentalSensor_InterfaceInstanceName[] = "sensor";

//  
//  Telemetry names for this interface.
//
static const char* DigitalTwinSampleEnvironmentalSensor_TemperatureTelemetry = "temp";
static const char* DigitalTwinSampleEnvironmentalSensor_HumidityTelemetry = "humid";


//
//  Property names and data for DigitalTwin read-only properties for this interface.
//
// digitaltwinSample_DeviceState* represents the environmental sensor's read-only property, whether its online or not.
static const char digitaltwinSample_DeviceStateProperty[] = "state";
static const char digitaltwinSample_DeviceStateData[] = "true";
static const int digitaltwinSample_DeviceStateDataLen = sizeof(digitaltwinSample_DeviceStateData) - 1;

//
//  Callback command names for this interface.
//
static const char digitaltwinSample_EnvironmentalSensorCommandBlink[] = "blink";
static const char digitaltwinSample_EnvironmentalSensorCommandTurnOn[] =  "turnon";
static const char digitaltwinSample_EnvironmentalSensorCommandTurnOff[] =  "turnoff";
static const char digitaltwinSample_EnvironmentalSensorCommandRunDiagnostics[] = "rundiagnostics";

//
// Command status codes
//
static const int commandStatusSuccess = 200;
static const int commandStatusPending = 202;
static const int commandStatusFailure = 500;
static const int commandStatusNotPresent = 501;

//
// What we respond to various commands with.  Must be valid JSON.
//
static const char digitaltwinSample_EnviromentalSensor_BlinkResponse[] = "{ \"description\": \"leds blinking\" }";

static const char digitaltwinSample_EnviromentalSensor_OutOfMemory[] = "\"Out of memory\"";
static const char digitaltwinSample_EnviromentalSensor_NotImplemented[] = "\"Requested command not implemented on this interface\"";
static const char digitaltwinSample_EnviromentalSensor_RunDiagnosticsBusy[] = "\"Running of diagnostics already active.  Only one request may be active at a time\"";
static const char digitaltwinSample_EnviromentalSensor_DiagnosticInProgress[] = "\"Diagnostic still in progress\"";
static const char digitaltwinSample_EnviromentalSensor_DiagnosticsComplete[] = "\"Successfully run diagnostics\"";


//
// Property names that are updatebale from the server application/operator.
//

static const char digitaltwinSample_EnvironmentalSensorPropertyCustomerName[] = "name";
static const char digitaltwinSample_EnvironmentalSensorPropertyBrightness[] = "brightness";

// State of simulated diagnostic run.
typedef enum DIGITALTWIN_SAMPLE_DIAGNOSTIC_STATE_TAG
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_INACTIVE,
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_PHASE1,
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_PHASE2
} DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE;

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
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE diagnosticState;
    char* requestId;
} DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE;

// State for interface.  For simplicity we set this as a global and set during DigitalTwin_InterfaceClient_Create, but  
// callbacks of this interface don't reference it directly but instead use userContextCallback passed to them.
static DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE digitaltwinSample_EnvironmentalSensorState;

// DigitalTwinSampleEnvironmentalSensor_SetCommandResponse is a helper that fills out a DIGITALTWIN_CLIENT_COMMAND_RESPONSE
static int DigitalTwinSampleEnvironmentalSensor_SetCommandResponse(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, const char* responseData, int status)
{
    size_t responseLen = strlen(responseData);
    memset(dtCommandResponse, 0, sizeof(*dtCommandResponse));
    dtCommandResponse->version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;
    int result;

    // Allocate a copy of the response data to return to the invoker.  The DigitalTwin layer that invoked the application callback
    // takes responsibility for freeing this data.
    if (mallocAndStrcpy_s((char**)&dtCommandResponse->responseData, responseData) != 0)
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
static void DigitalTwinSampleEnvironmentalSensor_BlinkCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE* sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)userInterfaceContext;

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Blink command invoked.  It has been invoked %d times previously", sensorState->numTimesBlinkCommandCalled);
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Blink data=<%.*s>", (int)dtCommandRequest->requestDataLen, dtCommandRequest->requestData);

    sensorState->numTimesBlinkCommandCalled++;

    (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponse(dtCommandResponse, digitaltwinSample_EnviromentalSensor_BlinkResponse, commandStatusSuccess);
}

// Implement the callback to process the command "turnon".
static void DigitalTwinSampleEnvironmentalSensor_TurnOnLightCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE* sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)userInterfaceContext;
    (void)sensorState; // Sensor state not used in this sample

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Turn on light command invoked");
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Turn on light data=<%.*s>", (int)dtCommandRequest->requestDataLen, dtCommandRequest->requestData);

    (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponseEmptyBody(dtCommandResponse, commandStatusSuccess);
}

// Implement the callback to process the command "turnoff".
static void DigitalTwinSampleEnvironmentalSensor_TurnOffLightCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE* sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)userInterfaceContext;
    (void)sensorState; // Sensor state not used in this sample

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Turn off light command invoked");
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Turn off light data=<%.*s>", (int)dtCommandRequest->requestDataLen, dtCommandRequest->requestData);

    (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponseEmptyBody(dtCommandResponse, commandStatusSuccess);
}


// Implement the callback to process the command "rundiagnostic".  Note that this is an asyncronous command, so all we do in this 
// stage is to do some rudimentary checks and to store off the fact we're async for later.
static void DigitalTwinSampleEnvironmentalSensor_RunDiagnosticsCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE *sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)userInterfaceContext;

    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Run diagnostics command invoked");
    LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Diagnostics data=<%.*s>, requestId=<%s>", (int)dtCommandRequest->requestDataLen, dtCommandRequest->requestData, dtCommandRequest->requestId);

    if (sensorState->diagnosticState != DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_INACTIVE)
    {
        // If the diagnostic is already in progress, do not allow simultaneous requests.
        // Note that the requirement for only one simultaneous asynchronous command at a time *is for simplifying the sample only*.
        // The underlying DigitalTwin protocol will allow multiple simultaneous requests to be sent to the client; whether the
        // device allows this or not is a decision for the interface & device implementors.
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Run diagnostics already active.  Cannot support multiple in parallel.");
        DigitalTwinSampleEnvironmentalSensor_SetCommandResponse(dtCommandResponse, digitaltwinSample_EnviromentalSensor_RunDiagnosticsBusy, commandStatusFailure);
    }
    // At this point we need to save the requestId.  This is what the server uses to correlate subsequent responses from this operation.
    else if (mallocAndStrcpy_s(&sensorState->requestId, dtCommandRequest->requestId) != 0)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Cannot allocate requestId.");
        (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponse(dtCommandResponse, digitaltwinSample_EnviromentalSensor_OutOfMemory, commandStatusFailure);
    }
    else if (DigitalTwinSampleEnvironmentalSensor_SetCommandResponseEmptyBody(dtCommandResponse, commandStatusPending) != 0)
    {
        // Because DigitalTwinSampleEnvironmentalSensor_SetCommandResponse failed, it means 
        // the server will get an error response back.  Do NOT change our diagnosticState
        // variable in this case or else server and client will be in different states.
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Failed setting response to server.");
        free(sensorState->requestId);
    }
    else
    {
        // Moving us into DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_PHASE1 will mean our periodic wakeup will process this.
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Successfully set sensorState to run diagnostics.  Will run later.");
        sensorState->diagnosticState = DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_PHASE1;
    }
}

// DigitalTwinSampleEnvironmentalSensor_SetAsyncUpdateState is a helper to fill in a DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE structure.
static void DigitalTwinSampleEnvironmentalSensor_SetAsyncUpdateState(DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE *asyncCommandUpdate, const char* propertyData, int status)
{
    memset(asyncCommandUpdate, 0, sizeof(*asyncCommandUpdate));
    asyncCommandUpdate->version = DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE_VERSION_1;
    asyncCommandUpdate->commandName = digitaltwinSample_EnvironmentalSensorCommandRunDiagnostics;
    asyncCommandUpdate->requestId = digitaltwinSample_EnvironmentalSensorState.requestId;
    asyncCommandUpdate->propertyData = propertyData;
    asyncCommandUpdate->statusCode = status;
}

// DigitalTwinSampleEnvironmentalSensor_ProcessDiagnosticIfNecessaryAsync is periodically invoked in this sample by the main()
// thread.  It will evaluate whether the service has requested diagnostics to be run, which is an async operation.  If so,
// it will send the server an update status message depending on the state that we're at.
//
// THREADING NOTE: When this interface is invoked on the convenience layer (../digitaltwin_sample_device), this operation can
// run on any thread - while processing a callback, on the main() thread itself, or on a new thread spun up by the process.
// When running on the _LL_ layer (../digitaltwin_sample_ll_device) it *must* run on the main() thread because the the _LL_ is not
// thread safe by design.
DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleEnvironmentalSensor_ProcessDiagnosticIfNecessaryAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR;

    if (digitaltwinSample_EnvironmentalSensorState.diagnosticState == DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_INACTIVE)
    {
        // No pending commands to process, so this is a no-op
        result = DIGITALTWIN_CLIENT_OK;
    }
    else if (digitaltwinSample_EnvironmentalSensorState.diagnosticState == DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_PHASE1)
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: In phase1 of running diagnostics.  Will alert server that we are still in progress and transition to phase2");
            
        // In phase1 of the diagnostic, we *only* report that the diagnostic is in progress but not yet complete.  We also transition to the next stage.
        DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE asyncCommandUpdate;
        DigitalTwinSampleEnvironmentalSensor_SetAsyncUpdateState(&asyncCommandUpdate, digitaltwinSample_EnviromentalSensor_DiagnosticInProgress, commandStatusPending);

        if ((result = DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync(interfaceHandle, &asyncCommandUpdate)) != DIGITALTWIN_CLIENT_OK)
        {
            // We continue processing the diagnostic run even on DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync failure.
            // The UpdateAsync command is just a notification to server of state; the underlying diagnostic should not 
            // be blocked as it has already been initiated.
            LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        }
        else
        {
            result = DIGITALTWIN_CLIENT_OK;
        }

        digitaltwinSample_EnvironmentalSensorState.diagnosticState = DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_PHASE2;
    }
    else if (digitaltwinSample_EnvironmentalSensorState.diagnosticState == DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_PHASE2)
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: In phase2 of running diagnostics.  Will alert server that we are complete.");

        // In phase2 of the diagnostic, we're complete.  Indicate to service, free resources, and move us to inactive phase so subsequent commands can arrive.
        DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE asyncCommandUpdate;
        DigitalTwinSampleEnvironmentalSensor_SetAsyncUpdateState(&asyncCommandUpdate, digitaltwinSample_EnviromentalSensor_DiagnosticsComplete, commandStatusSuccess);

        if ((result = DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync(interfaceHandle, &asyncCommandUpdate)) != DIGITALTWIN_CLIENT_OK)
        {
            // See comments for DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync above for error handling (or lack thereof) motivation.
            LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        }
        else
        {
            result = DIGITALTWIN_CLIENT_OK;
        }

        free(digitaltwinSample_EnvironmentalSensorState.requestId);
        digitaltwinSample_EnvironmentalSensorState.requestId = NULL;
        digitaltwinSample_EnvironmentalSensorState.diagnosticState = DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_INACTIVE;
    }

    return result;
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
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Updating property property=<%s> failed, error=<%s>", (const char*)userContextCallback, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtReportedStatus));
    }
}

// Processes a property update, which the server initiated, for customer name.
static void DigitalTwinSampleEnvironmentalSensor_CustomerNameCallback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE *sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)userInterfaceContext;
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
    result = DigitalTwin_InterfaceClient_ReportPropertyAsync(sensorState->interfaceClientHandle, digitaltwinSample_EnvironmentalSensorPropertyCustomerName, (const char*)dtClientPropertyUpdate->propertyDesired,
                                                             &propertyResponse, DigitalTwinSampleEnvironmentalSensor_PropertyCallback, (void*)digitaltwinSample_EnvironmentalSensorPropertyCustomerName);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_ReportPropertyAsync for CustomerName failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
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
static void DigitalTwinSampleEnvironmentalSensor_BrightnessCallback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE *sensorState = (DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_STATE*)userInterfaceContext;
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
    result = DigitalTwin_InterfaceClient_ReportPropertyAsync(sensorState->interfaceClientHandle, digitaltwinSample_EnvironmentalSensorPropertyBrightness, (const char*)dtClientPropertyUpdate->propertyDesired, &propertyResponse,
                                                             DigitalTwinSampleEnvironmentalSensor_PropertyCallback, (void*)digitaltwinSample_EnvironmentalSensorPropertyBrightness);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_ReportPropertyAsync for Brightness failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
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

    result = DigitalTwin_InterfaceClient_ReportPropertyAsync(interfaceHandle, digitaltwinSample_DeviceStateProperty, digitaltwinSample_DeviceStateData,
                                                             NULL, DigitalTwinSampleEnvironmentalSensor_PropertyCallback, (void*)digitaltwinSample_DeviceStateProperty);

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Reporting property=<%s> failed, error=<%s>", digitaltwinSample_DeviceStateProperty, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
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
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Interface received failed, status=<%s>.", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus));
    }
}


// DigitalTwinSample_ProcessCommandUpdate receives commands from the server.  This implementation acts as a simple dispatcher
// to the functions to perform the actual processing.
void DigitalTwinSample_ProcessCommandUpdate(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    if (strcmp(dtCommandRequest->commandName, digitaltwinSample_EnvironmentalSensorCommandBlink) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_BlinkCallback(dtCommandRequest, dtCommandResponse, userInterfaceContext);
    }
    else if (strcmp(dtCommandRequest->commandName, digitaltwinSample_EnvironmentalSensorCommandTurnOn) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_TurnOnLightCallback(dtCommandRequest, dtCommandResponse, userInterfaceContext);
    }
    else if (strcmp(dtCommandRequest->commandName, digitaltwinSample_EnvironmentalSensorCommandTurnOff) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_TurnOffLightCallback(dtCommandRequest, dtCommandResponse, userInterfaceContext);
    }
    else if (strcmp(dtCommandRequest->commandName, digitaltwinSample_EnvironmentalSensorCommandRunDiagnostics) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_RunDiagnosticsCallback(dtCommandRequest, dtCommandResponse, userInterfaceContext);
    }
    else
    {
        // If the command is not implemented by this interface, by convention we return a 501 error to server.
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Command name <%s> is not associated with this interface", dtCommandRequest->commandName);
        (void)DigitalTwinSampleEnvironmentalSensor_SetCommandResponse(dtCommandResponse, digitaltwinSample_EnviromentalSensor_NotImplemented, commandStatusNotPresent);
    }
}

// DigitalTwinSampleEnvironmentalSensor_ProcessPropertyUpdate receives updated properties from the server.  This implementation
// acts as a simple dispatcher to the functions to perform the actual processing.
static void DigitalTwinSampleEnvironmentalSensor_ProcessPropertyUpdate(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    if (strcmp(dtClientPropertyUpdate->propertyName, digitaltwinSample_EnvironmentalSensorPropertyCustomerName) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_CustomerNameCallback(dtClientPropertyUpdate, userInterfaceContext);
    }
    else if (strcmp(dtClientPropertyUpdate->propertyName, digitaltwinSample_EnvironmentalSensorPropertyBrightness) == 0)
    {
        DigitalTwinSampleEnvironmentalSensor_BrightnessCallback(dtClientPropertyUpdate, userInterfaceContext);
    }
    else
    {
        // If the property is not implemented by this interface, presently we only record a log message but do not have a mechanism to report back to the service
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Property name <%s> is not associated with this interface", dtClientPropertyUpdate->propertyName);
    }
}


//
// DigitalTwinSampleEnvironmentalSensor_CreateInterface is the initial entry point into the DigitalTwin Sample Environmental Sensor interface.
// It simply creates a DIGITALTWIN_INTERFACE_CLIENT_HANDLE that is mapped to the environmental sensor interface name.
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
    digitaltwinSample_EnvironmentalSensorState.diagnosticState = DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_DIAGNOSTIC_STATE_INACTIVE;
    
    if ((result =  DigitalTwin_InterfaceClient_Create(DigitalTwinSampleEnvironmentalSensor_InterfaceId, DigitalTwinSampleEnvironmentalSensor_InterfaceInstanceName, DigitalTwinSampleEnvironmentalSensor_InterfaceRegisteredCallback, (void*)&digitaltwinSample_EnvironmentalSensorState, &interfaceHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Unable to allocate interface client handle for interfaceId=<%s>, interfaceInstanceName=<%s>, error=<%s>", DigitalTwinSampleEnvironmentalSensor_InterfaceId, DigitalTwinSampleEnvironmentalSensor_InterfaceInstanceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceHandle = NULL;
    }
    else if ((result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(interfaceHandle, DigitalTwinSampleEnvironmentalSensor_ProcessPropertyUpdate)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        DigitalTwinSampleEnvironmentalSensor_Close(interfaceHandle);
        interfaceHandle = NULL;
    }
    else if ((result = DigitalTwin_InterfaceClient_SetCommandsCallback(interfaceHandle, DigitalTwinSample_ProcessCommandUpdate)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallback failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        DigitalTwinSampleEnvironmentalSensor_Close(interfaceHandle);
        interfaceHandle = NULL;
    }
    else
    {
        LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE.  interfaceId=<%s>, interfaceInstanceName=<%s>, handle=<%p>", DigitalTwinSampleEnvironmentalSensor_InterfaceId, DigitalTwinSampleEnvironmentalSensor_InterfaceInstanceName, interfaceHandle);
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
    if (dtTelemetryStatus == DIGITALTWIN_CLIENT_OK)
    {
        // This tends to overwhelm the logging on output based on how frequently this function is invoked, so removing by default.
        // LogInfo("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin successfully delivered telemetry message for <%s>", (const char*)userContextCallback);
    }
    else
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin failed delivered telemetry message for <%s>, error=<%s>", (const char*)userContextCallback, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT,dtTelemetryStatus));
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

    char currentTemperatureMessage[128];
    char currentHumidityMessage[128];

    sprintf(currentTemperatureMessage, "%.3f", currentTemperature);
    sprintf(currentHumidityMessage, "%.3f", currentHumidity);

    if ((result = DigitalTwin_InterfaceClient_SendTelemetryAsync(interfaceHandle, DigitalTwinSampleEnvironmentalSensor_TemperatureTelemetry, 
                                                         currentTemperatureMessage, DigitalTwinSampleEnvironmentalSensor_TelemetryCallback, 
                                                         (void*)DigitalTwinSampleEnvironmentalSensor_TemperatureTelemetry)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_SendTelemetryAsync failed for sending %s", DigitalTwinSampleEnvironmentalSensor_TemperatureTelemetry);
    }
    else if ((result = DigitalTwin_InterfaceClient_SendTelemetryAsync(interfaceHandle, DigitalTwinSampleEnvironmentalSensor_HumidityTelemetry, 
                                                         currentHumidityMessage, DigitalTwinSampleEnvironmentalSensor_TelemetryCallback, 
                                                         (void*)DigitalTwinSampleEnvironmentalSensor_HumidityTelemetry)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_SendTelemetryAsync failed for sending %s", DigitalTwinSampleEnvironmentalSensor_HumidityTelemetry);
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

    free(digitaltwinSample_EnvironmentalSensorState.requestId);
    digitaltwinSample_EnvironmentalSensorState.requestId = NULL;
}

