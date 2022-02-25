// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"

#include <stdbool.h>
#include "iotdevice.h"
#include "datapublisher.h"
#include "commanddecoder.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"

#define LOG_DEVICE_ERROR \
    LogError("(result = %s)", MU_ENUM_TO_STRING(DEVICE_RESULT, result))



typedef struct DEVICE_HANDLE_DATA_TAG
{
    SCHEMA_MODEL_TYPE_HANDLE model;
    DATA_PUBLISHER_HANDLE dataPublisherHandle;
    pfDeviceActionCallback deviceActionCallback;
    void* callbackUserContext;
    pfDeviceMethodCallback deviceMethodCallback;
    void* methodCallbackUserContext;

    COMMAND_DECODER_HANDLE commandDecoderHandle;
} DEVICE_HANDLE_DATA;

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_RESULT, DEVICE_RESULT_VALUES);

static EXECUTE_COMMAND_RESULT DeviceInvokeAction(void* actionCallbackContext, const char* relativeActionPath, const char* actionName, size_t argCount, const AGENT_DATA_TYPE* args)
{
    EXECUTE_COMMAND_RESULT result;

    if (actionCallbackContext == NULL)
    {
        result = EXECUTE_COMMAND_ERROR;
        LogError("(Error code = %s)", MU_ENUM_TO_STRING(DEVICE_RESULT, DEVICE_INVALID_ARG));
    }
    else
    {
        DEVICE_HANDLE_DATA* device = (DEVICE_HANDLE_DATA*)actionCallbackContext;

        result = device->deviceActionCallback((DEVICE_HANDLE)device, device->callbackUserContext, relativeActionPath, actionName, argCount, args);
    }

    return result;
}

static METHODRETURN_HANDLE DeviceInvokeMethod(void* methodCallbackContext, const char* relativeMethodPath, const char* methodName, size_t argCount, const AGENT_DATA_TYPE* args)
{
    METHODRETURN_HANDLE result;

    if (methodCallbackContext == NULL)
    {
        result = NULL;
        LogError("(Error code = %s)", MU_ENUM_TO_STRING(DEVICE_RESULT, DEVICE_INVALID_ARG));
    }
    else
    {
        DEVICE_HANDLE_DATA* device = (DEVICE_HANDLE_DATA*)methodCallbackContext;

        result = device->deviceMethodCallback((DEVICE_HANDLE)device, device->methodCallbackUserContext, relativeMethodPath, methodName, argCount, args);
    }

    return result;
}

DEVICE_RESULT Device_Create(SCHEMA_MODEL_TYPE_HANDLE modelHandle, pfDeviceActionCallback deviceActionCallback, void* callbackUserContext, pfDeviceMethodCallback methodCallback, void* methodCallbackContext, bool includePropertyPath, DEVICE_HANDLE* deviceHandle)
{
    DEVICE_RESULT result;

    if (modelHandle == NULL || deviceHandle == NULL || deviceActionCallback == NULL || methodCallback == NULL || methodCallbackContext == NULL)
    {
        result = DEVICE_INVALID_ARG;
        LOG_DEVICE_ERROR;
    }
    else
    {
        DEVICE_HANDLE_DATA* device = (DEVICE_HANDLE_DATA*)malloc(sizeof(DEVICE_HANDLE_DATA));
        if (device == NULL)
        {
            result = DEVICE_ERROR;
            LOG_DEVICE_ERROR;
        }
        else
        {
            if ((device->dataPublisherHandle = DataPublisher_Create(modelHandle, includePropertyPath)) == NULL)
            {
                free(device);

                result = DEVICE_DATA_PUBLISHER_FAILED;
                LOG_DEVICE_ERROR;
            }
            else
            {
                device->model = modelHandle;
                device->deviceActionCallback = deviceActionCallback;
                device->callbackUserContext = callbackUserContext;
                device->deviceMethodCallback = methodCallback;
                device->methodCallbackUserContext = methodCallbackContext;

                if ((device->commandDecoderHandle = CommandDecoder_Create(modelHandle, DeviceInvokeAction, device, DeviceInvokeMethod, device)) == NULL)
                {
                    DataPublisher_Destroy(device->dataPublisherHandle);
                    free(device);

                    result = DEVICE_COMMAND_DECODER_FAILED;
                    LOG_DEVICE_ERROR;
                }
                else
                {
                    *deviceHandle = (DEVICE_HANDLE)device;
                    result = DEVICE_OK;
                }
            }
        }
    }

    return result;
}

void Device_Destroy(DEVICE_HANDLE deviceHandle)
{
    if (deviceHandle != NULL)
    {
        DEVICE_HANDLE_DATA* device = (DEVICE_HANDLE_DATA*)deviceHandle;

        DataPublisher_Destroy(device->dataPublisherHandle);
        CommandDecoder_Destroy(device->commandDecoderHandle);

        free(device);
    }
}

TRANSACTION_HANDLE Device_StartTransaction(DEVICE_HANDLE deviceHandle)
{
    TRANSACTION_HANDLE result;

    if (deviceHandle == NULL)
    {
        result = NULL;
        LogError("(Error code = %s)", MU_ENUM_TO_STRING(DEVICE_RESULT, DEVICE_INVALID_ARG));
    }
    else
    {
        DEVICE_HANDLE_DATA* deviceInstance = (DEVICE_HANDLE_DATA*)deviceHandle;

        result = DataPublisher_StartTransaction(deviceInstance->dataPublisherHandle);
    }

    return result;
}

DEVICE_RESULT Device_PublishTransacted(TRANSACTION_HANDLE transactionHandle, const char* propertyPath, const AGENT_DATA_TYPE* data)
{
    DEVICE_RESULT result;

    if (
        (transactionHandle == NULL) ||
        (propertyPath == NULL) ||
        (data == NULL)
       )
    {
        result = DEVICE_INVALID_ARG;
        LOG_DEVICE_ERROR;
    }
    else if (DataPublisher_PublishTransacted(transactionHandle, propertyPath, data) != DATA_PUBLISHER_OK)
    {
        result = DEVICE_DATA_PUBLISHER_FAILED;
        LOG_DEVICE_ERROR;
    }
    else
    {
        result = DEVICE_OK;
    }

    return result;
}

DEVICE_RESULT Device_EndTransaction(TRANSACTION_HANDLE transactionHandle, unsigned char** destination, size_t* destinationSize)
{
    DEVICE_RESULT result;

    if (
        (transactionHandle == NULL) ||
        (destination == NULL) ||
        (destinationSize == NULL)
        )
    {
        result = DEVICE_INVALID_ARG;
        LOG_DEVICE_ERROR;
    }
    else if (DataPublisher_EndTransaction(transactionHandle, destination, destinationSize) != DATA_PUBLISHER_OK)
    {
        result = DEVICE_DATA_PUBLISHER_FAILED;
        LOG_DEVICE_ERROR;
    }
    else
    {
        result = DEVICE_OK;
    }

    return result;
}

DEVICE_RESULT Device_CancelTransaction(TRANSACTION_HANDLE transactionHandle)
{
    DEVICE_RESULT result;

    if (transactionHandle == NULL)
    {
        result = DEVICE_INVALID_ARG;
        LOG_DEVICE_ERROR;
    }
    else if (DataPublisher_CancelTransaction(transactionHandle) != DATA_PUBLISHER_OK)
    {
        result = DEVICE_DATA_PUBLISHER_FAILED;
        LOG_DEVICE_ERROR;
    }
    else
    {
        result = DEVICE_OK;
    }

    return result;
}

EXECUTE_COMMAND_RESULT Device_ExecuteCommand(DEVICE_HANDLE deviceHandle, const char* command)
{
    EXECUTE_COMMAND_RESULT result;
    if (
        (deviceHandle == NULL) ||
        (command == NULL)
        )
    {
        result = EXECUTE_COMMAND_ERROR;
        LogError("invalid parameter (NULL passed to Device_ExecuteCommand DEVICE_HANDLE deviceHandle=%p, const char* command=%p", deviceHandle, command);
    }
    else
    {
        DEVICE_HANDLE_DATA* device = (DEVICE_HANDLE_DATA*)deviceHandle;
        result = CommandDecoder_ExecuteCommand(device->commandDecoderHandle, command);
    }
    return result;
}

METHODRETURN_HANDLE Device_ExecuteMethod(DEVICE_HANDLE deviceHandle, const char* methodName, const char* methodPayload)
{
    METHODRETURN_HANDLE result;
    if (
        (deviceHandle == NULL) ||
        (methodName == NULL) /*methodPayload can be NULL*/
        )
    {
        result = NULL;
        LogError("invalid parameter (NULL passed to Device_ExecuteMethod DEVICE_HANDLE deviceHandle=%p, const char* methodPayload=%p", deviceHandle, methodPayload);
    }
    else
    {
        DEVICE_HANDLE_DATA* device = (DEVICE_HANDLE_DATA*)deviceHandle;
        result = CommandDecoder_ExecuteMethod(device->commandDecoderHandle, methodName, methodPayload);
    }
    return result;
}

REPORTED_PROPERTIES_TRANSACTION_HANDLE Device_CreateTransaction_ReportedProperties(DEVICE_HANDLE deviceHandle)
{
    REPORTED_PROPERTIES_TRANSACTION_HANDLE result;

    if (deviceHandle == NULL)
    {
        LogError("invalid argument DEVICE_HANDLE deviceHandle=%p", deviceHandle);
        result = NULL;
    }
    else
    {
        DEVICE_HANDLE_DATA* deviceInstance = (DEVICE_HANDLE_DATA*)deviceHandle;
        result = DataPublisher_CreateTransaction_ReportedProperties(deviceInstance->dataPublisherHandle);
        if (result == NULL)
        {
            LogError("unable to DataPublisher_CreateTransaction_ReportedProperties");
            /*return as is*/
        }
        else
        {
            /*return as is*/
        }
    }

    return result;
}

DEVICE_RESULT Device_PublishTransacted_ReportedProperty(REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle, const char* reportedPropertyPath, const AGENT_DATA_TYPE* data)
{
    DEVICE_RESULT result;
    if (
        (transactionHandle == NULL) ||
        (reportedPropertyPath == NULL) ||
        (data == NULL)
        )
    {
        LogError("invalid argument REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle=%p, const char* reportedPropertyPath=%s, const AGENT_DATA_TYPE* data=%p", transactionHandle, reportedPropertyPath, data);
        result = DEVICE_INVALID_ARG;
    }
    else
    {
        DATA_PUBLISHER_RESULT r = DataPublisher_PublishTransacted_ReportedProperty(transactionHandle, reportedPropertyPath, data);
        if (r != DATA_PUBLISHER_OK)
        {
            LogError("unable to DataPublisher_PublishTransacted_ReportedProperty");
            result = DEVICE_DATA_PUBLISHER_FAILED;
        }
        else
        {
            result = DEVICE_OK;
        }
    }
    return result;
}

DEVICE_RESULT Device_CommitTransaction_ReportedProperties(REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle, unsigned char** destination, size_t* destinationSize)
{
    DEVICE_RESULT result;

    if (
        (transactionHandle == NULL) ||
        (destination == NULL) ||
        (destinationSize == NULL)
        )
    {
        LogError("invalid argument REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle=%p, unsigned char** destination=%p, size_t* destinationSize=%p", transactionHandle, destination, destinationSize);
        result = DEVICE_INVALID_ARG;
    }
    else
    {
        DATA_PUBLISHER_RESULT r = DataPublisher_CommitTransaction_ReportedProperties(transactionHandle, destination, destinationSize);

        if (r != DATA_PUBLISHER_OK)
        {
            LogError("unable to DataPublisher_CommitTransaction_ReportedProperties");
            result = DEVICE_DATA_PUBLISHER_FAILED;
        }
        else
        {
            result = DEVICE_OK;
        }
    }
    return result;
}

void Device_DestroyTransaction_ReportedProperties(REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle)
{
    if (transactionHandle == NULL)
    {
        LogError("invalid argument REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle=%p", transactionHandle);
    }
    else
    {
        DataPublisher_DestroyTransaction_ReportedProperties(transactionHandle);
    }
}

DEVICE_RESULT Device_IngestDesiredProperties(void* startAddress, DEVICE_HANDLE deviceHandle, const char* jsonPayload, bool parseDesiredNode)
{
    DEVICE_RESULT result;
    if (
        (deviceHandle == NULL) ||
        (jsonPayload == NULL) ||
        (startAddress == NULL)
        )
    {
        LogError("invalid argument void* startAddress=%p, DEVICE_HANDLE deviceHandle=%p, const char* jsonPayload=%p\n", startAddress, deviceHandle, jsonPayload);
        result = DEVICE_INVALID_ARG;
    }
    else
    {
        DEVICE_HANDLE_DATA* device = (DEVICE_HANDLE_DATA*)deviceHandle;
        if (CommandDecoder_IngestDesiredProperties(startAddress, device->commandDecoderHandle, jsonPayload, parseDesiredNode) != EXECUTE_COMMAND_SUCCESS)
        {
            LogError("failure in CommandDecoder_IngestDesiredProperties");
            result = DEVICE_ERROR;
        }
        else
        {
            result = DEVICE_OK;
        }
    }
    return result;
}
