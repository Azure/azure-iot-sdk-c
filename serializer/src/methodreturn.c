// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <limits.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/strings.h"
#include "parson.h"

#include "methodreturn.h"

typedef struct METHODRETURN_HANDLE_DATA_TAG
{
    METHODRETURN_DATA data;
}METHODRETURN_HANDLE_DATA;

bool is_json_present_and_unparsable(const char* jsonValue)
{
    bool is_present_and_unparsable;
    if (jsonValue == NULL)
    {
        // Null json is not considered invalid here
        is_present_and_unparsable = false;
    }
    else
    {
        JSON_Value* temp = json_parse_string(jsonValue);
        if (temp == NULL)
        {
            is_present_and_unparsable = true;
        }
        else
        {
            json_value_free(temp);
            is_present_and_unparsable = false;
        }
    }
    return is_present_and_unparsable;
}

METHODRETURN_HANDLE MethodReturn_Create(int statusCode, const char* jsonValue)
{
    METHODRETURN_HANDLE result;
    if (is_json_present_and_unparsable(jsonValue))
    {
        LogError("%s is not JSON", jsonValue);
        result = NULL;
    }
    else
    {
        result = (METHODRETURN_HANDLE_DATA*)calloc(1, sizeof(METHODRETURN_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("unable to malloc");
            /*return as is*/
        }
        else
        {
            if (jsonValue == NULL)
            {
                result->data.jsonValue = NULL;
                result->data.statusCode = statusCode;
            }
            else
            {
                if (mallocAndStrcpy_s(&(result->data.jsonValue), jsonValue) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s");
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->data.statusCode = statusCode;
                }
            }
        }
    }

    return result;
}

void MethodReturn_Destroy(METHODRETURN_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("invalid argument METHODRETURN_HANDLE handle=%p", handle);
    }
    else
    {
        if (handle->data.jsonValue != NULL)
        {
            free(handle->data.jsonValue);
        }
        free(handle);
    }
}


const METHODRETURN_DATA* MethodReturn_GetReturn(METHODRETURN_HANDLE handle)
{
    const METHODRETURN_DATA* result;
    if (handle == NULL)
    {
        result = NULL;
    }
    else
    {
        result = &(handle->data);
    }
    return result;
}
