// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"

#include "multitree.h"
#include <string.h>
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/const_defines.h"
#include "azure_c_shared_utility/safe_math.h"

/*assume a name cannot be longer than 100 characters*/
#define INNER_NODE_NAME_SIZE 128

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(MULTITREE_RESULT, MULTITREE_RESULT_VALUES);

typedef struct MULTITREE_HANDLE_DATA_TAG
{
    char* name;
    void* value;
    MULTITREE_CLONE_FUNCTION cloneFunction;
    MULTITREE_FREE_FUNCTION freeFunction;
    size_t nChildren;
    struct MULTITREE_HANDLE_DATA_TAG** children; /*an array of nChildren count of MULTITREE_HANDLE_DATA*   */
}MULTITREE_HANDLE_DATA;


MULTITREE_HANDLE MultiTree_Create(MULTITREE_CLONE_FUNCTION cloneFunction, MULTITREE_FREE_FUNCTION freeFunction)
{
    MULTITREE_HANDLE_DATA* result;

    if ((cloneFunction == NULL) ||
        (freeFunction == NULL))
    {
        LogError("CloneFunction or FreeFunction is Null.");
        result = NULL;
    }
    else
    {
        result = (MULTITREE_HANDLE_DATA*)calloc(1, sizeof(MULTITREE_HANDLE_DATA));
        if (result != NULL)
        {
            result->name = NULL;
            result->value = NULL;
            result->cloneFunction = cloneFunction;
            result->freeFunction = freeFunction;
            result->nChildren = 0;
            result->children = NULL;
        }
        else
        {
            LogError("MultiTree_Create failed because malloc failed");
        }
    }

    return (MULTITREE_HANDLE)result;
}


/*return NULL if a child with the name "name" doesn't exists*/
/*returns a pointer to the existing child (if any)*/
static MULTITREE_HANDLE_DATA* getChildByName(MULTITREE_HANDLE_DATA* node, const char* name)
{
    MULTITREE_HANDLE_DATA* result = NULL;
    size_t i;
    for (i = 0; i < node->nChildren; i++)
    {
        if (strcmp(node->children[i]->name, name) == 0)
        {
            result = node->children[i];
            break;
        }
    }
    return result;
}

/*helper function to create a child immediately under this node*/
/*return 0 if it created it, any other number is error*/

typedef enum CREATELEAF_RESULT_TAG
{
    CREATELEAF_OK,
    CREATELEAF_ALREADY_EXISTS,
    CREATELEAF_EMPTY_NAME,
    CREATELEAF_ERROR,
    CREATELEAF_RESULT_COUNT // Used to track the number of elements in the enum
                            // Do not remove, or add new enum values below this one
}CREATELEAF_RESULT;

static STATIC_VAR_UNUSED const char* CreateLeaf_ResultAsString[CREATELEAF_RESULT_COUNT] =
{
    MU_TOSTRING(CREATELEAF_OK),
    MU_TOSTRING(CREATELEAF_ALREADY_EXISTS),
    MU_TOSTRING(CREATELEAF_EMPTY_NAME),
    MU_TOSTRING(CREATELEAF_ERROR)
};

/*name cannot be empty, value can be empty or NULL*/
#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#endif
#ifdef _MSC_VER
#pragma warning(disable: 4701) /* potentially uninitialized local variable 'result' used */ /* the scanner cannot track linked "newNode" and "result" therefore the warning*/
#endif
static CREATELEAF_RESULT createLeaf(MULTITREE_HANDLE_DATA* node, const char*name, const char*value, MULTITREE_HANDLE_DATA** childNode)
{
    CREATELEAF_RESULT result;
    /*can only create it if it doesn't exist*/
    if (strlen(name) == 0)
    {
        result = CREATELEAF_EMPTY_NAME;
        LogError("(result = %s)", CreateLeaf_ResultAsString[result]);
    }
    else if (getChildByName(node, name) != NULL)
    {
        result = CREATELEAF_ALREADY_EXISTS;
        LogError("(result = %s)", CreateLeaf_ResultAsString[result]);
    }
    else
    {
        MULTITREE_HANDLE_DATA* newNode = (MULTITREE_HANDLE_DATA*)calloc(1, sizeof(MULTITREE_HANDLE_DATA));
        if (newNode == NULL)
        {
            result = CREATELEAF_ERROR;
            LogError("(result = %s)", CreateLeaf_ResultAsString[result]);
        }
        else
        {
            newNode->nChildren = 0;
            newNode->children = NULL;
            if (mallocAndStrcpy_s(&(newNode->name), name) != 0)
            {
                /*not nice*/
                free(newNode);
                newNode = NULL;
                result = CREATELEAF_ERROR;
                LogError("(result = %s)", CreateLeaf_ResultAsString[result]);
            }
            else
            {
                newNode->cloneFunction = node->cloneFunction;
                newNode->freeFunction = node->freeFunction;

                if (value == NULL)
                {
                    newNode->value = NULL;
                }
                else if (node->cloneFunction(&(newNode->value), value) != 0)
                {
                    free(newNode->name);
                    newNode->name = NULL;
                    free(newNode);
                    newNode = NULL;
                    result = CREATELEAF_ERROR;
                    LogError("(result = %s)", CreateLeaf_ResultAsString[result]);
                }
                else
                {
                    /*all is fine until now*/
                }
            }


            if (newNode!=NULL)
            {
                /*allocate space in the father node*/
                MULTITREE_HANDLE_DATA** newChildren;
                size_t realloc_size = safe_multiply_size_t(safe_add_size_t(node->nChildren, 1), sizeof(MULTITREE_HANDLE_DATA*));
                if (realloc_size == SIZE_MAX ||
                    (newChildren = (MULTITREE_HANDLE_DATA**)realloc(node->children, realloc_size)) == NULL)
                {
                    /*no space for the new node*/
                    newNode->value = NULL;
                    free(newNode->name);
                    newNode->name = NULL;
                    free(newNode);
                    newNode = NULL;
                    result = CREATELEAF_ERROR;
                    LogError("(result = %s), size:%zu", CreateLeaf_ResultAsString[result], realloc_size);
                }
                else
                {
                    node->children = newChildren;
                    node->children[node->nChildren] = newNode;
                    node->nChildren++;
                    if (childNode != NULL)
                    {
                        *childNode = newNode;
                    }
                    result = CREATELEAF_OK;
                }
            }
        }
    }

    return result;
#ifdef _MSC_VER
#pragma warning(default: 4701) /* potentially uninitialized local variable 'result' used */ /* the scanner cannot track linked "newNode" and "result" therefore the warning*/
#endif
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif
}

MULTITREE_RESULT MultiTree_AddLeaf(MULTITREE_HANDLE treeHandle, const char* destinationPath, const void* value)
{
    MULTITREE_RESULT result;
    if (treeHandle == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else if (destinationPath == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else if (value == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else if (strlen(destinationPath) == 0)
    {
        result = MULTITREE_EMPTY_CHILD_NAME;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        /*break the path into components*/
        /*find the first child name*/
        MULTITREE_HANDLE_DATA * node = (MULTITREE_HANDLE_DATA *)treeHandle;
        char * whereIsDelimiter;
        /*if first character is / then skip it*/
        if (destinationPath[0] == '/')
        {
            destinationPath++;
        }
        /*if there's just a string, it needs to be created here*/
        whereIsDelimiter = (char*)strchr(destinationPath, '/');
        if (whereIsDelimiter == NULL)
        {
            CREATELEAF_RESULT res = createLeaf(node, destinationPath, (const char*)value, NULL);
            switch (res)
            {
                default:
                {
                    result = MULTITREE_ERROR;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                    break;
                }
                case CREATELEAF_ALREADY_EXISTS:
                {
                    result = MULTITREE_ALREADY_HAS_A_VALUE;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                    break;
                }
                case CREATELEAF_OK:
                {
                    result = MULTITREE_OK;
                    break;
                }
                case CREATELEAF_EMPTY_NAME:
                {
                    result = MULTITREE_EMPTY_CHILD_NAME;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                    break;
                }
            }
        }
        else
        {
            /*if there's more or 1 delimiter in the path... */
            char firstInnerNodeName[INNER_NODE_NAME_SIZE];
            if ((whereIsDelimiter - destinationPath) >= INNER_NODE_NAME_SIZE)
            {
                result = MULTITREE_ERROR;
                LogError("Destination path is too large %lu", (unsigned long)(whereIsDelimiter - destinationPath));
            }
            else if (memcpy(firstInnerNodeName, destinationPath, whereIsDelimiter - destinationPath) == NULL)
            {
                result = MULTITREE_ERROR;
                LogError("(result = MULTITREE_ERROR)");
            }
            else
            {
                firstInnerNodeName[whereIsDelimiter - destinationPath] = 0;
                MULTITREE_HANDLE_DATA *child = getChildByName(node, firstInnerNodeName);
                if (child == NULL)
                {
                    CREATELEAF_RESULT res = createLeaf(node, firstInnerNodeName, NULL, NULL);
                    switch (res)
                    {
                        default:
                        {
                            result = MULTITREE_ERROR;
                            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                            break;
                        }
                        case(CREATELEAF_EMPTY_NAME):
                        {
                            result = MULTITREE_EMPTY_CHILD_NAME;
                            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                            break;
                        }
                        case(CREATELEAF_OK):
                        {
                            MULTITREE_HANDLE_DATA *createdChild = getChildByName(node, firstInnerNodeName);
                            result = MultiTree_AddLeaf(createdChild, whereIsDelimiter, value);
                            break;
                        }
                    };
                }
                else
                {
                    result = MultiTree_AddLeaf(child, whereIsDelimiter, value);
                }
            }
        }
    }
    return result;
}

MULTITREE_RESULT MultiTree_AddChild(MULTITREE_HANDLE treeHandle, const char* childName, MULTITREE_HANDLE* childHandle)
{
    MULTITREE_RESULT result;
    if ((treeHandle == NULL) ||
        (childName == NULL) ||
        (childHandle == NULL))
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        MULTITREE_HANDLE_DATA* childNode;

        CREATELEAF_RESULT res = createLeaf((MULTITREE_HANDLE_DATA*)treeHandle, childName, NULL, &childNode);
        switch (res)
        {
            default:
            {
                result = MULTITREE_ERROR;
                LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                break;
            }
            case CREATELEAF_ALREADY_EXISTS:
            {
                result = MULTITREE_ALREADY_HAS_A_VALUE;
                LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                break;
            }
            case CREATELEAF_OK:
            {
                *childHandle = childNode;

                result = MULTITREE_OK;
                break;
            }
            case CREATELEAF_EMPTY_NAME:
            {
                result = MULTITREE_EMPTY_CHILD_NAME;
                LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                break;
            }
        }
    }

    return result;
}

MULTITREE_RESULT MultiTree_GetChildCount(MULTITREE_HANDLE treeHandle, size_t* count)
{
    MULTITREE_RESULT result;
    if (treeHandle == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else if (count == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        *count = ((MULTITREE_HANDLE_DATA*)treeHandle)->nChildren;
        result = MULTITREE_OK;
    }
    return result;
}

MULTITREE_RESULT MultiTree_GetChild(MULTITREE_HANDLE treeHandle, size_t index, MULTITREE_HANDLE *childHandle)
{
    MULTITREE_RESULT result;
    if (treeHandle == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else if (childHandle == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        MULTITREE_HANDLE_DATA * node = (MULTITREE_HANDLE_DATA *)treeHandle;
        if (node->nChildren <= index)
        {
            result = MULTITREE_INVALID_ARG;
            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
        }
        else
        {
            *childHandle = node->children[index];
            result = MULTITREE_OK;
        }
    }
    return result;
}

MULTITREE_RESULT MultiTree_GetName(MULTITREE_HANDLE treeHandle, STRING_HANDLE destination)
{
    MULTITREE_RESULT result;
    if (treeHandle == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else if (destination == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        MULTITREE_HANDLE_DATA *node = (MULTITREE_HANDLE_DATA*)treeHandle;
        if (node->name == NULL)
        {
            result = MULTITREE_EMPTY_CHILD_NAME;
            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
        }
        else if (STRING_concat(destination, node->name)!=0)
        {
            result = MULTITREE_ERROR;
            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
        }
        else
        {
            result = MULTITREE_OK;
        }
    }

    return result;
}

MULTITREE_RESULT MultiTree_GetChildByName(MULTITREE_HANDLE treeHandle, const char* childName, MULTITREE_HANDLE *childHandle)
{
    MULTITREE_RESULT result;

    if ((treeHandle == NULL) ||
        (childHandle == NULL) ||
        (childName == NULL))
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        MULTITREE_HANDLE_DATA * node = (MULTITREE_HANDLE_DATA *)treeHandle;
        size_t i;

        for (i = 0; i < node->nChildren; i++)
        {
            if (strcmp(node->children[i]->name, childName) == 0)
            {
                break;
            }
        }

        if (i == node->nChildren)
        {
            result = MULTITREE_CHILD_NOT_FOUND;
            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
        }
        else
        {
            *childHandle = node->children[i];

            result = MULTITREE_OK;
        }
    }
    return result;
}

MULTITREE_RESULT MultiTree_GetValue(MULTITREE_HANDLE treeHandle, const void** destination)
{
    MULTITREE_RESULT result;
    if (treeHandle == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else if (destination == NULL)
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        MULTITREE_HANDLE_DATA * node = (MULTITREE_HANDLE_DATA*)treeHandle;
        if (node->value == NULL)
        {
            result = MULTITREE_EMPTY_VALUE;
            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
        }
        else
        {
            *destination = node->value;
            result = MULTITREE_OK;
        }
    }
    return result;
}

MULTITREE_RESULT MultiTree_SetValue(MULTITREE_HANDLE treeHandle, void* value)
{
    MULTITREE_RESULT result;

    if ((treeHandle == NULL) ||
        (value == NULL))
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        MULTITREE_HANDLE_DATA * node = (MULTITREE_HANDLE_DATA*)treeHandle;
        if (node->value != NULL)
        {
            result = MULTITREE_ALREADY_HAS_A_VALUE;
            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
        }
        else
        {
            if (node->cloneFunction(&node->value, value) != 0)
            {
                result = MULTITREE_ERROR;
                LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
            }
            else
            {
                result = MULTITREE_OK;
            }
        }
    }
    return result;
}

void MultiTree_Destroy(MULTITREE_HANDLE treeHandle)
{
    if (treeHandle != NULL)
    {
        MULTITREE_HANDLE_DATA* node = (MULTITREE_HANDLE_DATA*)treeHandle;
        size_t i;
        for (i = 0; i < node->nChildren;i++)
        {
            MultiTree_Destroy(node->children[i]);
        }
        if (node->children != NULL)
        {
            free(node->children);
            node->children = NULL;
        }

        if (node->name != NULL)
        {
            free(node->name);
            node->name = NULL;
        }

        if (node->value != NULL)
        {
            node->freeFunction(node->value);
            node->value = NULL;
        }

        free(node);
    }
}

MULTITREE_RESULT MultiTree_GetLeafValue(MULTITREE_HANDLE treeHandle, const char* leafPath, const void** destination)
{
    MULTITREE_RESULT result;

    if ((treeHandle == NULL) ||
        (leafPath == NULL) ||
        (destination == NULL))
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else if (strlen(leafPath) == 0)
    {
        result = MULTITREE_EMPTY_CHILD_NAME;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        /*break the path into components*/
        /*find the first child name*/
        MULTITREE_HANDLE_DATA* node = (MULTITREE_HANDLE_DATA *)treeHandle;
        const char* pos = leafPath;
        const char * whereIsDelimiter;

        /*if first character is / then skip it*/
        if (*pos == '/')
        {
            pos++;
        }

        if (*pos == '\0')
        {
            result = MULTITREE_EMPTY_CHILD_NAME;
            LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
        }
        else
        {
            result = MULTITREE_OK;

            while (*pos != '\0')
            {
                size_t i;
                size_t childCount = node->nChildren;

                whereIsDelimiter = pos;

                while ((*whereIsDelimiter != '/') && (*whereIsDelimiter != '\0'))
                {
                    whereIsDelimiter++;
                }

                if (whereIsDelimiter == pos)
                {
                    result = MULTITREE_EMPTY_CHILD_NAME;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                    break;
                }
                else if (childCount == 0)
                {
                    result = MULTITREE_CHILD_NOT_FOUND;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                    break;
                }
                else
                {
                    for (i = 0; i < childCount; i++)
                    {
                        if (strncmp(node->children[i]->name, pos, whereIsDelimiter - pos) == 0)
                        {
                            node = node->children[i];
                            break;
                        }
                    }

                    if (i == childCount)
                    {
                        result = MULTITREE_CHILD_NOT_FOUND;
                        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                        break;
                    }
                    else
                    {
                        if (*whereIsDelimiter == '/')
                        {
                            pos = whereIsDelimiter + 1;
                        }
                        else
                        {
                            /* end of path */
                            pos = whereIsDelimiter;
                            break;
                        }
                    }
                }
            }

            if (*pos == 0)
            {
                if (node->value == NULL)
                {
                    result = MULTITREE_EMPTY_VALUE;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
                }
                else
                {
                    *destination = node->value;
                    result = MULTITREE_OK;
                }
            }
        }
    }
    return result;
}

MULTITREE_RESULT MultiTree_DeleteChild(MULTITREE_HANDLE treeHandle, const char* childName)
{
    MULTITREE_RESULT result;
    if ((treeHandle == NULL) ||
        (childName == NULL))
    {
        result = MULTITREE_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(MULTITREE_RESULT, result));
    }
    else
    {
        size_t i;
        size_t childToRemove = treeHandle->nChildren;
        MULTITREE_HANDLE treeToRemove = NULL;

        for (i = 0; i < treeHandle->nChildren; i++)
        {
            if (0 == strcmp(treeHandle->children[i]->name, childName))
            {
                childToRemove = i;
                treeToRemove = treeHandle->children[childToRemove];
                break;
            }
        }

        if (i == treeHandle->nChildren)
        {
            result = MULTITREE_CHILD_NOT_FOUND;
            // Don't log error; this function is best effort only.  Caller will determine actual error state.
        }
        else
        {
            for (i = childToRemove; i < treeHandle->nChildren - 1; i++)
            {
                treeHandle->children[i] = treeHandle->children[i+1];
            }

            MultiTree_Destroy(treeToRemove);

            // Even though this isn't reachable anymore after decrementing count, NULL out for cleanliness
            treeHandle->children[treeHandle->nChildren - 1] = NULL;
            treeHandle->nChildren = treeHandle->nChildren - 1;

            result = MULTITREE_OK;
        }
    }

    return result;
}

