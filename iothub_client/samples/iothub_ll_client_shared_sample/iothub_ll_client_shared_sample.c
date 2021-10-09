// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

// NOTE: There are limitations and alternatives in this SDK's implementation of multiplexing.  See
// ../../../doc/multiplexing_limitations.md for additional details.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
//#define SAMPLE_HTTP

#ifdef SAMPLE_AMQP
#include "iothubtransportamqp.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
#include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
#include "iothubtransporthttp.h"
#endif // SAMPLE_HTTP

#define sizeofarray(x) (sizeof(x)/sizeof(x[0]))

typedef struct DEVICE_CREDENTIAL_TAG
{
    const char* id;
    const char* key;
} DEVICE_CREDENTIAL;

typedef struct DEVICE_STATE_TAG
{
    const DEVICE_CREDENTIAL* credential;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE client_handle;
    size_t messages_received;
} DEVICE_STATE;

static const char* hubName = "<iot hub name>";
static const char* hubSuffix = "azure-devices.net";

static const DEVICE_CREDENTIAL g_deviceCredentials[] = {
    { .id = "<device id 1>", .key = "<device key 1>" },
    { .id = "<device id 2>", .key = "<device key 2>" }
};

#define NUMBER_OF_DEVICES sizeofarray(g_deviceCredentials)

static DEVICE_STATE g_device_states[NUMBER_OF_DEVICES];

static bool g_continueRunning;
static char msgText[1024];
static char propText[1024];
#define MESSAGE_COUNT       5
#define DOWORK_LOOP_NUM     3

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    DEVICE_STATE* device_state = (DEVICE_STATE*)userContextCallback;
    const unsigned char* buffer = NULL;
    size_t size = 0;
    const char* messageId;
    const char* correlationId;

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL)
    {
        messageId = "<null>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL)
    {
        correlationId = "<null>";
    }

    // Increment the counter
    device_state->messages_received++;

    // Message content
    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(message);
    if (contentType == IOTHUBMESSAGE_BYTEARRAY)
    {
        if (IoTHubMessage_GetByteArray(message, &buffer, &size) == IOTHUB_MESSAGE_OK)
        {
            (void)printf("Received Message [%s; %zu]\r\n Message ID: %s\r\n Correlation ID: %s\r\n BINARY Data: <<<%.*s>>> & Size=%d\r\n", 
                device_state->credential->id, device_state->messages_received, messageId, correlationId, (int)size, buffer, (int)size);
        }
        else
        {
            (void)printf("Failed getting the BINARY body of the message received.\r\n");
        }
    }
    else if (contentType == IOTHUBMESSAGE_STRING)
    {
        if ((buffer = (const unsigned char*)IoTHubMessage_GetString(message)) != NULL && (size = strlen((const char*)buffer)) > 0)
        {
            (void)printf("Received Message [%s; %zu]\r\n Message ID: %s\r\n Correlation ID: %s\r\n STRING Data: <<<%.*s>>> & Size=%d\r\n", 
                device_state->credential->id, device_state->messages_received, messageId, correlationId, (int)size, buffer, (int)size);

            // If we receive the work 'quit' then we stop running
        }
        else
        {
            (void)printf("Failed getting the STRING body of the message received.\r\n");
        }
    }
    else
    {
        (void)printf("Failed getting the body of the message received (type %i).\r\n", contentType);
    }

    // Retrieve properties from the message
    const char* property_key = "property_key";
    const char* property_value = IoTHubMessage_GetProperty(message, property_key);
    if (property_value != NULL)
    {
        printf("\r\nMessage Properties:\r\n");
        printf("\tKey: %s Value: %s\r\n", property_key, property_value);
    }
    if (memcmp(buffer, "quit", size) == 0)
    {
        g_continueRunning = false;
    }
    /* Some device specific action code goes here... */
    return IOTHUBMESSAGE_ACCEPTED;
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    DEVICE_STATE* device_state = (DEVICE_STATE*)userContextCallback;
    (void)printf("Confirmation message received from device %s with result = %s\r\n", device_state->credential->id, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    /* Some device specific action code goes here... */
}

static IOTHUB_MESSAGE_HANDLE create_events(const DEVICE_STATE* device_state)
{
    IOTHUB_MESSAGE_HANDLE message_handle;

    srand((unsigned int)time(NULL));
    double avgWindSpeed = 10.0;
    double minTemperature = 20.0;
    double minHumidity = 60.0;
    double temperature = 0;
    double humidity = 0;

    temperature = minTemperature + (rand() % 10);
    humidity = minHumidity +  (rand() % 20);

    (void)sprintf_s(msgText, sizeof(msgText), "{\"deviceId\":\"%s\",\"windSpeed\":%.2f,\"temperature\":%.2f,\"humidity\":%.2f}", device_state->credential->id, avgWindSpeed + ((double)(rand() % 4) + 2.0), temperature, humidity);
    message_handle = IoTHubMessage_CreateFromString(msgText);

    (void)sprintf_s(propText, sizeof(propText), temperature > 28 ? "true" : "false");
    (void)IoTHubMessage_SetProperty(message_handle, "temperatureAlert", propText);

    return message_handle;
}

int main(void)
{
    TRANSPORT_HANDLE transport_handle;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

    for (int i = 0; i < NUMBER_OF_DEVICES; i++)
    {
        g_device_states[i].credential = &g_deviceCredentials[i];
        g_device_states[i].client_handle = NULL;
        g_device_states[i].messages_received = 0;
    }

#ifdef SAMPLE_AMQP
    protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    protocol = HTTP_Protocol;
#endif // SAMPLE_HTTP

    g_continueRunning = true;

    (void)printf("Starting the IoTHub client shared sample.  Send `quit` message to either device to close...\r\n");

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    if ((transport_handle = IoTHubTransport_Create(protocol, hubName, hubSuffix)) == NULL)
    {
        printf("Failed to creating the protocol handle.\r\n");
    }
    else
    {
        bool setup_ok = true;

        for (int i = 0; i < NUMBER_OF_DEVICES && setup_ok; i++)
        {
            IOTHUB_CLIENT_DEVICE_CONFIG config = { 0 };
            config.deviceId = g_device_states[i].credential->id;
            config.deviceKey = g_device_states[i].credential->key;
            config.deviceSasToken = NULL;
            config.protocol = protocol;
            config.transportHandle = IoTHubTransport_GetLLTransport(transport_handle);

            if ((g_device_states[i].client_handle = IoTHubDeviceClient_LL_CreateWithTransport(&config)) == NULL)
            {
                (void)printf("ERROR: iotHubClientHandle is NULL (%s)!\r\n", config.deviceId);
                setup_ok = false;
            }
            else
            {            
                bool traceOn = true;
                
    #ifdef SAMPLE_HTTP
                unsigned int timeout = 241000;
                // Because it can poll "after 9 seconds" polls will happen effectively // at ~10 seconds.
                // Note that for scalability, the default value of minimumPollingTime
                // is 25 minutes. For more information, see:
                // https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging
                unsigned int minimumPollingTime = 9;
    #endif
                
                if (IoTHubDeviceClient_LL_SetOption(g_device_states[i].client_handle, OPTION_LOG_TRACE, &traceOn) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("Failed setting log tracing (%s)!\r\n", config.deviceId);
                    setup_ok = false;
                }
    #ifdef SET_TRUSTED_CERT_IN_SAMPLES
                // Setting the Trusted Certificate. This is only necessary on systems without
                // built in certificate stores.
                else if (IoTHubDeviceClient_LL_SetOption(g_device_states[i].client_handle, OPTION_TRUSTED_CERT, certificates) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("Failed setting log tracing (%s)!\r\n", config.deviceId);
                    setup_ok = false;
                }
    #endif // SET_TRUSTED_CERT_IN_SAMPLES

    #ifdef SAMPLE_HTTP
                else if (IoTHubDeviceClient_LL_SetOption(g_device_states[i].client_handle, OPTION_MIN_POLLING_TIME, &minimumPollingTime) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("Failed setting log tracing (%s)!\r\n", config.deviceId);
                    setup_ok = false;
                }
                else if (IoTHubDeviceClient_LL_SetOption(g_device_states[i].client_handle, OPTION_HTTP_TIMEOUT, &timeout) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("Failed setting log tracing (%s)!\r\n", config.deviceId);
                    setup_ok = false;
                }
    #endif // SAMPLE_HTTP
                /* Setting Message call back, so we can receive Commands. */
                else if (IoTHubDeviceClient_LL_SetMessageCallback(g_device_states[i].client_handle, ReceiveMessageCallback, &g_device_states[i]) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("Failed setting cloud-to-device message callback (%s)!\r\n", config.deviceId);
                    setup_ok = false;
                }
            }
        }

        if (setup_ok)
        {
            /* Now that we are ready to receive commands, let's send some messages */
            size_t messages_sent = 0;
            size_t send_frequency_ms = 3000;
            size_t sleep_ms = 100;
            size_t counter = send_frequency_ms;
            IOTHUB_MESSAGE_HANDLE message_handle;
            do
            {
                if (counter >= send_frequency_ms && messages_sent < MESSAGE_COUNT)
                {
                    for (int i = 0; i < NUMBER_OF_DEVICES; i++)
                    {
                        // Create the event hub message
                        message_handle = create_events(&g_device_states[i]);
                        (void)IoTHubDeviceClient_LL_SendEventAsync(g_device_states[i].client_handle, message_handle, SendConfirmationCallback, &g_device_states[i]);
                        // The message is copied to the sdk so the we can destroy it
                        IoTHubMessage_Destroy(message_handle);
                    }                    

                    messages_sent++;
                    counter = 0;
                }

                for (int i = 0; i < NUMBER_OF_DEVICES; i++)
                {
                    IoTHubDeviceClient_LL_DoWork(g_device_states[i].client_handle);
                }
                ThreadAPI_Sleep((unsigned int)sleep_ms);
                counter += sleep_ms;

                g_continueRunning = !(messages_sent > MESSAGE_COUNT);
            } while (g_continueRunning);

            (void)printf("client_amqp_shared_sample has received a quit message, call DoWork %d more time to complete final sending...\r\n", DOWORK_LOOP_NUM);
            for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
            {
                for (int i = 0; i < NUMBER_OF_DEVICES; i++)
                {
                    IoTHubDeviceClient_LL_DoWork(g_device_states[i].client_handle);
                }
                ThreadAPI_Sleep(1);
            }
        }

        // Clean up the iothub sdk handle
        for (int i = 0; i < NUMBER_OF_DEVICES; i++)
        {
            IoTHubDeviceClient_LL_Destroy(g_device_states[i].client_handle);
        }

        IoTHubTransport_Destroy(transport_handle);

        // Free all the sdk subsystem
        IoTHub_Deinit();
    }
    return 0;
}
