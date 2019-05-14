// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

// CAVEAT #2: This is a very minimal sample used to demonstrate how one can build one's own application in a cross compile
// environment using Docker.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothubtransportmqtt.h"

/* This sample uses the convenience APIs of iothub_client for example purposes. */

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/* Paste in your device connection string  */
static const char* connectionString = "[device connection string]";

#define MESSAGE_COUNT        5
int g_interval = 10000;  // 10 sec send interval initially
static size_t g_message_count_send_confirmations = 0;

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get envoked
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol = MQTT_Protocol;

    IOTHUB_MESSAGE_HANDLE message_handle;
    float telemetry_temperature;
    float telemetry_humidity;
    const char* telemetry_scale = "Celcius";
    char telemetry_msg_buffer[80];

    int messagecount = 0;

    printf("\r\nThis sample will send messages continuously and accept C2D messages.\r\nPress Ctrl+C to terminate the sample.\r\n\r\n");

    IOTHUB_DEVICE_CLIENT_HANDLE device_handle;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    (void)printf("Creating IoTHub handle\r\n");
    // Create the iothub handle here
    device_handle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, protocol);
    if (device_handle == NULL)
    {
        (void)printf("Failure creating Iothub device.  Hint: Check you connection string.\r\n");
    }
    else
    {
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate.  This is only necessary on system with without
        // built in certificate stores.
        (void)IoTHubDeviceClient_SetOption(device_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

        int i = MESSAGE_COUNT;

        while(i--)
        {
            // Construct the iothub message
            telemetry_temperature = 20.0f + ((float)rand() / RAND_MAX) * 15.0f;
            telemetry_humidity = 60.0f + ((float)rand() / RAND_MAX) * 20.0f;

            sprintf(telemetry_msg_buffer, "{\"temperature\":%.3f,\"humidity\":%.3f,\"scale\":\"%s\"}", 
                telemetry_temperature, telemetry_humidity, telemetry_scale);

            message_handle = IoTHubMessage_CreateFromString(telemetry_msg_buffer);

            (void)printf("\r\nSending message %d to IoTHub\r\nMessage: %s\r\n", (int)(messagecount + 1), telemetry_msg_buffer);
            IoTHubDeviceClient_SendEventAsync(device_handle, message_handle, send_confirm_callback, NULL);

            // The message is copied to the sdk so the we can destroy it
            IoTHubMessage_Destroy(message_handle);
            messagecount = messagecount + 1;

            ThreadAPI_Sleep(g_interval);
        }

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_Destroy(device_handle);
    }
    // Free all the sdk subsystem
    IoTHub_Deinit();

    return 0;
}
