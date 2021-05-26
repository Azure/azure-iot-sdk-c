// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// TODO: There is some vestigial code from origal (un memory friendly) initial way of implementing this.  Remove eventually.

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "iothub_client_properties.h"
#include "parson.h"

static const char PROPERTY_FORMAT_COMPONENT_START[] = "{\"%s\":_t:c, ";
static const char PROPERTY_FORMAT_NAME_VALUE[] = "\"%s\":%s%s";
static const char PROPERTY_FORMAT_WRITEABLE_RESPONSE[] = "{\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d}%s";
static const char PROPERTY_FORMAT_WRITEABLE_RESPONSE_WITH_DESCRIPTION[] = "{\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}%s";

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

    if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_CLOSE_BRACE)) < 0)
    {
        LogError("Cannot build properites string");
        return (size_t)-1;
    }
    AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);
 
    return (requiredBytes + 1);
}

// BuildWriteableResponseProperties is used to build up the actual serializedProperties string based 
// on the writeable response properties.  If serializedProperties==NULL and serializedPropertiesLength=0, just like
// analogous snprintf it will just calculate the amount of space caller needs to allocate.
static size_t BuildWriteableResponseProperties(const IOTHUB_CLIENT_WRITEABLE_PROPERTY_RESPONSE* properties, size_t numProperties, const char* componentName, unsigned char* serializedProperties, size_t serializedPropertiesLength)
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
            if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_WRITEABLE_RESPONSE, properties[i].name, 
                                                properties[i].value, properties[i].result, properties[i].ackVersion, CommaIfNeeded(lastProperty))) < 0)
            {
                LogError("Cannot build properites string");
                return (size_t)-1;
            }
        }
        else
        {
            if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_WRITEABLE_RESPONSE_WITH_DESCRIPTION, properties[i].name, 
                                                properties[i].value, properties[i].result, properties[i].ackVersion, properties[i].description, CommaIfNeeded(lastProperty))) < 0)
            {
                LogError("Cannot build properites string");
                return (size_t)-1;
            }
        }

        AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);
    }

    if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_CLOSE_BRACE)) < 0)
    {
        LogError("Cannot build properites string");
        return (size_t)-1;
    }
    AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);

    return requiredBytes;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Serialize_ReportedProperties(const IOTHUB_CLIENT_REPORTED_PROPERTY* properties, size_t numProperties, const char* componentName, unsigned char** serializedProperties, size_t* serializedPropertiesLength)
{
    IOTHUB_CLIENT_RESULT result;
    size_t requiredBytes = 0;
    unsigned char* serializedPropertiesBuffer = NULL;

    if ((properties == NULL) || (properties->version != IOTHUB_CLIENT_REPORTED_PROPERTY_VERSION_1) || (numProperties == 0) || (serializedProperties == NULL) || (serializedPropertiesLength == 0))
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

IOTHUB_CLIENT_RESULT IoTHubClient_Serialize_WriteablePropertyResponse(
    const IOTHUB_CLIENT_WRITEABLE_PROPERTY_RESPONSE* properties,
    size_t numProperties,
    const char* componentName,
    unsigned char** serializedProperties,
    size_t* serializedPropertiesLength)
{
    IOTHUB_CLIENT_RESULT result;
    size_t requiredBytes = 0;
    unsigned char* serializedPropertiesBuffer = NULL;

    if ((properties == NULL) || (properties->version != IOTHUB_CLIENT_WRITEABLE_PROPERTY_RESPONSE_VERSION_1) || (numProperties == 0) || (serializedProperties == NULL) || (serializedPropertiesLength == 0))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if ((requiredBytes = BuildWriteableResponseProperties(properties, numProperties, componentName, NULL, 0)) < 0)
    {
        LogError("Cannot determine required length of reported properties buffer");
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((serializedPropertiesBuffer = calloc(1, requiredBytes)) == NULL)
    {
        LogError("Cannot allocate %ul bytes", requiredBytes);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if (BuildWriteableResponseProperties(properties, numProperties, componentName, serializedPropertiesBuffer, requiredBytes) < 0)
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
static IOTHUB_CLIENT_RESULT GetDesiredAndReportedTwinJson(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, JSON_Value* rootValue, JSON_Object** desiredObject, JSON_Object** reportedObject)
{
    IOTHUB_CLIENT_RESULT result;
    JSON_Object* rootObject = NULL;

    if ((rootObject = json_value_get_object(rootValue)) == NULL)
    {
        LogError("Unable to get root object of JSON");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        if (payloadType == IOTHUB_CLIENT_PROPERTY_PAYLOAD_COMPLETE)
        {
            // NULL values are NOT errors in this case.  TODO: Check about what empty twin looks like for comments here, but I still don't think this should be fatal.
            *desiredObject = json_object_get_object(rootObject, TWIN_DESIRED_OBJECT_NAME);
            *reportedObject = json_object_get_object(rootObject, TWIN_REPORTED_OBJECT_NAME);
        }
        else
        {
            // For a patch update, IoTHub does not explicitly put a "desired:" JSON envelope.  The "desired-ness" is implicit 
            // in this case, so here we simply need the root of the JSON itself.
            *desiredObject = rootObject;
            *reportedObject = NULL;
        }
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// GetTwinVersion retrieves the $version field from JSON document received from IoT Hub.  
// The application needs this value when acknowledging writeable properties received from the service.
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
static bool IsJsonObjectAComponentInModel(const char* objectName, const char** componentsInModel, size_t numComponentsInModel)
{
    bool result = false;

    for (size_t i = 0; i < numComponentsInModel; i++)
    {
        if (strcmp(objectName, componentsInModel[i]) == 0)
        {
            result = true;
            break;
        }
    }

    return result;
}

// GetNumberPropertiesForComponent determines how many properties a component has.
static IOTHUB_CLIENT_RESULT GetNumberPropertiesForComponent(JSON_Value* componentValue, size_t *numComponentProperties)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;
    size_t i;

    JSON_Object* componentObject = json_value_get_object(componentValue);
    size_t numChildren = json_object_get_count(componentObject);
    *numComponentProperties = 0;

    for (i = 0; i < numChildren; i++)
    {
        const char* propertyName = json_object_get_name(componentObject, i);
        JSON_Value* propertyValue = json_object_get_value_at(componentObject, i);

        if ((propertyName == NULL) || (propertyValue == NULL))
        {
            LogError("Unexpected error retrieving the property name and/or value at element at index=%lu", (unsigned long)i);
            result = IOTHUB_CLIENT_ERROR;
            return result;
        }

        // When a component is received from a full twin, it will have a "__t" as one of the child elements.  This is metadata that indicates
        // to solutions that the JSON object corresponds to a component and not a property of the root component.  Because this is 
        // metadata and not part of this component's modeled properties, we ignore it when processing this loop.
        if (strcmp(propertyName, TWIN_COMPONENT_MARKER) == 0)
        {
            continue;
        }

        *numComponentProperties = (*numComponentProperties) + 1;
    }

    if (i == numChildren)
    {
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// GetNumberOfProperties examines the jsonObject to retrieve the number of properties.
static IOTHUB_CLIENT_RESULT GetNumberOfProperties(const char** componentsInModel, size_t numComponentsInModel, JSON_Object* jsonObject, size_t *numProperties)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;
    size_t numChildren;
    size_t currentPropertyCount = 0;
    size_t i;

    numChildren = json_object_get_count(jsonObject);

    for (i = 0; i < numChildren; i++)
    {
        const char* name = json_object_get_name(jsonObject, i);
        JSON_Value* value = json_object_get_value_at(jsonObject, i);

        if ((name == NULL) || (value == NULL))
        {
            LogError("Unable to retrieve name and/or value from index %lu", i);
            result = IOTHUB_CLIENT_ERROR;
            break;
        }
        else if (strcmp(name, TWIN_VERSION) == 0)
        {
            // The version field is metadata and not user property data.  Skip.
            continue;
        }
        else if ((json_type(value) == JSONObject) && IsJsonObjectAComponentInModel(name, componentsInModel, numComponentsInModel))
        {
            // If this current JSON is an element AND the name is one of the componentsInModel that the application knows about,
            // then this JSON element represents a component.
            size_t numComponentProperties;
            if ((result = GetNumberPropertiesForComponent(value, &numComponentProperties)) != IOTHUB_CLIENT_OK)
            {
                LogError("Cannot get number of properties for component %s", name);
                break;
            }

            currentPropertyCount += numComponentProperties;
        }
        else
        {
            // If the child element is NOT an object OR its not a model the application knows about, this is a property of the model's root component.
            currentPropertyCount++;
        }
    }

    if (numChildren == i)
    {
        result = IOTHUB_CLIENT_OK;
        *numProperties = currentPropertyCount;
    }

    return result;
}

static IOTHUB_CLIENT_RESULT FillPropertiesForComponent(IOTHUB_CLIENT_PROPERTY_TYPE propertyType, const char* componentName, JSON_Value* componentValue, IOTHUB_CLIENT_DESERIALIZED_PROPERTY* properties, size_t* numComponentProperties)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;
    size_t i;

    JSON_Object* componentObject = json_value_get_object(componentValue);
    size_t numChildren = json_object_get_count(componentObject);
    *numComponentProperties = 0;

    for (i = 0; i < numChildren; i++)
    {
        const char* propertyName = json_object_get_name(componentObject, i);
        JSON_Value* propertyValue = json_object_get_value_at(componentObject, i);

        if ((propertyName == NULL) || (propertyValue == NULL))
        {
            LogError("Unexpected error retrieving the property name and/or value at element at index=%lu", (unsigned long)i);
            result = IOTHUB_CLIENT_ERROR;
            return result;
        }

        // When a component is received from a full twin, it will have a "__t" as one of the child elements.  This is metadata that indicates
        // to solutions that the JSON object corresponds to a component and not a property of the root component.  Because this is 
        // metadata and not part of this component's modeled properties, we ignore it when processing this loop.
        if (strcmp(propertyName, TWIN_COMPONENT_MARKER) == 0)
        {
            continue;
        }

        const char* propertyStringJson;
        
        if ((propertyStringJson = json_serialize_to_string(propertyValue)) == NULL)
        {
            LogError("Unable to retrieve JSON string for property value");
            result = IOTHUB_CLIENT_ERROR;
            break;
        }
            
        // If the child element is NOT an object OR its not a model the application knows about, this is a property of the model's root component.
        properties[*numComponentProperties].version = IOTHUB_CLIENT_DESERIALIZED_PROPERTY_VERSION;
        properties[*numComponentProperties].propertyType = propertyType;
        properties[*numComponentProperties].componentName = componentName;
        properties[*numComponentProperties].propertyName = propertyName;
        properties[*numComponentProperties].propertyValue = propertyStringJson;

        *numComponentProperties = (*numComponentProperties) + 1;
    }

    if (i == numChildren)
    {
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// FillProperties retrieves properties that are children of jsonObject and puts them into properties, starting at the 0 index.
static IOTHUB_CLIENT_RESULT FillProperties(IOTHUB_CLIENT_PROPERTY_TYPE propertyType, const char** componentsInModel, size_t numComponentsInModel, JSON_Object* jsonObject, IOTHUB_CLIENT_DESERIALIZED_PROPERTY* properties)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;

    size_t numChildren;
    size_t currentProperty = 0;
    size_t i;

    numChildren = json_object_get_count(jsonObject);

    for (i = 0; i < numChildren; i++)
    {
        const char* name = json_object_get_name(jsonObject, i);
        JSON_Value* value = json_object_get_value_at(jsonObject, i);

        if ((name == NULL) || (value == NULL))
        {
            LogError("Unable to retrieve name and/or value from index %lu", i);
            result = IOTHUB_CLIENT_ERROR;
            break;
        }
        else if (strcmp(name, TWIN_VERSION) == 0)
        {
            // The version field is metadata and not user property data.  Skip.
            continue;
        }
        else if ((json_type(value) == JSONObject) && IsJsonObjectAComponentInModel(name, componentsInModel, numComponentsInModel))
        {
            size_t numComponentProperties = 0;

            if ((result = FillPropertiesForComponent(propertyType, name, value, properties + currentProperty, &numComponentProperties)) != IOTHUB_CLIENT_ERROR)
            {
                LogError("Cannot set properties for component %s", name);
                break;
            }
            currentProperty += numComponentProperties;
        }
        else
        {
            const char* propertyStringJson;

            if ((propertyStringJson = json_serialize_to_string(value)) == NULL)
            {
                LogError("Unable to retrieve JSON string for property value");
                result = IOTHUB_CLIENT_ERROR;
                break;
            }
                
            // If the child element is NOT an object OR its not a model the application knows about, this is a property of the model's root component.
            properties[currentProperty].version = IOTHUB_CLIENT_DESERIALIZED_PROPERTY_VERSION;
            properties[currentProperty].propertyType = propertyType;
            properties[currentProperty].componentName = NULL;
            properties[currentProperty].propertyName = name;
            properties[currentProperty].propertyValue = propertyStringJson;

            currentProperty++;
            
        }
    }

    if (i == numChildren)
    {
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* serializedProperties,
    size_t serializedPropertiesLength,
    const char** componentsInModel,
    size_t numComponentsInModel,
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY** properties,
    size_t* numProperties,
    int* propertiesVersion)
{
    IOTHUB_CLIENT_RESULT result;
    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* desiredObject;
    JSON_Object* reportedObject;

    IOTHUB_CLIENT_DESERIALIZED_PROPERTY* propertiesBuffer = NULL;
    size_t totalProperties = 0;
    size_t numDesiredProperties;
    size_t numReportedProperties;

    if ((serializedProperties == NULL) || (serializedPropertiesLength == 0) || (numProperties == 0) || (properties == NULL) || (numProperties == 0) || (propertiesVersion == 0))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if ((jsonStr = CopyPayloadToString(serializedProperties, serializedPropertiesLength)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse device twin JSON");
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((result = GetDesiredAndReportedTwinJson(payloadType, rootValue, &desiredObject, &reportedObject)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot retrieve desired and/or reported object from JSON");
    }
    else if ((result = GetTwinVersion(desiredObject, propertiesVersion)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot retrieve properties version");
    }
    else if ((result = GetNumberOfProperties(componentsInModel, numComponentsInModel, desiredObject, &numDesiredProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else if ((result = GetNumberOfProperties(componentsInModel, numComponentsInModel, reportedObject, &numReportedProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of reported properties");
    }
    else if ((totalProperties = numDesiredProperties + numReportedProperties) == 0)
    {
        // No properties available is not an error but does mean we should stop further processing.
        result = IOTHUB_CLIENT_OK;
    }
    else if ((propertiesBuffer = calloc(totalProperties, sizeof(IOTHUB_CLIENT_DESERIALIZED_PROPERTY))) == NULL)
    {
        LogError("Cannot allocate %ul properties", totalProperties);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((result = FillProperties(IOTHUB_CLIENT_PROPERTY_TYPE_WRITEABLE, componentsInModel, numComponentsInModel, desiredObject, propertiesBuffer)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else if ((result = FillProperties(IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE, componentsInModel, numComponentsInModel, reportedObject, propertiesBuffer + numDesiredProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else
    {
        result = IOTHUB_CLIENT_OK;
    }

    if ((result != IOTHUB_CLIENT_OK) && (propertiesBuffer != NULL))
    {
        IoTHubClient_Deserialized_Properties_Destroy(propertiesBuffer, totalProperties);
    }
    else
    {
        *properties = propertiesBuffer;
        *numProperties = totalProperties;
    }
    
    json_value_free(rootValue);
    free(jsonStr);

    return result;
}

void IoTHubClient_Deserialized_Properties_Destroy(
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY* properties,
    size_t numProperties)
{
    (void)properties;
    (void)numProperties;
}

typedef enum COMPONENT_PARSE_STATE_TAG
{
    COMPONENT_PARSE_INIT,
    COMPONENT_PARSE_ROOT,
    COMPONENT_PARSE_SUB_COMPONENT,
    COMPONENT_PARSE_COMPLETE
} COMPONENT_PARSE_STATE;

typedef enum PROPERTY_PARSE_STATE_TAG
{
    PROPERTY_PARSE_INIT,
    PROPERTY_PARSE_DESIRED,
    PROPERTY_PARSE_REPORTED,
    PROPERTY_PARSE_COMPLETE
} PROPERTY_PARSE_STATE;

typedef struct IOTHUB_CLIENT_PROPERTY_CONTEXT_TAG {
    const char** componentsInModel;
    size_t numComponentsInModel;
    JSON_Value* rootValue;
    JSON_Object* desiredObject;
    JSON_Object* reportedObject;
    COMPONENT_PARSE_STATE componentParseState;
    PROPERTY_PARSE_STATE propertyParseState;
    int currentComponentIndex;
    int currentPropertyIndex;
} IOTHUB_CLIENT_PROPERTY_CONTEXT;

    
IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties_CreateIterator(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* payLoad,
    size_t payLoadLength,
    const char** componentsInModel,
    size_t numComponentsInModel,
    IOTHUB_CLIENT_PROPERTY_CONTEXT_HANDLE* propertyContextHandle,
    int* propertiesVersion)
{
    IOTHUB_CLIENT_RESULT result;
    char* jsonStr = NULL;

    IOTHUB_CLIENT_PROPERTY_CONTEXT* propertyContext = calloc(1, sizeof(IOTHUB_CLIENT_PROPERTY_CONTEXT));

    if ((payLoad == NULL) || (payLoadLength == 0) || (propertyContextHandle == NULL) || (propertiesVersion == 0))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        memset(propertyContext, 0, sizeof(*propertyContext));

        if ((jsonStr = CopyPayloadToString(payLoad, payLoadLength)) == NULL)
        {
            LogError("Unable to allocate twin buffer");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if ((propertyContext->rootValue = json_parse_string(jsonStr)) == NULL)
        {
            LogError("Unable to parse device twin JSON");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if ((result = GetDesiredAndReportedTwinJson(payloadType, propertyContext->rootValue, &propertyContext->desiredObject, &propertyContext->reportedObject)) != IOTHUB_CLIENT_OK)
        {
            LogError("Cannot retrieve desired and/or reported object from JSON");
        }
        else if ((result = GetTwinVersion(propertyContext->desiredObject, propertiesVersion)) != IOTHUB_CLIENT_OK)
        {
            LogError("Cannot retrieve properties version");
        }
        else
        {
            // TODO: These need to be copied (unfortunately) per general contract of SDK.  OK for now for proof of concept.
            propertyContext->componentsInModel = componentsInModel;
            propertyContext->numComponentsInModel = numComponentsInModel;
            propertyContext->componentParseState = COMPONENT_PARSE_INIT;
            propertyContext->propertyParseState = PROPERTY_PARSE_INIT;
            propertyContext->currentObject = NULL; // TODO: Figure out initial setting based on various combinitions of desired|reported, root|no-root.
            *propertyContextHandle = propertyContext;
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}


bool HasNonComponentProperties(IOTHUB_CLIENT_PROPERTY_CONTEXT* propertyContext)
{

}

IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties_GetNextComponent(
    IOTHUB_CLIENT_PROPERTY_CONTEXT_HANDLE propertyContextHandle,
    const char** componentName,
    bool* componentSpecified)
{
    IOTHUB_CLIENT_PROPERTY_CONTEXT* propertyContext = (IOTHUB_CLIENT_PROPERTY_CONTEXT*)propertyContextHandle;
    JSON_Object* currentObject;
    IOTHUB_CLIENT_RESULT result;
    bool processRoot = false;

    // HasNonComponentProperties will enumerate all properties under root (desired or desired|reported, depending)
    // If it's 100% components, then don't bother returning back a *componentName=NULL to caller for 0 properties.

    if (propertyContext->componentParseState == COMPONENT_PARSE_COMPLETE)
    {
        // It's not valid for caller to repeatedly call us after we've told it we're done.
        return IOTHUB_CLIENT_INVALID_ARG;
    }

    if (propertyContext->componentParseState == COMPONENT_PARSE_INIT)
    {
        if (HasNonComponentProperties(propertyContext))
        {
            // If there are JSON properterties that are NOT mapped to a component, then we start with these.
            propertyContext->componentParseState = COMPONENT_PARSE_ROOT;
            *componentName = NULL;
            return IOTHUB_CLIENT_OK;
        }
        else
        {
            // All the underlying JSON properties map to components, and not properties of the root components.
            propertyContext->componentParseState = COMPONENT_PARSE_SUB_COMPONENT;
        }
    }

    if (property->componentParseState == COMPONENT_PARSE_ROOT)
    {
        // After application has processed the root, transition to sub-component parsing.
        propertyContext->componentParseState = COMPONENT_PARSE_SUB_COMPONENT;
    }

    if (propertyContext->componentParseState == COMPONENT_PARSE_SUB_COMPONENT)
    {
        for (; propertyContext->currentComponentIndex < propertyContext->numComponentsInModel; currentIndex++)
        {
            // We're enumerating components now.  Only return a component if it has properties.
            if (CurrentComponentHasProperties(propertyContext, componentName))
            {
                // Reset property enumeration state for subsequent app calls to IoTHubClient_Deserialize_Properties_GetNextProperty
                propertyContext->propertyParseState = PROPERTY_PARSE_INIT;
                propertyContext->currentPropertyIndex = 0;
                *componentSpecified = true;
                return IOTHUB_CLIENT_OK;
            }
        }
    }

    propertyContext->componentParseState = COMPONENT_PARSE_COMPLETE;
    *componentName = NULL;
    *componentSpecified = false;

    return IOTHUB_CLIENT_OK;
}


IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties_GetNextProperty(
    IOTHUB_CLIENT_PROPERTY_CONTEXT_HANDLE propertyContextHandle,
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property,
    bool* propertySpecified)
{
    IOTHUB_CLIENT_PROPERTY_CONTEXT* propertyContext = (IOTHUB_CLIENT_PROPERTY_CONTEXT*)propertyContextHandle;

    JSON_Object* currentObject;

    if (propertyContext->propertyParseState == PROPERTY_PARSE_COMPLETE)
    {
        // It's not valid for caller to repeatedly call us after we've told it we're done.
        return IOTHUB_CLIENT_INVALID_ARG;
    }

    if (propertyContext->propertyParseState == PROPERTY_PARSE_INIT)
    {
        if (GetDesiredPropertyForCurrentComponent(propertyContext))
        {
            propertyContext->propertyParseState = PROPERTY_PARSE_DESIRED;
        }
        else if (GetDesiredPropertyForCurrentComponent(propertyContext))
        {
            propertyContext->propertyParseState = PROPERTY_PARSE_REPORTED;
        }
        else
        {
            // We shouldn't be able to get here since initial component enum guarantees at least one property.
            return IOTHUB_CLIENT_ERROR;
        }
    }

    const char* componentName = (propertyContext->componentParseState == COMPONENT_PARSE_ROOT) ? NULL : propertyContext->componentsInModel[propertyContext->currentComponentIndex];
    JSON_Object* currentObject;

    if (propertyContext->propertyParseState == PROPERTY_PARSE_DESIRED)
    {
        currentObject = (propertyContext->componentParseState == COMPONENT_PARSE_ROOT) : propertyContext->desiredObject : json_object_get_object(propertyContext->desiredObject, componentName);

        if (propertyContext->componentParseState == COMPONENT_PARSE_ROOT)
        {
            // Skip over any subcomponents
        }

        if (json_object_get_count(currentObject) >= propertyContext->currentPropertyIndex)
        {
            propertyContext->currentPropertyIndex = 0;
            propertyContext->propertyParseState = PROPERTY_PARSE_REPORTED;
        }
    }

    if (propertyContext->propertyParseState = PROPERTY_PARSE_REPORTED)
    {
        currentObject = (propertyContext->componentParseState == COMPONENT_PARSE_ROOT) : propertyContext->reportedObject : json_object_get_object(propertyContext->reportedObject, componentName);

        if (propertyContext->componentParseState == COMPONENT_PARSE_ROOT)
        {
            // Skip over any subcomponents
        }

        if (json_object_get_count(currentObject) >= propertyContext->currentPropertyIndex)
        {
            propertyContext->propertyParseState = PROPERTY_PARSE_COMPLETE;
            *property = NULL;
            *propertySpecified = false;
        }

    }

    // Fills in IOTHUB_CLIENT_DESERIALIZED_PROPERTY based on where we're at in the traversal.
    FillProperties(currentObject, properties);

    propertyContext->currentPropertyIndex++;
    return IOTHUB_CLIENT_OK;
}

void IoTHubClient_Deserialize_Properties_FreeProperty(
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property)
{
    if (property != NULL)
    {
        json_free_serialized_string((char*)property->value);
    }
}

void IoTHubClient_Deserialize_Properties_End(IOTHUB_CLIENT_PROPERTY_CONTEXT_HANDLE propertyContextHandle)
{
    (void)propertyContextHandle;
}
    
