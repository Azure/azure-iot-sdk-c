// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

#include <stdio.h>
#include <stdlib.h>

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include <resolv.h> // res_init()

/* This sample uses the _LL APIs of iothub_client for example purposes.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
#include "iothubtransportmqtt.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
#include "iothubtransportmqtt_websockets.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
#include "iothubtransportamqp.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
#include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
#include "iothubtransporthttp.h"
#endif // SAMPLE_HTTP

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/* Paste in the your x509 iothub connection string  */
/*  "HostName=<host_name>;DeviceId=<device_id>;x509=true"                      */
static const char *connectionString = "[device connection string]";

static const char *x509certificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICpDCCAYwCCQCfIjBnPxs5TzANBgkqhkiG9w0BAQsFADAUMRIwEAYDVQQDDAls\n"
    "b2NhbGhvc3QwHhcNMTYwNjIyMjM0MzI3WhcNMTYwNjIzMjM0MzI3WjAUMRIwEAYD\n"
    "...\n"
    "+s88wBF907s1dcY45vsG0ldE3f7Y6anGF60nUwYao/fN/eb5FT5EHANVMmnK8zZ2\n"
    "tjWUt5TFnAveFoQWIoIbtzlTbOxUFwMrQFzFXOrZoDJmHNWc2u6FmVAkowoOSHiE\n"
    "dkyVdoGPCXc=\n"
    "-----END CERTIFICATE-----";

static const char *x509privatekey =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpQIBAAKCAQEA0zKK+Uu5I0nXq2V6+2gbdCsBXZ6j1uAgU/clsCohEAek1T8v\n"
    "qj2tR9Mz9iy9RtXPMHwzcQ7aXDaz7RbHdw7tYXqSw8iq0Mxq2s3p4mo6gd5vEOiN\n"
    "...\n"
    "EyePNmkCgYEAng+12qvs0de7OhkTjX9FLxluLWxfN2vbtQCWXslLCG+Es/ZzGlNF\n"
    "SaqVID4EAUgUqFDw0UO6SKLT+HyFjOr5qdHkfAmRzwE/0RBN69g2qLDN3Km1Px/k\n"
    "xyJyxc700uV1eKiCdRLRuCbUeecOSZreh8YRIQQXoG8uotO5IttdVRc=\n"
    "-----END RSA PRIVATE KEY-----";

static bool g_continueRunning = true;

typedef struct EVENT_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId; // For tracking the messages within the user callback.
} EVENT_INSTANCE;

static int iothub_message_callbacks_received = 0;
static int iothub_messages_sent = 0;

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get invoked
    iothub_message_callbacks_received++;
    (void)printf("Confirmation callback received for message %d with result %s\r\n", iothub_message_callbacks_received, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

bool azure_iot_connected = false;
static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                       void *user_context)
{
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        if (!azure_iot_connected)
        {
            azure_iot_connected = true;
            printf("\t** iothub connected **\n");
        }
    }
    else
    {
        if (azure_iot_connected)
        {
            azure_iot_connected = false;
            printf("\t** iothub disconnected **\n");
        }
    }
}

static void reportedStateCallback(int status_code, void *userContextCallback)
{
    (void)userContextCallback;
    printf("Device Twin reported properties update completed with result: %d", status_code);
}

bool iothub_queue_limit_reached()
{
    const int MAX_OUTSTANDING_MESSAGES = 5;

    int outstanding_messages = iothub_messages_sent - iothub_message_callbacks_received;
    if (outstanding_messages >= MAX_OUTSTANDING_MESSAGES)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool send_packet(IOTHUB_DEVICE_CLIENT_LL_HANDLE * device_ll_handle)
{
    static int packet_number = 0;

    char packet[5000] = "a";

    packet_number++;


    for(int i = 0; i < 4220; i++)
    {
        strncat(packet, "a", 1);
    }

    for(int i = 0; i < packet_number; i++)
    {
        strncat(packet, "b", 1);
    }

    printf("created packet with %zd bytes\n", strlen(packet));


    if (azure_iot_connected && !iothub_queue_limit_reached())
    {
        printf("sending packet to azure\n");

        IOTHUB_MESSAGE_HANDLE message_handle;
        message_handle = IoTHubMessage_CreateFromString(packet);

        IoTHubDeviceClient_LL_SendEventAsync(*device_ll_handle, message_handle, send_confirm_callback, NULL);
        iothub_messages_sent++;

        // The message is copied to the sdk so the we can destroy it
        IoTHubMessage_Destroy(message_handle);

        IoTHubDeviceClient_LL_DoWork(*device_ll_handle);
    }
    else
    {
        printf("not sending packet to azure, queue full\n");
    }

    return true;
}

// sleep N milliseconds, but call iothub dowork at the recommended interval during this sleep
void iothub_sleep(unsigned int milliseconds, IOTHUB_DEVICE_CLIENT_LL_HANDLE *device_ll_handle)
{
    IoTHubDeviceClient_LL_DoWork(*device_ll_handle);

    const int IOTHUB_DOWORK_CALL_FREQUENCY_MS = 100;
    while (milliseconds > IOTHUB_DOWORK_CALL_FREQUENCY_MS)
    {
        milliseconds -= IOTHUB_DOWORK_CALL_FREQUENCY_MS;
        ThreadAPI_Sleep(IOTHUB_DOWORK_CALL_FREQUENCY_MS);
        IoTHubDeviceClient_LL_DoWork(*device_ll_handle);
    }
    ThreadAPI_Sleep(milliseconds);
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

    const int LOOP_ITERATIONS_BETWEEN_PACKETS = 20;
    int loop_iterations_until_sending_packet = LOOP_ITERATIONS_BETWEEN_PACKETS;

    // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
    protocol = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    protocol = HTTP_Protocol;
#endif // SAMPLE_HTTP

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    (void)printf("Creating IoTHub handle\r\n");
    // Create the iothub handle here
    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (device_ll_handle == NULL)
    {
        (void)printf("Failure createing Iothub device.  Hint: Check your connection string.\r\n");
    }
    else
    {
        // Set any option that are neccessary.
        // For available options please see the iothub_sdk_options.md documentation
        //bool traceOn = true;
        //IoTHubClient_LL_SetOption(iothub_ll_handle, OPTION_LOG_TRACE, &traceOn);
        // Setting the Trusted Certificate.  This is only necessary on system with without
        // built in certificate stores.
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

        IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL);

        // Set the X509 certificates in the SDK
        if (
                (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_X509_CERT, x509certificate) != IOTHUB_CLIENT_OK) ||
                (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_X509_PRIVATE_KEY, x509privatekey) != IOTHUB_CLIENT_OK))
        {
            printf("failure to set options for x509, aborting\r\n");
        }
        else
        {
            do
            {
                if (loop_iterations_until_sending_packet <= 0)
                {
                    loop_iterations_until_sending_packet = LOOP_ITERATIONS_BETWEEN_PACKETS;
                    if (!send_packet(&device_ll_handle))
                    {
                        printf("send_packet() failed\n");
                    }
                }

                loop_iterations_until_sending_packet--;
                iothub_sleep(1000, &device_ll_handle);

            } while (g_continueRunning);
        }
        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    }
    // Free all the sdk subsystem
    IoTHub_Deinit();

    printf("Enter a character to continue");
    getchar();

    return 0;
}
