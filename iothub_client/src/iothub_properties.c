// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothub_properties.h"
#include "parson.h"

IOTHUB_CLIENT_RESULT IoTHub_Serialize_Properties(
    const IOTHUB_PROPERTY* properties,
    size_t numProperties,
    const char* componentName,
    unsigned char** serializedProperties,
    size_t* serializedPropertiesLength)
{
/*
    // First step is to calculate the length
    size_t requiredBytes = 0;
    size_t additionalBytes;
    
    if (componentName != NULL)
    {
        additionalBytes = sprintf(NULL, "{%s:_t:c", componentName);
        requiredBytes += additionalBytes;
    }

    for (size_t i = 0; i < numProperties; i++)
    {
        additionalBytes += sprintf(NULL, "%s:%s", properties[i]->name, properties[i]->value);
        if (i != (numProperties -1))
            additionalBytes += 1; // for the ','
    }

    if (componentName != NULL)
    {
        additionalBytes = sprintf(NULL, "}", componentName); // or maybe just ++ if it's just 1 byte?
        requiredBytes += additionalBytes;
    }
    additionalBytes++; // for the closing '}'

    *serializedProperties = malloc(...);
    *serializedPropertiesLength = bytes;

    building the string is analogous.  Investigate a single function for both that just either takes a buffer or it doesn't?
*/

    (void)properties;
    (void)numProperties; 
    (void)componentName;
    (void)serializedProperties;
    (void)serializedPropertiesLength;
    return 0;
}


IOTHUB_CLIENT_RESULT IoTHub_Serialize_ResponseToWriteableProperties(
    const IOTHUB_WRITEABLE_PROPERTY_RESPONSE* properties,
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

IOTHUB_CLIENT_RESULT IoTHub_Deserialize_WriteableProperty(
    IOTHUB_WRITEABLE_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* serializedProperties,
    size_t serializedPropertiesLength,
    const char** componentName,
    size_t numComponents,
    IOTHUB_WRITEABLE_PROPERTY** writeProperties,
    size_t* numWriteableProperties,
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
    GetNumberPropertiesAndAlloc(pnpComponents, numPnPComponents, writeProperties, numWriteableProperties);

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
    (void)componentName;
    (void)numComponents;
    (void)writeProperties;
    (void)numWriteableProperties;
    (void)propertiesVersion;
    return 0;
}

