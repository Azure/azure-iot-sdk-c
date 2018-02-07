// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This file is under development and it is subject to change

#ifndef IOTHUB_MESSAGE_MONITORING_H
#define IOTHUB_MESSAGE_MONITORING_H

#ifdef __cplusplus
extern "C"
{
#else
#endif

#include <stdlib.h>
#include "azure_c_shared_utility/umock_c_prod.h"
#include "iothub_message_monitoring_ll.h"

typedef struct IOTHUB_MESSAGE_MONITORING_TAG* IOTHUB_MESSAGE_MONITORING_HANDLE;

/** @brief	Creates a IoT Hub Service Client Messaging handle for use it in consequent APIs.
*
* @param	serviceClientHandle	Service client handle.
*
* @return	A non-NULL @c IOTHUB_MESSAGE_MONITORING_HANDLE value that is used when
* 			invoking other functions for IoT Hub DeviceMethod and @c NULL on failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_MONITORING_HANDLE, IoTHubMessageMonitoring_Create, IOTHUB_SERVICE_CLIENT_AUTH_HANDLE, serviceClientHandle);

/** @brief	Disposes of resources allocated by the IoT Hub Service Client Messaging. 
*
* @param	messagingClientHandle	The handle created by a call to the create function.
*/
MOCKABLE_FUNCTION(, void, IoTHubMessageMonitoring_Destroy, IOTHUB_MESSAGE_MONITORING_HANDLE, messagingClientHandle);

/** @brief	Opens connection to IoTHub.
*
* @param	messagingClientHandle	The handle created by a call to the create function.
* @param	openCompleteCallback  	The callback specified by the user for receiving
* 									confirmation of the connection opened.
* 									The user can specify a @c NULL value here to
* 									indicate that no callback is required.
* @param	userContextCallback		User specified context that will be provided to the
* 									callback. This can be @c NULL.
*
* @return	IOTHUB_MESSAGING_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_MONITORING_RESULT, IoTHubMessageMonitoring_Open, IOTHUB_MESSAGE_MONITORING_HANDLE, messageMonitoringHandle, const char*, consumerGroup, size_t, partitionNumber, IOTHUB_MESSAGE_FILTER*, filter, IOTHUB_MESSAGE_MONITORING_STATE_CHANGED_CALLBACK, stateChangedCallback, void*, userContext);

/** @brief	Closes connection to IoTHub.
*
* @param	messagingClientHandle	The handle created by a call to the create function.
*/
MOCKABLE_FUNCTION(, void, IoTHubMessageMonitoring_Close, IOTHUB_MESSAGE_MONITORING_HANDLE, messagingClientHandle);

/**
* @brief	This API specifies a callback to be used when the service receives a message from the device.
*
* @param	messagingClientHandle		        The handle created by a call to the create function.
* @param	messageReceivedCallback	            The callback specified by the user to be used for receiving messages sent by a device client.
*
* @param	userContextCallback		            User specified context that will be provided to the
* 									            callback. This can be @c NULL.
*
*			@b NOTE: The application behavior is undefined if the user calls
*			the ::IoTHubMessageMonitoring_Destroy or IoTHubMessageMonitoring_Close function from within any callback.
*
* @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_MONITORING_RESULT, IoTHubMessageMonitoring_SetMessageCallback, IOTHUB_MESSAGE_MONITORING_HANDLE, messageMonitoringHandle, IOTHUB_MESSAGE_RECEIVED_CALLBACK, messageReceivedCallback, void*, userContextCallback);

MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_MONITORING_RESULT, IoTHubMessageMonitoring_SetOption, IOTHUB_MESSAGE_MONITORING_HANDLE, messagingHandle, const char*, name, const void*, value);

#ifdef __cplusplus
}
#endif

#endif // IOTHUB_MESSAGE_MONITORING_H
