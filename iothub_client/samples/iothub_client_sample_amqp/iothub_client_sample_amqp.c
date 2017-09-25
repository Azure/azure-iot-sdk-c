// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransportamqp.h"

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = "[device connection string]";

static int callbackCounter;
static bool g_continueRunning;
static char msgText[1024];
static char propText[1024];
#define MESSAGE_COUNT       5
#define DOWORK_LOOP_NUM     3

typedef struct EVENT_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    int* counter = (int*)userContextCallback;
    const unsigned char* buffer = NULL;
    size_t size = 0;
    const char* messageId;
    const char* correlationId;
    const char* userDefinedContentType;
    const char* userDefinedContentEncoding;

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL)
    {
        messageId = "<null>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL)
    {
        correlationId = "<null>";
    }

    if ((userDefinedContentType = IoTHubMessage_GetContentTypeSystemProperty(message)) == NULL)
    {
        userDefinedContentType = "<null>";
    }

    if ((userDefinedContentEncoding = IoTHubMessage_GetContentEncodingSystemProperty(message)) == NULL)
    {
        userDefinedContentEncoding = "<null>";
    }

    // Message content
    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(message);

    if (contentType == IOTHUBMESSAGE_BYTEARRAY)
    {
        if (IoTHubMessage_GetByteArray(message, &buffer, &size) == IOTHUB_MESSAGE_OK)
        {
            (void)printf("Received Message [%d]\r\n Message ID: %s\r\n Correlation ID: %s\r\n Content-Type: %s\r\n Content-Encoding: %s\r\n BINARY Data: <<<%.*s>>> & Size=%d\r\n", 
                *counter, messageId, correlationId, userDefinedContentType, userDefinedContentEncoding, (int)size, buffer, (int)size);
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
            (void)printf("Received Message [%d]\r\n Message ID: %s\r\n Correlation ID: %s\r\n Content-Type: %s\r\n Content-Encoding: %s\r\n STRING Data: <<<%.*s>>> & Size=%d\r\n", 
                *counter, messageId, correlationId, userDefinedContentType, userDefinedContentEncoding, (int)size, buffer, (int)size);

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
    MAP_HANDLE mapProperties = IoTHubMessage_Properties(message);
    if (mapProperties != NULL)
    {
        const char*const* keys;
        const char*const* values;
        size_t propertyCount = 0;
        if (Map_GetInternals(mapProperties, &keys, &values, &propertyCount) == MAP_OK)
        {
            if (propertyCount > 0)
            {
                size_t index;

                printf(" Message Properties:\r\n");
                for (index = 0; index < propertyCount; index++)
                {
                    printf("\tKey: %s Value: %s\r\n", keys[index], values[index]);
                }
                printf("\r\n");
            }
        }
    }

    if (size == (strlen("quit") * sizeof(char)) && memcmp(buffer, "quit", size) == 0)
    {
        g_continueRunning = false;
    }

    /* Some device specific action code goes here... */
    (*counter)++;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EVENT_INSTANCE* eventInstance = (EVENT_INSTANCE*)userContextCallback;
    (void)printf("Confirmation[%d] received for message tracking id = %zu with result = %s\r\n", callbackCounter, eventInstance->messageTrackingId, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    /* Some device specific action code goes here... */
    callbackCounter++;
    IoTHubMessage_Destroy(eventInstance->messageHandle);
}

void iothub_client_sample_amqp_run(void)
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    EVENT_INSTANCE messages[MESSAGE_COUNT];

    g_continueRunning = true;
    srand((unsigned int)time(NULL));
    double avgWindSpeed = 10.0;

    callbackCounter = 0;
    int receiveContext = 0;

    (void)printf("Starting the IoTHub client sample AMQP...\r\n");

    if (platform_init() != 0)
    {
        printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, AMQP_Protocol)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            bool traceOn = true;
            IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

            // Set keep alive is optional. If it is not set the default (240 secs) will be used.
            uint32_t c2d_keep_alive_freq_secs = 120;
            IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_C2D_KEEP_ALIVE_FREQ_SECS, &c2d_keep_alive_freq_secs);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
            // For mbed add the certificate information
            if (IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_TRUSTED_CERT, certificates) != IOTHUB_CLIENT_OK)
            {
                printf("failure to set option \"TrustedCerts\"\r\n");
            }
#endif // SET_TRUSTED_CERT_IN_SAMPLES

            /* Setting Message call back, so we can receive Commands. */
            if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, &receiveContext) != IOTHUB_CLIENT_OK)
            {
                (void)printf("ERROR: IoTHubClient_SetMessageCallback..........FAILED!\r\n");
            }
            else
            {
                (void)printf("IoTHubClient_SetMessageCallback...successful.\r\n");

                /* Now that we are ready to receive commands, let's send some messages */
                size_t iterator = 0;
                do
                {
                    if (iterator < MESSAGE_COUNT)
                    {
                        sprintf_s(msgText, sizeof(msgText), "{\"deviceId\":\"myFirstDevice\",\"windSpeed\":%.2f}", avgWindSpeed + (rand() % 4 + 2));
                        if ((messages[iterator].messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText))) == NULL)
                        {
                            (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
                        }
                        else
                        {
                            messages[iterator].messageTrackingId = iterator;

                            MAP_HANDLE propMap = IoTHubMessage_Properties(messages[iterator].messageHandle);
                            (void)sprintf_s(propText, sizeof(propText), "PropMsg_%zu", iterator);
                            if (Map_AddOrUpdate(propMap, "PropName", propText) != MAP_OK)
                            {
                                (void)printf("ERROR: Map_AddOrUpdate Failed!\r\n");
                            }

                            // Setting messages with the same UUID values just for example.
                            (void)IoTHubMessage_SetMessageId(messages[iterator].messageHandle, "dec14a98-c5fc-430e-b4e3-33c1c434dcaf");
                            (void)IoTHubMessage_SetCorrelationId(messages[iterator].messageHandle, "33c1c434dcaf-c5fc-430e-b4e3-dec14a98");

                            (void)IoTHubMessage_SetContentTypeSystemProperty(messages[iterator].messageHandle, "application/json");
                            (void)IoTHubMessage_SetContentEncodingSystemProperty(messages[iterator].messageHandle, "utf-8");

                            if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messages[iterator].messageHandle, SendConfirmationCallback, &messages[iterator]) != IOTHUB_CLIENT_OK)
                            {
                                (void)printf("ERROR: IoTHubClient_SendEventAsync..........FAILED!\r\n");
                            }
                            else
                            {
                                (void)printf("IoTHubClient_SendEventAsync accepted data for transmission to IoT Hub.\r\n");
                            }
                        }
                    }
                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                    ThreadAPI_Sleep(1);

                    iterator++;
                } while (g_continueRunning);

                (void)printf("iothub_client_sample_amqp has gotten quit message, call DoWork %d more time to complete final sending...\r\n", DOWORK_LOOP_NUM);
                for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
                {
                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                    ThreadAPI_Sleep(1);
                }
            }
            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }
}

int main(void)
{
    iothub_client_sample_amqp_run();
    return 0;
}
