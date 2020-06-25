// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "pnp_protocol_helpers.h"
#include "parson.h"

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"

static const char PnP_PropertyWithoutResponseSchemaWithoutComponent[] = "{ \"%s\": %s }";
static const char PnP_PropertyWithoutResponseSchemaWithComponent[] = "{\"""%s\": { \"%s\": %s } }";

static const char PnP_PropertyWithResponseSchemaWithoutComponent[] =  "{ \"%s\": { \"value\":  %s, \"ac\": %d, \"ad\": \"%s\", \"av\": %d } } ";
static const char PnP_PropertyWithResponseSchemaWithComponent[] =  "{\"""%s\": { \"%s\": { \"value\":  %s, \"ac\": %d, \"ad\": \"%s\", \"av\": %d } } }";


// Returns the JSON to report via a DeviceTwin, without specifying additional metadata such as ackCode or ackVersion
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

// Returns the JSON to report via a DeviceTwin,  specifying additional metadata such as ackCode or ackVersion
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

// When deviceMethod receives an incoming request, parses out the (optional) componentName along with the command being targetted.
void PnPHelper_ParseCommandName(const char* deviceMethodName, const char** componentName, size_t* componentNameLength, const char** pnpCommandName, size_t* pnpCommandNameLength)
{
    const char* separator;

    if ((separator = strstr(deviceMethodName, "*")) == NULL) 
    {
         *componentName = NULL;
         *componentNameLength = 0;
         *pnpCommandName = deviceMethodName;
         *pnpCommandNameLength = strlen(deviceMethodName);
    }
    else 
    {
        *componentName = deviceMethodName;
        *componentNameLength = separator - deviceMethodName;
        *pnpCommandName = separator + 1;
        *pnpCommandNameLength = strlen(*pnpCommandName);
    }
}

// PnPHelper_CreateTelemetryMessageHandle will create an IOTHUB_MESSAGE_HANDLE and return to the application, specifying
// the componentName if set.
IOTHUB_MESSAGE_HANDLE PnPHelper_CreateTelemetryMessageHandle(const char* componentName, const char* telemetryData) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;
    
    if ((messageHandle = IoTHubMessage_CreateFromString(telemetryData)) == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
        result = MU_FAILURE;
    }
    else if (componentName == NULL)
    {
        // If there is no component then there's nothing else to do.
        result = 0;
    }
    // If the component will be used, then specify its property.
    else if ((iothubClientResult = IoTHubMessage_SetProperty(messageHandle, "$.sub", componentName)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubMessage_SetProperty failed, error=%d", iothubClientResult);
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

// VisitDesiredChildObject is invoked for each child element of the desired twin if and only if the element is an object.
// This function determines if the object corresponds to a component (which itself has children properties) or a
// property of the root component that has happens to be an object.
static void VisitDesiredChildObject(const char* objectName, JSON_Value* value, PnPHelperPropertyCallbackFunction callbackFromApplication)
{
    (void)callbackFromApplication;
    JSON_Object* object = json_value_get_object(value);

    // PnP indicates that a JSON object is a component, not a property of the root component, but 
    // adding a "__t"="c" field as one of its children.

    // These APIs will "fail", by design, if there is no such marker (which indicates the object
    // is owned by the root component).
    JSON_Value* componentTypeMarker = json_object_get_value(object, "__t");
    const char* componentTypeStr = json_value_get_string(componentTypeMarker);

    if ((componentTypeStr != NULL) && (strcmp(componentTypeStr, "c") == 0))
    {
        // We are processing a component.  Visit each of its child elements, which correspond to its properties.
        size_t numChildren = json_object_get_count(object);
        for (size_t i = 0; i < numChildren; i++)
        {
            const char* name = json_object_get_name(object, i);
            JSON_Value* subValue = json_object_get_value_at(object, i);

            // The "__t" marker is the metadata indicating this is a component.  Don't call back on this.
            if (strcmp(name, "__t") == 0)
                continue;

            const char* jsonStr = json_serialize_to_string(subValue);
            printf("val=%s\n", jsonStr);

            printf("componentName=%s,name=%s,val=%s\n", objectName, name, jsonStr);
        }
    }
    else
    {
        printf("NOT a subcomponent.  Treating as a property.");
        const char* jsonStr = json_serialize_to_string(value);
        printf("val=%s\n", jsonStr);
    }
    
}

// VisitDesiredObject visits each JSON child element of the desired settings from the twin.
static int VisitDesiredObject(JSON_Object* desiredObject, PnPHelperPropertyCallbackFunction callbackFromApplication)
{
    JSON_Value* versionValue = NULL;
    size_t numChildren;
    int version;
    int result;

    if ((versionValue = json_object_get_value(desiredObject, "$version")) == NULL)
    {
        LogError("Cannot retrieve '$version' field for twin.");
        result = MU_FAILURE;
    }
    else
    {
        version = (int)json_value_get_number(versionValue);

        numChildren = json_object_get_count(desiredObject);

        for (size_t i = 0; i < numChildren; i++)
        {
            const char* name = json_object_get_name(desiredObject, i);
            JSON_Value* value = json_object_get_value_at(desiredObject, i);

            if (strcmp(name,"$version") == 0)
            {
                // The version field is metadata and should be skipped in enumeration loop.
                continue;
            }

            JSON_Value_Type jsonTye = json_type(value);

            if (jsonTye != JSONObject)
            {
                // If the child element is NOT an object, then it means that this is a property of the model's root component.
                const char* jsonStr = json_serialize_to_string(value);
                printf("name=%s,val=%s\n", name, jsonStr);
            }
            else
            {
                // If the child element is an object, the processing becomes more complex.
                VisitDesiredChildObject(name, value, callbackFromApplication);
            }
        }

        result = 0;
    }

    return result;
}


// We first must make a copy of the payLoad data.  This is a simple byte string and is not guaranteed to be a NULL terminated string from transport layer,
// so in our copy we can NULL terminate.
// TODO: https://github.com/Azure/azure-iot-sdk-c/issues/1195 tracks having the HubClient itself allocate an extra byte to save us this copy.
char* CopyTwinPayloadToString(const unsigned char* payLoad, size_t size)
{
    char* jsonStr;

    if ((jsonStr = (char*)malloc(size+1)) == NULL)
    {
        LogError("Unable to allocate %lu size buffer", (unsigned long)size);
    }
    else
    {
        memcpy(jsonStr, payLoad, size);
        jsonStr[size] = 0;
    }

    return jsonStr;
}

// GetDesiredJson retrieves JSON_Object* in the JSON tree corresponding to the desired payload.
JSON_Object* GetDesiredJson(DEVICE_TWIN_UPDATE_STATE updateState, JSON_Value* rootValue)
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
            // We only care about desired in this sample, so just retrieve it.
            desiredObject = json_object_get_object(rootObject, "desired");
        }
        else
        {
            // For a patch update, IoTHub only sends "desired" portion of twin and does explicitly put a "desired:" JSON envelope.
            desiredObject = rootObject;
        }

    }

    return desiredObject;
}


int PnPHelper_ProcessTwinData(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payLoad, size_t size, PnPHelperPropertyCallbackFunction callbackFromApplication) 
{

    char* jsonStr;
    JSON_Value* rootValue = NULL;
    JSON_Object* desiredObject = NULL;
    int result;

    if ((jsonStr = CopyTwinPayloadToString(payLoad, size)) == NULL)
    {
        LogError("Unable to allocate %lu size buffer", (unsigned long)size);
        result = MU_FAILURE;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse twin JSON");
        result = MU_FAILURE;
    }
    else if ((desiredObject = GetDesiredJson(updateState, rootValue)) == NULL)
    {
        LogError("Cannot retrieve desired ");
        result = MU_FAILURE;
    }
    else
    {
        // Visit each sub-element in the desired portion of the twin JSON and invoke callbackFromApplication as appropriate.
        result = VisitDesiredObject(desiredObject, callbackFromApplication);
    }

    json_value_free(rootValue);
    free(jsonStr);

    return 0;
}

