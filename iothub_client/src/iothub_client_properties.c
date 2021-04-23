// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "iothub_client_properties.h"
#include "parson.h"

static const char PROPERTY_FORMAT_COMPONENT_START[] = "{%s:_t:c";
static const char PROPERTY_FORMAT_NAME_VALUE[] = "%s:%s";
static const char PROPERTY_FORMAT_OPEN_BRACE[] = "{";
static const char PROPERTY_FORMAT_CLOSE_BRACE[] = "}";

// BuildReportedProperties is used to build up the actual serializedProperties string based 
// on the properties.  If serializedProperties==NULL and serializedPropertiesLength=0, just like
// analogous snprintf it will just calculate the amount of space caller needs to allocate.
static size_t BuildReportedProperties(const IOTHUB_CLIENT_REPORTED_PROPERTY* properties, size_t numProperties, const char* componentName, unsigned char* serializedProperties, size_t serializedPropertiesLength)
{
    size_t requiredBytes = 0;
    size_t currentOutputBytes;
    size_t remainingBytes = serializedPropertiesLength;
    char* currentWrite = (char*)serializedProperties;

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
    requiredBytes += currentOutputBytes;
    if (currentWrite != NULL)
    {
        currentWrite += currentOutputBytes;
        remainingBytes -= currentOutputBytes;
    }

    for (size_t i = 0; i < numProperties; i++)
    {
        if ((currentOutputBytes += snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_NAME_VALUE, properties[i].name, properties[i].value)) < 0)
        {
            LogError("Cannot build properites string");
            return (size_t)-1;
        }
        requiredBytes += currentOutputBytes;
        if (currentWrite != NULL)
        {
            currentWrite += currentOutputBytes;
            remainingBytes -= currentOutputBytes;
        }
    }

    if ((currentOutputBytes = snprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_CLOSE_BRACE)) < 0)
    {
        LogError("Cannot build properites string");
        return (size_t)-1;
    }
    requiredBytes += currentOutputBytes;
 
    return requiredBytes;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Serialize_ReportedProperties(const IOTHUB_CLIENT_REPORTED_PROPERTY* properties, size_t numProperties, const char* componentName, unsigned char** serializedProperties, size_t* serializedPropertiesLength)
{
    IOTHUB_CLIENT_RESULT result;
    size_t requiredBytes = 0;
    unsigned char* serializedPropertiesBuffer = NULL;

    if ((properties == NULL) || (numProperties == 0) || (serializedProperties == NULL) || (serializedPropertiesLength == 0))
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
/*
    // analogous to above just more complicated
*/
    (void)properties;
    (void)numProperties;
    (void)componentName;
    (void)serializedProperties;
    (void)serializedPropertiesLength;
    return 0;
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
/*
    const char* jsonString = CopyToString(serializedProperties, serializedPropertiesLength); // per parson demanding \0 closure
    rootValue = json_parse_string(jsonString);
    free(jsonString); // not needed so discard early

    desiredObject = GetDesiredJson(payloadType); // depends on whether there's a "Desired." first or not.

    // make sure version is present, is a number, etc.
    version = (int)json_value_get_number(versionValue); 

    // Simple visitor that goes around tree and counts # of properties needed for the writeProperties struct and allocs
    GetNumberPropertiesAndAlloc(componentsNames, numComponents, writeProperties, numWriteableProperties);

    // Note that we need to do same thing if the reported is present.
    numChildren = json_object_get_count(desiredObject);
    for (size_t i = 0; i < numChildren; i++) {
        const char* name = json_object_get_name(desiredObject, i);
        JSON_Value* value = json_object_get_value_at(desiredObject, i);
        /// special case if Desired && $version
        if ((json_type(value) == JSONObject) && IsJsonObjectAComponentInModel(name, componentsInModel, numComponentsInModel)) {
            VisitComponentProperties(..., &currentProperty); // this behaves the same way, and udpates property we're writing to.
        }
        else {
            writeProperties[currentProperty].version = STRUCT_VER;
            writeProperties[currentProperty].propertyType = desired|reported;
            writeProperties[currentProperty].componentName = NULL;
            writeProperties[currentProperty].propertyName = name;
            writeProperties[currentProperty].propertyValue = json_serialize_to_string(value);
        }
        currentProperty++;
    }
}
*/
    (void)payloadType;
    (void)serializedProperties;
    (void)serializedPropertiesLength;
    (void)componentsName;
    (void)numComponents;
    (void)properties;
    (void)numProperties;
    (void)propertiesVersion;
    return 0;
}

