// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "iothub_device_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"

#include "internal/dt_interface_list.h"
#include "internal/dt_interface_private.h"
#include "internal/dt_client_core.h"

typedef enum DT_CLIENT_STATE_TAG
{
    DT_CLIENT_STATE_RUNNING,      // ClientCore will accept requests.  Default state.
    DT_CLIENT_STATE_SHUTTING_DOWN // The client shutdown the DTClient handle, but there are interfaces outstanding to it.
} DT_CLIENT_STATE;

typedef enum DT_CLIENT_REGISTRATION_STATUS_TAG
{
    // DT_CLIENT_REGISTRATION_STATUS_IDLE means that no DigitalTwin interfaces are registered nor in the state of being registered.
    // We're in this state after DigitalTwin registration, and if registration fails (since we can recover from a fail).
    DT_CLIENT_REGISTRATION_STATUS_IDLE,
    // DT_CLIENT_REGISTRATION_STATUS_REGISTERING indicates that DigitalTwin is in middle of registering its interfaces.
    // Other DigitalTwin operations - including trying to register interfaces again - don't happen in this state.
    DT_CLIENT_REGISTRATION_STATUS_REGISTERING,
    // DT_CLIENT_REGISTRATION_STATUS_REGISTERED signals that DigitalTwin interfaces have been successfully registered
    // with the service and that we're ready for DigitalTwin operations.
    DT_CLIENT_REGISTRATION_STATUS_REGISTERED
} DT_CLIENT_REGISTRATION_STATUS;

typedef struct DT_REGISTER_INTERFACES_CALLBACK_CONTEXT_TAG
{
    DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK dtInterfaceRegisteredCallback;
    void* userContextCallback;
} DT_REGISTER_INTERFACES_CALLBACK_CONTEXT;

// DT_CLIENT_CORE is the underlying representation of objects exposed up to the caller
// such as DIGITALTWIN_DEVICE_CLIENT_HANDLE, DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE, etc.
typedef struct DT_CLIENT_CORE_TAG
{
    bool processingCallback;    // Whether we're in the middle of processing a callback or not.
    DT_CLIENT_STATE clientState;
    DT_IOTHUB_BINDING iothubBinding;
    DT_LOCK_THREAD_BINDING lockThreadBinding;
    DT_CLIENT_REGISTRATION_STATUS registrationStatus;
    SINGLYLINKEDLIST_HANDLE reportedPropertyList; // List of DT_REPORTED_PROPERTY_CALLBACK_CONTEXT's
    bool registeredForDeviceMethodCallbacks;
    bool registeredForDeviceTwinCallbacks;
    DT_REGISTER_INTERFACES_CALLBACK_CONTEXT registerInterfacesCallbackContext;
    DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle;
} DT_CLIENT_CORE;

typedef struct DT_REPORTED_PROPERTY_CALLBACK_CONTEXT_TAG
{
    DT_CLIENT_CORE* dtClientCore;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle;
    void* userContextCallback;
} DT_REPORTED_PROPERTY_CALLBACK_CONTEXT;

typedef struct DT_SEND_TELEMETRY_CALLBACK_CONTEXT_TAG
{
    DT_CLIENT_CORE* dtClientCore;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle;
    void* userContextCallback;
} DT_SEND_TELEMETRY_CALLBACK_CONTEXT;


// When we're poll active callbacks to drain while shutting down DigitalTwin Client Core,
// this is the amount of time to sleep between poll intervals.
static const unsigned int pollTimeWaitForCallbacksMilliseconds = 10;


// Error codes for commands used when interface callback doesn't process command for whatever reason.
static const char methodNotPresentError[] = "\"Method not present\"";
static const size_t methodNotPresentErrorLen = sizeof(methodNotPresentError) - 1;
static const char methodInternalError[] =  "\"Internal error\"";
static const size_t methodInternalErrorLen = sizeof(methodInternalError) - 1;

static const int httpBadRequestStatusCode = 400;

MU_DEFINE_ENUM_STRINGS(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);

// Converts codes from IoTHub* API's into corresponding DigitalTwin error codes.
static DIGITALTWIN_CLIENT_RESULT GetDTSendStatusCodeFromIoTHubStatus(IOTHUB_CLIENT_CONFIRMATION_RESULT iothubResult)
{
    DIGITALTWIN_CLIENT_RESULT dtSendTelemetryStatus;
    switch (iothubResult)
    {
        case IOTHUB_CLIENT_CONFIRMATION_OK:
            dtSendTelemetryStatus = DIGITALTWIN_CLIENT_OK;
            break;

        case IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY:
            dtSendTelemetryStatus = DIGITALTWIN_CLIENT_ERROR_HANDLE_DESTROYED;
            break;

        case IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT:
            dtSendTelemetryStatus = DIGITALTWIN_CLIENT_ERROR_TIMEOUT;
            break;

        default:
            dtSendTelemetryStatus = DIGITALTWIN_CLIENT_ERROR;
            break;
    }

    return dtSendTelemetryStatus;
}

// Invokes appropriate IoTHub*_SendEventAsync function for given handle type.
static int InvokeBindingSendEventAsync(DT_CLIENT_CORE* dtClientCore, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, DT_SEND_TELEMETRY_CALLBACK_CONTEXT* dtSendTelemetryCallbackContext)
{
    return dtClientCore->iothubBinding.dtClientSendEventAsync(dtClientCore->iothubBinding.iothubClientHandle, eventMessageHandle, eventConfirmationCallback, dtSendTelemetryCallbackContext);
}

// Invokes appropriate IoTHub*_DoWork function for given handle type.
static void InvokeBindingDoWork(DT_CLIENT_CORE* dtClientCore)
{
    // DoWork by definition is only for single-threaded scenarios where there is no expectation (or even implementation)
    // of locking logic.  So no need to InvokeBindingLock() or unlock here.
    dtClientCore->iothubBinding.dtClientDoWork(dtClientCore->iothubBinding.iothubClientHandle);
}

// Invokes Lock() (for convenience layer based handles) or else a no-op (for _LL_)
static int InvokeBindingLock(DT_CLIENT_CORE* dtClientCore, bool* lockHeld)
{
    LOCK_RESULT lockResult = dtClientCore->lockThreadBinding.dtBindingLock(dtClientCore->lockThreadBinding.dtBindingLockHandle);
    int result;

    if (lockResult != LOCK_OK)
    {
        LogError("Failed to grab lock, result = %d", lockResult);
        result = MU_FAILURE;
    }
    else
    {
        // Setting whether lock is held here helps caller on cleanup.
        *lockHeld = true;
        result = 0;        
    }

    return result;
}
// Invokes Unlock() (for convenience layer based handles) or else a no-op (for _LL_)
static int InvokeBindingUnlock(DT_CLIENT_CORE* dtClientCore, bool* lockHeld)
{
    LOCK_RESULT lockResult;
    int result;
    
    if (*lockHeld == true)
    {
        lockResult = dtClientCore->lockThreadBinding.dtBindingUnlock(dtClientCore->lockThreadBinding.dtBindingLockHandle);
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
static void InvokeBindingLockDeinitIfNeeded(DT_CLIENT_CORE* dtClientCore)
{
    if (dtClientCore->lockThreadBinding.dtBindingLockHandle != NULL)
    {
        dtClientCore->lockThreadBinding.dtBindingLockDeinit(dtClientCore->lockThreadBinding.dtBindingLockHandle);
    }
}

// Invokes Thread_Sleep (for convenience layer based handles) or else a no-op (for _LL_)
static void InvokeBindingSleep(DT_CLIENT_CORE* dtClientCore, unsigned int milliseconds)
{
    dtClientCore->lockThreadBinding.dtBindingThreadSleep(milliseconds);
}

// Invokes IoTHub*_Destroy() for given handle
static void InvokeBindingDeviceClientDestroyIfNeeded(DT_CLIENT_CORE* dtClientCore)
{
    if (dtClientCore->iothubBinding.iothubClientHandle != NULL)
    {
        dtClientCore->iothubBinding.dtClientDestroy(dtClientCore->iothubBinding.iothubClientHandle);
        dtClientCore->iothubBinding.iothubClientHandle = NULL;
    }
}

// Invokes IoTHub*_SetDeviceTwinCallback for given handle
static int InvokeBindingSetDeviceTwinCallbackIfNeeded(DT_CLIENT_CORE* dtClientCore, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback)
{
    int result;

    if (dtClientCore->registeredForDeviceTwinCallbacks == true)
    {
        result = 0;
    }
    else
    {
        result = dtClientCore->iothubBinding.dtClientSetTwinCallback(dtClientCore->iothubBinding.iothubClientHandle, deviceTwinCallback, dtClientCore);
        if (result == 0)
        {
            dtClientCore->registeredForDeviceTwinCallbacks = true;
        }
    }
    return result;
}

// BeginClientCoreCallbackProcessing is invoked as the first step when DT_CLIENT_CORE receives
// a callback from the IoTHub_* layer.  If client is shutting down it will immediately
// exit out of the callback.  Otherwise mark our state as processing callback, as the
// primary state change API's (DT_ClientCoreRegisterInterfacesAsync and DT_ClientCoreDestroy) respect this.
static int BeginClientCoreCallbackProcessing(DT_CLIENT_CORE* dtClientCore)
{
    int result;
    bool lockHeld = false;

    if (InvokeBindingLock(dtClientCore, &lockHeld) != 0)
    {
        LogError("Unable to obtain lock");
        result = MU_FAILURE;
    }
    else if (dtClientCore->clientState == DT_CLIENT_STATE_SHUTTING_DOWN)
    {
        LogError("Cannot process callback for clientCore.  It is in process of being destroyed");
        result = MU_FAILURE;
    }
    else
    {
        dtClientCore->processingCallback = true;
        result = 0;
    }

    (void)InvokeBindingUnlock(dtClientCore, &lockHeld);
    return result;
}

// EndClientCoreCallbackProcessing is invoked on completion of a callback function
// to change DT_CLIENT_CORE's state such that it's not processing the callback anymore.
static void EndClientCoreCallbackProcessing(DT_CLIENT_CORE* dtClientCore)
{
    bool lockHeld = false;

    if (InvokeBindingLock(dtClientCore, &lockHeld) != 0)
    {
        LogError("Unable to obtain lock");
    }

    // Even if we can't grab the lock, unconditionally set this.
    dtClientCore->processingCallback = false;
    (void)InvokeBindingUnlock(dtClientCore, &lockHeld);
}

// Invokes appropriate IoTHub*_SetDeviceMethodCallback for given handle type.
static int InvokeBindingSetDeviceMethodCallbackIfNeeded(DT_CLIENT_CORE* dtClientCore, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback)
{
    int result;

    if (dtClientCore->registeredForDeviceMethodCallbacks == true)
    {
        result = 0;
    }
    else
    {
        result = dtClientCore->iothubBinding.dtClientSetMethodCallback((void*)dtClientCore->iothubBinding.iothubClientHandle, deviceMethodCallback, dtClientCore);
        if (result == 0)
        {
            dtClientCore->registeredForDeviceMethodCallbacks = true;
        }
    }

    return result;
}

// Unconditionally DT_REPORTED_PROPERTY_CALLBACK_CONTEXT item during list destruction.
static bool RemoveReportedPropertyCallbackContext(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    free((void*)item);
    *continue_processing = true;
    return true;
}

// When we can't invoke a user defined command callback, the DigitalTwin SDK 
// itself is responsible for reporting errors to the caller.
// MapCommandResultToDeviceMethod errors into (roughly) corresponding HTTP error codes.
// Note that DigitalTwin caller does not free memory allocated here; instead IoTHub_* layer free()'s after use.
static int MapCommandResultToDeviceMethod(DT_COMMAND_PROCESSOR_RESULT commandResult, unsigned char** response, size_t* response_size)
{
    int result;

    switch (commandResult)
    {
        case DT_COMMAND_PROCESSOR_NOT_APPLICABLE:
            if (mallocAndStrcpy_s((char**)response, methodNotPresentError) != 0)
            {
                LogError("Cannot malloc error string for return to caller");
                *response = NULL;
                *response_size = 0;
            }
            else
            {
                *response_size = methodNotPresentErrorLen;
            }
            result = DT_COMMAND_ERROR_NOT_IMPLEMENTED_CODE;
            break;

        default:
            if (mallocAndStrcpy_s((char**)response, methodInternalError) != 0)
            {
                LogError("Cannot malloc error string for return to caller");
                *response = NULL;
                *response_size = 0;
            }
            else
            {
                *response_size = methodInternalErrorLen;
            }
            result = DT_COMMAND_ERROR_STATUS_CODE;
            break;
    }

    return result;
}


// Layer invoked when a device method for DigitalTwin is received.
static int DTDeviceMethod_Callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    int result;

    if (userContextCallback == NULL)
    {
        LogError("userContextCallback=%p.  Cannot process callback.  Returning error to server", userContextCallback);
        result = MapCommandResultToDeviceMethod(DT_COMMAND_PROCESSOR_ERROR, response, response_size);
    }
    else
    {
        DT_CLIENT_CORE* dtClientCore = (DT_CLIENT_CORE*)userContextCallback;

        DigitalTwinLogInfo("DigitalTwin Client Core: Processing method callback for method %s, payload=%p, size=%lu", method_name, payload, (unsigned long)size);

        if (BeginClientCoreCallbackProcessing(dtClientCore) != 0)
        {
            LogError("Skipping callback processing");
            result = MapCommandResultToDeviceMethod(DT_COMMAND_PROCESSOR_ERROR, response, response_size);
        }
        else
        {
            DT_COMMAND_PROCESSOR_RESULT commandProcessorResult = DT_InterfaceList_InvokeCommand(dtClientCore->dtInterfaceListHandle, method_name, payload, size, response, response_size, &result);
            if (commandProcessorResult != DT_COMMAND_PROCESSOR_PROCESSED)
            {
                LogError("Command %s is not handled by any interface registered interface", method_name);
                result = MapCommandResultToDeviceMethod(commandProcessorResult, response, response_size);
            }

            EndClientCoreCallbackProcessing(dtClientCore);
        }
    }

    return result;
}

// Removes resources associated with DT_CLIENT_CORE.  The lock must not be held at this point and ref count should be 0.
static void FreeDTDeviceClientCore(DT_CLIENT_CORE* dtClientCore)
{
    // The destruction of the IoTHub binding handle MUST be the first action we take.  For convenience layer, this call will
    // block until IoTHub's dispatcher thread routine has completed.  By invoking this first, we ensure that there's
    // no possibility that a callback will arrive after we've deleted underlying data structure in DT_CLIENT_CORE.
    InvokeBindingDeviceClientDestroyIfNeeded(dtClientCore);
    
    if (dtClientCore->reportedPropertyList != NULL)
    {
        singlylinkedlist_remove_if(dtClientCore->reportedPropertyList, RemoveReportedPropertyCallbackContext, NULL);
        singlylinkedlist_destroy(dtClientCore->reportedPropertyList);
    }
    
    DT_InterfaceList_Destroy(dtClientCore->dtInterfaceListHandle);
    InvokeBindingLockDeinitIfNeeded(dtClientCore);
    free(dtClientCore);
}

// Polls for whether there are any active callbacks, because certain operations (like destroying a DigitalTwin client)
// cannot proceed if there are active workers.  Enters and leaves with lock held.
static void BlockOnActiveCallbacks(DT_CLIENT_CORE* dtClientCore, bool *lockHeld)
{
    while (dtClientCore->processingCallback == true)
    {
        (void)InvokeBindingUnlock(dtClientCore, lockHeld);
        InvokeBindingSleep(dtClientCore, pollTimeWaitForCallbacksMilliseconds);
        (void)InvokeBindingLock(dtClientCore, lockHeld);
    }
}

// Destroys a DT_CLIENT_CORE object once ref count is 0. We can't immediately delete the data, because interfaces may be pointing at it.  
// However, the ClientCore should be considered invalid at this state (interfaces trying to use it should fail).
// Furthermore we block until all worker threads have completed.
void DT_ClientCoreDestroy(DT_CLIENT_CORE_HANDLE dtClientCoreHandle)
{
    DT_CLIENT_CORE* dtClientCore = (DT_CLIENT_CORE*)dtClientCoreHandle;
    if (dtClientCore == NULL)
    {
        LogError("Invalid parameter.  dtClientCore=%p", dtClientCore);
    }
    else
    {
        bool lockHeld = false;
        
        if (InvokeBindingLock(dtClientCore, &lockHeld) != 0)
        {
            LogError("bindingLock failed");
        }
        else
        {
            // Even though there maybe interface pointers with references to this device still, 
            // destroying the DT_CLIENT_CORE_HANDLE means that subsequent calls they make as 
            // well as any pending IoTHub* callbacks should immediately fail.
            dtClientCore->clientState = DT_CLIENT_STATE_SHUTTING_DOWN;

            // We must poll until all callback threads have been processed.  Once we return from destroy, 
            // the caller is allowed to free any resources / interfaces associated with ClientCore so
            // we must be sure that they can never be invoked via a callback again.
            BlockOnActiveCallbacks(dtClientCore, &lockHeld);

            if (dtClientCore->dtInterfaceListHandle != NULL)
            {
                DT_InterfaceList_UnbindInterfaces(dtClientCore->dtInterfaceListHandle);
            }
            (void)InvokeBindingUnlock(dtClientCore, &lockHeld);
            FreeDTDeviceClientCore(dtClientCore);
        }
    }
}

// DT_ClientCoreCreate creates the DT_CLIENT_CORE_HANDLE object, which the upper layer
// will typecast directly for application (e.g. to a DIGITALTWIN_DEVICE_CLIENT_HANDLE).  Also stores
// the iothubBunding list for given handle type.
DT_CLIENT_CORE_HANDLE DT_ClientCoreCreate(DT_IOTHUB_BINDING* iotHubBinding, DT_LOCK_THREAD_BINDING* lockThreadBinding)
{
    int result = MU_FAILURE;
    DT_CLIENT_CORE* dtClientCore;
    LOCK_HANDLE lockHandle = NULL;

    if ((iotHubBinding == NULL) || (lockThreadBinding == NULL))
    {
        LogError("Invalid parameter(s). iotHubBinding=%p, lockThreadBinding=%p", iotHubBinding, lockThreadBinding);
        dtClientCore = NULL;
    }
    else if ((dtClientCore = (DT_CLIENT_CORE*)calloc(1, sizeof(DT_CLIENT_CORE))) == NULL)
    {
        LogError("Failed allocating client core handle");
    }
    else if ((lockHandle = lockThreadBinding->dtBindingLockInit()) == NULL)
    {
        LogError("Failed initializing lock");
    }
    else if ((dtClientCore->dtInterfaceListHandle = DigitalTwin_InterfaceList_Create()) == NULL)
    {
        LogError("Failed allocating dtInterfaceListHandle");
    }
    else if ((dtClientCore->reportedPropertyList = singlylinkedlist_create()) == NULL)
    {
        LogError("Failed allocating reportedPropertyList");
    }
    else
    {
        dtClientCore->clientState = DT_CLIENT_STATE_RUNNING;
        dtClientCore->registrationStatus = DT_CLIENT_REGISTRATION_STATUS_IDLE;
        memcpy(&dtClientCore->iothubBinding, iotHubBinding, sizeof(dtClientCore->iothubBinding));
        memcpy(&dtClientCore->lockThreadBinding, lockThreadBinding, sizeof(dtClientCore->lockThreadBinding));
        dtClientCore->lockThreadBinding.dtBindingLockHandle = lockHandle;
        lockHandle = NULL;
        result = 0;
    }

    if ((result != 0) && (dtClientCore != NULL))
    {
        FreeDTDeviceClientCore(dtClientCore);
        dtClientCore = NULL;
    }

    if (lockHandle != NULL)
    {    
        lockThreadBinding->dtBindingLockDeinit(lockHandle);
    }

    return (DT_CLIENT_CORE_HANDLE)dtClientCore;
}

// SetRegistrationCompleteInvokeCallbacks is invoked after DigitalTwin registration completes, either with success or failure.
// It invokes both the application's callback, if present, as well as notifying the underlying interface handles 
// themselves of the status of the registration.
static void SetRegistrationCompleteInvokeCallbacks(DT_CLIENT_CORE* dtClientCore, DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus)
{
    if (dtInterfaceStatus == DIGITALTWIN_CLIENT_OK)
    {
        // On success, we're ready to process additional DigitalTwin operations.
        dtClientCore->registrationStatus = DT_CLIENT_REGISTRATION_STATUS_REGISTERED; 
    }
    else
    {
        // Failure is not permanent - the caller can attempt to re-register interfaces.
        dtClientCore->registrationStatus = DT_CLIENT_REGISTRATION_STATUS_IDLE;
    }

    DT_InterfaceList_RegistrationCompleteCallback(dtClientCore->dtInterfaceListHandle, dtInterfaceStatus);

    if (dtClientCore->registerInterfacesCallbackContext.dtInterfaceRegisteredCallback != NULL)
    {
        dtClientCore->registerInterfacesCallbackContext.dtInterfaceRegisteredCallback(dtInterfaceStatus, dtClientCore->registerInterfacesCallbackContext.userContextCallback);
    }
}

// Invokes appropriate IoTHub*_SendReportedState function for given handle type.
static int InvokeBindingSendReportedStateAsync(DT_CLIENT_CORE* dtClientCore, const unsigned char* reportedState, size_t reportedStateLen, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContext)
{
    return dtClientCore->iothubBinding.dtClientSendReportedState(dtClientCore->iothubBinding.iothubClientHandle, reportedState, reportedStateLen, reportedStateCallback, userContext);
}

// CreateReportedPropertyCallbackContext allocates and fills out a context structure that is used to correlate
// reported property callbacks into the DT_ClientCore and ultimately API caller layer.
static DT_REPORTED_PROPERTY_CALLBACK_CONTEXT* CreateReportedPropertyCallbackContext(DT_CLIENT_CORE* dtClientCore, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, void* userContextCallback)
{
    DT_REPORTED_PROPERTY_CALLBACK_CONTEXT* result;
    
    if ((result = calloc(1, sizeof(DT_REPORTED_PROPERTY_CALLBACK_CONTEXT))) == NULL)
    {
        LogError("Unable to allocate DT_REPORTED_PROPERTY_CALLBACK_CONTEXT strecture");
    }
    else if (singlylinkedlist_add(dtClientCore->reportedPropertyList, result) == 0)
    {
        LogError("Unable to add to list");
        free(result);
        result = NULL;
    }
    else
    {
        result->dtClientCore = dtClientCore;
        result->dtInterfaceClientHandle = dtInterfaceClientHandle;
        result->userContextCallback = userContextCallback;
    }

    return result;
}

// Returns true when the match_context is equal to the current pointer traversing through the list.
static bool RemoveReportedPropertyCallbackContext_Visitor_IfMatch(const void* item, const void* match_context, bool* continue_processing)
{
    bool result;

    if (item == match_context)
    {
        *continue_processing = false;
        result = true;
    }
    else
    {
        *continue_processing = true;
        result = false;
    }

    return result;
}

// Frees a DT_REPORTED_PROPERTY_CALLBACK_CONTEXT structure and removes it from the linked list.
static void FreeReportedPropertyCallbackContext(DT_CLIENT_CORE* dtClientCore, DT_REPORTED_PROPERTY_CALLBACK_CONTEXT* dtReportedPropertyCallbackContext)
{
    singlylinkedlist_remove_if(dtClientCore->reportedPropertyList, RemoveReportedPropertyCallbackContext_Visitor_IfMatch, dtReportedPropertyCallbackContext);
    free(dtReportedPropertyCallbackContext);
}

// Reported states are used for both interface updates completions as well as properties of these interfaces.
// ReportedDTStateUpdate_Callback is invoked for either case (because underlying IoTHub_* API's only allow one callback for *all* twin
// reported callback operations).  This determines which state we're in, invokes appropriate callbacks, and frees the context we've allocated.
static void ReportedDTStateUpdate_Callback(int status_code, void* userContextCallback)
{
    // Note that a NULL callback is not necessarily an error, because reporting SDKInformation also goes through this path but does not set userContextCallback
    if (userContextCallback != NULL)
    {
        DT_REPORTED_PROPERTY_CALLBACK_CONTEXT* dtReportedPropertyCallbackContext = (DT_REPORTED_PROPERTY_CALLBACK_CONTEXT*)userContextCallback;
        DT_CLIENT_CORE* dtClientCore = dtReportedPropertyCallbackContext->dtClientCore;

        DigitalTwinLogInfo("DigitalTwin Client Core: Processing callback for reported state update.  status=%d, userContextCallback=%p", status_code, userContextCallback);

        if (status_code == httpBadRequestStatusCode)
        {
            // Print this additional status since we have HTTP status code at this layer.  The callback invocation on the interface
            // will LogInfo (if enabled) specific name of property/interface which is not readily available at dt_client_core layer.
            LogError("Reported property failed with a %d status code", status_code);
            LogError("This typically but not always means the JSON the application sent was invalid.");
        }

        if (BeginClientCoreCallbackProcessing(dtClientCore) != 0)
        {
            LogError("Skipping callback processing");
        }
        else
        {
            DIGITALTWIN_CLIENT_RESULT dtReportedStatus = (status_code < 300) ? DIGITALTWIN_CLIENT_OK : DIGITALTWIN_CLIENT_ERROR;
            (void)DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(dtClientCore->dtInterfaceListHandle, dtReportedPropertyCallbackContext->dtInterfaceClientHandle, dtReportedStatus, dtReportedPropertyCallbackContext->userContextCallback);

            EndClientCoreCallbackProcessing(dtClientCore);
        }

        FreeReportedPropertyCallbackContext(dtClientCore, dtReportedPropertyCallbackContext);
    }
}

// DeviceTwinDT_Callback is invoked when the device twin is updated, which occurs when RW properties are updated.
static void DeviceTwinDT_Callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    if (userContextCallback == NULL)
    {
        LogError("userContextCallback=%p.  Cannot process callback", userContextCallback);
    }
    else
    {
        DT_CLIENT_CORE* dtClientCore = (DT_CLIENT_CORE*)userContextCallback;

        DigitalTwinLogInfo("DigitalTwin Client Core: Device Twin callback called.  updateState=%s, payload=%p, size=%lu, userContextCallback=%p", 
                    MU_ENUM_TO_STRING(DEVICE_TWIN_UPDATE_STATE,update_state), payLoad, (unsigned long)size, userContextCallback);
        
        if (BeginClientCoreCallbackProcessing(dtClientCore) != 0)
        {
            LogError("Skipping callback processing");
        }
        else
        {
            bool fullTwin = (update_state == DEVICE_TWIN_UPDATE_COMPLETE);
            DT_InterfaceList_ProcessTwinCallbackForProperties(dtClientCore->dtInterfaceListHandle, fullTwin, payLoad, size);
            EndClientCoreCallbackProcessing(dtClientCore);
        }
    }
}

// Helper that allocates DT_SEND_TELEMETRY_CALLBACK_CONTEXT.
static DT_SEND_TELEMETRY_CALLBACK_CONTEXT* CreateDTSendTelemetryCallbackContext(DT_CLIENT_CORE* dtClientCore, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, void* userContextCallback)
{
    DT_SEND_TELEMETRY_CALLBACK_CONTEXT* dtSendTelemetryCallbackContext;

    if ((dtSendTelemetryCallbackContext = (DT_SEND_TELEMETRY_CALLBACK_CONTEXT*)calloc(1, sizeof(DT_SEND_TELEMETRY_CALLBACK_CONTEXT))) == NULL)
    {
        LogError("Cannot allocate DT_SEND_TELEMETRY_CALLBACK_CONTEXT");
    }
    else
    {
        dtSendTelemetryCallbackContext->dtClientCore = dtClientCore;
        dtSendTelemetryCallbackContext->dtInterfaceClientHandle = dtInterfaceClientHandle;
        dtSendTelemetryCallbackContext->userContextCallback = userContextCallback;
    }

    return dtSendTelemetryCallbackContext;
}

// SendSdkInformation sends information about this SDK to the corresponding reported properties.
// This sending of SDK info is a best effort only; if it fails, we will not otherwise block application
// from using the rest of the SDK.
static void SendSdkInformation(DT_CLIENT_CORE* dtClientCore)
{
    STRING_HANDLE sdkInfo = NULL;
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_GetSdkInformation(&sdkInfo);

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DT_InterfaceClient_GetSdkInfo failed, error = %d", result);
    }
    else
    {
        const char* sdkInfoString = STRING_c_str(sdkInfo);
        const size_t sdkInfoLen = strlen(sdkInfoString);

        LogInfo("Sending reported state for sdkInfo=%s", sdkInfoString);
        (void)InvokeBindingSendReportedStateAsync(dtClientCore, (unsigned const char*)sdkInfoString, sdkInfoLen, ReportedDTStateUpdate_Callback, NULL);
    }

    STRING_delete(sdkInfo);
}


// RegisterDTInterfaces_Callback is called when we receive acknowledgement from the service that the message we sent registering the interfaces was received.
static void RegisterDTInterfaces_Callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    DigitalTwinLogInfo("DigitalTwin Client Core: Processing register DigitalTwin interfaces callback.  confirmationResult=%s", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT,result));

    if (userContextCallback == NULL)
    {
        LogError("userContextCallback=%p.  Cannot process callback", userContextCallback);
    }
    else
    {
        DT_SEND_TELEMETRY_CALLBACK_CONTEXT* dtSendTelemetryCallbackContext = (DT_SEND_TELEMETRY_CALLBACK_CONTEXT*)userContextCallback;
        DT_CLIENT_CORE* dtClientCore = dtSendTelemetryCallbackContext->dtClientCore;

        if (BeginClientCoreCallbackProcessing(dtClientCore) != 0)
        {
            LogError("Skipping callback processing");
        }
        else
        {
            DIGITALTWIN_CLIENT_RESULT dtReportedInterfacesStatus = (result == IOTHUB_CLIENT_CONFIRMATION_OK) ? DIGITALTWIN_CLIENT_OK : DIGITALTWIN_CLIENT_ERROR;
            
            if (dtReportedInterfacesStatus == DIGITALTWIN_CLIENT_OK)
            {
                // Only after we've registered our interfaces should we start listening for incoming commands and properties.
                // If either of these fail, we will report an error to the application.
                DigitalTwinLogInfo("DigitalTwin Client Core: Interfaces successfully registered.  Register for device method and twin callbacks if needed");

                if (InvokeBindingSetDeviceMethodCallbackIfNeeded(dtClientCore, DTDeviceMethod_Callback) != 0)
                {
                    LogError("Cannot bind to device method callbacks");
                    dtReportedInterfacesStatus = DIGITALTWIN_CLIENT_ERROR;
                }
                else if (InvokeBindingSetDeviceTwinCallbackIfNeeded(dtClientCore, DeviceTwinDT_Callback) != 0)
                {
                    LogError("Cannot bind to device twin callbacks");
                    dtReportedInterfacesStatus = DIGITALTWIN_CLIENT_ERROR;
                }
                else
                {   
                    SendSdkInformation(dtClientCore);
                }
            }

            // Mark internal state as complete and signal to application the status of the DigitalTwin registration request.
            SetRegistrationCompleteInvokeCallbacks(dtClientCore, dtReportedInterfacesStatus);

            EndClientCoreCallbackProcessing(dtClientCore);
        }

        free(dtSendTelemetryCallbackContext);
    }

}


// SendInterfacesToRegisterMessage creates a telemetry message that will register the specified interfaces with the service.
static DIGITALTWIN_CLIENT_RESULT SendInterfacesToRegisterMessage(DT_CLIENT_CORE* dtClientCore, const char* deviceCapabilityModel)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DT_SEND_TELEMETRY_CALLBACK_CONTEXT* dtSendTelemetryCallbackContext = NULL;
    IOTHUB_MESSAGE_HANDLE telemetryRegisterMessage = NULL;

    if ((result = DT_InterfaceList_CreateRegistrationMessage(dtClientCore->dtInterfaceListHandle, deviceCapabilityModel, &telemetryRegisterMessage)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DT_InterfaceList_CreateRegistrationMessage failed");
    }
    else if ((dtSendTelemetryCallbackContext = CreateDTSendTelemetryCallbackContext(dtClientCore, NULL, NULL)) == NULL)
    {
        LogError("Unable to create callback context");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if (InvokeBindingSendEventAsync(dtClientCore, telemetryRegisterMessage, RegisterDTInterfaces_Callback, dtSendTelemetryCallbackContext) != 0)
    {
        LogError("InvokeBindingSendEventAsync failed");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else
    {
        LogInfo("Successfully queued registration message");
        result = DIGITALTWIN_CLIENT_OK;
    }

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        free(dtSendTelemetryCallbackContext);
    }

    IoTHubMessage_Destroy(telemetryRegisterMessage);
    return result;
}

// DT_ClientCoreRegisterInterfacesAsync updates the list of interfaces we're supporting and begins 
// protocol update of server.
DIGITALTWIN_CLIENT_RESULT DT_ClientCoreRegisterInterfacesAsync(DT_CLIENT_CORE_HANDLE dtClientCoreHandle, const char* deviceCapabilityModel, DIGITALTWIN_INTERFACE_CLIENT_HANDLE* dtInterfaces, unsigned int numDTInterfaces, DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK dtInterfaceRegisteredCallback, void* userContextCallback)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DT_CLIENT_CORE* dtClientCore = (DT_CLIENT_CORE*)dtClientCoreHandle;
    bool lockHeld = false;

    if ((dtClientCoreHandle == NULL) || (deviceCapabilityModel == NULL))
    {
        LogError("Invalid parameter(s). dtClientCoreHandle=%p, deviceCapabilityModel=%p", dtClientCoreHandle, deviceCapabilityModel);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (DT_InterfaceClient_CheckNameValid(deviceCapabilityModel, true) != 0)
    {
        LogError("Invalid deviceCapabilityModel %s", deviceCapabilityModel);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (InvokeBindingLock(dtClientCore, &lockHeld) != 0)
    {
        LogError("Lock failed");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else if (dtClientCore->registrationStatus == DT_CLIENT_REGISTRATION_STATUS_REGISTERING)
    {
        LogError("Cannot register because status is %d but must be idle", dtClientCore->registrationStatus);
        result = DIGITALTWIN_CLIENT_ERROR_REGISTRATION_PENDING;
    }
    else if (dtClientCore->registrationStatus == DT_CLIENT_REGISTRATION_STATUS_REGISTERED)
    {
        LogError("Cannot register because interface is already registered");
        result = DIGITALTWIN_CLIENT_ERROR_INTERFACE_ALREADY_REGISTERED;
    }
    else if ((result = DT_InterfaceList_BindInterfaces(dtClientCore->dtInterfaceListHandle, dtInterfaces, numDTInterfaces, dtClientCoreHandle, &dtClientCore->lockThreadBinding)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DT_InterfaceList_BindInterfaces failed, result = %d", result);
    }
    else if (SendInterfacesToRegisterMessage(dtClientCore, deviceCapabilityModel) != 0)
    {
        LogError("SendInterfacesToRegisterMessage failed");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else
    {
        dtClientCore->registerInterfacesCallbackContext.dtInterfaceRegisteredCallback = dtInterfaceRegisteredCallback;
        dtClientCore->registerInterfacesCallbackContext.userContextCallback = userContextCallback;
    
        // Once we're processing an interface update, no other caller initiated operations on this device client can occur.
        dtClientCore->registrationStatus = DT_CLIENT_REGISTRATION_STATUS_REGISTERING;
        result = DIGITALTWIN_CLIENT_OK;
    }

    (void)InvokeBindingUnlock(dtClientCore, &lockHeld);
    return result;
}

// SendDTTelemetry_Callback is hooked into IoTHub Client API's callback on SendEvent completion and translates this to the DigitalTwin telemetry callback.
static void SendDTTelemetry_Callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (userContextCallback != NULL)
    {
        DT_SEND_TELEMETRY_CALLBACK_CONTEXT* dtSendTelemetryCallbackContext = (DT_SEND_TELEMETRY_CALLBACK_CONTEXT*)userContextCallback;
        DT_CLIENT_CORE* dtClientCore = dtSendTelemetryCallbackContext->dtClientCore;

        DigitalTwinLogInfo("DigitalTwin Client Core: Processing telemetry callback.  confirmationResult=%s, userContextCallback=%p", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT,result), userContextCallback);

        if (BeginClientCoreCallbackProcessing(dtClientCore) != 0)
        {
            LogError("Skipping callback processing");
        }
        else
        {
            DIGITALTWIN_CLIENT_RESULT dtSendTelemetryStatus = GetDTSendStatusCodeFromIoTHubStatus(result);

            (void)DT_InterfaceList_ProcessTelemetryCallback(dtClientCore->dtInterfaceListHandle, dtSendTelemetryCallbackContext->dtInterfaceClientHandle, dtSendTelemetryStatus, dtSendTelemetryCallbackContext->userContextCallback);
            EndClientCoreCallbackProcessing(dtClientCore);
        }

        free(dtSendTelemetryCallbackContext);
    }
}

// DT_ClientCoreSendTelemetryAsync sends the specified telemetry to Azure IoTHub in proper DigitalTwin data format.
DIGITALTWIN_CLIENT_RESULT DT_ClientCoreSendTelemetryAsync(DT_CLIENT_CORE_HANDLE dtClientCoreHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, IOTHUB_MESSAGE_HANDLE telemetryMessageHandle, void* userContextCallback)
{
    DT_CLIENT_CORE* dtClientCore = (DT_CLIENT_CORE*)dtClientCoreHandle;
    DIGITALTWIN_CLIENT_RESULT result;
    DT_SEND_TELEMETRY_CALLBACK_CONTEXT* dtSendTelemetryCallbackContext = NULL;
    bool lockHeld = false;

    if ((dtClientCoreHandle == NULL) || (dtInterfaceClientHandle == NULL) || (telemetryMessageHandle == NULL))
    {
        LogError("Invalid parameter, one or more paramaters is invalid. dtClientCoreHandle=%p, dtInterfaceClientHandle=%p, telemetryMessageHandle=%p", dtClientCoreHandle, dtInterfaceClientHandle, telemetryMessageHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (InvokeBindingLock(dtClientCore, &lockHeld) != 0)
    {
        LogError("Lock failed");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else if (dtClientCore->clientState == DT_CLIENT_STATE_SHUTTING_DOWN)
    {
        LogError("Client is shutting down");
        result = DIGITALTWIN_CLIENT_ERROR_SHUTTING_DOWN;
    }
    else if ((dtSendTelemetryCallbackContext = CreateDTSendTelemetryCallbackContext(dtClientCore, dtInterfaceClientHandle, userContextCallback)) == NULL)
    {
        LogError("Cannot allocate DT_SEND_TELEMETRY_CALLBACK_CONTEXT");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if (InvokeBindingSendEventAsync(dtClientCore, telemetryMessageHandle, SendDTTelemetry_Callback, dtSendTelemetryCallbackContext) != 0)
    {
        LogError("SendDTTelemetry failed");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    // No events that can fail should come after we've sent the event, or else our state will be considerably more complicated (e.g. this function could
    // return an error but there would still be a send callback pending for it that would arrive later).
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        free(dtSendTelemetryCallbackContext);
    }

    (void)InvokeBindingUnlock(dtClientCore, &lockHeld);
    return result;
}

// DT_ClientCoreReportPropertyStatusAsync is an API apps ultimately invoke through to send data.  The data is already serilaized by caller,
// so this layer just interacts with IoTHub* layer.
DIGITALTWIN_CLIENT_RESULT DT_ClientCoreReportPropertyStatusAsync(DT_CLIENT_CORE_HANDLE dtClientCoreHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, const unsigned char* dataToSend, size_t dataToSendLen, void* userContextCallback)
{
    DT_CLIENT_CORE* dtClientCore = (DT_CLIENT_CORE*)dtClientCoreHandle;
    DIGITALTWIN_CLIENT_RESULT result;
    bool lockHeld = false;
    DT_REPORTED_PROPERTY_CALLBACK_CONTEXT* reportedPropertyCallbackContext;

    if ((dtClientCoreHandle == NULL) || (dtInterfaceClientHandle == NULL) || (dataToSend == NULL) || (dataToSendLen == 0))
    {
        LogError("Invalid parameter, one or more paramaters is NULL. dtClientCoreHandle=%p, dtInterfaceClientHandle=%p, dataToSend=%p, dataToSendLen=%lu", dtClientCoreHandle, dtInterfaceClientHandle, dataToSend, (unsigned long)dataToSendLen);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if (InvokeBindingLock(dtClientCore, &lockHeld) != 0)
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else if (dtClientCore->clientState == DT_CLIENT_STATE_SHUTTING_DOWN)
    {
        LogError("Client is shutting down");
        result = DIGITALTWIN_CLIENT_ERROR_SHUTTING_DOWN;
    }
    else if ((reportedPropertyCallbackContext = CreateReportedPropertyCallbackContext(dtClientCore, dtInterfaceClientHandle, userContextCallback)) == NULL)
    {
        LogError("Cannot allocate callback context");
        result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
    }
    else if (InvokeBindingSendReportedStateAsync(dtClientCore, dataToSend, dataToSendLen, ReportedDTStateUpdate_Callback, reportedPropertyCallbackContext) != 0)
    {
        // Free reportedPropertyCallbackContext here; after we register callback, the callback will be responsible for freeing this memory.
        FreeReportedPropertyCallbackContext(dtClientCore, reportedPropertyCallbackContext);
        LogError("Invoking binding for SendReportedStatu");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else
    {
        result = DIGITALTWIN_CLIENT_OK;
    }

    (void)InvokeBindingUnlock(dtClientCore, &lockHeld);
    return result;
}

// For _LL_ clients specifically, invokes back to the binding handle that implements IoTHub*_DoWork.
void DT_ClientCoreDoWork(DT_CLIENT_CORE_HANDLE dtClientCoreHandle)
{
    if (dtClientCoreHandle == NULL)
    {
        LogError("Invalid parameter: dtClientCoreHandle=NULL");
    }
    else
    {
        InvokeBindingDoWork((DT_CLIENT_CORE*)dtClientCoreHandle);
    }
}

