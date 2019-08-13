// Copyright (c) Microsoft. All rights reserved. 
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"

#include "internal/dt_interface_list.h"
#include "parson.h"

#define DT_JSON_MODEL_INFORMATION "modelInformation."

static const char* DT_JSON_INTERFACE_IDS_OBJECT_NAME = DT_JSON_MODEL_INFORMATION "interfaces";
static const char* DT_JSON_CAPABILITY_MODEL_ID = DT_JSON_MODEL_INFORMATION "capabilityModelId";

static const char* DT_MODEL_DISCOVERY_INTERFACE_ID = "urn:azureiot:ModelDiscovery:ModelInformation:1";
static const char* DT_MODEL_DISCOVERY_INTERFACE_NAME = "urn_azureiot_ModelDiscovery_ModelInformation";
static const char* DT_CAPABILITY_REPORT_INTERFACE_TELEMETRY_TYPE = "modelInformation";

static const char* DT_SDK_INFORMATION_INTERFACE_ID = "urn:azureiot:Client:SDKInformation:1";

// DT_INTERFACE_LIST represents the list of currently registered interfaces.  It also
// tracks the interfaces as registered by the server.
typedef struct DT_INTERFACE_LIST_TAG
{
    // Interfaces registered from client
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE* dtInterfaceClientHandles;
    unsigned int numDtInterfaceClientHandles;
} DT_INTERFACE_LIST;

// Creates a DIGITALTWIN_INTERFACE_LIST_HANDLE.
DIGITALTWIN_INTERFACE_LIST_HANDLE DigitalTwin_InterfaceList_Create(void)
{
    DT_INTERFACE_LIST* dtInterfaceList;

    if ((dtInterfaceList = calloc(1, sizeof(DT_INTERFACE_LIST))) == NULL)
    {
        LogError("Cannot allocate interface list");
    }

    return (DIGITALTWIN_INTERFACE_LIST_HANDLE)dtInterfaceList;
}

// Destroys a DIGITALTWIN_INTERFACE_LIST_HANDLE .
void DT_InterfaceList_Destroy(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle)
{
    if (dtInterfaceListHandle != NULL)
    {
        DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;
        free(dtInterfaceList->dtInterfaceClientHandles);
        free(dtInterfaceList);
    }
}

// Marks all interface handles as being unbound.  This call can be invoked during a client core deletion or during an error rollback.
static void UnbindExistingInterfaceHandles(DT_INTERFACE_LIST* dtInterfaceList)
{
    for (unsigned int i = 0; i < dtInterfaceList->numDtInterfaceClientHandles; i++)
    {
        (void)DT_InterfaceClient_UnbindFromClientHandle(dtInterfaceList->dtInterfaceClientHandles[i]);
    }
    
    dtInterfaceList->numDtInterfaceClientHandles = 0;
    free(dtInterfaceList->dtInterfaceClientHandles);
    dtInterfaceList->dtInterfaceClientHandles = NULL;
}

// DT_InterfaceList_BindInterfaces clears out any existing interfaces already registered, indicates to underlying
// DIGITALTWIN_INTERFACE_CLIENT_HANDLE's that they're being registered, and stores list.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceList_BindInterfaces(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE* dtInterfaces, unsigned int numDTInterfaces, DT_CLIENT_CORE_HANDLE dtClientCoreHandle, DT_LOCK_THREAD_BINDING* lockThreadBinding)
{
    DIGITALTWIN_CLIENT_RESULT result;

    if (dtInterfaceListHandle == NULL)
    {
        LogError("Invalid parameter: dtInterfaceListHandle=%p", dtInterfaceListHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else
    {
        DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;
        unsigned int i;
        
        if ((numDTInterfaces > 0) &&  (dtInterfaceList->dtInterfaceClientHandles = (DIGITALTWIN_INTERFACE_CLIENT_HANDLE*)calloc(numDTInterfaces, sizeof(DIGITALTWIN_INTERFACE_CLIENT_HANDLE))) == NULL)
        {
            LogError("Cannot allocate interfaces");
            result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            result = DIGITALTWIN_CLIENT_OK;

            for (i = 0; i < numDTInterfaces; i++)
            {
                if ((result = DT_InterfaceClient_BindToClientHandle(dtInterfaces[i], dtClientCoreHandle, lockThreadBinding)) != DIGITALTWIN_CLIENT_OK)
                {
                    LogError("Cannot register DeviceTwin interface %d in list", i);
                    break;
                }
                else
                {
                    dtInterfaceList->dtInterfaceClientHandles[i] = dtInterfaces[i];
                    dtInterfaceList->numDtInterfaceClientHandles++;
                }
            }

            if (i == numDTInterfaces)
            {
                result = DIGITALTWIN_CLIENT_OK;
            }
            else
            {
                // Otherwise we're in an error state and we should unregister any interfaces that may have been set to reset them.
                UnbindExistingInterfaceHandles(dtInterfaceList);
            }
        }
    }

    return result;
}


// DT_InterfaceList_UnbindInterfaces is used to tell registered interfaces that clientCore no longer
// needs reference to them.  For DT_ClientCoreDestroy, this is straightforward.
// If DT_ClientCoreRegisterInterfacesAsync() is called multiple times, we first need to mark the interfaces
// as not in a registered state.  The same DIGITALTWIN_INTERFACE_CLIENT_HANDLE may be safely passed into multiple
// calls to DT_ClientCoreRegisterInterfacesAsync; in that case this will just momentarily UnRegister the
// interface until the next stage re-registers it.  If the interface client isn't being re-registered, however,
// this step is required to effectively DeleteReference on the handle so it can be destroyed.
void DT_InterfaceList_UnbindInterfaces(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle)
{
    DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;

    if (NULL == dtInterfaceList)
    {
        LogError("Invalid parameter.  dtInterfaceList=%p", dtInterfaceList);
    }
    else
    {
        UnbindExistingInterfaceHandles(dtInterfaceList);
    }
}

void DT_InterfaceList_RegistrationCompleteCallback(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle, DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus)
{
    DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;

    if (dtInterfaceListHandle == NULL)
    {
        LogError("Invalid parameter: dtInterfaceListHandle=%p", dtInterfaceListHandle);
    }
    else
    {
        unsigned int i;

        for (i = 0; i < dtInterfaceList->numDtInterfaceClientHandles; i++)
        {
            DT_InterfaceClient_RegistrationCompleteCallback(dtInterfaceList->dtInterfaceClientHandles[i], dtInterfaceStatus);
        }
    }
}

// Validates that the dtInterfaceClientHandle is still in list of registered interface handles.  It's possible,
// for example, that (A) a request to send telemetry and it was posted, (B) the caller re-ran DT_ClientCoreRegisterInterfacesAsync() without the 
// given interface, and (C) the response callback for given interface arrives on core layer.  In this case we need to swallow the message.
static bool IsInterfaceHandleValid(DT_INTERFACE_LIST* dtInterfaceList, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle)
{
    bool result = false;

    for (unsigned int i = 0; i < dtInterfaceList->numDtInterfaceClientHandles; i++)
    {
        if (dtInterfaceList->dtInterfaceClientHandles[i] == dtInterfaceClientHandle)
        {
            result = true;
            break;
        }
    }

    return result;
}

DT_COMMAND_PROCESSOR_RESULT DT_InterfaceList_InvokeCommand(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle, const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, int* resultFromCommandCallback)
{
    DT_COMMAND_PROCESSOR_RESULT commandProcessorResult;

    if (dtInterfaceListHandle == NULL)
    {
        commandProcessorResult = DT_COMMAND_PROCESSOR_ERROR;
    }
    else
    {
        DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;
        commandProcessorResult = DT_COMMAND_PROCESSOR_NOT_APPLICABLE;

        unsigned int i;
        for (i = 0; i < dtInterfaceList->numDtInterfaceClientHandles; i++)
        {
            // We visit each registered interface to see if it can process this command.  If it's not for this interface, it just
            // returns DT_COMMAND_PROCESSOR_NOT_APPLICABLE and we continue search.
            commandProcessorResult = DT_InterfaceClient_InvokeCommandIfSupported(dtInterfaceList->dtInterfaceClientHandles[i], method_name, payload, size, response, response_size, resultFromCommandCallback);
            if (commandProcessorResult != DT_COMMAND_PROCESSOR_NOT_APPLICABLE)
            {
                // The visited interface processed (for failure or success), stop searching.
                break;
            }
        }
    }

    return commandProcessorResult;
}

// Processes twin, looking explicitly property handling.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceList_ProcessTwinCallbackForProperties(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle, bool fullTwin, const unsigned char* payLoad, size_t size)
{
    DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    if (dtInterfaceListHandle == NULL)
    {
        LogError("Invalid parameter.  dtInterfaceListHandle=%p", dtInterfaceListHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else
    {
        // Invoke interface callbacks for any new/updated fields.
        for (unsigned int i = 0; i < dtInterfaceList->numDtInterfaceClientHandles; i++)
        {
            DT_InterfaceClient_ProcessTwinCallback(dtInterfaceList->dtInterfaceClientHandles[i], fullTwin, payLoad, size);
        }

        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// Invoked when DigitalTwin Core layer processes has a property update callback.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContextCallback)
{
    DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    if (dtInterfaceListHandle == NULL)
    {
        LogError("Invalid parameter.  dtInterfaceListHandle=%p", dtInterfaceListHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (IsInterfaceHandleValid(dtInterfaceList, dtInterfaceClientHandle) == false)
    {
        // Even though the interface is always valid at time of the reported property dispatch, this can happen if
        // the caller destroyed the interface before the callback completed.
        LogError("Interface for handle %p is no longer valid", dtInterfaceClientHandle);
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else
    {
        (void)DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(dtInterfaceClientHandle, dtReportedStatus, userContextCallback);
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// Invoked when a SendTelemetry callback has arrived.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceList_ProcessTelemetryCallback(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT dtSendTelemetryStatus, void* userContextCallback)
{
    DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    if (dtInterfaceListHandle == NULL)
    {
        LogError("Invalid parameter.  dtInterfaceListHandle=%p", dtInterfaceListHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (IsInterfaceHandleValid(dtInterfaceList, dtInterfaceClientHandle) == false)
    {
        // See comments in DT_InterfaceList_ProcessReportedPropertiesUpdateCallback why this can fail
        LogError("Interface handle %p is no longer valid; swallowing callback for telemetry", dtInterfaceClientHandle);
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else
    {
        (void)DT_InterfaceClient_ProcessTelemetryCallback(dtInterfaceClientHandle, dtSendTelemetryStatus, userContextCallback);
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// CreateDTInterfacesJson will allocate a json object and populate it with the names of DigitalTwin interface Ids and models to register.
static DIGITALTWIN_CLIENT_RESULT CreateDTInterfacesJson(DT_INTERFACE_LIST* dtInterfaceList, JSON_Value** interfacesValue)
{
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR;
    JSON_Object* interfacesObject = NULL;

    if ((*interfacesValue = json_value_init_object()) == NULL)
    {
        LogError("json_value_init_object failed");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((interfacesObject = json_value_get_object(*interfacesValue)) == NULL)
    {
        LogError("json_value_get_object failed");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    // The modelInformation interface is implemented by the DigitalTwin SDK itself
    // (it must be since it's what's used for interface registration).  Add it here.
    else if (json_object_set_string(interfacesObject, DT_MODEL_DISCOVERY_INTERFACE_NAME, DT_MODEL_DISCOVERY_INTERFACE_ID) != JSONSuccess)
    {
        LogError("json_object_set_string failed");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    // The SDKInformation interface is also implemented directly by the DigitalTwin SDK.
    else if (json_object_set_string(interfacesObject, DT_SDK_INFORMATION_INTERFACE_NAME, DT_SDK_INFORMATION_INTERFACE_ID) != JSONSuccess)
    {
        LogError("json_object_set_string failed");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        unsigned int i;
        for (i = 0; i < dtInterfaceList->numDtInterfaceClientHandles; i++)
        {
            const char* interfaceId;
            const char* interfaceInstanceName;

            if ((interfaceId = DT_InterfaceClient_GetInterfaceId(dtInterfaceList->dtInterfaceClientHandles[i])) == NULL)
            {
                LogError("DT_InterfaceClient_GetInterfaceId failed");
                result = DIGITALTWIN_CLIENT_ERROR;
                break;
            }
            else if ((interfaceInstanceName = DT_InterfaceClient_GetInterfaceInstanceName(dtInterfaceList->dtInterfaceClientHandles[i])) == NULL)
            {
                LogError("DT_InterfaceClient_GetInterfaceInstanceName failed");
                result = DIGITALTWIN_CLIENT_ERROR;
                break;
            }
            else if (json_object_set_string(interfacesObject, interfaceInstanceName, interfaceId) != JSONSuccess)
            {
                LogError("json_object_set_string failed");
                result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
                break;
            }
        }

        if (i == dtInterfaceList->numDtInterfaceClientHandles)
        {
            result = 0;
        }
    }

    if (result != 0)
    {
        if (interfacesObject != NULL)
        {
            json_object_clear(interfacesObject);
        }

        if (*interfacesValue != NULL)
        {
            json_value_free(*interfacesValue);
            *interfacesValue = NULL;
        }
    }

    return result;
}

// CreateMessageBodyForRegistration will allocate the blob of JSon to be put into the telemetry message's body
static DIGITALTWIN_CLIENT_RESULT CreateMessageBodyForRegistration(DT_INTERFACE_LIST* dtInterfaceList, const char* deviceCapabilityModel, char** messageBody)
{
    DIGITALTWIN_CLIENT_RESULT result;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Value* interfacesValue = NULL;

    if ((rootValue = json_value_init_object()) == NULL)
    {
        LogError("json_value_init_object failed");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((rootObject = json_value_get_object(rootValue)) == NULL)
    {
        LogError("json_value_get_object failed");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else if (json_object_dotset_string(rootObject, DT_JSON_CAPABILITY_MODEL_ID, deviceCapabilityModel) != JSONSuccess)
    {
        LogError("json_object_set_string fails setting %s", DT_JSON_CAPABILITY_MODEL_ID);
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((result = CreateDTInterfacesJson(dtInterfaceList, &interfacesValue)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("CreateDTInterfacesJson fails, error = %d", result);
    }
    else if (json_object_dotset_value(rootObject, DT_JSON_INTERFACE_IDS_OBJECT_NAME, interfacesValue) != JSONSuccess)
    {
        LogError("json_object_set_value failed");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((*messageBody = json_serialize_to_string(rootValue)) == NULL)
    {
        LogError("json_serialize_to_string failed");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    json_value_free(rootValue);
    return result;
}


// DT_InterfaceList_CreateRegistrationMessage creates a telemetry message (for the DigitalTwin V1 binding) that, when sent to the service,
// lets the service know which interfaces have been registered.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceList_CreateRegistrationMessage(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle, const char* deviceCapabilityModel, IOTHUB_MESSAGE_HANDLE* messageHandle)
{
    DT_INTERFACE_LIST* dtInterfaceList = (DT_INTERFACE_LIST*)dtInterfaceListHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    char* messageBody = NULL;

    if ((dtInterfaceListHandle == NULL) || (deviceCapabilityModel == NULL) || (messageHandle == NULL))
    {
        LogError("Invalid arg(s).  dtInterfaceListHandle=%p, deviceCapabilityModel=%p, messageHandle=%p", dtInterfaceListHandle, deviceCapabilityModel, messageHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if ((result = CreateMessageBodyForRegistration(dtInterfaceList, deviceCapabilityModel, &messageBody)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("CreateMessageBodyForRegistration failed, error = %d", result);
    }
    else if ((result = DT_InterfaceClient_CreateTelemetryMessage(DT_MODEL_DISCOVERY_INTERFACE_ID, DT_MODEL_DISCOVERY_INTERFACE_NAME, 
                                                                 DT_CAPABILITY_REPORT_INTERFACE_TELEMETRY_TYPE,
                                                                 messageBody, messageHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DT_InterfaceClient_CreateTelemetryMessage failed, error = %d", result);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else
    {   
        result = DIGITALTWIN_CLIENT_OK;
    }

    json_free_serialized_string(messageBody);
    return result;
}


