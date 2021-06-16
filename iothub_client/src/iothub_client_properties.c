// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// TODO: There is some vestigial code from origal (un memory friendly) initial way of implementing this.  Remove eventually.

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "iothub_client_properties.h"
#include "parson.h"

typedef enum COMPONENT_PARSE_STATE_TAG
{
    COMPONENT_PARSE_ROOT,
    COMPONENT_PARSE_SUB_COMPONENT,
} COMPONENT_PARSE_STATE;

typedef enum PROPERTY_PARSE_STATE_TAG
{
    PROPERTY_PARSE_DESIRED,
    PROPERTY_PARSE_REPORTED
} PROPERTY_PARSE_STATE;

typedef struct IOTHUB_CLIENT_PROPERTY_ITERATOR_TAG {
    char** componentsInModel;
    size_t numComponentsInModel;
    JSON_Value* rootValue;
    JSON_Object* desiredObject;
    JSON_Object* reportedObject;
    COMPONENT_PARSE_STATE componentParseState;
    PROPERTY_PARSE_STATE propertyParseState;
    size_t currentComponentIndex;
    size_t currentPropertyIndex;
} IOTHUB_CLIENT_PROPERTY_ITERATOR;

static const char PROPERTY_FORMAT_COMPONENT_START[] = "{\"%s\":{\"__t\":\"c\", ";
static const char PROPERTY_FORMAT_NAME_VALUE[] = "\"%s\":%s%s";
static const char PROPERTY_FORMAT_WRITABLE_RESPONSE[] = "\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d}%s";
static const char PROPERTY_FORMAT_WRITABLE_RESPONSE_WITH_DESCRIPTION[] = "{\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}%s";

static const char PROPERTY_OPEN_BRACE[] = "{";
static const char PROPERTY_CLOSE_BRACE[] = "}";
static const char PROPERTY_COMMA[] = ",";
static const char PROPERTY_EMPTY[] = "";


static const char TWIN_DESIRED_OBJECT_NAME[] = "desired";
static const char TWIN_REPORTED_OBJECT_NAME[] = "reported";
static const char TWIN_VERSION[] = "$version";

// IoTHub adds a JSON field "__t":"c" into desired top-level JSON objects that represent components.  Without this marking, the object 
// is treated as a property of the root component.
static const char TWIN_COMPONENT_MARKER[] = "__t";

// When building a list of properties, whether to append a "," or nothing depends on whether more items are coming.
static const char* CommaIfNeeded(bool lastItem)
{
    return lastItem ? PROPERTY_EMPTY : PROPERTY_COMMA;
}

// After builder functions have successfully invoked snprintf, update our traversal pointer and remaining lengths.
static void AdvanceCountersAfterWrite(size_t currentOutputBytes, char** currentWrite, size_t* requiredBytes, size_t* remainingBytes)
{
    // The number of bytes to be required is always updated by number the snprintf wrote to.
    *requiredBytes += currentOutputBytes;
    if (*currentWrite != NULL)
    {
        // if *currentWrite is NULL, then we leave it as NULL since the caller is calculating the number of bytes needed.
        // Otherwise we need to update it for the next write.
        *currentWrite += currentOutputBytes;
        *remainingBytes -= currentOutputBytes;
    }
}

// Adds opening brace for JSON to be generated, either a simple "{" or more complex format if componentName is specified.
static size_t BuildOpeningBrace(const char* componentName, char* currentWrite, size_t remainingBytes)
{
    size_t currentOutputBytes;

    if (componentName != NULL)
    {
        if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_COMPONENT_START, componentName)) < 0)
        {
            LogError("Cannot build properites string");
            return (size_t)-1;
        }
    }
    else
    {
        if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_OPEN_BRACE)) < 0)
        {
            LogError("Cannot build properites string");
            return (size_t)-1;
        }
    }

    return currentOutputBytes;
}

static size_t BuildClosingBrace(bool isComponent, char** currentWrite, size_t* requiredBytes, size_t* remainingBytes)
{
    size_t currentOutputBytes = 0;
    size_t numberOfBraces = isComponent ? 2 : 1;

    for (size_t i = 0; i < numberOfBraces; i++)
    {
        if ((currentOutputBytes += snprintf(*currentWrite, *remainingBytes, PROPERTY_CLOSE_BRACE)) < 0)
        {
            LogError("Cannot build properites string");
            return (size_t)-1;
        }
        AdvanceCountersAfterWrite(currentOutputBytes, currentWrite, requiredBytes, remainingBytes);
    }

    return currentOutputBytes;
}


// BuildReportedProperties is used to build up the actual serializedProperties string based 
// on the properties.  If serializedProperties==NULL and serializedPropertiesLength=0, just like
// analogous snprintf it will just calculate the amount of space caller needs to allocate.
static size_t BuildReportedProperties(const IOTHUB_CLIENT_REPORTED_PROPERTY* properties, size_t numProperties, const char* componentName, unsigned char* serializedProperties, size_t serializedPropertiesLength)
{
    size_t requiredBytes = 0;
    size_t currentOutputBytes;
    size_t remainingBytes = serializedPropertiesLength;
    char* currentWrite = (char*)serializedProperties;

    if ((currentOutputBytes = BuildOpeningBrace(componentName,  currentWrite, remainingBytes)) < 0)
    {
        LogError("Cannot build properites string");
        return (size_t)-1;
    }
    AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);

    for (size_t i = 0; i < numProperties; i++)
    {
        bool lastProperty = (i == (numProperties - 1));
        if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_NAME_VALUE, 
                                            properties[i].name, properties[i].value, CommaIfNeeded(lastProperty))) < 0)
        {
            LogError("Cannot build properites string");
            return (size_t)-1;
        }
        AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);
    }

    if (BuildClosingBrace(componentName != NULL, &currentWrite, &requiredBytes, &remainingBytes) == -1)
    {
        LogError("Cannot build properites string");
        return (size_t)-1;
    }

    return (requiredBytes + 1);
}

// BuildWritableResponseProperties is used to build up the actual serializedProperties string based 
// on the writable response properties.  If serializedProperties==NULL and serializedPropertiesLength=0, just like
// analogous snprintf it will just calculate the amount of space caller needs to allocate.
static size_t BuildWritableResponseProperties(const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE* properties, size_t numProperties, const char* componentName, unsigned char* serializedProperties, size_t serializedPropertiesLength)
{
    size_t requiredBytes = 0;
    size_t currentOutputBytes;
    size_t remainingBytes = serializedPropertiesLength;
    char* currentWrite = (char*)serializedProperties;

    if ((currentOutputBytes = BuildOpeningBrace(componentName,  currentWrite, remainingBytes)) < 0)
    {
        LogError("Cannot build properites string");
        return (size_t)-1;
    }
    AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);

    for (size_t i = 0; i < numProperties; i++)
    {
        bool lastProperty = (i == (numProperties - 1));
        if (properties[i].description == NULL)
        {
            if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_WRITABLE_RESPONSE, properties[i].name, 
                                                properties[i].value, properties[i].result, properties[i].ackVersion, CommaIfNeeded(lastProperty))) < 0)
            {
                LogError("Cannot build properites string");
                return (size_t)-1;
            }
        }
        else
        {
            if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_WRITABLE_RESPONSE_WITH_DESCRIPTION, properties[i].name, 
                                                properties[i].value, properties[i].result, properties[i].ackVersion, properties[i].description, CommaIfNeeded(lastProperty))) < 0)
            {
                LogError("Cannot build properites string");
                return (size_t)-1;
            }
        }

        AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);
    }

    if (BuildClosingBrace(componentName != NULL, &currentWrite, &requiredBytes, &remainingBytes) == -1)
    {
        LogError("Cannot build properites string");
        return (size_t)-1;
    }

    return requiredBytes;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Serialize_ReportedProperties(const IOTHUB_CLIENT_REPORTED_PROPERTY* properties, size_t numProperties, const char* componentName, unsigned char** serializedProperties, size_t* serializedPropertiesLength)
{
    IOTHUB_CLIENT_RESULT result;
    size_t requiredBytes = 0;
    unsigned char* serializedPropertiesBuffer = NULL;

    if ((properties == NULL) || (properties->structVersion != IOTHUB_CLIENT_REPORTED_PROPERTY_VERSION_1) || (numProperties == 0) || (serializedProperties == NULL) || (serializedPropertiesLength == 0))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if ((requiredBytes = BuildReportedProperties(properties, numProperties, componentName, NULL, 0)) < 0)
    {
        LogError("Cannot determine required length of reported properties buffer");
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((serializedPropertiesBuffer = calloc(1, requiredBytes)) == NULL)
    {
        LogError("Cannot allocate %ul bytes", requiredBytes);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if (BuildReportedProperties(properties, numProperties, componentName, serializedPropertiesBuffer, requiredBytes) < 0)
    {
        LogError("Cannot write properties buffer");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        result = IOTHUB_CLIENT_OK;
    }

    if (result == IOTHUB_CLIENT_OK)
    {
        *serializedProperties = serializedPropertiesBuffer;
        *serializedPropertiesLength = requiredBytes;
    }
    else if (serializedPropertiesBuffer != NULL)
    {
        free(serializedPropertiesBuffer);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Serialize_WritablePropertyResponse(
    const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE* properties,
    size_t numProperties,
    const char* componentName,
    unsigned char** serializedProperties,
    size_t* serializedPropertiesLength)
{
    IOTHUB_CLIENT_RESULT result;
    size_t requiredBytes = 0;
    unsigned char* serializedPropertiesBuffer = NULL;

    if ((properties == NULL) || (properties->structVersion != IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_VERSION_1) || (numProperties == 0) || (serializedProperties == NULL) || (serializedPropertiesLength == 0))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if ((requiredBytes = BuildWritableResponseProperties(properties, numProperties, componentName, NULL, 0)) < 0)
    {
        LogError("Cannot determine required length of reported properties buffer");
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((serializedPropertiesBuffer = calloc(1, requiredBytes)) == NULL)
    {
        LogError("Cannot allocate %ul bytes", requiredBytes);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if (BuildWritableResponseProperties(properties, numProperties, componentName, serializedPropertiesBuffer, requiredBytes) < 0)
    {
        LogError("Cannot write properties buffer");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        result = IOTHUB_CLIENT_OK;
    }

    if (result == IOTHUB_CLIENT_OK)
    {
        *serializedProperties = serializedPropertiesBuffer;
        *serializedPropertiesLength = requiredBytes;
    }
    else if (serializedPropertiesBuffer != NULL)
    {
        free(serializedPropertiesBuffer);
    }

    return result;
}


// We have an explicit destroy function for serialized properties, even though IoTHubClient_Serialize_ReportedProperties
// and IoTHubClient_Serialize_WritablePropertyResponse used malloc() and app could've just free() directly, because
// * If SDK is setup to use a custom allocator (gballoc.h) then we use same malloc() / free() matching
// * This gives us flexibility to use non-malloc based allocators in future
// * Maintains symmetry with _Destroy() as mechanism to free resources SDK allocates.
void IoTHubClient_Serialize_Properties_Destroy(unsigned char* serializedProperties)
{
    if (serializedProperties != NULL)
    {
        free(serializedProperties);
    }
}

// The underlying twin payload received from the client SDK is not NULL terminated, but
// parson requires this.  Make a temporary copy of string to NULL terminate.
static char* CopyPayloadToString(const unsigned char* payload, size_t size)
{
    char* jsonStr;
    size_t sizeToAllocate = size + 1;

    if ((jsonStr = (char*)malloc(sizeToAllocate)) == NULL)
    {
        LogError("Unable to allocate %lu size buffer", (unsigned long)(sizeToAllocate));
    }
    else
    {
        memcpy(jsonStr, payload, size);
        jsonStr[size] = '\0';
    }

    return jsonStr;
}

// GetDesiredAndReportedTwinJson retrieves the desired and (if available) the reported sections of IoT Hub twin as JSON_Object's for later use.
// When a full twin is sent, the JSON consists of {"desired":{...}, "reported":{...}}".  If we're processing an update patch, 
// IoT Hub does not send us the "reported" and the "desired" is by convention taken to be the root of the JSON document received.
static IOTHUB_CLIENT_RESULT GetDesiredAndReportedTwinJson(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator)
{
    IOTHUB_CLIENT_RESULT result;
    JSON_Object* rootObject;

    if ((rootObject = json_value_get_object(propertyIterator->rootValue)) == NULL)
    {
        LogError("Unable to get root object of JSON");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        if (payloadType == IOTHUB_CLIENT_PROPERTY_PAYLOAD_COMPLETE)
        {
            // NULL values are NOT errors, as the JSON may legitimately not have these fields.
            propertyIterator->desiredObject = json_object_get_object(rootObject, TWIN_DESIRED_OBJECT_NAME);
            propertyIterator->reportedObject = json_object_get_object(rootObject, TWIN_REPORTED_OBJECT_NAME);
        }
        else
        {
            // For a patch update, IoTHub does not explicitly put a "desired:" JSON envelope.  The "desired-ness" is implicit 
            // in this case, so here we simply need the root of the JSON itself.
            propertyIterator->desiredObject = rootObject;
            propertyIterator->reportedObject = NULL;
        }
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// GetTwinVersion retrieves the $version field from JSON document received from IoT Hub.  
// The application needs this value when acknowledging writable properties received from the service.
static IOTHUB_CLIENT_RESULT GetTwinVersion(JSON_Object* desiredObject, int* propertiesVersion)
{
    IOTHUB_CLIENT_RESULT result;
    JSON_Value* versionValue = NULL;

    if ((versionValue = json_object_get_value(desiredObject, TWIN_VERSION)) == NULL)
    {
        LogError("Cannot retrieve %s field for twin", TWIN_VERSION);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if (json_value_get_type(versionValue) != JSONNumber)
    {
        LogError("JSON field %s is not a number", TWIN_VERSION);
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        *propertiesVersion = (int)json_value_get_number(versionValue);
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// IsJsonObjectAComponentInModel checks whether the objectName, read from the top-level child of the desired device twin JSON, 
// is in componentsInModel that the application passed into us.  There is no way for the SDK to otherwise be able to 
// tell a property from a sub-component, as the protocol does not guaranteed we will received a "__t" marker over the network.
static bool IsJsonObjectAComponentInModel(IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator, const char* objectName)
{
    bool result = false;

    if (objectName == NULL)
    {
        result = false;
    }
    else
    {
        size_t i;
        for (i = 0; i < propertyIterator->numComponentsInModel; i++)
        {
            if (strcmp(objectName, propertyIterator->componentsInModel[i]) == 0)
            {
                break;
            }
        }
        result = (i != propertyIterator->numComponentsInModel);
    }

    return result;
}

static IOTHUB_CLIENT_PROPERTY_ITERATOR* AllocatePropertyIterator(const char** componentsInModel, size_t numComponentsInModel)
{
    IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator;
    bool success = false;

    if ((propertyIterator = (IOTHUB_CLIENT_PROPERTY_ITERATOR*)calloc(1, sizeof(IOTHUB_CLIENT_PROPERTY_ITERATOR))) == NULL)
    {
        LogError("Cannot allocate IOTHUB_CLIENT_PROPERTY_ITERATOR");
    }
    else if (numComponentsInModel == 0)
    {
        propertyIterator->numComponentsInModel = 0;
        success = true;
    }
    else
    {
        propertyIterator->numComponentsInModel = numComponentsInModel;

        if ((propertyIterator->componentsInModel = (char**)calloc(numComponentsInModel, sizeof(char*))) == NULL)
        {
            LogError("Cannot allocate IOTHUB_CLIENT_PROPERTY_ITERATOR");
        }
        else
        {
            size_t i;
            for (i = 0; i < numComponentsInModel; i++)
            {
                if (mallocAndStrcpy_s(&propertyIterator->componentsInModel[i], componentsInModel[i]) != 0)
                {
                    LogError("Cannot allocate IOTHUB_CLIENT_PROPERTY_ITERATOR");
                }
            }

            if (i == numComponentsInModel)
            {
                success = true;
            }
        }

    }

    if (success == false)
    {
        IoTHubClient_Deserialize_Properties_DestroyIterator(propertyIterator);
        propertyIterator = NULL;
    }

    return propertyIterator;
}

static bool ValidateIteratorInputs(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* payload,
    size_t payloadLength,
    const char** componentsInModel,
    size_t numComponentsInModel,
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE* propertyIteratorHandle,
    int* propertiesVersion)
{
    IOTHUB_CLIENT_RESULT result;

    if ((payloadType != IOTHUB_CLIENT_PROPERTY_PAYLOAD_COMPLETE) && (payloadType != IOTHUB_CLIENT_PROPERTY_PAYLOAD_PARTIAL))
    {
        LogError("Payload type %d is invalid", payloadType);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if ((payload == NULL) || (payloadLength == 0) || (propertyIteratorHandle == NULL) || (propertiesVersion == 0))
    {
        LogError("NULL arguments passed");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if (((componentsInModel == NULL) && (numComponentsInModel > 0)) ||
             ((componentsInModel != NULL) && (numComponentsInModel == 0)))
    {
        LogError("Invalid componentsInModel/numComponentsInModel combination");
        result = IOTHUB_CLIENT_INVALID_ARG;

    }
    else
    {
        size_t i;
        for (i = 0; i < numComponentsInModel; i++)
        {
            if (componentsInModel[i] == NULL)
            {
                LogError("componentsInModel at index %ul is NULL", i);
                break;
            }
        }

        if (i == numComponentsInModel)
        {
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
    }

    return result;
}

// FillProperty retrieves properties that are children of jsonObject and puts them into properties, starting at the 0 index.
static IOTHUB_CLIENT_RESULT FillProperty(IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator, const char* componentName, JSON_Value* propertyValue, const char* propertyName, IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;
    const char* propertyStringJson;

    if ((propertyStringJson = json_serialize_to_string(propertyValue)) == NULL)
    {
        LogError("Unable to retrieve JSON string for property value");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        property->propertyType = (propertyIterator->propertyParseState == PROPERTY_PARSE_DESIRED) ? 
                                 IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE : IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE;
        property->componentName = componentName;
        property->name = propertyName;
        property->valueType = IOTHUB_CLIENT_PROPERTY_VALUE_STRING;
        property->value.str = propertyStringJson;
        property->valueLength = strlen(propertyStringJson);
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

static bool IsReservedPropertyKeyword(const char* objectName)
{
    return ((objectName == NULL) || (strcmp(objectName, TWIN_VERSION) == 0));
}

static bool IsReservedComponentKeyword(const char* objectName)
{
    return ((objectName == NULL) || (strcmp(objectName, TWIN_COMPONENT_MARKER) == 0));
}

static bool GetNextComponentProperty(IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator, JSON_Object* containerObject, const char** propertyName, JSON_Value** propertyValue)
{
    JSON_Value* componentValue = json_object_get_value_at(containerObject, propertyIterator->currentPropertyIndex);
    JSON_Object* componentObject = json_value_get_object(componentValue);

    *propertyName = NULL;
    *propertyValue = NULL;

    if (componentObject == NULL)
    {
        // This isn't a JSON Object so nothing to traverse
        ;
    }
    else
    {
        size_t numComponentChildren = json_object_get_count(componentObject);

        while (propertyIterator->currentComponentIndex < numComponentChildren)
        {
            const char* objectName = json_object_get_name(componentObject, propertyIterator->currentComponentIndex);
            
            if (IsReservedComponentKeyword(objectName))
            {
                // This objectName corresponds to twin/property metadata that is not passed to application
                propertyIterator->currentComponentIndex++;
            }
            else
            {
                *propertyName = objectName;
                *propertyValue = json_object_get_value_at(componentObject, propertyIterator->currentComponentIndex);
                propertyIterator->currentComponentIndex++;
                break;
            }
        }
    }

    return (*propertyValue != NULL);
}

static bool GetNextPropertyToEnumerate(IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator, const char** componentName, const char** propertyName, JSON_Value** propertyValue)
{
    JSON_Object* containerObject;
    *propertyName = NULL;
    *propertyValue = NULL;

    if (propertyIterator->propertyParseState == PROPERTY_PARSE_DESIRED)
    {
        containerObject = propertyIterator->desiredObject;
    }
    else
    {
        containerObject = propertyIterator->reportedObject;
    }

    size_t numChildren = json_object_get_count(containerObject);

    while (propertyIterator->currentPropertyIndex < numChildren)
    {
        const char* objectName = json_object_get_name(containerObject, propertyIterator->currentPropertyIndex);

        if (propertyIterator->componentParseState == COMPONENT_PARSE_SUB_COMPONENT)
        {
            // The propertyIterator->currentPropertyIndex corresponds to a subcomponent we're traversing.
            if (GetNextComponentProperty(propertyIterator, containerObject, propertyName, propertyValue) == true)
            {
                // The component has additional properties to return to application.
                *componentName = objectName;
                break;
            }
            else
            {
                // We've parsed all the properties of this component.  Move onto the next top-level JSON element
                propertyIterator->componentParseState = COMPONENT_PARSE_ROOT;
                propertyIterator->currentPropertyIndex++;
                propertyIterator->currentComponentIndex = 0;
                continue;
            }
        }

        if (IsJsonObjectAComponentInModel(propertyIterator, objectName))
        {
            // This top-level property is the name of a component.  
            propertyIterator->componentParseState = COMPONENT_PARSE_SUB_COMPONENT;
            continue;
        }
        else if (IsReservedPropertyKeyword(objectName))
        {
            // This objectName corresponds to twin/property metadata that is not passed to application
            propertyIterator->currentPropertyIndex++;
            continue;
        }
        else
        {
            *componentName = NULL;
            *propertyName = objectName;
            *propertyValue = json_object_get_value_at(containerObject, propertyIterator->currentPropertyIndex);
            propertyIterator->currentPropertyIndex++;
            break;
        }
    }

    return (*propertyValue != NULL);
}


IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties_CreateIterator(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* payload,
    size_t payloadLength,
    const char** componentsInModel,
    size_t numComponentsInModel,
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE* propertyIteratorHandle,
    int* propertiesVersion)
{
    IOTHUB_CLIENT_RESULT result;
    char* jsonStr = NULL;

    IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator = NULL;

    if ((result = ValidateIteratorInputs(payloadType, payload, payloadLength, componentsInModel, numComponentsInModel, propertyIteratorHandle, propertiesVersion)) != IOTHUB_CLIENT_OK)
    {
        LogError("Invalid argument");
    }
    else if ((propertyIterator = AllocatePropertyIterator(componentsInModel, numComponentsInModel)) == NULL)
    {
        LogError("Cannot allocate IOTHUB_CLIENT_PROPERTY_ITERATOR");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        if ((jsonStr = CopyPayloadToString(payload, payloadLength)) == NULL)
        {
            LogError("Unable to allocate twin buffer");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if ((propertyIterator->rootValue = json_parse_string(jsonStr)) == NULL)
        {
            LogError("Unable to parse device twin JSON");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if ((result = GetDesiredAndReportedTwinJson(payloadType, propertyIterator)) != IOTHUB_CLIENT_OK)
        {
            LogError("Cannot retrieve desired and/or reported object from JSON");
        }
        else if ((result = GetTwinVersion(propertyIterator->desiredObject, propertiesVersion)) != IOTHUB_CLIENT_OK)
        {
            LogError("Cannot retrieve properties version");
        }
        else
        {
            propertyIterator->propertyParseState = PROPERTY_PARSE_DESIRED;
            propertyIterator->componentParseState = COMPONENT_PARSE_ROOT;
            *propertyIteratorHandle = propertyIterator;
            result = IOTHUB_CLIENT_OK;
        }
    }

    free(jsonStr);

    if (result != IOTHUB_CLIENT_OK)
    {
        IoTHubClient_Deserialize_Properties_DestroyIterator(propertyIterator);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties_GetNextProperty(
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE propertyIteratorHandle,
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property,
    bool* propertySpecified)
{
    IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator = (IOTHUB_CLIENT_PROPERTY_ITERATOR*)propertyIteratorHandle;
    IOTHUB_CLIENT_RESULT result;

    JSON_Value* propertyValue = NULL;
    const char* propertyName = NULL;
    const char* componentName = NULL;

    if (propertyIterator->propertyParseState == PROPERTY_PARSE_DESIRED)
    {
        if ((GetNextPropertyToEnumerate(propertyIterator, &componentName, &propertyName, &propertyValue)) == false)
        {
            // If we can't find another desired object, then transition to start searching through reported.
            propertyIterator->currentPropertyIndex = 0;
            propertyIterator->propertyParseState = PROPERTY_PARSE_REPORTED;
        }
    }

    if (propertyIterator->propertyParseState == PROPERTY_PARSE_REPORTED)
    {
        GetNextPropertyToEnumerate(propertyIterator, &componentName, &propertyName, &propertyValue);
    }

    if (propertyValue != NULL)
    {
        // Fills in IOTHUB_CLIENT_DESERIALIZED_PROPERTY based on where we're at in the traversal.
        if ((result = FillProperty(propertyIterator, componentName, propertyValue, propertyName, property)) != IOTHUB_CLIENT_OK)
        {
            LogError("Cannot Fill Properties");
        }
        else
        {
            *propertySpecified = true;
        }
    }
    else
    {
        *propertySpecified = false;
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

void IoTHubClient_Deserialize_Properties_DestroyProperty(
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property)
{
    if (property != NULL)
    {
        // The only field in IOTHUB_CLIENT_DESERIALIZED_PROPERTY that is allocated when filling
        // the structure is the JSON representation.  The remaining fields are shallow copies pointing
        // into the parsed JSON fields which are freed by IoTHubClient_Deserialize_Properties_DestroyIterator.
        json_free_serialized_string((char*)property->value.str);
    }
}

void IoTHubClient_Deserialize_Properties_DestroyIterator(IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE propertyIteratorHandle)
{
    if (propertyIteratorHandle != NULL)
    {
        IOTHUB_CLIENT_PROPERTY_ITERATOR* propertyIterator = (IOTHUB_CLIENT_PROPERTY_ITERATOR*)propertyIteratorHandle;
        for (size_t i = 0; i < propertyIterator->numComponentsInModel; i++)
        {
            free(propertyIterator->componentsInModel[i]);
        }
        json_value_free(propertyIterator->rootValue);
        free(propertyIterator);
    }
}

