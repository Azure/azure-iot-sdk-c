// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>  

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "provisioning_sc_shared_helpers.h"
#include "parson.h"

int mallocAndStrcpy_overwrite(char** dest, const char* source)
{
    int result = 0;
    char* temp = NULL;

    if (dest == NULL || source == NULL)
    {
        LogError("Invalid input");
        result = __FAILURE__;
    }
    else if (mallocAndStrcpy_s(&temp, source) != 0)
    {
        LogError("Failed to copy value from source");
        result = __FAILURE__;
    }
    else
    {
        free(*dest);
        *dest = temp;
    }

    return result;
}

int copy_json_string_field(char** dest, JSON_Object* root_object, const char* json_key)
{
    int result = 0;

    const char* string = json_object_get_string(root_object, json_key);
    if (string != NULL)
    {
        if (mallocAndStrcpy_s(dest, string) != 0)
        {
            result = __FAILURE__;
        }
    }

    return result;
}

int json_serialize_and_set_struct(JSON_Object* root_object, const char* json_key, void* structure, TO_JSON_FUNCTION toJson, NECESSITY necessity)
{
    int result;

    if (necessity == OPTIONAL && structure == NULL)
    {
        result = 0;
    }
    else
    {
        JSON_Value* struct_val;
        if (structure == NULL)
        {
            LogError("NULL structure");
            result = __FAILURE__;
        }
        else if ((struct_val = toJson(structure)) == NULL)
        {
            LogError("Failed converting structure to JSON Value");
            result = __FAILURE__;
        }
        else if (json_object_set_value(root_object, json_key, struct_val) != JSONSuccess)
        {
            LogError("Failed to set JSON Value in JSON Object");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

int json_deserialize_and_get_struct(void** dest, JSON_Object* root_object, const char* json_key, FROM_JSON_FUNCTION fromJson, NECESSITY necessity)
{
    int result;

    JSON_Object* struct_object = json_object_get_object(root_object, json_key);

    if (necessity == OPTIONAL && struct_object == NULL)
    {
        result = 0;
    }
    else if (necessity == REQUIRED && struct_object == NULL)
    {
        LogError("object required");
        result = __FAILURE__;
    }
    else
    {
        if ((*dest = fromJson(struct_object)) == NULL)
        {
            LogError("Failed to deserialize from JSON");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}