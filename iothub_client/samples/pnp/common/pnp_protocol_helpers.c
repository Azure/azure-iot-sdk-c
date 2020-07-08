// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pnp_protocol_helpers.h"
#include "parson.h"

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"

// Format used when building a response for a root property that does not contain metadata
static const char g_propertyWithoutResponseSchemaWithoutComponent[] = "{ \"%s\": %s }";
// Format used when building a response for a component's property that does not contain metadata
static const char g_propertyWithoutResponseSchemaWithComponent[] = "{\"""%s\": { \"__t\":\"c\", \"%s\": %s } }";
// Format used when building a response for a root property that does contain metadata
static const char g_propertyWithResponseSchemaWithoutComponent[] =  "{ \"%s\": { \"value\":  %s, \"ac\": %d, \"ad\": \"%s\", \"av\": %d } } ";
// Format used when building a response for a component's property that does contain metadata
static const char g_propertyWithResponseSchemaWithComponent[] =  "{\"""%s\": { \"__t\":\"c\", \"%s\": { \"value\":  %s, \"ac\": %d, \"ad\": \"%s\", \"av\": %d } } }";

// Character that separates a PnP component's from the specific command on the component.
static const char g_commandSeparator = '*';

// The version of the desired twin is represented by the $version metadata.
static const char g_JSONPropertyVersion[] = "$version";

// IoTHub adds a JSON field "__t":"c" into desired top-level JSON objects that represent components.  Without this marking, the object 
// is treated as a property off the root component.
static const char g_JSONComponentMetadata[] = "__t";

static const char g_JSONDesiredName[] = "desired";

// When sending PnP telemetry and using a component, we need to add a property to the message 
// (PnP_TelemetryComponentPropertyName) indicating the component's name.
static const char PnP_TelemetryComponentProperty[] = "$.sub";

STRING_HANDLE PnPHelper_CreateReportedProperty(const char* componentName, const char* propertyName, const char* propertyValue)
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
        LogError("Unable to allocate JSON buffer to send");
    }

    return jsonToSend;
}

STRING_HANDLE PnPHelper_CreateReportedPropertyWithStatus(const char* componentName, const char* propertyName, const char* propertyValue, int result, const char* description, int ackVersion)
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
        LogError("Unable to allocate JSON buffer to send");
    }

    return jsonToSend;    
}

void PnPHelper_ParseCommandName(const char* deviceMethodName, unsigned const char** componentName, size_t* componentNameSize, const char** pnpCommandName)
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
        // component and not a subcomponent is being targeted (e.g. "reboot").
        *componentName = NULL;
        *componentNameSize = 0;
        *pnpCommandName = deviceMethodName;
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
// VisitComponentProperties visits each sub element of the the given objectName in the desired JSON.  Each of these elements corresponds to
// a property of this component, which we'll invoke the application's pnpPropertyCallback to inform.
// 
static void VisitComponentProperties(const char* objectName, JSON_Value* value, int version, PnPHelperPropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
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
            LogError("Unexpected error retrieving the property name and/or value of component %s at element %lu", objectName, (unsigned long)i);
            continue;
        }

        // The g_JSONComponentMetadata marker is the metadata indicating this is a component the device gets on
        // when its gets the full device twin.  Don't call the application's callback for metadata.
        if (strcmp(propertyName, g_JSONComponentMetadata) == 0)
        {
            continue;
        }

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
static bool VisitDesiredObject(JSON_Object* desiredObject, const char** componentsInModel, size_t numComponentsInModel, PnPHelperPropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
{
    JSON_Value* versionValue = NULL;
    size_t numChildren;
    int version;
    bool result;

    if ((versionValue = json_object_get_value(desiredObject, g_JSONPropertyVersion)) == NULL)
    {
        LogError("Cannot retrieve %s field for twin", g_JSONPropertyVersion);
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

            if (strcmp(name, g_JSONPropertyVersion) == 0)
            {
                // The version field is metadata and should be skipped in enumeration loop.
                continue;
            }

            if ((json_type(value) == JSONObject) && IsJsonObjectAComponentInModel(name, componentsInModel, numComponentsInModel))
            {
                // If this current JSON is an element and the name is one of the componentsInModel that the application knows about,
                // then this is a component that will need additional processing.
                VisitComponentProperties(name, value, version, pnpPropertyCallback, userContextCallback);
            }
            else
            {
                // If the child element is NOT an object OR its not a model the application knows about, this is a property of the model's root component.
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
            desiredObject = json_object_get_object(rootObject, g_JSONDesiredName);
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

bool PnPHelper_ProcessTwinData(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, const char** componentsInModel, size_t numComponentsInModel, PnPHelperPropertyCallbackFunction pnpPropertyCallback, void* userContextCallback)
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

