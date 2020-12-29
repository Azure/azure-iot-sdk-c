// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "pnp_device_client_ll.h"
#ifdef USE_PROV_MODULE_FULL
// DPS functionality using symmetric keys is only available if the cmake 
// flags <-Duse_prov_client=ON -Dhsm_type_symm_key=ON -Drun_e2e_tests=OFF> are enabled when building the Azure IoT C SDK.
#include "pnp_dps_ll.h"
#endif

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
// For devices that do not have (or want) an OS level trusted certificate store,
// but instead bring in default trusted certificates from the Azure IoT C SDK.
#include "azure_c_shared_utility/shared_util_options.h"
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

//
// AllocateDeviceClientHandle does the actual createHandle call, depending on the security type
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE AllocateDeviceClientHandle(const PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceHandle = NULL;

    if (pnpDeviceConfiguration->securityType == PNP_CONNECTION_SECURITY_TYPE_CONNECTION_STRING)
    {
        if ((deviceHandle = IoTHubDeviceClient_LL_CreateFromConnectionString(pnpDeviceConfiguration->u.connectionString, MQTT_Protocol)) == NULL)
        {
            LogError("Failure creating IotHub client.  Hint: Check your connection string");
        }
    }
#ifdef USE_PROV_MODULE_FULL
    else if ((deviceHandle = PnP_CreateDeviceClientLLHandle_ViaDps(pnpDeviceConfiguration)) == NULL)
    {
        LogError("Cannot retrieve IoT Hub connection information from DPS client");
    }
#endif /* USE_PROV_MODULE_FULL */

    return deviceHandle;
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE PnP_CreateDeviceClientLLHandle(const PNP_DEVICE_CONFIGURATION* pnpDeviceConfiguration)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;
    bool urlAutoEncodeDecode = true;
    int iothubInitResult;
    bool result;

    // Before invoking ANY IoT Hub or DPS functionality, IoTHub_Init must be invoked.
    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        LogError("Failure to initialize client, error=%d", iothubInitResult);
        result = false;
    }
    else if ((deviceHandle = AllocateDeviceClientHandle(pnpDeviceConfiguration)) == NULL)
    {
        LogError("Unable to allocate deviceHandle");
        result = false;
    }
    // Sets verbosity level
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_LOG_TRACE, &pnpDeviceConfiguration->enableTracing)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set logging option, error=%d", iothubResult);
        result = false;
    }
    // Sets the name of ModelId for this PnP device.
    // This *MUST* be set before the client is connected to IoTHub.  We do not automatically connect when the 
    // handle is created, but will implicitly connect to subscribe for device method and device twin callbacks below.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_MODEL_ID, pnpDeviceConfiguration->modelId)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the ModelID, error=%d", iothubResult);
        result = false;
    }
    // Optionally, set the callback function that processes incoming device methods, which is the channel PnP Commands are transferred over
    else if ((pnpDeviceConfiguration->deviceMethodCallback != NULL) && (iothubResult = IoTHubDeviceClient_LL_SetDeviceMethodCallback(deviceHandle, pnpDeviceConfiguration->deviceMethodCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set device method callback, error=%d", iothubResult);
        result = false;
    }
    // Optionall, set the callback function that processes device twin changes from the IoTHub, which is the channel that PnP Properties are 
    // transferred over.  This will also automatically retrieve the full twin for the application on startup. 
    else if ((pnpDeviceConfiguration->deviceTwinCallback != NULL) && (iothubResult = IoTHubDeviceClient_LL_SetDeviceTwinCallback(deviceHandle, pnpDeviceConfiguration->deviceTwinCallback, (void*)deviceHandle)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set device twin callback, error=%d", iothubResult);
        result = false;
    }
    // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set auto Url encode option, error=%d", iothubResult);
        result = false;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on systems without built in certificate stores.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the trusted cert, error=%d", iothubResult);
        result = false;
    }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    else
    {
        result = true;
    }

    if ((result == false) && (deviceHandle != NULL))
    {
        IoTHubDeviceClient_LL_Destroy(deviceHandle);
        deviceHandle = NULL;
    }

    if ((result == false) &&  (iothubInitResult == 0))
    {
        IoTHub_Deinit();
    }

    return deviceHandle;
}
