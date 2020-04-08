// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This file implements logic specific to DigitalTwin commands.

#include <stdlib.h>
#include <azure_c_shared_utility/xlogging.h>

#include "dt_e2e_commands.h"
#include "dt_e2e_util.h"


//*****************************************************************************
// Logic to tie test commands into DigitalTwin framework and the test's main()
//*****************************************************************************
static void* DT_E2E_CommandContext = (void*)0x12344321;

// Interface name for commands testing
static const char DT_E2E_Properties_InterfaceId[] = "dtmi:azureiot:testinterfaces:cdevicesdk:commands;1";
static const char DT_E2E_Properties_InterfaceName[] = "testCommands";

// Command names that the service SDK invokes to invoke tests on this interface
static const char* DT_E2E_command_commandNames[] = { 
    "TerminateTestRun",
    "PayloadRequestResponse1",
    "PayloadRequestResponse2",
    "PayloadEmptyRequestResponse",
    "ReturnErrorWithResponseBody",
    "ReturnErrorEmptyResponseBody"
};

static void DT_E2E_PayloadRequestResponse1(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);
static void DT_E2E_TerminateTestRun(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);
static void DT_E2E_PayloadRequestResponse2(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);
static void DT_E2E_PayloadEmptyRequestResponse(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);
static void DT_E2E_ReturnErrorWithResponseBody(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);
static void DT_E2E_ReturnErrorEmptyResponseBody(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext);

// Functions that implement test commands that the test driver invokes for this interface.
static const DIGITALTWIN_COMMAND_EXECUTE_CALLBACK DT_E2E_commands_commandCallbacks[] = { 
    DT_E2E_TerminateTestRun,
    DT_E2E_PayloadRequestResponse1,
    DT_E2E_PayloadRequestResponse2,
    DT_E2E_PayloadEmptyRequestResponse,
    DT_E2E_ReturnErrorWithResponseBody,
    DT_E2E_ReturnErrorEmptyResponseBody
};

static const int DT_E2E_number_commands = sizeof(DT_E2E_command_commandNames) / sizeof(DT_E2E_command_commandNames[0]);

//
// DT_E2E_CommandCallbackProcess receives all commands from the service.
//
static void DT_E2E_CommandCallbackProcess(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    DT_E2E_Util_InvokeCommand(DT_E2E_command_commandNames, DT_E2E_commands_commandCallbacks, DT_E2E_number_commands, dtCommandRequest, dtCommandResponse, userInterfaceContext);
}

//
// DT_E2E_Commands_CreateInterface is invoked by test's main() to setup the DIGITALTWIN_INTERFACE_CLIENT_HANDLE for this test.
//
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DT_E2E_Commands_CreateInterface(void)
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;
    DIGITALTWIN_CLIENT_RESULT result;
   
    if ((result =  DigitalTwin_InterfaceClient_Create(DT_E2E_Properties_InterfaceId, DT_E2E_Properties_InterfaceName, NULL, DT_E2E_CommandContext, &interfaceHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_COMMANDS_INTERFACE: Unable to allocate interface client handle for <%s>, error=<%s>", DT_E2E_Properties_InterfaceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceHandle = NULL;
    }
    else if ((result =  DigitalTwin_InterfaceClient_SetCommandsCallback(interfaceHandle, DT_E2E_CommandCallbackProcess, DT_E2E_CommandContext)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_COMMANDS_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallbacks failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        DigitalTwin_InterfaceClient_Destroy(interfaceHandle);
        interfaceHandle = NULL;
    }
    else
    {
        LogInfo("TEST_COMMANDS_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE.  InterfaceName=<%s>, handle=<%p>", DT_E2E_Properties_InterfaceName, interfaceHandle);
    }

    return interfaceHandle;
}

//
// DT_E2E_Commands_DestroyInterface frees interface handle created at test initialization
//
void DT_E2E_Commands_DestroyInterface(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    if (interfaceHandle != NULL)
    {
        DigitalTwin_InterfaceClient_Destroy(interfaceHandle);
    }
}


//*****************************************************************************
// DigitalTwin specific command helpers
//*****************************************************************************

// Values to send service when a test fails on device side
static const char* DT_E2E_CommandFailedMessage = "\"Test failed on device side.\"";
static const int DT_E2E_CommandFailedStatus = 500;

//
// VerifyExpectedCommand makes sure that the dtCommandRequest provided by the DigitalTwin SDK matches the expected for a given test.
//
static int VerifyExpectedCommand(const char* testName, const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, const char* expectedPayload, void* userInterfaceContext)
{
    int result;

    if (userInterfaceContext != DT_E2E_CommandContext)
    {
        LogError("TEST_COMMANDS_INTERFACE:: Test <%s> fails.  Requested payload user context is <%p> but expected is <%p>",
                 testName, userInterfaceContext, DT_E2E_CommandContext);
        DT_E2E_Util_Fatal_Error_Occurred();
        result = MU_FAILURE;
    }
    else if (dtCommandRequest->version != DIGITALTWIN_CLIENT_COMMAND_REQUEST_VERSION_1)
    {
        LogError("TEST_COMMANDS_INTERFACE:: Test <%s> fails.  Requested payload structure version is <%d> but expected is <%d>", 
                 testName, dtCommandRequest->version, DIGITALTWIN_CLIENT_COMMAND_REQUEST_VERSION_1);
        DT_E2E_Util_Fatal_Error_Occurred();
        result = MU_FAILURE;
    }
    else if (strncmp((const char*)dtCommandRequest->requestData, expectedPayload, strlen(expectedPayload)) != 0)
    {
        LogError("TEST_COMMANDS_INTERFACE:: Test <%s> fails.  Requested payload is <%s> but expected is <%s>", 
                 testName, (const char*)dtCommandRequest->requestData, expectedPayload);
        DT_E2E_Util_Fatal_Error_Occurred();
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

//*****************************************************************************
// Actual test routines and data
//*****************************************************************************

//
//  DT_E2E_TerminateTestRun is invoked by the service caller when it is time to terminate the test executable.
//
static const int DT_E2E_TerminateTestRun_Status = 200;

static void DT_E2E_TerminateTestRun(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    (void)dtCommandRequest;
    (void)userInterfaceContext;
    DT_E2E_Util_Gracefully_End_Test_Execution();
    SetCommandResponseEmptyPayload(dtCommandResponse, DT_E2E_TerminateTestRun_Status);
}

//
// DT_E2E_PayloadRequestResponse1 simply verifies that expected payload is received and sets a pre-defined response payload that the service caller checks.
//
static const char* DT_E2E_PayloadRequest1_ExpectedRequest = "\"ExpectedPayload for DT_E2E_PayloadRequestResponse1\"";
static const char* DT_E2E_PayloadRequest1_Response = "\"DT_E2E_PayloadRequestResponse1's response\"";
static const int DT_E2E_PayloadRequest1_Status = 200;

static void DT_E2E_PayloadRequestResponse1(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    if (VerifyExpectedCommand(__FUNCTION__, dtCommandRequest, DT_E2E_PayloadRequest1_ExpectedRequest, userInterfaceContext) == 0)
    {
        SetCommandResponse(dtCommandResponse, DT_E2E_PayloadRequest1_Response, DT_E2E_PayloadRequest1_Status);
    }
    else
    {
        SetCommandResponse(dtCommandResponse, DT_E2E_CommandFailedMessage, DT_E2E_CommandFailedStatus);
    }
}

//
// DT_E2E_PayloadRequestResponse2 simply verifies that expected payload is received and sets a pre-defined response payload that the service caller checks.
//
static const char* DT_E2E_PayloadRequest2_ExpectedRequest = "{\"SlightlyMoreComplexRequestJson\":{\"SomethingEmbedded\":[1,2,3,4]}}";
static const char* DT_E2E_PayloadRequest2_Response = "{\"SlightlyMoreComplexResponseJson\":{\"SomethingEmbedded\":[5,6,7,8]}}";
static const int DT_E2E_PayloadRequest2_Status = 200;

static void DT_E2E_PayloadRequestResponse2(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    if (VerifyExpectedCommand(__FUNCTION__, dtCommandRequest, DT_E2E_PayloadRequest2_ExpectedRequest, userInterfaceContext) == 0)
    {
        SetCommandResponse(dtCommandResponse, DT_E2E_PayloadRequest2_Response, DT_E2E_PayloadRequest2_Status);
    }
    else
    {
        SetCommandResponse(dtCommandResponse, DT_E2E_CommandFailedMessage, DT_E2E_CommandFailedStatus);
    }
}

//
// DT_E2E_PayloadEmptyRequestResponse verifies that all is well when both Service SDK and this test have empty payloads specified
//
// Even if the service SDK sets NULL/None/etc. "pointer" on its side, it is translated to "null" by SDK
static const char* DT_E2E_EmptyPayloadRequest_ExpectedRequest = "null";
static const int DT_E2E_EmptyPayloadRequest_Status = 200;

static void DT_E2E_PayloadEmptyRequestResponse(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    if (VerifyExpectedCommand(__FUNCTION__, dtCommandRequest, DT_E2E_EmptyPayloadRequest_ExpectedRequest, userInterfaceContext) == 0)
    {
        SetCommandResponseEmptyPayload(dtCommandResponse, DT_E2E_EmptyPayloadRequest_Status);
    }
    else
    {
        SetCommandResponse(dtCommandResponse, DT_E2E_CommandFailedMessage, DT_E2E_CommandFailedStatus);
    }
}

//
// DT_E2E_ReturnErrorWithResponseBody returns an error code as result of command.  The error code itself is specified in the payload of the request body from the service.
// A request payload is included as part of the response.
//
static const char* DT_E2E_ReturnErrorWithResponseBody_Response = "\" Payload data returned as part of error response \"";

static void DT_E2E_ReturnErrorWithResponseBody(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    (void)userInterfaceContext;
    int desiredResponseCode = atoi((const char*)dtCommandRequest->requestData);

    if (desiredResponseCode == 0)
    {
        LogError("TEST_COMMANDS_INTERFACE:  Request data <%s> is not an integer.  It needs to be for response to server", (const char*)dtCommandRequest->requestData);
        DT_E2E_Util_Fatal_Error_Occurred();
        SetCommandResponse(dtCommandResponse, DT_E2E_CommandFailedMessage, DT_E2E_CommandFailedStatus);
    }
    else
    {
        SetCommandResponse(dtCommandResponse, DT_E2E_ReturnErrorWithResponseBody_Response, desiredResponseCode);
    }
}

//
// DT_E2E_ReturnErrorEmptyResponseBody returns an error code as result of command.  The error code itself is specified in the payload of the request body from the service.
// A request payload is NOT included as part of the response.
// 
static void DT_E2E_ReturnErrorEmptyResponseBody(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    (void)userInterfaceContext;
    int desiredResponseCode = atoi((const char*)dtCommandRequest->requestData);

    if (desiredResponseCode == 0)
    {
        LogError("TEST_COMMANDS_INTERFACE:  Request data <%s> is not an integer.  It needs to be for response to server", (const char*)dtCommandRequest->requestData);
        DT_E2E_Util_Fatal_Error_Occurred();
        SetCommandResponse(dtCommandResponse, DT_E2E_CommandFailedMessage, DT_E2E_CommandFailedStatus);
    }
    else
    {
        SetCommandResponseEmptyPayload(dtCommandResponse, desiredResponseCode);
    }
}

