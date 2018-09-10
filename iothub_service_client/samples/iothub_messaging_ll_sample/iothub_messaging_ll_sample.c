// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/map.h"     // Needed for message properties
#include "azure_c_shared_utility/singlylinkedlist.h" // Needed for feedback info
#include "azure_c_shared_utility/threadapi.h"

#include "iothub_service_client_auth.h"
#include "iothub_messaging_ll.h"
#include "iothub_message.h"


/* Paste in the your iothub connection string  */
static const char* connectionString = "[device connection string]";
static const char* deviceId = "[Device Id of receiving device]";

static const char* MESSAGE_TO_DEVICE = "[Message to be sent to Device]";
#define MESSAGE_COUNT 5
static size_t g_feedbackCount = 0;

static void openCompleteCallback(void* context)
{
    (void)printf("Open completed, context: %s\n", (const char*)context);
}

static void sendCompleteCallback(void* context, IOTHUB_MESSAGING_RESULT messagingResult)
{
    (void)context;
    if (messagingResult == IOTHUB_MESSAGING_OK)
    {
        (void)printf("Message has been sent successfully\n");
    }
    else
    {
        (void)printf("Send failed\n");
    }
    g_feedbackCount++;
}

static void feedbackReceivedCallback(void* context, IOTHUB_SERVICE_FEEDBACK_BATCH* feedbackBatch)
{
    printf("Feedback received, context: %s\n", (const char*)context);

    if (feedbackBatch != NULL)
    {
        (void)printf("Batch userId       : %s\r\n", feedbackBatch->userId);
        (void)printf("Batch lockToken    : %s\r\n", feedbackBatch->lockToken);
        if (feedbackBatch->feedbackRecordList != NULL)
        {
            LIST_ITEM_HANDLE feedbackRecord = singlylinkedlist_get_head_item(feedbackBatch->feedbackRecordList);
            while (feedbackRecord != NULL)
            {
                IOTHUB_SERVICE_FEEDBACK_RECORD* feedback = (IOTHUB_SERVICE_FEEDBACK_RECORD*)singlylinkedlist_item_get_value(feedbackRecord);
                if (feedback != NULL)
                {
                    (void)printf("Feedback\r\n");
                    (void)printf("    deviceId        : %s\r\n", feedback->deviceId);
                    (void)printf("    generationId    : %s\r\n", feedback->generationId);
                    (void)printf("    correlationId   : %s\r\n", feedback->correlationId);
                    (void)printf("    description     : %s\r\n", feedback->description);
                    (void)printf("    enqueuedTimeUtc : %s\r\n", feedback->enqueuedTimeUtc);

                    feedbackRecord = singlylinkedlist_get_next_item(feedbackRecord);
                }
            }
        }
    }
}

int main(void)
{
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle;
    IOTHUB_MESSAGING_HANDLE iotHubMessagingHandle;
    IOTHUB_MESSAGING_RESULT iotHubMessagingResult;

    (void)platform_init();

    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    if (iotHubServiceClientHandle == NULL)
    {
        (void)printf("IoTHubServiceClientAuth_CreateFromConnectionString failed\n");
    }
    else
    {
        (void)printf("Creating Messaging object...\n");
        iotHubMessagingHandle = IoTHubMessaging_LL_Create(iotHubServiceClientHandle);
        if (iotHubMessagingHandle == NULL)
        {
            (void)printf("IoTHubMessaging_LL_Create failed\n");
        }
        else
        {
            iotHubMessagingResult = IoTHubMessaging_LL_SetFeedbackMessageCallback(iotHubMessagingHandle, feedbackReceivedCallback, "Context string for feedback");

            // Open the messaging object
            iotHubMessagingResult = IoTHubMessaging_LL_Open(iotHubMessagingHandle, openCompleteCallback, "Context string for open");

            for (int i = 0; i < MESSAGE_COUNT; i++)
            {
                // Create the message and send it to the device.  Send binary date with the
                // IoTHubMessage_CreateFromByteArray(binary data, length of binary data);
                IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(MESSAGE_TO_DEVICE);
                if (messageHandle == NULL)
                {
                    (void)printf("IoTHubMessage_CreateFromByteArray failed\n");
                    break;
                }
                else
                {
                    const char* MSG_ID = "Sample_MessageId";
                    const char* MSG_CORRELATION_ID = "Sample_MessageCorrelationId";
                    const char* MSG_PROP_KEYS[3] = { "Sample_Key1", "Sample_Key2", "Sample_Key3" };
                    const char* MSG_PROP_VALS[3] = { "Sample_Val1", "Sample_Val2", "Sample_Val3" };

                    (void)IoTHubMessage_SetMessageId(messageHandle, MSG_ID);
                    (void)IoTHubMessage_SetCorrelationId(messageHandle, MSG_CORRELATION_ID);

                    MAP_HANDLE mapHandle = IoTHubMessage_Properties(messageHandle);
                    for (size_t index = 0; index < 3; index++)
                    {
                        (void)Map_AddOrUpdate(mapHandle, MSG_PROP_KEYS[index], MSG_PROP_VALS[index]);
                    }

                    iotHubMessagingResult = IoTHubMessaging_LL_Send(iotHubMessagingHandle, deviceId, messageHandle, sendCompleteCallback, NULL);
                    if (iotHubMessagingResult != IOTHUB_MESSAGING_OK)
                    {
                        (void)printf("IoTHubMessaging_LL_Send failed\n");
                    }
                    else
                    {
                        (void)printf("IoTHubMessaging_LL_Send accepted data for transmission to IoT Hub.\r\n");
                    }
                    IoTHubMessage_Destroy(messageHandle);
                }
            }

            g_feedbackCount = 0;
            while (g_feedbackCount < MESSAGE_COUNT)
            {
                IoTHubMessaging_LL_DoWork(iotHubMessagingHandle);
                ThreadAPI_Sleep(10);
            }

            /* Wait for user to press a key. */
            (void)printf("Press any key to exit the application. \r\n");
            (void)getchar();

            IoTHubMessaging_LL_Close(iotHubMessagingHandle);
        }

        (void)printf("Calling IoTHubMessaging_LL_Destroy...\n");
        IoTHubMessaging_LL_Destroy(iotHubMessagingHandle);

        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
    }
    platform_deinit();
}
