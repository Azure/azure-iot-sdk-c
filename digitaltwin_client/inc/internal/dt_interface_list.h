// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DT_INTERFACE_LIST_H
#define DT_INTERFACE_LIST_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "iothub_message.h"

#include "internal/dt_interface_private.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#include <stdbool.h>
#endif

typedef struct DT_INTERFACE_LIST* DIGITALTWIN_INTERFACE_LIST_HANDLE;

MOCKABLE_FUNCTION(, DIGITALTWIN_INTERFACE_LIST_HANDLE, DigitalTwin_InterfaceList_Create);
MOCKABLE_FUNCTION(, void, DT_InterfaceList_Destroy, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceListHandle);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceList_BindInterfaces, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceListHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE*, dtInterfaces, unsigned int, numDTInterfaces, DT_CLIENT_CORE_HANDLE, dtClientCoreHandle, DT_LOCK_THREAD_BINDING*, lockThreadBinding);
MOCKABLE_FUNCTION(, void, DT_InterfaceList_UnbindInterfaces, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceListHandle);
MOCKABLE_FUNCTION(, void, DT_InterfaceList_RegistrationCompleteCallback, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceListHandle, DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus);
MOCKABLE_FUNCTION(, DT_COMMAND_PROCESSOR_RESULT, DT_InterfaceList_InvokeCommand, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceListHandle, const char*, method_name, const unsigned char*, payload, size_t, size, unsigned char**, response, size_t*, response_size, int*, resultFromCommandCallback);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceList_ProcessTwinCallbackForProperties, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceClientHandle, bool, fullTwin, const unsigned char*, payLoad, size_t, size);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceList_ProcessTelemetryCallback, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceListHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT, dtSendTelemetryStatus, void*, userContextCallback);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceList_ProcessReportedPropertiesUpdateCallback, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceListHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT, dtReportedStatus, void*, userContextCallback);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceList_CreateRegistrationMessage, DIGITALTWIN_INTERFACE_LIST_HANDLE, dtInterfaceListHandle, const char*, deviceCapabilityModel, IOTHUB_MESSAGE_HANDLE*, messageHandle);

#ifdef __cplusplus
}
#endif

#endif // DT_INTERFACE_LIST_H

