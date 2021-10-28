// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"

#include <stdbool.h>
#include "datapublisher.h"
#include "jsonencoder.h"
#include "datamarshaller.h"
#include "agenttypesystem.h"
#include "schema.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DATA_PUBLISHER_RESULT, DATA_PUBLISHER_RESULT_VALUES)

#define LOG_DATA_PUBLISHER_ERROR \
    LogError("(result = %s)", MU_ENUM_TO_STRING(DATA_PUBLISHER_RESULT, result))

#define DEFAULT_MAX_BUFFER_SIZE 10240
static size_t maxBufferSize_ = DEFAULT_MAX_BUFFER_SIZE;

typedef struct DATA_PUBLISHER_HANDLE_DATA_TAG
{
    DATA_MARSHALLER_HANDLE DataMarshallerHandle;
    SCHEMA_MODEL_TYPE_HANDLE ModelHandle;
} DATA_PUBLISHER_HANDLE_DATA;

typedef struct TRANSACTION_HANDLE_DATA_TAG
{
    DATA_PUBLISHER_HANDLE_DATA* DataPublisherInstance;
    size_t ValueCount;
    DATA_MARSHALLER_VALUE* Values;
} TRANSACTION_HANDLE_DATA;

typedef struct REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA_TAG
{
    DATA_PUBLISHER_HANDLE_DATA* DataPublisherInstance;
    VECTOR_HANDLE value; /*holds (DATA_MARSHALLER_VALUE*) */
}REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA;

DATA_PUBLISHER_HANDLE DataPublisher_Create(SCHEMA_MODEL_TYPE_HANDLE modelHandle, bool includePropertyPath)
{
    DATA_PUBLISHER_HANDLE_DATA* result;

    if (modelHandle == NULL)
    {
        result = NULL;
        LogError("(result = %s)", MU_ENUM_TO_STRING(DATA_PUBLISHER_RESULT, DATA_PUBLISHER_INVALID_ARG));
    }
    else if ((result = (DATA_PUBLISHER_HANDLE_DATA*)malloc(sizeof(DATA_PUBLISHER_HANDLE_DATA))) == NULL)
    {
        result = NULL;
        LogError("(result = %s)", MU_ENUM_TO_STRING(DATA_PUBLISHER_RESULT, DATA_PUBLISHER_ERROR));
    }
    else
    {
        if ((result->DataMarshallerHandle = DataMarshaller_Create(modelHandle, includePropertyPath)) == NULL)
        {
            free(result);

            result = NULL;
            LogError("(result = %s)", MU_ENUM_TO_STRING(DATA_PUBLISHER_RESULT, DATA_PUBLISHER_MARSHALLER_ERROR));
        }
        else
        {
            result->ModelHandle = modelHandle;
        }
    }

    return result;
}

void DataPublisher_Destroy(DATA_PUBLISHER_HANDLE dataPublisherHandle)
{
    if (dataPublisherHandle != NULL)
    {
        DATA_PUBLISHER_HANDLE_DATA* dataPublisherInstance = (DATA_PUBLISHER_HANDLE_DATA*)dataPublisherHandle;
        DataMarshaller_Destroy(dataPublisherInstance->DataMarshallerHandle);

        free(dataPublisherHandle);
    }
}

TRANSACTION_HANDLE DataPublisher_StartTransaction(DATA_PUBLISHER_HANDLE dataPublisherHandle)
{
    TRANSACTION_HANDLE_DATA* transaction;

    if (dataPublisherHandle == NULL)
    {
        transaction = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(DATA_PUBLISHER_RESULT, DATA_PUBLISHER_INVALID_ARG));
    }
    else
    {
        transaction = (TRANSACTION_HANDLE_DATA*)calloc(1, sizeof(TRANSACTION_HANDLE_DATA));
        if (transaction == NULL)
        {
            LogError("Allocating transaction failed (Error code: %s)", MU_ENUM_TO_STRING(DATA_PUBLISHER_RESULT, DATA_PUBLISHER_ERROR));
        }
        else
        {
            transaction->ValueCount = 0;
            transaction->Values = NULL;
            transaction->DataPublisherInstance = (DATA_PUBLISHER_HANDLE_DATA*)dataPublisherHandle;
        }
    }

    return transaction;
}

DATA_PUBLISHER_RESULT DataPublisher_PublishTransacted(TRANSACTION_HANDLE transactionHandle, const char* propertyPath, const AGENT_DATA_TYPE* data)
{
    DATA_PUBLISHER_RESULT result;
    char* propertyPathCopy;

    if ((transactionHandle == NULL) ||
        (propertyPath == NULL) ||
        (data == NULL))
    {
        result = DATA_PUBLISHER_INVALID_ARG;
        LOG_DATA_PUBLISHER_ERROR;
    }
    else if (mallocAndStrcpy_s(&propertyPathCopy, propertyPath) != 0)
    {
        result = DATA_PUBLISHER_ERROR;
        LOG_DATA_PUBLISHER_ERROR;
    }
    else
    {
        TRANSACTION_HANDLE_DATA* transaction = (TRANSACTION_HANDLE_DATA*)transactionHandle;
        AGENT_DATA_TYPE* propertyValue;

        if (!Schema_ModelPropertyByPathExists(transaction->DataPublisherInstance->ModelHandle, propertyPath))
        {
            free(propertyPathCopy);

            result = DATA_PUBLISHER_SCHEMA_FAILED;
            LOG_DATA_PUBLISHER_ERROR;
        }
        else if ((propertyValue = (AGENT_DATA_TYPE*)calloc(1, sizeof(AGENT_DATA_TYPE))) == NULL)
        {
            free(propertyPathCopy);

            result = DATA_PUBLISHER_ERROR;
            LOG_DATA_PUBLISHER_ERROR;
        }
        else if (Create_AGENT_DATA_TYPE_from_AGENT_DATA_TYPE(propertyValue, data) != AGENT_DATA_TYPES_OK)
        {
            free(propertyPathCopy);
            free(propertyValue);

            result = DATA_PUBLISHER_AGENT_DATA_TYPES_ERROR;
            LOG_DATA_PUBLISHER_ERROR;
        }
        else
        {
            size_t i;
            DATA_MARSHALLER_VALUE* propertySlot = NULL;

            for (i = 0; i < transaction->ValueCount; i++)
            {
                if (strcmp(transaction->Values[i].PropertyPath, propertyPath) == 0)
                {
                    propertySlot = &transaction->Values[i];
                    break;
                }
            }

            if (propertySlot == NULL)
            {
                DATA_MARSHALLER_VALUE* newValues = (DATA_MARSHALLER_VALUE*)realloc(transaction->Values, sizeof(DATA_MARSHALLER_VALUE)* (transaction->ValueCount + 1));
                if (newValues != NULL)
                {
                    transaction->Values = newValues;
                    propertySlot = &transaction->Values[transaction->ValueCount];
                    propertySlot->Value = NULL;
                    propertySlot->PropertyPath = NULL;
                    transaction->ValueCount++;
                }
            }

            if (propertySlot == NULL)
            {
                Destroy_AGENT_DATA_TYPE((AGENT_DATA_TYPE*)propertyValue);
                free(propertyValue);
                free(propertyPathCopy);

                result = DATA_PUBLISHER_ERROR;
                LOG_DATA_PUBLISHER_ERROR;
            }
            else
            {
                if (propertySlot->Value != NULL)
                {
                    Destroy_AGENT_DATA_TYPE((AGENT_DATA_TYPE*)propertySlot->Value);
                    free((AGENT_DATA_TYPE*)propertySlot->Value);
                }
                if (propertySlot->PropertyPath != NULL)
                {
                    char* existingValue = (char*)propertySlot->PropertyPath;
                    free(existingValue);
                }

                propertySlot->PropertyPath = propertyPathCopy;
                propertySlot->Value = propertyValue;

                result = DATA_PUBLISHER_OK;
            }
        }
    }

    return result;
}

DATA_PUBLISHER_RESULT DataPublisher_EndTransaction(TRANSACTION_HANDLE transactionHandle, unsigned char** destination, size_t* destinationSize)
{
    DATA_PUBLISHER_RESULT result;

    if (
        (transactionHandle == NULL) ||
        (destination == NULL) ||
        (destinationSize == NULL)
        )
    {
        result = DATA_PUBLISHER_INVALID_ARG;
        LOG_DATA_PUBLISHER_ERROR;
    }
    else
    {
        TRANSACTION_HANDLE_DATA* transaction = (TRANSACTION_HANDLE_DATA*)transactionHandle;

        if (transaction->ValueCount == 0)
        {
            result = DATA_PUBLISHER_EMPTY_TRANSACTION;
            LOG_DATA_PUBLISHER_ERROR;
        }
        else if (DataMarshaller_SendData(transaction->DataPublisherInstance->DataMarshallerHandle, transaction->ValueCount, transaction->Values, destination, destinationSize) != DATA_MARSHALLER_OK)
        {
            result = DATA_PUBLISHER_MARSHALLER_ERROR;
            LOG_DATA_PUBLISHER_ERROR;
        }
        else
        {
            result = DATA_PUBLISHER_OK;
        }

        (void)DataPublisher_CancelTransaction(transactionHandle);
    }

    return result;
}

DATA_PUBLISHER_RESULT DataPublisher_CancelTransaction(TRANSACTION_HANDLE transactionHandle)
{
    DATA_PUBLISHER_RESULT result;

    if (transactionHandle == NULL)
    {
        result = DATA_PUBLISHER_INVALID_ARG;
        LOG_DATA_PUBLISHER_ERROR;
    }
    else
    {
        TRANSACTION_HANDLE_DATA* transaction = (TRANSACTION_HANDLE_DATA*)transactionHandle;
        size_t i;

        for (i = 0; i < transaction->ValueCount; i++)
        {
            Destroy_AGENT_DATA_TYPE((AGENT_DATA_TYPE*)transaction->Values[i].Value);
            free((char*)transaction->Values[i].PropertyPath);
            free((AGENT_DATA_TYPE*)transaction->Values[i].Value);
        }

        free(transaction->Values);
        free(transaction);

        result = DATA_PUBLISHER_OK;
    }

    return result;
}

void DataPublisher_SetMaxBufferSize(size_t value)
{
    maxBufferSize_ = value;
}

size_t DataPublisher_GetMaxBufferSize(void)
{
    return maxBufferSize_;
}

REPORTED_PROPERTIES_TRANSACTION_HANDLE DataPublisher_CreateTransaction_ReportedProperties(DATA_PUBLISHER_HANDLE dataPublisherHandle)
{
    REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA* result;
    if (dataPublisherHandle == NULL)
    {
        LogError("invalid argument DATA_PUBLISHER_HANDLE dataPublisherHandle=%p", dataPublisherHandle);
        result = NULL;
    }
    else
    {
        result = (REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA*)malloc(sizeof(REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("unable to malloc");
            /*return as is */
        }
        else
        {
            result->value = VECTOR_create(sizeof(DATA_MARSHALLER_VALUE*));
            if (result->value == NULL)
            {
                LogError("unable to VECTOR_create");
                free(result);
                result = NULL;
            }
            else
            {
                result->DataPublisherInstance = dataPublisherHandle;
            }
        }
    }

    return result;
}

static bool reportedPropertyExistsByPath(const void* element, const void* value)
{
    DATA_MARSHALLER_VALUE* dataMarshallerValue = *(DATA_MARSHALLER_VALUE**)element;
    return (strcmp(dataMarshallerValue->PropertyPath, (const char*)value) == 0);
}

DATA_PUBLISHER_RESULT DataPublisher_PublishTransacted_ReportedProperty(REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle, const char* reportedPropertyPath, const AGENT_DATA_TYPE* data)
{
    DATA_PUBLISHER_RESULT result;
    if (
        (transactionHandle == NULL) ||
        (reportedPropertyPath == NULL) ||
        (data == NULL)
        )
    {
        LogError("invalid argument REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle=%p, const char* reportedPropertyPath=%p, const AGENT_DATA_TYPE* data=%p", transactionHandle, reportedPropertyPath, data);
        result = DATA_PUBLISHER_INVALID_ARG;
    }
    else
    {
        REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA* handleData = (REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA*)transactionHandle;
        if (!Schema_ModelReportedPropertyByPathExists(handleData->DataPublisherInstance->ModelHandle, reportedPropertyPath))
        {
            LogError("unable to find a reported property by path \"%s\"", reportedPropertyPath);
            result = DATA_PUBLISHER_INVALID_ARG;
        }
        else
        {
            DATA_MARSHALLER_VALUE** existingValue = VECTOR_find_if(handleData->value, reportedPropertyExistsByPath, reportedPropertyPath);
            if(existingValue != NULL)
            {
                AGENT_DATA_TYPE *clone = (AGENT_DATA_TYPE *)calloc(1, sizeof(AGENT_DATA_TYPE));
                if(clone == NULL)
                {
                    LogError("unable to malloc");
                    result = DATA_PUBLISHER_ERROR;
                }
                else
                {
                    if (Create_AGENT_DATA_TYPE_from_AGENT_DATA_TYPE(clone, data) != AGENT_DATA_TYPES_OK)
                    {
                        LogError("unable to Create_AGENT_DATA_TYPE_from_AGENT_DATA_TYPE");
                        free(clone);
                        result = DATA_PUBLISHER_ERROR;
                    }
                    else
                    {
                        Destroy_AGENT_DATA_TYPE((AGENT_DATA_TYPE*)((*existingValue)->Value));
                        free((void*)((*existingValue)->Value));
                        (*existingValue)->Value = clone;
                        result = DATA_PUBLISHER_OK;
                    }
                }
            }
            else
            {
                /*totally new reported property*/
                DATA_MARSHALLER_VALUE* newValue = (DATA_MARSHALLER_VALUE*)malloc(sizeof(DATA_MARSHALLER_VALUE));
                if (newValue == NULL)
                {
                    LogError("unable to malloc");
                    result = DATA_PUBLISHER_ERROR;
                }
                else
                {
                    if (mallocAndStrcpy_s((char**)&(newValue->PropertyPath), reportedPropertyPath) != 0)
                    {
                        LogError("unable to mallocAndStrcpy_s");
                        free(newValue);
                        result = DATA_PUBLISHER_ERROR;
                    }
                    else
                    {
                        if ((newValue->Value = (AGENT_DATA_TYPE*)calloc(1, sizeof(AGENT_DATA_TYPE))) == NULL)
                        {
                            LogError("unable to malloc");
                            free((void*)newValue->PropertyPath);
                            free(newValue);
                            result = DATA_PUBLISHER_ERROR;
                        }
                        else
                        {
                            if (Create_AGENT_DATA_TYPE_from_AGENT_DATA_TYPE((AGENT_DATA_TYPE*)newValue->Value, data) != AGENT_DATA_TYPES_OK)
                            {
                                LogError("unable to Create_AGENT_DATA_TYPE_from_AGENT_DATA_TYPE");
                                free((void*)newValue->Value);
                                free((void*)newValue->PropertyPath);
                                free(newValue);
                                result = DATA_PUBLISHER_ERROR;
                            }
                            else
                            {
                                if (VECTOR_push_back(handleData->value, &newValue, 1) != 0)
                                {
                                    LogError("unable to VECTOR_push_back");
                                    Destroy_AGENT_DATA_TYPE((AGENT_DATA_TYPE*)newValue->Value);
                                    free((void*)newValue->Value);
                                    free((void*)newValue->PropertyPath);
                                    free(newValue);
                                    result = DATA_PUBLISHER_ERROR;
                                }
                                else
                                {
                                    result = DATA_PUBLISHER_OK;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

DATA_PUBLISHER_RESULT DataPublisher_CommitTransaction_ReportedProperties(REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle, unsigned char** destination, size_t* destinationSize)
{
    DATA_PUBLISHER_RESULT result;
    if (
        (transactionHandle == NULL) ||
        (destination == NULL) ||
        (destinationSize == NULL)
        )
    {
        LogError("invalid argument REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle=%p, unsigned char** destination=%p, size_t* destinationSize=%p", transactionHandle, destination, destinationSize);
        result = DATA_PUBLISHER_INVALID_ARG;
    }
    else
    {
        REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA* handle = (REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA*)transactionHandle;
        if (VECTOR_size(handle->value) == 0)
        {
            LogError("cannot commit empty transaction");
            result = DATA_PUBLISHER_INVALID_ARG;
        }
        else
        {
            if (DataMarshaller_SendData_ReportedProperties(handle->DataPublisherInstance->DataMarshallerHandle, handle->value, destination, destinationSize) != DATA_MARSHALLER_OK)
            {
                LogError("unable to DataMarshaller_SendData_ReportedProperties");
                result = DATA_PUBLISHER_ERROR;
            }
            else
            {
                result = DATA_PUBLISHER_OK;
            }
        }
    }

    return result;
}

void DataPublisher_DestroyTransaction_ReportedProperties(REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle)
{
    if (transactionHandle == NULL)
    {
        LogError("invalig argument REPORTED_PROPERTIES_TRANSACTION_HANDLE transactionHandle=%p", transactionHandle);
    }
    else
    {
        REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA* handleData = (REPORTED_PROPERTIES_TRANSACTION_HANDLE_DATA*)transactionHandle;
        size_t i, nReportedProperties;
        nReportedProperties = VECTOR_size(handleData->value);
        for (i = 0;i < nReportedProperties;i++)
        {
            DATA_MARSHALLER_VALUE *value = *(DATA_MARSHALLER_VALUE**)VECTOR_element(handleData->value, i);
            Destroy_AGENT_DATA_TYPE((AGENT_DATA_TYPE*)value->Value);
            free((void*)value->Value);
            free((void*)value->PropertyPath);
            free((void*)value);
        }
        VECTOR_destroy(handleData->value);
        free(handleData);
    }
    return;
}
