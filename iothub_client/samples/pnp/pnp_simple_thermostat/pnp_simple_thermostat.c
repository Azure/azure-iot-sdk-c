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
#include "azure_c_shared_utility/xlogging.h"

#include "parson.h"
#include "pnp_device_client_helpers.h"

// Paste in your device connection string
static const char* g_connectionString = "[device connection string]";

// Amount of time to sleep between sending telemetry to Hub, in milliseconds.  Set to 1 minute.
static unsigned int g_sleepBetweenTelemetrySends = 60 * 1000;

// Whether tracing at the IoTHub client is enabled or not. 
static bool g_hubClientTraceEnabled = true;

// DTMI indicating this device's ModelId.
static const char g_ModelId[] = "dtmi:com:example:Thermostat;1";

// Global device handle.
IOTHUB_DEVICE_CLIENT_HANDLE g_deviceHandle;

// Name of command this component supports to get report information
static const char g_GetMinMaxReport[] = "getMaxMinReport";
// Name of json field to parse for "since".
static const char g_SinceJsonCommandSetting[] = "commandRequest.value.since";
// Return codes for device methods
static int g_commandSuccess = 200;
static int g_commandBadFormat = 400;
static int g_commandNotFoundStatus = 404;
static int g_commandInternalError = 500;

//
// CopyTwinPayloadToString takes the twin payload data, which arrives as a potentially non-NULL terminated string, and creates
// a new copy of the data with a NULL terminator.  The JSON parser this sample uses, parson, only operates over NULL terminated strings.
//
static char* CopyTwinPayloadToString(const unsigned char* payload, size_t size)
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
// BuildMaxMinCommandResponse builds the response to the command for getMaxMinReport
//

// snprintf format for building getMaxMinReport
static const char g_minMaxCommandResponseFormat[] = "{ \"tempReport\": { \"maxTemp\": %.2f, \"minTemp\": %.2f, \"avgTemp\": %.2f, \"startTime\": \"%s\", \"endTime\": \"%s\"  }}";

static bool BuildMaxMinCommandResponse(const char* sinceStr, unsigned char** response, size_t* responseSize)
{
    int responseBuilderSize;
    unsigned char* responseBuilder = NULL;
    bool result;

    if ((responseBuilderSize = snprintf(NULL, 0, g_minMaxCommandResponseFormat, 5.0, 6.0, 7.0, sinceStr, sinceStr)) < 0)
    {
        LogError("snprintf to determine string length failed");
        result = false;
    }
    else if ((responseBuilder = calloc(1, responseBuilderSize + 1)) == NULL)
    {
        LogError("Unable to allocate %lu bytes", (unsigned long)(responseBuilderSize + 1));
        result = false;
    }
    else if ((responseBuilderSize = snprintf((char*)responseBuilder, responseBuilderSize + 1, g_minMaxCommandResponseFormat, 5.0, 6.0, 7.0, sinceStr, sinceStr)) < 0)
    {
        LogError("snprintf to output buffer failed");
        result = false;
    }
    else
    {
        result = true;
    }

    if (result == true)
    {
        *response = responseBuilder;
        *responseSize = (size_t)responseBuilderSize;
        LogInfo("Response=<%s>", (const char*)responseBuilder);
    }
    else
    {
        free(responseBuilder);
    }

    return response;    
}       


//
// Thermostat_DeviceMethodCallback is invoked by IoT SDK when a device method arrives.
//
static int Thermostat_DeviceMethodCallback(const char* methodName, const unsigned char* payload, size_t size, unsigned char** response, size_t* responseSize, void* userContextCallback)
{
    (void)userContextCallback;

    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    //const char* sinceStr;
    int result;

    LogInfo("Device method %s arrived", methodName);


    *response = NULL;
    *responseSize = 0;

    if (strcmp(methodName, g_GetMinMaxReport) != 0)
    {
        LogError("Method name %s is not supported on this component", methodName);
        result = g_commandNotFoundStatus;
    }
    else if ((jsonStr = CopyTwinPayloadToString(payload, size)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
        result = g_commandInternalError;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse twin JSON");
        result = g_commandBadFormat;
    }
    else if ((rootObject = json_value_get_object(rootValue)) == NULL)
    {
        LogError("Unable to get root object of JSON");
        result = g_commandInternalError;
    }
    /*else if ((sinceStr = json_object_dotget_string(rootObject, g_SinceJsonCommandSetting)) == NULL)
    {
        LogError("Cannot retrieve JSON field %s", g_SinceJsonCommandSetting);
        result = g_commandBadFormat;
    }
    */
    else if (BuildMaxMinCommandResponse("since--data", response, responseSize) == false)
    {
        LogError("Unable to build response");
        result = g_commandInternalError;        
    }
    else
    {
        result = g_commandSuccess;
    }

    LogInfo("json=<%s>", json_serialize_to_string(rootValue));

    free(jsonStr);
    json_value_free(rootValue);

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


static const char PnP_JsonPropertyVersion[] = "$version";
static const char PnP_JsonTargetTemperature[] = "targetTemperature";
static double g_currentTemperature = 30;

//
// Thermostat_DeviceTwinCallback is invoked by IoT SDK when a twin - either full twin or a PATCH update - arrives.
//
static void Thermostat_DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, void* userContextCallback)
{
    (void)userContextCallback;

    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* desiredObject;
    JSON_Value* versionValue = NULL;
    JSON_Value* targetTemperatureValue = NULL;

    if ((jsonStr = CopyTwinPayloadToString(payload, size)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse twin JSON");
    }
    else if ((desiredObject = GetDesiredJson(updateState, rootValue)) == NULL)
    {
        LogError("Cannot retrieve desired JSON object");
    }
    else if ((targetTemperatureValue = json_object_get_value(desiredObject, PnP_JsonTargetTemperature)) == NULL)
    {
        // The target temperature not being set is NOT an error, but it does indicate we can't proceed any further.
        ;
    }
    else if ((versionValue = json_object_get_value(desiredObject, PnP_JsonPropertyVersion)) == NULL)
    {
        // The $version does need to be set in *any* legitimate twin desired document.  Its absence suggests 
        // something is fundamentally wrong with how we've received the twin and we should not proceed.
        LogError("Cannot retrieve %s field for twin", PnP_JsonPropertyVersion);
    }
    else
    {
        //int version = (int)json_value_get_number(versionValue);
        double targetTemperature = json_value_get_number(targetTemperatureValue);

        LogInfo("Received targetTemperature = %f", targetTemperature);
        g_currentTemperature = targetTemperature;
    }

    json_value_free(rootValue);
    free(jsonStr);
}


//
// Thermostat_SendCurrentTemperature sends a PnP telemetry indicating the current temperature
//
void Thermostat_SendCurrentTemperature(void) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;

    char temperatureStringBuffer[32];

    if (snprintf(temperatureStringBuffer, sizeof(temperatureStringBuffer), "%.02f", g_currentTemperature) < 0)
    {
        LogError("snprintf failed");
    }
    else if ((messageHandle = IoTHubMessage_CreateFromString(temperatureStringBuffer)) == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
    }
    else if ((iothubResult = IoTHubDeviceClient_SendEventAsync(g_deviceHandle, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send telemetry message, error=%d", iothubResult);
    }

    IoTHubMessage_Destroy(messageHandle);
}

int main(void)
{
    g_deviceHandle = PnPHelper_CreateDeviceClientHandle(g_connectionString, g_ModelId, g_hubClientTraceEnabled, Thermostat_DeviceMethodCallback, Thermostat_DeviceTwinCallback);

    if (g_deviceHandle == NULL)
    {
        LogError("Failure creating Iothub device");
    }
    else
    {
        LogInfo("Successfully created device client handle.  Hit Control-C to exit program\n");
        // TODO: Add a sample showing simple property update

        while (true)
        {
            // Wake up periodically to send telemetry.
            // IOTHUB_DEVICE_CLIENT_HANDLE brings in the IoTHub device convenience layer, which means that the IoTHub SDK itself 
            // spins a worker thread to perform all operations.
            Thermostat_SendCurrentTemperature();
            ThreadAPI_Sleep(g_sleepBetweenTelemetrySends);
        }

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_Destroy(g_deviceHandle);
        // Free all the sdk subsystem
        IoTHub_Deinit();        
    }

    return 0;
}
