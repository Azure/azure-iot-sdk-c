// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This file is under development and it is subject to change

#ifndef IOTHUB_MESSAGE_MONITORING_LL_H
#define IOTHUB_MESSAGE_MONITORING_LL_H

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/map.h"
#include "iothub_message.h"
#include "iothub_service_client_auth.h"

#define OPTION_ENABLE_LOGGING "enable-logging"

#ifdef __cplusplus
extern "C"
{
#else
#endif

#define IOTHUB_MESSAGE_MONITORING_RESULT_VALUES       \
    IOTHUB_MESSAGE_MONITORING_OK,                     \
    IOTHUB_MESSAGE_MONITORING_INVALID_ARG,            \
    IOTHUB_MESSAGE_MONITORING_ERROR

DEFINE_ENUM(IOTHUB_MESSAGE_MONITORING_RESULT, IOTHUB_MESSAGE_MONITORING_RESULT_VALUES);

#define IOTHUB_MESSAGE_MONITORING_STATE_VALUES        \
    IOTHUB_MESSAGE_MONITORING_STATE_CLOSED,           \
    IOTHUB_MESSAGE_MONITORING_STATE_OPENING,          \
    IOTHUB_MESSAGE_MONITORING_STATE_OPEN,             \
    IOTHUB_MESSAGE_MONITORING_STATE_ERROR

DEFINE_ENUM(IOTHUB_MESSAGE_MONITORING_STATE, IOTHUB_MESSAGE_MONITORING_STATE_VALUES);

typedef struct IOTHUB_MESSAGE_FILTER_TAG
{
    time_t receiveTimeStartUTC;
} IOTHUB_MESSAGE_FILTER;

typedef struct IOTHUB_MESSAGE_MONITORING_LL_TAG* IOTHUB_MESSAGE_MONITORING_LL_HANDLE;

typedef void(*IOTHUB_MESSAGE_MONITORING_STATE_CHANGED_CALLBACK)(void* context, IOTHUB_MESSAGE_MONITORING_STATE new_state, IOTHUB_MESSAGE_MONITORING_STATE previous_state);
typedef void(*IOTHUB_MESSAGE_RECEIVED_CALLBACK)(void* context, IOTHUB_MESSAGE_HANDLE message);

/** @brief	Creates a IoT Hub Service Client Messaging handle for use it in consequent APIs.
*
* @param	iotHubMessagingServiceClientHandle	Service client handle.
*
* @return	A non-NULL @c IOTHUB_MESSAGE_MONITORING_CLIENT_HANDLE value that is used when
* 			invoking other functions for IoT Hub DeviceMethod and @c NULL on failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_MONITORING_LL_HANDLE, IoTHubMessageMonitoring_LL_Create, IOTHUB_SERVICE_CLIENT_AUTH_HANDLE, iotHubMessagingServiceClientHandle);

/** @brief	Disposes of resources allocated by the IoT Hub Service Client Messaging.
*
* @param	messagingClientHandle	The handle created by a call to the create function.
*/
MOCKABLE_FUNCTION(, void, IoTHubMessageMonitoring_LL_Destroy, IOTHUB_MESSAGE_MONITORING_LL_HANDLE, messagingHandle);

/** @brief	Opens connection to IoTHub.
*
* @param	messagingClientHandle	The handle created by a call to the create function.
* @param	openCompleteCallback  	The callback specified by the user for receiving
* 									confirmation of the connection opened.
* 									The user can specify a @c NULL value here to
* 									indicate that no callback is required.
* @param	userContext             User specified context that will be provided to the
* 									callback. This can be @c NULL.
*
* @return	IOTHUB_MESSAGE_MONITORING_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_MONITORING_RESULT, IoTHubMessageMonitoring_LL_Open, IOTHUB_MESSAGE_MONITORING_LL_HANDLE, messagingHandle, const char*, consumerGroup, size_t, partitionNumber, IOTHUB_MESSAGE_FILTER*, filter, IOTHUB_MESSAGE_MONITORING_STATE_CHANGED_CALLBACK, stateChangedCallback, void*, userContext);

/** @brief	Closes connection to IoTHub.
*
* @param	messagingClientHandle	The handle created by a call to the create function.
*/
MOCKABLE_FUNCTION(, void, IoTHubMessageMonitoring_LL_Close, IOTHUB_MESSAGE_MONITORING_LL_HANDLE, messagingHandle);

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
*			the ::IoTHubMessaging_Destroy or IoTHubMessaging_Close function from within any callback.
*
* @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_MONITORING_RESULT, IoTHubMessageMonitoring_LL_SetMessageCallback, IOTHUB_MESSAGE_MONITORING_LL_HANDLE, messagingHandle, IOTHUB_MESSAGE_RECEIVED_CALLBACK, messageReceivedCallback, void*, userContextCallback);

/**
* @brief	This function is meant to be called by the user when work
* 			(sending/receiving) can be done by the IoTHubServiceClient.
*
* @param	messagingHandle	The handle created by a call to the create function.
*
*			All IoTHubServiceClient interactions (in regards to network traffic
*			and/or user level callbacks) are the effect of calling this
*			function and they take place synchronously inside _DoWork.
*/
MOCKABLE_FUNCTION(, void, IoTHubMessageMonitoring_LL_DoWork, IOTHUB_MESSAGE_MONITORING_LL_HANDLE, messagingHandle);


MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_MONITORING_RESULT, IoTHubMessageMonitoring_LL_SetOption, IOTHUB_MESSAGE_MONITORING_LL_HANDLE, messagingHandle, const char*, name, const void*, value);

#ifdef __cplusplus
}
#endif

#endif // IOTHUB_MESSAGE_MONITORING_LL_H
