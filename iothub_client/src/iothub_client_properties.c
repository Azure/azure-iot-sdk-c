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
    else if ((result = GetVersion(desiredObject, propertiesVersion)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot retrieve properties version");
    }
    else if ((result = GetNumberProperties(IOTHUB_CLIENT_PROPERTY_TYPE_WRITEABLE, componentsName, numComponents, desiredObject, &numDesiredProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else if ((result = GetNumberProperties(IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE,  componentsName, numComponents, reportedObject, &numReportedProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of reported properties");
    }
    else if ((propertiesBuffer = calloc(numDesiredProperties + numReportedProperties, sizeof(IOTHUB_CLIENT_DESERIALIZED_PROPERTY))) == NULL)
    {
        LogError("Cannot allocate %ul properties", numDesiredProperties + numReportedProperties);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((result = FillProperties(IOTHUB_CLIENT_PROPERTY_TYPE_WRITEABLE,  componentsName, numComponents, desiredObject, numDesiredProperties, propertiesBuffer)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else if ((result = FillProperties(IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE,  componentsName, numComponents, reportedObject, numReportedProperties, propertiesBuffer + numDesiredProperties)) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot determine number of desired properties");
    }
    else
    {
        result = IOTHUB_CLIENT_OK;
    }
    
    json_value_free(rootValue);
    free(jsonStr);

    return result;
}

void IoTHubClient_Deserialized_Properties_Destroy(
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY** properties,
    size_t* numProperties)
{
    (void)properties;
    (void)numProperties;
}

