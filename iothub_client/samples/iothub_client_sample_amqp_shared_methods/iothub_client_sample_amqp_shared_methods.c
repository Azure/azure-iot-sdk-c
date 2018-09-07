// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_message.h"
#include "iothub_client_options.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS

#ifdef SAMPLE_AMQP
#include "iothubtransportamqp.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
#include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS

static const char* hubName = "[IoT Hub Name]";
static const char* hubSuffix = "[IoT Hub Suffix]";
static const char* deviceId1 = "[device id 1]";
static const char* deviceId2 = "[device id 2]";
static const char* deviceKey1 = "[device key 1]";
static const char* deviceKey2 = "[device key 2]";

static int DeviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
    const char* device_id = (const char*)userContextCallback;

    (void)printf("\r\nDevice Method called for device %s\r\n", device_id);
    (void)printf("Device Method name:    %s\r\n", method_name);
    (void)printf("Device Method payload: %.*s\r\n", (int)size, (const char*)payload);

    int status = 200;
    char* RESPONSE_STRING = "{ \"Response\": \"This is the response from the device\" }";
    (void)printf("\r\nResponse status: %d\r\n", status);
    (void)printf("Response payload: %s\r\n\r\n", RESPONSE_STRING);

    *resp_size = strlen(RESPONSE_STRING);
    if ((*response = malloc(*resp_size)) == NULL)
    {
        status = -1;
    }
    else
    {
        memcpy(*response, RESPONSE_STRING, *resp_size);
    }
    return status;
}

int main(void)
{
    TRANSPORT_HANDLE transport_handle;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_DEVICE_CLIENT_HANDLE device_handle1;
    IOTHUB_DEVICE_CLIENT_HANDLE device_handle2;

#ifdef SAMPLE_AMQP
    protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS

    (void)printf("Starting the IoTHub client sample C2D methods on AMQP over WebSockets with multiplexing ...\r\n");

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    if ((transport_handle = IoTHubTransport_Create(protocol, hubName, hubSuffix)) == NULL)
    {
        (void)printf("Failed to creating the protocol handle.\r\n");
    }
    else
    {
        IOTHUB_CLIENT_CONFIG client_config1;
        client_config1.deviceId = deviceId1;
        client_config1.deviceKey = deviceKey1;
        client_config1.deviceSasToken = NULL;
        client_config1.iotHubName = hubName;
        client_config1.iotHubSuffix = hubSuffix;
        client_config1.protocol = protocol;
        client_config1.protocolGatewayHostName = NULL;

        IOTHUB_CLIENT_CONFIG client_config2;
        client_config2.deviceId = deviceId2;
        client_config2.deviceKey = deviceKey2;
        client_config2.deviceSasToken = NULL;
        client_config2.iotHubName = hubName;
        client_config2.iotHubSuffix = hubSuffix;
        client_config2.protocol = protocol;
        client_config2.protocolGatewayHostName = NULL;

        if ((device_handle1 = IoTHubDeviceClient_CreateWithTransport(transport_handle, &client_config1)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle1 is NULL!\r\n");
        }
        else if ((device_handle2 = IoTHubDeviceClient_CreateWithTransport(transport_handle, &client_config2)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle1 is NULL!\r\n");
        }
        else
        {
            // Set any option that are neccessary.
            // For available options please see the iothub_sdk_options.md documentation
            //bool traceOn = true;
            //IoTHubDeviceClient_SetOption(device_handle1, OPTION_LOG_TRACE, &traceOn);
            //IoTHubDeviceClient_SetOption(device_handle2, OPTION_LOG_TRACE, &traceOn);
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
            // Setting the Trusted Certificate.  This is only necessary on system without
            // built in certificate stores.
            IoTHubDeviceClient_SetOption(device_handle1, OPTION_TRUSTED_CERT, certificates);
            IoTHubDeviceClient_SetOption(device_handle2, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

            /* Here subscribe for C2D methods */
            if (IoTHubDeviceClient_SetDeviceMethodCallback(device_handle1, DeviceMethodCallback, (void*)deviceId1) != IOTHUB_CLIENT_OK)
            {
                (void)printf("ERROR: IoTHubClient_SetDeviceMethodCallback for the first device..........FAILED!\r\n");
            }
            else if (IoTHubDeviceClient_SetDeviceMethodCallback(device_handle2, DeviceMethodCallback, (void*)deviceId2) != IOTHUB_CLIENT_OK)
            {
                (void)printf("ERROR: IoTHubClient_SetDeviceMethodCallback for the second device..........FAILED!\r\n");
            }
            else
            {
                (void)printf("IoTHubClient_SetMessageCallback...successful.\r\n");
                (void)printf("Waiting for C2D methods ...\r\n");

                (void)printf("Press enter to close application\r\n");
                (void)getchar();
            }

            // Clean up the iothub sdk handle
            IoTHubDeviceClient_Destroy(device_handle1);
            IoTHubDeviceClient_Destroy(device_handle2);
        }
        IoTHubTransport_Destroy(transport_handle);
        // Free all the sdk subsystem
        IoTHub_Deinit();
    }
    return 0;
}
