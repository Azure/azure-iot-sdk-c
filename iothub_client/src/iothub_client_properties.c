// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdarg.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include "iothub_client_properties.h"
#include "parson.h"

typedef enum COMPONENT_PARSE_STATE_TAG
{
    COMPONENT_PARSE_STATE_ROOT,
    COMPONENT_PARSE_STATE_SUB_COMPONENT,
} COMPONENT_PARSE_STATE;

typedef enum PROPERTY_PARSE_STATE_TAG
{
    PROPERTY_PARSE_STATE_DESIRED,
    PROPERTY_PARSE_STATE_REPORTED
} PROPERTY_PARSE_STATE;

typedef struct IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_TAG {
    JSON_Value* rootValue;
    JSON_Object* desiredObject;
    JSON_Object* reportedObject;
    COMPONENT_PARSE_STATE componentParseState;
    PROPERTY_PARSE_STATE propertyParseState;
    size_t currentComponentIndex;
    size_t currentPropertyIndex;
    int propertiesVersion;
} IOTHUB_CLIENT_PROPERTIES_DESERIALIZER;

// sprintf format strings and string constants for writing properties.
static const char PROPERTY_FORMAT_COMPONENT_START[] = "{\"%s\":{\"__t\":\"c\",";
static const char PROPERTY_FORMAT_NAME_VALUE[] = "\"%s\":%s%s";
static const char PROPERTY_FORMAT_WRITABLE_RESPONSE[] = "\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d}%s";
static const char PROPERTY_FORMAT_WRITABLE_RESPONSE_WITH_DESCRIPTION[] = "\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}%s";
static const char PROPERTY_OPEN_BRACE[] = "{";
static const char PROPERTY_CLOSE_BRACE[] = "}";
static const char PROPERTY_COMMA[] = ",";
static const char PROPERTY_EMPTY[] = "";

// metadata in underlying device twin
static const char TWIN_DESIRED_OBJECT_NAME[] = "desired";
static const char TWIN_REPORTED_OBJECT_NAME[] = "reported";
static const char TWIN_VERSION[] = "$version";
// IoTHub adds a JSON field "__t":"c" into desired top-level JSON objects that represent components.  Without this marking, the object 
// is treated as a property of the root component.
static const char TWIN_COMPONENT_MARKER_NAME[] = "__t";
static const char TWIN_COMPONENT_MARKER_VALUE[] = "c";

// When writing a list of properties, whether to append a "," or nothing depends on whether more items are coming.
static const char* AddCommaIfNeeded(bool lastItem)
{
    return lastItem ? PROPERTY_EMPTY : PROPERTY_COMMA;
}

// AdvanceCountersAfterWrite updates the traversal pointer and lengths after snprintf'ng JSON
static void AdvanceCountersAfterWrite(int currentOutputBytes, char** currentWrite, size_t* requiredBytes, size_t* remainingBytes)
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

// To correctly check snprintf for success, we need to check both it's not negative AND 
// that the (number of bytes written ) != (sizeof buffer).
static int PropertySnprintf(char* buffer, size_t count, const char* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    int currentOutputBytes = vsnprintf(buffer, count, format, arg_list);
    if ((currentOutputBytes < 0) || (currentOutputBytes == (int)count))
    {
        return -1;
    }

    return currentOutputBytes;
}


// WriteOpeningBrace adds opening brace for JSON to be generated, either a simple "{" or more complex format if componentName is specified.
static bool WriteOpeningBrace(const char* componentName, char** currentWrite, size_t* requiredBytes, size_t* remainingBytes)
{
    int currentOutputBytes;

    if (componentName != NULL)
    {
        if ((currentOutputBytes = PropertySnprintf(*currentWrite, *remainingBytes, PROPERTY_FORMAT_COMPONENT_START, componentName)) < 0)
        {
            LogError("Cannot write properties string");
            return false;
        }
    }
    else
    {
        if ((currentOutputBytes = PropertySnprintf(*currentWrite, *remainingBytes, PROPERTY_OPEN_BRACE)) < 0)
        {
            LogError("Cannot write properties string");
            return false;
        }
    }

    AdvanceCountersAfterWrite(currentOutputBytes, currentWrite, requiredBytes, remainingBytes);
    return true;
}

// WriteOpeningBrace writes the required number of closing } while writing JSON
static bool WriteClosingBrace(bool isComponent, char** currentWrite, size_t* requiredBytes, size_t* remainingBytes)
{
    int currentOutputBytes;
    size_t numberOfBraces = isComponent ? 2 : 1;

    for (size_t i = 0; i < numberOfBraces; i++)
    {
        if ((currentOutputBytes = PropertySnprintf(*currentWrite, *remainingBytes, PROPERTY_CLOSE_BRACE)) < 0)
        {
            LogError("Cannot write properties string");
            return false;
        }
        AdvanceCountersAfterWrite(currentOutputBytes, currentWrite, requiredBytes, remainingBytes);
    }

    return true;
}

// WriteReportedProperties writes the actual serializedProperties string based 
// on the properties.  If serializedProperties==NULL and serializedPropertiesLength==0, just like
// analogous snprintf it will just calculate the amount of space caller needs to allocate.
static bool WriteReportedProperties(const IOTHUB_CLIENT_PROPERTY_REPORTED* properties, size_t numProperties, const char* componentName, unsigned char* serializedProperties, size_t serializedPropertiesLength, size_t* requiredBytes)
{
    char* currentWrite = (char*)serializedProperties;
    size_t remainingBytes = serializedPropertiesLength;

    *requiredBytes = 0;

    if (WriteOpeningBrace(componentName,  &currentWrite, requiredBytes, &remainingBytes) != true)
    {
        LogError("Cannot write properties string");
        return false;
    }

    for (size_t i = 0; i < numProperties; i++)
    {
        bool lastProperty = (i == (numProperties - 1));
        int currentOutputBytes;

        if ((currentOutputBytes = PropertySnprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_NAME_VALUE, 
                                                    properties[i].name, properties[i].value, AddCommaIfNeeded(lastProperty))) < 0)
        {
            LogError("Cannot write properties string");
            return false;
        }
        AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, requiredBytes, &remainingBytes);
    }

    if (WriteClosingBrace(componentName != NULL, &currentWrite, requiredBytes, &remainingBytes) != true)
    {
        LogError("Cannot write properties string");
        return false;
    }

    *requiredBytes = (*requiredBytes + 1);
    return true;
}

// WriteWritableResponseProperties is used to write serializedProperties string based 
// on the writable response properties.  If serializedProperties==NULL and serializedPropertiesLength==0, just like
// analogous snprintf it will just calculate the amount of space caller needs to allocate.
static bool WriteWritableResponseProperties(const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE* properties, size_t numProperties, const char* componentName, unsigned char* serializedProperties, size_t serializedPropertiesLength, size_t* requiredBytes)
{
    char* currentWrite = (char*)serializedProperties;
    size_t remainingBytes = serializedPropertiesLength;

    *requiredBytes = 0;

    if (WriteOpeningBrace(componentName,  &currentWrite, requiredBytes, &remainingBytes) != true)
    {
        LogError("Cannot write properties string");
        return false;
    }

    for (size_t i = 0; i < numProperties; i++)
    {
        bool lastProperty = (i == (numProperties - 1));
        int currentOutputBytes;

        if (properties[i].description == NULL)
        {
            if ((currentOutputBytes = PropertySnprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_WRITABLE_RESPONSE, properties[i].name, 
                                                        properties[i].value, properties[i].result, properties[i].ackVersion, AddCommaIfNeeded(lastProperty))) < 0)
            {
                LogError("Cannot write properties string");
                return false;
            }
        }
        else
        {
            if ((currentOutputBytes = PropertySnprintf(currentWrite, remainingBytes, PROPERTY_FORMAT_WRITABLE_RESPONSE_WITH_DESCRIPTION, properties[i].name, 
                                                        properties[i].value, properties[i].result, properties[i].ackVersion, properties[i].description, AddCommaIfNeeded(lastProperty))) < 0)
            {
                LogError("Cannot write properties string");
                return false;
            }
        }

        AdvanceCountersAfterWrite(currentOutputBytes, &currentWrite, requiredBytes, &remainingBytes);
    }

    if (WriteClosingBrace(componentName != NULL, &currentWrite, requiredBytes, &remainingBytes) != true)
    {
        LogError("Cannot write properties string");
        return false;
    }

    *requiredBytes = (*requiredBytes + 1);
    return true;
}

static bool VerifySerializeReportedProperties(const IOTHUB_CLIENT_PROPERTY_REPORTED* properties, size_t numProperties)
{
    bool result = false;

    if ((properties == NULL) || (numProperties == 0))
    {
        LogError("Invalid properties");
        result = false;
    }
    else
    {
        size_t i;
        for (i = 0; i < numProperties; i++)
        {
            if ((properties[i].structVersion != IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1) || (properties[i].name == NULL) || (properties[i].value == NULL))
            {
                LogError("Property at index %lu is invalid", (unsigned long)i);
                break;
            }
        }

        result = (i == numProperties);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Properties_Serializer_CreateReported(const IOTHUB_CLIENT_PROPERTY_REPORTED* properties, size_t numProperties, const char* componentName, unsigned char** serializedProperties, size_t* serializedPropertiesLength)
{
    IOTHUB_CLIENT_RESULT result;
    size_t requiredBytes = 0;
    unsigned char* serializedPropertiesBuffer = NULL;

    if ((VerifySerializeReportedProperties(properties, numProperties) == false) || (serializedProperties == NULL) || (serializedPropertiesLength == NULL))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if (WriteReportedProperties(properties, numProperties, componentName, NULL, 0, &requiredBytes) != true)
    {
        LogError("Cannot determine required length of reported properties buffer");
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((serializedPropertiesBuffer = calloc(1, requiredBytes)) == NULL)
    {
        LogError("Cannot allocate %lu bytes", (unsigned long)requiredBytes);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if (WriteReportedProperties(properties, numProperties, componentName, serializedPropertiesBuffer, requiredBytes, &requiredBytes) != true)
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
        // We allocated an additional byte than we technically need so snprintf() could output null-terminator.
        // The APIs this output pairs up with (e.g. IoTHubDeviceClient_LL_SendPropertiesAsync()) do not want
        // this null-terminator since the bytes are memcpy'd directly to network.  So account for this here.
        *serializedPropertiesLength = requiredBytes - 1;
    }
    else if (serializedPropertiesBuffer != NULL)
    {
        free(serializedPropertiesBuffer);
    }

    return result;
}

static bool VerifySerializeWritableReportedProperties(const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE* properties, size_t numProperties)
{
    bool result = false;

    if ((properties == NULL) || (numProperties == 0))
    {
        LogError("Invalid properties");
        result = false;
    }
    else
    {
        size_t i;
        for (i = 0; i < numProperties; i++)
        {
            if ((properties[i].structVersion != IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1) || (properties[i].name == NULL) || (properties[i].value == NULL))
            {
                LogError("Property at index %lu is invalid", (unsigned long)i);
                break;
            }
        }

        result = (i == numProperties);
    }

    return result;
}


IOTHUB_CLIENT_RESULT IoTHubClient_Properties_Serializer_CreateWritableResponse(
    const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE* properties,
    size_t numProperties,
    const char* componentName,
    unsigned char** serializedProperties,
    size_t* serializedPropertiesLength)
{
    IOTHUB_CLIENT_RESULT result;
    size_t requiredBytes = 0;
    unsigned char* serializedPropertiesBuffer = NULL;

    if ((VerifySerializeWritableReportedProperties(properties, numProperties) == false) || (serializedProperties == NULL) || (serializedPropertiesLength == NULL))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if (WriteWritableResponseProperties(properties, numProperties, componentName, NULL, 0, &requiredBytes) != true)
    {
        LogError("Cannot determine required length of reported properties buffer");
        result = IOTHUB_CLIENT_ERROR;
    }
    else if ((serializedPropertiesBuffer = calloc(1, requiredBytes)) == NULL)
    {
        LogError("Cannot allocate %lu bytes", (unsigned long)requiredBytes);
        result = IOTHUB_CLIENT_ERROR;
    }
    else if (WriteWritableResponseProperties(properties, numProperties, componentName, serializedPropertiesBuffer, requiredBytes, &requiredBytes) != true)
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
        // See comments in IoTHubClient_Properties_Serializer_CreateReported for background on why we substract one.
        *serializedPropertiesLength = requiredBytes - 1;
    }
    else if (serializedPropertiesBuffer != NULL)
    {
        free(serializedPropertiesBuffer);
    }

    return result;
}


// We have an explicit destroy function for serialized properties, even though IoTHubClient_Properties_Serializer_CreateReported
// and IoTHubClient_Properties_Serializer_CreateWritableResponse used malloc() and app could've theoretically done free() directly.  Because:
// * If the SDK is setup to use a custom allocator (gballoc.h) then we use same malloc() / free() matching to free.
// * This gives us flexibility to use non-malloc based allocators in future.
// * Maintains symmetry with _Destroy() as mechanism to free resources SDK allocates.
void IoTHubClient_Properties_Serializer_Destroy(unsigned char* serializedProperties)
{
    if (serializedProperties != NULL)
    {
        free(serializedProperties);
    }
}

//
// CopyPayloadToString creates a null-terminated string from the payload buffer.
// payload is not guaranteed to be null-terminated by the IoT Hub device SDK.
//
static char* CopyPayloadToString(const unsigned char* payload, size_t size)
{
    char* jsonStr;
    size_t sizeToAllocate = size + 1;

    if ((jsonStr = (char*)calloc(1, sizeToAllocate)) == NULL)
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

// GetDesiredAndReportedTwinJson retrieves the desired and (if available) the reported sections of IoT Hub twin as JSON_Objects for later use.
// When a full twin is sent, the JSON consists of {"desired":{...}, "reported":{...}}".  If we're processing an update patch, 
// IoT Hub does not send us the "reported" and the "desired" is by convention taken to be the root of the JSON document received.
static IOTHUB_CLIENT_RESULT GetDesiredAndReportedTwinJson(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer)
{
    IOTHUB_CLIENT_RESULT result;
    JSON_Object* rootObject;

    if ((rootObject = json_value_get_object(propertiesDeserializer->rootValue)) == NULL)
    {
        LogError("Unable to get root object of JSON");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        if (payloadType == IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL)
        {
            // NULL values are NOT errors, as the JSON may legitimately not have these fields.
            propertiesDeserializer->desiredObject = json_object_get_object(rootObject, TWIN_DESIRED_OBJECT_NAME);
            propertiesDeserializer->reportedObject = json_object_get_object(rootObject, TWIN_REPORTED_OBJECT_NAME);
        }
        else
        {
            // For a patch update, IoTHub does not explicitly put a "desired:" JSON envelope.  The "desired-ness" is implicit 
            // in this case, so here we simply need the root of the JSON itself.
            propertiesDeserializer->desiredObject = rootObject;
            propertiesDeserializer->reportedObject = NULL;
        }
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// GetTwinVersion retrieves the $version field from JSON document received from IoT Hub.  
// The application needs this value when acknowledging writable properties received from the service.
static IOTHUB_CLIENT_RESULT GetTwinVersion(IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer)
{
    IOTHUB_CLIENT_RESULT result;
    JSON_Value* versionValue = NULL;

    if ((versionValue = json_object_get_value(propertiesDeserializer->desiredObject, TWIN_VERSION)) == NULL)
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
        propertiesDeserializer->propertiesVersion = (int)json_value_get_number(versionValue);
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// IsJsonObjectAComponent indicates whether the current containerObject we're visiting corresponds to
// a PnP component (indicated by a "__t":"c" field as a containerObject child) or not.
static bool IsJsonObjectAComponent(IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer, JSON_Object* containerObject)
{
    bool result;

    // First retrieve the JSON Object (assuming its an object) at the current place we're iterating.
    JSON_Value* jsonValue = json_object_get_value_at(containerObject, propertiesDeserializer->currentPropertyIndex);
    JSON_Object* jsonObject = json_value_get_object(jsonValue);

    // Examine it's object to see if the "__t:c" has been set.  If jsonObject isn't actually of type JSON_Object,
    // parson will gracefully return NULL for a string lookup.
    const char* componentMarkerValue = json_object_get_string(jsonObject, TWIN_COMPONENT_MARKER_NAME);
    result = (componentMarkerValue != NULL) && (strcmp(componentMarkerValue, TWIN_COMPONENT_MARKER_VALUE) == 0);

    return result;
}

// ValidateDeserializerInputs makes sure that the parameter list passed in IoTHubClient_Properties_Deserializer_Create is valid.
static IOTHUB_CLIENT_RESULT ValidateDeserializerInputs(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* payload,
    size_t payloadLength,
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE* propertiesDeserializerHandle)
{
    IOTHUB_CLIENT_RESULT result;

    if ((payloadType != IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL) && (payloadType != IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES))
    {
        LogError("Payload type %d is invalid", payloadType);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if ((payload == NULL) || (payloadLength == 0) || (propertiesDeserializerHandle == NULL))
    {
        LogError("NULL arguments passed");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// FillProperty retrieves properties that are children of jsonObject and puts them into properties 'property' to be returned to the application.
static IOTHUB_CLIENT_RESULT FillProperty(IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer, const char* componentName, JSON_Value* propertyValue, const char* propertyName, IOTHUB_CLIENT_PROPERTY_PARSED* property)
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
        // Note that most fields in the returned IOTHUB_CLIENT_PROPERTY_PARSED are shallow copies.  This memory remains valid until
        // the application calls IoTHubClient_Properties_Deserializer_Destroy.
        property->propertyType = (propertiesDeserializer->propertyParseState == PROPERTY_PARSE_STATE_DESIRED) ? 
                                 IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE : IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_CLIENT;
        property->componentName = componentName;
        property->name = propertyName;
        property->valueType = IOTHUB_CLIENT_PROPERTY_VALUE_STRING;
        property->value.str = propertyStringJson;
        property->valueLength = strlen(propertyStringJson);
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

// IsReservedPropertyKeyword returns true if the given objectName is part of reserved PnP/Twin metadata for properties, false otherwise.
static bool IsReservedPropertyKeyword(const char* objectName)
{
    return ((objectName == NULL) || (strcmp(objectName, TWIN_VERSION) == 0));
}

// IsReservedPropertyKeyword returns true if the given objectName is part of reserved PnP/Twin metadata for components, false otherwise.
static bool IsReservedComponentKeyword(const char* objectName)
{
    return ((objectName == NULL) || (strcmp(objectName, TWIN_COMPONENT_MARKER_NAME) == 0));
}

// GetNextComponentProperty advances through a given component's properties, returning either the next to be enumerated or that there's nothing left to traverse.
static bool GetNextComponentProperty(IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer, JSON_Object* containerObject, const char** propertyName, JSON_Value** propertyValue)
{
    JSON_Value* componentValue = json_object_get_value_at(containerObject, propertiesDeserializer->currentPropertyIndex);
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

        while (propertiesDeserializer->currentComponentIndex < numComponentChildren)
        {
            const char* objectName = json_object_get_name(componentObject, propertiesDeserializer->currentComponentIndex);
            
            if (IsReservedComponentKeyword(objectName))
            {
                // This objectName corresponds to twin/property metadata that is not passed to application
                propertiesDeserializer->currentComponentIndex++;
            }
            else
            {
                *propertyName = objectName;
                *propertyValue = json_object_get_value_at(componentObject, propertiesDeserializer->currentComponentIndex);
                propertiesDeserializer->currentComponentIndex++;
                break;
            }
        }
    }

    return (*propertyValue != NULL);
}

// GetNextPropertyToEnumerate advances through a property list, returning either the next property to be enumerated or that there's nothing left to traverse.
static bool GetNextPropertyToEnumerate(IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer, const char** componentName, const char** propertyName, JSON_Value** propertyValue)
{
    JSON_Object* containerObject;
    *propertyName = NULL;
    *propertyValue = NULL;

    if (propertiesDeserializer->propertyParseState == PROPERTY_PARSE_STATE_DESIRED)
    {
        containerObject = propertiesDeserializer->desiredObject;
    }
    else
    {
        containerObject = propertiesDeserializer->reportedObject;
    }

    size_t numChildren = json_object_get_count(containerObject);

    while (propertiesDeserializer->currentPropertyIndex < numChildren)
    {
        const char* objectName = json_object_get_name(containerObject, propertiesDeserializer->currentPropertyIndex);

        if (propertiesDeserializer->componentParseState == COMPONENT_PARSE_STATE_SUB_COMPONENT)
        {
            // The propertiesDeserializer->currentPropertyIndex corresponds to a subcomponent we're traversing.
            if (GetNextComponentProperty(propertiesDeserializer, containerObject, propertyName, propertyValue) == true)
            {
                // The component has additional properties to return to application.
                *componentName = objectName;
                break;
            }
            else
            {
                // We've parsed all the properties of this component.  Move onto the next top-level JSON element
                propertiesDeserializer->componentParseState = COMPONENT_PARSE_STATE_ROOT;
                propertiesDeserializer->currentPropertyIndex++;
                propertiesDeserializer->currentComponentIndex = 0;
                continue;
            }
        }

        if (IsJsonObjectAComponent(propertiesDeserializer, containerObject))
        {
            // This top-level property is the name of a component.  
            propertiesDeserializer->componentParseState = COMPONENT_PARSE_STATE_SUB_COMPONENT;
            continue;
        }
        else if (IsReservedPropertyKeyword(objectName))
        {
            // This objectName corresponds to twin/property metadata that is not passed to application
            propertiesDeserializer->currentPropertyIndex++;
            continue;
        }
        else
        {
            // We've found a property of the root component.
            *componentName = NULL;
            *propertyName = objectName;
            *propertyValue = json_object_get_value_at(containerObject, propertiesDeserializer->currentPropertyIndex);
            propertiesDeserializer->currentPropertyIndex++;
            break;
        }
    }

    return (*propertyValue != NULL);
}


IOTHUB_CLIENT_RESULT IoTHubClient_Properties_Deserializer_Create(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* payload,
    size_t payloadLength,
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE* propertiesDeserializerHandle)
{
    IOTHUB_CLIENT_RESULT result;
    char* jsonStr = NULL;

    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer = NULL;

    if ((result = ValidateDeserializerInputs(payloadType, payload, payloadLength, propertiesDeserializerHandle)) != IOTHUB_CLIENT_OK)
    {
        LogError("Invalid argument");
    }
    else if ((propertiesDeserializer = (IOTHUB_CLIENT_PROPERTIES_DESERIALIZER*)calloc(1, sizeof(IOTHUB_CLIENT_PROPERTIES_DESERIALIZER))) == NULL)
    {
        LogError("Cannot allocate IOTHUB_CLIENT_PROPERTIES_DESERIALIZER");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        if ((jsonStr = CopyPayloadToString(payload, payloadLength)) == NULL)
        {
            LogError("Unable to allocate twin buffer");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if ((propertiesDeserializer->rootValue = json_parse_string(jsonStr)) == NULL)
        {
            LogError("Unable to parse device twin JSON");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if ((result = GetDesiredAndReportedTwinJson(payloadType, propertiesDeserializer)) != IOTHUB_CLIENT_OK)
        {
            LogError("Cannot retrieve desired and/or reported object from JSON");
        }
        // Retrieve the twin version and cache with the propertiesDeserializer.  We do this in the enumeration creation,
        // even though IoTHubClient_Properties_Deserializer_GetVersion theoretically could've parsed this on demand,
        // because if this fails we want to fail creation of the enumerator.  A twin without version info
        // is not valid and the application will not be able to properly acknowledge writable properties in this case.
        else if ((result = GetTwinVersion(propertiesDeserializer)) != IOTHUB_CLIENT_OK)
        {
            LogError("Cannot retrieve properties version");
        }
        else
        {
            propertiesDeserializer->propertyParseState = PROPERTY_PARSE_STATE_DESIRED;
            propertiesDeserializer->componentParseState = COMPONENT_PARSE_STATE_ROOT;
            *propertiesDeserializerHandle = propertiesDeserializer;
            result = IOTHUB_CLIENT_OK;
        }
    }

    free(jsonStr);

    if (result != IOTHUB_CLIENT_OK)
    {
        IoTHubClient_Properties_Deserializer_Destroy(propertiesDeserializer);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Properties_Deserializer_GetVersion(
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE propertiesDeserializerHandle,
    int* propertiesVersion)
{
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer = (IOTHUB_CLIENT_PROPERTIES_DESERIALIZER*)propertiesDeserializerHandle;
    IOTHUB_CLIENT_RESULT result;

    if ((propertiesDeserializerHandle == NULL) || (propertiesVersion == NULL))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        *propertiesVersion = propertiesDeserializer->propertiesVersion;
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_Properties_Deserializer_GetNext(
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE propertiesDeserializerHandle,
    IOTHUB_CLIENT_PROPERTY_PARSED* property,
    bool* propertySpecified)
{
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer = (IOTHUB_CLIENT_PROPERTIES_DESERIALIZER*)propertiesDeserializerHandle;
    IOTHUB_CLIENT_RESULT result;

    JSON_Value* propertyValue = NULL;
    const char* propertyName = NULL;
    const char* componentName = NULL;

    if ((propertiesDeserializerHandle == NULL) || (property == NULL) || (propertySpecified == NULL) || (property->structVersion != IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1))
    {
        LogError("Invalid argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        if (propertiesDeserializer->propertyParseState == PROPERTY_PARSE_STATE_DESIRED)
        {
            if ((GetNextPropertyToEnumerate(propertiesDeserializer, &componentName, &propertyName, &propertyValue)) == false)
            {
                // If we can't find another desired object, then transition to start searching through reported.
                propertiesDeserializer->currentPropertyIndex = 0;
                propertiesDeserializer->propertyParseState = PROPERTY_PARSE_STATE_REPORTED;
            }
        }

        if (propertiesDeserializer->propertyParseState == PROPERTY_PARSE_STATE_REPORTED)
        {
            GetNextPropertyToEnumerate(propertiesDeserializer, &componentName, &propertyName, &propertyValue);
        }

        if (propertyValue != NULL)
        {
            // Fills in IOTHUB_CLIENT_PROPERTY_PARSED based on where we're at in the traversal.
            if ((result = FillProperty(propertiesDeserializer, componentName, propertyValue, propertyName, property)) != IOTHUB_CLIENT_OK)
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
    }

    return result;
}

void IoTHubClient_Properties_DeserializerProperty_Destroy(
    IOTHUB_CLIENT_PROPERTY_PARSED* property)
{
    if (property != NULL)
    {
        // The only field in IOTHUB_CLIENT_PROPERTY_PARSED that is allocated when filling
        // the structure is the JSON representation.  The remaining fields are shallow copies pointing
        // into the parsed JSON fields which are freed by IoTHubClient_Properties_Deserializer_Destroy.
        json_free_serialized_string((char*)property->value.str);
    }
}

void IoTHubClient_Properties_Deserializer_Destroy(IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE propertiesDeserializerHandle)
{
    if (propertiesDeserializerHandle != NULL)
    {
        IOTHUB_CLIENT_PROPERTIES_DESERIALIZER* propertiesDeserializer = (IOTHUB_CLIENT_PROPERTIES_DESERIALIZER*)propertiesDeserializerHandle;
        json_value_free(propertiesDeserializer->rootValue);
        free(propertiesDeserializer);
    }
}
