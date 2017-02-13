// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "optionhandler.h"
#include "xlogging.h"
#include "gballoc.h"
#include "vector.h"

typedef struct OPTION_TAG
{
    const char* name;
    void* storage;
}OPTION;

typedef struct OPTIONHANDLER_HANDLE_DATA_TAG
{
    pfCloneOption cloneOption;
    pfDestroyOption destroyOption;
    pfSetOption setOption;
    VECTOR_HANDLE storage;
}OPTIONHANDLER_HANDLE_DATA;

OPTIONHANDLER_HANDLE OptionHandler_Create(pfCloneOption cloneOption, pfDestroyOption destroyOption, pfSetOption setOption)
{
    /*Codes_SRS_OPTIONHANDLER_02_001: [ OptionHandler_Create shall fail and retun NULL if any parameters are NULL. ]*/
    OPTIONHANDLER_HANDLE_DATA* result;
    if (
        (cloneOption == NULL) ||
        (destroyOption == NULL) ||
        (setOption == NULL)
        )
    {
        LogError("invalid parameter = pfCloneOption cloneOption=%p, pfDestroyOption destroyOption=%p, pfSetOption setOption=%p", cloneOption, destroyOption, setOption);
        result = NULL;
    }
    else
    {
        result = (OPTIONHANDLER_HANDLE_DATA*)malloc(sizeof(OPTIONHANDLER_HANDLE_DATA));
        if (result == NULL)
        {
            /*Codes_SRS_OPTIONHANDLER_02_004: [ Otherwise, OptionHandler_Create shall fail and return NULL. ]*/
            LogError("unable to malloc");
            /*return as is*/
        }
        else
        {
            /*Codes_SRS_OPTIONHANDLER_02_002: [ OptionHandler_Create shall create an empty VECTOR that will hold pairs of const char* and void*. ]*/
            result->storage = VECTOR_create(sizeof(OPTION));
            if (result->storage == NULL)
            {
                /*Codes_SRS_OPTIONHANDLER_02_004: [ Otherwise, OptionHandler_Create shall fail and return NULL. ]*/
                LogError("unable to VECTOR_create");
                free(result);
                result= NULL;
            }
            else
            {
                /*Codes_SRS_OPTIONHANDLER_02_003: [ If all the operations succeed then OptionHandler_Create shall succeed and return a non-NULL handle. ]*/
                result->cloneOption = cloneOption;
                result->destroyOption = destroyOption;
                result->setOption = setOption;
                /*return as is*/
            }
        }
    }
    return result;

}

OPTIONHANDLER_RESULT OptionHandler_AddOption(OPTIONHANDLER_HANDLE handle, const char* name, const void* value)
{
    OPTIONHANDLER_RESULT result;
    /*Codes_SRS_OPTIONHANDLER_02_001: [ OptionHandler_Create shall fail and retun NULL if any parameters are NULL. ]*/
    if (
        (handle == NULL) ||
        (name == NULL) ||
        (value == NULL)
        )
    {
        LogError("invalid arguments: OPTIONHANDLER_HANDLE handle=%p, const char* name=%p, void* value=%p", handle, name, value);
        result= OPTIONHANDLER_INVALIDARG;
    }
    else
    {
        const char* cloneOfName;
        if (mallocAndStrcpy_s((char**)&cloneOfName, name) != 0)
        {
            /*Codes_SRS_OPTIONHANDLER_02_009: [ Otherwise, OptionHandler_AddProperty shall succeed and return OPTIONHANDLER_ERROR. ]*/
            LogError("unable to clone name");
            result = OPTIONHANDLER_ERROR;
        }
        else 
        {
            /*Codes_SRS_OPTIONHANDLER_02_006: [ OptionHandler_AddProperty shall call pfCloneOption passing name and value. ]*/
            void* cloneOfValue = handle->cloneOption(name, value);
            if (cloneOfValue == NULL)
            {
                /*Codes_SRS_OPTIONHANDLER_02_009: [ Otherwise, OptionHandler_AddProperty shall succeed and return OPTIONHANDLER_ERROR. ]*/
                LogError("unable to clone value");
                free((void*)cloneOfName);
                result = OPTIONHANDLER_ERROR;
            }
            else
            {
                OPTION temp;
                temp.name = cloneOfName;
                temp.storage = cloneOfValue;
                /*Codes_SRS_OPTIONHANDLER_02_007: [ OptionHandler_AddProperty shall use VECTOR APIs to save the name and the newly created clone of value. ]*/
                if (VECTOR_push_back(handle->storage, &temp, 1) != 0)
                {
                    /*Codes_SRS_OPTIONHANDLER_02_009: [ Otherwise, OptionHandler_AddProperty shall succeed and return OPTIONHANDLER_ERROR. ]*/
                    LogError("unable to VECTOR_push_back");
                    handle->destroyOption(name, cloneOfValue);
                    free((void*)cloneOfName);
                    result = OPTIONHANDLER_ERROR;
                }
                else
                {
                    /*Codes_SRS_OPTIONHANDLER_02_008: [ If all the operations succed then OptionHandler_AddProperty shall succeed and return OPTIONHANDLER_OK. ]*/
                    result = OPTIONHANDLER_OK;
                }
            }
        }
    }
    return result;
}

OPTIONHANDLER_RESULT OptionHandler_FeedOptions(OPTIONHANDLER_HANDLE handle, void* destinationHandle)
{
    OPTIONHANDLER_RESULT result;
    /*Codes_SRS_OPTIONHANDLER_02_010: [ OptionHandler_FeedOptions shall fail and return OPTIONHANDLER_INVALIDARG if any argument is NULL. ]*/
    if (
        (handle == NULL) ||
        (destinationHandle == NULL)
        )
    {
        LogError("invalid arguments OPTIONHANDLER_HANDLE handle=%p, void* destinationHandle=%p", handle, destinationHandle);
        result = OPTIONHANDLER_INVALIDARG;
    }
    else
    {
        /*Codes_SRS_OPTIONHANDLER_02_011: [ Otherwise, OptionHandler_FeedOptions shall use VECTOR's iteration mechanisms to retrieve pairs of name, value (const char* and void*). ]*/
        size_t nOptions = VECTOR_size(handle->storage), i;
        for (i = 0;i < nOptions;i++)
        {
            OPTION* option = (OPTION*)VECTOR_element(handle->storage, i);
            /*Codes_SRS_OPTIONHANDLER_02_012: [ OptionHandler_FeedOptions shall call for every pair of name,value setOption passing destinationHandle, name and value. ]*/
            if (handle->setOption(destinationHandle, option->name, option->storage) != 0)
            {
                LogError("failure while trying to _SetOption");
                break;
            }
        }
            
        if (i == nOptions)
        {
            /*Codes_SRS_OPTIONHANDLER_02_014: [ Otherwise, OptionHandler_FeedOptions shall fail and return OPTIONHANDLER_ERROR. ]*/
            result = OPTIONHANDLER_OK;
        }
        else
        {
            /*Codes_SRS_OPTIONHANDLER_02_013: [ If all the operations succeed then OptionHandler_FeedOptions shall succeed and return OPTIONHANDLER_OK. ]*/
            result = OPTIONHANDLER_ERROR;
        }
    }
    return result;
}

void OptionHandler_Destroy(OPTIONHANDLER_HANDLE handle)
{   
    /*Codes_SRS_OPTIONHANDLER_02_015: [ OptionHandler_Destroy shall do nothing if parameter handle is NULL. ]*/
    if (handle == NULL)
    {
        LogError("invalid argument OPTIONHANDLER_HANDLE handle=%p", handle);
    }
    else
    {
        /*Codes_SRS_OPTIONHANDLER_02_016: [ Otherwise, OptionHandler_Destroy shall free all used resources. ]*/
        size_t nOptions = VECTOR_size(handle->storage), i;
        for (i = 0;i < nOptions;i++)
        {
            OPTION* option = (OPTION*)VECTOR_element(handle->storage, i);
            handle->destroyOption(option->name, option->storage);
            free((void*)option->name);
        }

        VECTOR_destroy(handle->storage);
        free(handle);
    }
}
