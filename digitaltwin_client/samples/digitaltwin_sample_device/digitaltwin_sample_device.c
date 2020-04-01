// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This EXE interacts with the DIGITALTWIN_DEVICE_CLIENT_HANDLE layer and registers the interfaces.
// It relies on other sample libraries (e.g. the deviceInfo and environmentalSensor samples)
// to create the interfaces, send data on them, and process callbacks for them.  This reflects
// the way that an actual DigitalTwin device may be composed from multiple interfaces, potentially
// written by different developers or even taken from across open source interface clients.

//
// Core header files for C and IoTHub layer
//
#include <stdio.h>
#include <iothub.h>
#include <iothub_device_client.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <azure_c_shared_utility/threadapi.h>
#include "azure_c_shared_utility/xlogging.h"

//
// Hedaer files for interacting with DigitalTwin layer.
//
#include <digitaltwin_device_client.h>
#include <digitaltwin_interface_client.h>

//
// Headers that implement the sample interfaces that this sample device registers
//
#include <../digitaltwin_sample_device_info/digitaltwin_sample_device_info.h>
#include <../digitaltwin_sample_environmental_sensor/digitaltwin_sample_environmental_sensor.h>
#include <../digitaltwin_sample_sdk_info/digitaltwin_sample_sdk_info.h>

#define DIGITALTWIN_SAMPLE_DEVICE_NUM_INTERFACES 3
#define DIGITALTWIN_SAMPLE_DEVICE_INFO_INDEX 0
#define DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX 1
#define DIGITALTWIN_SAMPLE_SDK_INFO_INDEX 2


// Amount to sleep between querying state from the register interface loop
static const int digitalTwinSampleDevice_registerInterfacePollSleep = 1000;

// Maximum amount of times we'll poll for interface being ready.  Current
// defaults between this and 'digitalTwinSampleDevice_registerInterfacePollSleep' mean 
// we'll wait up to a minute for interface registration.
static const int digitalTwinSampleDevice_registerInterfaceMaxPolls = 60;

// Amount to sleep in the main() thread for periodic maintenance tasks of interfaces, after registration
static const int digitalTwinSampleDevice_mainPollingInterval = 1000;

// Everytime the main loop wakes up, on the digitalTwinSampleDevice_sendTelemetryFrequency(th) pass will send a telemetry message 
static const int digitalTwinSampleDevice_sendTelemetryFrequency = 20;

//
// TODO`s: Configure core settings of application for your Digital Twin
//

// TODO: Fill in DIGITALTWIN_SAMPLE_DEVICE_CAPABILITY_MODEL_ID. E.g. 
#define DIGITALTWIN_SAMPLE_DEVICE_CAPABILITY_MODEL_ID "urn:YOUR_COMPANY_NAME_HERE:sample_device:1"

//
// END TODO section
//

// DigitalTwinSampleDevice_InitializeIotHubDeviceHandle initializes underlying IoTHub client, creates a device handle with the specified connection string,
// and sets some options on this handle prior to beginning.
static IOTHUB_DEVICE_HANDLE DigitalTwinSampleDevice_InitializeIotHubDeviceHandle(const char* connectionString, bool traceOn)
{
    IOTHUB_DEVICE_HANDLE deviceHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubClientResult;

    if (IoTHub_Init() != 0)
    {
        LogError("IoTHub_Init failed");
    }
    else
    {
        if ((deviceHandle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, MQTT_Protocol)) == NULL)
        {
            LogError("Failed to create device handle");
        }
        else if ((iothubClientResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_LOG_TRACE, &traceOn)) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed to set option %s, error=%d", OPTION_LOG_TRACE, iothubClientResult);
            IoTHubDeviceClient_Destroy(deviceHandle);
            deviceHandle = NULL;
        }
        else
        {
            // Never log the complete connection string , as this contains information
            // that could compromise security of the device.
            LogInfo("Successfully created device with connectionString=<****>, deviceHandle=<%p>", deviceHandle);
        }

        if (deviceHandle == NULL)
        {
            IoTHub_Deinit();
        }
    }

    return deviceHandle;
}

// main entry point.  Takes one argument, an IoTHub connection string to run the DIGITALTWIN_DEVICE_CLIENT off of.
int main(int argc, char *argv[])
{
    DIGITALTWIN_CLIENT_RESULT result;
    IOTHUB_DEVICE_HANDLE deviceHandle = NULL; 
    DIGITALTWIN_DEVICE_CLIENT_HANDLE dtDeviceClientHandle = NULL;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandles[DIGITALTWIN_SAMPLE_DEVICE_NUM_INTERFACES];

    memset(&interfaceClientHandles, 0, sizeof(interfaceClientHandles));

    if (argc != 2)
    {
        LogError("USAGE: digitaltwin_sample_device [IoTHub device connection string]");        
    }
    // First, we create a standard IOTHUB_DEVICE_HANDLE handle for DigitalTwin to consume.
    else if ((deviceHandle = DigitalTwinSampleDevice_InitializeIotHubDeviceHandle(argv[1], false)) == NULL)
    {
        LogError("Could not allocate IoTHub Device handle");
    }
    // DigitalTwin_DeviceClient_CreateFromDeviceHandle creates a DigitalTwin handle that processes DigitalTwin specific commands, using the deviceHandle we've created.
    // This DIGITALTWIN_DEVICE_CLIENT_HANDLE handles registering interface(s) and acts as a multiplexer/demultiplexer in case where multiple 
    // interfaces are tied to a given DIGITALTWIN_DEVICE_CLIENT_HANDLE.
    //
    // The call to DigitalTwin_DeviceClient_CreateFromDeviceHandle() transfers ownership
    // of this handle (including destroying it) to the DigitalTwin layer, analogous to a .release()
    // in some C++ helpers.  DO NOT USE deviceHandle after this point.
    else if ((result = DigitalTwin_DeviceClient_CreateFromDeviceHandle(deviceHandle, DIGITALTWIN_SAMPLE_DEVICE_CAPABILITY_MODEL_ID, &dtDeviceClientHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwin_DeviceClient_CreateFromDeviceHandle failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
    }
    // Invoke to the DeviceInfo interface - implemented in a separate library - to create DIGITALTWIN_INTERFACE_CLIENT_HANDLE.
    // NOTE: Other than creation and destruction, NO operations may occur on any DIGITALTWIN_INTERFACE_CLIENT_HANDLE
    // until after we've completed its registration.
    else if ((interfaceClientHandles[DIGITALTWIN_SAMPLE_DEVICE_INFO_INDEX] = DigitalTwinSampleDeviceInfo_CreateInterface()) == NULL)
    {
        LogError("DigitalTwinSampleDeviceInfo_CreateInterface failed");
    }
    // Invoke to create SdkInfo interface - implemented in a separate library - to create DIGITALTWIN_INTERFACE_CLIENT_HANDLE.
    else if ((interfaceClientHandles[DIGITALTWIN_SAMPLE_SDK_INFO_INDEX] = DigitalTwinSampleSdkInfo_CreateInterface()) == NULL)
    {
        LogError("DigitalTwinSampleSdkInfo_CreateInterface failed");
    }
    // Invoke to create Environmental Sensor interface - implemented in a separate library - to create DIGITALTWIN_INTERFACE_CLIENT_HANDLE.
    else if ((interfaceClientHandles[DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX] = DigitalTwinSampleEnvironmentalSensor_CreateInterface()) == NULL)
    {
        LogError("DigitalTwinSampleEnvironmentalSensor_CreateInterface failed");
    }
    // Register the interface we've created with Azure IoT.  This call will block until interfaces
    // are successfully registered, we get a failure from server, or we timeout.
    else if (DigitalTwin_DeviceClient_RegisterInterfaces(dtDeviceClientHandle, interfaceClientHandles, DIGITALTWIN_SAMPLE_DEVICE_NUM_INTERFACES) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwin_DeviceClient_RegisterInterfaces failed");
    }
    else
    {
        int numberOfIterations = 0;

        // DigitalTwin and all interfaces are ready to begin sending and receiving data.
        LogInfo("Beginning worker loop.  Will wake up every 20 seconds to send a telemetry message.  Hit Control-C to terminate");
        do 
        {
            if ((numberOfIterations % digitalTwinSampleDevice_sendTelemetryFrequency) == 0)
            {
                // Callbacks happen on an asynchronous worker thread that DigitalTwin.IoT SDK has spun up for this purpose, so
                // its OK to sleep for long durations on this thread.
                (void)DigitalTwinSampleEnvironmentalSensor_SendTelemetryMessagesAsync(interfaceClientHandles[DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX]);
            }
            (void)DigitalTwinSampleEnvironmentalSensor_ProcessDiagnosticIfNecessaryAsync(interfaceClientHandles[DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX]);
            numberOfIterations++;

            ThreadAPI_Sleep(digitalTwinSampleDevice_mainPollingInterval);
        }
        while (true);
    }

    if (interfaceClientHandles[DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX] != NULL)
    {
        DigitalTwinSampleEnvironmentalSensor_Close(interfaceClientHandles[DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX]);
    }

    if (interfaceClientHandles[DIGITALTWIN_SAMPLE_DEVICE_INFO_INDEX] != NULL)
    {
        DigitalTwinSampleDeviceInfo_Close(interfaceClientHandles[DIGITALTWIN_SAMPLE_DEVICE_INFO_INDEX]);
    }

    if (interfaceClientHandles[DIGITALTWIN_SAMPLE_SDK_INFO_INDEX] != NULL)
    {
        DigitalTwinSampleSdkInfo_Close(interfaceClientHandles[DIGITALTWIN_SAMPLE_SDK_INFO_INDEX]);
    }

    if (dtDeviceClientHandle != NULL)
    {
        DigitalTwin_DeviceClient_Destroy(dtDeviceClientHandle);
    }

    if ((dtDeviceClientHandle == NULL) && (deviceHandle != NULL))
    {
        // Only destroy the deviceHandle directly if we've never created a dtDeviceClientHandle
        // (dtDeviceClientHandle de facto takes ownership of deviceHandle once its created).
        IoTHubDeviceClient_Destroy(deviceHandle);
    }

    if (deviceHandle != NULL)
    {
        IoTHub_Deinit();
    }
    
    return 0;
}

