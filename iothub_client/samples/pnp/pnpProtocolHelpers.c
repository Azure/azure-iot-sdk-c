
// This is prototype for function that application writes to process property changes.
typedef PropertyCallbackFunction(const char* componentName, const char* propertyName, const char* propertyValue);



// Returns the JSON to report via a DeviceTwin, without specifying additional metadata such as ackCode or ackVersion
char* PnPHelper_CreateReportedProperty(const char* componentName, const char* propertyName, const char* propertyValue)
{
    if (componentName != NULL) {
        "
            "{componentName}" : {
               "__t": "c",
               "{propertyName}" : {propertyValue}
            }
        ";
    }
    else {
       return "{propertyName}: {propertyValue}";

    }
}

// Returns the JSON to report via a DeviceTwin,  specifying additional metadata such as ackCode or ackVersion
char* PnPHelper_CreateReportedPropertyWithStatus(const char* componentName, const char* propertyName, const char* propertyValue, int ackCode, int ackVersion)
{
    if (componentName != NULL) {
        return 
        "
            "{componentName}" : {
               "__t": "c",
               "{propertyName}" : {
                  "value" : {propertyValue},
                  "ac": {ackCode},
                  "ackVersion": {ackVersion}
               }
            }
        ";
    }
    else {
        return 
        "
           "{propertyName}" : {
              "value" : {propertyValue},
              "ac": {ackCode},
              "ackVersion": {ackVersion}
           }
        ";
    }
}

// When deviceMethod receives an incoming request, parses out the (optional) componentName along with the command being targetted.
void PnPHelper_ParseCommandName(const char* deviceMethodName, char** componentName, char** commandName)
{
    const char* separator;
    if ((separator = strstr(deviceMethodName, "*")) == NULL) {
         *componentName = NULL;
         *commandName = deviceMethodName;
    }
    else {  
        *separator = '\0'; // I don't think we can actually do this in even sample code but for now...
        *componentName = deviceMethodName;
        *commandName = separator + 1;
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

void PnPHelper_ProcessTwinData(const char* deviceTwin, PropertyCallbackFunction callbackFromApplication) 
{
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
              // The value of property can either be specified in "value" or else directly
              if (propertyName.value != null)
                propertyValue = propertyName.value.body;
              else
                propertyValue = propertyName.body;

              // Invoke the application's callback 
              callbackFromApplication(componentName, propertyName, propertyValue);
           }
        }
        else
        {
            // This top-level field isn't a component, so treat it as a property of the Model itself (aka root node)
            componentName = NULL;
            propertyName = elements-name;

            if (element.value != null)
              propertyValue = element.value.body;
            else
              propertyValue = element.body;

             callbackFromApplication(componentName, propertyName, propertyValue);
        }
    }
}


