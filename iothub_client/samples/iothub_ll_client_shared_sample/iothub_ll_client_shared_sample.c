// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

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
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
#include "iothubtransportmqtt_websockets.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
#include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
#include "iothubtransporthttp.h"
#endif // SAMPLE_HTTP

static const char* hubName = "[IoT Hub Name]";
static const char* hubSuffix = "[IoT Hub Suffix]";
static const char* deviceId1 = "[device id 1]";
static const char* deviceId2 = "[device id 2]";
static const char* deviceKey1 = "[device key 1]";
static const char* deviceKey2 = "[device key 2]";

//static int callbackCounter;
static bool g_continueRunning;
static char msgText[1024];
static char propText[1024];
#define MESSAGE_COUNT       5
#define DOWORK_LOOP_NUM     3

typedef struct EVENT_INSTANCE_TAG
{
    const char* deviceId;
} EVENT_INSTANCE;

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    int* counter = (int*)userContextCallback;
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
    *counter = (*counter) + 1;

    // Message content
    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(message);
    if (contentType == IOTHUBMESSAGE_BYTEARRAY)
    {
        if (IoTHubMessage_GetByteArray(message, &buffer, &size) == IOTHUB_MESSAGE_OK)
        {
            (void)printf("Received Message [%d]\r\n Message ID: %s\r\n Correlation ID: %s\r\n BINARY Data: <<<%.*s>>> & Size=%d\r\n", *counter, messageId, correlationId, (int)size, buffer, (int)size);
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
            (void)printf("Received Message [%d]\r\n Message ID: %s\r\n Correlation ID: %s\r\n STRING Data: <<<%.*s>>> & Size=%d\r\n", *counter, messageId, correlationId, (int)size, buffer, (int)size);

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
    EVENT_INSTANCE* event_info = (EVENT_INSTANCE*)userContextCallback;
    (void)printf("Confirmation message received from device %s with result = %s\r\n", event_info->deviceId, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    /* Some device specific action code goes here... */
}

static IOTHUB_MESSAGE_HANDLE create_events(const EVENT_INSTANCE* event_info)
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

    (void)sprintf_s(msgText, sizeof(msgText), "{\"deviceId\":\"%s\",\"windSpeed\":%.2f,\"temperature\":%.2f,\"humidity\":%.2f}", event_info->deviceId, avgWindSpeed + ((double)(rand() % 4) + 2.0), temperature, humidity);
    message_handle = IoTHubMessage_CreateFromString(msgText);
    //message_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText))) == NULL)

    (void)sprintf_s(propText, sizeof(propText), temperature > 28 ? "true" : "false");
    (void)IoTHubMessage_SetProperty(message_handle, "temperatureAlert", propText);

    return message_handle;
}

int main(void)
{
    TRANSPORT_HANDLE transport_handle;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle1;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle2;

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

    //callbackCounter = 0;
    int receiveContext1 = 0;
    int receiveContext2 = 0;

    (void)printf("Starting the IoTHub client shared sample.  Send `quit` message to either device to close...\r\n");

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    if ((transport_handle = IoTHubTransport_Create(protocol, hubName, hubSuffix)) == NULL)
    {
        printf("Failed to creating the protocol handle.\r\n");
    }
    else
    {
        EVENT_INSTANCE device1_event;
        EVENT_INSTANCE device2_event;

        device1_event.deviceId = deviceId1;

        IOTHUB_CLIENT_DEVICE_CONFIG config1 = { 0 };
        config1.deviceId = deviceId1;
        config1.deviceKey = deviceKey1;
        config1.deviceSasToken = NULL;
        config1.protocol = protocol;
        config1.transportHandle = IoTHubTransport_GetLLTransport(transport_handle);

        device2_event.deviceId = deviceId2;

        IOTHUB_CLIENT_DEVICE_CONFIG config2 = { 0 };
        config2.deviceId = deviceId2;
        config2.deviceKey = deviceKey2;
        config2.deviceSasToken = NULL;
        config2.protocol = protocol;
        config2.transportHandle = IoTHubTransport_GetLLTransport(transport_handle);

        if ((device_ll_handle1 = IoTHubDeviceClient_LL_CreateWithTransport(&config1)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle1 is NULL!\r\n");
        }
        else if ((device_ll_handle2 = IoTHubDeviceClient_LL_CreateWithTransport(&config2)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle1 is NULL!\r\n");
        }
        else
        {
            // Set any option that are neccessary.
            // For available options please see the iothub_sdk_options.md documentation
            //bool traceOn = true;
            //IoTHubDeviceClient_LL_SetOption(device_ll_handle1, OPTION_LOG_TRACE, &traceOn);
            //IoTHubDeviceClient_LL_SetOption(device_ll_handle2, OPTION_LOG_TRACE, &traceOn);
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
            // Setting the Trusted Certificate. This is only necessary on systems without
            // built in certificate stores.
            IoTHubDeviceClient_LL_SetOption(device_ll_handle1, OPTION_TRUSTED_CERT, certificates);
            IoTHubDeviceClient_LL_SetOption(device_ll_handle2, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#ifdef SAMPLE_HTTP
            unsigned int timeout = 241000;
            // Because it can poll "after 9 seconds" polls will happen effectively // at ~10 seconds.
            // Note that for scalabilty, the default value of minimumPollingTime
            // is 25 minutes. For more information, see:
            // https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging
            unsigned int minimumPollingTime = 9;
            IoTHubDeviceClient_LL_SetOption(device_ll_handle1, OPTION_MIN_POLLING_TIME, &minimumPollingTime);
            IoTHubDeviceClient_LL_SetOption(device_ll_handle1, OPTION_HTTP_TIMEOUT, &timeout);
            IoTHubDeviceClient_LL_SetOption(device_ll_handle2, OPTION_MIN_POLLING_TIME, &minimumPollingTime);
            IoTHubDeviceClient_LL_SetOption(device_ll_handle2, OPTION_HTTP_TIMEOUT, &timeout);
#endif // SAMPLE_HTTP

            /* Setting Message call back, so we can receive Commands. */
            (void)IoTHubDeviceClient_LL_SetMessageCallback(device_ll_handle1, ReceiveMessageCallback, &receiveContext1);
            (void)IoTHubDeviceClient_LL_SetMessageCallback(device_ll_handle2, ReceiveMessageCallback, &receiveContext2);

            /* Now that we are ready to receive commands, let's send some messages */
            size_t messages_sent = 0;
            IOTHUB_MESSAGE_HANDLE message_handle;
            do
            {
                if (messages_sent < MESSAGE_COUNT)
                {
                    // Create the event hub message
                    message_handle = create_events(&device1_event);
                    (void)IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle1, message_handle, SendConfirmationCallback, &device1_event);
                    // The message is copied to the sdk so the we can destroy it
                    IoTHubMessage_Destroy(message_handle);

                    message_handle = create_events(&device2_event);
                    (void)IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle2, message_handle, SendConfirmationCallback, &device2_event);
                    // The message is copied to the sdk so the we can destroy it
                    IoTHubMessage_Destroy(message_handle);

                    messages_sent++;
                }

                IoTHubDeviceClient_LL_DoWork(device_ll_handle1);
                IoTHubDeviceClient_LL_DoWork(device_ll_handle2);
                ThreadAPI_Sleep(1);
            } while (g_continueRunning);

            (void)printf("client_amqp_shared_sample has gotten quit message, call DoWork %d more time to complete final sending...\r\n", DOWORK_LOOP_NUM);
            for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
            {
                IoTHubDeviceClient_LL_DoWork(device_ll_handle1);
                IoTHubDeviceClient_LL_DoWork(device_ll_handle2);
                ThreadAPI_Sleep(1);
            }

            // Clean up the iothub sdk handle
            IoTHubDeviceClient_LL_Destroy(device_ll_handle1);
            IoTHubDeviceClient_LL_Destroy(device_ll_handle2);
        }
        IoTHubTransport_Destroy(transport_handle);
        // Free all the sdk subsystem
        IoTHub_Deinit();
    }
    return 0;
}
