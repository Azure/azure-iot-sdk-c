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
static const char DigitalTwinSampleModelDefinition_InterfaceId[] = "urn:azureiot:ModelDiscovery:ModelDefinition:1";
static const char DigitalTwinSampleModelDefinition_InterfaceInstanceName[] = "urn_azureiot_ModelDiscovery_ModelDefinition";

//
//  Callback function declarations and DigitalTwin command names for this interface.
//
static void DTMD_DataCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userContextCallback);

//
//  Callback command names for this interface.
//
static const char digitaltwinSample_GetModelDefinitionCommand[] = "getModelDefinition";

// Key value for Model Definition command
static const char digitaltwinSample_GetModelDefinitionCommand_Id[] = "id";

// Command Status for Model Definition command
static const int commandStatusSuccess = 200;
static const int commandStatusNotFound = 404;
static const int commandStatusFailure = 500;
static const int commandStatusNotPresent = 501;

static const char digitaltwinSample_ModelDefinition_NotImplemented[] = "\"Requested command not implemented on this interface\"";

static DIGITALTWIN_CLIENT_RESULT DTMD_AddInterfaceDtdl(const char *interfaceId, char *data, MODEL_DEFINITION_CLIENT_HANDLE mdHandle)
{
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR;
    
    if (NULL == interfaceId || NULL == data || NULL == mdHandle)
    {
        LogError("handle is NULL");
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else
    {
        if (Map_AddOrUpdate(mdHandle->map, interfaceId, data) != MAP_OK)
        {
            LogError("failed to Map_AddOrUpdate, result= %d", result);
            result = DIGITALTWIN_CLIENT_ERROR;
        }
        else
        {
            result = DIGITALTWIN_CLIENT_OK;
        }
    }

    return result;
}

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

        // Allocate a copy of the response data to return to the invoker.  The DigitalTwin layer that invoked DTMD_DataCallback
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

static const char *DTMD_Process_CmdRequest(const char *jsonPayload, MODEL_DEFINITION_CLIENT *mdHandle)
{
    const char *dtdl = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    const char* interfaceId = NULL;

    if ((NULL == jsonPayload) || (NULL == mdHandle))
    {
        LogError("Invalid input parameters");
    }
    else
    {
        // Parse request json payload
        if ((rootValue = json_parse_string(jsonPayload)) == NULL)
        {
            LogError("json_parse_string failed");
        }
        else if ((rootObject = json_value_get_object(rootValue)) == NULL)
        {
            LogError("json_value_get_object fails");
        }
        else if ((interfaceId = json_object_get_string(rootObject, digitaltwinSample_GetModelDefinitionCommand_Id)) == NULL)
        {
            LogError("json value <%s> is not available", "id");
        }
        else if (NULL == (dtdl = Map_GetValueFromKey(mdHandle->map, interfaceId)))
        {
            LogError("Could not retrive value for the given key\n");
        }
    }

    if (rootObject != NULL)
    {
        json_object_clear(rootObject);
    }

    if (rootValue != NULL)
    {
        json_value_free(rootValue);
    }

    return dtdl;
}

// Implement the callback to process the command "getModelDefinition".  Information pertaining to the request is specified in DIGITALTWIN_CLIENT_COMMAND_REQUEST,
// and the callback fills out data it wishes to return to the caller on the service in DIGITALTWIN_CLIENT_COMMAND_RESPONSE.
static void DTMD_DataCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userContextCallback)
{
    MODEL_DEFINITION_CLIENT *md_handle = (MODEL_DEFINITION_CLIENT *)userContextCallback;
    LogInfo("MODEL_DEFINITION_INTERFACE:  Request received for interface=<%s>", dtClientCommandContext->requestData);

    if (NULL != userContextCallback)
    {
        const char * DTMD_DataResponse = DTMD_Process_CmdRequest((const char *)dtClientCommandContext->requestData, md_handle);
        if (NULL == DTMD_DataResponse)
        {
            (void)DTMD_SetCommandResponse(dtClientCommandResponseContext, DTMD_DataResponse, commandStatusNotFound);
        }
        else
        {
            (void)DTMD_SetCommandResponse(dtClientCommandResponseContext, DTMD_DataResponse, commandStatusSuccess);
        }
    }
    else
    {
        LogError("MODEL_DEFINITION_INTERFACE: invalid arguments");
    }
}

// Encoding JSON file content before storing for all upcoming dtdl request handling.
static char * DTMD_ParseDTDL(char *data)
{
    char * parsedData = NULL;
    JSON_Value* v = NULL;
    JSON_Object* o = NULL;

    if (NULL != (v = json_value_init_object()))
    {
        if (NULL != (o = json_value_get_object(v)))
        {
            json_object_set_string(o, "Value", data);
            parsedData = json_serialize_to_string(v);
        }
        else
        {
            LogError("json_value_get_object failed");
        }
    }
    else
    {
        LogError("json_value_init_object failed");
    }

    if (o != NULL)
    {
        json_object_clear(o);
    }

    if (v != NULL)
    {
        json_value_free(v);
    }

    return parsedData;
}

// DTMD_ProcessCommandUpdate receives commands from the server.  This implementation acts as a simple dispatcher
// to the functions to perform the actual processing.
static void DTMD_ProcessCommandUpdate(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    if (strcmp(dtCommandRequest->commandName, digitaltwinSample_GetModelDefinitionCommand) == 0)
    {
        DTMD_DataCallback(dtCommandRequest, dtCommandResponse, userInterfaceContext);
    }
    else
    {
        // If the command is not implemented by this interface, by convention we return a 501 error to server.
        LogError("MODEL_DEFINITION_INTERFACE: Command name <%s> is not associated with this interface", dtCommandRequest->commandName);
        (void)DTMD_SetCommandResponse(dtCommandResponse, digitaltwinSample_ModelDefinition_NotImplemented, commandStatusNotPresent);
    }
}

DIGITALTWIN_CLIENT_RESULT DigitalTwin_ModelDefinition_Create(MODEL_DEFINITION_CLIENT_HANDLE *md_handle_result, DIGITALTWIN_INTERFACE_CLIENT_HANDLE *md_interface_client_handle)
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandle = NULL;
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR;
    MODEL_DEFINITION_CLIENT_HANDLE md_handle = NULL;

    md_handle = (MODEL_DEFINITION_CLIENT*)calloc(1, (sizeof(MODEL_DEFINITION_CLIENT)));

    if (NULL == md_handle)
    {
        LogError("MODEL_DEFINITION_INTERFACE: Unable to allocate memory for handle");
    }
    else if (NULL == (md_handle->map = Map_Create(NULL)))
    {
        LogError("MODEL_DEFINITION_INTERFACE: Unable to allocate memory for map");
        result = DIGITALTWIN_CLIENT_ERROR;
        DigitalTwin_ModelDefinition_Destroy(md_handle);
        md_handle = NULL;
    }
    else
    {
        if ((result = DigitalTwin_InterfaceClient_Create(DigitalTwinSampleModelDefinition_InterfaceId, DigitalTwinSampleModelDefinition_InterfaceInstanceName, DTMD_InterfaceRegisteredCallback, (void *)md_handle, &interfaceClientHandle)) != DIGITALTWIN_CLIENT_OK)
        {
            LogError("MODEL_DEFINITION_INTERFACE: Unable to allocate interface client handle for interfaceId=<%s>, InterfaceInstanceName=<%s>, error=<%s>", DigitalTwinSampleModelDefinition_InterfaceId, DigitalTwinSampleModelDefinition_InterfaceInstanceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
            interfaceClientHandle = NULL;
            DigitalTwin_ModelDefinition_Destroy(md_handle);
            md_handle = NULL;
        }
        else if ((result = DigitalTwin_InterfaceClient_SetCommandsCallback(interfaceClientHandle, DTMD_ProcessCommandUpdate)) != DIGITALTWIN_CLIENT_OK)
        {
            LogError("MODEL_DEFINITION_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallback failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
            interfaceClientHandle = NULL;
            DigitalTwin_ModelDefinition_Destroy(md_handle);
            md_handle = NULL;
        }
        else
        {
            LogInfo("MODEL_DEFINITION_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE. interfaceId=<%s>, InterfaceInstanceName=<%s>, handle=<%p>", DigitalTwinSampleModelDefinition_InterfaceId, DigitalTwinSampleModelDefinition_InterfaceInstanceName, interfaceClientHandle);
            md_handle->dt_handle = interfaceClientHandle;
        }
    }

    *md_handle_result = md_handle;
    *md_interface_client_handle = interfaceClientHandle;
    return result;
}

void DigitalTwin_ModelDefinition_Destroy(MODEL_DEFINITION_CLIENT_HANDLE handle)
{
    if (NULL != handle)
    {
        // On shutdown, in general the first call made should be to DigitalTwin_InterfaceClient_Destroy.
        // This will block if there are any active callbacks in this interface, and then
        // mark the underlying handle such that no future callbacks shall come to it.
        if (NULL != handle->dt_handle)
        {
            DigitalTwin_InterfaceClient_Destroy(handle->dt_handle);
        }

        // Destroy map
        if(NULL != handle->map)
        { 
            Map_Destroy(handle->map);
        }

        free(handle);
    }
}

DIGITALTWIN_CLIENT_RESULT DigitalTwin_ModelDefinition_Publish_Interface(const char *interfaceId, char *data, MODEL_DEFINITION_CLIENT *mdHandle)
{       
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    if (NULL != data)
    {
        char* modelDefEncoded = DTMD_ParseDTDL(data);

        if (NULL != modelDefEncoded)
        {
            result = DTMD_AddInterfaceDtdl(interfaceId, data, mdHandle);

            if (DIGITALTWIN_CLIENT_OK != result)
            {
                free(modelDefEncoded);
            }
        }
    }
    else
    {
        LogError("MODEL_DEFINITION_INTERFACE: NULL input data for Publish Interface\n");
    }
    return result;
}