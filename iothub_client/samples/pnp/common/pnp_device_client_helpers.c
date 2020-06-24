// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"

//#include "iothub_message.h"
//#include "azure_c_shared_utility/crt_abstractions.h"
//#include "azure_c_shared_utility/platform.h"
//#include "azure_c_shared_utility/shared_util_options.h"
//#include "azure_c_shared_utility/tickcounter.h"

IOTHUB_DEVICE_CLIENT_HANDLE InitializeIoTHubDeviceHandleForPnP(const char* connectionString, const char* modelId, bool enableTracing, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback)
{
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle;

    (void)IoTHub_Init();

    deviceHandle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (deviceHandle == NULL)
    {
        (void)printf("Failure creating Iothub device.  Hint: Check you connection string.\r\n");
    }
    else
    {
        // Sets the callback function that processes incoming device methods from the hub.
        (void)IoTHubDeviceClient_SetDeviceMethodCallback(deviceHandle, deviceMethodCallback, NULL);

        // Sets the callback function that processes incoming twin properties from the hub.  This will
        // also automatically retrieve the full twin for the application.
        (void)IoTHubDeviceClient_SetDeviceTwinCallback(deviceHandle, deviceTwinCallback, NULL);

        // Sets the name of ModelId for this PnP device.
        (void)IoTHubDeviceClient_SetOption(deviceHandle, OPTION_MODEL_ID, modelId);

        // Sets the level of verbosity of the logging
        if (enableTracing == true)
        {
            (void)IoTHubDeviceClient_SetOption(deviceHandle, OPTION_LOG_TRACE, &enableTracing);
        }

        // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically for the application.
        bool urlEncodeOn = true;
        (void)IoTHubDeviceClient_SetOption(deviceHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate.  This is only necessary on system with without
        // built in certificate stores.
        (void)IoTHubDeviceClient_SetOption(deviceHandle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    }

    return deviceHandle;
}

