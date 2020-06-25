// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"

#include "azure_c_shared_utility/xlogging.h"

IOTHUB_DEVICE_CLIENT_HANDLE InitializeIoTHubDeviceHandleForPnP(const char* connectionString, const char* modelId, bool enableTracing, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback)
{
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;
    bool urlEncodeOn = true;
    bool iothubInitFailed = false;
    int iothubInitResult;
    int result;

    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        iothubInitFailed = true;
        LogError("Failure to initialize client.  Error=%d", iothubInitResult);
        result = MU_FAILURE;
    }
    else if ((deviceHandle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, MQTT_Protocol)) == NULL)
    {
        LogError("Failure creating Iothub device.  Hint: Check you connection string");
        result = MU_FAILURE;
    }
    // Sets verbosity level
    else if ((iothubResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_LOG_TRACE, &enableTracing)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set logging option, error=%d", iothubResult);
        result = MU_FAILURE;
    }
    // Sets the name of ModelId for this PnP device.
    // This *MUST* be set before the client is connected to IoTHub.  The IoTHubDevice does not automatically create
    // a connection when the handle is created, but will implicitly create one in order to subscribe for DeviceMethod and twin callbacks
    // below.
    else if ((iothubResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_MODEL_ID, modelId)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the ModelID, error=%d", iothubResult);
        result = MU_FAILURE;
    }
    // Sets the callback function that processes incoming device methods, which are the channel PnP Commands are transferred over
    else if ((iothubResult = IoTHubDeviceClient_SetDeviceMethodCallback(deviceHandle, deviceMethodCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set device method callback, error=%d", iothubResult);
        result = MU_FAILURE;
    }
    // Sets the callback function that processes device twin changes from the hub, which is the channel that PnP Properties are 
    // transferred over.  This will also automatically retrieve the full twin for the application on initial connection.
    else if ((iothubResult = IoTHubDeviceClient_SetDeviceTwinCallback(deviceHandle, deviceTwinCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set device twin callback, error=%d", iothubResult);
        result = MU_FAILURE;
    }
    // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically for the application.
    else if ((iothubResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set auto Url encode option, error=%d", iothubResult);
        result = MU_FAILURE;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on system with without
    // built in certificate stores.
    else if ((iothubResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set auto Url encode option, error=%d", iothubResult);
        result = MU_FAILURE;
    }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    else
    {
        result = 0;
    }

    if ((result != 0) && (deviceHandle != NULL))
    {
        IoTHubDeviceClient_Destroy(deviceHandle);
        deviceHandle = NULL;
    }

    if (iothubInitFailed == true)
    {
        IoTHub_Deinit();
    }

    return deviceHandle;
}

