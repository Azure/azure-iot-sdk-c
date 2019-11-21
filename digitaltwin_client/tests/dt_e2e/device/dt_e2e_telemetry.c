// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <azure_c_shared_utility/xlogging.h>

#include "dt_e2e_telemetry.h"
#include "dt_e2e_util.h"


//*****************************************************************************
// Logic to tie test commands into DigitalTwin framework and the test's main()
//*****************************************************************************

// Interface name for telemetry testing
static const char DT_E2E_Telemetry_InterfaceId[] = "urn:azureiot:testinterfaces:cdevicesdk:telemetry:1";
static const char DT_E2E_Telemetry_InterfaceName[] = "testTelemetry";

// Command names that the service SDK invokes to invoke tests on this interface
static const char* DT_E2E_telemetry_commandNames[] = { 
    "SendTelemetryMessage",
    "VerifyTelemetryAcknowledged"
};

static void DT_E2E_SendTelemetryMessage(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);
static void DT_E2E_VerifyTelemetryAcknowledged(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);
static void DT_E2E_TelemetryCommandCallbackProcess(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);

// Functions that implement tests service invokes for this interface; maps to DT_E2E_telemetry_commandNames.
static const DIGITALTWIN_COMMAND_EXECUTE_CALLBACK DT_E2E_telemetry_commandCallbacks[] = { 
    DT_E2E_SendTelemetryMessage,
    DT_E2E_VerifyTelemetryAcknowledged
};

static const int DT_E2E_number_TelemetryCommands = sizeof(DT_E2E_telemetry_commandCallbacks) / sizeof(DT_E2E_telemetry_commandCallbacks[0]);

//
// DT_E2E_TelemetryCommandCallbackProcess receives all commands from the service.
//
static void DT_E2E_TelemetryCommandCallbackProcess(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    DT_E2E_Util_InvokeCommand(DT_E2E_telemetry_commandNames, DT_E2E_telemetry_commandCallbacks, DT_E2E_number_TelemetryCommands, dtCommandRequest, dtCommandResponse, userInterfaceContext);
}

// DT_E2E_TELEMETRY_TEST_CONTEXT specifies the context passed to device method callback routines for telemetry testing.
typedef struct DT_E2E_TELEMETRY_TEST_CONTEXT_TAG
{
    // Interface handle associated with this callback
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;
    // The number of messages this test interface has queued
    unsigned int numMessagesSent;
    // The number of messages this test interface has received a server side ACK for
    unsigned int numMessagesAcknowledged;
} DT_E2E_TELEMETRY_TEST_CONTEXT;

DT_E2E_TELEMETRY_TEST_CONTEXT DT_E2E_TelemetryTest_Context;

//
// DT_E2E_Telemetry_CreateInterface is invoked by test's main() to setup the DIGITALTWIN_INTERFACE_CLIENT_HANDLE for this test.
//
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DT_E2E_Telemetry_CreateInterface(void)
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    memset(&DT_E2E_TelemetryTest_Context, 0, sizeof(DT_E2E_TelemetryTest_Context));
   
    if ((result =  DigitalTwin_InterfaceClient_Create(DT_E2E_Telemetry_InterfaceId, DT_E2E_Telemetry_InterfaceName, NULL, &DT_E2E_TelemetryTest_Context, &interfaceHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_TELEMETRY_INTERFACE: Unable to allocate interface client handle for <%s>, error=<%s>", DT_E2E_Telemetry_InterfaceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceHandle = NULL;
    }
    else if ((result =  DigitalTwin_InterfaceClient_SetCommandsCallback(interfaceHandle, DT_E2E_TelemetryCommandCallbackProcess, &DT_E2E_TelemetryTest_Context)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_TELEMETRY_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallbacks failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        DigitalTwin_InterfaceClient_Destroy(interfaceHandle);
        interfaceHandle = NULL;
    }
    else
    {
        DT_E2E_TelemetryTest_Context.interfaceHandle = interfaceHandle;
        LogInfo("TEST_TELEMETRY_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE.  InterfaceName=<%s>, handle=<%p>", DT_E2E_Telemetry_InterfaceName, interfaceHandle);
    }

    return interfaceHandle;
}

//
// DT_E2E_Telemetry_DestroyInterface frees interface handle created at test initialization
//
void DT_E2E_Telemetry_DestroyInterface(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    if (interfaceHandle != NULL)
    {
        DigitalTwin_InterfaceClient_Destroy(interfaceHandle);
    }
}

//*****************************************************************************
// DigitalTwin specific telemetry helpers
//*****************************************************************************

// Values to send service when a test fails on device side
static const char* DT_E2E_CommandFailedMessage = "\"Test failed on device side.\"";
static const int DT_E2E_CommandFailedStatus = 500;

//*****************************************************************************
// Actual test routines and data
//*****************************************************************************

//
// DT_E2E_TelemetryConfirmationCallback is invoked when a telemetry message is acknowledged by
// the service, either on success or failure
//
void DT_E2E_TelemetryConfirmationCallback(DIGITALTWIN_CLIENT_RESULT dtTelemetryStatus, void* userContextCallback)
{
    if (dtTelemetryStatus != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_TELEMETRY_INTERFACE: DT_E2E_TelemetryConfirmationCallback callback received error=<%d>", dtTelemetryStatus);
        DT_E2E_Util_Fatal_Error_Occurred();
    }
    else
    {
        DT_E2E_TELEMETRY_TEST_CONTEXT* telemetryTestContext = (DT_E2E_TELEMETRY_TEST_CONTEXT*)userContextCallback;
        telemetryTestContext->numMessagesAcknowledged++;
        LogInfo("TEST_TELEMETRY_INTERFACE: DT_E2E_TelemetryConfirmationCallback received successful ack.  <%d> messages successfully ACK'd so far", telemetryTestContext->numMessagesAcknowledged);
    }
}


static const char* DT_E2E_Telemetry_MessageName = "DT_E2E_TestTelemetryName";
static const char* DT_E2E_TelemetrySentSuccessMessage = "\"Successfully Queued Telemetry message\"";
static const int DT_E2E_TelemetryCommandSuccessStatus = 200;


// 
// DT_E2E_SendTelemetryMessage will trigger a telemetry message to be sent, containing the specified payload
//
static void DT_E2E_SendTelemetryMessage(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    DT_E2E_TELEMETRY_TEST_CONTEXT* telemetryTestContext = (DT_E2E_TELEMETRY_TEST_CONTEXT*)userInterfaceContext;

    DIGITALTWIN_CLIENT_RESULT result;

    if ((result = DigitalTwin_InterfaceClient_SendTelemetryAsync(telemetryTestContext->interfaceHandle, dtCommandRequest->requestData, dtCommandRequest->requestDataLen,
                                                                 DT_E2E_TelemetryConfirmationCallback, telemetryTestContext)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_TELEMETRY_INTERFACE: DigitalTwin_InterfaceClient_SendTelemetryAsync fails, error=<%d>", result);
        DT_E2E_Util_Fatal_Error_Occurred();
        SetCommandResponse(dtCommandResponse, DT_E2E_CommandFailedMessage, DT_E2E_CommandFailedStatus);
    }
    else
    {
        telemetryTestContext->numMessagesSent++;
        LogInfo("TEST_TELEMETRY_INTERFACE: DigitalTwin_InterfaceClient_SendTelemetryAsync successfully queued telemetry message.  Total queued=<%d>", telemetryTestContext->numMessagesSent);
        SetCommandResponse(dtCommandResponse, DT_E2E_TelemetrySentSuccessMessage, DT_E2E_TelemetryCommandSuccessStatus);
    }
}

// 
// DT_E2E_VerifyTelemetryAcknowledged makes sure that for each telemetry message that has been sent, an acknowledegment has been received.
// Because messages may take time to be ack'd after initial send, the initiator may need to call this function multiple times as ack's come in.
//

static const char* DT_E2E_VerifyAllTelemetryAcknowledgedMessage = "\"All telemetry messages sent have been acknowledged\"";
static const char* DT_E2E_NotAllTelemetryAcknowledgedMessage = "\"Not telemetry messages sent have been acknowledged\"";
// Use a 202 here to indicate that this status is not necessarily fatal (remote caller will need to poll again)
static const int DT_E2E_NotAllTelemetryAcknowledgedStatus = 202;

static void DT_E2E_VerifyTelemetryAcknowledged(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    (void)dtCommandRequest;
    DT_E2E_TELEMETRY_TEST_CONTEXT* telemetryTestContext = (DT_E2E_TELEMETRY_TEST_CONTEXT*)userInterfaceContext;

    if (telemetryTestContext->numMessagesSent != telemetryTestContext->numMessagesAcknowledged)
    {
        // The use of LogInfo() is intentional; the server will need to call this function multiple times to poll
        // for all messages being sent.  While technically this is a verification failure, given how test fits into e2e 
        // framework this state is not considered fatal.
        LogInfo("TEST_TELEMETRY_INTERFACE: Number of sent messages <%d> is NOT identical to number of ack'd messages <%d>", telemetryTestContext->numMessagesSent, telemetryTestContext->numMessagesAcknowledged);
        SetCommandResponse(dtCommandResponse, DT_E2E_NotAllTelemetryAcknowledgedMessage, DT_E2E_NotAllTelemetryAcknowledgedStatus);
    }
    else
    {
        LogInfo("TEST_TELEMETRY_INTERFACE: Number of sent messages <%d> is identical to number of ack'd messages", telemetryTestContext->numMessagesSent);
        SetCommandResponse(dtCommandResponse, DT_E2E_VerifyAllTelemetryAcknowledgedMessage, DT_E2E_TelemetryCommandSuccessStatus);
    }
}

