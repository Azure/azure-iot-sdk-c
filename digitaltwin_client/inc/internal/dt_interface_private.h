// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DT_INTERFACE_CLIENT_CORE_H 
#define DT_INTERFACE_CLIENT_CORE_H 

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "digitaltwin_interface_client.h"
#include "dt_lock_thread_binding.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#include <stdbool.h>
#endif

#ifndef DT_CLIENT_CORE_HANDLE_TYPE_DEFINED
#define DT_CLIENT_CORE_HANDLE_TYPE_DEFINED
typedef struct DT_CLIENT_CORE* DT_CLIENT_CORE_HANDLE;
#endif

#define DT_SDK_INFORMATION_INTERFACE_NAME "urn_azureiot_Client_SDKInformation"

#define DT_COMMAND_PROCESSOR_RESULT_VALUES  \
    DT_COMMAND_PROCESSOR_ERROR,             \
    DT_COMMAND_PROCESSOR_NOT_APPLICABLE,    \
    DT_COMMAND_PROCESSOR_PROCESSED

MU_DEFINE_ENUM(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_RESULT_VALUES);

#define DT_COMMAND_ERROR_STATUS_CODE  500
#define DT_COMMAND_ERROR_NOT_IMPLEMENTED_CODE 501

MOCKABLE_FUNCTION(, int, DT_InterfaceClient_CheckNameValid, const char*, valueToCheck, bool, isInterfaceId);
MOCKABLE_FUNCTION(, const char*, DT_InterfaceClient_GetInterfaceInstanceName, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle);
MOCKABLE_FUNCTION(, const char*, DT_InterfaceClient_GetInterfaceId, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle);


MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceClient_ProcessTwinCallback, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, bool, fullTwin, const unsigned char*, payLoad, size_t, size);
MOCKABLE_FUNCTION(, DT_COMMAND_PROCESSOR_RESULT, DT_InterfaceClient_InvokeCommandIfSupported, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, const char*, method_name, const unsigned char*, payload, size_t, size, unsigned char**, response, size_t*, response_size, int*, responseCode);

MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceClient_BindToClientHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, DT_CLIENT_CORE_HANDLE, dtClientCoreHandle, DT_LOCK_THREAD_BINDING*, lockThreadBinding);
MOCKABLE_FUNCTION(, void, DT_InterfaceClient_UnbindFromClientHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle);
MOCKABLE_FUNCTION(, void, DT_InterfaceClient_RegistrationCompleteCallback, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceClient_ProcessTelemetryCallback, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT, dtSendTelemetryStatus, void*, userContextCallback);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, DIGITALTWIN_CLIENT_RESULT, dtReportedStatus, void*, userContextCallback);

MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceClient_CreateTelemetryMessage, const char*, interfaceId, const char*, interfaceInstanceName, const char*, telemetryName, const char*, messageData, IOTHUB_MESSAGE_HANDLE*, telemetryMessageHandle);
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DT_InterfaceClient_GetSdkInformation, STRING_HANDLE*, sdkInfoHandle);


#ifdef __cplusplus
}
#endif

#endif // DT_INTERFACE_CLIENT_CORE_H 

