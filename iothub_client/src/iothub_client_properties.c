// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "iothub_client_properties.h"
#include "parson.h"

static const char PROPERTY_FORMAT_COMPONENT_START[] = "{%s:_t:c";
static const char PROPERTY_FORMAT_NAME_VALUE[] = "%s:%s";
static const char PROPERTY_FORMAT_WRITEABLE_RESPONSE[] = "{\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d}}";
static const char PROPERTY_FORMAT_WRITEABLE_RESPONSE_WITH_DESCRIPTION[] = "{\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}}";

static const char PROPERTY_FORMAT_OPEN_BRACE[] = "{";
static const char PROPERTY_FORMAT_CLOSE_BRACE[] = "}";

static const char TWIN_DESIRED_OBJECT_NAME[] = "desired";
static const char TWIN_REPORTED_OBJECT_NAME[] = "reported";
static const char TWIN_VERSION[] = "$version";

// IoTHub adds a JSON field "__t":"c" into desired top-level JSON objects that represent components.  Without this marking, the object 
// is treated as a property off the root component.
static const char TWIN_COMPONENT_MARKER[] = "__t";



// After builder functions have successfully invoked snprintf, update our traversal pointer and remaining lengths
static void AdvanceCountersAfterWrite(size_t currentOutputBytes, char** currentWrite, size_t* requiredBytes, size_t* remainingBytes)
{
    // The number of bytes to be required is always updated by number the snprintf wrote to.
    *requiredBytes += currentOutputBytes;
    if (*currentWrite != NULL)
    {
        // if *currentWrite is NULL, then we leave it as NULL since the caller is doing its required buffer calculation.
        // Otherwise we need to update it for the next write.
        *currentWrite += currentOutputBytes;
        *remainingBytes -= currentOutputBytes;
    }
}

// Adds opening brace for JSON to be generated, either a simple "{" or more complex if componentName is specified
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
        if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_OPEN_BRACE)) < 0)
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
        if ((currentOutputBytes += snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_NAME_VALUE, properties[i].name, properties[i].value)) < 0)
        {
            LogError("Cannot build properites string");
            return (size_t)-1;
        }
        AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);
    }

    if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_CLOSE_BRACE)) < 0)
    {
        LogError("Cannot build properites string");
        return (size_t)-1;
    }
    AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);
 
    return requiredBytes;
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
        if (properties[i].description == NULL)
        {
            if ((currentOutputBytes += snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_WRITEABLE_RESPONSE, properties[i].name, properties[i].value, properties[i].result, properties[i].ackVersion)) < 0)
            {
                LogError("Cannot build properites string");
                return (size_t)-1;
            }
        }
        else
        {
            if ((currentOutputBytes += snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_WRITEABLE_RESPONSE_WITH_DESCRIPTION, properties[i].name, properties[i].value, properties[i].result, properties[i].ackVersion, properties[i].description)) < 0)
            {
                LogError("Cannot build properites string");
                return (size_t)-1;
            }
        }

        AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, &requiredBytes, &remainingBytes);
    }

    if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_CLOSE_BRACE)) < 0)
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
            // For a complete update, the JSON from IoTHub will contain both "desired" and "reported" - the full twin.
            // We only care about "desired" in this sample, so just retrieve it.

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

// GetTwinVersion retrieves the $version field from JSON
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
// is in componentsInModel that the application passed into us.
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

// GetNumberProperties examines the jsonObject to retrieve the number of properties.
static IOTHUB_CLIENT_RESULT GetNumberProperties(const char** componentsName, size_t numComponents, JSON_Object* jsonObject, size_t *numProperties)
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
        else if ((json_type(value) == JSONObject) && IsJsonObjectAComponentInModel(name, componentsName, numComponents))
        {
            // If this current JSON is an element AND the name is one of the componentsInModel that the application knows about,
            // then this json element represents a component.
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
        
        if ((propertyStringJson = json_string(propertyValue)) == NULL)
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


static IOTHUB_CLIENT_RESULT FillProperties(IOTHUB_CLIENT_PROPERTY_TYPE propertyType, const char** componentsName, size_t numComponents, JSON_Object* jsonObject, IOTHUB_CLIENT_DESERIALIZED_PROPERTY* properties)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;

    size_t numChildren;
    size_t currentProperty = 0;

    numChildren = json_object_get_count(jsonObject);

    for (size_t i = 0; i < numChildren; i++)
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
        else if ((json_type(value) == JSONObject) && IsJsonObjectAComponentInModel(name, componentsName, numComponents))
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

            if ((propertyStringJson = json_string(value)) == NULL)
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

    return result;
}


IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* serializedProperties,
    size_t serializedPropertiesLength,
    const char** componentsName,
    size_t numComponents,
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
    size_t numDesiredProperties = 0;
    size_t numReportedProperties = 0;

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
    else if ((result = GetNumberProperties(componentsName, numComponents, desiredObject, &numDesiredProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else if ((result = GetNumberProperties(componentsName, numComponents, reportedObject, &numReportedProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of reported properties");
    }
    else if ((propertiesBuffer = calloc(numDesiredProperties + numReportedProperties, sizeof(IOTHUB_CLIENT_DESERIALIZED_PROPERTY))) == NULL)
    {
        LogError("Cannot allocate %ul properties", numDesiredProperties + numReportedProperties);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((result = FillProperties(IOTHUB_CLIENT_PROPERTY_TYPE_WRITEABLE, componentsName, numComponents, desiredObject, propertiesBuffer)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else if ((result = FillProperties(IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE, componentsName, numComponents, reportedObject, propertiesBuffer + numDesiredProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else
    {
        result = IOTHUB_CLIENT_OK;
    }

    if ((result != IOTHUB_CLIENT_OK) && (propertiesBuffer != NULL))
    {
        IoTHubClient_Deserialized_Properties_Destroy(propertiesBuffer, numDesiredProperties + numReportedProperties);
    }
    else
    {
        *properties = propertiesBuffer;
        *numProperties = numDesiredProperties + numReportedProperties;
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
