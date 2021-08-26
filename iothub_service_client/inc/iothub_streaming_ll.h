// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_STREAMING_LL_H
#define IOTHUB_STREAMING_LL_H

#include "iothub_service_client_auth.h"
#include "umock_c/umock_c_prod.h"
#include "azure_macro_utils/macro_utils.h"
#include "iothub_client_streaming.h"

#ifdef __cplusplus
extern "C"
{
#else
#endif

#define IOTHUB_STREAMING_RESULT_VALUES       \
    IOTHUB_STREAMING_OK,                     \
    IOTHUB_STREAMING_INVALID_ARG,            \
    IOTHUB_STREAMING_ERROR                   \

DEFINE_ENUM(IOTHUB_STREAMING_RESULT, IOTHUB_STREAMING_RESULT_VALUES);

typedef struct IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE_TAG* IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE;

typedef void(*IOTHUB_OPEN_COMPLETE_CALLBACK)(void* context);

/** @brief	Creates an IoT Hub Service Client Streaming Low-Layer handle.
*
* @param	serviceClientAuthHandle	Service client authentication handle.
*
* @return	A non-NULL @c IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE value that is used when
* 			invoking other functions for IoT Hub Streaming, or @c NULL if any failure occurs.
*/
MOCKABLE_FUNCTION(, IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE, IoTHubStreaming_LL_Create, IOTHUB_SERVICE_CLIENT_AUTH_HANDLE, serviceClientAuthHandle);

/** @brief	Disposes of resources allocated by the IoT Hub Service Client Streaming Low Layer handle.
*
* @param	clientHandle	The IoT Hub Service Client Streaming Low-Layer handle.
*/
MOCKABLE_FUNCTION(, void, IoTHubStreaming_LL_Destroy, IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE, clientHandle);

/** @brief	Opens connection to IoT Hub.
*
* @param	clientHandle	        The IoT Hub Service Client Streaming Low-Layer handle.
*
* @param	openCompleteCallback  	Callback specified for receiving confirmation that the connection has been opened.
* 									A @c NULL value can be specific here to indicate that no callback is required.
* @param	userContextCallback		User-specified context that will be provided to the callback. This can be @c NULL.
*
* @return	IOTHUB_STREAMING_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_STREAMING_RESULT, IoTHubStreaming_LL_Open, IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE, clientHandle, IOTHUB_OPEN_COMPLETE_CALLBACK, openCompleteCallback, const void*, userContextCallback);

/** @brief	Closes connection to IoT Hub.
*
* @param	clientHandle	        The IoT Hub Service Client Streaming Low-Layer handle.
*/
MOCKABLE_FUNCTION(, void, IoTHubStreaming_LL_Close, IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE, clientHandle);

/** @brief	Sends a request to a specific device client for establishing a new streaming connection.
*
* @param	clientHandle	        The IoT Hub Service Client Streaming Low-Layer handle.
*
* @param	deviceId  	            ID of the device that will receive the streaming request.
*
* @param	streamRequestInfo		Information necessary to send the stream request.
*
* @param    streamRequestCallback   Callback invoked when a response from the device client is received.
*                                   The callback carries the the response if the device client has accepted or rejected the streaming request,
*                                   any custom data it might have sent as well as the information necessary by this local endpoint to establish
*                                   connection to the device streaming gateway.
*
* @param    context                 User-specified context that will be provided to the callback. This can be @c NULL.
*
* @return	IOTHUB_STREAMING_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_STREAMING_RESULT, IotHubStreaming_LL_SendStreamRequestAsync, IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE, clientHandle, const char*, deviceId, STREAM_REQUEST_INFO*, streamRequestInfo, IOTHUB_CLIENT_OUTGOING_STREAM_REQUEST_CALLBACK, streamRequestCallback, const void*, context);

/**
* @brief	This function must be called by the user frequently (sug, every 100 milliseconds)
*           so I/O and callback dispatching can be performed by the IoT Hub Streaming Client.
*
* @param	clientHandle	    The IoT Hub Service Client Streaming Low-Layer handle.
*
*/
MOCKABLE_FUNCTION(, void, IoTHubStreaming_LL_DoWork, IOTHUB_STREAMING_LL_SVC_CLIENT_HANDLE, clientHandle);

#ifdef __cplusplus
}
#endif

#endif // IOTHUB_STREAMING_LL_H
