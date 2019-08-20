// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

//
// Header files for interacting with DigitalTwin layer.
//
#include "parson.h"
#include "digitaltwin_device_client.h"
#include "digitaltwin_interface_client.h"
#include "digitaltwin_model_definition.h"

typedef struct MODEL_DEFINITION_CLIENT_TAG
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE dt_handle;
    MAP_HANDLE map;
}MODEL_DEFINITION_CLIENT;

// DigitalTwin interface name from service perspective.
#define DigitalTwinSampleModelDefinition_InterfaceId             "urn:azureiot:ModelDiscovery:ModelDefinition:1";
#define DigitalTwinSampleModelDefinition_InterfaceInstanceName   "urn_azureiot_ModelDiscovery_ModelDefinition";

//
//  Callback function declarations and DigitalTwin command names for this interface.
//
static void DTMD_GetDefinitionCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userContextCallback);

//
//  Callback command names for this interface.
//
#define digitaltwinSample_GetModelDefinitionCommand   "getModelDefinition";

// Command Status for Model Definition command
static const int commandStatusSuccess = 200;
static const int commandStatusNotFound = 404;
static const int commandStatusFailure = 500;
static const int commandStatusNotPresent = 501;

static const char digitaltwinSample_ModelDefinition_NotImplemented[] = "\"Requested command not implemented on this interface\"";

// DTMD_InterfaceRegisteredCallback is invoked when this interface
// is successfully or unsuccessfully registered with the service, and also when the interface is deleted.
static void DTMD_InterfaceRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userContextCallback)
{
    (void)userContextCallback;
    if (dtInterfaceStatus == DIGITALTWIN_CLIENT_OK)
    {
        LogInfo("MODEL_DEFINITION_INTERFACE: Interface successfully registered.");
    }
    else if (dtInterfaceStatus == DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING)
    {
        // Once an interface is marked as unregistered, it cannot be used for any DigitalTwin SDK calls.
        LogInfo("MODEL_DEFINITION_INTERFACE: Interface received unregistering callback.");
    }
    else
    {
        LogError("MODEL_DEFINITION_INTERFACE: Interface received failed, status=<%s>.", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus));
    }
}

// DTMD_SetCommandResponse is a helper that fills out a DIGITALTWIN_CLIENT_COMMAND_RESPONSE
static int DTMD_SetCommandResponse(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, const char* responseData, int status)
{
    int result;

    memset(dtClientCommandResponseContext, 0, sizeof(*dtClientCommandResponseContext));
    dtClientCommandResponseContext->version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;
    dtClientCommandResponseContext->status = status;

    if (responseData != NULL)
    {
        size_t responseLen = strlen(responseData);

        // Allocate a copy of the response data to return to the invoker.  The DigitalTwin layer that invoked DTMD_GetDefinitionCallback
        // takes responsibility for freeing this data.
        if ((dtClientCommandResponseContext->responseData = malloc(responseLen + 1)) == NULL)
        {
            LogError("MODEL_DEFINITION_INTERFACE: Unable to allocate response data");
            dtClientCommandResponseContext->status = commandStatusFailure;
            result = MU_FAILURE;
        }
        else
        {
            strcpy((char*)dtClientCommandResponseContext->responseData, responseData);
            dtClientCommandResponseContext->responseDataLen = responseLen;
            result = 0;
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

// DTMD_Lookup_Interface converts the jsonPayload into an interfaceId to be queried in the map list
static const char *DTMD_Lookup_Interface(const char *jsonPayload, MODEL_DEFINITION_CLIENT *mdHandle)
{
    const char *modelDefinition = NULL;
    JSON_Value* rootValue = NULL;
    const char* interfaceId = NULL;

    // Parse request json payload.  The interface is stored as a string, however as it is a JValue string
    // we still need to parse so we can turn the ""InterfaceId"" into the "InterfaceId".
    if ((rootValue = json_parse_string(jsonPayload)) == NULL)
    {
        LogError("json_parse_string failed");
    }
    else if ((interfaceId = json_value_get_string(rootValue)) == NULL)
    {
        LogError("json_value_get_string fails");
    }
    else if ((modelDefinition = Map_GetValueFromKey(mdHandle->map, interfaceId)) == NULL)
    {
        LogError("Could not retrive value for the given key");
    } 

    if (rootValue != NULL)
    {
        json_value_free(rootValue);
    }

    return modelDefinition;
}


// Implement the callback to process the command "getModelDefinition".  Information pertaining to the request is specified in DIGITALTWIN_CLIENT_COMMAND_REQUEST,
// and the callback fills out data it wishes to return to the caller on the service in DIGITALTWIN_CLIENT_COMMAND_RESPONSE.
static void DTMD_GetDefinitionCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userContextCallback)
{
    MODEL_DEFINITION_CLIENT *mdHandle = (MODEL_DEFINITION_CLIENT *)userContextCallback;
    LogInfo("MODEL_DEFINITION_INTERFACE:  Request received for interface=<%s>", dtClientCommandContext->requestData);

    const char * DTMD_DataResponse = DTMD_Lookup_Interface((const char *)dtClientCommandContext->requestData, mdHandle);
    if (DTMD_DataResponse == NULL)
    {
        LogError("MODEL_DEFINITION_INTERFACE:  Indicating failure of lookup of <%s>", dtClientCommandContext->requestData);
        (void)DTMD_SetCommandResponse(dtClientCommandResponseContext, DTMD_DataResponse, commandStatusNotFound);
    }
    else
    {
        LogInfo("MODEL_DEFINITION_INTERFACE:  Indicating successful lookup of <%s>", dtClientCommandContext->requestData);
        (void)DTMD_SetCommandResponse(dtClientCommandResponseContext, DTMD_DataResponse, commandStatusSuccess);
    }
}

// DTMD_ProcessCommandUpdate receives commands from the server.  This implementation acts as a simple dispatcher
// to the functions to perform the actual processing.
static void DTMD_ProcessCommandUpdate(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    const char* modeDefinitionCommandName = digitaltwinSample_GetModelDefinitionCommand;
    
    if (userInterfaceContext == NULL)
    {
        LogError("MODEL_DEFINITION_INTERFACE: invalid arguments");
    }
    else if (strcmp(dtCommandRequest->commandName, modeDefinitionCommandName) == 0)
    {
        DTMD_GetDefinitionCallback(dtCommandRequest, dtCommandResponse, userInterfaceContext);
    }
    else
    {
        // If the command is not implemented by this interface, by convention we return a 501 error to server.
        LogError("MODEL_DEFINITION_INTERFACE: Command name <%s> is not associated with this interface", dtCommandRequest->commandName);
        (void)DTMD_SetCommandResponse(dtCommandResponse, digitaltwinSample_ModelDefinition_NotImplemented, commandStatusNotPresent);
    }
}

// DigitalTwin_ModelDefinition_Create creates the md_interface_client_handle for later applications to use the Model Definition logic.
DIGITALTWIN_CLIENT_RESULT DigitalTwin_ModelDefinition_Create(MODEL_DEFINITION_CLIENT_HANDLE *mdHandle_result, DIGITALTWIN_INTERFACE_CLIENT_HANDLE *md_interface_client_handle)
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandle = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    MODEL_DEFINITION_CLIENT_HANDLE mdHandle = NULL;

    const char* interfaceId = DigitalTwinSampleModelDefinition_InterfaceId;
    const char* interfaceName = DigitalTwinSampleModelDefinition_InterfaceInstanceName;

    if ((mdHandle = (MODEL_DEFINITION_CLIENT*)calloc(1, (sizeof(MODEL_DEFINITION_CLIENT)))) == NULL)
    {
        LogError("MODEL_DEFINITION_INTERFACE: Unable to allocate memory for handle");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((mdHandle->map = Map_Create(NULL)) == NULL)
    {
        LogError("MODEL_DEFINITION_INTERFACE: Unable to allocate memory for map");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((result = DigitalTwin_InterfaceClient_Create(interfaceId, interfaceName, DTMD_InterfaceRegisteredCallback, (void *)mdHandle, &interfaceClientHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("MODEL_DEFINITION_INTERFACE: Unable to allocate interface client handle for interfaceId=<%s>, InterfaceInstanceName=<%s>, error=<%s>", interfaceId, interfaceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceClientHandle = NULL;
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((result = DigitalTwin_InterfaceClient_SetCommandsCallback(interfaceClientHandle, DTMD_ProcessCommandUpdate)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("MODEL_DEFINITION_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallback failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceClientHandle = NULL;
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else
    {
        LogInfo("MODEL_DEFINITION_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE. interfaceId=<%s>, InterfaceInstanceName=<%s>, handle=<%p>", interfaceId, interfaceName, interfaceClientHandle);
        mdHandle->dt_handle = interfaceClientHandle;
        result = DIGITALTWIN_CLIENT_OK;
    }

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        DigitalTwin_ModelDefinition_Destroy(mdHandle);
        mdHandle = NULL;
    }

    *mdHandle_result = mdHandle;
    *md_interface_client_handle = interfaceClientHandle;
    return result;
}

// DigitalTwin_ModelDefinition_Destroy destroyes the
void DigitalTwin_ModelDefinition_Destroy(MODEL_DEFINITION_CLIENT_HANDLE mdHandle)
{
    if (mdHandle != NULL)
    {
        // On shutdown, in general the first call made should be to DigitalTwin_InterfaceClient_Destroy.
        // This will block if there are any active callbacks in this interface, and then
        // mark the underlying handle such that no future callbacks shall come to it.
        if (NULL != mdHandle->dt_handle)
        {
            DigitalTwin_InterfaceClient_Destroy(mdHandle->dt_handle);
        }

        // Destroy map
        if (NULL != mdHandle->map)
        { 
            Map_Destroy(mdHandle->map);
        }

        free(mdHandle);
    }
}

// DigitalTwin_ModelDefinition_Publish_Interface adds a new interface to the internal map.
DIGITALTWIN_CLIENT_RESULT DigitalTwin_ModelDefinition_Publish_Interface(const char *interfaceId, char *data, MODEL_DEFINITION_CLIENT *mdHandle)
{       
    DIGITALTWIN_CLIENT_RESULT result;
    MAP_RESULT mapResult;

    if ((interfaceId == NULL) || (data == NULL) || (mdHandle == NULL))
    {
        LogError("MODEL_DEFINITION_INTERFACE: invalid parameter(s)");
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if ((mapResult = Map_AddOrUpdate(mdHandle->map, interfaceId, data)) != MAP_OK)
    {
        LogError("Map_AddOrUpdate failed, err=%d", mapResult);
        // Because the map does not own the memory, free it here
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }
    
    return result;
}

