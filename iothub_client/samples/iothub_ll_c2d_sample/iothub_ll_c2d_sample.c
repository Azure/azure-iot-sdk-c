// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"

#ifdef USE_MQTT
    #include "iothubtransportmqtt.h"
    #ifdef USE_WEBSOCKETS
        #include "iothubtransportmqtt_websockets.h"
    #endif
#endif
#ifdef USE_AMQP
    #include "iothubtransportamqp.h"
    #ifdef USE_WEBSOCKETS
        #include "iothubtransportamqp_websockets.h"
    #endif
#endif
#ifdef USE_HTTP
    #include "iothubtransporthttp.h"
#endif

#include "iothub_client_options.h"
#include "certs.h"

/* String containing Hostname, Device Id & Device Key in the format:                         */
/* Paste in the your iothub connection string  */
static const char* connectionString = "[device connection string]";

#define MESSAGE_COUNT        3
static bool g_continueRunning = true;
static size_t g_message_recv_count = 0;
DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);
DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)user_context;

    IOTHUBMESSAGE_CONTENT_TYPE content_type = IoTHubMessage_GetContentType(message);
    if (content_type == IOTHUBMESSAGE_BYTEARRAY)
    {
        const unsigned char* buff_msg;
        size_t buff_len;

        if (IoTHubMessage_GetByteArray(message, &buff_msg, &buff_len) != IOTHUB_MESSAGE_OK)
        {
            (void)printf("Failure retrieving byte array message\r\n");
        }
    }
    else
    {
        const char* string_msg = IoTHubMessage_GetString(message);
        if (string_msg == NULL)
        {
            (void)printf("Failure retrieving byte array message\r\n");
        }
    }
    g_message_recv_count++;

    return IOTHUBMESSAGE_ACCEPTED;
}

static void iothub_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)user_context;
    (void)printf("iothub_connection_status result: %s reason: %s\r\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result), ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    size_t messages_count = 0;

    // Select the Protocol to use with the connection
#ifdef USE_AMQP
    //protocol = AMQP_Protocol_over_WebSocketsTls;
    protocol = AMQP_Protocol;
#endif
#ifdef USE_MQTT
    //protocol = MQTT_Protocol;
    //protocol = MQTT_WebSocket_Protocol;
#endif
#ifdef USE_HTTP
    //protocol = HTTP_Protocol;
#endif

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
    // Setting the Trusted Certificate.  This is only necessary on system with without
    // built in certificate stores.
    IoTHubClient_LL_SetOption(iothub_ll_handle, OPTION_TRUSTED_CERT, certificates);

    (void)IoTHubClient_LL_SetConnectionStatusCallback(iothub_ll_handle, iothub_connection_status, NULL);

    if (IoTHubClient_LL_SetMessageCallback(iothub_ll_handle, receive_msg_callback, &messages_count) != IOTHUB_CLIENT_OK)
    {
        (void)printf("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
    }
    else
    {
        do
        {
            if (g_message_recv_count >= MESSAGE_COUNT)
            {
                // After all messages are all received stop running
                g_continueRunning = false;
            }

            IoTHubClient_LL_DoWork(iothub_ll_handle);
            ThreadAPI_Sleep(1);

        } while (g_continueRunning);
    }

    // Clean up the iothub sdk handle
    IoTHubClient_LL_Destroy(iothub_ll_handle);

    // Free all the sdk subsystem
    platform_deinit();

    printf("Press any key to continue");
    getchar();

    return 0;
}
