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

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

// Paste in your device connection string
static const char* g_connectionString = "[device connection string]";

// Amount of time to sleep between sending telemetry to Hub, in milliseconds.  Set to 1 minute.
static unsigned int g_sleepBetweenTelemetrySends = 60 * 1000;

// Whether tracing is enabled or not
static bool g_traceOn = true;

// DTMI indicating this device's ModelId.
static const char g_ModelId[] = "dtmi:com:example:TemperatureController;1";

static void send_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    (void)printf("Confirmation callback received result %s\r\n", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

// TempControl_DeviceMethodCallback is invoked by IoT SDK when a device method arrives.
static int TempControl_DeviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
    (void)userContextCallback;
    (void)payload;
    (void)response;
    (void)resp_size;
    (void)size;

    char *commandName;
    char *componentName;
    size_t commandNameLength;
    size_t componentNameLength;

    int result = 200;

    // Parse the method_name into its PnP (optional) componentName and commandName.
    PnPHelper_ParseCommandName(method_name, &componentName, &componentNameLength, &commandName, &commandNameLength);

    // Now that we've parsed the component/command, we route to appropriate handler
    if (componentName == NULL) 
    {
        // Process by no-telemetry logic(commandName)
    }
    else if (strcmp(componentName, "thermostat")) 
    {
        // look for 'commandName' and send to appropriate handler.
    }
    else 
    {

    }
    //...
    return result;
}

// TempControl_ApplicationPropertyCallback implements a visitor pattern, where each property in a received DeviceTwin 
// causes this to be invoked.
static void TempControl_ApplicationPropertyCallback(const char* componentName, const char* propertyName, const char* propertyValue, int version)
{
    if (strcmp(componentName,"thermostat") == 0) 
    {
        if (strcmp(propertyName,"targetTemperature") == 0)
        {
            // Use the helper to get JSON we should end back
            STRING_HANDLE jsonToSend = PnPHelper_CreateReportedPropertyWithStatus("thermostat", "targetTemperature", propertyValue, 200, "", version);
            const char* jsonToSendStr = STRING_c_str(jsonToSend);
            // Use DeviceClient to transmit the JSON
            IoTHubDeviceClient_SendReportedState(NULL/* TODO: deviceClient*/, (const unsigned char*)jsonToSendStr, strlen(jsonToSendStr), NULL, NULL);
        }
    }
}

// TempControl_DeviceTwinCallback is invoked by IoT SDK when a twin - either full twin or a PATCH update - arrives.
static void TempControl_DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)userContextCallback;
    // We use a visitor pattern.  The helper visits each node in the passed in JSON and the helper calls back TempControl_ApplicationPropertyCallback, once per 
    // property.
    PnPHelper_ProcessTwinData(update_state, payLoad, size, TempControl_ApplicationPropertyCallback);
}

// TempControl_SendCurrentTemperature sends a telemetry value indicating the current temperature
void TempControl_SendCurrentTemperature(IOTHUB_DEVICE_CLIENT_HANDLE iotHubClientHandle) 
{
    IOTHUB_MESSAGE_HANDLE h = PnPHelper_CreateTelemetryMessageHandle("thermostat", "22");
    IoTHubDeviceClient_SendEventAsync(iotHubClientHandle, h, NULL, NULL);
    IoTHubMessage_Destroy(h);
}

int main(void)
{
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle;

    deviceHandle = PnPHelper_CreateDeviceClient(g_connectionString, g_ModelId, g_traceOn, TempControl_DeviceMethodCallback, TempControl_DeviceTwinCallback);

    if (deviceHandle == NULL)
    {
        LogError("Failure creating Iothub device");
    }
    else
    {
        printf("Successfully created deviceClient handle.  Hit Control-C to exit program\n");
        while (true)
        {
            // Wake up periodically to send telemetry.
            // IOTHUB_DEVICE_CLIENT_HANDLE brings in the IoTHub device convenience layer, which means that the IoTHub SDK itself 
            // spins a worker thread to perform all operations.
            TempControl_SendCurrentTemperature(deviceHandle);
            ThreadAPI_Sleep(g_sleepBetweenTelemetrySends);
        }

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_Destroy(deviceHandle);
        // Free all the sdk subsystem
        IoTHub_Deinit();        
    }

    return 0;
}

