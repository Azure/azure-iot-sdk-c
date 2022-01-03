// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"

#include "schema.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"


MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(SCHEMA_RESULT, SCHEMA_RESULT_VALUES);

typedef struct SCHEMA_PROPERTY_HANDLE_DATA_TAG
{
    const char* PropertyName;
    const char* PropertyType;
} SCHEMA_PROPERTY_HANDLE_DATA;

typedef struct SCHEMA_REPORTED_PROPERTY_HANDLE_DATA_TAG
{
    const char* reportedPropertyName;
    const char* reportedPropertyType;
} SCHEMA_REPORTED_PROPERTY_HANDLE_DATA;

typedef struct SCHEMA_DESIRED_PROPERTY_HANDLE_DATA_TAG
{
    pfOnDesiredProperty onDesiredProperty;
    pfDesiredPropertyInitialize desiredPropertInitialize;
    pfDesiredPropertyDeinitialize desiredPropertDeinitialize;
    const char* desiredPropertyName;
    const char* desiredPropertyType;
    pfDesiredPropertyFromAGENT_DATA_TYPE desiredPropertyFromAGENT_DATA_TYPE;
    size_t offset;
} SCHEMA_DESIRED_PROPERTY_HANDLE_DATA;

typedef struct SCHEMA_ACTION_ARGUMENT_HANDLE_DATA_TAG
{
    const char* Name;
    const char* Type;
} SCHEMA_ACTION_ARGUMENT_HANDLE_DATA;

typedef struct SCHEMA_METHOD_ARGUMENT_HANDLE_DATA_TAG
{
    char* Name;
    char* Type;
} SCHEMA_METHOD_ARGUMENT_HANDLE_DATA;

typedef struct SCHEMA_ACTION_HANDLE_DATA_TAG
{
    const char* ActionName;
    size_t ArgumentCount;
    SCHEMA_ACTION_ARGUMENT_HANDLE* ArgumentHandles;
} SCHEMA_ACTION_HANDLE_DATA;

typedef struct SCHEMA_METHOD_HANDLE_DATA_TAG
{
    char* methodName;
    VECTOR_HANDLE methodArguments; /*holds SCHEMA_METHOD_ARGUMENT_HANDLE*/
} SCHEMA_METHOD_HANDLE_DATA;

typedef struct MODEL_IN_MODEL_TAG
{
    pfOnDesiredProperty onDesiredProperty; /*is NULL if not specified or if the model in model is not WITH_DESIRED_PROPERTY*/
    size_t offset; /*offset of the model in model (offsetof)*/
    const char* propertyName;
    SCHEMA_MODEL_TYPE_HANDLE modelHandle;
} MODEL_IN_MODEL;

typedef struct SCHEMA_MODEL_TYPE_HANDLE_DATA_TAG
{
    VECTOR_HANDLE methods; /*holds SCHEMA_METHOD_HANDLE*/
    VECTOR_HANDLE desiredProperties; /*holds SCHEMA_DESIRED_PROPERTY_HANDLE_DATA*/
    const char* Name;
    SCHEMA_HANDLE SchemaHandle;
    SCHEMA_PROPERTY_HANDLE* Properties;
    size_t PropertyCount;
    VECTOR_HANDLE reportedProperties; /*holds SCHEMA_REPORTED_PROPERTY_HANDLE*/
    SCHEMA_ACTION_HANDLE* Actions;
    size_t ActionCount;
    VECTOR_HANDLE models;
    size_t DeviceCount;
} SCHEMA_MODEL_TYPE_HANDLE_DATA;

typedef struct SCHEMA_STRUCT_TYPE_HANDLE_DATA_TAG
{
    const char* Name;
    SCHEMA_PROPERTY_HANDLE* Properties;
    size_t PropertyCount;
} SCHEMA_STRUCT_TYPE_HANDLE_DATA;

typedef struct SCHEMA_HANDLE_DATA_TAG
{
    void* metadata;
    const char* Namespace;
    SCHEMA_MODEL_TYPE_HANDLE* ModelTypes;
    size_t ModelTypeCount;
    SCHEMA_STRUCT_TYPE_HANDLE* StructTypes;
    size_t StructTypeCount;
} SCHEMA_HANDLE_DATA;

static VECTOR_HANDLE g_schemas = NULL;

static void DestroyProperty(SCHEMA_PROPERTY_HANDLE propertyHandle)
{
    SCHEMA_PROPERTY_HANDLE_DATA* propertyType = (SCHEMA_PROPERTY_HANDLE_DATA*)propertyHandle;
    free((void*)propertyType->PropertyName);
    free((void*)propertyType->PropertyType);
    free(propertyType);
}

static void DestroyActionArgument(SCHEMA_ACTION_ARGUMENT_HANDLE actionArgumentHandle)
{
    SCHEMA_ACTION_ARGUMENT_HANDLE_DATA* actionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE_DATA*)actionArgumentHandle;
    if (actionArgument != NULL)
    {
        free((void*)actionArgument->Name);
        free((void*)actionArgument->Type);
        free(actionArgument);
    }
}

static void DestroyMethodArgument(SCHEMA_METHOD_ARGUMENT_HANDLE methodArgumentHandle)
{
    free(methodArgumentHandle->Name);
    free(methodArgumentHandle->Type);
    free(methodArgumentHandle);
}

static void DestroyAction(SCHEMA_ACTION_HANDLE actionHandle)
{
    SCHEMA_ACTION_HANDLE_DATA* action = (SCHEMA_ACTION_HANDLE_DATA*)actionHandle;
    if (action != NULL)
    {
        size_t j;

        for (j = 0; j < action->ArgumentCount; j++)
        {
            DestroyActionArgument(action->ArgumentHandles[j]);
        }
        free(action->ArgumentHandles);

        free((void*)action->ActionName);
        free(action);
    }
}

static void DestroyMethod(SCHEMA_METHOD_HANDLE methodHandle)
{
    size_t nArguments = VECTOR_size(methodHandle->methodArguments);

    for (size_t j = 0; j < nArguments; j++)
    {
        SCHEMA_METHOD_ARGUMENT_HANDLE* methodArgumentHandle = VECTOR_element(methodHandle->methodArguments, j);
        if (methodArgumentHandle != NULL)
        {
            DestroyMethodArgument(*methodArgumentHandle);
        }
    }
    free(methodHandle->methodName);
    VECTOR_destroy(methodHandle->methodArguments);
    free(methodHandle);
}

static void DestroyMethods(SCHEMA_MODEL_TYPE_HANDLE modelHandle)
{
    size_t nMethods = VECTOR_size(modelHandle->methods);

    for (size_t j = 0; j < nMethods; j++)
    {
        SCHEMA_METHOD_HANDLE* methodHandle = VECTOR_element(modelHandle->methods, j);
        if (methodHandle != NULL)
        {
            DestroyMethod(*methodHandle);
        }
    }
    VECTOR_destroy(modelHandle->methods);
}


static void DestroyStruct(SCHEMA_STRUCT_TYPE_HANDLE structTypeHandle)
{
    size_t i;
    SCHEMA_STRUCT_TYPE_HANDLE_DATA* structType = (SCHEMA_STRUCT_TYPE_HANDLE_DATA*)structTypeHandle;
    if (structType != NULL)
    {
        for (i = 0; i < structType->PropertyCount; i++)
        {
            DestroyProperty(structType->Properties[i]);
        }
        free(structType->Properties);

        free((void*)structType->Name);

        free(structType);
    }
}

static void DestroyModel(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle)
{
    SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
    size_t i, nReportedProperties;

    free((void*)modelType->Name);
    modelType->Name = NULL;

    for (i = 0; i < modelType->PropertyCount; i++)
    {
        DestroyProperty(modelType->Properties[i]);
    }

    free(modelType->Properties);

    for (i = 0; i < modelType->ActionCount; i++)
    {
        DestroyAction(modelType->Actions[i]);
    }

    DestroyMethods(modelType);

    /*destroy the vector holding the added models. This does not destroy the said models, however, their names shall be*/
    for (i = 0; i < VECTOR_size(modelType->models); i++)
    {
        MODEL_IN_MODEL* temp = (MODEL_IN_MODEL*)VECTOR_element(modelType->models, i);
        if (temp != NULL)
        {
            free((void*)temp->propertyName);
        }
    }

    nReportedProperties = VECTOR_size(modelType->reportedProperties);
    for (i = 0;i < nReportedProperties;i++)
    {
        SCHEMA_REPORTED_PROPERTY_HANDLE_DATA* reportedProperty = *(SCHEMA_REPORTED_PROPERTY_HANDLE_DATA **)VECTOR_element(modelType->reportedProperties, i);
        free((void*)reportedProperty->reportedPropertyName);
        free((void*)reportedProperty->reportedPropertyType);
        free(reportedProperty);
    }
    VECTOR_destroy(modelType->reportedProperties);

    size_t nDesiredProperties = VECTOR_size(modelType->desiredProperties);
    for (i = 0;i < nDesiredProperties;i++)
    {
        SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* desiredProperty = *(SCHEMA_DESIRED_PROPERTY_HANDLE_DATA **)VECTOR_element(modelType->desiredProperties, i);
        free((void*)desiredProperty->desiredPropertyName);
        free((void*)desiredProperty->desiredPropertyType);
        free(desiredProperty);
    }
    VECTOR_destroy(modelType->desiredProperties);

    VECTOR_clear(modelType->models);
    VECTOR_destroy(modelType->models);

    free(modelType->Actions);
    free(modelType);
}

static SCHEMA_RESULT AddModelProperty(SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType, const char* name, const char* type)
{
    SCHEMA_RESULT result;

    if ((modelType == NULL) ||
        (name == NULL) ||
        (type == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        size_t i;

        for (i = 0; i < modelType->PropertyCount; i++)
        {
            SCHEMA_PROPERTY_HANDLE_DATA* property = (SCHEMA_PROPERTY_HANDLE_DATA*)modelType->Properties[i];
            if (strcmp(property->PropertyName, name) == 0)
            {
                break;
            }
        }

        if (i < modelType->PropertyCount)
        {
            result = SCHEMA_DUPLICATE_ELEMENT;
            LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
        }
        else
        {
            SCHEMA_PROPERTY_HANDLE* newProperties = (SCHEMA_PROPERTY_HANDLE*)realloc(modelType->Properties, sizeof(SCHEMA_PROPERTY_HANDLE) * (modelType->PropertyCount + 1));
            if (newProperties == NULL)
            {
                result = SCHEMA_ERROR;
                LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
            }
            else
            {
                SCHEMA_PROPERTY_HANDLE_DATA* newProperty;

                modelType->Properties = newProperties;
                if ((newProperty = (SCHEMA_PROPERTY_HANDLE_DATA*)malloc(sizeof(SCHEMA_PROPERTY_HANDLE_DATA))) == NULL)
                {
                    result = SCHEMA_ERROR;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                }
                else
                {
                    if (mallocAndStrcpy_s((char**)&newProperty->PropertyName, name) != 0)
                    {
                        free(newProperty);
                        result = SCHEMA_ERROR;
                        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                    }
                    else if (mallocAndStrcpy_s((char**)&newProperty->PropertyType, type) != 0)
                    {
                        free((void*)newProperty->PropertyName);
                        free(newProperty);
                        result = SCHEMA_ERROR;
                        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                    }
                    else
                    {
                        modelType->Properties[modelType->PropertyCount] = (SCHEMA_PROPERTY_HANDLE)newProperty;
                        modelType->PropertyCount++;

                        result = SCHEMA_OK;
                    }
                }

                /* If possible, reduce the memory of over allocation */
                if (result != SCHEMA_OK)
                {
                    if (modelType->PropertyCount > 0)
                    {
                        SCHEMA_PROPERTY_HANDLE *oldProperties = (SCHEMA_PROPERTY_HANDLE *)realloc(modelType->Properties, sizeof(SCHEMA_PROPERTY_HANDLE) * modelType->PropertyCount);
                        if (oldProperties == NULL)
                        {
                            result = SCHEMA_ERROR;
                            LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                        }
                        else
                        {
                            modelType->Properties = oldProperties;
                        }
                    }
                    else
                    {
                        modelType->Properties = NULL;
                    }
                }
            }
        }
    }

    return result;
}

static bool SchemaHandlesMatch(const SCHEMA_HANDLE* handle, const SCHEMA_HANDLE* otherHandle)
{
    return (*handle == *otherHandle);
}

static bool SchemaNamespacesMatch(const SCHEMA_HANDLE* handle, const char* schemaNamespace)
{
    const SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)*handle;
    return (strcmp(schema->Namespace, schemaNamespace) == 0);
}

SCHEMA_HANDLE Schema_Create(const char* schemaNamespace, void* metadata)
{
    SCHEMA_HANDLE_DATA* result;
    if (
        (schemaNamespace == NULL)||
        (metadata == NULL)
        )
    {
        LogError("invalid arg const char* schemaNamespace=%p, void* metadata=%p",schemaNamespace, metadata);
        result = NULL;
    }
    else
    {
        if (g_schemas == NULL && (g_schemas = VECTOR_create(sizeof(SCHEMA_HANDLE_DATA*) ) ) == NULL)
        {
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
        }
        else if ((result = (SCHEMA_HANDLE_DATA*)calloc(1, sizeof(SCHEMA_HANDLE_DATA))) == NULL)
        {
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
        }
        else if (mallocAndStrcpy_s((char**)&result->Namespace, schemaNamespace) != 0)
        {
            free(result);
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
        }
        else if (VECTOR_push_back(g_schemas, &result, 1) != 0)
        {
            free((void*)result->Namespace);
            free(result);
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
        }
        else
        {
            result->ModelTypes = NULL;
            result->ModelTypeCount = 0;
            result->StructTypes = NULL;
            result->StructTypeCount = 0;
            result->metadata = metadata;
        }
    }

    return (SCHEMA_HANDLE)result;
}

size_t Schema_GetSchemaCount(void)
{
    return VECTOR_size(g_schemas);
}

SCHEMA_HANDLE Schema_GetSchemaByNamespace(const char* schemaNamespace)
{
    SCHEMA_HANDLE result = NULL;

    if (schemaNamespace != NULL)
    {
        SCHEMA_HANDLE* handle = (g_schemas==NULL)?NULL:(SCHEMA_HANDLE*)VECTOR_find_if(g_schemas, (PREDICATE_FUNCTION)SchemaNamespacesMatch, schemaNamespace);
        if (handle != NULL)
        {
            result = *handle;
        }
    }

    return result;
}

const char* Schema_GetSchemaNamespace(SCHEMA_HANDLE schemaHandle)
{
    const char* result;

    if (schemaHandle == NULL)
    {
        result = NULL;
    }
    else
    {
        result = ((SCHEMA_HANDLE_DATA*)schemaHandle)->Namespace;
    }

    return result;
}

void Schema_Destroy(SCHEMA_HANDLE schemaHandle)
{
    if (schemaHandle != NULL)
    {
        SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;
        size_t i;

        for (i = 0; i < schema->ModelTypeCount; i++)
        {
            DestroyModel(schema->ModelTypes[i]);
        }

        free(schema->ModelTypes);

        for (i = 0; i < schema->StructTypeCount; i++)
        {
            DestroyStruct(schema->StructTypes[i]);
        }

        free(schema->StructTypes);
        free((void*)schema->Namespace);
        free(schema);

        schema = (SCHEMA_HANDLE_DATA*)VECTOR_find_if(g_schemas, (PREDICATE_FUNCTION)SchemaHandlesMatch, &schemaHandle);
        if (schema != NULL)
        {
            VECTOR_erase(g_schemas, schema, 1);
        }
        // If the g_schema is empty then destroy it
        if (VECTOR_size(g_schemas) == 0)
        {
            VECTOR_destroy(g_schemas);
            g_schemas = NULL;
        }
    }
}

SCHEMA_RESULT Schema_DestroyIfUnused(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle)
{
    SCHEMA_RESULT result;

    if (modelTypeHandle != NULL)
    {
        SCHEMA_HANDLE schemaHandle = Schema_GetSchemaForModelType(modelTypeHandle);
        if (schemaHandle == NULL)
        {
            result = SCHEMA_ERROR;
        }
        else
        {
            SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;
            size_t nIndex;

            for (nIndex = 0; nIndex < schema->ModelTypeCount; nIndex++)
            {
                SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)schema->ModelTypes[nIndex];
                if (modelType->DeviceCount > 0)
                    break;
            }
            if (nIndex == schema->ModelTypeCount)
            {
                Schema_Destroy(schemaHandle);
                result = SCHEMA_OK;
            }
            else
            {
                result = SCHEMA_MODEL_IN_USE;
            }
        }
    }
    else
    {
        result = SCHEMA_INVALID_ARG;
    }
    return result;
}

SCHEMA_RESULT Schema_AddDeviceRef(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle)
{
    SCHEMA_RESULT result;
    if (modelTypeHandle == NULL)
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        model->DeviceCount++;
        result = SCHEMA_OK;
    }
    return result;
}

SCHEMA_RESULT Schema_ReleaseDeviceRef(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle)
{
    SCHEMA_RESULT result;
    if (modelTypeHandle == NULL)
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        if (model->DeviceCount > 0)
        {
            model->DeviceCount--;
            result = SCHEMA_OK;
        }
        else
        {
result = SCHEMA_DEVICE_COUNT_ZERO;
LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
        }
    }
    return result;
}

SCHEMA_MODEL_TYPE_HANDLE Schema_CreateModelType(SCHEMA_HANDLE schemaHandle, const char* modelName)
{
    SCHEMA_MODEL_TYPE_HANDLE result;

    if ((schemaHandle == NULL) ||
        (modelName == NULL))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;

        size_t i;
        for (i = 0; i < schema->ModelTypeCount; i++)
        {
            SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)(schema->ModelTypes[i]);
            if (strcmp(model->Name, modelName) == 0)
            {
                break;
            }
        }

        if (i < schema->ModelTypeCount)
        {
            result = NULL;
            LogError("%s Model Name already exists", modelName);
        }

        else
        {
            SCHEMA_MODEL_TYPE_HANDLE* newModelTypes = (SCHEMA_MODEL_TYPE_HANDLE*)realloc(schema->ModelTypes, sizeof(SCHEMA_MODEL_TYPE_HANDLE) * (schema->ModelTypeCount + 1));
            if (newModelTypes == NULL)
            {
                result = NULL;
                LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
            }
            else
            {
                SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType;
                schema->ModelTypes = newModelTypes;

                if ((modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)calloc(1, sizeof(SCHEMA_MODEL_TYPE_HANDLE_DATA))) == NULL)
                {

                    result = NULL;
                    LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                }
                else if (mallocAndStrcpy_s((char**)&modelType->Name, modelName) != 0)
                {
                    result = NULL;
                    free(modelType);
                    LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                }
                else
                {
                    modelType->models = VECTOR_create(sizeof(MODEL_IN_MODEL));
                    if (modelType->models == NULL)
                    {
                        LogError("unable to VECTOR_create");
                        free((void*)modelType->Name);
                        free((void*)modelType);
                        result = NULL;
                    }
                    else
                    {
                        if ((modelType->reportedProperties = VECTOR_create(sizeof(SCHEMA_REPORTED_PROPERTY_HANDLE))) == NULL)
                        {
                            LogError("failed to VECTOR_create (reported properties)");
                            VECTOR_destroy(modelType->models);
                            free((void*)modelType->Name);
                            free((void*)modelType);
                            result = NULL;

                        }
                        else
                        {
                            if ((modelType->desiredProperties = VECTOR_create(sizeof(SCHEMA_DESIRED_PROPERTY_HANDLE))) == NULL)
                            {
                                LogError("failure in VECTOR_create (desired properties)");
                                VECTOR_destroy(modelType->reportedProperties);
                                VECTOR_destroy(modelType->models);
                                free((void*)modelType->Name);
                                free((void*)modelType);
                                result = NULL;
                            }
                            else
                            {
                                if ((modelType->methods = VECTOR_create(sizeof(SCHEMA_METHOD_HANDLE))) == NULL)
                                {
                                    LogError("failure in VECTOR_create (desired properties)");
                                    VECTOR_destroy(modelType->desiredProperties);
                                    VECTOR_destroy(modelType->reportedProperties);
                                    VECTOR_destroy(modelType->models);
                                    free((void*)modelType->Name);
                                    free((void*)modelType);
                                    result = NULL;
                                }
                                else
                                {
                                    modelType->PropertyCount = 0;
                                    modelType->Properties = NULL;
                                    modelType->ActionCount = 0;
                                    modelType->Actions = NULL;
                                    modelType->SchemaHandle = schemaHandle;
                                    modelType->DeviceCount = 0;

                                    schema->ModelTypes[schema->ModelTypeCount] = modelType;
                                    schema->ModelTypeCount++;
                                    result = (SCHEMA_MODEL_TYPE_HANDLE)modelType;
                                }
                            }
                        }
                    }
                }

                /* If possible, reduce the memory of over allocation */
                if ((result == NULL) &&(schema->ModelTypeCount>0))
                {
                    SCHEMA_MODEL_TYPE_HANDLE* oldModelTypes = (SCHEMA_MODEL_TYPE_HANDLE*)realloc(schema->ModelTypes, sizeof(SCHEMA_MODEL_TYPE_HANDLE) * schema->ModelTypeCount);
                    if (oldModelTypes == NULL)
                    {
                        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                    }
                    else
                    {
                        schema->ModelTypes = oldModelTypes;
                    }
                }
            }
        }
    }

    return result;
}

SCHEMA_HANDLE Schema_GetSchemaForModelType(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle)
{
    SCHEMA_HANDLE result;

    if (modelTypeHandle == NULL)
    {
        result = NULL;
    }
    else
    {
        result = ((SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle)->SchemaHandle;
    }

    return result;
}

SCHEMA_RESULT Schema_AddModelProperty(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* propertyName, const char* propertyType)
{
    return AddModelProperty((SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle, propertyName, propertyType);
}

static bool reportedPropertyExists(const void* element, const void* value)
{
    SCHEMA_REPORTED_PROPERTY_HANDLE_DATA* reportedProperty = *(SCHEMA_REPORTED_PROPERTY_HANDLE_DATA**)element;
    return (strcmp(reportedProperty->reportedPropertyName, value) == 0);
}

SCHEMA_RESULT Schema_AddModelReportedProperty(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* reportedPropertyName, const char* reportedPropertyType)
{
    SCHEMA_RESULT result;
    if (
        (modelTypeHandle == NULL) ||
        (reportedPropertyName == NULL) ||
        (reportedPropertyType == NULL)
        )
    {
        LogError("invalid argument SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* reportedPropertyName=%p, const char* reportedPropertyType=%p", modelTypeHandle, reportedPropertyName, reportedPropertyType);
        result = SCHEMA_INVALID_ARG;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        if (VECTOR_find_if(modelType->reportedProperties, reportedPropertyExists, reportedPropertyName) != NULL)
        {
            LogError("unable to add reportedProperty %s because it already exists", reportedPropertyName);
            result = SCHEMA_DUPLICATE_ELEMENT;
        }
        else
        {
            SCHEMA_REPORTED_PROPERTY_HANDLE_DATA* reportedProperty = (SCHEMA_REPORTED_PROPERTY_HANDLE_DATA*)malloc(sizeof(SCHEMA_REPORTED_PROPERTY_HANDLE_DATA));
            if (reportedProperty == NULL)
            {
                LogError("unable to malloc");
                result = SCHEMA_ERROR;
            }
            else
            {
                if (mallocAndStrcpy_s((char**)&reportedProperty->reportedPropertyName, reportedPropertyName) != 0)
                {
                    LogError("unable to mallocAndStrcpy_s");
                    free(reportedProperty);
                    result = SCHEMA_ERROR;
                }
                else
                {
                    if (mallocAndStrcpy_s((char**)&reportedProperty->reportedPropertyType, reportedPropertyType) != 0)
                    {
                        LogError("unable to mallocAndStrcpy_s");
                        free((void*)reportedProperty->reportedPropertyName);
                        free(reportedProperty);
                        result = SCHEMA_ERROR;
                    }
                    else
                    {
                        if (VECTOR_push_back(modelType->reportedProperties, &reportedProperty, 1) != 0)
                        {
                            LogError("unable to VECTOR_push_back");
                            free((void*)reportedProperty->reportedPropertyType);
                            free((void*)reportedProperty->reportedPropertyName);
                            free(reportedProperty);
                            result = SCHEMA_ERROR;
                        }
                        else
                        {
                            result = SCHEMA_OK;
                        }
                    }
                }
            }
        }
    }
    return result;
}

SCHEMA_ACTION_HANDLE Schema_CreateModelAction(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* actionName)
{
    SCHEMA_ACTION_HANDLE result;

    if ((modelTypeHandle == NULL) ||
        (actionName == NULL))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        size_t i;

        for (i = 0; i < modelType->ActionCount; i++)
        {
            SCHEMA_ACTION_HANDLE_DATA* action = (SCHEMA_ACTION_HANDLE_DATA*)modelType->Actions[i];
            if (strcmp(action->ActionName, actionName) == 0)
            {
                break;
            }
        }

        if (i < modelType->ActionCount)
        {
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_DUPLICATE_ELEMENT));
        }
        else
        {
            SCHEMA_ACTION_HANDLE* newActions = (SCHEMA_ACTION_HANDLE*)realloc(modelType->Actions, sizeof(SCHEMA_ACTION_HANDLE) * (modelType->ActionCount + 1));
            if (newActions == NULL)
            {
                result = NULL;
                LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
            }
            else
            {
                SCHEMA_ACTION_HANDLE_DATA* newAction;
                modelType->Actions = newActions;

                if ((newAction = (SCHEMA_ACTION_HANDLE_DATA*)calloc(1, sizeof(SCHEMA_ACTION_HANDLE_DATA))) == NULL)
                {
                    result = NULL;
                    LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                }
                else
                {
                    if (mallocAndStrcpy_s((char**)&newAction->ActionName, actionName) != 0)
                    {
                        free(newAction);
                        result = NULL;
                        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                    }
                    else
                    {
                        newAction->ArgumentCount = 0;
                        newAction->ArgumentHandles = NULL;

                        modelType->Actions[modelType->ActionCount] = newAction;
                        modelType->ActionCount++;
                        result = (SCHEMA_ACTION_HANDLE)(newAction);
                    }

                    /* If possible, reduce the memory of over allocation */
                    if (result == NULL)
                    {
                        if (modelType->ActionCount > 0)
                        {
                            SCHEMA_ACTION_HANDLE *oldActions = (SCHEMA_ACTION_HANDLE *)realloc(modelType->Actions, sizeof(SCHEMA_ACTION_HANDLE) * modelType->ActionCount);
                            if (oldActions == NULL)
                            {
                                LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                            }
                            else
                            {
                                modelType->Actions = oldActions;
                            }
                        }
                        else
                        {
                            modelType->Actions = NULL;
                        }
                    }
                }
            }
        }
    }
    return result;
}


static bool methodExists(const void* element, const void* value)
{
    const SCHEMA_METHOD_HANDLE* method = (const SCHEMA_METHOD_HANDLE*)element;
    return (strcmp((*method)->methodName, value) == 0);
}


SCHEMA_METHOD_HANDLE Schema_CreateModelMethod(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* methodName)
{
    SCHEMA_METHOD_HANDLE result;

    if ((modelTypeHandle == NULL) ||
        (methodName == NULL))
    {
        result = NULL;
        LogError("invalid argument: SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* methodName=%p", modelTypeHandle, methodName);
    }
    else
    {
        if (VECTOR_find_if(modelTypeHandle->methods, methodExists, methodName) != NULL)
        {
            LogError("method %s already exists", methodName);
            result = NULL;
        }
        else
        {
            result = malloc(sizeof(SCHEMA_METHOD_HANDLE_DATA));
            if (result == NULL)
            {
                LogError("failed to malloc");
                /*return as is*/
            }
            else
            {
                result->methodArguments = VECTOR_create(sizeof(SCHEMA_METHOD_ARGUMENT_HANDLE));
                if (result->methodArguments == NULL)
                {
                    LogError("failure in VECTOR_create");
                    free(result);
                    result = NULL;
                }
                else
                {
                    if (mallocAndStrcpy_s(&result->methodName, methodName) != 0)
                    {
                        LogError("failure in mallocAndStrcpy_s");
                        VECTOR_destroy(result->methodArguments);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        if (VECTOR_push_back(modelTypeHandle->methods, &result, 1) != 0)
                        {
                            LogError("failure in VECTOR_push_back");
                            free(result->methodName);
                            VECTOR_destroy(result->methodArguments);
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            /*return as is*/
                        }
                    }
                }
            }
        }
    }
    return result;
}


SCHEMA_RESULT Schema_AddModelActionArgument(SCHEMA_ACTION_HANDLE actionHandle, const char* argumentName, const char* argumentType)
{
    SCHEMA_RESULT result;

    if ((actionHandle == NULL) ||
        (argumentName == NULL) ||
        (argumentType == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_ACTION_HANDLE_DATA* action = (SCHEMA_ACTION_HANDLE_DATA*)actionHandle;

        size_t i;
        for (i = 0; i < action->ArgumentCount; i++)
        {
            SCHEMA_ACTION_ARGUMENT_HANDLE_DATA* actionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE_DATA*)action->ArgumentHandles[i];
            if (strcmp((actionArgument->Name), argumentName) == 0)
            {
                break;
            }
        }

        if (i < action->ArgumentCount)
        {
            result = SCHEMA_DUPLICATE_ELEMENT;
            LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
        }
        else
        {
            SCHEMA_ACTION_ARGUMENT_HANDLE* newArguments = (SCHEMA_ACTION_ARGUMENT_HANDLE*)realloc(action->ArgumentHandles, sizeof(SCHEMA_ACTION_ARGUMENT_HANDLE) * (action->ArgumentCount + 1));
            if (newArguments == NULL)
            {
                result = SCHEMA_ERROR;
                LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
            }
            else
            {
                action->ArgumentHandles = newArguments;

                SCHEMA_ACTION_ARGUMENT_HANDLE_DATA* newActionArgument;

                action->ArgumentHandles = newArguments;
                if ((newActionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE_DATA*)malloc(sizeof(SCHEMA_ACTION_ARGUMENT_HANDLE_DATA))) == NULL)
                {
                    result = SCHEMA_ERROR;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                }
                else
                {
                    if (mallocAndStrcpy_s((char**)&newActionArgument->Name, argumentName) != 0)
                    {
                        free(newActionArgument);
                        result = SCHEMA_ERROR;
                        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                    }
                    else if (mallocAndStrcpy_s((char**)&newActionArgument->Type, argumentType) != 0)
                    {
                        free((void*)newActionArgument->Name);
                        free(newActionArgument);
                        result = SCHEMA_ERROR;
                        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                    }
                    else
                    {
                        action->ArgumentHandles[action->ArgumentCount] = newActionArgument;
                        action->ArgumentCount++;

                        result = SCHEMA_OK;
                    }
                }

                /* If possible, reduce the memory of over allocation */
                if (result == SCHEMA_ERROR)
                {
                    if (action->ArgumentCount > 0)
                    {
                        SCHEMA_ACTION_ARGUMENT_HANDLE *oldArguments = (SCHEMA_ACTION_ARGUMENT_HANDLE *)realloc(action->ArgumentHandles, sizeof(SCHEMA_ACTION_ARGUMENT_HANDLE) * action->ArgumentCount);
                        if (oldArguments == NULL)
                        {
                            LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                        }
                        else
                        {
                            action->ArgumentHandles = oldArguments;
                        }
                    }
                    else
                    {
                        action->ArgumentHandles = NULL;
                    }
                }
            }
        }
    }
    return result;
}

static bool methodFindArgumentByBame(const void* element, const void* value)
{
    /*element is a pointer to SCHEMA_METHOD_ARGUMENT_HANDLE*/
    const SCHEMA_METHOD_ARGUMENT_HANDLE* decodedElement = (const SCHEMA_METHOD_ARGUMENT_HANDLE*)element;
    const char* name = (const char*)value;
    return (strcmp((*decodedElement)->Name, name) == 0);
}

SCHEMA_RESULT Schema_AddModelMethodArgument(SCHEMA_METHOD_HANDLE methodHandle, const char* argumentName, const char* argumentType)
{
    SCHEMA_RESULT result;
    if ((methodHandle == NULL) ||
        (argumentName == NULL) ||
        (argumentType == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        if (VECTOR_find_if(methodHandle->methodArguments, methodFindArgumentByBame, argumentName) != NULL)
        {
            LogError("an argument with name %s already exists", argumentName);
            result = SCHEMA_INVALID_ARG;
        }
        else
        {
            SCHEMA_METHOD_ARGUMENT_HANDLE_DATA* argument = malloc(sizeof(SCHEMA_METHOD_ARGUMENT_HANDLE_DATA));
            if (argument == NULL)
            {
                LogError("failure to malloc");
                result = SCHEMA_ERROR;
            }
            else
            {
                if (mallocAndStrcpy_s(&argument->Name, argumentName) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s");
                    free(argument);
                    result = SCHEMA_ERROR;
                }
                else
                {
                    if (mallocAndStrcpy_s(&argument->Type, argumentType) != 0)
                    {
                        LogError("failure in mallocAndStrcpy_s");
                        free(argument->Name);
                        free(argument);
                        result = SCHEMA_ERROR;
                    }
                    else
                    {
                        if (VECTOR_push_back(methodHandle->methodArguments, &argument, 1) != 0)
                        {
                            LogError("failure in VECTOR_push_back");
                            free(argument->Name);
                            free(argument->Type);
                            free(argument);
                            result = SCHEMA_ERROR;
                        }
                        else
                        {
                            result = SCHEMA_OK;
                        }
                    }
                }
            }
        }
    }
    return result;
}

SCHEMA_PROPERTY_HANDLE Schema_GetModelPropertyByName(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* propertyName)
{
    SCHEMA_PROPERTY_HANDLE result;

    if ((modelTypeHandle == NULL) ||
        (propertyName == NULL))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        size_t i;
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

        for (i = 0; i < modelType->PropertyCount; i++)
        {
            SCHEMA_PROPERTY_HANDLE_DATA* modelProperty = (SCHEMA_PROPERTY_HANDLE_DATA*)modelType->Properties[i];
            if (strcmp(modelProperty->PropertyName, propertyName) == 0)
            {
                break;
            }
        }

        if (i == modelType->PropertyCount)
        {
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ELEMENT_NOT_FOUND));
        }
        else
        {
            result = (SCHEMA_PROPERTY_HANDLE)(modelType->Properties[i]);
        }
    }

    return result;
}

SCHEMA_RESULT Schema_GetModelPropertyCount(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t* propertyCount)
{
    SCHEMA_RESULT result;

    if ((modelTypeHandle == NULL) ||
        (propertyCount == NULL))
    {
        result = SCHEMA_INVALID_ARG;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

        *propertyCount = modelType->PropertyCount;

        result = SCHEMA_OK;
    }

    return result;
}

SCHEMA_RESULT Schema_GetModelReportedPropertyCount(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t* reportedPropertyCount)
{
    SCHEMA_RESULT result;
    if (
        (modelTypeHandle == NULL)||
        (reportedPropertyCount==NULL)
        )
    {
        LogError("SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, size_t* reportedPropertyCount=%p", modelTypeHandle, reportedPropertyCount);
        result = SCHEMA_INVALID_ARG;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        *reportedPropertyCount = VECTOR_size(modelType->reportedProperties);
        result = SCHEMA_OK;
    }
    return result;
}

SCHEMA_REPORTED_PROPERTY_HANDLE Schema_GetModelReportedPropertyByName(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* reportedPropertyName)
{
    SCHEMA_REPORTED_PROPERTY_HANDLE result;
    if(
        (modelTypeHandle == NULL) ||
        (reportedPropertyName == NULL)
        )
    {
        LogError("invalid argument SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* reportedPropertyName=%p", modelTypeHandle, reportedPropertyName);
        result = NULL;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        if((result = VECTOR_find_if(modelType->reportedProperties, reportedPropertyExists, reportedPropertyName))==NULL)
        {
            LogError("a reported property with name \"%s\" does not exist", reportedPropertyName);
        }
        else
        {
            /*return as is*/
        }
    }
    return result;
}

SCHEMA_REPORTED_PROPERTY_HANDLE Schema_GetModelReportedPropertyByIndex(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t index)
{
    SCHEMA_REPORTED_PROPERTY_HANDLE result;
    if (modelTypeHandle == NULL)
    {
        LogError("invalid argument SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, size_t index=%lu", modelTypeHandle, (unsigned long)index);
        result = NULL;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

        if ((result = VECTOR_element(modelType->reportedProperties, index)) == NULL)
        {
            LogError("index %lu is invalid", (unsigned long)index);
        }
        else
        {
            /*return as is*/
        }
    }
    return result;
}

SCHEMA_PROPERTY_HANDLE Schema_GetModelPropertyByIndex(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t index)
{
    SCHEMA_PROPERTY_HANDLE result;
    SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

    if ((modelTypeHandle == NULL) ||
        (index >= modelType->PropertyCount))
    {
        result = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        result = modelType->Properties[index];
    }

    return result;
}

SCHEMA_ACTION_HANDLE Schema_GetModelActionByName(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* actionName)
{
    SCHEMA_ACTION_HANDLE result;

    if ((modelTypeHandle == NULL) ||
        (actionName == NULL))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        size_t i;
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

        for (i = 0; i < modelType->ActionCount; i++)
        {
            SCHEMA_ACTION_HANDLE_DATA* modelAction = (SCHEMA_ACTION_HANDLE_DATA*)modelType->Actions[i];
            if (strcmp(modelAction->ActionName, actionName) == 0)
            {
                break;
            }
        }

        if (i == modelType->ActionCount)
        {
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ELEMENT_NOT_FOUND));
        }
        else
        {
            result = modelType->Actions[i];
        }
    }

    return result;
}

static bool matchModelMethod(const void* element, const void* value)
{
    /*element is a pointer to SCHEMA_METHOD_HANDLE_DATA*/
    const SCHEMA_METHOD_HANDLE* decodedElement = (const SCHEMA_METHOD_HANDLE* )element;
    const char* name = (const char*)value;
    return (strcmp((*decodedElement)->methodName, name) == 0);
}

SCHEMA_METHOD_HANDLE Schema_GetModelMethodByName(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* methodName)
{
    SCHEMA_METHOD_HANDLE result;

    if ((modelTypeHandle == NULL) ||
        (methodName == NULL))
    {
        result = NULL;
        LogError("invalid arguments SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* methodName=%p", modelTypeHandle, methodName);
    }
    else
    {
        SCHEMA_METHOD_HANDLE* found = VECTOR_find_if(modelTypeHandle->methods, matchModelMethod, methodName);
        if (found == NULL)
        {
            LogError("no such method by name = %s", methodName);
            result = NULL;
        }
        else
        {
            result = *found;
        }
    }

    return result;
}

SCHEMA_RESULT Schema_GetModelActionCount(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t* actionCount)
{
    SCHEMA_RESULT result;

    if ((modelTypeHandle == NULL) ||
        (actionCount == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result=%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

        *actionCount = modelType->ActionCount;

        result = SCHEMA_OK;
    }

    return result;
}

SCHEMA_ACTION_HANDLE Schema_GetModelActionByIndex(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t index)
{
    SCHEMA_ACTION_HANDLE result;
    SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

    if ((modelType == NULL) ||
        (index >= modelType->ActionCount))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        result = modelType->Actions[index];
    }

    return result;
}

const char* Schema_GetModelActionName(SCHEMA_ACTION_HANDLE actionHandle)
{
    const char* result;

    if (actionHandle == NULL)
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_ACTION_HANDLE_DATA* action = (SCHEMA_ACTION_HANDLE_DATA*)actionHandle;
        result = action->ActionName;
    }

    return result;
}

SCHEMA_RESULT Schema_GetModelActionArgumentCount(SCHEMA_ACTION_HANDLE actionHandle, size_t* argumentCount)
{
    SCHEMA_RESULT result;

    if ((actionHandle == NULL) ||
        (argumentCount == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result=%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_ACTION_HANDLE_DATA* modelAction = (SCHEMA_ACTION_HANDLE_DATA*)actionHandle;

        *argumentCount = modelAction->ArgumentCount;

        result = SCHEMA_OK;
    }

    return result;
}

SCHEMA_RESULT Schema_GetModelMethodArgumentCount(SCHEMA_METHOD_HANDLE methodHandle, size_t* argumentCount)
{
    SCHEMA_RESULT result;

    if ((methodHandle == NULL) ||
        (argumentCount == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result=%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        *argumentCount = VECTOR_size(methodHandle->methodArguments);
        result = SCHEMA_OK;
    }

    return result;
}


SCHEMA_ACTION_ARGUMENT_HANDLE Schema_GetModelActionArgumentByName(SCHEMA_ACTION_HANDLE actionHandle, const char* actionArgumentName)
{
    SCHEMA_ACTION_ARGUMENT_HANDLE result;

    if ((actionHandle == NULL) ||
        (actionArgumentName == NULL))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_ACTION_HANDLE_DATA* modelAction = (SCHEMA_ACTION_HANDLE_DATA*)actionHandle;
        size_t i;

        for (i = 0; i < modelAction->ArgumentCount; i++)
        {
            SCHEMA_ACTION_ARGUMENT_HANDLE_DATA* actionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE_DATA*)modelAction->ArgumentHandles[i];
            if (strcmp(actionArgument->Name, actionArgumentName) == 0)
            {
                break;
            }
        }

        if (i == modelAction->ArgumentCount)
        {
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ELEMENT_NOT_FOUND));
        }
        else
        {
            result = modelAction->ArgumentHandles[i];
        }
    }

    return result;
}

SCHEMA_ACTION_ARGUMENT_HANDLE Schema_GetModelActionArgumentByIndex(SCHEMA_ACTION_HANDLE actionHandle, size_t argumentIndex)
{
    SCHEMA_ACTION_ARGUMENT_HANDLE result;
    SCHEMA_ACTION_HANDLE_DATA* modelAction = (SCHEMA_ACTION_HANDLE_DATA*)actionHandle;

    if ((actionHandle == NULL) ||
        (argumentIndex >= modelAction->ArgumentCount))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        result = modelAction->ArgumentHandles[argumentIndex];
    }

    return result;
}

SCHEMA_METHOD_ARGUMENT_HANDLE Schema_GetModelMethodArgumentByIndex(SCHEMA_METHOD_HANDLE methodHandle, size_t argumentIndex)
{
    SCHEMA_METHOD_ARGUMENT_HANDLE result;

    if (methodHandle == NULL)
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_METHOD_ARGUMENT_HANDLE *temp = VECTOR_element(methodHandle->methodArguments, argumentIndex);
        if (temp == NULL)
        {
            result = NULL;
        }
        else
        {
            result = *temp;
        }
    }

    return result;
}


const char* Schema_GetActionArgumentName(SCHEMA_ACTION_ARGUMENT_HANDLE actionArgumentHandle)
{
    const char* result;
    if (actionArgumentHandle == NULL)
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_ACTION_ARGUMENT_HANDLE_DATA* actionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE_DATA*)actionArgumentHandle;
        result = actionArgument->Name;
    }
    return result;
}

const char* Schema_GetMethodArgumentName(SCHEMA_METHOD_ARGUMENT_HANDLE methodArgumentHandle)
{
    const char* result;
    if (methodArgumentHandle == NULL)
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        result = methodArgumentHandle->Name;
    }
    return result;
}


const char* Schema_GetActionArgumentType(SCHEMA_ACTION_ARGUMENT_HANDLE actionArgumentHandle)
{
    const char* result;
    if (actionArgumentHandle == NULL)
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_ACTION_ARGUMENT_HANDLE_DATA* actionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE_DATA*)actionArgumentHandle;
        result = actionArgument->Type;
    }
    return result;
}

const char* Schema_GetMethodArgumentType(SCHEMA_METHOD_ARGUMENT_HANDLE methodArgumentHandle)
{
    const char* result;
    if (methodArgumentHandle == NULL)
    {
        result = NULL;
        LogError("invalid argument SCHEMA_METHOD_ARGUMENT_HANDLE methodArgumentHandle=%p", methodArgumentHandle);
    }
    else
    {
        result = methodArgumentHandle->Type;
    }
    return result;
}


SCHEMA_STRUCT_TYPE_HANDLE Schema_CreateStructType(SCHEMA_HANDLE schemaHandle, const char* typeName)
{
    SCHEMA_STRUCT_TYPE_HANDLE result;
    SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;

    if ((schema == NULL) ||
        (typeName == NULL))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_STRUCT_TYPE_HANDLE_DATA* structType;
        size_t i;

        for (i = 0; i < schema->StructTypeCount; i++)
        {
            structType = (SCHEMA_STRUCT_TYPE_HANDLE_DATA*)schema->StructTypes[i];
            if (strcmp(structType->Name, typeName) == 0)
            {
                break;
            }
        }

        if (i < schema->StructTypeCount)
        {
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_DUPLICATE_ELEMENT));
        }
        else
        {
            SCHEMA_STRUCT_TYPE_HANDLE* newStructTypes = (SCHEMA_STRUCT_TYPE_HANDLE*)realloc(schema->StructTypes, sizeof(SCHEMA_STRUCT_TYPE_HANDLE) * (schema->StructTypeCount + 1));
            if (newStructTypes == NULL)
            {
                result = NULL;
                LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
            }
            else
            {
                schema->StructTypes = newStructTypes;
                if ((structType = (SCHEMA_STRUCT_TYPE_HANDLE_DATA*)calloc(1, sizeof(SCHEMA_STRUCT_TYPE_HANDLE_DATA))) == NULL)
                {
                    result = NULL;
                    LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                }
                else if (mallocAndStrcpy_s((char**)&structType->Name, typeName) != 0)
                {
                    result = NULL;
                    free(structType);
                    LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                }
                else
                {
                    schema->StructTypes[schema->StructTypeCount] = structType;
                    schema->StructTypeCount++;
                    structType->PropertyCount = 0;
                    structType->Properties = NULL;

                    result = (SCHEMA_STRUCT_TYPE_HANDLE)structType;
                }

                /* If possible, reduce the memory of over allocation */
                if (result == NULL)
                {
                    if (schema->StructTypeCount > 0)
                    {
                        SCHEMA_STRUCT_TYPE_HANDLE *oldStructTypes = (SCHEMA_STRUCT_TYPE_HANDLE *)realloc(schema->StructTypes, sizeof(SCHEMA_STRUCT_TYPE_HANDLE) * schema->StructTypeCount);
                        if (oldStructTypes == NULL)
                        {
                            result = NULL;
                            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ERROR));
                        }
                        else
                        {
                            schema->StructTypes = oldStructTypes;
                        }
                    }
                    else
                    {
                        schema->StructTypes = NULL;
                    }
                }
            }
        }
    }

    return result;
}

const char* Schema_GetStructTypeName(SCHEMA_STRUCT_TYPE_HANDLE structTypeHandle)
{
    const char* result;

    if (structTypeHandle == NULL)
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        result = ((SCHEMA_STRUCT_TYPE_HANDLE_DATA*)structTypeHandle)->Name;
    }

    return result;
}

SCHEMA_RESULT Schema_GetStructTypeCount(SCHEMA_HANDLE schemaHandle, size_t* structTypeCount)
{
    SCHEMA_RESULT result;

    if ((schemaHandle == NULL) ||
        (structTypeCount == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;
        *structTypeCount = schema->StructTypeCount;
        result = SCHEMA_OK;
    }

    return result;
}

SCHEMA_STRUCT_TYPE_HANDLE Schema_GetStructTypeByName(SCHEMA_HANDLE schemaHandle, const char* name)
{
    SCHEMA_STRUCT_TYPE_HANDLE result;
    SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;

    if ((schemaHandle == NULL) ||
        (name == NULL))
    {
        result = NULL;
        LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        size_t i;

        for (i = 0; i < schema->StructTypeCount; i++)
        {
            SCHEMA_STRUCT_TYPE_HANDLE_DATA* structType = (SCHEMA_STRUCT_TYPE_HANDLE_DATA*)schema->StructTypes[i];
            if (strcmp(structType->Name, name) == 0)
            {
                break;
            }
        }

        if (i == schema->StructTypeCount)
        {
            result = NULL;
            LogError("(Error code:%s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ELEMENT_NOT_FOUND));
        }
        else
        {
            result = schema->StructTypes[i];
        }
    }

    return result;
}

SCHEMA_STRUCT_TYPE_HANDLE Schema_GetStructTypeByIndex(SCHEMA_HANDLE schemaHandle, size_t index)
{
    SCHEMA_STRUCT_TYPE_HANDLE result;
    SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;

    if ((schemaHandle == NULL) ||
        (index >= schema->StructTypeCount))
    {
        result = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {

        result = schema->StructTypes[index];
    }
    return result;
}

SCHEMA_RESULT Schema_AddStructTypeProperty(SCHEMA_STRUCT_TYPE_HANDLE structTypeHandle, const char* propertyName, const char* propertyType)
{
    SCHEMA_RESULT result;

    if ((structTypeHandle == NULL) ||
        (propertyName == NULL) ||
        (propertyType == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        size_t i;
        SCHEMA_STRUCT_TYPE_HANDLE_DATA* structType = (SCHEMA_STRUCT_TYPE_HANDLE_DATA*)structTypeHandle;

        for (i = 0; i < structType->PropertyCount; i++)
        {
            SCHEMA_PROPERTY_HANDLE_DATA* property = (SCHEMA_PROPERTY_HANDLE_DATA*)structType->Properties[i];
            if (strcmp(property->PropertyName, propertyName) == 0)
            {
                break;
            }
        }

        if (i < structType->PropertyCount)
        {
            result = SCHEMA_DUPLICATE_ELEMENT;
            LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
        }
        else
        {
            SCHEMA_PROPERTY_HANDLE* newProperties = (SCHEMA_PROPERTY_HANDLE*)realloc(structType->Properties, sizeof(SCHEMA_PROPERTY_HANDLE) * (structType->PropertyCount + 1));
            if (newProperties == NULL)
            {
                result = SCHEMA_ERROR;
                LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
            }
            else
            {
                SCHEMA_PROPERTY_HANDLE_DATA* newProperty;

                structType->Properties = newProperties;
                if ((newProperty = (SCHEMA_PROPERTY_HANDLE_DATA*)malloc(sizeof(SCHEMA_PROPERTY_HANDLE_DATA))) == NULL)
                {
                    result = SCHEMA_ERROR;
                    LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                }
                else
                {
                    if (mallocAndStrcpy_s((char**)&newProperty->PropertyName, propertyName) != 0)
                    {
                        free(newProperty);
                        result = SCHEMA_ERROR;
                        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                    }
                    else if (mallocAndStrcpy_s((char**)&newProperty->PropertyType, propertyType) != 0)
                    {
                        free((void*)newProperty->PropertyName);
                        free(newProperty);
                        result = SCHEMA_ERROR;
                        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                    }
                    else
                    {
                        structType->Properties[structType->PropertyCount] = (SCHEMA_PROPERTY_HANDLE)newProperty;
                        structType->PropertyCount++;

                        result = SCHEMA_OK;
                    }
                }

                /* If possible, reduce the memory of over allocation */
                if (result != SCHEMA_OK)
                {
                    if (structType->PropertyCount > 0)
                    {
                        SCHEMA_PROPERTY_HANDLE *oldProperties = (SCHEMA_PROPERTY_HANDLE *)realloc(structType->Properties, sizeof(SCHEMA_PROPERTY_HANDLE) * structType->PropertyCount);
                        if (oldProperties == NULL)
                        {
                            result = SCHEMA_ERROR;
                            LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
                        }
                        else
                        {
                            structType->Properties = oldProperties;
                        }
                    }
                    else
                    {
                        structType->Properties = NULL;
                    }
                }
            }
        }
    }

    return result;
}

SCHEMA_PROPERTY_HANDLE Schema_GetStructTypePropertyByName(SCHEMA_STRUCT_TYPE_HANDLE structTypeHandle, const char* propertyName)
{
    SCHEMA_PROPERTY_HANDLE result;

    if ((structTypeHandle == NULL) ||
        (propertyName == NULL))
    {
        result = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        size_t i;
        SCHEMA_STRUCT_TYPE_HANDLE_DATA* structType = (SCHEMA_STRUCT_TYPE_HANDLE_DATA*)structTypeHandle;

        for (i = 0; i < structType->PropertyCount; i++)
        {
            SCHEMA_PROPERTY_HANDLE_DATA* modelProperty = (SCHEMA_PROPERTY_HANDLE_DATA*)structType->Properties[i];
            if (strcmp(modelProperty->PropertyName, propertyName) == 0)
            {
                break;
            }
        }

        if (i == structType->PropertyCount)
        {
            result = NULL;
            LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_ELEMENT_NOT_FOUND));
        }
        else
        {
            result = structType->Properties[i];
        }
    }

    return result;
}

SCHEMA_RESULT Schema_GetStructTypePropertyCount(SCHEMA_STRUCT_TYPE_HANDLE structTypeHandle, size_t* propertyCount)
{
    SCHEMA_RESULT result;

    if ((structTypeHandle == NULL) ||
        (propertyCount == NULL))
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_STRUCT_TYPE_HANDLE_DATA* structType = (SCHEMA_STRUCT_TYPE_HANDLE_DATA*)structTypeHandle;

        *propertyCount = structType->PropertyCount;

        result = SCHEMA_OK;
    }

    return result;
}

SCHEMA_PROPERTY_HANDLE Schema_GetStructTypePropertyByIndex(SCHEMA_STRUCT_TYPE_HANDLE structTypeHandle, size_t index)
{
    SCHEMA_PROPERTY_HANDLE result;
    SCHEMA_STRUCT_TYPE_HANDLE_DATA* structType = (SCHEMA_STRUCT_TYPE_HANDLE_DATA*)structTypeHandle;

    if ((structTypeHandle == NULL) ||
        (index >= structType->PropertyCount))
    {
        result = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        result = structType->Properties[index];
    }

    return result;
}

const char* Schema_GetPropertyName(SCHEMA_PROPERTY_HANDLE propertyHandle)
{
    const char* result;

    if (propertyHandle == NULL)
    {
        result = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_PROPERTY_HANDLE_DATA* propertyType = (SCHEMA_PROPERTY_HANDLE_DATA*)propertyHandle;

        result = propertyType->PropertyName;
    }

    return result;
}

const char* Schema_GetPropertyType(SCHEMA_PROPERTY_HANDLE propertyHandle)
{
    const char* result;

    if (propertyHandle == NULL)
    {
        result = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_PROPERTY_HANDLE_DATA* modelProperty = (SCHEMA_PROPERTY_HANDLE_DATA*)propertyHandle;

        result = modelProperty->PropertyType;
    }

    return result;
}

SCHEMA_RESULT Schema_GetModelCount(SCHEMA_HANDLE schemaHandle, size_t* modelCount)
{
    SCHEMA_RESULT result;
    if ((schemaHandle == NULL) ||
        (modelCount == NULL) )
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(result = %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;
        *modelCount = schema->ModelTypeCount;
        result = SCHEMA_OK;
    }
    return result;
}

SCHEMA_MODEL_TYPE_HANDLE Schema_GetModelByName(SCHEMA_HANDLE schemaHandle, const char* modelName)
{
    SCHEMA_MODEL_TYPE_HANDLE result;

    if ((schemaHandle == NULL) ||
        (modelName == NULL))
    {
        result = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;
        size_t i;
        for (i = 0; i < schema->ModelTypeCount; i++)
        {
            SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)schema->ModelTypes[i];
            if (strcmp(modelName, modelType->Name)==0)
            {
                break;
            }
        }
        if (i == schema->ModelTypeCount)
        {
            result = NULL;
        }
        else
        {
            result = schema->ModelTypes[i];
        }
    }
    return result;
}

SCHEMA_MODEL_TYPE_HANDLE Schema_GetModelByIndex(SCHEMA_HANDLE schemaHandle, size_t index)
{
    SCHEMA_MODEL_TYPE_HANDLE result;
    SCHEMA_HANDLE_DATA* schema = (SCHEMA_HANDLE_DATA*)schemaHandle;

    if ((schemaHandle == NULL) ||
        (index >= schema->ModelTypeCount))
    {
        result = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {

        result = schema->ModelTypes[index];
    }
    return result;
}

const char* Schema_GetModelName(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle)
{
    const char* result;
    if (modelTypeHandle == NULL)
    {
        result = NULL;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        result = modelType->Name;
    }
    return result;
}

SCHEMA_RESULT Schema_AddModelModel(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* propertyName, SCHEMA_MODEL_TYPE_HANDLE modelType, size_t offset, pfOnDesiredProperty onDesiredProperty)
{
    SCHEMA_RESULT result;
    if (
        (modelTypeHandle == NULL) ||
        (propertyName == NULL) ||
        (modelType == NULL)
        )
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, SCHEMA_INVALID_ARG));
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* parentModel = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        MODEL_IN_MODEL temp;
        temp.modelHandle = modelType;
        temp.offset = offset;
        temp.onDesiredProperty = onDesiredProperty;
        if (mallocAndStrcpy_s((char**)&(temp.propertyName), propertyName) != 0)
        {
            result = SCHEMA_ERROR;
            LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
        }
        else if (VECTOR_push_back(parentModel->models, &temp, 1) != 0)
        {
            free((void*)temp.propertyName);
            result = SCHEMA_ERROR;
            LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
        }
        else
        {

            result = SCHEMA_OK;
        }
    }
    return result;
}


SCHEMA_RESULT Schema_GetModelModelCount(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t* modelCount)
{
    SCHEMA_RESULT result;
    if (
        (modelTypeHandle == NULL) ||
        (modelCount == NULL)
        )
    {
        result = SCHEMA_INVALID_ARG;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(SCHEMA_RESULT, result));
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        *modelCount = VECTOR_size(model->models);
        /*SRS_SCHEMA_99_168: [If the function succeeds, it shall return SCHEMA_OK.]*/
        result = SCHEMA_OK;
    }
    return result;
}

static bool matchModelName(const void* element, const void* value)
{
    MODEL_IN_MODEL* decodedElement = (MODEL_IN_MODEL*)element;
    const char* name = (const char*)value;
    return (strcmp(decodedElement->propertyName, name) == 0);
}

SCHEMA_MODEL_TYPE_HANDLE Schema_GetModelModelByName(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* propertyName)
{
    SCHEMA_MODEL_TYPE_HANDLE result;
    if (
        (modelTypeHandle == NULL) ||
        (propertyName == NULL)
        )
    {
        result = NULL;
        LogError("error SCHEMA_INVALID_ARG");
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        void* temp = VECTOR_find_if(model->models, matchModelName, propertyName);
        if (temp == NULL)
        {
            LogError("specified propertyName not found (%s)", propertyName);
            result = NULL;
        }
        else
        {
            result = ((MODEL_IN_MODEL*)temp)->modelHandle;
        }
    }
    return result;
}

size_t Schema_GetModelModelByName_Offset(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* propertyName)
{
    size_t result;
    if (
        (modelTypeHandle == NULL) ||
        (propertyName == NULL)
        )
    {
        result = 0;
        LogError("error SCHEMA_INVALID_ARG");
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        void* temp = VECTOR_find_if(model->models, matchModelName, propertyName);
        if (temp == NULL)
        {
            LogError("specified propertyName not found (%s)", propertyName);
            result = 0;
        }
        else
        {
            result = ((MODEL_IN_MODEL*)temp)->offset;
        }
    }
    return result;
}

pfOnDesiredProperty Schema_GetModelModelByName_OnDesiredProperty(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* propertyName)
{
    pfOnDesiredProperty result;
    if (
        (modelTypeHandle == NULL) ||
        (propertyName == NULL)
        )
    {
        result = NULL;
        LogError("invalid argument SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* propertyName=%p",modelTypeHandle, propertyName);
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        void* temp = VECTOR_find_if(model->models, matchModelName, propertyName);
        if (temp == NULL)
        {
            LogError("specified propertyName not found (%s)", propertyName);
            result = NULL;
        }
        else
        {
            result = ((MODEL_IN_MODEL*)temp)->onDesiredProperty;
        }
    }
    return result;

}

size_t Schema_GetModelModelByIndex_Offset(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t index)
{
    size_t result;
    if (modelTypeHandle == NULL)
    {
        result = 0;
        LogError("error SCHEMA_INVALID_ARG");
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        void* temp = VECTOR_element(model->models, index);
        if (temp == 0)
        {
            LogError("specified index [out of bounds] (%lu)", (unsigned long)index);
            result = 0;
        }
        else
        {
            result = ((MODEL_IN_MODEL*)temp)->offset;
        }
    }
    return result;
}

SCHEMA_MODEL_TYPE_HANDLE Schema_GetModelModelyByIndex(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t index)
{
    SCHEMA_MODEL_TYPE_HANDLE result;
    if (modelTypeHandle == NULL)
    {
        result = NULL;
        LogError("error SCHEMA_INVALID_ARG");
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        size_t nModelsInModel;
        nModelsInModel = VECTOR_size(model->models);
        if (index < nModelsInModel)
        {
            result = ((MODEL_IN_MODEL*)VECTOR_element(model->models, index))->modelHandle;
        }
        else
        {
            LogError("attempted out of bounds access in array.");
            result = NULL; /*out of bounds array access*/
        }
    }
    return result;
}

const char* Schema_GetModelModelPropertyNameByIndex(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t index)
{
    const char* result;
    if (modelTypeHandle == NULL)
    {
        result = NULL;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* model = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        size_t nModelsInModel;
        nModelsInModel = VECTOR_size(model->models);
        if (index < nModelsInModel)
        {
            result = ((MODEL_IN_MODEL*)VECTOR_element(model->models, index))->propertyName;
        }
        else
        {
            LogError("attempted out of bounds access in array.");
            result = NULL; /*out of bounds array access*/
        }
    }
    return result;
}

bool Schema_ModelPropertyByPathExists(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* propertyPath)
{
    bool result;

    if ((modelTypeHandle == NULL) ||
        (propertyPath == NULL))
    {
        LogError("error SCHEMA_INVALID_ARG");
        result = false;
    }
    else
    {
        const char* slashPos;
        result = false;

        if (*propertyPath == '/')
        {
            propertyPath++;
        }

        do
        {
            const char* endPos;
            size_t i;
            size_t modelCount;
            SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

            slashPos = strchr(propertyPath, '/');
            endPos = slashPos;
            if (endPos == NULL)
            {
                endPos = &propertyPath[strlen(propertyPath)];
            }

            /* get the child-model */
            modelCount = VECTOR_size(modelType->models);
            for (i = 0; i < modelCount; i++)
            {
                MODEL_IN_MODEL* childModel = (MODEL_IN_MODEL*)VECTOR_element(modelType->models, i);
                if (childModel != NULL &&
                    (strncmp(childModel->propertyName, propertyPath, endPos - propertyPath) == 0) &&
                    (strlen(childModel->propertyName) == (size_t)(endPos - propertyPath)))
                {
                    /* found */
                    modelTypeHandle = childModel->modelHandle;
                    break;
                }
            }

            if (i < modelCount)
            {
                /* model found, check if there is more in the path */
                if (slashPos == NULL)
                {
                    /* this is the last one, so this is the thing we were looking for */
                    result = true;
                    break;
                }
                else
                {
                    /* continue looking, there's more  */
                    propertyPath = slashPos + 1;
                }
            }
            else
            {
                /* no model found, let's see if this is a property */
                for (i = 0; i < modelType->PropertyCount; i++)
                {
                    SCHEMA_PROPERTY_HANDLE_DATA* property = (SCHEMA_PROPERTY_HANDLE_DATA*)modelType->Properties[i];
                    if ((strncmp(property->PropertyName, propertyPath, endPos - propertyPath) == 0) &&
                        (strlen(property->PropertyName) == (size_t)(endPos - propertyPath)))
                    {
                        /* found property */
                        result = true;
                        break;
                    }
                }

                break;
            }
        } while (slashPos != NULL);
    }

    return result;
}

bool Schema_ModelReportedPropertyByPathExists(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* reportedPropertyPath)
{
    bool result;

    if ((modelTypeHandle == NULL) ||
        (reportedPropertyPath == NULL))
    {
        LogError("invalid argument SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* reportedPropertyPath=%s", modelTypeHandle, reportedPropertyPath);
        result = false;
    }
    else
    {
        const char* slashPos;

        result = false;

        if (*reportedPropertyPath == '/')
        {
            reportedPropertyPath++;
        }

        do
        {
            const char* endPos;
            size_t i;
            size_t modelCount;
            SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

            slashPos = strchr(reportedPropertyPath, '/');
            endPos = slashPos;
            if (endPos == NULL)
            {
                endPos = &reportedPropertyPath[strlen(reportedPropertyPath)];
            }

            modelCount = VECTOR_size(modelType->models);
            for (i = 0; i < modelCount; i++)
            {
                MODEL_IN_MODEL* childModel = (MODEL_IN_MODEL*)VECTOR_element(modelType->models, i);
                if (childModel != NULL &&
                    (strncmp(childModel->propertyName, reportedPropertyPath, endPos - reportedPropertyPath) == 0) &&
                    (strlen(childModel->propertyName) == (size_t)(endPos - reportedPropertyPath)))
                {
                    /* found */
                    modelTypeHandle = childModel->modelHandle;
                    break;
                }
            }

            if (i < modelCount)
            {
                /* model found, check if there is more in the path */
                if (slashPos == NULL)
                {
                    /* this is the last one, so this is the thing we were looking for */
                    result = true;
                    break;
                }
                else
                {
                    /* continue looking, there's more  */
                    reportedPropertyPath = slashPos + 1;
                }
            }
            else
            {
                /* no model found, let's see if this is a property */
                result = (VECTOR_find_if(modelType->reportedProperties, reportedPropertyExists, reportedPropertyPath) != NULL);
                if (!result)
                {
                    LogError("no such reported property \"%s\"", reportedPropertyPath);
                }
                break;
            }
        } while (slashPos != NULL);
    }

    return result;
}

static bool desiredPropertyExists(const void* element, const void* value)
{
    SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* desiredProperty = *(SCHEMA_DESIRED_PROPERTY_HANDLE_DATA**)element;
    return (strcmp(desiredProperty->desiredPropertyName, value) == 0);
}

SCHEMA_RESULT Schema_AddModelDesiredProperty(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* desiredPropertyName, const char* desiredPropertyType, pfDesiredPropertyFromAGENT_DATA_TYPE desiredPropertyFromAGENT_DATA_TYPE, pfDesiredPropertyInitialize desiredPropertyInitialize, pfDesiredPropertyDeinitialize desiredPropertyDeinitialize, size_t offset, pfOnDesiredProperty onDesiredProperty)
{
    SCHEMA_RESULT result;
    if (
        (modelTypeHandle == NULL) ||
        (desiredPropertyName == NULL) ||
        (desiredPropertyType == NULL) ||
        (desiredPropertyFromAGENT_DATA_TYPE == NULL) ||
        (desiredPropertyInitialize == NULL) ||
        (desiredPropertyDeinitialize== NULL)
        )
    {
        LogError("invalid arg SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* desiredPropertyName=%p, const char* desiredPropertyType=%p, pfDesiredPropertyFromAGENT_DATA_TYPE desiredPropertyFromAGENT_DATA_TYPE=%p, pfDesiredPropertyInitialize desiredPropertyInitialize=%p, pfDesiredPropertyDeinitialize desiredPropertyDeinitialize=%p, size_t offset=%lu",
            modelTypeHandle, desiredPropertyName, desiredPropertyType, desiredPropertyFromAGENT_DATA_TYPE, desiredPropertyInitialize, desiredPropertyDeinitialize, (unsigned long)offset);
        result = SCHEMA_INVALID_ARG;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* handleData = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        if (VECTOR_find_if(handleData->desiredProperties, desiredPropertyExists, desiredPropertyName) != NULL)
        {
            LogError("unable to Schema_AddModelDesiredProperty because a desired property with the same name (%s) already exists", desiredPropertyName);
            result = SCHEMA_DUPLICATE_ELEMENT;
        }
        else
        {
            SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* desiredProperty = (SCHEMA_DESIRED_PROPERTY_HANDLE_DATA*)calloc(1, sizeof(SCHEMA_DESIRED_PROPERTY_HANDLE_DATA));
            if (desiredProperty == NULL)
            {
                LogError("failure in malloc");
                result = SCHEMA_ERROR;
            }
            else
            {
                if (mallocAndStrcpy_s((char**)&desiredProperty->desiredPropertyName, desiredPropertyName) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s");
                    free(desiredProperty);
                    result = SCHEMA_ERROR;
                }
                else
                {
                    if (mallocAndStrcpy_s((char**)&desiredProperty->desiredPropertyType, desiredPropertyType) != 0)
                    {
                        LogError("failure in mallocAndStrcpy_s");
                        free((void*)desiredProperty->desiredPropertyName);
                        free(desiredProperty);
                        result = SCHEMA_ERROR;
                    }
                    else
                    {
                        if (VECTOR_push_back(handleData->desiredProperties, &desiredProperty, 1) != 0)
                        {
                            LogError("failure in VECTOR_push_back");
                            free((void*)desiredProperty->desiredPropertyType);
                            free((void*)desiredProperty->desiredPropertyName);
                            free(desiredProperty);
                            result = SCHEMA_ERROR;
                        }
                        else
                        {
                            desiredProperty->desiredPropertyFromAGENT_DATA_TYPE = desiredPropertyFromAGENT_DATA_TYPE;
                            desiredProperty->desiredPropertInitialize = desiredPropertyInitialize;
                            desiredProperty->desiredPropertDeinitialize = desiredPropertyDeinitialize;
                            desiredProperty->onDesiredProperty = onDesiredProperty; /*NULL is a perfectly fine value*/
                            desiredProperty->offset = offset;
                            result = SCHEMA_OK;
                        }
                    }
                }
            }
        }
    }
    return result;
}


SCHEMA_RESULT Schema_GetModelDesiredPropertyCount(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t* desiredPropertyCount)
{
    SCHEMA_RESULT result;
    if (
        (modelTypeHandle == NULL) ||
        (desiredPropertyCount == NULL)
        )
    {
        LogError("invalid arg: SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, size_t* desiredPropertyCount=%p", modelTypeHandle, desiredPropertyCount);
        result = SCHEMA_INVALID_ARG;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* handleData = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        *desiredPropertyCount = VECTOR_size(handleData->desiredProperties);
        result = SCHEMA_OK;
    }
    return result;
}

SCHEMA_DESIRED_PROPERTY_HANDLE Schema_GetModelDesiredPropertyByName(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* desiredPropertyName)
{
    SCHEMA_DESIRED_PROPERTY_HANDLE result;
    if (
        (modelTypeHandle == NULL) ||
        (desiredPropertyName == NULL)
        )
    {
        LogError("invalid arg SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* desiredPropertyName=%p", modelTypeHandle, desiredPropertyName);
        result = NULL;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* handleData = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        SCHEMA_DESIRED_PROPERTY_HANDLE* temp = VECTOR_find_if(handleData->desiredProperties, desiredPropertyExists, desiredPropertyName);
        if (temp == NULL)
        {
            LogError("no such desired property by name %s", desiredPropertyName);
            result = NULL;
        }
        else
        {
            result = *temp;
        }
    }
    return result;
}

SCHEMA_DESIRED_PROPERTY_HANDLE Schema_GetModelDesiredPropertyByIndex(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, size_t index)
{
    SCHEMA_DESIRED_PROPERTY_HANDLE result;
    if (modelTypeHandle == NULL)
    {
        LogError("invalid argument SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, size_t index=%lu", modelTypeHandle, (unsigned long)index);
        result = NULL;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* handleData = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;
        SCHEMA_DESIRED_PROPERTY_HANDLE* temp = VECTOR_element(handleData->desiredProperties, index);
        if (temp == NULL)
        {
            LogError("VECTOR_element produced NULL (likely out of bounds index)");
            result = NULL;
        }
        else
        {
            result = *temp;
        }
    }
    return result;
}

bool Schema_ModelDesiredPropertyByPathExists(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* desiredPropertyPath)
{
    bool result;
    if (
        (modelTypeHandle == NULL) ||
        (desiredPropertyPath == NULL)
        )
    {
        LogError("invalid arg SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* desiredPropertyPath=%p", modelTypeHandle, desiredPropertyPath);
        result = false;
    }
    else
    {
        const char* slashPos;

        result = false;

        if (*desiredPropertyPath == '/')
        {
            desiredPropertyPath++;
        }

        do
        {
            const char* endPos;
            size_t i;
            size_t modelCount;
            SCHEMA_MODEL_TYPE_HANDLE_DATA* modelType = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

            slashPos = strchr(desiredPropertyPath, '/');
            endPos = slashPos;
            if (endPos == NULL)
            {
                endPos = &desiredPropertyPath[strlen(desiredPropertyPath)];
            }

            modelCount = VECTOR_size(modelType->models);
            for (i = 0; i < modelCount; i++)
            {
                MODEL_IN_MODEL* childModel = (MODEL_IN_MODEL*)VECTOR_element(modelType->models, i);
                if (childModel != NULL &&
                    (strncmp(childModel->propertyName, desiredPropertyPath, endPos - desiredPropertyPath) == 0) &&
                    (strlen(childModel->propertyName) == (size_t)(endPos - desiredPropertyPath)))
                {
                    /* found */
                    modelTypeHandle = childModel->modelHandle;
                    break;
                }
            }

            if (i < modelCount)
            {
                /* model found, check if there is more in the path */
                if (slashPos == NULL)
                {
                    /* this is the last one, so this is the thing we were looking for */
                    result = true;
                    break;
                }
                else
                {
                    /* continue looking, there's more  */
                    desiredPropertyPath = slashPos + 1;
                }
            }
            else
            {
                /* no model found, let's see if this is a property */
                result = (VECTOR_find_if(modelType->desiredProperties, desiredPropertyExists, desiredPropertyPath) != NULL);
                if (!result)
                {
                    LogError("no such desired property \"%s\"", desiredPropertyPath);
                }
                break;
            }
        } while (slashPos != NULL);
    }
    return result;
}

const char* Schema_GetModelDesiredPropertyType(SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle)
{
    const char* result;
    if (desiredPropertyHandle == NULL)
    {
        LogError("invalid argument SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle=%p", desiredPropertyHandle);
        result = NULL;
    }
    else
    {
        SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* desirePropertyHandleData = (SCHEMA_DESIRED_PROPERTY_HANDLE_DATA*)desiredPropertyHandle;
        result = desirePropertyHandleData->desiredPropertyType;
    }
    return result;
}

pfDesiredPropertyFromAGENT_DATA_TYPE Schema_GetModelDesiredProperty_pfDesiredPropertyFromAGENT_DATA_TYPE(SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle)
{
    pfDesiredPropertyFromAGENT_DATA_TYPE result;
    if (desiredPropertyHandle == NULL)
    {
        LogError("invalid argument SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle=%p", desiredPropertyHandle);
        result = NULL;
    }
    else
    {
        SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* desirePropertyHandleData = (SCHEMA_DESIRED_PROPERTY_HANDLE_DATA*)desiredPropertyHandle;
        result = desirePropertyHandleData->desiredPropertyFromAGENT_DATA_TYPE;
    }
    return result;
}

pfOnDesiredProperty Schema_GetModelDesiredProperty_pfOnDesiredProperty(SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle)
{
    pfOnDesiredProperty result;
    if (desiredPropertyHandle == NULL)
    {
        LogError("invalid argument SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle=%p", desiredPropertyHandle);
        result = NULL;
    }
    else
    {
        SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* desirePropertyHandleData = (SCHEMA_DESIRED_PROPERTY_HANDLE_DATA*)desiredPropertyHandle;
        result = desirePropertyHandleData->onDesiredProperty;
    }
    return result;
}

size_t Schema_GetModelDesiredProperty_offset(SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle)
{
    size_t result;
    if (desiredPropertyHandle == NULL)
    {
        LogError("invalid argument SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle=%p", desiredPropertyHandle);
        result = 0;
    }
    else
    {
        SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* desirePropertyHandleData = (SCHEMA_DESIRED_PROPERTY_HANDLE_DATA*)desiredPropertyHandle;
        result = desirePropertyHandleData->offset;
    }
    return result;
}

static bool modelInModelExists(const void* element, const void* value)
{
    MODEL_IN_MODEL* modelInModel = (MODEL_IN_MODEL*)element;
    return (strcmp(modelInModel->propertyName, value) == 0);
}

SCHEMA_MODEL_ELEMENT Schema_GetModelElementByName(SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle, const char* elementName)
{
    SCHEMA_MODEL_ELEMENT result;
    if (
        (modelTypeHandle == NULL) ||
        (elementName == NULL)
        )
    {
        LogError("invalid argument SCHEMA_MODEL_TYPE_HANDLE modelTypeHandle=%p, const char* elementName=%p", modelTypeHandle, elementName);
        result.elementType = SCHEMA_SEARCH_INVALID_ARG;
    }
    else
    {
        SCHEMA_MODEL_TYPE_HANDLE_DATA* handleData = (SCHEMA_MODEL_TYPE_HANDLE_DATA*)modelTypeHandle;

        SCHEMA_DESIRED_PROPERTY_HANDLE* desiredPropertyHandle = VECTOR_find_if(handleData->desiredProperties, desiredPropertyExists, elementName);
        if (desiredPropertyHandle != NULL)
        {
            result.elementType = SCHEMA_DESIRED_PROPERTY;
            result.elementHandle.desiredPropertyHandle = *desiredPropertyHandle;
        }
        else
        {
            size_t nProcessedProperties = 0;
            SCHEMA_PROPERTY_HANDLE_DATA* property = NULL;
            for (size_t i = 0; i < handleData->PropertyCount;i++)
            {
                property = (SCHEMA_PROPERTY_HANDLE_DATA*)(handleData->Properties[i]);
                if (strcmp(property->PropertyName, elementName) == 0)
                {
                    i = handleData->PropertyCount; /*found it*/
                }
                else
                {
                    nProcessedProperties++;
                }
            }

            if (nProcessedProperties < handleData->PropertyCount)
            {
                result.elementType = SCHEMA_PROPERTY;
                result.elementHandle.propertyHandle = property;
            }
            else
            {

                SCHEMA_REPORTED_PROPERTY_HANDLE* reportedPropertyHandle = VECTOR_find_if(handleData->reportedProperties, reportedPropertyExists, elementName);
                if (reportedPropertyHandle != NULL)
                {
                    result.elementType = SCHEMA_REPORTED_PROPERTY;
                    result.elementHandle.reportedPropertyHandle = *reportedPropertyHandle;
                }
                else
                {

                    size_t nProcessedActions = 0;
                    SCHEMA_ACTION_HANDLE_DATA* actionHandleData = NULL;
                    for (size_t i = 0;i < handleData->ActionCount; i++)
                    {
                        actionHandleData = (SCHEMA_ACTION_HANDLE_DATA*)(handleData->Actions[i]);
                        if (strcmp(actionHandleData->ActionName, elementName) == 0)
                        {
                            i = handleData->ActionCount; /*get out quickly*/
                        }
                        else
                        {
                            nProcessedActions++;
                        }
                    }

                    if (nProcessedActions < handleData->ActionCount)
                    {
                        result.elementType = SCHEMA_MODEL_ACTION;
                        result.elementHandle.actionHandle = actionHandleData;
                    }
                    else
                    {
                        MODEL_IN_MODEL* modelInModel = VECTOR_find_if(handleData->models, modelInModelExists, elementName);
                        if (modelInModel != NULL)
                        {
                            result.elementType = SCHEMA_MODEL_IN_MODEL;
                            result.elementHandle.modelHandle = modelInModel->modelHandle;
                        }
                        else
                        {
                            result.elementType = SCHEMA_NOT_FOUND;
                        }
                    }
                }
            }
        }
    }
    return result;
}

pfDesiredPropertyDeinitialize Schema_GetModelDesiredProperty_pfDesiredPropertyDeinitialize(SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle)
{
    pfDesiredPropertyDeinitialize result;
    if (desiredPropertyHandle == NULL)
    {
        LogError("invalid argument SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle=%p", desiredPropertyHandle);
        result = NULL;
    }
    else
    {
        SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* handleData = (SCHEMA_DESIRED_PROPERTY_HANDLE_DATA*)desiredPropertyHandle;
        result = handleData->desiredPropertDeinitialize;
    }
    return result;
}

pfDesiredPropertyInitialize Schema_GetModelDesiredProperty_pfDesiredPropertyInitialize(SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle)
{
    pfDesiredPropertyInitialize result;
    if (desiredPropertyHandle == NULL)
    {
        LogError("invalid argument SCHEMA_DESIRED_PROPERTY_HANDLE desiredPropertyHandle=%p", desiredPropertyHandle);
        result = NULL;
    }
    else
    {
        SCHEMA_DESIRED_PROPERTY_HANDLE_DATA* handleData = (SCHEMA_DESIRED_PROPERTY_HANDLE_DATA*)desiredPropertyHandle;
        result = handleData->desiredPropertInitialize;
    }
    return result;
}

static bool SchemaHasModel(const SCHEMA_HANDLE* schemaHandle, const char* modelName)
{
    return (Schema_GetModelByName(*schemaHandle, modelName) != NULL);
}


SCHEMA_HANDLE Schema_GetSchemaForModel(const char* modelName)
{
    SCHEMA_HANDLE result;
    if (modelName == NULL)
    {
        LogError("invalid arg const char* modelName=%p", modelName);
        result = NULL;
    }
    else
    {
        SCHEMA_HANDLE* temp = VECTOR_find_if(g_schemas, (PREDICATE_FUNCTION)SchemaHasModel, modelName);

        if (temp == NULL)
        {
            LogError("unable to find a schema that has a model named %s", modelName);
            result = NULL;
        }
        else
        {
            result = *temp;
        }
    }

    return result;
}

void* Schema_GetMetadata(SCHEMA_HANDLE schemaHandle)
{
    void* result;
    if (schemaHandle == NULL)
    {
        LogError("invalid arg SCHEMA_HANDLE schemaHandle=%p", schemaHandle);
        result = NULL;
    }
    else
    {
        result = ((SCHEMA_HANDLE_DATA*)schemaHandle)->metadata;
    }
    return result;
}
