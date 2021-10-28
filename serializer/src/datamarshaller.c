// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h> /*for free*/
#include "azure_c_shared_utility/gballoc.h"

#include <stdbool.h>
#include "datamarshaller.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "schema.h"
#include "jsonencoder.h"
#include "agenttypesystem.h"
#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"
#include "azure_c_shared_utility/vector.h"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DATA_MARSHALLER_RESULT, DATA_MARSHALLER_RESULT_VALUES);

#define LOG_DATA_MARSHALLER_ERROR \
    LogError("(result = %s)", MU_ENUM_TO_STRING(DATA_MARSHALLER_RESULT, result));

typedef struct DATA_MARSHALLER_HANDLE_DATA_TAG
{
    SCHEMA_MODEL_TYPE_HANDLE ModelHandle;
    bool IncludePropertyPath;
} DATA_MARSHALLER_HANDLE_DATA;

static int NoCloneFunction(void** destination, const void* source)
{
    *destination = (void*)source;
    return 0;
}

static void NoFreeFunction(void* value)
{
    (void)value;
}

DATA_MARSHALLER_HANDLE DataMarshaller_Create(SCHEMA_MODEL_TYPE_HANDLE modelHandle, bool includePropertyPath)
{
    DATA_MARSHALLER_HANDLE_DATA* result;

    if (modelHandle == NULL)
    {
        result = NULL;
        LogError("(result = %s)", MU_ENUM_TO_STRING(DATA_MARSHALLER_RESULT, DATA_MARSHALLER_INVALID_ARG));
    }
    else if ((result = (DATA_MARSHALLER_HANDLE_DATA*)calloc(1, sizeof(DATA_MARSHALLER_HANDLE_DATA))) == NULL)
    {
        result = NULL;
        LogError("(result = %s)", MU_ENUM_TO_STRING(DATA_MARSHALLER_RESULT, DATA_MARSHALLER_ERROR));
    }
    else
    {
        /*everything ok*/
        result->ModelHandle = modelHandle;
        result->IncludePropertyPath = includePropertyPath;
    }
    return result;
}

void DataMarshaller_Destroy(DATA_MARSHALLER_HANDLE dataMarshallerHandle)
{
    if (dataMarshallerHandle != NULL)
    {
        DATA_MARSHALLER_HANDLE_DATA* dataMarshallerInstance = (DATA_MARSHALLER_HANDLE_DATA*)dataMarshallerHandle;
        free(dataMarshallerInstance);
    }
}

DATA_MARSHALLER_RESULT DataMarshaller_SendData(DATA_MARSHALLER_HANDLE dataMarshallerHandle, size_t valueCount, const DATA_MARSHALLER_VALUE* values, unsigned char** destination, size_t* destinationSize)
{
    DATA_MARSHALLER_HANDLE_DATA* dataMarshallerInstance = (DATA_MARSHALLER_HANDLE_DATA*)dataMarshallerHandle;
    DATA_MARSHALLER_RESULT result;
    MULTITREE_HANDLE treeHandle;

    if ((values == NULL) ||
        (dataMarshallerHandle == NULL) ||
        (destination == NULL) ||
        (destinationSize == NULL) ||
        (valueCount == 0))
    {
        result = DATA_MARSHALLER_INVALID_ARG;
        LOG_DATA_MARSHALLER_ERROR
    }
    else
    {
        size_t i;
        bool includePropertyPath = dataMarshallerInstance->IncludePropertyPath;
        /* VS complains wrongly that result is not initialized */
        result = DATA_MARSHALLER_ERROR;

        for (i = 0; i < valueCount; i++)
        {
            if ((values[i].PropertyPath == NULL) ||
                (values[i].Value == NULL))
            {
                result = DATA_MARSHALLER_INVALID_MODEL_PROPERTY;
                LOG_DATA_MARSHALLER_ERROR
                break;
            }

            if ((!dataMarshallerInstance->IncludePropertyPath) &&
                (values[i].Value->type == EDM_COMPLEX_TYPE_TYPE) &&
                (valueCount > 1))
            {
                includePropertyPath = true;
            }
        }

        if (i == valueCount)
        {
            if ((treeHandle = MultiTree_Create(NoCloneFunction, NoFreeFunction)) == NULL)
            {
                result = DATA_MARSHALLER_MULTITREE_ERROR;
                LOG_DATA_MARSHALLER_ERROR
            }
            else
            {
                size_t j;
                result = DATA_MARSHALLER_OK; /* addressing warning in VS compiler */
                for (j = 0; j < valueCount; j++)
                {
                    if ((includePropertyPath == false) && (values[j].Value->type == EDM_COMPLEX_TYPE_TYPE))
                    {
                        size_t k;

                        for (k = 0; k < values[j].Value->value.edmComplexType.nMembers; k++)
                        {
                            if (MultiTree_AddLeaf(treeHandle, values[j].Value->value.edmComplexType.fields[k].fieldName, (void*)values[j].Value->value.edmComplexType.fields[k].value) != MULTITREE_OK)
                            {
                                break;
                            }
                        }

                        if (k < values[j].Value->value.edmComplexType.nMembers)
                        {
                            result = DATA_MARSHALLER_MULTITREE_ERROR;
                            LOG_DATA_MARSHALLER_ERROR
                            break;
                        }
                    }
                    else
                    {
                        if (MultiTree_AddLeaf(treeHandle, values[j].PropertyPath, (void*)values[j].Value) != MULTITREE_OK)
                        {
                            result = DATA_MARSHALLER_MULTITREE_ERROR;
                            LOG_DATA_MARSHALLER_ERROR
                            break;
                        }
                    }

                }

                if (j == valueCount)
                {
                    STRING_HANDLE payload = STRING_new();
                    if (payload == NULL)
                    {
                        result = DATA_MARSHALLER_ERROR;
                        LOG_DATA_MARSHALLER_ERROR
                    }
                    else
                    {
                        if (JSONEncoder_EncodeTree(treeHandle, payload, (JSON_ENCODER_TOSTRING_FUNC)AgentDataTypes_ToString) != JSON_ENCODER_OK)
                        {
                            result = DATA_MARSHALLER_JSON_ENCODER_ERROR;
                            LOG_DATA_MARSHALLER_ERROR
                        }
                        else
                        {
                            size_t resultSize = STRING_length(payload);
                            unsigned char* temp = malloc(resultSize);
                            if (temp == NULL)
                            {
                                result = DATA_MARSHALLER_ERROR;
                                LOG_DATA_MARSHALLER_ERROR;
                            }
                            else
                            {
                                (void)memcpy(temp, STRING_c_str(payload), resultSize);
                                *destination = temp;
                                *destinationSize = resultSize;
                                result = DATA_MARSHALLER_OK;
                            }
                        }
                        STRING_delete(payload);
                    }
                } /* if (j==valueCount)*/
                MultiTree_Destroy(treeHandle);
            } /* MultiTree_Create */
        }
    }

    return result;
}


DATA_MARSHALLER_RESULT DataMarshaller_SendData_ReportedProperties(DATA_MARSHALLER_HANDLE dataMarshallerHandle, VECTOR_HANDLE values, unsigned char** destination, size_t* destinationSize)
{
    DATA_MARSHALLER_RESULT result;
    if (
        (dataMarshallerHandle == NULL) ||
        (values == NULL) ||
        (destination == NULL) ||
        (destinationSize == NULL)
        )
    {
        LogError("invalid argument DATA_MARSHALLER_HANDLE dataMarshallerHandle=%p, VECTOR_HANDLE values=%p, unsigned char** destination=%p, size_t* destinationSize=%p",
            dataMarshallerHandle,
            values,
            destination,
            destinationSize);
        result = DATA_MARSHALLER_INVALID_ARG;
    }
    else
    {
        JSON_Value* json = json_value_init_object();
        if (json == NULL)
        {
            LogError("failure calling json_value_init_object");
            result = DATA_MARSHALLER_ERROR;
        }
        else
        {
            JSON_Object* jsonObject = json_object(json);
            if (jsonObject == NULL)
            {
                LogError("failure calling json_object");
                result = DATA_MARSHALLER_ERROR;
            }
            else
            {
                size_t nReportedProperties = VECTOR_size(values), nProcessedProperties = 0;

                for (size_t i = 0;i < nReportedProperties; i++)
                {
                    DATA_MARSHALLER_VALUE* v = *(DATA_MARSHALLER_VALUE**)VECTOR_element(values, i);
                    STRING_HANDLE s = STRING_new();
                    if (s == NULL)
                    {
                        LogError("failure calling STRING_new");
                        i = nReportedProperties;/*forces loop to break, result is set in the "if" following this for*/
                    }
                    else
                    {
                        if (AgentDataTypes_ToString(s, v->Value) != AGENT_DATA_TYPES_OK)
                        {
                            LogError("failure calling AgentDataTypes_ToString");
                            i = nReportedProperties;/*forces loop to break, result is set in the "if" following this for*/
                        }
                        else
                        {
                            JSON_Value * rightSide = json_parse_string(STRING_c_str(s));
                            if (rightSide == NULL)
                            {
                                LogError("failure calling json_parse_string");
                                i = nReportedProperties;/*forces loop to break, result is set in the "if" following this for*/
                            }
                            else
                            {
                                char* leftSide;
                                if (mallocAndStrcpy_s(&leftSide, v->PropertyPath) != 0)
                                {
                                    LogError("failure calling mallocAndStrcpy_s");
                                    json_value_free(rightSide);
                                    i = nReportedProperties;/*forces loop to break, result is set in the "if" following this for*/
                                }
                                else
                                {
                                    char *whereIsSlash;
                                    while ((whereIsSlash = strchr(leftSide, '/')) != NULL)
                                    {
                                        *whereIsSlash = '.';
                                    }

                                    if (json_object_dotset_value(jsonObject, leftSide, rightSide) != JSONSuccess)
                                    {
                                        LogError("failure calling json_object_dotset_value");
                                        json_value_free(rightSide);
                                        i = nReportedProperties;/*forces loop to break, result is set in the "if" following this for*/
                                    }
                                    else
                                    {
                                        /*all is fine with this property... */
                                        nProcessedProperties++;
                                    }
                                    free(leftSide);
                                }
                            }
                        }
                        STRING_delete(s);
                    }
                }

                if (nProcessedProperties != nReportedProperties)
                {
                    result = DATA_MARSHALLER_ERROR;
                    /*all properties have NOT been processed*/
                    /*return result as is*/
                }
                else
                {
                    char* temp = json_serialize_to_string_pretty(json);
                    if (temp == NULL)
                    {
                        LogError("failure calling json_serialize_to_string_pretty ");
                        result = DATA_MARSHALLER_ERROR;
                    }
                    else
                    {
                        *destination = (unsigned char*)temp;
                        *destinationSize = strlen(temp);
                        result = DATA_MARSHALLER_OK;
                        /*all is fine... */
                    }
                }
            }
            json_value_free(json);
        }
    }
    return result;
}
