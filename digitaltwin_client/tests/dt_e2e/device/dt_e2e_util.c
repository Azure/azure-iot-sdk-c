// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <azure_c_shared_utility/xlogging.h>

#include "dt_e2e_util.h"


// dtTestE2EContinueExecution controls whether the test is active or when it shuts down gracefully (e.g. initiated by remote test script)
bool dtTestE2EContinueExecution;

// dtTestE2EFatalError is set when an error that should cause the test execution to terminate is hit.
bool dtTestE2EFatalError;

//
// DT_E2E_Util_Init inits core utility variables
//
void DT_E2E_Util_Init()
{
    dtTestE2EContinueExecution = true;
    dtTestE2EFatalError = false;
}

//
// DT_E2E_Util_Gracefully_End_Test_Execution is invoked when a test
// indicates the test driver framework is requesting a graceful shutdown.
//
void DT_E2E_Util_Gracefully_End_Test_Execution()
{
    LogInfo("Marking test EXE as ready for graceful shutdown");
    dtTestE2EContinueExecution = false;
}

//
// DT_E2E_Util_Fatal_Error_Occurred is called when a fatal test error has occured, 
// e.g. unexpected incoming parameters or a memory allocation failure or ...
// We do NOT shutdown the test execution if this happens, since we don't know the severity
// of the error and whether it'd impede additional tests from executing.
//
void DT_E2E_Util_Fatal_Error_Occurred()
{
    LogInfo("Marking test EXE as having received a fatal error.");
    dtTestE2EFatalError = true;
}

//
// DT_E2E_Util_Should_Continue_Execution indicates whether or not the test should continue 
//
bool DT_E2E_Util_Should_Continue_Execution()
{
    return dtTestE2EContinueExecution;
}

//
// DT_E2E_Util_Should_Continue_Execution indicates whether the test has failed.
//
bool DT_E2E_Util_Has_Fatal_Error_Occurred()
{
    return dtTestE2EFatalError;
}

//
// SetCommandResponse fills out the dtCommandResponse structure to return to the DigitalTwin service caller.
//
void SetCommandResponse(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, const char* responseData, int responseStatus)
{
    size_t responseLen = strlen(responseData);
    memset(dtCommandResponse, 0, sizeof(*dtCommandResponse));
    dtCommandResponse->version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;

    // Allocate a copy of the response data to return to the invoker.  The DigitalTwin layer that invoked the 
    // given callback takes responsibility for freeing this data.
    if ((dtCommandResponse->responseData = (unsigned char*)malloc(responseLen + 1)) == NULL)
    {
        LogError("TEST_TELEMETRY_INTERFACE:  Unable to allocate response data");
        DT_E2E_Util_Fatal_Error_Occurred();
        dtCommandResponse->status = 500;
    }
    else
    {
        strcpy((char*)dtCommandResponse->responseData, responseData);
        dtCommandResponse->responseDataLen = responseLen;
        dtCommandResponse->status = responseStatus;
    }
}

//
// SetCommandResponseEmptyPayload sets up dtCommandResponse when returning an empty payload
//
void SetCommandResponseEmptyPayload(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, int responseStatus)
{
    memset(dtCommandResponse, 0, sizeof(*dtCommandResponse));
    dtCommandResponse->version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;
    dtCommandResponse->status = responseStatus;
}

//
// DT_E2E_Util_InvokeCommand will dispatch appropriate command based on name/function callback in table.  Each individual test (commands/properties/telemetry)
// provides this callback with its test specific table and the command received and this handles the rest.
//
void DT_E2E_Util_InvokeCommand(const char** commandNames, const DIGITALTWIN_COMMAND_EXECUTE_CALLBACK* commandCallbacks, const int numCommands,
                               const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, 
                               DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, 
                               void* userInterfaceContext)
{
    int i;
    
    for (i= 0; i < numCommands; i++)
    {
        if (strcmp(commandNames[i], dtCommandRequest->commandName) == 0)
        {
            commandCallbacks[i](dtCommandRequest, dtCommandResponse, userInterfaceContext);
            break;
        }
    }

    if (i == numCommands)
    {
        LogError("Command <%s> not found in callback entries", dtCommandRequest->commandName);
        SetCommandResponseEmptyPayload(dtCommandResponse, 501);
    }
}

