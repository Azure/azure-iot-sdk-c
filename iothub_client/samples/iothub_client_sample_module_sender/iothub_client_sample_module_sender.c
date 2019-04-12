// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "iothub_module_client_ll.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothub.h"

static int callbackCounter;
static char msgText[1024];
static char propText[1024];
#define MESSAGE_COUNT 500
#define DOWORK_LOOP_NUM     60


typedef struct EVENT_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EVENT_INSTANCE* eventInstance = (EVENT_INSTANCE*)userContextCallback;
    (void)printf("Confirmation[%d] received for message tracking id = %lu with result = %s\r\n", callbackCounter, (unsigned long)eventInstance->messageTrackingId, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    /* Some device specific action code goes here... */
    callbackCounter++;
    IoTHubMessage_Destroy(eventInstance->messageHandle);
}

int main(void)
{
    IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle;
    EVENT_INSTANCE messages[MESSAGE_COUNT];

    srand((unsigned int)time(NULL));
    double avgWindSpeed = 10.0;
    double minTemperature = 20.0;
    double minHumidity = 60.0;

    callbackCounter = 0;

    if (IoTHub_Init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
        return 1;
    }
    else if ((iotHubModuleClientHandle = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol)) == NULL)
    {
        (void)printf("ERROR: iotHubModuleClientHandle is NULL!\r\n");
    }
    else
    {
        // Uncomment the following lines to enable verbose logging (e.g., for debugging).
        // bool traceOn = true;
        // IoTHubModuleClient_LL_SetOption(iotHubModuleClientHandle, OPTION_LOG_TRACE, &traceOn);

        size_t iterator = 0;
        double temperature = 0;
        double humidity = 0;
        do
        {
            if (iterator < MESSAGE_COUNT)
            {
                temperature = minTemperature + (rand() % 10);
                humidity = minHumidity +  (rand() % 20);
                sprintf_s(msgText, sizeof(msgText), "{\"deviceId\":\"myFirstDevice\",\"windSpeed\":%.2f,\"temperature\":%.2f,\"humidity\":%.2f}", avgWindSpeed + (rand() % 4 + 2), temperature, humidity);
                if ((messages[iterator].messageHandle = IoTHubMessage_CreateFromString(msgText)) == NULL)
                {
                    (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
                }
                else
                {
                    (void)IoTHubMessage_SetMessageId(messages[iterator].messageHandle, "MSG_ID");
                    (void)IoTHubMessage_SetCorrelationId(messages[iterator].messageHandle, "CORE_ID");

                    messages[iterator].messageTrackingId = iterator;
                    MAP_HANDLE propMap = IoTHubMessage_Properties(messages[iterator].messageHandle);
                    (void)sprintf_s(propText, sizeof(propText), temperature > 28 ? "true" : "false");
                    Map_AddOrUpdate(propMap, "temperatureAlert", propText);

                    if (IoTHubModuleClient_LL_SendEventToOutputAsync(iotHubModuleClientHandle, messages[iterator].messageHandle, "temperatureOutput", SendConfirmationCallback, &messages[iterator]) != IOTHUB_CLIENT_OK)
                    {
                        (void)printf("ERROR: IoTHubModuleClient_LL_SendEventAsync..........FAILED!\r\n");
                    }
                    else
                    {
                        (void)printf("IoTHubModuleClient_LL_SendEventAsync accepted message [%d] for transmission to IoT Hub.\r\n", (int)iterator);
                    }
                }

            }
            IoTHubModuleClient_LL_DoWork(iotHubModuleClientHandle);
            ThreadAPI_Sleep(1000);
            iterator++;
        } while (1);

        // Loop while we wait for messages to drain off.
        size_t index = 0;
        for (index = 0; index < DOWORK_LOOP_NUM; index++)
        {
            IoTHubModuleClient_LL_DoWork(iotHubModuleClientHandle);
            ThreadAPI_Sleep(100);
        }

        IoTHubModuleClient_LL_Destroy(iotHubModuleClientHandle);
    }

    (void)printf("Finished executing\n");
    IoTHub_Deinit();
    return 0;
}

