// Copyright (c) Microsoft. All rights reserved. 
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/uuid.h"

#include "dt_client_core.h"
#include "internal/dt_interface_private.h"
#include "digitaltwin_client_version.h"


#include "parson.h"

#define DT_INTERFACE_PREFIX "$iotin:"

static const char DT_InterfaceIdPrefix[] = "urn:";
static const size_t DT_InterfaceIdPrefixLen = sizeof(DT_InterfaceIdPrefix) - 1;

static const char DT_InterfaceIdUnderscore = '_';
static const char DT_InterfaceIdSeparator = ':';
static const size_t DT_InterfaceMaxLength = 256;

static const char commandSeparator = '*';
static const char* DT_PROPERTY_UPDATE_JSON_VERSON = "$version";
// Using MQTT property directly because we don't support AMQP presently and cannot support HTTP.
static const char DT_INTERFACE_ID_PROPERTY[] = "$.ifid";
static const char DT_INTERFACE_NAME_PROPERTY[] = "$.ifname";
static const char DT_MESSAGE_SCHEMA_PROPERTY[] = "$.schema";
static const char DT_JSON_MESSAGE_CONTENT_TYPE[] = "application/json";
static const char DT_REQUEST_ID_PROPERTY[] = "iothub-command-request-id";
static const char DT_STATUS_CODE_PROPERTY[] = "iothub-command-statuscode";
static const char DT_COMMAND_NAME_PROPERTY[] = "iothub-command-name";
static const size_t DT_INTERFACE_PREFIX_LENGTH = sizeof(DT_INTERFACE_PREFIX) - 1;
static const char DT_JSON_DESIRED[] = "desired";
static const char DT_JSON_REPORTED[] = "reported";
static const char DT_JSON_COMMAND_REQUEST_ID[] = "commandRequest.requestId";
static const char DT_JSON_COMMAND_REQUEST_PAYLOAD[] = "commandRequest.value";
static const char DT_JSON_NULL[] = "null";

static const char DT_PropertyWithResponseSchema[] =  "{\""  DT_INTERFACE_PREFIX "%s\": { \"%s\": { \"value\":  %s, \"sc\": %d, \"sd\": \"%s\", \"sv\": %d } } }";
static const char DT_PropertyWithoutResponseSchema[] = "{\""  DT_INTERFACE_PREFIX "%s\": { \"%s\": { \"value\": %s } } }";
static const char DT_AsyncResultSchema[] = "asyncResult";

#define DT_SDK_INFORMATION_LANGUAGE_PROPERTY "\"language\":"
#define DT_SDK_INFORMATION_VERSION_PROPERTY "\"version\":"
#define DT_SDK_INFORMATION_VENDOR_PROPERTY "\"vendor\":"
#define DT_JSON_VALUE_STRING_BLOCK "{ \"value\":\"%s\"}"

static const char DT_SdkLanguage[] = "C";
static const char DT_SdkVendor[] = "Microsoft";
  
static const char DT_SdkInfoSchema[] = "{\"" DT_INTERFACE_PREFIX DT_SDK_INFORMATION_INTERFACE_NAME "\": "
                                            " { " DT_SDK_INFORMATION_LANGUAGE_PROPERTY DT_JSON_VALUE_STRING_BLOCK ","
                                                  DT_SDK_INFORMATION_VERSION_PROPERTY DT_JSON_VALUE_STRING_BLOCK ","
                                                  DT_SDK_INFORMATION_VENDOR_PROPERTY DT_JSON_VALUE_STRING_BLOCK "}"
                                       "}";

#define DT_MAX_STATUS_CODE_STRINGLEN    16

#define DT_INTERFACE_STATE_VALUES                   \
    DT_INTERFACE_STATE_UNINITIALIZED,               \
    DT_INTERFACE_STATE_CREATED,                     \
    DT_INTERFACE_STATE_BOUND_TO_CLIENT_HANDLE,      \
    DT_INTERFACE_STATE_REGISTERED,                  \
    DT_INTERFACE_STATE_UNBOUND_FROM_CLIENT_HANDLE,  \
    DT_INTERFACE_STATE_PENDING_DESTROY              \

MU_DEFINE_ENUM(DT_INTERFACE_STATE, DT_INTERFACE_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS(DT_INTERFACE_STATE, DT_INTERFACE_STATE_VALUES);

// DT_INTERFACE_CLIENT corresponds to an application level handle (e.g. DIGITALTWIN_INTERFACE_CLIENT_HANDLE).
typedef struct DT_INTERFACE_CLIENT_TAG
{
    DT_INTERFACE_STATE interfaceState;
    DT_LOCK_THREAD_BINDING lockThreadBinding;
    // The remaining fields are read only after DT_InterfaceClientCore_Create and the callback registration.
    // Because they can't be modified(other than at create/delete time), they don't require lock when reading them.
    DT_CLIENT_CORE_HANDLE dtClientCoreHandle;
    void* userInterfaceContext;
    char* interfaceId;
    char* interfaceInstanceName;
    size_t interfaceInstanceNameLen;
    DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK dtInterfaceRegisteredCallback;
    DIGITALTWIN_PROPERTY_UPDATE_CALLBACK propertyUpdatedCallback;
    DIGITALTWIN_COMMAND_EXECUTE_CALLBACK commandExecuteCallback;
} DT_INTERFACE_CLIENT;

typedef struct DT_REPORTED_PROPERTIES_UPDATE_CALLBACK_CONTEXT_TAG
{
    DIGITALTWIN_REPORTED_PROPERTY_UPDATED_CALLBACK dtReportedPropertyCallback;
    void* userContextCallback;
} DT_REPORT_PROPERTIES_UPDATE_CALLBACK_CONTEXT;

typedef struct DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT_TAG
{
    DIGITALTWIN_CLIENT_TELEMETRY_CONFIRMATION_CALLBACK telemetryConfirmationCallback;
    void* userContextCallback;
} DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT;

MU_DEFINE_ENUM_STRINGS(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_RESULT_VALUES);

// Invokes Lock() (for convenience layer based handles) or else a no-op (for _LL_)
static int InvokeBindingInterfaceLock(DT_INTERFACE_CLIENT* dtInterfaceClient, bool* lockHeld)
{
    LOCK_RESULT lockResult;
    int result;

    if (dtInterfaceClient->lockThreadBinding.dtBindingLockHandle == NULL)
    {
        LogError("Client is not yet bound to a lock binding.");
        result = MU_FAILURE;
    }
    else if ((lockResult = dtInterfaceClient->lockThreadBinding.dtBindingLock(dtInterfaceClient->lockThreadBinding.dtBindingLockHandle)) != LOCK_OK)
    {
        LogError("Unable to grab lock.  Error=%d", lockResult);
        result = MU_FAILURE;
    }
    else
    {
        *lockHeld = true;
        result = 0;
    }

    return result;
}

// InvokeBindingInterfaceLockIfAvailable will attempt to Lock() on our interface lock, if it exists.  There are scenarios
// on destruction however where this handle may be NULL (namely if the interface handle was created but never bound to a client).
// This is not really the way we should do it, but fixing it correctly will be considerably more work including API design.
// https://github.com/Azure/azure-iot-sdk-c-pnp/issues/43 tracks the correct fix.
static int InvokeBindingInterfaceLockIfAvailable(DT_INTERFACE_CLIENT* dtInterfaceClient, bool* lockHeld)
{
    int result;

    if (dtInterfaceClient->lockThreadBinding.dtBindingLockHandle == NULL)
    {
        result = 0;
    }
    else
    {
        result = InvokeBindingInterfaceLock(dtInterfaceClient, lockHeld);
    }
    return result;
}


// Invokes Unlock() (for convenience layer based handles) or else a no-op (for _LL_)
static int InvokeBindingInterfaceUnlock(DT_INTERFACE_CLIENT* dtInterfaceClient, bool* lockHeld)
{
    LOCK_RESULT lockResult;
    int result;
    
    if (*lockHeld == true)
    {
        lockResult = dtInterfaceClient->lockThreadBinding.dtBindingUnlock(dtInterfaceClient->lockThreadBinding.dtBindingLockHandle);
        if (lockResult != LOCK_OK)
        {
            LogError("Unlock failed, result = %d", lockResult);
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
        // Unconditionally mark lock held as false, since even on failure no point re-invoking dtBindingUnlock
        *lockHeld = false;
    }
    else
    {
        result = 0;
    }

    return result;
}

// Invokes Lock_Deinit() (for convenience layer based handles) or else a no-op (for _LL_)
static void InvokeBindingInterfaceLockDeinitIfNeeded(DT_INTERFACE_CLIENT* dtInterfaceClient)
{
    if (dtInterfaceClient->lockThreadBinding.dtBindingLockHandle != NULL)
    {
        dtInterfaceClient->lockThreadBinding.dtBindingLockDeinit(dtInterfaceClient->lockThreadBinding.dtBindingLockHandle);
    }
}

// Destroys DT_INTERFACE_CLIENT memory.
static void FreeDTInterface(DT_INTERFACE_CLIENT* dtInterfaceClient)
{
    InvokeBindingInterfaceLockDeinitIfNeeded(dtInterfaceClient);
    free(dtInterfaceClient->interfaceId);
    free(dtInterfaceClient->interfaceInstanceName);
    free(dtInterfaceClient);
}


// Implement a local isalpha because some compilers for embedded do not have this
static bool DT_IsAlpha(char c)
{
    return ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z'));
}

// DT_InterfaceClient_CheckNameValid makes sure that the passed names (specifically on registration) is legal.
// In general the SDK does NOT do policy checking like this, leaving it to the server.  We make an exception
// in this case because an ill-formatted values on registration over MQTT will result in the
// TCP connection being closed.  Because this can be so hard for developer to diagnose interface being
// invalid versus network issues, we provide this check.
int DT_InterfaceClient_CheckNameValid(const char* valueToCheck, bool isInterfaceId)
{
    int result = 0;
    const char* current = valueToCheck;

    if (valueToCheck == NULL)
    {
        LogError("Invalid parameter.  valueToCheck=%p", valueToCheck);
        result = MU_FAILURE;
    }
    else if (strlen(valueToCheck) > DT_InterfaceMaxLength)
    {
        LogError("Name %s is too long, maximum length is %lu", valueToCheck, (unsigned long)DT_InterfaceMaxLength);
        result = MU_FAILURE;
    }
    else if (isInterfaceId)
    {
        if (strncmp(DT_InterfaceIdPrefix, current, DT_InterfaceIdPrefixLen) != 0)
        {
            LogError("ID name must be prefixed with %s.  Actual value=%s", DT_InterfaceIdPrefix, valueToCheck);
            result = MU_FAILURE;
        }
        else
        {
            current += DT_InterfaceIdPrefixLen;
        }
    }

    if (result == 0)
    {
        while (*current != 0)
        {
            if (!ISDIGIT(*current) && !DT_IsAlpha(*current) && (*current != DT_InterfaceIdUnderscore))
            {
                if (isInterfaceId && (*current == DT_InterfaceIdSeparator))
                {
                    // DT_InterfaceIdSeparator is allowed only for id's.
                    ;
                }
                else
                {
                    LogError("Character (char=%c,decimal=%d) in %s is illegal", *current, (int)*current, valueToCheck);
                    result = MU_FAILURE;
                    break;
                }
            }
            current++;
        }
    }

    return result;
}


// Retrieves a shallow copy of the interface ID for caller.
const char* DT_InterfaceClient_GetInterfaceId(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle)
{
    const char* result;
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;

    if (dtInterfaceClientHandle == NULL)
    {
        LogError("Invalid interfaceClient handle passed");
        result = NULL;
    }
    else
    {
        result = dtInterfaceClient->interfaceId;
    }
    return result;
}

// Retrieves a shallow copy of the interface name for caller.
const char* DT_InterfaceClient_GetInterfaceInstanceName(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle)
{
    const char* result;
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;

    if (dtInterfaceClientHandle == NULL)
    {
        LogError("Invalid interfaceClient handle passed");
        result = NULL;
    }
    else
    {
        result = dtInterfaceClient->interfaceInstanceName;
    }
    return result;
}

// Determines whether a desired state machine transition is legal based on current state of dtInterfaceClient
static bool IsDesiredInterfaceStateTransitionAllowed(DT_INTERFACE_CLIENT* dtInterfaceClient, DT_INTERFACE_STATE desiredState)
{
    bool result;

    const DT_INTERFACE_STATE currentState = dtInterfaceClient->interfaceState;

    switch (desiredState)
    {
        case DT_INTERFACE_STATE_BOUND_TO_CLIENT_HANDLE:
            // To transition to a bound state, only valid state is directly from creation.  In future we may allow a re-binding
            // (e.g. currentState == DT_INTERFACE_STATE_UNBOUND_FROM_CLIENT_HANDLE would be allowed here) but there is not a 
            // thread safe way to do this with current design.
            result = (currentState == DT_INTERFACE_STATE_CREATED);
            break;

        case DT_INTERFACE_STATE_REGISTERED:
            result = (currentState == DT_INTERFACE_STATE_BOUND_TO_CLIENT_HANDLE);
            break;

        case DT_INTERFACE_STATE_UNBOUND_FROM_CLIENT_HANDLE:
            // To transition to unbound state, we need to have been bound at minimum and potentially registered/failed registered
            result = ((currentState == DT_INTERFACE_STATE_BOUND_TO_CLIENT_HANDLE) ||
                      (currentState == DT_INTERFACE_STATE_REGISTERED));
            break;

        case DT_INTERFACE_STATE_PENDING_DESTROY:
            // We can always transition to pending destroy state.  Caller may or may not be able to immediately cleanup, depending on
            // whether or not there is a clientCore that has a reference to this interface still.
            result = true;
            break;

        default:
            LogError("Unknown desired state %d specified", desiredState);
            result = false;
            break;
    }

    if (result != true)
    {
        LogError("Cannot transition from state %d to %d", dtInterfaceClient->interfaceState, desiredState);
    }

    return result;
}

static void SetInterfaceState(DT_INTERFACE_CLIENT* dtInterfaceClient, DT_INTERFACE_STATE desiredState)
{
    DigitalTwinLogInfo("DigitalTwin Interface : Changing interface state on interface %s from %s to %s", dtInterfaceClient->interfaceInstanceName,  
                                                                                             MU_ENUM_TO_STRING(DT_INTERFACE_STATE, dtInterfaceClient->interfaceState), 
                                                                                             MU_ENUM_TO_STRING(DT_INTERFACE_STATE, desiredState));
    dtInterfaceClient->interfaceState = desiredState;
}

// DigitalTwin_InterfaceClient_Create initializes a DIGITALTWIN_INTERFACE_CLIENT_HANDLE
DIGITALTWIN_CLIENT_RESULT DigitalTwin_InterfaceClient_Create(const char* interfaceId, const char* interfaceInstanceName, DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK dtInterfaceRegisteredCallback, void* userInterfaceContext, DIGITALTWIN_INTERFACE_CLIENT_HANDLE* dtInterfaceClient)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DT_INTERFACE_CLIENT* dtNewHandle;
        
    if ((interfaceId == NULL) || (interfaceInstanceName == NULL) || (dtInterfaceClient == NULL))
    {
        LogError("Invalid parameter(s): interfaceId=%p, interfaceInstanceName=%p, dtInterfaceClient=%p", interfaceId, interfaceInstanceName, dtInterfaceClient);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (DT_InterfaceClient_CheckNameValid(interfaceId, true) != 0)
    {
        LogError("Invalid interfaceId %s", interfaceId);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (DT_InterfaceClient_CheckNameValid(interfaceInstanceName, false) != 0)
    {
        LogError("Invalid interfaceInstanceName %s", interfaceInstanceName);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if ((dtNewHandle = calloc(1, sizeof(DT_INTERFACE_CLIENT))) == NULL)
    {
        LogError("Cannot allocate interface client");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if (mallocAndStrcpy_s(&dtNewHandle->interfaceId, interfaceId) != 0)
    {
        LogError("Cannot allocate interfaceInstanceName");
        FreeDTInterface(dtNewHandle);
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if (mallocAndStrcpy_s(&dtNewHandle->interfaceInstanceName, interfaceInstanceName) != 0)
    {
        LogError("Cannot allocate interfaceInstanceName");
        FreeDTInterface(dtNewHandle);
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        SetInterfaceState(dtNewHandle, DT_INTERFACE_STATE_CREATED);
        dtNewHandle->userInterfaceContext = userInterfaceContext;
        dtNewHandle->dtInterfaceRegisteredCallback = dtInterfaceRegisteredCallback;
        dtNewHandle->interfaceInstanceNameLen = strlen(dtNewHandle->interfaceInstanceName);
        *dtInterfaceClient = dtNewHandle;
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback sets the passed dtPropertyUpdatedCallback to the interface.
DIGITALTWIN_CLIENT_RESULT DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, DIGITALTWIN_PROPERTY_UPDATE_CALLBACK dtPropertyUpdatedCallback)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    if ((dtInterfaceClientHandle == NULL) || (dtPropertyUpdatedCallback == NULL))
    {
        LogError("Invalid parameter, dtInterfaceClientHandle=%p, dtPropertyUpdatedCallback=%p", dtInterfaceClientHandle, dtPropertyUpdatedCallback);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (dtInterfaceClient->propertyUpdatedCallback != NULL)
    {
        LogError("PropertyUpdateCallback has already been set for this interface.  Cannot be called multiple times on given interface");
        result = DIGITALTWIN_CLIENT_ERROR_CALLBACK_ALREADY_REGISTERED;
    }
    else if (dtInterfaceClient->interfaceState != DT_INTERFACE_STATE_CREATED)
    {   
        LogError("PropertyUpdateCallback cannot be set after an interface registration has begun.  Current interface state=%s", 
                  MU_ENUM_TO_STRING(DT_INTERFACE_STATE, dtInterfaceClient->interfaceState));
        result = DIGITALTWIN_CLIENT_ERROR_INTERFACE_ALREADY_REGISTERED;
    }
    else
    {
        dtInterfaceClient->propertyUpdatedCallback = dtPropertyUpdatedCallback;
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// DigitalTwin_InterfaceClient_SetCommandsCallback sets the passed dtCommandExecuteCallback to the interface.
DIGITALTWIN_CLIENT_RESULT DigitalTwin_InterfaceClient_SetCommandsCallback(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, DIGITALTWIN_COMMAND_EXECUTE_CALLBACK dtCommandExecuteCallback)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    if ((dtInterfaceClientHandle == NULL) || (dtCommandExecuteCallback == NULL))
    {
        LogError("Invalid parameter, dtInterfaceClientHandle=%p, dtCommandExecuteCallback=%p", dtInterfaceClientHandle, dtCommandExecuteCallback);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (dtInterfaceClient->commandExecuteCallback != NULL)
    {
        LogError("CommandCallback has already been set for this interface.  Cannot be called multiple times on given interface");
        result = DIGITALTWIN_CLIENT_ERROR_CALLBACK_ALREADY_REGISTERED;
    }
    else if (dtInterfaceClient->interfaceState != DT_INTERFACE_STATE_CREATED)
    {   
        LogError("CommandCallback cannot be set after an interface registration has begun.  Current interface state=%s", 
                  MU_ENUM_TO_STRING(DT_INTERFACE_STATE, dtInterfaceClient->interfaceState));
        result = DIGITALTWIN_CLIENT_ERROR_INTERFACE_ALREADY_REGISTERED;
    }
    else
    {
        dtInterfaceClient->commandExecuteCallback = dtCommandExecuteCallback;
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}



// Determines whether the dtInterfaceClient can immediately free its resources during destruction, or whether it is bound up to a 
// clientCore caller which has a reference (in which case deletion will be deferred).
static bool IsInterfaceReadyForDestruction(DT_INTERFACE_CLIENT* dtInterfaceClient)
{
    bool result;
    
    switch (dtInterfaceClient->interfaceState)
    {
        case DT_INTERFACE_STATE_BOUND_TO_CLIENT_HANDLE:
        case DT_INTERFACE_STATE_REGISTERED:
        {
            // These states means that the interface still has a reference to a clientCore handle and can't be destroyed yet.
            // Not being able to do this now is not an error, so do not log here.
            result = false;
        }
        break;

        default:
        {
            result = true;
        }
    }

    return result;    
}

// DT_InterfaceClient_BindToClientHandle will establish the link between this DIGITALTWIN_INTERFACE_CLIENT_HANDLE with the DT_CLIENT_CORE_HANDLE that will use it.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceClient_BindToClientHandle(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, DT_CLIENT_CORE_HANDLE dtClientCoreHandle, DT_LOCK_THREAD_BINDING* lockThreadBinding)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    DIGITALTWIN_CLIENT_RESULT result;
    LOCK_HANDLE lockHandle;

    if ((dtInterfaceClientHandle == NULL) || (dtClientCoreHandle == NULL) || (lockThreadBinding == NULL))
    {
        LogError("Invalid parameter. dtInterfaceClientHandle=%p, dtClientCoreHandle=%p, lockThreadBinding=%p", dtInterfaceClientHandle, dtClientCoreHandle, lockThreadBinding);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (IsDesiredInterfaceStateTransitionAllowed(dtInterfaceClient, DT_INTERFACE_STATE_BOUND_TO_CLIENT_HANDLE) == false)
    {
        LogError("Interface is in state %d which does not permit BindToClientHandle", dtInterfaceClient->interfaceState);
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else if ((lockHandle = lockThreadBinding->dtBindingLockInit()) == NULL)
    {
        LogError("Failed initializing lock");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else
    {
        SetInterfaceState(dtInterfaceClient, DT_INTERFACE_STATE_BOUND_TO_CLIENT_HANDLE);
        dtInterfaceClient->dtClientCoreHandle = dtClientCoreHandle;
        memcpy(&dtInterfaceClient->lockThreadBinding, lockThreadBinding, sizeof(dtInterfaceClient->lockThreadBinding));
        dtInterfaceClient->lockThreadBinding.dtBindingLockHandle = lockHandle;
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}


// Marks the interface as unregistered; namely that the caller is through using it and it is
// safe to be deleted, either now (if it's already been destroyed) or later (when interface destroy comes)
// Once the caller initiates the unbind, the caller itself is promising that no callbacks will be invoked
// on this object.
void DT_InterfaceClient_UnbindFromClientHandle(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    bool lockHeld = false;

    if (dtInterfaceClientHandle == NULL)
    {
        LogError("Invalid parameter, dtInterfaceClientHandle=%p", dtInterfaceClientHandle);
    }
    else if (InvokeBindingInterfaceLock(dtInterfaceClientHandle, &lockHeld) != 0)
    {
        LogError("bindingLock failed");
    }
    else
    {
        // If we have been marked for a destroy already (via DigitalTwin_InterfaceClient_Destroy) then at this point
        // we are dropping our reference to this object and we can free.
        if (dtInterfaceClient->interfaceState == DT_INTERFACE_STATE_PENDING_DESTROY)
        {
            (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);
            FreeDTInterface(dtInterfaceClient);
        }
        else if (IsDesiredInterfaceStateTransitionAllowed(dtInterfaceClient, DT_INTERFACE_STATE_UNBOUND_FROM_CLIENT_HANDLE) == false)
        {
            LogError("Interface is in state %d which does not permit unbinding", dtInterfaceClient->interfaceState);
            (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);
        }
        else
        {

            if (dtInterfaceClient->dtInterfaceRegisteredCallback != NULL)
            {
                (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);
                dtInterfaceClient->dtInterfaceRegisteredCallback(DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING, dtInterfaceClient->userInterfaceContext);
                (void)InvokeBindingInterfaceLock(dtInterfaceClient, &lockHeld);
            }

            SetInterfaceState(dtInterfaceClient, DT_INTERFACE_STATE_UNBOUND_FROM_CLIENT_HANDLE);
            dtInterfaceClient->dtClientCoreHandle = NULL;
            (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);

            // After we enter the state DT_INTERFACE_STATE_UNBOUND_FROM_CLIENT_HANDLE and release the lock, 
            // ABSOLUTELY NO calls to this handle must happen on any callbacks.  This is marking
            // this interface as ready for another thread to delete it.
        }
    }
}

// The attempt to register this interface in the Cloud has either succeeded or failed.
void DT_InterfaceClient_RegistrationCompleteCallback(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    bool lockHeld = false;

    if (dtInterfaceClientHandle == NULL)
    {
        LogError("Invalid parameter, dtInterfaceClientHandle=%p", dtInterfaceClientHandle);
    }
    else if (InvokeBindingInterfaceLock(dtInterfaceClientHandle, &lockHeld) != 0)
    {
        LogError("bindingLock failed");
    }
    else if (IsDesiredInterfaceStateTransitionAllowed(dtInterfaceClient, DT_INTERFACE_STATE_REGISTERED) == false)
    {
        LogError("Interface is in state %d which does not transitioning to %d", dtInterfaceClient->interfaceState, DT_INTERFACE_STATE_REGISTERED);
    }
    else
    {
        if (dtInterfaceStatus != DIGITALTWIN_CLIENT_OK)
        {
            LogError("Interface %s failed on registration with error %s.  Interface is NOT registered, remaining in state=%s", dtInterfaceClient->interfaceInstanceName, 
                      MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus), MU_ENUM_TO_STRING(DT_INTERFACE_STATE, dtInterfaceClient->interfaceState));
        }
        else 
        {
            SetInterfaceState(dtInterfaceClient, DT_INTERFACE_STATE_REGISTERED);
        }
        (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);

        if (dtInterfaceClient->dtInterfaceRegisteredCallback != NULL)
        {
            dtInterfaceClient->dtInterfaceRegisteredCallback(dtInterfaceStatus, dtInterfaceClient->userInterfaceContext);
        }
    }

    (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);
}

// For specified property, generate appropriate json for updating it.  The json returned is 
// of the form { "IdForThisInterface" : { "propertyName" : propertyValue }.
static DIGITALTWIN_CLIENT_RESULT CreateJsonForPropertyWithoutResponse(DT_INTERFACE_CLIENT* dtInterfaceClient, const char* propertyName, const char* propertyData, STRING_HANDLE* jsonToSend)
{
    DIGITALTWIN_CLIENT_RESULT result;

    // We need to copy propertyData into propertyDataString because the caller didn't necessarily NULL terminate this string.
    if ((*jsonToSend = STRING_construct_sprintf(DT_PropertyWithoutResponseSchema, dtInterfaceClient->interfaceInstanceName, propertyName, propertyData)) == NULL)
    {
        LogError("Cannot allocate string handle");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// Creates json blob for indicating a property response
static DIGITALTWIN_CLIENT_RESULT CreateJsonForPropertyWithResponse(DT_INTERFACE_CLIENT* dtInterfaceClient, const char* propertyName, const char* propertyData, const DIGITALTWIN_CLIENT_PROPERTY_RESPONSE* dtResponse, STRING_HANDLE* jsonToSend)
{
    DIGITALTWIN_CLIENT_RESULT result;

    // See comments in CreateJsonForPropertyWithoutResponse about why we can't safely use parson to process propertyData.
    if ((*jsonToSend = STRING_construct_sprintf(DT_PropertyWithResponseSchema, 
                                                dtInterfaceClient->interfaceInstanceName, propertyName, propertyData, dtResponse->statusCode, 
                                                dtResponse->statusDescription, dtResponse->responseVersion)) == NULL)
    {
        LogError("Cannot allocate string handle");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// Creates a structure for tracking reported properties context.  We need to manually keep this because
// when we close IoTHub_* handle (and unlike for pending send telemetrys), we do NOT get callbacks for pending
// properties updates that don't get delivered.  That means they must be deleted here.
static DT_REPORT_PROPERTIES_UPDATE_CALLBACK_CONTEXT* CreateReportedPropertiesUpdateCallbackContext(DIGITALTWIN_REPORTED_PROPERTY_UPDATED_CALLBACK dtReportedPropertyCallback, void* userContextCallback)
{
    DT_REPORT_PROPERTIES_UPDATE_CALLBACK_CONTEXT* result;

    if ((result = calloc(1, sizeof(DT_REPORT_PROPERTIES_UPDATE_CALLBACK_CONTEXT))) == NULL)
    {
        LogError("Cannot allocate context for DT_REPORT_PROPERTIES_UPDATE_CALLBACK_CONTEXT");
    }
    else
    {
        result->dtReportedPropertyCallback = dtReportedPropertyCallback;
        result->userContextCallback = userContextCallback;
    }

    return result;
}

// Clients have access to DIGITALTWIN_INTERFACE_CLIENT_HANDLE handles immediately after their creation and they can attempt
// to do things like send telemetry on them right away.  This is NOT permitted, however, until the interface
// has been registered with the cloud (DT_INTERFACE_STATE_REGISTERED).  This function checks the state.
static bool IsInterfaceAvailable(DT_INTERFACE_CLIENT* dtInterfaceClient)
{
    bool result;
    bool lockHeld = false;

    if (InvokeBindingInterfaceLock(dtInterfaceClient, &lockHeld) != 0)
    {
        LogError("Cannot obtain lock");
        result = false;
    }
    else if (dtInterfaceClient->interfaceState != DT_INTERFACE_STATE_REGISTERED)
    {
        LogError("Interface is in state %d.  Cannot process client request.", dtInterfaceClient->interfaceState);
        result = false;
    }
    else
    {
        result = true;
    }

    (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);
    return result;
}

// CreateJsonForProperty is used to build up the JSON for a given property.
static DIGITALTWIN_CLIENT_RESULT CreateJsonForProperty(DT_INTERFACE_CLIENT* dtInterfaceClient, const char* propertyName, const char* propertyData, const DIGITALTWIN_CLIENT_PROPERTY_RESPONSE* dtResponse, STRING_HANDLE* jsonToSend)
{
    DIGITALTWIN_CLIENT_RESULT result;

    if (dtResponse == NULL)
    {
        result = CreateJsonForPropertyWithoutResponse(dtInterfaceClient, propertyName, propertyData, jsonToSend);
    }
    else
    {
        result = CreateJsonForPropertyWithResponse(dtInterfaceClient, propertyName, propertyData, dtResponse, jsonToSend);
    }

    return result;
}

// VerifyPropertyResponseIfNeeded verifies that if the property response has been set by caller, then it is valid.
static int VerifyPropertyResponseIfNeeded(const DIGITALTWIN_CLIENT_PROPERTY_RESPONSE* dtResponse)
{
    int result;

    if (dtResponse == NULL)
    {   
        // A NULL is acceptable if the caller does not have values to report here.
        result = 0;
    }
    else if (dtResponse->version != DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1)
    {
        LogError("Invalid DIGITALTWIN_CLIENT_PROPERTY_RESPONSE set.  Caller set it to <%d> but only <%d> is supported", dtResponse->version, DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

DIGITALTWIN_CLIENT_RESULT DigitalTwin_InterfaceClient_ReportPropertyAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, const char* propertyName, const char* propertyData, const DIGITALTWIN_CLIENT_PROPERTY_RESPONSE* dtResponse, DIGITALTWIN_REPORTED_PROPERTY_UPDATED_CALLBACK dtReportedPropertyCallback, void* userContextCallback)
{
    STRING_HANDLE jsonToSend = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    DT_REPORT_PROPERTIES_UPDATE_CALLBACK_CONTEXT* sendPropertiesUpdateContext = NULL;

    if ((dtInterfaceClientHandle == NULL) || (propertyName == NULL) || (propertyData == NULL))
    {
        LogError("Invalid parameter, one or more paramaters is NULL. dtInterfaceClientHandle=%p, propertyName=%p", dtInterfaceClientHandle, propertyName);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (VerifyPropertyResponseIfNeeded(dtResponse) != 0)
    {
        LogError("Invalid property response specified");
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (IsInterfaceAvailable(dtInterfaceClient) == false)
    {
        LogError("Interface not registered with Cloud");
        result = DIGITALTWIN_CLIENT_ERROR_INTERFACE_NOT_REGISTERED;
    }
    else if ((result = CreateJsonForProperty(dtInterfaceClient, propertyName, propertyData, dtResponse, &jsonToSend)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("Error creating json for reported property %s.  err = %d", propertyName, result);
    }
    else if ((sendPropertiesUpdateContext = CreateReportedPropertiesUpdateCallbackContext(dtReportedPropertyCallback, userContextCallback)) == NULL)
    {
        LogError("Cannot create update reportied properties callback context");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        unsigned const char* dataToSend = (unsigned const char*)STRING_c_str(jsonToSend);
        size_t dataToSendLen = strlen((const char*)dataToSend);
        if ((result = DT_ClientCoreReportPropertyStatusAsync(dtInterfaceClient->dtClientCoreHandle, dtInterfaceClientHandle, dataToSend, dataToSendLen, sendPropertiesUpdateContext)) != DIGITALTWIN_CLIENT_OK)
        {
            LogError("DT_ClientCoreReportPropertyStatusAsync failed, error = %d", result);
        }
        else
        {
            result = DIGITALTWIN_CLIENT_OK;
        }
    }

    if ((result != DIGITALTWIN_CLIENT_OK) && (sendPropertiesUpdateContext != NULL))
    {
        free(sendPropertiesUpdateContext);
    }

    STRING_delete(jsonToSend);
    return result;

}

// CreateJsonForTelemetryMessage returns a body that has the telemetry name wrapping the message contents
static DIGITALTWIN_CLIENT_RESULT CreateJsonForTelemetryMessage(const char* telemetryName, const char* messageData, STRING_HANDLE* jsonToSend)
{
    DIGITALTWIN_CLIENT_RESULT result;

    // See comments in CreateJsonForPropertyWithoutResponse about why we can't safely use parson to process propertyData.
    if ((*jsonToSend = STRING_construct_sprintf("{ \"%s\": %s }", telemetryName, messageData)) == NULL)
    {
        LogError("Cannot allocate string handle for telemetry message");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// Allocates a properly setup IOTHUB_MESSAGE_HANDLE for processing onto IoTHub.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceClient_CreateTelemetryMessage(const char* interfaceId, const char* interfaceInstanceName, const char* telemetryName, const char* messageData, IOTHUB_MESSAGE_HANDLE* telemetryMessageHandle)
{
    DIGITALTWIN_CLIENT_RESULT result;
    IOTHUB_MESSAGE_RESULT iothubMessageResult;

    if ((interfaceInstanceName == NULL) || (telemetryName == NULL) || (messageData == NULL) || (telemetryMessageHandle == NULL))
    {
        LogError("Invalid parameter(s): interfaceInstanceName=%p, telemetryName=%p, messageData=%p, telemetryMessageHandle=%p",
                 interfaceInstanceName, telemetryName, messageData, telemetryMessageHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else
    {
        size_t messageDataLen = strlen((const char*)messageData);

        if ((*telemetryMessageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)messageData, messageDataLen)) == NULL)
        {
            LogError("Cannot allocate IoTHubMessage for telemetry");
            result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
        }
        else if ((interfaceId != NULL) && ((iothubMessageResult = IoTHubMessage_SetProperty(*telemetryMessageHandle, DT_INTERFACE_ID_PROPERTY, interfaceId)) != IOTHUB_MESSAGE_OK))
        {
            LogError("Cannot set property %s, error = %d", DT_INTERFACE_ID_PROPERTY, iothubMessageResult);
            result = DIGITALTWIN_CLIENT_ERROR;
        }
        else if ((iothubMessageResult = IoTHubMessage_SetProperty(*telemetryMessageHandle, DT_INTERFACE_NAME_PROPERTY, interfaceInstanceName)) != IOTHUB_MESSAGE_OK)
        {
            LogError("Cannot set property %s, error = %d", DT_INTERFACE_ID_PROPERTY, iothubMessageResult);
            result = DIGITALTWIN_CLIENT_ERROR;
        }        
        else if ((iothubMessageResult = IoTHubMessage_SetProperty(*telemetryMessageHandle, DT_MESSAGE_SCHEMA_PROPERTY, telemetryName)) != IOTHUB_MESSAGE_OK)
        {
            LogError("Cannot set property %s, error = %d", DT_MESSAGE_SCHEMA_PROPERTY, iothubMessageResult);
            result = DIGITALTWIN_CLIENT_ERROR;
        }
        else if ((iothubMessageResult = IoTHubMessage_SetContentTypeSystemProperty(*telemetryMessageHandle, DT_JSON_MESSAGE_CONTENT_TYPE)) != IOTHUB_MESSAGE_OK)
        {
            LogError("Cannot set property %s, error = %d", DT_MESSAGE_SCHEMA_PROPERTY, iothubMessageResult);
            result = DIGITALTWIN_CLIENT_ERROR;
        }
        else
        {
            result = DIGITALTWIN_CLIENT_OK;
        }
    }

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        if ((telemetryMessageHandle != NULL) && ((*telemetryMessageHandle) != NULL))
        {
            IoTHubMessage_Destroy((*telemetryMessageHandle));
            *telemetryMessageHandle = NULL;
        }
    }

    return result;
}

// Creates a DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT item.
static DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT* CreateInterfaceSendTelemetryCallbackContext(DIGITALTWIN_CLIENT_TELEMETRY_CONFIRMATION_CALLBACK telemetryConfirmationCallback, void* userContextCallback)
{
    DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT* result;

    if ((result = calloc(1, sizeof(DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT))) == NULL)
    {
        LogError("Cannot allocate context for DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT");
    }
    else
    {
        result->telemetryConfirmationCallback = telemetryConfirmationCallback;
        result->userContextCallback = userContextCallback;
    }

    return result;
}

// DigitalTwin_InterfaceClient_SendTelemetryAsync sends the specified telemetry to Azure IoTHub in proper DigitalTwin data format.
DIGITALTWIN_CLIENT_RESULT DigitalTwin_InterfaceClient_SendTelemetryAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, const char* telemetryName, const char* messageData, DIGITALTWIN_CLIENT_TELEMETRY_CONFIRMATION_CALLBACK telemetryConfirmationCallback, void* userContextCallback)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    IOTHUB_MESSAGE_HANDLE telemetryMessageHandle = NULL;
    DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT* sendTelemetryCallbackContext = NULL;
    STRING_HANDLE jsonToSend = NULL;

    if ((dtInterfaceClientHandle == NULL) || (telemetryName == NULL) || (messageData == NULL))
    {
        LogError("Invalid parameter, one or more paramaters is NULL. dtInterfaceClientHandle=%p, telemetryName=%p", dtInterfaceClientHandle, telemetryName);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (IsInterfaceAvailable(dtInterfaceClient) == false)
    {
        LogError("Interface not registered with Cloud");
        result = DIGITALTWIN_CLIENT_ERROR_INTERFACE_NOT_REGISTERED;
    }
    else if ((result = CreateJsonForTelemetryMessage(telemetryName, messageData, &jsonToSend)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("Error creating json for telemetry message.  telemetryName=%s.  err = %d", telemetryName, result);
    }
    else
    {
        const char* dataToSend = STRING_c_str(jsonToSend);

        if ((result = DT_InterfaceClient_CreateTelemetryMessage(NULL, dtInterfaceClient->interfaceInstanceName, telemetryName, dataToSend, &telemetryMessageHandle)) != DIGITALTWIN_CLIENT_OK)
        {
            LogError("Cannot create send telemetry message, error = %d", result);
            result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
        }
        else if ((sendTelemetryCallbackContext = CreateInterfaceSendTelemetryCallbackContext(telemetryConfirmationCallback, userContextCallback)) == NULL)
        {
            LogError("Cannot create send telemetry message callback context");
            result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
        }
        else if ((result = DT_ClientCoreSendTelemetryAsync(dtInterfaceClient->dtClientCoreHandle, dtInterfaceClientHandle, telemetryMessageHandle, sendTelemetryCallbackContext)) != DIGITALTWIN_CLIENT_OK)
        {
            LogError("DT_ClientCoreSendTelemetryAsync failed, error = %d", result);
        }
    }

    if ((result != DIGITALTWIN_CLIENT_OK) && (sendTelemetryCallbackContext != NULL))
    {
        free(sendTelemetryCallbackContext);
    }

    IoTHubMessage_Destroy(telemetryMessageHandle);
    STRING_delete(jsonToSend);
    return result;
}

// DigitalTwin_InterfaceClient_Destroy destroys DIGITALTWIN_INTERFACE_CLIENT_HANDLE object.
void DigitalTwin_InterfaceClient_Destroy(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    bool lockHeld = false;

    if (dtInterfaceClientHandle == NULL)
    {
        LogError("Invalid parameter. interfaceClientHandle=%p", dtInterfaceClientHandle);
    }
    else if (InvokeBindingInterfaceLockIfAvailable(dtInterfaceClientHandle, &lockHeld) != 0)
    {
        LogError("bindingLock failed");
    }
    else if (IsDesiredInterfaceStateTransitionAllowed(dtInterfaceClient, DT_INTERFACE_STATE_PENDING_DESTROY) == false)
    {
        LogError("State %d cannot transition to DT_INTERFACE_STATE_PENDING_DESTROY", dtInterfaceClient->interfaceState);
        (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);
    }
    else
    {
        bool freeInterface = IsInterfaceReadyForDestruction(dtInterfaceClient);
        SetInterfaceState(dtInterfaceClient, DT_INTERFACE_STATE_PENDING_DESTROY);

        (void)InvokeBindingInterfaceUnlock(dtInterfaceClient, &lockHeld);

        if (freeInterface)
        {
            // If the DTDevice doesn't have a reference to this interface, we can delete it now.
            // Otherwise we'll properly delete this object once DigitalTwin device marks us as unregistered.
            FreeDTInterface(dtInterfaceClient);
        }
    }
}

// If available, retrieves a serialized json for given property.
static char* GetPayloadFromProperty(JSON_Object* dtInterfaceJson, const char* propertyName)
{
    char* payLoadForProperty;

    STRING_HANDLE propertyToQuery = STRING_construct_sprintf("%s.%s", propertyName, "value");
    if (propertyToQuery == NULL)
    {
        LogError("Unable to STRING_construct_sprintf for property to query");
        payLoadForProperty = NULL;
    }
    else
    {
        JSON_Value* jsonPropertyValue = json_object_dotget_value(dtInterfaceJson, STRING_c_str(propertyToQuery));
        if (jsonPropertyValue == NULL)
        {
            // Don't log errors in this case, as caller may invoke us not knowing if propertyName is present or not.
            payLoadForProperty = NULL;
        }
        else if ((payLoadForProperty = json_serialize_to_string(jsonPropertyValue)) == NULL)
        {
            LogError("json_serialize_to_string fails");
        }
    }

    STRING_delete(propertyToQuery);
    return payLoadForProperty;
}

// For each high-level property associated with this interface, determine if there is a callback function associated with it.
static void ProcessPropertyUpdateIfNeededFromDesired(DT_INTERFACE_CLIENT* dtInterfaceClient, JSON_Object* dtInterfaceDesiredJson, JSON_Object* dtInterfaceReportedJson, size_t propertyNumber, int jsonVersion)
{
    const char* propertyName;
    if ((propertyName = json_object_get_name(dtInterfaceDesiredJson, propertyNumber)) == NULL)
    {
        LogError("json_object_get_name fails");
    }
    else
    {
        char* payloadForDesiredProperty = NULL; 
        char* payloadForInitialProperty = NULL;

        if ((payloadForDesiredProperty = GetPayloadFromProperty(dtInterfaceDesiredJson, propertyName)) == NULL)
        {
            LogError("Unable to retrieve payload for property %s on interface %s", propertyName, dtInterfaceClient->interfaceInstanceName);
        }
        else
        {
            DIGITALTWIN_CLIENT_PROPERTY_UPDATE dtClientPropertyUpdate;

            dtClientPropertyUpdate.version = DIGITALTWIN_CLIENT_PROPERTY_UPDATE_VERSION_1;
            dtClientPropertyUpdate.propertyName = propertyName;

            if (dtInterfaceReportedJson)
            {
                payloadForInitialProperty = GetPayloadFromProperty(dtInterfaceReportedJson, propertyName);
            }
            else
            {
                payloadForInitialProperty = NULL;
            }

            dtClientPropertyUpdate.propertyReported = (unsigned const char*)payloadForInitialProperty;
            dtClientPropertyUpdate.propertyReportedLen = payloadForInitialProperty ? strlen(payloadForInitialProperty) : 0;
            dtClientPropertyUpdate.propertyDesired = (unsigned const char*)payloadForDesiredProperty;
            dtClientPropertyUpdate.propertyDesiredLen = strlen(payloadForDesiredProperty);
            dtClientPropertyUpdate.desiredVersion = jsonVersion;
        
            DigitalTwinLogInfo("DigitalTwin Interface : Invoking property callback for interface %s, propertyName=%s, userInterfaceContext=%p", 
                                dtInterfaceClient->interfaceInstanceName, propertyName, dtInterfaceClient->userInterfaceContext);
                
            dtInterfaceClient->propertyUpdatedCallback(&dtClientPropertyUpdate, dtInterfaceClient->userInterfaceContext);

            DigitalTwinLogInfo("DigitalTwin Interface: Invoking property callback returned");
        }

        json_free_serialized_string(payloadForDesiredProperty);
        json_free_serialized_string(payloadForInitialProperty);
    }
}

// Process the JSON_Object that represents a device twin's contents and invoke appropriate callbacks.
static void ProcessPropertiesForTwin(DT_INTERFACE_CLIENT* dtInterfaceClient, JSON_Object* desiredObject, JSON_Object* reportedObject)
{
    JSON_Object* dtInterfaceDesiredJson;
    JSON_Object* dtInterfaceReportedJson;

    STRING_HANDLE desiredPathForInterface = NULL;
    STRING_HANDLE reportedPathForInterface = NULL;

    // If there are property updates for this interface - which there may or may not be - the will in the passed in JSON 
    // under the paths built up in 'desiredPathForInterface' and 'reportedPathForInterface'.
    if ((desiredPathForInterface = STRING_construct_sprintf("%s%s", DT_INTERFACE_PREFIX, dtInterfaceClient->interfaceInstanceName)) == NULL)
    {   
        LogError("Cannot build desiredPathForInterface string");
    }
    else if ((reportedPathForInterface = STRING_construct_sprintf("%s%s", DT_INTERFACE_PREFIX, dtInterfaceClient->interfaceInstanceName)) == NULL)
    {
        LogError("Cannot build reportedPathForInterface string");
    }
    else
    {
        // Now that we have constructed the path to query, actually query the json object itself.
        dtInterfaceDesiredJson = json_object_get_object(desiredObject, STRING_c_str(desiredPathForInterface));
        if (reportedObject != NULL)
        {
            // If we're getting full twin, then it will include reported properties, too.
            dtInterfaceReportedJson = json_object_get_object(reportedObject, STRING_c_str(reportedPathForInterface));
        }
        else
        {
            dtInterfaceReportedJson = NULL;
        }

        if ((dtInterfaceDesiredJson== NULL) && (dtInterfaceReportedJson == NULL))
        {
            ; // Not being able to find this interface's name is json is not an error.
        }
        else
        {
            int jsonVersion = (int)json_object_dotget_number(desiredObject, DT_PROPERTY_UPDATE_JSON_VERSON);
        
            size_t numDesiredChildrenOnInterface = dtInterfaceDesiredJson ? json_object_get_count(dtInterfaceDesiredJson) : 0;
        
            for (size_t i = 0; i < numDesiredChildrenOnInterface; i++)
            {
                ProcessPropertyUpdateIfNeededFromDesired(dtInterfaceClient, dtInterfaceDesiredJson, dtInterfaceReportedJson, i, jsonVersion);
            }
        }
    }

    STRING_delete(reportedPathForInterface);
    STRING_delete(desiredPathForInterface);
}

// GetDesiredAndReportedJsonObjects will return from rootValue JSON_Object(s) representing desired and (for full twin only) reported of the twin
static int GetDesiredAndReportedJsonObjects(JSON_Value* rootValue, bool fullTwin, JSON_Object** desiredObject, JSON_Object** reportedObject)
{
    int result;

    if (fullTwin)
    {
        JSON_Object* rootObject = NULL;

        if ((rootObject = json_value_get_object(rootValue)) == NULL)
        {
            LogError("json_value_get_object fails");
            result = MU_FAILURE;
        }
        else if ((*desiredObject = json_object_get_object(rootObject, DT_JSON_DESIRED)) == NULL)
        {
            LogError("json_object_get_object fails retrieving %s", DT_JSON_DESIRED);
            result = MU_FAILURE;
        }
        else if ((*reportedObject = json_object_get_object(rootObject, DT_JSON_REPORTED)) == NULL)
        {
            LogError("json_object_get_object fails retrieving %s", DT_JSON_REPORTED);
            result = MU_FAILURE;
        }
        else
        {   
            result = 0;
        }
    }
    else
    {
        // When processing a TWIN PATCH operation, we don't receive explicit "desired" JSON string but this is implied.
        if ((*desiredObject = json_value_get_object(rootValue)) == NULL)
        {
            LogError("json_value_get_object fails for desired on twin update");
            result = MU_FAILURE;
        }
        else
        {
            *reportedObject = NULL;
            result = 0;
        }
    }

    return result;
}

// When a twin arrives from IoTHub_* twin callback layer, this function is invoked per interface to see if there are property callbacks previously registered by the 
// application that need to be notified of change.  Note that this function is called for each DIGITALTWIN_INTERFACE_CLIENT_HANDLE, as the caller does not parse
// the twin payload but instead relies on DT_InterfaceClient_ProcessTwinCallback to process it or silently ignore.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceClient_ProcessTwinCallback(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, bool fullTwin, const unsigned char* payLoad, size_t size)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    DIGITALTWIN_CLIENT_RESULT result;
    STRING_HANDLE jsonStringHandle = NULL;
    const char* jsonString;

    if ((dtInterfaceClientHandle == NULL) || (payLoad == NULL) || (size == 0))
    {
        LogError("Invalid parameter. dtInterfaceClientHandle=%p, payLoad=%p, size=%lu", dtInterfaceClientHandle, payLoad, (unsigned long)size);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (dtInterfaceClient->propertyUpdatedCallback == NULL)
    {
        // Not having a callback is not an error.  There's no point in parsing any twin data in this case, though, if 
        // there would be nothing to process it in the first place.
        LogInfo("Interface name <%s> does not have a property update callback registered with it.  Skipping twin update processing", dtInterfaceClient->interfaceInstanceName);
        result = DIGITALTWIN_CLIENT_OK;
    }
    else
    {
        JSON_Value* rootValue = NULL;
        JSON_Object* desiredObject = NULL;
        JSON_Object* reportedObject = NULL;

        if ((jsonStringHandle = STRING_from_byte_array(payLoad, size)) == NULL) 
        {
            LogError("STRING_construct_n failed");
            result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
        }
        else if ((jsonString = STRING_c_str(jsonStringHandle)) == NULL)
        {
            LogError("STRING_c_str failed");
            result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
        }
        else if ((rootValue = json_parse_string(jsonString)) == NULL)
        {
            LogError("json_parse_string failed");
            result = DIGITALTWIN_CLIENT_ERROR;
        }
        else if (GetDesiredAndReportedJsonObjects(rootValue, fullTwin, &desiredObject, &reportedObject) != 0)
        {
            LogError("GetDesiredAndReportedJsonObjects failed");
            result = DIGITALTWIN_CLIENT_ERROR;
        }
        else
        {
            ProcessPropertiesForTwin(dtInterfaceClient, desiredObject, reportedObject);
            result = DIGITALTWIN_CLIENT_OK;
        }

        if (rootValue != NULL)
        {
            json_value_free(rootValue);
        }
    }

    STRING_delete(jsonStringHandle);
    return result;
}

// Indicates whether given command is processed by this interface.  Commands arrive in the format
// "<interfaceInstanceName>*<commandOnInterfaceToInvoke>", so we need to match "<interfaceInstanceName>*" here.
static bool IsCommandMatchForInterfaceInstanceName(DT_INTERFACE_CLIENT* dtInterfaceClient, const char* methodName)
{
    bool result;

    if (strncmp(methodName, DT_INTERFACE_PREFIX, DT_INTERFACE_PREFIX_LENGTH) != 0)
    {
        LogError("Method name not prefixed with %s identifier.  Invalid request", DT_INTERFACE_PREFIX);
        result = false;
    }
    else if (strncmp(dtInterfaceClient->interfaceInstanceName, methodName + DT_INTERFACE_PREFIX_LENGTH, dtInterfaceClient->interfaceInstanceNameLen) == 0)
    {
        // If we match on length, still need to check that what's being queried has a command separator as character immediately following.
        result = (methodName[dtInterfaceClient->interfaceInstanceNameLen + DT_INTERFACE_PREFIX_LENGTH] == commandSeparator);
    }
    else
    {
        result = false;
    }

    return result;
}

// Actually invokes user's callback for a DigitalTwin (synchronous) command and fills out results for callee.
static void InvokeDTCommand(DT_INTERFACE_CLIENT* dtInterfaceClient, const char* commandName, const char* requestId, const char* payloadForCallback, unsigned char** response, size_t* response_size, int* responseCode)
{
    DIGITALTWIN_CLIENT_COMMAND_REQUEST dtClientCommandRequest;
    dtClientCommandRequest.version = DIGITALTWIN_CLIENT_COMMAND_REQUEST_VERSION_1;
    dtClientCommandRequest.commandName = commandName;
    dtClientCommandRequest.requestData = (const unsigned char*)payloadForCallback;
    dtClientCommandRequest.requestDataLen = strlen(payloadForCallback);
    dtClientCommandRequest.requestId = requestId;
    
    DIGITALTWIN_CLIENT_COMMAND_RESPONSE dtClientResponse;
    memset(&dtClientResponse, 0, sizeof(dtClientResponse));
    dtClientResponse.version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;
    
    DigitalTwinLogInfo("DigitalTwin Interface : Invoking callback to process command %s on interface name %s", commandName, dtInterfaceClient->interfaceInstanceName);

    dtInterfaceClient->commandExecuteCallback(&dtClientCommandRequest, &dtClientResponse, dtInterfaceClient->userInterfaceContext);
    
    DigitalTwinLogInfo("DigitalTwin Interface: Callback to process command returned.  responseData=%p, responseLen=%lu, responseStatus=%d", 
                        dtClientResponse.responseData, (unsigned long)dtClientResponse.responseDataLen, dtClientResponse.status);

    if (dtClientResponse.responseData == NULL)
    {
        // The application returning NULL for responseData is supported, as not all commands need to have modeling requiring a response.
        // We must return something over the network, however, so setting this to json null.
        if (mallocAndStrcpy_s((char**)&dtClientResponse.responseData, DT_JSON_NULL) == 0)
        {
            dtClientResponse.responseDataLen = strlen((const char*)dtClientResponse.responseData);
        }
        else
        {
            // Not much we can do on failure case, since whole point was to have some data to return.
            LogError("Unable to allocate a `null` response after application.  Because server expects some response, server request will likely timeout");
        }
    }

    // We do NOT free response here.  We instead pass it up to IoTHub Device SDK and the SDK itself is responsible for freeing.
    *response = dtClientResponse.responseData;
    *response_size = dtClientResponse.responseDataLen;
    *responseCode = dtClientResponse.status;
}

//
//  ParseCommandPayload parses the envelope of a payload request
//
static int ParseCommandPayload(const unsigned char* payLoad, size_t size, JSON_Value** rootValue, const char** requestId, const char** payloadForCallback)
{
    STRING_HANDLE jsonStringHandle = NULL;
    JSON_Object* rootObject;
    JSON_Value* payloadForCallbackValue;
    int result;

    if ((jsonStringHandle = STRING_from_byte_array(payLoad, size)) == NULL) 
    {
        LogError("STRING_construct_n failed");
        result = MU_FAILURE;
    }
    else if ((*rootValue = json_parse_string(STRING_c_str(jsonStringHandle))) == NULL)
    {
        LogError("json_parse_string failed");
        result = MU_FAILURE;
    }
    else if ((rootObject = json_value_get_object(*rootValue)) == NULL)
    {
        LogError("json_value_get_object fails");
        result = MU_FAILURE;
    }
    else if ((*requestId = json_object_dotget_string(rootObject, DT_JSON_COMMAND_REQUEST_ID)) == NULL)
    {
        LogError("json value <%s> is not available", DT_JSON_COMMAND_REQUEST_ID);
        result = MU_FAILURE;
    }
    else if ((payloadForCallbackValue = json_object_dotget_value(rootObject, DT_JSON_COMMAND_REQUEST_PAYLOAD)) == NULL)
    {
        LogError("json value <%s> is not available", DT_JSON_COMMAND_REQUEST_PAYLOAD);
        result = MU_FAILURE;
    }
    else if ((*payloadForCallback = json_serialize_to_string(payloadForCallbackValue)) == NULL)
    {
        LogError("json value <%s> is not available", DT_JSON_COMMAND_REQUEST_PAYLOAD);
        result = MU_FAILURE;
    }
    else 
    {
        result = 0;
    }

    if ((result != 0) && (*rootValue != NULL))
    {
        json_value_free(*rootValue);
        *rootValue = NULL;
    }

    STRING_delete(jsonStringHandle);
    return result;
}


// DT_InterfaceClient_InvokeCommandIfSupported is invoked whenever a device_method is received by the calling layer.  Note that 
// it may be invoked even if it's not for the given interface the command is arriving on (because caller has no parsing logic), in 
// which case it will return DT_COMMAND_PROCESSOR_NOT_APPLICABLE to tell the caller to move onto the next DIGITALTWIN_INTERFACE_CLIENT_HANDLE.
DT_COMMAND_PROCESSOR_RESULT DT_InterfaceClient_InvokeCommandIfSupported(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, const char* methodName, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, int* responseCode)
{
    DT_COMMAND_PROCESSOR_RESULT result;
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    JSON_Value* rootValue = NULL;
    const char* requestId = NULL;
    const char* payloadForCallback = NULL;

    if ((dtInterfaceClientHandle == NULL) || (methodName == NULL) || (response == NULL) || (response_size == NULL) || (responseCode == NULL))
    {
        LogError("Invalid parameter(s).  dtInterfaceClientHandle=%p, methodName=%p, response=%p, response_size=%p, responseCode=%p", dtInterfaceClientHandle, methodName, response, response_size, responseCode);
        result = DT_COMMAND_PROCESSOR_ERROR;
    }
    else if (IsCommandMatchForInterfaceInstanceName(dtInterfaceClient, methodName) == false)
    {
        // The method name is coming for a different interface.  There's nothing for us to do but silently ignore in this case.
        result = DT_COMMAND_PROCESSOR_NOT_APPLICABLE;
    }
    else if (dtInterfaceClient->commandExecuteCallback == NULL)
    {
        LogError("Interface <%s> cannot support this command because no callbacks are tied to it", dtInterfaceClient->interfaceInstanceName);
        result = DT_COMMAND_PROCESSOR_NOT_APPLICABLE;
    }
    else if (ParseCommandPayload(payload, size, &rootValue, &requestId, &payloadForCallback) != 0)
    {
        LogError("Payload request <%.*s> is not properly formatted json", (int)size, payload);
        result = DT_COMMAND_PROCESSOR_ERROR;
    }
    else
    {
        // Skip past the <interfaceInstanceName>* preamble to get the actual command name from DigitalTwin layer to map back to
        const char* commandName = methodName + DT_INTERFACE_PREFIX_LENGTH + dtInterfaceClient->interfaceInstanceNameLen + 1;

        InvokeDTCommand(dtInterfaceClient, commandName, requestId, payloadForCallback, response, response_size, responseCode);
        result = DT_COMMAND_PROCESSOR_PROCESSED;
    }

    if (payloadForCallback != NULL)
    {
        free((char*)payloadForCallback);
    }

    if (rootValue != NULL)
    {
        json_value_free(rootValue);
    }

    return result;
}

static int VerifyAsyncCommandUpdate(const DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE* dtClientAsyncCommandUpdate)
{
    int result;

    if ((dtClientAsyncCommandUpdate->commandName == NULL) || (dtClientAsyncCommandUpdate->requestId == NULL) ||
        (dtClientAsyncCommandUpdate->propertyData == NULL) || (dtClientAsyncCommandUpdate->version != DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE_VERSION_1))
    {
        LogError("DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE missing required field(s)");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

// The async command update is a special telemetry message which requires additional fields.  Add them here.
static DIGITALTWIN_CLIENT_RESULT DT_InterfaceClient_SetAsyncResponseProperties(IOTHUB_MESSAGE_HANDLE telemetryMessageHandle, const DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE* dtClientAsyncCommandUpdate)
{
    DIGITALTWIN_CLIENT_RESULT result;
    IOTHUB_MESSAGE_RESULT iothubMessageResult;
    char statusCodeString[DT_MAX_STATUS_CODE_STRINGLEN];

    if (0 > snprintf(statusCodeString, sizeof(statusCodeString), "%d", dtClientAsyncCommandUpdate->statusCode))
    {
        LogError("Cannot conver status code to a string");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else if ((iothubMessageResult = IoTHubMessage_SetProperty(telemetryMessageHandle, DT_STATUS_CODE_PROPERTY, statusCodeString)) != IOTHUB_MESSAGE_OK)
    {
        LogError("Cannot set property %s, error = %d", DT_STATUS_CODE_PROPERTY, iothubMessageResult);
        result = DIGITALTWIN_CLIENT_ERROR;
    } 
    else if ((iothubMessageResult = IoTHubMessage_SetProperty(telemetryMessageHandle, DT_REQUEST_ID_PROPERTY, dtClientAsyncCommandUpdate->requestId)) != IOTHUB_MESSAGE_OK)
    {
        LogError("Cannot set property %s, error = %d", DT_INTERFACE_ID_PROPERTY, iothubMessageResult);
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else if ((iothubMessageResult = IoTHubMessage_SetProperty(telemetryMessageHandle, DT_COMMAND_NAME_PROPERTY, dtClientAsyncCommandUpdate->commandName)) != IOTHUB_MESSAGE_OK)
    {
        LogError("Cannot set property %s, error = %d", DT_INTERFACE_ID_PROPERTY, iothubMessageResult);
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

// DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync is invoked by application when an async DigitalTwin command is executing, updating
// DigitalTwin server with status information provided by caller in a specially formatted telemetry message.
DIGITALTWIN_CLIENT_RESULT DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, const DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE* dtClientAsyncCommandUpdate)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    DIGITALTWIN_CLIENT_RESULT result;
    STRING_HANDLE jsonToSend = NULL;
    IOTHUB_MESSAGE_HANDLE telemetryMessageHandle = NULL;

    if ((dtInterfaceClientHandle == NULL) || (dtClientAsyncCommandUpdate == NULL))
    {
        LogError("Invalid parameter(s).  dtInterfaceClientHandle=%p, dtClientAsyncCommandUpdate=%p", dtInterfaceClientHandle, dtClientAsyncCommandUpdate);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (VerifyAsyncCommandUpdate(dtClientAsyncCommandUpdate) != 0)
    {
        LogError("DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE structure from caller invalid");
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;            
    }
    else if (IsInterfaceAvailable(dtInterfaceClient) == false)
    {
        LogError("Interface not registered with Cloud");
        result = DIGITALTWIN_CLIENT_ERROR_INTERFACE_NOT_REGISTERED;
    }
    else if ((result = CreateJsonForTelemetryMessage(DT_AsyncResultSchema, dtClientAsyncCommandUpdate->propertyData, &jsonToSend)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("CreateJsonForTelemetryMessage failed, error = %d", result);
    }
    else if ((result = DT_InterfaceClient_CreateTelemetryMessage(NULL, dtInterfaceClient->interfaceInstanceName, DT_AsyncResultSchema, STRING_c_str(jsonToSend),  &telemetryMessageHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("Cannot create send telemetry message, error = %d", result);
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((result = DT_InterfaceClient_SetAsyncResponseProperties(telemetryMessageHandle, dtClientAsyncCommandUpdate)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("Cannot create send telemetry message, error = %d", result);
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if ((result = DT_ClientCoreSendTelemetryAsync(dtInterfaceClient->dtClientCoreHandle, dtInterfaceClientHandle, telemetryMessageHandle, NULL)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DT_ClientCoreSendTelemetryAsync failed, error = %d", result);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    IoTHubMessage_Destroy(telemetryMessageHandle);
    STRING_delete(jsonToSend);
    return result;
}

// Processes callback for Sending a DigitalTwin Telemetry.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceClient_ProcessTelemetryCallback(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT dtSendTelemetryStatus, void* userContextCallback)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    if (dtInterfaceClientHandle == NULL)
    {
        LogError("Invalid parameter, dtInterfaceClientHandle=%p", dtInterfaceClientHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (userContextCallback == NULL)
    {
        // userContextCallback being NULL is not an error as not all telemetry invokers may care about callback notification.
        result = DIGITALTWIN_CLIENT_OK;
    }
    else
    {
        DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT* sendTelemetryCallbackContext = (DT_INTERFACE_SEND_TELEMETRY_CALLBACK_CONTEXT*)userContextCallback;

        if (sendTelemetryCallbackContext->telemetryConfirmationCallback == NULL)
        {
            // Caller did not register a callback for this telemetry.  Not an error, no logging needed.
            result = DIGITALTWIN_CLIENT_OK;
        }
        else
        {
            (void)dtInterfaceClient; // When DIGITALTWIN_LOGGING_ENABLED is 0, dtInterfaceClient not used and otherwise causes a false positive on -Wunused-variable.
            DigitalTwinLogInfo("DigitalTwin Interface: Invoking telemetry confirmation callback for interface=%s, reportedStatus=%s, userContextCallback=%p", 
                       dtInterfaceClient->interfaceInstanceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtSendTelemetryStatus), sendTelemetryCallbackContext->userContextCallback);

            sendTelemetryCallbackContext->telemetryConfirmationCallback(dtSendTelemetryStatus, sendTelemetryCallbackContext->userContextCallback);

            DigitalTwinLogInfo("DigitalTwin Interface: Invoking telemetry confirmation returned");
            result = DIGITALTWIN_CLIENT_OK;
        }
        free(sendTelemetryCallbackContext);
    }

    return result;
}

// Processes callback for reported property callback.
DIGITALTWIN_CLIENT_RESULT DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContextCallback)
{
    DT_INTERFACE_CLIENT* dtInterfaceClient = (DT_INTERFACE_CLIENT*)dtInterfaceClientHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    if (dtInterfaceClientHandle == NULL)
    {
        LogError("Invalid parameter, dtInterfaceClientHandle=%p", dtInterfaceClientHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (userContextCallback == NULL)
    {
        // Caller signaled a property update but didn't specify callback when the property completed.  This is NOT an
        // error, but means our processing of this callback is done at this point.
        result = DIGITALTWIN_CLIENT_OK;
    }
    else
    {
        DT_REPORT_PROPERTIES_UPDATE_CALLBACK_CONTEXT* dtReportedPropertyCallback = (DT_REPORT_PROPERTIES_UPDATE_CALLBACK_CONTEXT*)userContextCallback;

        if (dtReportedPropertyCallback->dtReportedPropertyCallback == NULL)
        {
            // Caller did not register a callback for this property.  Not an error, no logging needed.
            result = DIGITALTWIN_CLIENT_OK;
        }
        else
        {
            (void)dtInterfaceClient; // When DIGITALTWIN_LOGGING_ENABLED is 0, dtInterfaceClient not used and otherwise causes a false positive on -Wunused-variable.
            DigitalTwinLogInfo("DigitalTwin Interface: Invoking reported property update for interface=%s, reportedStatus=%s, userContextCallback=%p", 
                        dtInterfaceClient->interfaceInstanceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtReportedStatus), dtReportedPropertyCallback->userContextCallback);
            
            dtReportedPropertyCallback->dtReportedPropertyCallback(dtReportedStatus, dtReportedPropertyCallback->userContextCallback);

            DigitalTwinLogInfo("DigitalTwin Interface: Invoking reported property update returned"); 
            result = DIGITALTWIN_CLIENT_OK;
        }

        free(dtReportedPropertyCallback);
    }

    return result;
}

// Dt_InterfaceClient_GetSdkInfo returns a STRING_HANDLE containing properly formatted JSON containing the properties for this interface
DIGITALTWIN_CLIENT_RESULT DT_InterfaceClient_GetSdkInformation(STRING_HANDLE* sdkInfoHandle)
{
    DIGITALTWIN_CLIENT_RESULT result;

    if ((*sdkInfoHandle = STRING_construct_sprintf(DT_SdkInfoSchema, DT_SdkLanguage, DigitalTwin_Client_GetVersionString(), DT_SdkVendor)) == NULL)
    {
        LogError("STRING_construct_sprintf trying to create sdkInfo value");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    return result;
}

