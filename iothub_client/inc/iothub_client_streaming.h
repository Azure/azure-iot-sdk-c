// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_CLIENT_STREAMING_H
#define IOTHUB_CLIENT_STREAMING_H

#ifdef __cplusplus
#include <cstdlib>
extern "C"
{
#else
#include <stdlib.h>
#include <stdbool.h>
#endif

#include "umock_c/umock_c_prod.h"
#include "azure_macro_utils/macro_utils.h"

#define DEVICE_STREAM_RESPONSE_CODE_SUCCESS    "204"

typedef struct DEVICE_STREAM_C2D_REQUEST_TAG
{
    /**
    * @brief    Name of the stream. This is a null-terminated string.
    */
    char* name;
    /**
    * @brief    Websockets URL to connect to the streaming gateway. This is a null-terminated string.
    */
    char* url;
    /**
    * @brief    Authorization token to be provided to the streaming gateway upon connection. This is a null-terminated string.
    */
    char* authorization_token;
    /**
    * @brief    Request ID used to correlate requests and responses. Do not modify its value.
    */
    char* request_id;
} DEVICE_STREAM_C2D_REQUEST;

typedef struct DEVICE_STREAM_C2D_RESPONSE_TAG
{
    /**
    * @brief    Indicates if the stream request was accepted or rejected by the local endpoint. Use true to accept, or false to reject.
    */
    bool accept;
    /**
    * @brief    Request ID used to correlate requests and responses. Do not modify its value.
    */
    char* request_id;
} DEVICE_STREAM_C2D_RESPONSE;

/**
* @brief     Callback invoked for new cloud-to-device stream requests.
* @param     request         Contains the basic information to connect to the streaming gateway, as well as optional custom data provided by the originating endpoint.
* @param     context         User-defined context, as provided in the call to *_SetStreamRequestCallback.
* @return    An instance of #DEVICE_STREAM_C2D_RESPONSE indicating if the stream request is accepted or rejected.
*/
typedef DEVICE_STREAM_C2D_RESPONSE* (*DEVICE_STREAM_C2D_REQUEST_CALLBACK)(const DEVICE_STREAM_C2D_REQUEST* request, void* context);

/**
* @brief     Creates a response for a given device streaming request.
* @param     request         Contains the basic information to connect to the streaming gateway, as well as optional custom data provided by the originating endpoint.
* @param     accept          Boolean value that indicates if the device streaming is to be accepted or not by the user.
* @return    An instance of #DEVICE_STREAM_C2D_RESPONSE, containing the accept/reject answer as selected through parameter accept.
*/
MOCKABLE_FUNCTION(, DEVICE_STREAM_C2D_RESPONSE*, IoTHubClient_StreamC2DResponseCreate, const DEVICE_STREAM_C2D_REQUEST*, request, bool, accept);

/**
* @brief     Clones a device streaming request.
* @param     request         The device streaming request to be cloned.
* @return    A deep copy of the device streaming request provided.
*/
MOCKABLE_FUNCTION(, DEVICE_STREAM_C2D_REQUEST*, IoTHubClient_StreamC2DRequestClone, const DEVICE_STREAM_C2D_REQUEST*, request);

/**
* @brief     Destroys a device streaming response instance.
* @param     response        The device streaming response to be destroyed.
* @return    Nothing.
*/
MOCKABLE_FUNCTION(, void, IoTHubClient_StreamC2DResponseDestroy, DEVICE_STREAM_C2D_RESPONSE*, response);

/**
* @brief     Destroys a device streaming request instance.
* @param     request         The device streaming request to be destroyed.
* @return    Nothing.
*/
MOCKABLE_FUNCTION(, void, IoTHubClient_StreamC2DRequestDestroy, DEVICE_STREAM_C2D_REQUEST*, request);

#ifdef __cplusplus
}
#endif

#endif // IOTHUB_CLIENT_STREAMING_H