// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample implements a PnP based Temperature Control model.  It demonstrates a somewhat
// complex model in that the model has properties, commands, and telemetry off of the root component
// as well as subcomponents.  

// PnP is a serialization and deserialization convention built on top of IoTHub DeviceClient
// primitives of telemetry, device methods, and device twins.  A sample helper, ../common/pnp_protocol_helpers.h,
// is used here to perfom the needed (de)serialization.

// TODO: Link to final GitHub URL of the DTDL once checked in.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"

#include "pnp_device_client_helpers.h"
#include "pnp_protocol_helpers.h"

// Paste in your device connection string
static const char* g_connectionString = "[device connection string]";

// Amount of time to sleep between sending telemetry to Hub, in milliseconds.  Set to 1 minute.
static unsigned int g_sleepBetweenTelemetrySends = 60 * 1000;

// Whether tracing at the IoTHub client is enabled or not. 
static bool g_hubClientTraceEnabled = true;

// DTMI indicating this device's ModelId.
static const char g_ModelId[] = "dtmi:com:example:simplethermostat;1";

// This is a convention in this program *only* for a friendly way to print out the root component via logging
static const char g_RootComponentName[] = "root component";

// Global device handle.
IOTHUB_DEVICE_CLIENT_HANDLE g_deviceHandle;


//
// TempControl_DeviceMethodCallback is invoked by IoT SDK when a device method arrives.
//
static int TempControl_DeviceMethodCallback(const char* methodName, const unsigned char* payload, size_t size, unsigned char** response, size_t* responseSize, void* userContextCallback)
{
    (void)userContextCallback;
    (void)payload;
    (void)response;
    (void)responseSize;
    (void)size;

    const char *commandName;
    const char *componentName;
    size_t commandNameLength;
    size_t componentNameLength;

    int result = 200;

    // Parse the methodName into its PnP (optional) componentName and commandName.
    PnPHelper_ParseCommandName(methodName, &componentName, &componentNameLength, &commandName, &commandNameLength);

    if (componentName != NULL)
    {
        LogInfo("Received PnP command for component=%.*s, command=%.*s", (int)componentNameLength, componentName, (int)commandNameLength, commandName);
    }
    else
    {
        LogInfo("Received PnP command for root component, command=%.*s", (int)commandNameLength, commandName);
    }

    return result;
}

//
// GetComponentNameForLogging is a helper that returns either the component name or a friendly name for a NULL componentName
//
static const char * GetComponentNameForLogging(const char *componentName)
{
    return (componentName == NULL) ? g_RootComponentName  : componentName;
}

//
// TempControl_ApplicationPropertyCallback is the callback function that the PnP helper layer invokes per property update.
//
static void TempControl_ApplicationPropertyCallback(const char* componentName, const char* propertyName, JSON_Value* propertyValue, int version)
{
    char* propertyValueStr = NULL;
    STRING_HANDLE jsonToSend = NULL;
    IOTHUB_CLIENT_RESULT iothubClientResult;

    if ((propertyValueStr = json_serialize_to_string(propertyValue)) == NULL)
    {
        LogError("json_serialize_to_string failed.  Unable to process component %s, property %s further", GetComponentNameForLogging(componentName), propertyName);
    }
    else
    {
        LogInfo("Received property update for component=%s, propert=%s, desired value=%s, version=%d", GetComponentNameForLogging(componentName), propertyName, propertyValueStr, version);

        // Now acknowledge the property has been received.  First build up the serialized buffer, using the helper to construct the formatted JSON.
        if ((jsonToSend = PnPHelper_CreateReportedPropertyWithStatus(componentName, propertyName, propertyValueStr, 200, "", version)) == NULL)
        {
            LogError("Unable to build reported property response");
        }
        else
        {
            const char* jsonToSendStr = STRING_c_str(jsonToSend);
            size_t jsonToSendStrLen = strlen(jsonToSendStr);

            if ((iothubClientResult = IoTHubDeviceClient_SendReportedState(g_deviceHandle, (const unsigned char*)jsonToSendStr, jsonToSendStrLen, NULL, NULL)) != IOTHUB_CLIENT_OK)
            {
                LogError("Unable to send reported state.  Error=%d", iothubClientResult);
            }
            else
            {
                LogInfo("Sending acknowledgement of property to IoTHub");
            }
        }
    }

    STRING_delete(jsonToSend);
    free(propertyValueStr);
}

//
// TempControl_DeviceTwinCallback is invoked by IoT SDK when a twin - either full twin or a PATCH update - arrives.
//
static void TempControl_DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, void* userContextCallback)
{
    (void)userContextCallback;

    // Invoke PnPHelper_ProcessTwinData to actualy process the data.  PnPHelper_ProcessTwinData uses a visitor pattern to parse
    // the JSON and then visit each property, invoking TempControl_ApplicationPropertyCallback on each element.
    if (PnPHelper_ProcessTwinData(updateState, payload, size, TempControl_ApplicationPropertyCallback) == false)
    {
        // If we're unable to parse the JSON for any reason (typically because the JSON is malformed or we ran out of memory)
        // there is no actiol we can take beyond logging.
        LogError("Unable to process twin json.  Ignoring any desired property update requests");
    }
}

//
// TempControl_SendCurrentTemperature sends a PnP telemetry indicating the current temperature
//
void TempControl_SendCurrentTemperature(void) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;

    if ((messageHandle = PnPHelper_CreateTelemetryMessageHandle("thermostat", "22")) == NULL)
    {
        LogError("Unable to create telemetry message");
    }
    else if ((iothubResult = IoTHubDeviceClient_SendEventAsync(g_deviceHandle, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send telemetry message, error=%d", iothubResult);
    }

    IoTHubMessage_Destroy(messageHandle);
}

int main(void)
{
    g_deviceHandle = PnPHelper_CreateDeviceClientHandle(g_connectionString, g_ModelId, g_hubClientTraceEnabled, TempControl_DeviceMethodCallback, TempControl_DeviceTwinCallback);

    if (g_deviceHandle == NULL)
    {
        LogError("Failure creating Iothub device");
    }
    else
    {
        LogInfo("Successfully created device client handle.  Hit Control-C to exit program\n");
        // TODO: Add a sample invoking PnPHelper_CreateReportedProperty() with a simlpe "readonly" property.

        while (true)
        {
            // Wake up periodically to send telemetry.
            // IOTHUB_DEVICE_CLIENT_HANDLE brings in the IoTHub device convenience layer, which means that the IoTHub SDK itself 
            // spins a worker thread to perform all operations.
            TempControl_SendCurrentTemperature();
            ThreadAPI_Sleep(g_sleepBetweenTelemetrySends);
        }

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_Destroy(g_deviceHandle);
        // Free all the sdk subsystem
        IoTHub_Deinit();        
    }

    return 0;
}

