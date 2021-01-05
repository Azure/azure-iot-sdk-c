// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Header associated with this .c file
#include "pnp_protocol.h"

// JSON parsing library
#include "parson.h"

// IoT core utility related header files
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"

// Format used when building a response for a root property that does not contain metadata
static const char g_propertyWithoutResponseSchemaWithoutComponent[] = "{\"%s\":%s}";
// Format used when building a response for a component's property that does not contain metadata
static const char g_propertyWithoutResponseSchemaWithComponent[] = "{\"""%s\":{\"__t\":\"c\",\"%s\":%s}}";
// Format used when building a response for a root property that does contain metadata
static const char g_propertyWithResponseSchemaWithoutComponent[] =  "{\"%s\":{\"value\":%s,\"ac\":%d,\"ad\":\"%s\",\"av\":%d}}";
// Format used when building a response for a component's property that does contain metadata
static const char g_propertyWithResponseSchemaWithComponent[] =  "{\"""%s\":{\"__t\":\"c\",\"%s\":{\"value\":%s,\"ac\":%d,\"ad\":\"%s\",\"av\":%d}}}";

// Character that separates a PnP component from the specific command on the component.
static const char g_commandSeparator = '*';

// The version of the desired twin is represented by the $version metadata.
static const char g_IoTHubTwinDesiredVersion[] = "$version";

// IoTHub adds a JSON field "__t":"c" into desired top-level JSON objects that represent components.  Without this marking, the object 
// is treated as a property off the root component.
static const char g_IoTHubTwinPnPComponentMarker[] = "__t";

// Name of desired JSON field when retrieving a full twin.
static const char g_IoTHubTwinDesiredObjectName[] = "desired";

// Telemetry message property used to indicate the message's component.
static const char PnP_TelemetryComponentProperty[] = "$.sub";

STRING_HANDLE PnP_CreateReportedProperty(const char* componentName, const char* propertyName, const char* propertyValue)
{
    STRING_HANDLE jsonToSend;

    if (componentName == NULL) 
    {
        jsonToSend = STRING_construct_sprintf(g_propertyWithoutResponseSchemaWithoutComponent, propertyName, propertyValue);
    }
    else 
    {
       jsonToSend = STRING_construct_sprintf(g_propertyWithoutResponseSchemaWithComponent, componentName, propertyName, propertyValue);
    }

    if (jsonToSend == NULL)
    {
        LogError("Unable to allocate JSON buffer");
    }

    return jsonToSend;
}

STRING_HANDLE PnP_CreateReportedPropertyWithStatus(const char* componentName, const char* propertyName, const char* propertyValue, int result, const char* description, int ackVersion)
{
    STRING_HANDLE jsonToSend;

    if (componentName == NULL) 
    {
        jsonToSend = STRING_construct_sprintf(g_propertyWithResponseSchemaWithoutComponent, propertyName, propertyValue, result, description, ackVersion);
    }
    else 
    {
       jsonToSend = STRING_construct_sprintf(g_propertyWithResponseSchemaWithComponent, componentName, propertyName, propertyValue, result, description, ackVersion);
    }

    if (jsonToSend == NULL)
    {
        LogError("Unable to allocate JSON buffer");
    }

    return jsonToSend;    
}

void PnP_ParseCommandName(const char* deviceMethodName, unsigned const char** componentName, size_t* componentNameSize, const char** pnpCommandName)
{
    const char* separator;

    if ((separator = strchr(deviceMethodName, g_commandSeparator)) != NULL)
    {
        // If a separator character is present in the device method name, then a command on a subcomponent of 
        // the model is being targeted (e.g. thermostat1*getMaxMinReport).
        *componentName = (unsigned const char*)deviceMethodName;
        *componentNameSize = separator - deviceMethodName;
        *pnpCommandName = separator + 1;
    }
    else 
    {
        // The separator character is optional.  If it is not present, it indicates a command of the root 
        // component and not a subcomponent (e.g. "reboot").
        *componentName = NULL;
        *componentNameSize = 0;
        *pnpCommandName = deviceMethodName;
    }
}

IOTHUB_MESSAGE_HANDLE PnP_CreateTelemetryMessageHandle(const char* componentName, const char* telemetryData) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    IOTHUB_MESSAGE_RESULT iothubMessageResult;
    bool result;
    
    if ((messageHandle = IoTHubMessage_CreateFromString(telemetryData)) == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
        result = false;
    }
    // If the component will be used, then specify this as a property of the message.
    else if ((componentName != NULL) && (iothubMessageResult = IoTHubMessage_SetProperty(messageHandle, PnP_TelemetryComponentProperty, componentName)) != IOTHUB_MESSAGE_OK)
    {
        LogError("IoTHubMessage_SetProperty=%s failed, error=%d", PnP_TelemetryComponentProperty, iothubMessageResult);
        result = false;
    }
    else
    {
        result = true;
    }

    if ((result == false) && (messageHandle != NULL))
    {
        IoTHubMessage_Destroy(messageHandle);
        messageHandle = NULL;
    }
    
    return messageHandle;
}

//
// VisitComponentProperties visits each sub element of the the given objectName in the desired JSON.  Each of these sub elements corresponds to
// a property of this component, which we'll invoke the application's pnpPropertyCallback to inform.
// 
static void VisitComponentProperties(const char* objectName, JSON_Value* value, int version, PnP_PropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
{
    JSON_Object* object = json_value_get_object(value);
    size_t numChildren = json_object_get_count(object);

    for (size_t i = 0; i < numChildren; i++)
    {
        const char* propertyName = json_object_get_name(object, i);
        JSON_Value* propertyValue = json_object_get_value_at(object, i);

        if ((propertyName == NULL) || (propertyValue == NULL))
        {
            // This should never happen because we are simply accessing parson tree.  Do not pass NULL to application in case it does occur.
            LogError("Unexpected error retrieving the property name and/or value of component=%s at element at index=%lu", objectName, (unsigned long)i);
            continue;
        }

        // When a component is received from a full twin, it will have a "__t" as one of the child elements.  This is metadata that indicates
        // to solutions that the JSON object corresponds to a component and not a property of the root component.  Because this is 
        // metadata and not part of this component's modeled properties, we ignore it when processing this loop.
        if (strcmp(propertyName, g_IoTHubTwinPnPComponentMarker) == 0)
        {
            continue;
        }

        // Invoke the application's passed in callback for it to process this property.
        pnpPropertyCallback(objectName, propertyName, propertyValue, version, userContextCallback);
    }
}

//
// IsJsonObjectAComponentInModel checks whether the objectName, read from the top-level child of the desired device twin JSON, 
// is in componentsInModel that the application passed into us.
//
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

//
// VisitDesiredObject visits each child JSON element of the desired device twin.  As we parse each property out, we invoke the application's passed in pnpPropertyCallback.
//
static bool VisitDesiredObject(JSON_Object* desiredObject, const char** componentsInModel, size_t numComponentsInModel, PnP_PropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
{
    JSON_Value* versionValue = NULL;
    size_t numChildren;
    int version;
    bool result;

    if ((versionValue = json_object_get_value(desiredObject, g_IoTHubTwinDesiredVersion)) == NULL)
    {
        LogError("Cannot retrieve %s field for twin", g_IoTHubTwinDesiredVersion);
        result = false;
    }
    else if (json_value_get_type(versionValue) != JSONNumber)
    {
        LogError("JSON field %s is not a number", g_IoTHubTwinDesiredVersion);
        result = false;
    }
    else
    {
        version = (int)json_value_get_number(versionValue);

        numChildren = json_object_get_count(desiredObject);

        // Visit each child JSON element of the desired device twin.
        for (size_t i = 0; i < numChildren; i++)
        {
            const char* name = json_object_get_name(desiredObject, i);
            JSON_Value* value = json_object_get_value_at(desiredObject, i);

            if (strcmp(name, g_IoTHubTwinDesiredVersion) == 0)
            {
                // The version field is metadata and should be ignored in this loop.
                continue;
            }

            if ((json_type(value) == JSONObject) && IsJsonObjectAComponentInModel(name, componentsInModel, numComponentsInModel))
            {
                // If this current JSON is an element AND the name is one of the componentsInModel that the application knows about,
                // then this json element represents a component.
                VisitComponentProperties(name, value, version, pnpPropertyCallback, userContextCallback);
            }
            else
            {
                // If the child element is NOT an object OR its not a model the application knows about, this is a property of the model's root component.
                // Invoke the application's passed in callback for it to process this property.
                pnpPropertyCallback(NULL, name, value, version, userContextCallback);
            }
        }

        result = true;
    }

    return result;
}

//
// GetDesiredJson retrieves JSON_Object* in the JSON tree corresponding to the desired payload.
//
static JSON_Object* GetDesiredJson(DEVICE_TWIN_UPDATE_STATE updateState, JSON_Value* rootValue)
{
    JSON_Object* rootObject = NULL;
    JSON_Object* desiredObject;

    if ((rootObject = json_value_get_object(rootValue)) == NULL)
    {
        LogError("Unable to get root object of JSON");
        desiredObject = NULL;
    }
    else
    {
        if (updateState == DEVICE_TWIN_UPDATE_COMPLETE)
        {
            // For a complete update, the JSON from IoTHub will contain both "desired" and "reported" - the full twin.
            // We only care about "desired" in this sample, so just retrieve it.
            desiredObject = json_object_get_object(rootObject, g_IoTHubTwinDesiredObjectName);
        }
        else
        {
            // For a patch update, IoTHub does not explicitly put a "desired:" JSON envelope.  The "desired-ness" is implicit 
            // in this case, so here we simply need the root of the JSON itself.
            desiredObject = rootObject;
        }
    }

    return desiredObject;
}

bool PnP_ProcessTwinData(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, const char** componentsInModel, size_t numComponentsInModel, PnP_PropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
{
    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* desiredObject;
    bool result;

    if ((jsonStr = PnP_CopyPayloadToString(payload, size)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
        result = false;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse device twin JSON");
        result = false;
    }
    else if ((desiredObject = GetDesiredJson(updateState, rootValue)) == NULL)
    {
        LogError("Cannot retrieve desired JSON object");
        result = false;
    }
    else
    {
        // Visit each sub-element in the desired portion of the twin JSON and invoke pnpPropertyCallback as appropriate.
        result = VisitDesiredObject(desiredObject, componentsInModel, numComponentsInModel, pnpPropertyCallback, userContextCallback);
    }

    json_value_free(rootValue);
    free(jsonStr);

    return result;
}

char* PnP_CopyPayloadToString(const unsigned char* payload, size_t size)
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
