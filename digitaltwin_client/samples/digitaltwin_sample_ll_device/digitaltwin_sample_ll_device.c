// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Low level sample for e2e test
// This EXE interacts with the DIGITALTWIN_DEVICE_CLIENT_HANDLE layer and registers the interfaces.
// It relies on other sample libraries (e.g. the deviceInfo and environmentalSensor samples)
// to create the interfaces, send data on them, and process callbacks for them.  This reflects
// the way that an actual DigitalTwin device may be composed from multiple interfaces, potentially
// written by different developers or even taken from across open source interface clients.

//
// Core header files for C and IoTHub layer
//

// ENABLE_IOT_CENTRAL is defined when connecting to IoT Central, NOT IoTHub.  Note that
// additional cmake options (in particular -Duse_prov_client:BOOL=ON and potentially others
// to specify the correct HSM) are required for the SDK to generate the required provisioning libraries.
#define ENABLE_IOT_CENTRAL

// USE_DPS_AUTH_SYMM_KEY uses symmetric key based authentication.  This requires the cmake option -Dhsm_type_symm_key:BOOL=ON 
// to be set so the underlying Azure IoT C SDK and provisioning client have the correct settings.
#define USE_DPS_AUTH_SYMM_KEY

#include <stdio.h>
#include <iothub.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/xlogging.h>
#include <azure_c_shared_utility/crt_abstractions.h>

//
// Hedaer files for interacting with DigitalTwin layer.
//
#include <digitaltwin_device_client_ll.h>
#include <digitaltwin_interface_client.h>

// #define ENABLE_MODEL_DEFINITION_INTERFACE to enable ModelDefinition interface.  It is left out of default sample because it is not required and will add resources on constrained devices.
#ifdef ENABLE_MODEL_DEFINITION_INTERFACE
#include <digitaltwin_model_definition.h>
#endif


//
// Headers that implement the sample interfaces that this sample device registers
//
#include <../digitaltwin_sample_device_info/digitaltwin_sample_device_info.h>
#include <../digitaltwin_sample_environmental_sensor/digitaltwin_sample_environmental_sensor.h>
#include <../digitaltwin_sample_model_definition/digitaltwin_sample_model_definition.h>

//
// TODO`s: Configure core settings of application for your Digital Twin/IoTHub/IoT Central instance
//

// TODO: Fill in DIGITALTWIN_SAMPLE_DEVICE_CAPABILITY_MODEL_ID
#define DIGITALTWIN_SAMPLE_DEVICE_CAPABILITY_MODEL_ID "urn:YOUR_COMPANY_NAME_HERE:sample_device:1"

#ifdef ENABLE_IOT_CENTRAL
// TODO: Specify ID scope if you intend on using IoT Central
static const char* digitalTwinSampleDevice_GlobalProvUri = "global.azure-devices-provisioning.net";
static const char* digitalTwinSampleDevice_IdScope = "[ID Scope]";
#endif

#ifdef USE_DPS_AUTH_SYMM_KEY
// TODO: Specify deviceId and keys if you intend on using IoT Central and symmetric key based auth.
static const char* digitalTwinSampleDevice_deviceId = "[Device ID]";
static const char* digitalTwinSampleDevice_key      = "[Device Key]";
#endif

//
// END TODO section
//

#ifdef ENABLE_IOT_CENTRAL
// IoT Central requires DPS.  Include required header and constants
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_security_factory.h"

// State of DPS registration process.  We cannot proceed with DPS until we get into the state APP_DPS_REGISTRATION_SUCCEEDED.
typedef enum APP_DPS_REGISTRATION_STATUS_TAG
{
    APP_DPS_REGISTRATION_PENDING,
    APP_DPS_REGISTRATION_SUCCEEDED,
    APP_DPS_REGISTRATION_FAILED
} APP_DPS_REGISTRATION_STATUS;

#ifdef USE_DPS_AUTH_SYMM_KEY
const SECURE_DEVICE_TYPE secureDeviceTypeForProvisioning = SECURE_DEVICE_TYPE_SYMMETRIC_KEY;
const IOTHUB_SECURITY_TYPE secureDeviceTypeForIotHub = IOTHUB_SECURITY_TYPE_SYMMETRIC_KEY;

#else // USE_DPS_AUTH_SYMM_KEY
const SECURE_DEVICE_TYPE secureDeviceTypeForProvisioning = SECURE_DEVICE_TYPE_X509;
const IOTHUB_SECURITY_TYPE secureDeviceTypeForIotHub = IOTHUB_SECURITY_TYPE_X509;


#endif // USE_DPS_AUTH_SYMM_KEY

// Amount to sleep between querying state from DPS registration loop
static const int digitalTwinSampleDevice_dpsRegistrationPollSleep = 1000;

// Maximum amount of times we'll poll for DPS registration being ready.
static const int digitalTwinSampleDevice_dpsRegistrationMaxPolls = 60;

static const char* digitaltwinSample_InitialRegistrationWithIoTCentral = "InitialRegistrationWithIoTCentral";

static const char* digitaltwinSample_CustomProvisioningData = "{"
                                                          "\"__iot:interfaces\":"
                                                          "{"
                                                              "\"CapabilityModelId\": \"" DIGITALTWIN_SAMPLE_DEVICE_CAPABILITY_MODEL_ID "\""
                                                          "}"
                                                      "}";
#endif


// Number of DigitalTwin Interfaces that this DigitalTwin device supports.
#ifdef ENABLE_MODEL_DEFINITION_INTERFACE
#define DIGITALTWIN_SAMPLE_DEVICE_MAX_INTERFACES 3
#else
#define DIGITALTWIN_SAMPLE_DEVICE_MAX_INTERFACES 2
#endif
#define DIGITALTWIN_SAMPLE_DEVICE_INFO_INDEX 0
#define DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX 1
#define DIGITALTWIN_SAMPLE_MODEL_DEFINITION_INDEX 2

// Amount to sleep between querying state from the register interface loop
static const int digitalTwinSampleDevice_ll_registerInterfacePollSleep = 100;

// Maximum amount of times we'll poll for interface being ready.  Current
// defaults between this and 'digitalTwinSampleDevice_ll_registerInterfacePollSleep' mean
// we'll wait up to a minute for interface registration.
static const int digitalTwinSampleDevice_ll_registerInterfaceMaxPolls = 600;

// Amount to sleep in the main() thread for periodic maintenance tasks of interfaces, after registration, and especially DoWork().
static const int digitalTwinSampleDevice_ll_mainPollingInterval = 100;

// Everytime the main loop wakes up, on the digitalTwinSampleDevice_ll_sendTelemetryFrequency(th) pass will send a telemetry message 
static const int digitalTwinSampleDevice_ll_sendTelemetryFrequency = 200;


// State of DigitalTwin registration process.  We cannot proceed with DigitalTwin until we get into the state APP_DIGITALTWIN_REGISTRATION_SUCCEEDED.
typedef enum APP_DIGITALTWIN_REGISTRATION_STATUS_TAG
{
    APP_DIGITALTWIN_REGISTRATION_PENDING,
    APP_DIGITALTWIN_REGISTRATION_SUCCEEDED,
    APP_DIGITALTWIN_REGISTRATION_FAILED
} APP_DIGITALTWIN_REGISTRATION_STATUS;

//
// DigitalTwinSampleDevice_InterfacesRegistered is invoked when the interfaces have been registered or failed.
// The userContextCallback pointer is set to whether we succeeded or failed and checked by thread blocking
// for registration to complete.
static void DigitalTwinSampleDevice_LL_InterfacesRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void *userContextCallback)
{
    APP_DIGITALTWIN_REGISTRATION_STATUS* registrationStatus = (APP_DIGITALTWIN_REGISTRATION_STATUS*)userContextCallback;

    if (dtInterfaceStatus == DIGITALTWIN_CLIENT_OK)
    {
        LogInfo("Interface registration callback invoked, interfaces have been successfully registered");
        *registrationStatus = APP_DIGITALTWIN_REGISTRATION_SUCCEEDED;
    }
    else
    {
        LogError("Interface registration callback invoked with an error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT,dtInterfaceStatus));
        *registrationStatus = APP_DIGITALTWIN_REGISTRATION_FAILED;
    }
}

APP_DIGITALTWIN_REGISTRATION_STATUS appDigitalTwinRegistrationStatus = APP_DIGITALTWIN_REGISTRATION_PENDING;

// Invokes DigitalTwin_DeviceClient_LL_RegisterInterfacesAsync, which indicates to Azure IoT which DigitalTwin interfaces this device supports.
// The DigitalTwin Handle *is not valid* until this operation has completed (as indicated by the callback DigitalTwinSampleDevice_RegisterDigitalTwinInterfacesAndWait being invoked).
static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDevice_LL_RegisterDigitalTwinInterfacesAndWait(DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE digitaltwinDeviceClientLLHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE* interfaceClientHandles, int numInterfaceClientHandles)
{
    DIGITALTWIN_CLIENT_RESULT result;

    // Give DigitalTwin interfaces to register.  DigitalTwin_DeviceClient_RegisterInterfacesAsync returns immediately
    if ((result = DigitalTwin_DeviceClient_LL_RegisterInterfacesAsync(digitaltwinDeviceClientLLHandle, DIGITALTWIN_SAMPLE_DEVICE_CAPABILITY_MODEL_ID, interfaceClientHandles, numInterfaceClientHandles, DigitalTwinSampleDevice_LL_InterfacesRegisteredCallback, &appDigitalTwinRegistrationStatus)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwin_DeviceClient_LL_RegisterInterfacesAsync failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
    }
    else
    {
        // After registration, we do a simple polling algorithm to check for whether
        // the callback DigitalTwinSampleDevice_InterfacesRegistered has changed appDigitalTwinRegistrationStatus.  Since we can't
        // do any other DigitalTwin operations at this point, we have to block here.
        for (int i = 0; (i < digitalTwinSampleDevice_ll_registerInterfaceMaxPolls) && (appDigitalTwinRegistrationStatus == APP_DIGITALTWIN_REGISTRATION_PENDING); i++)
        {
            DigitalTwin_DeviceClient_LL_DoWork(digitaltwinDeviceClientLLHandle);
            ThreadAPI_Sleep(digitalTwinSampleDevice_ll_registerInterfacePollSleep);
        }

        if (appDigitalTwinRegistrationStatus == APP_DIGITALTWIN_REGISTRATION_SUCCEEDED)
        {
            LogInfo("DigitalTwin interfaces successfully registered");
            result = DIGITALTWIN_CLIENT_OK;
        }
        else if (appDigitalTwinRegistrationStatus == APP_DIGITALTWIN_REGISTRATION_PENDING)
        {
            LogError("Timed out attempting to register DigitalTwin interfaces");
            result = DIGITALTWIN_CLIENT_ERROR;
        }
        else
        {
            LogError("Error registering DigitalTwin interfaces");
            result = DIGITALTWIN_CLIENT_ERROR;
        }
    }

    return result;
}


#ifdef ENABLE_IOT_CENTRAL
char* digitaltwinSample_provisioning_IoTHubUri;
char* digitaltwinSample_provisioning_DeviceId;

static void provisioningRegisterCallback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    APP_DPS_REGISTRATION_STATUS* appDpsRegistrationStatus = (APP_DPS_REGISTRATION_STATUS*)user_context;

    if (register_result != PROV_DEVICE_RESULT_OK)
    {
        LogError("DPS Provisioning callback called with error state %d", register_result);
        *appDpsRegistrationStatus = APP_DPS_REGISTRATION_FAILED;
    }
    else
    {
        if ((mallocAndStrcpy_s(&digitaltwinSample_provisioning_IoTHubUri, iothub_uri) != 0) ||
            (mallocAndStrcpy_s(&digitaltwinSample_provisioning_DeviceId, device_id) != 0))
        {
            LogError("Unable to copy provisioning information");
            *appDpsRegistrationStatus = APP_DPS_REGISTRATION_FAILED;
        }
        else
        {
            LogInfo("Provisioning callback indicates success.  iothubUri=%s, deviceId=%s", digitaltwinSample_provisioning_IoTHubUri, digitaltwinSample_provisioning_DeviceId);
            *appDpsRegistrationStatus = APP_DPS_REGISTRATION_SUCCEEDED;
        }
    }
}

static IOTHUB_DEVICE_CLIENT_LL_HANDLE DigitalTwinSampleDevice_LL_InitializeIotHubViaProvisioning(bool traceOn)
{
    PROV_DEVICE_RESULT provDeviceResult;
    PROV_DEVICE_LL_HANDLE provDeviceLLHandle = NULL;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceLLHandle = NULL;

    APP_DPS_REGISTRATION_STATUS appDpsRegistrationStatus = APP_DPS_REGISTRATION_PENDING;

    if (IoTHub_Init() != 0)
    {
        LogError("IoTHub_Init failed");
    }
#ifdef USE_DPS_AUTH_SYMM_KEY
    else if (prov_dev_set_symmetric_key_info(digitalTwinSampleDevice_deviceId, digitalTwinSampleDevice_key) != 0) 
    {
        LogError("prov_dev_set_symmetric_key_info failed.");
    }
#endif
    else if (prov_dev_security_init(secureDeviceTypeForProvisioning) != 0)
    {
        LogError("prov_dev_security_init failed");
    }
    else if ((provDeviceLLHandle = Prov_Device_LL_Create(digitalTwinSampleDevice_GlobalProvUri, digitalTwinSampleDevice_IdScope, Prov_Device_MQTT_Protocol)) == NULL)
    {
        LogError("failed calling Prov_Device_Create");
    }
    else if ((provDeviceResult = Prov_Device_LL_SetOption(provDeviceLLHandle, PROV_OPTION_LOG_TRACE, &traceOn)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Setting provisioning tracing on failed, error=%d",provDeviceResult);
    }
    else if ((provDeviceResult = Prov_Device_LL_Set_Provisioning_Payload(provDeviceLLHandle, digitaltwinSample_CustomProvisioningData)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Failed setting provisioning data, error=%d",provDeviceResult);
    }
    else if ((provDeviceResult = Prov_Device_LL_Register_Device(provDeviceLLHandle, provisioningRegisterCallback, &appDpsRegistrationStatus, NULL, NULL)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Prov_Device_Register_Device failed, error=%d", provDeviceResult);
    }
    else
    {
        for (int i = 0; (i < digitalTwinSampleDevice_dpsRegistrationMaxPolls) && (appDpsRegistrationStatus == APP_DPS_REGISTRATION_PENDING); i++)
        {
            ThreadAPI_Sleep(digitalTwinSampleDevice_dpsRegistrationPollSleep);
            Prov_Device_LL_DoWork(provDeviceLLHandle);
        }

        if (appDpsRegistrationStatus == APP_DPS_REGISTRATION_SUCCEEDED)
        {
            LogInfo("DPS successfully registered.  Continuing on to creation of IoTHub device client handle.");
        }
        else if (appDpsRegistrationStatus == APP_DIGITALTWIN_REGISTRATION_PENDING)
        {
            LogError("Timed out attempting to register DPS device");
        }
        else
        {
            LogError("Error registering device for DPS");
        }
    }

    if (provDeviceLLHandle != NULL)
    {
        // We are breaking our convention of only freeing resources at the end of the function.  
        // In the block immediately below, we create an deviceLLHandle.  On a low-memory footprint
        // device, having both a provDeviceLLHandle and a deviceLLHandle open at the same time
        // can create substantial memory pressure.  Free provDeviceLLHandle to help.
        Prov_Device_LL_Destroy(provDeviceLLHandle);
        provDeviceLLHandle = NULL;
    }

    if (appDpsRegistrationStatus == APP_DPS_REGISTRATION_SUCCEEDED)
    {
        // The initial provisioning stage has succeeded.  Now use this material to connect to IoTHub.
        IOTHUB_CLIENT_RESULT iothubClientResult;

        if (iothub_security_init(secureDeviceTypeForIotHub) != 0)
        {
            LogError("iothub_security_init failed");
        }
        else if ((deviceLLHandle = IoTHubDeviceClient_LL_CreateFromDeviceAuth(digitaltwinSample_provisioning_IoTHubUri, digitaltwinSample_provisioning_DeviceId, MQTT_Protocol)) == NULL)
        {
            LogError("IoTHubDeviceClient_CreateFromDeviceAuth failed");
        }
        else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceLLHandle, OPTION_LOG_TRACE, &traceOn)) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed to set option %s, error=%d", OPTION_LOG_TRACE, iothubClientResult);
            IoTHubDeviceClient_LL_Destroy(deviceLLHandle);
            deviceLLHandle = NULL;
        }
        else
        {
            // Never log the complete connection string , as this contains information
            // that could compromise security of the device.
            LogInfo("***** Successfully created device device=<%s> via provisioning *****", digitaltwinSample_provisioning_DeviceId);
        }
    }

    if (deviceLLHandle == NULL)
    {
        IoTHub_Deinit();
    }

    return deviceLLHandle;
}
#endif


// DigitalTwinSampleDevice_InitializeIotHubDeviceHandle initializes underlying IoTHub client, creates a device handle with the specified connection string,
// and sets some options on this handle prior to beginning.
static IOTHUB_DEVICE_CLIENT_LL_HANDLE DigitalTwinSampleDevice_LL_InitializeIotHubViaConnectionString(const char* connectionString, bool traceOn)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceLLHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubClientResult;

    if (IoTHub_Init() != 0)
    {
        LogError("IoTHub_Init failed");
    }
    else if ((deviceLLHandle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol)) == NULL)
    {
        LogError("Failed to create device ll handle");
    }
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceLLHandle, OPTION_LOG_TRACE, &traceOn)) != IOTHUB_CLIENT_OK)
    {
        LogError("Failed to set option %s, error=%d", OPTION_LOG_TRACE, iothubClientResult);
        IoTHubDeviceClient_LL_Destroy(deviceLLHandle);
        deviceLLHandle = NULL;
    }
    else
    {
        // Never log the complete connection string , as this contains information
        // that could compromise security of the device.
        LogInfo("Successfully created device with connectionString=<****>, deviceLLHandle=<%p>", deviceLLHandle);
    }

    if (deviceLLHandle == NULL)
    {
        IoTHub_Deinit();
    }

    return deviceLLHandle;
}


// commandArgument is either the connection string or else it's a flag for using DPS for IoT Central
static IOTHUB_DEVICE_CLIENT_LL_HANDLE DigitalTwinSampleDevice_LL_CreateHandle(const char* commandArgument)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceLLHandle = NULL;

#ifdef ENABLE_IOT_CENTRAL
    if (0 == strcmp(commandArgument, digitaltwinSample_InitialRegistrationWithIoTCentral))
    {
        deviceLLHandle = DigitalTwinSampleDevice_LL_InitializeIotHubViaProvisioning(true);
    }
    else
    {
        deviceLLHandle = DigitalTwinSampleDevice_LL_InitializeIotHubViaConnectionString(commandArgument, false);
    }
#else
    // If IoTCentral not enabled, always treat command argument as a connection string
    deviceLLHandle = DigitalTwinSampleDevice_LL_InitializeIotHubViaConnectionString(commandArgument, false);
#endif

    return deviceLLHandle;
}


// main entry point.  Takes one argument, an IoTHub connection string to run the DIGITALTWIN_DEVICE_CLIENT off of.
int main(int argc, char *argv[])
{
    DIGITALTWIN_CLIENT_RESULT result;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceLLHandle = NULL;
    DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE digitaltwinDeviceClientLLHandle = NULL;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandles[DIGITALTWIN_SAMPLE_DEVICE_MAX_INTERFACES];
#ifdef ENABLE_MODEL_DEFINITION_INTERFACE
    MODEL_DEFINITION_CLIENT_HANDLE modeldefClientHandle = NULL;
#endif

    memset(&interfaceClientHandles, 0, sizeof(interfaceClientHandles));

    // How we create the handle depends on whether we're using IoT Central or not through preview.  (Note this
    // will be substantially cleaned up prior to general release).  For now, however, for going to IoTHub directly
    // then the caller must pass a connection string.  For using IoT Central, the device will be created
    // automatically via DPS.
    if (argc != 2)
    {
        LogError("USAGE: digitaltwin_sample_device [IoTHub device connection string || InitialRegistrationWithIoTCentral]");
    }
    // First, we create a standard IOTHUB_DEVICE_CLIENT_LL_HANDLE handle for DigitalTwin to consume.
    else if ((deviceLLHandle = DigitalTwinSampleDevice_LL_CreateHandle(argv[1])) == NULL)
    {
        LogError("Could not allocate IoTHub Device handle");
    }
    // DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle creates a DigitalTwin handle that processes DigitalTwin specific commands, using the deviceLLHandle we've created.
    // This DIGITALTWIN_DEVICE_CLIENT_HANDLE handles registering interface(s) and acts as a multiplexer/demultiplexer in case where multiple
    // interfaces are tied to a given DIGITALTWIN_DEVICE_CLIENT_HANDLE.
    //
    // The call to DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle() transfers ownership
    // of this handle (including destroying it) to the DigitalTwin layer, analogous to a .release()
    // in some C++ helpers.  DO NOT USE deviceLLHandle after this point.  Note this behavior
    // will change once DPS integration becomes available later.
    else if ((result = DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle(deviceLLHandle, &digitaltwinDeviceClientLLHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
    }
    // Invoke to the DeviceInfo interface - implemented in a separate library - to create DIGITALTWIN_INTERFACE_CLIENT_HANDLE.
    // NOTE: Other than creation and destruction, NO operations may occur on any DIGITALTWIN_INTERFACE_CLIENT_HANDLE
    // until after we've completed its registration (see DigitalTwinSampleDevice_LL_RegisterDigitalTwinInterfacesAndWait).
    else if ((interfaceClientHandles[DIGITALTWIN_SAMPLE_DEVICE_INFO_INDEX] = DigitalTwinSampleDeviceInfo_CreateInterface()) == NULL)
    {
        LogError("DigitalTwinSampleDeviceInfo_CreateInterface failed");
    }
    // Invoke to create Environmental Sensor interface - implemented in a separate library - to create DIGITALTWIN_INTERFACE_CLIENT_HANDLE.
    else if ((interfaceClientHandles[DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX] = DigitalTwinSampleEnvironmentalSensor_CreateInterface()) == NULL)
    {
        LogError("DigitalTwinSampleEnvironmentalSensor_CreateInterface failed");
    }
#ifdef ENABLE_MODEL_DEFINITION_INTERFACE
    // Invoke to create Model Definition interface - implemented in a separate library - to create DIGITALTWIN_INTERFACE_CLIENT_HANDLE.
    else if ((result = DigitalTwin_ModelDefinition_Create(&modeldefClientHandle, &(interfaceClientHandles[DIGITALTWIN_SAMPLE_MODEL_DEFINITION_INDEX]))) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwin_ModelDefinition_Create failed");
    }
    // Add environmental sensor interface dtdl for model reference interface request callback
    else if ((result = DigitalTwinSampleModelDefinition_ParseAndPublish(modeldefClientHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwinSampleModelDefinition_ParseAndPublish failed");
    }
#endif
    // Register the interface we've created with Azure IoT.  This call will block until interfaces
    // are successfully registered, we get a failure from server, or we timeout.
    else if (DigitalTwinSampleDevice_LL_RegisterDigitalTwinInterfacesAndWait(digitaltwinDeviceClientLLHandle, interfaceClientHandles, DIGITALTWIN_SAMPLE_DEVICE_MAX_INTERFACES) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DigitalTwinSampleDevice_LL_RegisterDigitalTwinInterfacesAndWait failed");
    }
    else
    {
        // DigitalTwin and all interfaces are ready to begin sending and receiving data. 
        LogInfo("Beginning worker loop.  Will wake up periodically to send a telemetry message, process DoWork(), and potentially process async processing.  Hit Control-C to terminate");
        int numberOfIterations = 0;

        do
        {
            // For every digitalTwinSampleDevice_ll_sendTelemetryFrequency cycles through this do/while loop, we will send a telemetry message.
            // We don't do this on every DoWork to avoid sending too much traffic to the server.
            if ((numberOfIterations % digitalTwinSampleDevice_ll_sendTelemetryFrequency) == 0)
            {
                (void)DigitalTwinSampleEnvironmentalSensor_SendTelemetryMessagesAsync(interfaceClientHandles[DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX]);
            }
            (void)DigitalTwinSampleEnvironmentalSensor_ProcessDiagnosticIfNecessaryAsync(interfaceClientHandles[DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INDEX]);

            // All DigitalTwin operations - both processing any pending telemetry to send, listening for incoming data, and invocation of DigitalTwin callbacks -
            // occur *ONLY* when DoWork() is invoked when using the _LL_ layer.  This lets the application require only one thread.  See the
            // convenience layer sample to avoid requiring this explicit DoWork call and to let DigitalTwin SDK perform this work on a separate thread.
            DigitalTwin_DeviceClient_LL_DoWork(digitaltwinDeviceClientLLHandle);
            ThreadAPI_Sleep(digitalTwinSampleDevice_ll_mainPollingInterval);
            numberOfIterations++;
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

#ifdef ENABLE_MODEL_DEFINITION_INTERFACE
    if (modeldefClientHandle != NULL)
    {
        DigitalTwin_ModelDefinition_Destroy(modeldefClientHandle);
    }
#endif
    
    if (digitaltwinDeviceClientLLHandle != NULL)
    {
        DigitalTwin_DeviceClient_LL_Destroy(digitaltwinDeviceClientLLHandle);
    }

    if ((digitaltwinDeviceClientLLHandle == NULL) && (deviceLLHandle != NULL))
    {
        // Only destroy the deviceLLHandle directly if we've never created a dtDeviceClientHandle
        // (dtDeviceClientHandle de facto takes ownership of deviceLLHandle once its created).
        IoTHubDeviceClient_LL_Destroy(deviceLLHandle);
    }

    if (deviceLLHandle != NULL)
    {
        IoTHub_Deinit();
    }

    return 0;
}

