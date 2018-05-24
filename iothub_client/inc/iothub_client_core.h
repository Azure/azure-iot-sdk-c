// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_client_core.h
*	@brief Extends the IoTHubCLient_LL module with additional features.
*
*	@details IoTHubClient is a module that extends the IoTHubCLient_LL
*			 module with 2 features:
*				- scheduling the work for the IoTHubCLient from a
*				  thread, so that the user does not need to create their
*				  own thread
*				- thread-safe APIs
*/

#ifndef IOTHUB_CLIENT_CORE_H
#define IOTHUB_CLIENT_CORE_H

#include <stddef.h>
#include <stdint.h>
#include "azure_c_shared_utility/umock_c_prod.h"

#include "iothub_client_core_ll.h"

#ifndef IOTHUB_CLIENT_CORE_INSTANCE_TYPE
typedef struct IOTHUB_CLIENT_CORE_INSTANCE_TAG* IOTHUB_CLIENT_CORE_HANDLE;
#define IOTHUB_CLIENT_CORE_INSTANCE_TYPE
#endif // IOTHUB_CLIENT_CORE_INSTANCE

#ifdef __cplusplus
extern "C"
{
#endif

    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_CORE_HANDLE, IoTHubClientCore_CreateFromConnectionString, const char*, connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_CORE_HANDLE, IoTHubClientCore_Create, const IOTHUB_CLIENT_CONFIG*, config);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_CORE_HANDLE, IoTHubClientCore_CreateWithTransport, TRANSPORT_HANDLE, transportHandle, const IOTHUB_CLIENT_CONFIG*, config);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_CORE_HANDLE, IoTHubClientCore_CreateFromDeviceAuth, const char*, iothub_uri, const char*, device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);
    MOCKABLE_FUNCTION(, void, IoTHubClientCore_Destroy, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SendEventAsync, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_GetSendStatus, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SetMessageCallback, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, messageCallback, void*, userContextCallback);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SetConnectionStatusCallback, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, connectionStatusCallback, void*, userContextCallback);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SetRetryPolicy, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_GetRetryPolicy, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY*, retryPolicy, size_t*, retryTimeoutLimitInSeconds);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_GetLastMessageReceiveTime, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, time_t*, lastMessageReceiveTime);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SetOption, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, const char*, optionName, const void*, value);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SetDeviceTwinCallback, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, deviceTwinCallback, void*, userContextCallback);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SendReportedState, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, const unsigned char*, reportedState, size_t, size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, reportedStateCallback, void*, userContextCallback);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SetDeviceMethodCallback, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, deviceMethodCallback, void*, userContextCallback);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_SetDeviceMethodCallback_Ex, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK, inboundDeviceMethodCallback, void*, userContextCallback);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_DeviceMethodResponse, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, METHOD_HANDLE, methodId, const unsigned char*, response, size_t, response_size, int, statusCode);

#ifndef DONT_USE_UPLOADTOBLOB
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_UploadToBlobAsync, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, const char*, destinationFileName, const unsigned char*, source, size_t, size, IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK, iotHubClientFileUploadCallback, void*, context);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClientCore_UploadMultipleBlocksToBlobAsync, IOTHUB_CLIENT_CORE_HANDLE, iotHubClientHandle, const char*, destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK, getDataCallback, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, getDataCallbackEx, void*, context);
#endif /* DONT_USE_UPLOADTOBLOB */

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_CORE_H */
