// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
// This EXE acts as the device side component of DigitalTwin C SDK E2E testing.
// See ../readme.md for an architectural review of how it works.
//

//
// Core header files for C and IoTHub layer
//
#include <stdio.h>
#include <iothub.h>
#include <iothub_device_client.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/xlogging.h>
#include <azure_c_shared_utility/shared_util_options.h>

//
// Header files for interacting with DigitalTwin layer.
//
#include <digitaltwin_device_client.h>
#include <digitaltwin_client_common.h>
#include <digitaltwin_interface_client.h>

//
// Header files for test interfaces and utilities
//
#include "dt_e2e_util.h"
#include "dt_e2e_commands.h"
#include "dt_e2e_telemetry.h"
#include "dt_e2e_properties.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

//
// Indexies to help manage interfaces
//
#define DIGITALTWIN_E2E_DEVICE_MAX_INTERFACES 3
#define DIGITALTWIN_E2E_COMMAND_TEST_INDEX 0
#define DIGITALTWIN_E2E_TELEMETRY_TEST_INDEX 1
#define DIGITALTWIN_E2E_PROPERTY_TEST_INDEX 2

//
// DCM for the E2E test 
//
#define DIGITALTWIN_TEST_E2E_MODEL_ID "dtmi:azureiot:testinterfaces:cdevicesdk:DCM;1"

// Amount to sleep between querying state from the register interface loop
static const int digitalTwin_E2E_registerInterfacePollSleep = 1000;

// Maximum amount of times we'll poll for interface being ready.  Current
// defaults between this and 'digitalTwin_E2E_registerInterfacePollSleep' mean 
// we'll wait up to a minute for interface registration.
static const int digitalTwin_E2E_registerInterfaceMaxPolls = 60;

// digitalTwin_E2E_PollSleep sets time (1 second for now) between checking for whether test should stop execution.
static const int digitalTwin_E2E_PollSleep = 1000;

//
// DT_E2E_InitializeIotHubDeviceHandle initializes underlying IoTHub client, creates a device handle with the specified connection string,
// and sets some options on this handle prior to beginning.
//
static IOTHUB_DEVICE_CLIENT_HANDLE DT_E2E_InitializeIotHubDeviceHandle(const char* connectionString)
{
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubClientResult;
    bool traceOn = true;

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
        // Enable full logging at underlying IoTHub DeviceClient layer in case anything goes wrong.
        else if ((iothubClientResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_LOG_TRACE, &traceOn)) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed to set option %s, error=%d", OPTION_LOG_TRACE, iothubClientResult);
            IoTHubDeviceClient_Destroy(deviceHandle);
            deviceHandle = NULL;
        }
        else
        {
            LogInfo("Successfully created test device");
        }

        if (deviceHandle == NULL)
        {
            IoTHub_Deinit();
        }
    }

    return deviceHandle;
}


//
// DT_E2E_CreateTestInterfaces allocates test interface handles for e2e tests
//
static int DT_E2E_CreateTestInterfaces(DIGITALTWIN_INTERFACE_CLIENT_HANDLE* interfaceClientHandles)
{
    int result;

    if ((interfaceClientHandles[DIGITALTWIN_E2E_COMMAND_TEST_INDEX] = DT_E2E_Commands_CreateInterface()) == NULL)
    {
        LogError("Unable to create interface for testing commands");
        result = MU_FAILURE;
    }
    else if ((interfaceClientHandles[DIGITALTWIN_E2E_TELEMETRY_TEST_INDEX] = DT_E2E_Telemetry_CreateInterface()) == NULL)
    {
        LogError("Unable to create interface for testing commands");
        result = MU_FAILURE;
    }
    else if ((interfaceClientHandles[DIGITALTWIN_E2E_PROPERTY_TEST_INDEX] = DT_E2E_Properties_CreateInterface()) == NULL)
    {
        LogError("Unable to create interface for testing commands");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

//
// DT_E2E_GetDeviceConnectionString retrieves connection string.  Currently this is only
// passed from a command line.
//
const char* DT_E2E_GetDeviceConnectionString(int argc, char** argv)
{
    const char* result;

    if (argc != 2)
    {
        LogError("USAGE: %s [IoTHub device connection string]", argv[0]);
        result = NULL;
    }
    else
    {
        result = argv[1];
    }

    return result;
}

//
// Main will register the test interfaces and then go into a holding pattern until the graceful shutdown command is received.
// Logic inside the interfaces themselves handles further server interaction post-registration.
//
int main(int argc, char** argv)
{
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle = NULL; 
    DIGITALTWIN_DEVICE_CLIENT_HANDLE dtDeviceClientHandle = NULL;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandles[DIGITALTWIN_E2E_DEVICE_MAX_INTERFACES];
    int result;
    const char* deviceConnectionString;

    // Turn off buffering on output.  Sometimes too much is buffered at stdout/stderr level between
    // this executable's CRT and the Node.JS integration into stdout/stderr.  We want the
    // logs coming out as they arrive.
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    memset(&interfaceClientHandles, 0, sizeof(interfaceClientHandles));
    DT_E2E_Util_Init();

    if ((deviceConnectionString = DT_E2E_GetDeviceConnectionString(argc, argv)) == NULL)
    {
        LogError("Unable to get test device connection string");
        result = MU_FAILURE;
    }
    else if ((deviceHandle = DT_E2E_InitializeIotHubDeviceHandle(deviceConnectionString)) == NULL)
    {
        LogError("Could not allocate IoTHub Device handle");
        result = MU_FAILURE;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    else if ((result = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
    {
        LogError("Failed to set option %s, error=%d", OPTION_TRUSTED_CERT, (DIGITALTWIN_CLIENT_RESULT)result);
        result = MU_FAILURE;
    }
#endif
    else if ((result = (int)DigitalTwin_DeviceClient_CreateFromDeviceHandle(deviceHandle, DIGITALTWIN_TEST_E2E_MODEL_ID, &dtDeviceClientHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwin_DeviceClient_CreateFromDeviceHandle failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, (DIGITALTWIN_CLIENT_RESULT)result));
    }    
    else if (DT_E2E_CreateTestInterfaces(interfaceClientHandles) != 0)
    {
        LogError("Could not allocate IoTHub Device handle");
        result = MU_FAILURE;
    }
    else if (DigitalTwin_DeviceClient_RegisterInterfaces(dtDeviceClientHandle, interfaceClientHandles, DIGITALTWIN_E2E_DEVICE_MAX_INTERFACES) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwin_DeviceClient_RegisterInterfaces failed");
        result = MU_FAILURE;
    }
    else
    {
        // Now that we are registered, the test framework waits indefinitely as the remote test script executes.
        // We break out of this loop only when the graceful shutdown command arrives from the service.
        while (DT_E2E_Util_Should_Continue_Execution())
        {
            ThreadAPI_Sleep(digitalTwin_E2E_PollSleep);
        }

        // We care about whether the test ended gracefully or not so we can indicate appropriate error code.
        if (DT_E2E_Util_Has_Fatal_Error_Occurred())
        {
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }

    DT_E2E_Commands_DestroyInterface(interfaceClientHandles[DIGITALTWIN_E2E_COMMAND_TEST_INDEX]);
    DT_E2E_Telemetry_DestroyInterface(interfaceClientHandles[DIGITALTWIN_E2E_TELEMETRY_TEST_INDEX]);
    DT_E2E_Properties_DestroyInterface(interfaceClientHandles[DIGITALTWIN_E2E_PROPERTY_TEST_INDEX]);

    if (dtDeviceClientHandle != NULL)
    {
        DigitalTwin_DeviceClient_Destroy(dtDeviceClientHandle);
    }

    return result;
}
  
