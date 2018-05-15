// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/platform.h"

#include "iothub_client_core_ll.h"
#include "iothub_client_authorization.h"
#include "iothub_module_client_ll.h"
#include "iothub_transport_ll.h"
#include "iothub_client_private.h"
#include "iothub_client_options.h"
#include "iothub_client_version.h"
#include "iothub_client_diagnostic.h"
#include <stdint.h>

#ifndef DONT_USE_UPLOADTOBLOB
#include "iothub_client_ll_uploadtoblob.h"
#endif

IOTHUB_MODULE_CLIENT_LL_HANDLE IoTHubModuleClient_LL_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    return (IOTHUB_MODULE_CLIENT_LL_HANDLE)IoTHubClientCore_LL_CreateFromConnectionString(connectionString, protocol);
}

void IoTHubModuleClient_LL_Destroy(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle)
{
    IoTHubClientCore_LL_Destroy((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendEventAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SendEventAsync((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetSendStatus(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    return IoTHubClientCore_LL_GetSendStatus((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, iotHubClientStatus);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetMessageCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback)
{
	return IoTHubClientCore_LL_SetMessageCallback((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, messageCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetConnectionStatusCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void * userContextCallback)
{
    return IoTHubClientCore_LL_SetConnectionStatusCallback((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, connectionStatusCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetRetryPolicy(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    return IoTHubClientCore_LL_SetRetryPolicy((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, retryPolicy, retryTimeoutLimitInSeconds);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetRetryPolicy(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitInSeconds)
{
    return IoTHubClientCore_LL_GetRetryPolicy((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, retryPolicy, retryTimeoutLimitInSeconds);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetLastMessageReceiveTime(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime)
{
    return IoTHubClientCore_LL_GetLastMessageReceiveTime((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, lastMessageReceiveTime);
}

void IoTHubModuleClient_LL_DoWork(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle)
{
    IoTHubClientCore_LL_DoWork((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetOption(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value)
{
    return IoTHubClientCore_LL_SetOption((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, optionName, value);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetModuleTwinCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK moduleTwinCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SetDeviceTwinCallback((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, moduleTwinCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendReportedState(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SendReportedState((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, reportedState, size, reportedStateCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetModuleMethodCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SetDeviceMethodCallback_Ex((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, inboundDeviceMethodCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_ModuleMethodResponse(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    return IoTHubClientCore_LL_DeviceMethodResponse((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, methodId, response, response_size, status_response);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendEventToOutputAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SendEventToOutputAsync((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, eventMessageHandle, outputName, eventConfirmationCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetInputMessageCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SetInputMessageCallback((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, inputName, eventHandlerCallback, userContextCallback);
}


