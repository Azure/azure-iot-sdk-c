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

    return jsonToSend;
}

// Returns the JSON to report via a DeviceTwin,  specifying additional metadata such as ackCode or ackVersion
STRING_HANDLE PnPHelper_CreateReportedPropertyWithStatus(const char* componentName, const char* propertyName, const char* propertyValue, int ackCode, const char* description, int ackVersion)
{
    STRING_HANDLE jsonToSend;

    if (componentName == NULL) 
    {
        jsonToSend = STRING_construct_sprintf(PnP_PropertyWithResponseSchemaWithoutComponent, propertyName, propertyValue, ackCode, description, ackVersion);
    }
    else 
    {
       jsonToSend = STRING_construct_sprintf(PnP_PropertyWithResponseSchemaWithComponent, componentName, propertyName, propertyValue, ackCode, description, ackVersion);
    }

    return jsonToSend;    
}

// When deviceMethod receives an incoming request, parses out the (optional) componentName along with the command being targetted.
void PnPHelper_ParseCommandName(const char* deviceMethodName, const char** componentName, size_t* componentNameLength, const char** commandName, size_t* commandNameLength)
{
    const char* separator;
    if ((separator = strstr(deviceMethodName, "*")) == NULL) {
         *componentName = NULL;
         *componentNameLength = 0;
         *commandName = deviceMethodName;
         *commandNameLength = strlen(deviceMethodName);
    }
    else {  
        *componentName = deviceMethodName;
        *componentNameLength = separator - deviceMethodName;
        *commandName = separator + 1;
        *commandNameLength = strlen(*commandName);
    }
}

IOTHUB_MESSAGE_HANDLE PnPHelper_CreateTelemetryMessageHandle(const char* componentName, const char* telemetryData) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(telemetryData);
    if (componentName != NULL)
    {
        IoTHubMessage_SetProperty(messageHandle, "$.sub", componentName);
    }
    return messageHandle;
}

static void VisitPnPObject(const char* objectName, JSON_Value* value, PnPHelperPropertyCallbackFunction callbackFromApplication)
{
    (void)callbackFromApplication;
    JSON_Object* object = json_value_get_object(value);

    JSON_Value* componentTypeMarker = json_object_get_value(object, "__t");

    const char* componentTypeStr = json_value_get_string(componentTypeMarker);

    if ((componentTypeStr != NULL) && (strcmp(componentTypeStr, "c") == 0))
    {
        size_t numChildren = json_object_get_count(object);
        for (size_t i = 0; i < numChildren; i++)
        {
            const char* name = json_object_get_name(object, i);
            JSON_Value* subValue = json_object_get_value_at(object, i);

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

static void VisitPnPProperties(JSON_Object* desiredRoot, PnPHelperPropertyCallbackFunction callbackFromApplication)
{
    size_t numChildren = json_object_get_count(desiredRoot);

    JSON_Value* versionValue = json_object_get_value(desiredRoot, "$version");
    int version = (int)json_value_get_number(versionValue);
    printf("Version=%d", version);

    for (size_t i = 0; i < numChildren; i++)
    {
        const char* name = json_object_get_name(desiredRoot, i);
        JSON_Value* value = json_object_get_value_at(desiredRoot, i);

        if (strcmp(name,"$version") == 0)
        {
            // The version field is metadata and should be skipped in enumeration loop.
            continue;
        }

        JSON_Value_Type jsonTye = json_type(value);

        if (jsonTye == JSONObject)
        {
            VisitPnPObject(name, value, callbackFromApplication);
        }
        else
        {
            const char* jsonStr = json_serialize_to_string(value);
            printf("name=%s,val=%s\n", name, jsonStr);
        }
    }

}

int PnPHelper_ProcessTwinData(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payLoad, size_t size, PnPHelperPropertyCallbackFunction callbackFromApplication) 
{
    char* jsonStr = (char*)malloc(size+1);
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Object* desiredRoot = NULL;

    memcpy(jsonStr, payLoad, size);
    jsonStr[size] = 0;

    (void)callbackFromApplication;

    rootValue = json_parse_string(jsonStr);

    rootObject = json_value_get_object(rootValue);

    if (updateState == DEVICE_TWIN_UPDATE_COMPLETE)
    {
        desiredRoot = json_object_get_object(rootObject, "desired");
    }
    else
    {
        desiredRoot = rootObject;
    }

    VisitPnPProperties(desiredRoot, callbackFromApplication);

    json_value_free(rootValue);
    free(jsonStr);

    return 0;
}

