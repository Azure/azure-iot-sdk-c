// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

// NOTE: THIS IS A TEST SAMPLE TO VALIDATE ENOBUFS ERROR FIX CANDIDATE.

#include <stdio.h>
#include <stdlib.h>

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

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

typedef struct client_context_t_struct
{
    int id;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE handle;
    size_t messages_sent;
    size_t send_confirmations;
}
client_context_t;

/* Paste in the your iothub connection string  */
#define NUMBER_OF_CLIENTS 15

static const char* connectionString[NUMBER_OF_CLIENTS] = {
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]",
    "[device connection string]", 
    "[device connection string]", 
    "[device connection string]", 
    "[device connection string]"
};

static client_context_t client_contexts[NUMBER_OF_CLIENTS];

#define MESSAGE_COUNT        500
static bool g_continueRunning = true;

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    client_context_t* client_context = (client_context_t*)userContextCallback;
    client_context->send_confirmations++;
    (void)printf("send_confirm_callback(%d, %lu, %s)\r\n", client_context->id, (unsigned long)client_context->send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));

    if (client_context->send_confirmations >= MESSAGE_COUNT)
    {
        g_continueRunning = false;
    }
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    client_context_t* client_context = (client_context_t*)user_context;

    printf("connection_status_callback(%d, %s, %s)\r\n", client_context->id,
        MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result),
        MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_MESSAGE_HANDLE message_handle;
    int loop_count = 0;
    char telemetry_msg[1024];
    memset(telemetry_msg, 2, sizeof(telemetry_msg));
    telemetry_msg[1023] = 0;

    message_handle = IoTHubMessage_CreateFromString(telemetry_msg);

    // Select the Protocol to use with the connection
    protocol = MQTT_Protocol;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    (void)printf("Creating IoTHub Device handles\r\n");

    for (int i = 0; i < NUMBER_OF_CLIENTS; i++ )
    {
        client_contexts[i].id = i;
        client_contexts[i].handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString[i], protocol);

        if (client_contexts[i].handle == NULL)
        {
            (void)printf("Failure creating IotHub device %d. Hint: Check your connection string.\r\n", i);
        }
        else
        {
            bool traceOn = true;
            IoTHubDeviceClient_LL_SetOption(client_contexts[i].handle, OPTION_LOG_TRACE, &traceOn);

    #ifdef SET_TRUSTED_CERT_IN_SAMPLES
            // Setting the Trusted Certificate. This is only necessary on systems without
            // built in certificate stores.
                IoTHubDeviceClient_LL_SetOption(client_contexts[i].handle, OPTION_TRUSTED_CERT, certificates);
    #endif // SET_TRUSTED_CERT_IN_SAMPLES

    #if defined SAMPLE_MQTT || defined SAMPLE_MQTT_OVER_WEBSOCKETS
            //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
            //you are URL Encoding inputs yourself.
            //ONLY valid for use with MQTT
            bool urlEncodeOn = true;
            (void)IoTHubDeviceClient_LL_SetOption(client_contexts[i].handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
    #endif

            // Setting connection status callback to get indication of connection to iothub
            (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(client_contexts[i].handle, connection_status_callback, &client_contexts[i]);
        }
    }
    
    do
    {
        for (int i = 0; i < NUMBER_OF_CLIENTS; i++ )
        {
            if (loop_count % 1000)
            {
                (void)printf("Client %d sending message %ld\r\n", client_contexts[i].id, client_contexts[i].messages_sent);
                IoTHubDeviceClient_LL_SendEventAsync(client_contexts[i].handle, message_handle, send_confirm_callback, &client_contexts[i]);
                client_contexts[i].messages_sent++;
            }

            IoTHubDeviceClient_LL_DoWork(client_contexts[i].handle);
            ThreadAPI_Sleep(1);
            loop_count++;
        }
    } while (g_continueRunning);

    for (int i = 0; i < NUMBER_OF_CLIENTS; i++ )
    {
        printf("client [%d]: sent=%ld, confirmed=%ld\r\n",
            client_contexts[i].id, client_contexts[i].messages_sent, client_contexts[i].send_confirmations);
        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(client_contexts[i].handle);
    }

    // Free all the sdk subsystem
    IoTHub_Deinit();
    return 0;
}
