// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "pnp_protocol_helpers.h"

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

void PnPHelper_ProcessTwinData(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payLoad, size_t size, PnPHelperPropertyCallbackFunction callbackFromApplication) 
{
    (void)updateState;
    (void)payLoad;
    (void)size;
    (void)callbackFromApplication;

/*
    JsonObject obj = JsonParser(deviceTwin::desired);

    foreach (topLevel Json element in obj) {
        // Parse each top-level json element
        if (element."__t" == "c") 
        {
           // If it is a component, get the component name
           copmonentName = elements-name;
           // Components may have properties associated with them, so visit each and callback to passed function.
           foreach (subElement in element) 
           {
              propertyName = subElement's-name;

              // Invoke the application's callback 
              callbackFromApplication(componentName, propertyName, propertyName.body);
           }
        }
        else
        {
            // This top-level field isn't a component, so treat it as a property of the Model itself (aka root node)
            componentName = NULL;
            propertyName = elements-name;


             callbackFromApplication(componentName, propertyName, propertyName.body);
        }
    }
*/
}

