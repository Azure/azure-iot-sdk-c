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
#include "iothub_client_ll.h"
#include "iothub_transport_ll.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_private.h"
#include "iothub_client_options.h"
#include "iothub_client_version.h"
#include "iothub_client_diagnostic.h"
#include <stdint.h>

#ifndef DONT_USE_UPLOADTOBLOB
#include "iothub_client_ll_uploadtoblob.h"
#endif

IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    return (IOTHUB_CLIENT_LL_HANDLE)IoTHubClientCore_LL_CreateFromConnectionString(connectionString, protocol);
}

IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_Create(const IOTHUB_CLIENT_CONFIG* config)
{
    return (IOTHUB_CLIENT_LL_HANDLE)IoTHubClientCore_LL_Create(config);
}

IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateWithTransport(const IOTHUB_CLIENT_DEVICE_CONFIG * config)
{
    return (IOTHUB_CLIENT_LL_HANDLE)IoTHubClientCore_LL_CreateWithTransport(config);
}

IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateFromDeviceAuth(const char* iothub_uri, const char* device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    return (IOTHUB_CLIENT_LL_HANDLE)IoTHubClientCore_LL_CreateFromDeviceAuth(iothub_uri, device_id, protocol);
}

void IoTHubClient_LL_Destroy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
{
    IoTHubClientCore_LL_Destroy((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendEventAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SendEventAsync((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetSendStatus(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    return IoTHubClientCore_LL_GetSendStatus((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, iotHubClientStatus);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetMessageCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SetMessageCallback((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, messageCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetConnectionStatusCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void * userContextCallback)
{
    return IoTHubClientCore_LL_SetConnectionStatusCallback((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, connectionStatusCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    return IoTHubClientCore_LL_SetRetryPolicy((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, retryPolicy, retryTimeoutLimitInSeconds);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitInSeconds)
{
    return IoTHubClientCore_LL_GetRetryPolicy((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, retryPolicy, retryTimeoutLimitInSeconds);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetLastMessageReceiveTime(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime)
{
    return IoTHubClientCore_LL_GetLastMessageReceiveTime((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, lastMessageReceiveTime);
}

void IoTHubClient_LL_DoWork(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
{
    IoTHubClientCore_LL_DoWork((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceTwinCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SetDeviceTwinCallback((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, deviceTwinCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetOption(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value)
{
    return IoTHubClientCore_LL_SetOption((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, optionName, value);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendReportedState(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SendReportedState((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, reportedState, size, reportedStateCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceMethodCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SetDeviceMethodCallback((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, deviceMethodCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SetDeviceMethodCallback_Ex((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, inboundDeviceMethodCallback, userContextCallback);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_DeviceMethodResponse(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    return IoTHubClientCore_LL_DeviceMethodResponse((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, methodId, response, response_size, status_response);
}

#ifndef DONT_USE_UPLOADTOBLOB
IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* destinationFileName, const unsigned char* source, size_t size)
{
    return IoTHubClientCore_LL_UploadToBlob((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, destinationFileName, source, size);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadMultipleBlocksToBlob(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context)
{
    return IoTHubClientCore_LL_UploadMultipleBlocksToBlob((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, destinationFileName, getDataCallback, context);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadMultipleBlocksToBlobEx(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx, void* context)
{
    return IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx((IOTHUB_CLIENT_CORE_LL_HANDLE)iotHubClientHandle, destinationFileName, getDataCallbackEx, context);
}

#endif

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendEventToOutputAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if ((iotHubClientHandle == NULL) || (outputName == NULL) || (eventMessageHandle == NULL) || ((eventConfirmationCallback == NULL) && (userContextCallback != NULL)))
    {
        // Codes_SRS_IOTHUBCLIENT_LL_31_127: [ If `iotHubClientHandle`, `outputName`, or `eventConfirmationCallback` is `NULL`, `IoTHubClient_LL_SendEventToOutputAsync` shall return `IOTHUB_CLIENT_INVALID_ARG`. ]
        LogError("Invalid argument (iotHubClientHandle=%p, outputName=%p, eventMessageHandle=%p)", iotHubClientHandle, outputName, eventMessageHandle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        // Codes_SRS_IOTHUBCLIENT_LL_31_128: [ `IoTHubClient_LL_SendEventToOutputAsync` shall set the outputName of the message to send. ]
        if (IoTHubMessage_SetOutputName(eventMessageHandle, outputName) != IOTHUB_MESSAGE_OK)
        {
            LogError("IoTHubMessage_SetOutputName failed");
            result = IOTHUB_CLIENT_ERROR;
        }
        // Codes_SRS_IOTHUBCLIENT_LL_31_129: [ `IoTHubClient_LL_SendEventToOutputAsync` shall invoke `IoTHubClient_LL_SendEventAsync` to send the message. ]
        else if ((result = IoTHubClient_LL_SendEventAsync(iotHubClientHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
        {
            LogError("Call into IoTHubClient_LL_SendEventAsync failed, result=%d", result);
        }
    }

    return result;
}


static IOTHUB_CLIENT_RESULT create_event_handler_callback(IOTHUB_CLIENT_LL_HANDLE_DATA* handleData, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC callbackSync, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX callbackSyncEx, void* userContextCallback, void* userContextCallbackEx, size_t userContextCallbackExLength)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;
    bool add_to_list = false;

    if ((handleData->event_callbacks == NULL) && ((handleData->event_callbacks = singlylinkedlist_create()) == NULL))
    {
        LogError("Could not allocate linked list for callbacks");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        IOTHUB_EVENT_CALLBACK* event_callback = NULL;
        LIST_ITEM_HANDLE item_handle = singlylinkedlist_find(handleData->event_callbacks, is_event_equal_for_match, (const void*)inputName);
        if (item_handle == NULL)
        {
            // Codes_SRS_IOTHUBCLIENT_LL_31_134: [ `IoTHubClient_LL_SetInputMessageCallback` shall allocate a callback handle to associate callbacks from the transport => client if `inputName` isn't already present in the callback list. ]
            event_callback = (IOTHUB_EVENT_CALLBACK*)malloc(sizeof(IOTHUB_EVENT_CALLBACK));
            if (event_callback == NULL)
            {
                LogError("Could not allocate IOTHUB_EVENT_CALLBACK");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                memset(event_callback, 0, sizeof(*event_callback));
                add_to_list = true;
            }
        }
        else
        {
            // Codes_SRS_IOTHUBCLIENT_LL_31_135: [ `IoTHubClient_LL_SetInputMessageCallback` shall reuse the existing callback handle if `inputName` is already present in the callback list. ]
            event_callback = (IOTHUB_EVENT_CALLBACK*)singlylinkedlist_item_get_value(item_handle);
            if (event_callback == NULL)
            {
                LogError("singlylinkedlist_item_get_value failed looking up event callback");
            }
        }

        if (event_callback != NULL)
        {
            if (event_callback->inputName == NULL)
            {
                event_callback->inputName = STRING_construct(inputName);
            }

            if (event_callback->inputName != NULL)
            {
                event_callback->callbackAsync = callbackSync;
                event_callback->callbackAsyncEx = callbackSyncEx;

                free(event_callback->userContextCallbackEx);
                event_callback->userContextCallbackEx = NULL;

                if (userContextCallbackEx == NULL)
                {
                    event_callback->userContextCallback = userContextCallback;
                }

                if ((userContextCallbackEx != NULL) && 
                    (NULL == (event_callback->userContextCallbackEx = malloc(userContextCallbackExLength))))
                {
                    LogError("Unable to allocate userContextCallback");
                    delete_event(event_callback);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if ((add_to_list == true) && (NULL == singlylinkedlist_add(handleData->event_callbacks, event_callback)))
                {
                    LogError("Unable to add eventCallback to list");
                    delete_event(event_callback);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    if (userContextCallbackEx != NULL)
                    {
                        // Codes_SRS_IOTHUBCLIENT_LL_31_141: [`IoTHubClient_LL_SetInputMessageCallbackEx` shall copy the data passed in extended context. ]
                        memcpy(event_callback->userContextCallbackEx, userContextCallbackEx, userContextCallbackExLength);
                    }
                    result = IOTHUB_CLIENT_OK;
                }
            }
            else
            {
                delete_event(event_callback);
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }

    return result;
}

static IOTHUB_CLIENT_RESULT remove_event_unsubscribe_if_needed(IOTHUB_CLIENT_LL_HANDLE_DATA* handleData, const char* inputName)
{
    IOTHUB_CLIENT_RESULT result;

    LIST_ITEM_HANDLE item_handle = singlylinkedlist_find(handleData->event_callbacks, is_event_equal_for_match, (const void*)inputName);
    if (item_handle == NULL)
    {
        // Codes_SRS_IOTHUBCLIENT_LL_31_132: [ If `eventHandlerCallback` is NULL, `IoTHubClient_LL_SetInputMessageCallback` shall return `IOTHUB_CLIENT_ERROR` if the `inputName` is not present. ]
        LogError("Input name %s was not present", inputName);
        result = IOTHUB_CLIENT_ERROR;    
    }
    else
    {
        IOTHUB_EVENT_CALLBACK* event_callback = (IOTHUB_EVENT_CALLBACK*)singlylinkedlist_item_get_value(item_handle);
        if (event_callback == NULL)
        {
            LogError("singlylinkedlist_item_get_value failed");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            delete_event(event_callback);
            // Codes_SRS_IOTHUBCLIENT_LL_31_131: [ If `eventHandlerCallback` is NULL, `IoTHubClient_LL_SetInputMessageCallback` shall remove the `inputName` from its callback list if present. ]
            if (singlylinkedlist_remove(handleData->event_callbacks, item_handle) != 0)
            {
                LogError("singlylinkedlist_remove failed");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (singlylinkedlist_get_head_item(handleData->event_callbacks) == NULL)
                {
                    // Codes_SRS_IOTHUBCLIENT_LL_31_133: [ If `eventHandlerCallback` is NULL, `IoTHubClient_LL_SetInputMessageCallback` shall invoke `IoTHubTransport_Unsubscribe_InputQueue` if this was the last input callback. ]
                    handleData->IoTHubTransport_Unsubscribe_InputQueue(handleData);
                }
                result = IOTHUB_CLIENT_OK;
            }
        }
    }

    return result;
}


IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetInputMessageCallbackImpl(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX eventHandlerCallbackEx, void *userContextCallback, void *userContextCallbackEx, size_t userContextCallbackExLength)
{
    IOTHUB_CLIENT_RESULT result;

    if ((iotHubClientHandle == NULL) || (inputName == NULL))
    {
        // Codes_SRS_IOTHUBCLIENT_LL_31_130: [ If `iotHubClientHandle` or `inputName` is NULL, `IoTHubClient_LL_SetInputMessageCallback` shall return IOTHUB_CLIENT_INVALID_ARG. ]
        LogError("Invalid argument - iotHubClientHandle=%p, inputName=%p", iotHubClientHandle, inputName);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        if ((eventHandlerCallback == NULL) && (eventHandlerCallbackEx == NULL))
        {
            result = (IOTHUB_CLIENT_RESULT)remove_event_unsubscribe_if_needed(handleData, inputName);
        }
        else
        {
            bool registered_with_transport_handler = (handleData->event_callbacks != NULL) && (singlylinkedlist_get_head_item(handleData->event_callbacks) != NULL);
            if ((result = (IOTHUB_CLIENT_RESULT)create_event_handler_callback(handleData, inputName, eventHandlerCallback, eventHandlerCallbackEx, userContextCallback, userContextCallbackEx, userContextCallbackExLength)) != IOTHUB_CLIENT_OK)
            {
                LogError("create_event_handler_callback call failed, error = %d", result);
            }
            // Codes_SRS_IOTHUBCLIENT_LL_31_136: [ `IoTHubClient_LL_SetInputMessageCallback` shall invoke `IoTHubTransport_Subscribe_InputQueue` if this is the first callback being registered. ]
            else if (!registered_with_transport_handler && (handleData->IoTHubTransport_Subscribe_InputQueue(handleData->deviceHandle) != 0))
            {
                LogError("IoTHubTransport_Subscribe_InputQueue failed");
                delete_event_callback_list(handleData);
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
    }
    return result;

}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetInputMessageCallbackEx(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX eventHandlerCallbackEx, void *userContextCallbackEx, size_t userContextCallbackExLength)
{
    return IoTHubClient_LL_SetInputMessageCallbackImpl(iotHubClientHandle, inputName, NULL, eventHandlerCallbackEx, NULL, userContextCallbackEx, userContextCallbackExLength);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetInputMessageCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback)
{
    return IoTHubClient_LL_SetInputMessageCallbackImpl(iotHubClientHandle, inputName, eventHandlerCallback, NULL, userContextCallback, NULL, 0);
}

IOTHUB_CLIENT_RESULT IoTHubClient_SendEventToOutputAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if ((iotHubClientHandle == NULL) || (outputName == NULL) || (eventMessageHandle == NULL))
    {
        // Codes_SRS_IOTHUBCLIENT_31_100: [ If `iotHubClientHandle`, `outputName`, or `eventConfirmationCallback` is `NULL`, `IoTHubClient_SendEventToOutputAsync` shall return `IOTHUB_CLIENT_INVALID_ARG`. ]
        LogError("Invalid argument (iotHubClientHandle=%p, outputName=%p, eventMessageHandle=%p)", iotHubClientHandle, outputName, eventMessageHandle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        // Codes_SRS_IOTHUBCLIENT_31_101: [ `IoTHubClient_SendEventToOutputAsync` shall set the outputName of the message to send. ]
        if (IoTHubMessage_SetOutputName(eventMessageHandle, outputName) != IOTHUB_MESSAGE_OK)
        {
            LogError("IoTHubMessage_SetOutputName failed");
            result = IOTHUB_CLIENT_ERROR;
        }
        // Codes_SRS_IOTHUBCLIENT_31_102: [ `IoTHubClient_SendEventToOutputAsync` shall invoke `IoTHubClient_SendEventAsync` to send the message. ]
        else if ((result = IoTHubClient_SendEventAsync(iotHubClientHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
        {
            LogError("Call into IoTHubClient_SendEventAsync failed, result=%d", result);
        }
    }

    return result;
}


IOTHUB_CLIENT_RESULT IoTHubClient_SetInputMessageCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    // Codes_SRS_IOTHUBCLIENT_31_097: [ If iotHubClientHandle or inputName is NULL, `IoTHubClient_SetInputMessageCallback` shall return IOTHUB_CLIENT_INVALID_ARG. ]
    if ((iotHubClientHandle == NULL) || (inputName == NULL))
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("NULL iothubClientHandle");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        // Codes_SRS_IOTHUBCLIENT_31_098: [ `IoTHubClient_SetMessageCallback` shall start the worker thread if it was not previously started. ]
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                // Codes_SRS_IOTHUBCLIENT_31_099: [ `IoTHubClient_SetMessageCallback` shall call `IoTHubClient_LL_SetInputMessageCallback`, passing its input arguments ]
                IOTHUB_INPUTMESSAGE_CALLBACK_CONTEXT inputMessageCallbackContext;
                inputMessageCallbackContext.iotHubClientHandle = iotHubClientHandle;
                inputMessageCallbackContext.eventHandlerCallback = eventHandlerCallback;
                inputMessageCallbackContext.userContextCallback = userContextCallback;
                
                result = IoTHubClient_LL_SetInputMessageCallbackEx(iotHubClientInstance->IoTHubClientLLHandle, inputName, iothub_ll_inputmessage_callback, (void*)&inputMessageCallbackContext, sizeof(inputMessageCallbackContext));
                (void)Unlock(iotHubClientInstance->LockHandle);
            }
        }
    }

    return result;
}


