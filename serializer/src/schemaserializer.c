// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"

#include <stddef.h>
#include "schemaserializer.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_macro_utils/macro_utils.h"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_RESULT_VALUES);

#define LOG_SCHEMA_SERIALIZER_ERROR(result) LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_SERIALIZER_RESULT, (result)))

static const char* ConvertType(const char* sourceType)
{
    if (strcmp(sourceType, "ascii_char_ptr") == 0)
    {
        return "string";
    }
    else
    {
        return sourceType;
    }
}

SCHEMA_SERIALIZER_RESULT SchemaSerializer_SerializeCommandMetadata(SCHEMA_MODEL_TYPE_HANDLE modelHandle, STRING_HANDLE schemaText)
{
    SCHEMA_SERIALIZER_RESULT result;

    if ((modelHandle == NULL) ||
        (schemaText == NULL))
    {
        result = SCHEMA_SERIALIZER_INVALID_ARG;
        LogError("(result = %s), modelHandle = %p, schemaText = %p", MU_ENUM_TO_STRING(SCHEMA_SERIALIZER_RESULT, result), modelHandle, schemaText);
    }
    else
    {
        size_t commandCount;


        if ((STRING_concat(schemaText, "[") != 0) ||
            (Schema_GetModelActionCount(modelHandle, &commandCount) != SCHEMA_OK))
        {
            result = SCHEMA_SERIALIZER_ERROR;
            LOG_SCHEMA_SERIALIZER_ERROR(result);
        }
        else
        {
            size_t i;

            for (i = 0; i < commandCount; i++)
            {
                SCHEMA_ACTION_HANDLE actionHandle = Schema_GetModelActionByIndex(modelHandle, i);
                const char* commandName;
                size_t argCount;
                size_t j;

                if ((actionHandle == NULL) ||
                    (STRING_concat(schemaText, "{ \"Name\":\"") != 0) ||
                    ((commandName = Schema_GetModelActionName(actionHandle)) == NULL) ||
                    (STRING_concat(schemaText, commandName) != 0) ||
                    (STRING_concat(schemaText, "\", \"Parameters\":[") != 0) ||
                    (Schema_GetModelActionArgumentCount(actionHandle, &argCount) != SCHEMA_OK))
                {
                    LogError("Failed encoding action.");
                    break;
                }
                else
                {
                    for (j = 0; j < argCount; j++)
                    {
                        SCHEMA_ACTION_ARGUMENT_HANDLE argHandle = Schema_GetModelActionArgumentByIndex(actionHandle, j);
                        const char* argName;
                        const char* argType;

                        if ((argHandle == NULL) ||
                            (STRING_concat(schemaText, "{\"Name\":\"") != 0) ||
                            ((argName = Schema_GetActionArgumentName(argHandle)) == NULL) ||
                            (STRING_concat(schemaText, argName) != 0) ||
                            (STRING_concat(schemaText, "\",\"Type\":\"") != 0) ||
                            ((argType = Schema_GetActionArgumentType(argHandle)) == NULL) ||
                            (STRING_concat(schemaText, ConvertType(argType)) != 0))
                        {
                            LogError("Failed encoding argument.");
                            break;
                        }
                        else
                        {
                            if (j + 1 < argCount)
                            {
                                if (STRING_concat(schemaText, "\"},") != 0)
                                {
                                    LogError("Failed to concatenate arg end.");
                                    break;
                                }
                            }
                            else
                            {
                                if (STRING_concat(schemaText, "\"}") != 0)
                                {
                                    LogError("Failed to concatenate arg end.");
                                    break;
                                }
                            }
                        }
                    }

                    if (j < argCount)
                    {
                        break;
                    }

                    if (i + 1 < commandCount)
                    {
                        if (STRING_concat(schemaText, "]},") != 0)
                        {
                            LogError("Failed to concatenate.");
                            break;
                        }
                    }
                    else
                    {
                        if (STRING_concat(schemaText, "]}") != 0)
                        {
                            LogError("Failed to concatenate.");
                            break;
                        }
                    }
                }
            }

            if (i < commandCount)
            {
                result = SCHEMA_SERIALIZER_ERROR;
            }
            else if (STRING_concat(schemaText, "]") != 0)
            {
                LogError("Failed to concatenate commands object end.");
                result = SCHEMA_SERIALIZER_ERROR;
            }
            else
            {
                result = SCHEMA_SERIALIZER_OK;
            }
        }
    }

    return result;
}
