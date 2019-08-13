// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DT_CLIENT_CORE_H
#define DT_CLIENT_CORE_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "azure_c_shared_utility/lock.h"

#include "iothub_client_core_common.h"
#include "digitaltwin_client_common.h"
#include "dt_lock_thread_binding.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#include <stdbool.h>
#endif

// DT_CLIENT_CORE implements the core logic of DigitalTwin that is agnostic to the type of handle
// (e.g. _LL_ versus convenience, and devices verus module) that the caller is processing.  Note
// that modules are not yet implemented but would follow same paradigm.
#ifndef DT_CLIENT_CORE_HANDLE_TYPE_DEFINED
#define DT_CLIENT_CORE_HANDLE_TYPE_DEFINED
typedef struct DT_CLIENT_CORE* DT_CLIENT_CORE_HANDLE;
#endif

// Callbacks used by DT_IOTHUB_BINDING.
typedef int(*DT_CLIENT_SEND_EVENT_ASYNC)(void* iothubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
typedef int(*DT_CLIENT_SET_TWIN_CALLBACK)(void* iothubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);
typedef int(*DT_CLIENT_SEND_REPORTED_STATE)(void* iothubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback);
typedef int(*DT_CLIENT_SET_METHOD_CALLBACK)(void* iothubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback);
typedef void(*DT_CLIENT_DESTROY)(void* iothubClientHandle);
typedef void(*DT_CLIENT_DOWORK)(void* iothubClientHandle);

// DigitalTwin uses dependency injection via the DT_IOTHUB_BINDING structure.  The upper layer (e.g. DIGITALTWIN_DEVICE_CLIENT_HANDLE) will pass in callbacks to
// interact with its specific type of handle (e.g. IOTHUB_DEVICE_CLIENT_HANDLE's) and the DTClientCore invokes these to interact with IoTHub.
//
// We use this paradigm because DTClientCore can't take link-time dependencies on any of these specific device handles.  Otherwise a application building
// an _LL_ based DigitalTwin client would end up having the IoTHub convenience layer brought in.  This also makes it easier to abstract
// whether we're using a Device or (to be implemented later) a Module.
typedef struct DT_IOTHUB_BINDING_TAG
{
    // Context and callbacks for interacting with IoTHub client
    void* iothubClientHandle;
    DT_CLIENT_SEND_EVENT_ASYNC dtClientSendEventAsync;
    DT_CLIENT_SET_TWIN_CALLBACK dtClientSetTwinCallback;
    DT_CLIENT_SEND_REPORTED_STATE dtClientSendReportedState;
    DT_CLIENT_SET_METHOD_CALLBACK dtClientSetMethodCallback;
    DT_CLIENT_DOWORK dtClientDoWork;
    DT_CLIENT_DESTROY dtClientDestroy;
} DT_IOTHUB_BINDING;

// Functions that upper layers invoke to use DTClientCore.  
// The actual application itself never accesses a DT_CLIENT_CORE_HANDLE directly, but instead gets to it
// via a public API handle they create (e.g. DIGITALTWIN_DEVICE_CLIENT_HANDLE).
MOCKABLE_FUNCTION(, DT_CLIENT_CORE_HANDLE, DT_ClientCoreCreate, DT_IOTHUB_BINDING*, iotHubBinding, DT_LOCK_THREAD_BINDING*, lockThreadBinding);
MOCKABLE_FUNCTION(, void, DT_ClientCoreDestroy, DT_CLIENT_CORE_HANDLE, dtClientCoreHandle);
MOCKABLE_FUNCTION(, void, DT_ClientCoreDoWork, DT_CLIENT_CORE_HANDLE, dtClientCoreHandle);

MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_ClientCoreRegisterInterfacesAsync, DT_CLIENT_CORE_HANDLE, dtClientCoreHandle, const char*, deviceCapabilityModel, DIGITALTWIN_INTERFACE_CLIENT_HANDLE*, dtInterfaces , unsigned int, numDTInterfaces, DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK, dtInterfaceRegisteredCallback, void*, userContextCallback);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_ClientCoreSendTelemetryAsync, DT_CLIENT_CORE_HANDLE, dtClientCoreHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, IOTHUB_MESSAGE_HANDLE, telemetryMessageHandle, void*, userContextCallback);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_ClientCoreReportPropertyStatusAsync, DT_CLIENT_CORE_HANDLE, dtClientCoreHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, const unsigned char*, dataToSend, size_t, dataToSendLen, void*, userContextCallback);

#ifdef __cplusplus
}
#endif

#endif // DT_CLIENT_CORE_H

