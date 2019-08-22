// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
// The DigitalTwin E2E Tests have common helper functions implemented here.
//

#ifndef DT_E2E_UTIL_H
#define DT_E2E_UTIL_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stdbool.h>
#endif

#include <digitaltwin_interface_client.h>


void DT_E2E_Util_Init();
void DT_E2E_Util_Gracefully_End_Test_Execution();
void DT_E2E_Util_Fatal_Error_Occurred();
bool DT_E2E_Util_Should_Continue_Execution();
bool DT_E2E_Util_Has_Fatal_Error_Occurred();
void SetCommandResponse(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, const char* responseData, int responseStatus);
void SetCommandResponseEmptyPayload(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, int responseStatus);

void DT_E2E_Util_InvokeCommand(const char** commandNames, const DIGITALTWIN_COMMAND_EXECUTE_CALLBACK* commandCallbacks, const int numCommands,
                               const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, 
                               DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, 
                               void* userInterfaceContext);



#endif /* DT_E2E_UTIL_H */

