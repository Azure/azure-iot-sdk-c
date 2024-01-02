// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"

#include <stddef.h>

#include "commanddecoder.h"
#include "multitree.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "schema.h"
#include "codefirst.h"
#include "jsondecoder.h"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(COMMANDDECODER_RESULT, COMMANDDECODER_RESULT_VALUES);

typedef struct COMMAND_DECODER_HANDLE_DATA_TAG
{
    METHOD_CALLBACK_FUNC methodCallback;
    void* methodCallbackContext;
    SCHEMA_MODEL_TYPE_HANDLE ModelHandle;
    ACTION_CALLBACK_FUNC ActionCallback;
    void* ActionCallbackContext;
} COMMAND_DECODER_HANDLE_DATA;

static int DecodeValueFromNode(SCHEMA_HANDLE schemaHandle, AGENT_DATA_TYPE* agentDataType, MULTITREE_HANDLE node, const char* edmTypeName)
{
    /* because "pottentially uninitialized variable on MS compiler" */
    int result = 0;
    const char* argStringValue;
    AGENT_DATA_TYPE_TYPE primitiveType;

    if ((primitiveType = CodeFirst_GetPrimitiveType(edmTypeName)) == EDM_NO_TYPE)
    {
        SCHEMA_STRUCT_TYPE_HANDLE structTypeHandle;
        size_t propertyCount;

        if (((structTypeHandle = Schema_GetStructTypeByName(schemaHandle, edmTypeName)) == NULL) ||
            (Schema_GetStructTypePropertyCount(structTypeHandle, &propertyCount) != SCHEMA_OK))
        {
            result = MU_FAILURE;
            LogError("Getting Struct information failed.");
        }
        else
        {
            if (propertyCount == 0)
            {
                result = MU_FAILURE;
                LogError("Struct type with 0 members is not allowed");
            }
            else
            {
                AGENT_DATA_TYPE* memberValues = (AGENT_DATA_TYPE*)calloc(1, (sizeof(AGENT_DATA_TYPE)* propertyCount));
                if (memberValues == NULL)
                {
                    result = MU_FAILURE;
                    LogError("Failed allocating member values for command argument");
                }
                else
                {
                    const char** memberNames = (const char**)malloc(sizeof(const char*)* propertyCount);
                    if (memberNames == NULL)
                    {
                        result = MU_FAILURE;
                        LogError("Failed allocating member names for command argument.");
                    }
                    else
                    {
                        size_t j;
                        size_t k;

                        for (j = 0; j < propertyCount; j++)
                        {
                            SCHEMA_PROPERTY_HANDLE propertyHandle;
                            MULTITREE_HANDLE memberNode;
                            const char* propertyName;
                            const char* propertyType;

                            if ((propertyHandle = Schema_GetStructTypePropertyByIndex(structTypeHandle, j)) == NULL)
                            {
                                result = MU_FAILURE;
                                LogError("Getting struct member failed.");
                                break;
                            }
                            else if (((propertyName = Schema_GetPropertyName(propertyHandle)) == NULL) ||
                                     ((propertyType = Schema_GetPropertyType(propertyHandle)) == NULL))
                            {
                                result = MU_FAILURE;
                                LogError("Getting the struct member information failed.");
                                break;
                            }
                            else
                            {
                                memberNames[j] = propertyName;

                                if (MultiTree_GetChildByName(node, memberNames[j], &memberNode) != MULTITREE_OK)
                                {
                                    result = MU_FAILURE;
                                    LogError("Getting child %s failed", propertyName);
                                    break;
                                }
                                else if ((result = DecodeValueFromNode(schemaHandle, &memberValues[j], memberNode, propertyType)) != 0)
                                {
                                    break;
                                }
                            }
                        }

                        if (j == propertyCount)
                        {
                            if (Create_AGENT_DATA_TYPE_from_Members(agentDataType, edmTypeName, propertyCount, (const char* const*)memberNames, memberValues) != AGENT_DATA_TYPES_OK)
                            {
                                result = MU_FAILURE;
                                LogError("Creating the agent data type from members failed.");
                            }
                            else
                            {
                                result = 0;
                            }
                        }

                        for (k = 0; k < j; k++)
                        {
                            Destroy_AGENT_DATA_TYPE(&memberValues[k]);
                        }

                        free((void*)memberNames);
                    }

                    free(memberValues);
                }
            }
        }
    }
    else
    {
        if (MultiTree_GetValue(node, (const void **)&argStringValue) != MULTITREE_OK)
        {
            result = MU_FAILURE;
            LogError("Getting the string from the multitree failed.");
        }
        else if (CreateAgentDataType_From_String(argStringValue, primitiveType, agentDataType) != AGENT_DATA_TYPES_OK)
        {
            result = MU_FAILURE;
            LogError("Failed parsing node %s.", argStringValue);
        }
    }

    return result;
}

static EXECUTE_COMMAND_RESULT DecodeAndExecuteModelAction(COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance, SCHEMA_HANDLE schemaHandle, SCHEMA_MODEL_TYPE_HANDLE modelHandle, const char* relativeActionPath, const char* actionName, MULTITREE_HANDLE commandNode)
{
    EXECUTE_COMMAND_RESULT result;
    char tempStr[128];
    size_t strLength = strlen(actionName);

    if (strLength <= 1)
    {
        LogError("Invalid action name");
        result = EXECUTE_COMMAND_ERROR;
    }
    else if (strLength >= 128)
    {
        LogError("Invalid action name length");
        result = EXECUTE_COMMAND_ERROR;
    }
    else
    {
        SCHEMA_ACTION_HANDLE modelActionHandle;
        size_t argCount;
        MULTITREE_HANDLE parametersTreeNode;

        if (memcpy(tempStr, actionName, strLength-1) == NULL)
        {
            LogError("Invalid action name.");
            result = EXECUTE_COMMAND_ERROR;
        }
        else if (MultiTree_GetChildByName(commandNode, "Parameters", &parametersTreeNode) != MULTITREE_OK)
        {
            LogError("Error getting Parameters node.");
            result = EXECUTE_COMMAND_ERROR;
        }
        else
        {
            tempStr[strLength-1] = 0;
            if (((modelActionHandle = Schema_GetModelActionByName(modelHandle, tempStr)) == NULL) ||
                (Schema_GetModelActionArgumentCount(modelActionHandle, &argCount) != SCHEMA_OK))
            {
                LogError("Failed reading action %s from the schema", tempStr);
                result = EXECUTE_COMMAND_ERROR;
            }
            else
            {
                AGENT_DATA_TYPE* arguments = NULL;

                if (argCount > 0)
                {
                    arguments = (AGENT_DATA_TYPE*)calloc(1, (sizeof(AGENT_DATA_TYPE)* argCount));
                }

                if ((argCount > 0) &&
                    (arguments == NULL))
                {
                    LogError("Failed allocating arguments array");
                    result = EXECUTE_COMMAND_ERROR;
                }
                else
                {
                    size_t i;
                    size_t j;
                    result = EXECUTE_COMMAND_ERROR;

                    for (i = 0; i < argCount; i++)
                    {
                        SCHEMA_ACTION_ARGUMENT_HANDLE actionArgumentHandle;
                        MULTITREE_HANDLE argumentNode;
                        const char* argName;
                        const char* argType;

                        if (((actionArgumentHandle = Schema_GetModelActionArgumentByIndex(modelActionHandle, i)) == NULL) ||
                            ((argName = Schema_GetActionArgumentName(actionArgumentHandle)) == NULL) ||
                            ((argType = Schema_GetActionArgumentType(actionArgumentHandle)) == NULL))
                        {
                            LogError("Failed getting the argument information from the schema");
                            result = EXECUTE_COMMAND_ERROR;
                            break;
                        }
                        else if (MultiTree_GetChildByName(parametersTreeNode, argName, &argumentNode) != MULTITREE_OK)
                        {
                            LogError("Missing argument %s", argName);
                            result = EXECUTE_COMMAND_ERROR;
                            break;
                        }
                        else if (DecodeValueFromNode(schemaHandle, &arguments[i], argumentNode, argType) != 0)
                        {
                            result = EXECUTE_COMMAND_ERROR;
                            break;
                        }
                    }

                    if (i == argCount)
                    {
                        result = commandDecoderInstance->ActionCallback(commandDecoderInstance->ActionCallbackContext, relativeActionPath, tempStr, argCount, arguments);
                    }

                    for (j = 0; j < i; j++)
                    {
                        Destroy_AGENT_DATA_TYPE(&arguments[j]);
                    }

                    if (arguments != NULL)
                    {
                        free(arguments);
                    }
                }
            }
        }
    }
    return result;
}

static METHODRETURN_HANDLE DecodeAndExecuteModelMethod(COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance, SCHEMA_HANDLE schemaHandle, SCHEMA_MODEL_TYPE_HANDLE modelHandle, const char* relativeMethodPath, const char* methodName, MULTITREE_HANDLE methodTree)
{
    METHODRETURN_HANDLE result;
    size_t strLength = strlen(methodName);

    if (strLength == 0)
    {
        LogError("Invalid method name");
        result = NULL;
    }
    else
    {
        SCHEMA_METHOD_HANDLE modelMethodHandle;
        size_t argCount;

#ifdef _MSC_VER
#pragma warning(suppress: 6324) /* We intentionally use here strncpy */
#endif

        if (((modelMethodHandle = Schema_GetModelMethodByName(modelHandle, methodName)) == NULL) ||
            (Schema_GetModelMethodArgumentCount(modelMethodHandle, &argCount) != SCHEMA_OK))
        {
            LogError("Failed reading method %s from the schema", methodName);
            result = NULL;
        }
        else
        {

            if (argCount == 0)
            {
                /*no need for any parameters*/
                result = commandDecoderInstance->methodCallback(commandDecoderInstance->methodCallbackContext, relativeMethodPath, methodName, 0, NULL);
            }
            else
            {
                AGENT_DATA_TYPE* arguments;
                arguments = (AGENT_DATA_TYPE*)calloc(1, (sizeof(AGENT_DATA_TYPE)* argCount));
                if (arguments == NULL)
                {
                    LogError("Failed allocating arguments array");
                    result = NULL;
                }
                else
                {
                    size_t i;
                    size_t j;
                    result = NULL;

                    for (i = 0; i < argCount; i++)
                    {
                        SCHEMA_METHOD_ARGUMENT_HANDLE methodArgumentHandle;
                        MULTITREE_HANDLE argumentNode;
                        const char* argName;
                        const char* argType;

                        if (((methodArgumentHandle = Schema_GetModelMethodArgumentByIndex(modelMethodHandle, i)) == NULL) ||
                            ((argName = Schema_GetMethodArgumentName(methodArgumentHandle)) == NULL) ||
                            ((argType = Schema_GetMethodArgumentType(methodArgumentHandle)) == NULL))
                        {
                            LogError("Failed getting the argument information from the schema");
                            result = NULL;
                            break;
                        }
                        else if (MultiTree_GetChildByName(methodTree, argName, &argumentNode) != MULTITREE_OK)
                        {
                            LogError("Missing argument %s", argName);
                            result = NULL;
                            break;
                        }
                        else if (DecodeValueFromNode(schemaHandle, &arguments[i], argumentNode, argType) != 0)
                        {
                            LogError("failure in DecodeValueFromNode");
                            result = NULL;
                            break;
                        }
                    }

                    if (i == argCount)
                    {
                        result = commandDecoderInstance->methodCallback(commandDecoderInstance->methodCallbackContext, relativeMethodPath, methodName, argCount, arguments);
                    }

                    for (j = 0; j < i; j++)
                    {
                        Destroy_AGENT_DATA_TYPE(&arguments[j]);
                    }

                    free(arguments);
                }

            }
        }

    }
    return result;
}


static EXECUTE_COMMAND_RESULT ScanActionPathAndExecuteAction(COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance, SCHEMA_HANDLE schemaHandle, const char* actionPath, MULTITREE_HANDLE commandNode)
{
    EXECUTE_COMMAND_RESULT result;
    char* relativeActionPath;
    const char* actionName = actionPath;
    SCHEMA_MODEL_TYPE_HANDLE modelHandle = commandDecoderInstance->ModelHandle;

    do
    {
        /* find the slash */
        const char* slashPos = strchr(actionName, '/');
        if (slashPos == NULL)
        {
            size_t relativeActionPathLength;

            if (actionName == actionPath)
            {
                relativeActionPathLength = 0;
            }
            else
            {
                relativeActionPathLength = actionName - actionPath - 1;
            }

            relativeActionPath = (char*)malloc(relativeActionPathLength + 1);
            if (relativeActionPath == NULL)
            {
                LogError("Failed allocating relative action path");
                result = EXECUTE_COMMAND_ERROR;
            }
            else
            {
                strncpy(relativeActionPath, actionPath, relativeActionPathLength);
                relativeActionPath[relativeActionPathLength] = 0;

                /* no slash found, this must be an action */
                result = DecodeAndExecuteModelAction(commandDecoderInstance, schemaHandle, modelHandle, relativeActionPath, actionName, commandNode);

                free(relativeActionPath);
                actionName = NULL;
            }
            break;
        }
        else
        {
            /* found a slash, get the child model name */
            size_t modelLength = slashPos - actionName;
            char* childModelName = (char*)malloc(modelLength + 1);
            if (childModelName == NULL)
            {
                LogError("Failed allocating child model name");
                result = EXECUTE_COMMAND_ERROR;
                break;
            }
            else
            {
                strncpy(childModelName, actionName, modelLength);
                childModelName[modelLength] = 0;

                /* find the model */
                modelHandle = Schema_GetModelModelByName(modelHandle, childModelName);
                if (modelHandle == NULL)
                {
                    LogError("Getting the model %s failed", childModelName);
                    free(childModelName);
                    result = EXECUTE_COMMAND_ERROR;
                    break;
                }
                else
                {
                    free(childModelName);
                    actionName = slashPos + 1;
                    result = EXECUTE_COMMAND_ERROR; /*this only exists to quench a compiler warning about returning an uninitialized variable, which is not possible by design*/
                }
            }
        }
    } while (actionName != NULL);
    return result;
}

static METHODRETURN_HANDLE ScanMethodPathAndExecuteMethod(COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance, SCHEMA_HANDLE schemaHandle, const char* fullMethodName, MULTITREE_HANDLE methodTree)
{
    METHODRETURN_HANDLE result;
    char* relativeMethodPath;
    const char* methodName = fullMethodName;
    SCHEMA_MODEL_TYPE_HANDLE modelHandle = commandDecoderInstance->ModelHandle;

    do
    {
        /* find the slash */
        const char* slashPos = strchr(methodName, '/');
        if (slashPos == NULL)
        {
            size_t relativeMethodPathLength;

            if (methodName == fullMethodName)
            {
                relativeMethodPathLength = 0;
            }
            else
            {
                relativeMethodPathLength = methodName - fullMethodName - 1;
            }

            relativeMethodPath = (char*)malloc(relativeMethodPathLength + 1);
            if (relativeMethodPath == NULL)
            {
                LogError("Failed allocating relative method path");
                result = NULL;
            }
            else
            {
                strncpy(relativeMethodPath, fullMethodName, relativeMethodPathLength);
                relativeMethodPath[relativeMethodPathLength] = 0;

                /* no slash found, this must be an method */
                result = DecodeAndExecuteModelMethod(commandDecoderInstance, schemaHandle, modelHandle, relativeMethodPath, methodName, methodTree);

                free(relativeMethodPath);
                methodName = NULL;
            }
            break;
        }
        else
        {
            /* found a slash, get the child model name */
            size_t modelLength = slashPos - methodName;
            char* childModelName = (char*)malloc(modelLength + 1);
            if (childModelName == NULL)
            {
                LogError("Failed allocating child model name");
                result = NULL;
                break;
            }
            else
            {
                strncpy(childModelName, methodName, modelLength);
                childModelName[modelLength] = 0;

                /* find the model */
                modelHandle = Schema_GetModelModelByName(modelHandle, childModelName);
                if (modelHandle == NULL)
                {
                    LogError("Getting the model %s failed", childModelName);
                    free(childModelName);
                    result = NULL;
                    break;
                }
                else
                {
                    free(childModelName);
                    methodName = slashPos + 1;
                    result = NULL; /*this only exists to quench a compiler warning about returning an uninitialized variable, which is not possible by design*/
                }
            }
        }
    } while (methodName != NULL);
    return result;
}

static EXECUTE_COMMAND_RESULT DecodeCommand(COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance, MULTITREE_HANDLE commandNode)
{
    EXECUTE_COMMAND_RESULT result;
    SCHEMA_HANDLE schemaHandle;

    if ((schemaHandle = Schema_GetSchemaForModelType(commandDecoderInstance->ModelHandle)) == NULL)
    {
        LogError("Getting schema information failed");
        result = EXECUTE_COMMAND_ERROR;
    }
    else
    {
        const char* actionName;
        MULTITREE_HANDLE nameTreeNode;

        if ((MultiTree_GetChildByName(commandNode, "Name", &nameTreeNode) != MULTITREE_OK) ||
            (MultiTree_GetValue(nameTreeNode, (const void **)&actionName) != MULTITREE_OK))
        {
            LogError("Getting action name failed.");
            result = EXECUTE_COMMAND_ERROR;
        }
        else if (strlen(actionName) < 2)
        {
            LogError("Invalid action name.");
            result = EXECUTE_COMMAND_ERROR;
        }
        else
        {
            actionName++;
            result = ScanActionPathAndExecuteAction(commandDecoderInstance, schemaHandle, actionName, commandNode);
        }
    }
    return result;
}

static METHODRETURN_HANDLE DecodeMethod(COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance, const char* fullMethodName, MULTITREE_HANDLE methodTree)
{
    METHODRETURN_HANDLE result;
    SCHEMA_HANDLE schemaHandle;

    if ((schemaHandle = Schema_GetSchemaForModelType(commandDecoderInstance->ModelHandle)) == NULL)
    {
        LogError("Getting schema information failed");
        result = NULL;
    }
    else
    {
        result = ScanMethodPathAndExecuteMethod(commandDecoderInstance, schemaHandle, fullMethodName, methodTree);

    }
    return result;
}

EXECUTE_COMMAND_RESULT CommandDecoder_ExecuteCommand(COMMAND_DECODER_HANDLE handle, const char* command)
{
    EXECUTE_COMMAND_RESULT result;
    COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance = (COMMAND_DECODER_HANDLE_DATA*)handle;
    if (
        (command == NULL) ||
        (commandDecoderInstance == NULL)
    )
    {
        LogError("Invalid argument, COMMAND_DECODER_HANDLE handle=%p, const char* command=%p", handle, command);
        result = EXECUTE_COMMAND_ERROR;
    }
    else
    {
        size_t size = strlen(command);
        char* commandJSON;

        if (size == 0)
        {
            LogError("Failed because command size is zero");
            result = EXECUTE_COMMAND_ERROR;
        }
        else if ((commandJSON = (char*)malloc(size + 1)) == NULL)
        {
            LogError("Failed to allocate temporary storage for the commands JSON");
            result = EXECUTE_COMMAND_ERROR;
        }
        else
        {
            MULTITREE_HANDLE commandsTree;

            (void)memcpy(commandJSON, command, size);
            commandJSON[size] = '\0';

            if (JSONDecoder_JSON_To_MultiTree(commandJSON, &commandsTree) != JSON_DECODER_OK)
            {
                LogError("Decoding JSON to a multi tree failed");
                result = EXECUTE_COMMAND_ERROR;
            }
            else
            {
                result = DecodeCommand(commandDecoderInstance, commandsTree);

                MultiTree_Destroy(commandsTree);
            }

            free(commandJSON);
        }
    }
    return result;
}

METHODRETURN_HANDLE CommandDecoder_ExecuteMethod(COMMAND_DECODER_HANDLE handle, const char* fullMethodName, const char* methodPayload)
{
    METHODRETURN_HANDLE result;
    if (
        (handle == NULL) ||
        (fullMethodName == NULL) /*methodPayload can be NULL*/
        )
    {
        LogError("Invalid argument, COMMAND_DECODER_HANDLE handle=%p, const char* fullMethodName=%p", handle, fullMethodName);
        result = NULL;
    }
    else
    {
        COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance = (COMMAND_DECODER_HANDLE_DATA*)handle;
        if (commandDecoderInstance->methodCallback == NULL)
        {
            LogError("unable to execute a method when the methodCallback passed in CommandDecoder_Create is NULL");
            result = NULL;
        }
        else
        {
            if (methodPayload == NULL)
            {
                result = DecodeMethod(commandDecoderInstance, fullMethodName, NULL);
            }
            else
            {
                char* methodJSON;

                if (mallocAndStrcpy_s(&methodJSON, methodPayload) != 0)
                {
                    LogError("Failed to allocate temporary storage for the method JSON");
                    result = NULL;
                }
                else
                {
                    MULTITREE_HANDLE methodTree;
                    if (JSONDecoder_JSON_To_MultiTree(methodJSON, &methodTree) != JSON_DECODER_OK)
                    {
                        LogError("Decoding JSON to a multi tree failed");
                        result = NULL;
                    }
                    else
                    {
                        result = DecodeMethod(commandDecoderInstance, fullMethodName, methodTree);
                        MultiTree_Destroy(methodTree);
                    }
                    free(methodJSON);
                }
            }
        }
    }
    return result;
}


COMMAND_DECODER_HANDLE CommandDecoder_Create(SCHEMA_MODEL_TYPE_HANDLE modelHandle, ACTION_CALLBACK_FUNC actionCallback, void* actionCallbackContext, METHOD_CALLBACK_FUNC methodCallback, void* methodCallbackContext)
{
    COMMAND_DECODER_HANDLE_DATA* result;
    if (modelHandle == NULL)
    {
        LogError("Invalid arguments: SCHEMA_MODEL_TYPE_HANDLE modelHandle=%p, ACTION_CALLBACK_FUNC actionCallback=%p, void* actionCallbackContext=%p, METHOD_CALLBACK_FUNC methodCallback=%p, void* methodCallbackContext=%p",
            modelHandle, actionCallback, actionCallbackContext, methodCallback, methodCallbackContext);
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(COMMAND_DECODER_HANDLE_DATA));
        if (result == NULL)
        {
            /*return as is*/
        }
        else
        {
            result->ModelHandle = modelHandle;
            result->ActionCallback = actionCallback;
            result->ActionCallbackContext = actionCallbackContext;
            result->methodCallback = methodCallback;
            result->methodCallbackContext = methodCallbackContext;
        }
    }

    return result;
}

void CommandDecoder_Destroy(COMMAND_DECODER_HANDLE commandDecoderHandle)
{
    if (commandDecoderHandle != NULL)
    {
        COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance = (COMMAND_DECODER_HANDLE_DATA*)commandDecoderHandle;

        free(commandDecoderInstance);
    }
}

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(AGENT_DATA_TYPE_TYPE, AGENT_DATA_TYPE_TYPE_VALUES);

/*validates that the multitree (coming from a JSON) is actually a serialization of the model (complete or incomplete)*/
/*if the serialization contains more than the model, then it fails.*/
/*if the serialization does not contain mandatory items from the model, it fails*/
static bool validateModel_vs_Multitree(void* startAddress, SCHEMA_MODEL_TYPE_HANDLE modelHandle, MULTITREE_HANDLE desiredPropertiesTree, size_t offset)
{

    bool result;
    size_t nChildren;
    size_t nProcessedChildren = 0;
    (void)MultiTree_GetChildCount(desiredPropertiesTree, &nChildren);
    for (size_t i = 0;i < nChildren;i++)
    {
        MULTITREE_HANDLE child;
        if (MultiTree_GetChild(desiredPropertiesTree, i, &child) != MULTITREE_OK)
        {
            LogError("failure in MultiTree_GetChild");
            i = nChildren;
        }
        else
        {
            STRING_HANDLE childName = STRING_new();
            if (childName == NULL)
            {
                LogError("failure to STRING_new");
                i = nChildren;
            }
            else
            {
                if (MultiTree_GetName(child, childName) != MULTITREE_OK)
                {
                    LogError("failure to MultiTree_GetName");
                    i = nChildren;
                }
                else
                {
                    const char *childName_str = STRING_c_str(childName);
                    SCHEMA_MODEL_ELEMENT elementType = Schema_GetModelElementByName(modelHandle, childName_str);
                    switch (elementType.elementType)
                    {
                        default:
                        {
                            LogError("INTERNAL ERROR: unexpected function return");
                            i = nChildren;
                            break;
                        }
                        case (SCHEMA_PROPERTY):
                        {
                            LogError("cannot ingest name (WITH_DATA instead of WITH_DESIRED_PROPERTY): %s", childName_str);
                            i = nChildren;
                            break;
                        }
                        case (SCHEMA_REPORTED_PROPERTY):
                        {
                            LogError("cannot ingest name (WITH_REPORTED_PROPERTY instead of WITH_DESIRED_PROPERTY): %s", childName_str);
                            i = nChildren;
                            break;
                        }
                        case (SCHEMA_DESIRED_PROPERTY):
                        {
                            SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle = elementType.elementHandle.desiredPropertyHandle;

                            const char* desiredPropertyType = Schema_GetModelDesiredPropertyType(desiredPropertyHandle);
                            AGENT_DATA_TYPE output;
                            if (DecodeValueFromNode(Schema_GetSchemaForModelType(modelHandle), &output, child, desiredPropertyType) != 0)
                            {
                                LogError("failure in DecodeValueFromNode");
                                i = nChildren;
                            }
                            else
                            {
                                pfDesiredPropertyFromAGENT_DATA_TYPE leFunction = Schema_GetModelDesiredProperty_pfDesiredPropertyFromAGENT_DATA_TYPE(desiredPropertyHandle);
                                if (leFunction(&output, (char*)startAddress + offset + Schema_GetModelDesiredProperty_offset(desiredPropertyHandle)) != 0)
                                {
                                    LogError("failure in a function that converts from AGENT_DATA_TYPE to C data");
                                }
                                else
                                {
                                    pfOnDesiredProperty onDesiredProperty = Schema_GetModelDesiredProperty_pfOnDesiredProperty(desiredPropertyHandle);
                                    if (onDesiredProperty != NULL)
                                    {
                                        onDesiredProperty((char*)startAddress + offset);
                                    }
                                    nProcessedChildren++;
                                }
                                Destroy_AGENT_DATA_TYPE(&output);
                            }

                            break;
                        }
                        case(SCHEMA_MODEL_IN_MODEL):
                        {
                            SCHEMA_MODEL_TYPE_HANDLE modelModel = elementType.elementHandle.modelHandle;

                            if (!validateModel_vs_Multitree(startAddress, modelModel, child, offset + Schema_GetModelModelByName_Offset(modelHandle, childName_str)))
                            {
                                LogError("failure in validateModel_vs_Multitree");
                                i = nChildren;
                            }
                            else
                            {
                                /*if the model in model so happened to be a WITH_DESIRED_PROPERTY... (only those has non_NULL pfOnDesiredProperty) */
                                pfOnDesiredProperty onDesiredProperty = Schema_GetModelModelByName_OnDesiredProperty(modelHandle, childName_str);
                                if (onDesiredProperty != NULL)
                                {
                                    onDesiredProperty((char*)startAddress + offset);
                                }

                                nProcessedChildren++;
                            }

                            break;
                        }

                    } /*switch*/
                }
                STRING_delete(childName);
            }
        }
    }

    if(nProcessedChildren == nChildren)
    {
        result = true;
    }
    else
    {
        LogError("not all constituents of the JSON have been ingested");
        result = false;
    }
    return result;
}

static EXECUTE_COMMAND_RESULT DecodeDesiredProperties(void* startAddress, COMMAND_DECODER_HANDLE_DATA* handle, MULTITREE_HANDLE desiredPropertiesTree)
{
    return validateModel_vs_Multitree(startAddress, handle->ModelHandle, desiredPropertiesTree, 0 )?EXECUTE_COMMAND_SUCCESS:EXECUTE_COMMAND_FAILED;
}

/* Raw JSON has properties we don't need; potentially nodes other than "desired" for full TWIN as well as a $version we don't pass to callees */
static bool RemoveUnneededTwinProperties(MULTITREE_HANDLE initialParsedTree, bool parseDesiredNode, MULTITREE_HANDLE *desiredPropertiesTree)
{
    MULTITREE_HANDLE updateTree;
    bool result;

    if (parseDesiredNode)
    {
        if (MultiTree_GetChildByName(initialParsedTree, "desired", &updateTree) != MULTITREE_OK)
        {
            LogError("Unable to find 'desired' in tree");
            return false;
        }
    }
    else
    {
        // Tree already starts on node we want so just use it.
        updateTree = initialParsedTree;
    }

    MULTITREE_RESULT deleteChildResult = MultiTree_DeleteChild(updateTree, "$version");
    if ((deleteChildResult == MULTITREE_OK) || (deleteChildResult == MULTITREE_CHILD_NOT_FOUND))
    {
        *desiredPropertiesTree = updateTree;
        result = true;
    }
    else
    {
        *desiredPropertiesTree = NULL;
        result = false;
    }

    return result;
}

EXECUTE_COMMAND_RESULT CommandDecoder_IngestDesiredProperties(void* startAddress, COMMAND_DECODER_HANDLE handle, const char* jsonPayload, bool parseDesiredNode)
{
    EXECUTE_COMMAND_RESULT result;

    if(
        (startAddress == NULL) ||
        (handle == NULL) ||
        (jsonPayload == NULL)
        )
    {
        LogError("invalid argument COMMAND_DECODER_HANDLE handle=%p, const char* jsonPayload=%p", handle, jsonPayload);
        result = EXECUTE_COMMAND_ERROR;
    }
    else
    {
        char* copy;
        if (mallocAndStrcpy_s(&copy, jsonPayload) != 0)
        {
            LogError("failure in mallocAndStrcpy_s");
            result = EXECUTE_COMMAND_FAILED;
        }
        else
        {
            MULTITREE_HANDLE initialParsedTree;
            MULTITREE_HANDLE desiredPropertiesTree;

            if (JSONDecoder_JSON_To_MultiTree(copy, &initialParsedTree) != JSON_DECODER_OK)
            {
                LogError("Decoding JSON to a multi tree failed");
                result = EXECUTE_COMMAND_ERROR;
            }
            else
            {
                if (RemoveUnneededTwinProperties(initialParsedTree, parseDesiredNode, &desiredPropertiesTree) == false)
                {
                    LogError("Removing unneeded twin properties failed");
                    result = EXECUTE_COMMAND_ERROR;
                }
                else
                {
                    COMMAND_DECODER_HANDLE_DATA* commandDecoderInstance = (COMMAND_DECODER_HANDLE_DATA*)handle;

                    result = DecodeDesiredProperties(startAddress, commandDecoderInstance, desiredPropertiesTree);

                    // Do NOT free desiredPropertiesTree.  It is only a pointer into initialParsedTree.
                    MultiTree_Destroy(initialParsedTree);
                }
            }
            free(copy);
        }
    }
    return result;
}
