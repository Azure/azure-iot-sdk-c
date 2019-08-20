// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "iothub_client_streaming.h"

void stream_c2d_response_destroy(DEVICE_STREAM_C2D_RESPONSE* response)
{
    // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_021: [ If `request` is NULL, the function shall return ]
    if (response != NULL)
    {
        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_022: [ The memory allocated for `response` shall be released ]
        if (response->request_id != NULL)
        {
            free(response->request_id);
        }

        free(response);
    }
}

void stream_c2d_request_destroy(DEVICE_STREAM_C2D_REQUEST* request)
{
    // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_023: [ If `request` is NULL, the function shall return ]
    if (request != NULL)
    {
        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_024: [ The memory allocated for `request` shall be released ]
        if (request->request_id != NULL)
        {
            free(request->request_id);
        }

        if (request->name != NULL)
        {
            free(request->name);
        }

        if (request->url != NULL)
        {
            free(request->url);
        }

        if (request->authorization_token != NULL)
        {
            free(request->authorization_token);
        }

        free(request);
    }
}


DEVICE_STREAM_C2D_REQUEST* stream_c2d_request_clone(DEVICE_STREAM_C2D_REQUEST* request)
{
    DEVICE_STREAM_C2D_REQUEST* result;

    // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_016: [ If `request` is NULL, the function shall return NULL ]
    if (request == NULL)
    {
        LogError("Invalid argument (request is NULL)");
        result = NULL;
    }
    // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_017: [ The function shall allocate memory for a new instance of DEVICE_STREAM_C2D_REQUEST (aka `clone`) ]
    else if ((result = (DEVICE_STREAM_C2D_REQUEST*)malloc(sizeof(DEVICE_STREAM_C2D_REQUEST))) == NULL)
    {
        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_018: [ If malloc fails, the function shall return NULL ]
        LogError("Failed allocating request");
    }
    else
    {
        memset(result, 0, sizeof(DEVICE_STREAM_C2D_REQUEST));

        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_019: [ All fields of `request` shall be copied into `clone` ]
        if (request->request_id != NULL && mallocAndStrcpy_s(&result->request_id, request->request_id) != 0)
        {
            LogError("Failed cloning request id");
            // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_020: [ If any field fails to be copied, the function shall release the memory allocated and return NULL ]
            stream_c2d_request_destroy(result);
            result = NULL;
        }
        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_019: [ All fields of `request` shall be copied into `clone` ]
        else if (request->name != NULL && mallocAndStrcpy_s(&result->name, request->name) != 0)
        {
            LogError("Failed cloning name");
            // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_020: [ If any field fails to be copied, the function shall release the memory allocated and return NULL ]
            stream_c2d_request_destroy(result);
            result = NULL;
        }
        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_019: [ All fields of `request` shall be copied into `clone` ]
        else if (request->url != NULL && mallocAndStrcpy_s(&result->url, request->url) != 0)
        {
            LogError("Failed cloning uri");
            // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_020: [ If any field fails to be copied, the function shall release the memory allocated and return NULL ]
            stream_c2d_request_destroy(result);
            result = NULL;
        }
        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_019: [ All fields of `request` shall be copied into `clone` ]
        else if (request->authorization_token != NULL && mallocAndStrcpy_s(&result->authorization_token, request->authorization_token) != 0)
        {
            LogError("Failed cloning authorization token");
            // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_020: [ If any field fails to be copied, the function shall release the memory allocated and return NULL ]
            stream_c2d_request_destroy(result);
            result = NULL;
        }
    }

    return result;
}

DEVICE_STREAM_C2D_RESPONSE* stream_c2d_response_create(DEVICE_STREAM_C2D_REQUEST* request, bool accept)
{
    DEVICE_STREAM_C2D_RESPONSE* result;

    // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_010: [ If `request` is NULL, the function shall return NULL ]
    if (request == NULL)
    {
        LogError("Invalid argument (request=NULL)");
        result = NULL;
    }
    // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_011: [ The function shall allocate memory for a new instance of DEVICE_STREAM_C2D_RESPONSE (aka `response`) ]
    else if ((result = (DEVICE_STREAM_C2D_RESPONSE*)malloc(sizeof(DEVICE_STREAM_C2D_RESPONSE))) == NULL)
    {
        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_012: [ If malloc fails, the function shall return NULL ]
        LogError("Failed allocating DEVICE_STREAM_C2D_RESPONSE");
    }
    else
    {
        memset(result, 0, sizeof(DEVICE_STREAM_C2D_RESPONSE));

        // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_013: [ `request->request_id` shall be copied into `response->request_id` ]
        if (request->request_id != NULL && mallocAndStrcpy_s(&result->request_id, request->request_id) != 0)
        {
            LogError("Failed to copy request id");
            // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_015: [ If any values fail to be copied, the function shall release the memory allocated and return NULL ]
            stream_c2d_response_destroy(result);
            result = NULL;
        }
        else
        {
            // Codes_SRS_IOTHUB_CLIENT_STREAMING_09_015: [ If any values fail to be copied, the function shall release the memory allocated and return NULL ]
            result->accept = accept;
        }
    }

    return result;
}
