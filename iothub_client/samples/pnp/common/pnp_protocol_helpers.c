// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pnp_protocol_helpers.h"
#include "parson.h"

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"

// Format used when building a response for a root property that does not contain metadata
static const char PnP_PropertyWithoutResponseSchemaWithoutComponent[] = "{ \"%s\": %s }";
// Format used when building a response for a component's property that does not contain metadata
static const char PnP_PropertyWithoutResponseSchemaWithComponent[] = "{\"""%s\": { \"__t\":\"c\", \"%s\": %s } }";
// Format used when building a response for a root property that does contain metadata
static const char PnP_PropertyWithResponseSchemaWithoutComponent[] =  "{ \"%s\": { \"value\":  %s, \"ac\": %d, \"ad\": \"%s\", \"av\": %d } } ";
// Format used when building a response for a component's property that does contain metadata
static const char PnP_PropertyWithResponseSchemaWithComponent[] =  "{\"""%s\": { \"__t\":\"c\", \"%s\": { \"value\":  %s, \"ac\": %d, \"ad\": \"%s\", \"av\": %d } } }";

// Character that separates a PnP component's from the specific command on the component.
static const char PnP_CommandSeparator = '*';

// The version of the desired twin is represented by the $version metadata.
static const char PnP_JsonPropertyVersion[] = "$version";

// If a child element of PnP is a component, then there is an extra child { ... "__t":"c" ...} indicating this to our parser.
static const char PnP_PropertyComponentJsonName[] = "__t";
static const char PnP_PropertyComponentJsonValue[] = "c";

// When sending PnP telemetry and using a component, we need to add a property to the message 
// (PnP_TelemetryComponentPropertyName) indicating the component's name.
static const char PnP_TelemetryComponentProperty[] = "$.sub";

STRING_HANDLE PnPHelper_CreateReportedProperty(const char* componentName, const char* propertyName, const char* propertyValue)
{
    STRING_HANDLE jsonToSend;

    if (componentName == NULL) 
    {
        jsonToSend = STRING_construct_sprintf(PnP_PropertyWithoutResponseSchemaWithoutComponent, propertyName, propertyValue);
    }
    else 
    {
       jsonToSend = STRING_construct_sprintf(PnP_PropertyWithoutResponseSchemaWithComponent, componentName, propertyName, propertyValue);
    }

    if (jsonToSend == NULL)
    {
        LogError("Unable to allocate JSON buffer to send");
    }

    return jsonToSend;
}

STRING_HANDLE PnPHelper_CreateReportedPropertyWithStatus(const char* componentName, const char* propertyName, const char* propertyValue, int result, const char* description, int ackVersion)
{
    STRING_HANDLE jsonToSend;

    if (componentName == NULL) 
    {
        jsonToSend = STRING_construct_sprintf(PnP_PropertyWithResponseSchemaWithoutComponent, propertyName, propertyValue, result, description, ackVersion);
    }
    else 
    {
       jsonToSend = STRING_construct_sprintf(PnP_PropertyWithResponseSchemaWithComponent, componentName, propertyName, propertyValue, result, description, ackVersion);
    }

    if (jsonToSend == NULL)
    {
        LogError("Unable to allocate JSON buffer to send");
    }

    return jsonToSend;    
}

void PnPHelper_ParseCommandName(const char* deviceMethodName, const char** componentName, size_t* componentNameLength, const char** pnpCommandName, size_t* pnpCommandNameLength)
{
    const char* separator;

    if ((separator = strchr(deviceMethodName, PnP_CommandSeparator)) != NULL)
    {
        // If a separator character wis present in the device method name, then a command on a subcomponent of 
        // the model is being targeted.  (E.G. Thermostat1.Run).
        *componentName = deviceMethodName;
        *componentNameLength = separator - deviceMethodName;
        *pnpCommandName = separator + 1;
        *pnpCommandNameLength = strlen(*pnpCommandName);
    }
    else 
    {
        // The separator character is optional.  If it is not present, it indicates a command of the root 
        // component and not a subcomponent is being targeted.  (E.G. simply Run.)
        *componentName = NULL;
        *componentNameLength = 0;
        *pnpCommandName = deviceMethodName;
        *pnpCommandNameLength = strlen(deviceMethodName);
    }
}

IOTHUB_MESSAGE_HANDLE PnPHelper_CreateTelemetryMessageHandle(const char* componentName, const char* telemetryData) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    IOTHUB_MESSAGE_RESULT iothubMessageResult;
    int result;
    
    if ((messageHandle = IoTHubMessage_CreateFromString(telemetryData)) == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
        result = MU_FAILURE;
    }
    // If the component will be used, then specify this as a property of the message.
    else if ((componentName != NULL) && (iothubMessageResult = IoTHubMessage_SetProperty(messageHandle, PnP_TelemetryComponentProperty, componentName)) != IOTHUB_MESSAGE_OK)
    {
        LogError("IoTHubMessage_SetProperty(%s) failed, error=%d", PnP_TelemetryComponentProperty, iothubMessageResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    if ((result != 0) && (messageHandle != NULL))
    {
        IoTHubMessage_Destroy(messageHandle);
        messageHandle = NULL;
    }
    
    return messageHandle;
}

//
// VisitDesiredChildObject is invoked for each child element of the desired twin if and only if the element is an object.
// This function determines if the object corresponds to a component (which itself has children properties) or a
// property of the root component that has happens to be an object.
//
static void VisitDesiredChildObject(const char* objectName, JSON_Value* value, int version, PnPHelperPropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
{
    JSON_Object* object = json_value_get_object(value);

    // Determine whether we're processing the twin for a component or not, based on whether the component metadata tag
    // is a child element of the JSON or not.  These calls will return NULL, by design, for non-components.
    JSON_Value* componentTypeMarker = json_object_get_value(object, PnP_PropertyComponentJsonName);
    const char* componentTypeStr = json_value_get_string(componentTypeMarker);

    if ((componentTypeStr != NULL) && (strcmp(componentTypeStr, PnP_PropertyComponentJsonValue) == 0))
    {
        // We are processing a component.  Visit each of its child elements, which correspond to its properties, and invoke the 
        // application's property callback.
        size_t numChildren = json_object_get_count(object);
        for (size_t i = 0; i < numChildren; i++)
        {
            const char* propertyName = json_object_get_name(object, i);
            JSON_Value* propertyValue = json_object_get_value_at(object, i);

            if ((propertyName == NULL) || (propertyValue == NULL))
            {
                // This should never happen because we are simply accessing parson tree.  Do not pass NULL to application in case it does occur.
                LogError("Unexpected error retrieving the property name and/or value of component %s at element %lu", objectName, (unsigned long)i);
                continue;
            }

            // The PnP_PropertyComponentJsonName marker is the metadata indicating this is a component.  Don't call the application's callback.
            if (strcmp(propertyName, PnP_PropertyComponentJsonName) == 0)
            {
                continue;
            }

            pnpPropertyCallback(objectName, propertyName, propertyValue, version, userContextCallback);
        }
    }
    else
    {
        // Because there is no component marker, it means that this JSON object is a property of the root component.
        // Simply invoke the application's property callback directly.
        pnpPropertyCallback(NULL, objectName, value, version, userContextCallback);
    }
}

//
// VisitDesiredObject visits each child JSON element of the desired device twin for ultimate callback into the application.
//
static bool VisitDesiredObject(JSON_Object* desiredObject, PnPHelperPropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
{
    JSON_Value* versionValue = NULL;
    size_t numChildren;
    int version;
    bool result;

    if ((versionValue = json_object_get_value(desiredObject, PnP_JsonPropertyVersion)) == NULL)
    {
        LogError("Cannot retrieve %s field for twin", PnP_JsonPropertyVersion);
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

            if (strcmp(name, PnP_JsonPropertyVersion) == 0)
            {
                // The version field is metadata and should be skipped in enumeration loop.
                continue;
            }

            JSON_Value_Type jsonType = json_type(value);
            if (jsonType != JSONObject)
            {
                // If the child element is NOT an object, then it means that this is a property of the model's root component.
                pnpPropertyCallback(NULL, name, value, version, userContextCallback);
            }
            else
            {
                // If the child element is an object, the processing becomes more complex.
                VisitDesiredChildObject(name, value, version, pnpPropertyCallback, userContextCallback);
            }
        }

        result = true;
    }

    return result;
}

char* PnPHelper_CopyPayloadToString(const unsigned char* payload, size_t size)
{
    char* jsonStr;

    if ((jsonStr = (char*)malloc(size+1)) == NULL)
    {
        LogError("Unable to allocate %lu size buffer", (unsigned long)size);
    }
    else
    {
        memcpy(jsonStr, payload, size);
        jsonStr[size] = 0;
    }

    return jsonStr;
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
            desiredObject = json_object_get_object(rootObject, "desired");
        }
        else
        {
            // For a patch update, IoTHub only sends "desired" portion of twin and does not explicitly put a "desired:" JSON envelope.
            // So here we simply need the root of the JSON itself.
            desiredObject = rootObject;
        }

    }

    return desiredObject;
}

bool PnPHelper_ProcessTwinData(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, PnPHelperPropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
{
    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* desiredObject;
    bool result;

    if ((jsonStr = PnPHelper_CopyPayloadToString(payload, size)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
        result = false;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse twin JSON");
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
        result = VisitDesiredObject(desiredObject, pnpPropertyCallback, userContextCallback);
    }

    json_value_free(rootValue);
    free(jsonStr);

    return result;
}
