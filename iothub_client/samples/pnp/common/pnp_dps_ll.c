// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef USE_PROV_MODULE_FULL
#error "Missing CMake flag for DPS"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iothubtransportmqtt.h"
#include "pnp_sample_config.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "azure_c_shared_utility/shared_util_options.h"
#include "certs.h"
#endif

// DPS and IoT Hub related header files
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_prov_client/prov_device_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "iothub_device_client_ll.h"

// DPS and IoT core utility header files.
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

// State of DPS registration process.  We cannot proceed with IoT Hub connection unless/until 
// we get into the state PNP_DPS_REGISTRATION_SUCCEEDED.
typedef enum PNP_DPS_REGISTRATION_STATUS_TAG
{
    PNP_DPS_REGISTRATION_NOT_COMPLETE,
    PNP_DPS_REGISTRATION_SUCCEEDED,
    PNP_DPS_REGISTRATION_FAILED
} PNP_DPS_REGISTRATION_STATUS;

// Tracks current status in registering for DPS.
PNP_DPS_REGISTRATION_STATUS g_pnpDpsRegistrationStatus;

// Maximum amount of times we'll poll for DPS registration being ready.  Note that even though DPS works off of callbacks,
// the main() loop itself blocks to keep the sample more straightforward.
static const int g_dpsRegistrationMaxPolls = 60;
// Amount to sleep between querying state from DPS registration loop.
static const int g_dpsRegistrationPollSleep = 1000;

// IoT Hub for this device as determined by the DPS client runtime.
static char* g_dpsIothubUri;
// DeviceId for this device as determined by the DPS client runtime.
static char* g_dpsDeviceId;

// Maximum size ModelId DPS payload can be in this sample.  This comes from
// the DTDLv2 limits (see https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/dtdlv2.md#interface), where the 
// maximum number of characters for an ID is 128.  We reserve extra space for the JSON envelope.
#define PNP_DPS_REGISTRATION_MAX_PAYLOAD_SIZE (128 + 16)
// snprintf style format for building the payload for Prov_Device_LL_Set_Provisioning_Payload() call.
static const char PNP_DPS_REGISTRATION_FMT[] = "{\"modelId\":\"%s\"}";

//
// provisioningRegisterCallback is called by the DPS client when the DPS server has either succeeded or failed the DPS
// provisioning process.  We store in global variables the result code and (on success) the IoT Hub and device Id so we can 
// later use this to create an IoT Hub connection.
//
static void provisioningRegisterCallback(PROV_DEVICE_RESULT registerResult, const char* iothubUri, const char* deviceId, void* userContext)
{
    (void)userContext;

    if (registerResult != PROV_DEVICE_RESULT_OK)
    {
        LogError("DPS Provisioning callback called with error state %d", registerResult);
        g_pnpDpsRegistrationStatus = PNP_DPS_REGISTRATION_FAILED;
    }
    else
    {
        if ((mallocAndStrcpy_s(&g_dpsIothubUri, iothubUri) != 0) ||
            (mallocAndStrcpy_s(&g_dpsDeviceId, deviceId) != 0))
        {
            LogError("Unable to copy provisioning information");
            g_pnpDpsRegistrationStatus = PNP_DPS_REGISTRATION_FAILED;
        }
        else
        {
            LogInfo("Provisioning callback indicates success.  iothubUri=%s, deviceId=%s", iothubUri, deviceId);
            g_pnpDpsRegistrationStatus = PNP_DPS_REGISTRATION_SUCCEEDED;
        }
    }
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE PnP_CreateDeviceClientLLHandle_ViaDps(const PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;
    bool result;

    PROV_DEVICE_RESULT provDeviceResult;
    PROV_DEVICE_LL_HANDLE provDeviceClient = NULL;
    char modelIdPayload[PNP_DPS_REGISTRATION_MAX_PAYLOAD_SIZE];

    LogInfo("Initiating DPS client to retrieve IoT Hub connection information");
    g_pnpDpsRegistrationStatus = PNP_DPS_REGISTRATION_NOT_COMPLETE;

    // Initial DPS setup and handle creation.
    if ((prov_dev_set_symmetric_key_info(pnpDeviceConfiguration->u.dpsConnectionAuth.deviceId, pnpDeviceConfiguration->u.dpsConnectionAuth.deviceKey) != 0))
    {
        LogError("prov_dev_set_symmetric_key_info failed.");
        result = false;
    }
    else if (prov_dev_security_init(SECURE_DEVICE_TYPE_SYMMETRIC_KEY) != 0)
    {
        LogError("prov_dev_security_init failed");
        result = false;
    }
    else if ((provDeviceClient = Prov_Device_LL_Create(pnpDeviceConfiguration->u.dpsConnectionAuth.endpoint, pnpDeviceConfiguration->u.dpsConnectionAuth.idScope, Prov_Device_MQTT_Protocol)) == NULL)
    {
        LogError("Failed calling Prov_Device_LL_Create");
        result = false;
    }
    else if ((provDeviceResult = Prov_Device_LL_SetOption(provDeviceClient, PROV_OPTION_LOG_TRACE, &pnpDeviceConfiguration->enableTracing)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Setting provisioning tracing on failed, error=%d", provDeviceResult);
        result = false;
    }
    else if (snprintf(modelIdPayload, sizeof(modelIdPayload), PNP_DPS_REGISTRATION_FMT, pnpDeviceConfiguration->modelId) < 0)
    {
        LogError("building modelId payload unsuccessful");
        result = false;
    }
    else if ((provDeviceResult = Prov_Device_LL_Set_Provisioning_Payload(provDeviceClient, modelIdPayload)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Prov_Device_LL_Register_Device failed, error=%d", provDeviceResult);
        result = false;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on systems without built in certificate stores.
    else if ((provDeviceResult = Prov_Device_LL_SetOption(provDeviceClient, OPTION_TRUSTED_CERT, certificates)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Setting provisioning trusted certificate, error=%d", provDeviceResult);
        result = false;
    }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    // Initiate the registration process.  Note that this call does NOT directly invoke network I/O; instead the
    // Prov_Device_LL_DoWork is used to drive this.
    else if ((provDeviceResult = Prov_Device_LL_Register_Device(provDeviceClient, provisioningRegisterCallback, NULL, NULL, NULL)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Prov_Device_LL_Register_Device failed, error=%d", provDeviceResult);
        result = false;
    }
    else
    {
        for (int i = 0; (i < g_dpsRegistrationMaxPolls) && (g_pnpDpsRegistrationStatus == PNP_DPS_REGISTRATION_NOT_COMPLETE); i++)
        {
            // Because we are using the LL (lower-layer), the application itself must manually poll/query status periodically
            // via Prov_Device_LL_DoWork.  On success or failure from the service, provisioningRegisterCallback will be invoked.
            Prov_Device_LL_DoWork(provDeviceClient);
            ThreadAPI_Sleep(g_dpsRegistrationPollSleep);
        }

        // We break out of for loop above either after a timeout or when provisioningRegisterCallback above changes global state information.
        if (g_pnpDpsRegistrationStatus == PNP_DPS_REGISTRATION_SUCCEEDED)
        {
            LogInfo("DPS successfully registered.  Continuing on to creation of IoT Hub device client handle.");
            result = true;
        }
        else if (g_pnpDpsRegistrationStatus == PNP_DPS_REGISTRATION_NOT_COMPLETE)
        {
            LogError("Timed out attempting to register DPS device");
            result = false;
        }
        else
        {
            LogError("Error registering device for DPS");
            result = false;
        }
    }

    // Destroy the provisioning handle here, instead of the typical convention of doing so at the end of the function.
    // It is no longer required.  On very constrained devices, leaving it open while calling 
    // IoTHubDeviceClient_LL_CreateFromDeviceAuth introduces unneeded memory pressure.
    if (provDeviceClient != NULL)
    {
        Prov_Device_LL_Destroy(provDeviceClient);
    }

    if (result == true)
    {
        // Use IoTHubDeviceClient_LL_CreateFromDeviceAuth(), which will interface with a successful DPS regsitration
        // to create the deviceClient for the application.
        if (iothub_security_init(IOTHUB_SECURITY_TYPE_SYMMETRIC_KEY) != 0)
        {
            LogError("iothub_security_init failed");
            result = false;
        }
        else if ((deviceClient = IoTHubDeviceClient_LL_CreateFromDeviceAuth(g_dpsIothubUri, g_dpsDeviceId, MQTT_Protocol)) == NULL)
        {
            LogError("IoTHubDeviceClient_LL_CreateFromDeviceAuth failed");
            result = true;
        }
    }

    free(g_dpsIothubUri);
    free(g_dpsDeviceId);

    return deviceClient;
}
