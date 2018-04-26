// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"

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

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/* Paste in the your iothub connection string  */
static const char* connectionString = "[device connection string]";
#define MESSAGE_COUNT        5
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get envoked
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %zu with result %s\r\n", g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_MESSAGE_HANDLE message_handle;
    size_t messages_sent = 0;
    const char* telemetry_msg = "test_message";

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

    IOTHUB_CLIENT_LL_HANDLE iothub_ll_handle;

    // Used to initialize IoTHub SDK subsystem
    (void)platform_init();

    (void)printf("Creating IoTHub handle\r\n");
    // Create the iothub handle here
    iothub_ll_handle = IoTHubClient_LL_CreateFromConnectionString(connectionString, protocol);

    // Set any option that are neccessary.
    // For available options please see the iothub_sdk_options.md documentation

    //bool traceOn = true;
    //IoTHubClient_LL_SetOption(iothub_ll_handle, OPTION_LOG_TRACE, &traceOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on system with without
    // built in certificate stores.
    IoTHubClient_LL_SetOption(iothub_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_WS
    //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
    //you are URL Encoding inputs yourself.
    //ONLY valid for use with MQTT
    //bool urlEncodeOn = true;
    //IoTHubClient_LL_SetOption(iothub_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif

    do
    {
        if (messages_sent < MESSAGE_COUNT)
        {
            // Construct the iothub message from a string or a byte array
            message_handle = IoTHubMessage_CreateFromString(telemetry_msg);
            //message_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText)));

            // Set Message property
            (void)IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
            (void)IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
            (void)IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application%2fjson");
            (void)IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");

            // Add custom properties to message
            MAP_HANDLE propMap = IoTHubMessage_Properties(message_handle);
            Map_AddOrUpdate(propMap, "property_key", "property_value");

            (void)printf("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
            IoTHubClient_LL_SendEventAsync(iothub_ll_handle, message_handle, send_confirm_callback, NULL);

            // The message is copied to the sdk so the we can destroy it
            IoTHubMessage_Destroy(message_handle);

            messages_sent++;
        }
        else if (g_message_count_send_confirmations >= MESSAGE_COUNT)
        {
            // After all messages are all received stop running
            g_continueRunning = false;
        }

        IoTHubClient_LL_DoWork(iothub_ll_handle);
        ThreadAPI_Sleep(1);

    } while (g_continueRunning);

    // Clean up the iothub sdk handle
    IoTHubClient_LL_Destroy(iothub_ll_handle);

    // Free all the sdk subsystem
    platform_deinit();

    printf("Press any key to continue");
    getchar();

    return 0;
}
