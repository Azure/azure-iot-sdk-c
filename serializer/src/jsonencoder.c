// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/gballoc.h"

#include "jsonencoder.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"

#ifdef _MSC_VER
#pragma warning(disable: 4701) /* potentially uninitialized local variable 'result' used */ /* the scanner cannot track variable "i" and link it to childCount*/
#endif

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(JSON_ENCODER_TOSTRING_RESULT, JSON_ENCODER_TOSTRING_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(JSON_ENCODER_RESULT, JSON_ENCODER_RESULT_VALUES);

JSON_ENCODER_RESULT JSONEncoder_EncodeTree(MULTITREE_HANDLE treeHandle, STRING_HANDLE destination, JSON_ENCODER_TOSTRING_FUNC toStringFunc)
{
    JSON_ENCODER_RESULT result;

    size_t childCount;

    if ((treeHandle == NULL) ||
        (destination == NULL) ||
        (toStringFunc == NULL))
    {
        result = JSON_ENCODER_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
    }
    else if (MultiTree_GetChildCount(treeHandle, &childCount) != MULTITREE_OK)
    {
        result = JSON_ENCODER_MULTITREE_ERROR;
        LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
    }
    else
    {
        size_t i;
        if (STRING_concat(destination,  "{") != 0)
        {
            result = JSON_ENCODER_ERROR;
            LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
        }
        else
        {
            result = JSON_ENCODER_OK;
            for (i = 0; (i < childCount) && (result == JSON_ENCODER_OK); i++)
            {
                MULTITREE_HANDLE childTreeHandle;

                if ((i > 0) &&
                    (STRING_concat(destination, ", ") != 0))
                {
                    result = JSON_ENCODER_ERROR;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                }
                else if (STRING_concat(destination, "\"") != 0)
                {
                    result = JSON_ENCODER_ERROR;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                }
                else
                {
                    STRING_HANDLE name = STRING_new();
                    if (name == NULL)
                    {
                        result = JSON_ENCODER_ERROR;
                        LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                    }
                    else
                    {
                        if (MultiTree_GetChild(treeHandle, i, &childTreeHandle) != MULTITREE_OK)
                        {
                            result = JSON_ENCODER_MULTITREE_ERROR;
                            LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                        }
                        else if (MultiTree_GetName(childTreeHandle, name) != MULTITREE_OK)
                        {
                            result = JSON_ENCODER_MULTITREE_ERROR;
                            LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                        }
                        else if (STRING_concat_with_STRING(destination, name) != 0)
                        {
                            result = JSON_ENCODER_ERROR;
                            LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                        }
                        else if (STRING_concat(destination, "\":") != 0)
                        {
                            result = JSON_ENCODER_ERROR;
                            LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                        }
                        else
                        {
                            size_t innerChildCount;
                            if (MultiTree_GetChildCount(childTreeHandle, &innerChildCount) != MULTITREE_OK)
                            {
                                result = JSON_ENCODER_MULTITREE_ERROR;
                                LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                            }
                            else
                            {
                                if (innerChildCount > 0)
                                {
                                    STRING_HANDLE child = STRING_new();
                                    if (child == NULL)
                                    {
                                        result = JSON_ENCODER_ERROR;
                                        LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                                    }
                                    else
                                    {
                                        if ((result = JSONEncoder_EncodeTree(childTreeHandle, child, toStringFunc)) != JSON_ENCODER_OK)
                                        {
                                            LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                                        }
                                        else if (STRING_concat_with_STRING(destination, child)!=0)
                                        {
                                            result = JSON_ENCODER_ERROR;
                                            LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                                        }
                                        STRING_delete(child);
                                    }
                                }
                                else
                                {
                                    const void* value;
                                    if (MultiTree_GetValue(childTreeHandle, &value) != MULTITREE_OK)
                                    {
                                        result = JSON_ENCODER_MULTITREE_ERROR;
                                        LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));

                                    }
                                    else
                                    {
                                        STRING_HANDLE childValue = STRING_new();
                                        if (childValue == NULL)
                                        {
                                            result = JSON_ENCODER_ERROR;
                                            LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                                        }
                                        else
                                        {
                                            if (toStringFunc(childValue, value) != JSON_ENCODER_TOSTRING_OK)
                                            {
                                                result = JSON_ENCODER_TOSTRING_FUNCTION_ERROR;
                                                LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                                            }
                                            else if (STRING_concat_with_STRING(destination, childValue)!=0)
                                            {
                                                result = JSON_ENCODER_ERROR;
                                                LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                                            }
                                            else
                                            {
                                                /*do nothing, result = JSON_ENCODER_OK is set above at the beginning of the FOR loop*/
                                            }
                                            STRING_delete(childValue);
                                        }
                                    }
                                }
                            }
                        }
                        STRING_delete(name);
                    }
                }
            }

            if ((i == childCount) && (result == JSON_ENCODER_OK))
            {
                if (STRING_concat(destination,  "}") != 0)
                {
                    result = JSON_ENCODER_ERROR;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_RESULT, result));
                }
                else
                {
                    result = JSON_ENCODER_OK;
                }
            }
        }
    }

    return result;
#ifdef _MSC_VER
#pragma warning(disable: 4701) /* potentially uninitialized local variable 'result' used */ /* the scanner cannot track variable "i" and link it to childCount*/
#endif
}

JSON_ENCODER_TOSTRING_RESULT JSONEncoder_CharPtr_ToString(STRING_HANDLE destination, const void* value)
{
    JSON_ENCODER_TOSTRING_RESULT result;

    if ((destination == NULL) ||
        (value == NULL))
    {
        result = JSON_ENCODER_TOSTRING_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_TOSTRING_RESULT, result));
    }
    else if (STRING_concat(destination, (const char*)value) != 0)
    {
        result = JSON_ENCODER_TOSTRING_ERROR;
        LogError("(result = %s)", MU_ENUM_TO_STRING(JSON_ENCODER_TOSTRING_RESULT, result));
    }
    else
    {
        result = JSON_ENCODER_TOSTRING_OK;
    }

    return result;
}
