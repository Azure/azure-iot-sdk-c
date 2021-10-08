// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "iothub_module_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothubtransportmqtt.h"
#include "iothub.h"

typedef struct FILTERED_MESSAGE_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId;  // For tracking the messages within the user callback.
}
FILTERED_MESSAGE_INSTANCE;

size_t messagesReceivedByInput1Queue = 0;

// Prints relevant system properties about a message.
static void PrintMessageInformation(IOTHUB_MESSAGE_HANDLE message)
{
    // Well defined properties from protocol
    const char* messageId;
    const char* correlationId;
    const char* inputQueueName;
    const char* connectionModuleId;
    const char* connectionDeviceId;
    // Custom, application defined properties.
    const char* tempAlertProperty;

    IOTHUB_MESSAGE_RESULT messageResult;

    (void)printf("Received Message [%lu]\r\n", (unsigned long)messagesReceivedByInput1Queue);


    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(message);
    if (contentType != IOTHUBMESSAGE_BYTEARRAY)
    {
        // The transport will only create messages of type IOTHUBMESSAGE_BYTEARRAY, never IOTHUBMESSAGE_STRING.
        (void)printf("Warning: Unknown content type [%d] received for message.  Cannot display\r\n", (int)contentType);
    }
    else
    {
        unsigned const char* messageBody;
        size_t messageBodyLength;

        // IoTHubMessage_GetByteArray retrieves a shallow copy of the data.  Caller must NOT free messageBody.
        if ((messageResult = IoTHubMessage_GetByteArray(message, &messageBody, &messageBodyLength)) != IOTHUB_MESSAGE_OK)
        {
            (void)printf(" WARNING: messageBody = NULL\r\n");
        }
        else
        {
            (void)printf(" Received Binary message.\r\n  Data: <<<%.*s>>> & Size=%d\r\n", (int)messageBodyLength, messageBody, (int)messageBodyLength);
        }
    }

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL)
    {
        messageId = "<null>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL)
    {
        correlationId = "<null>";
    }

    if ((inputQueueName = IoTHubMessage_GetInputName(message)) == NULL)
    {
        inputQueueName = "<null>";
    }

    if ((connectionModuleId = IoTHubMessage_GetConnectionModuleId(message)) == NULL)
    {
        connectionModuleId = "<null>";
    }

    if ((connectionDeviceId = IoTHubMessage_GetConnectionDeviceId(message)) == NULL)
    {
        connectionDeviceId = "<null>";
    }

    if ((tempAlertProperty = IoTHubMessage_GetProperty(message, "temperatureAlert")) == NULL)
    {
        tempAlertProperty = "<null>";
    }

    (void)printf(" Correlation ID: [%s]\r\n InputQueueName: [%s]\r\n connectionModuleId: [%s]\r\n connectionDeviceId: [%s]\r\n temperatureAlert: [%s]\r\n",
        correlationId, inputQueueName, connectionModuleId, connectionDeviceId, tempAlertProperty);
}

// DefaultMessageCallback is invoked if a message arrives that does not map up to one of the queues
// specified by IoTHubModuleClient_LL_SetInputMessageCallback.
// In the context of this sample, such behavior is unexpected but not an error.
static IOTHUBMESSAGE_DISPOSITION_RESULT DefaultMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    (void)userContextCallback;
    (void)printf("Message arrived sent to the default queue\r\n");
    PrintMessageInformation(message);
    return IOTHUBMESSAGE_ACCEPTED;
}

// SendConfirmationCallbackFromFilter is invoked when the message that was forwarded on from 'InputQueue1FilterCallback'
// pipeline function is confirmed.
// The Azure IoT C SDK allows developers to acknowledge cloud-to-device messages
// (either sending DISPOSITION if using AMQP, or PUBACK if using MQTT) outside the following callback, in a separate function call.
// This would allow the user application to process the incoming message before acknowledging it to the Azure IoT Hub, since
// the callback below is supposed to return as fast as possible to unblock I/O.
// Please look for "IOTHUBMESSAGE_ASYNC_ACK" in iothub_ll_c2d_sample for more details.
static void SendConfirmationCallbackFromFilter(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    // The context corresponds to which message# we were at when we sent.
    FILTERED_MESSAGE_INSTANCE* filteredMessageInstance = (FILTERED_MESSAGE_INSTANCE*)userContextCallback;
    (void)printf("Confirmation[%lu] received for message with result = %d\r\n", (unsigned long)filteredMessageInstance->messageTrackingId, result);
    IoTHubMessage_Destroy(filteredMessageInstance->messageHandle);
    free(filteredMessageInstance);
}

// Allocates a context for callback and clones the message
// NOTE: The message MUST be cloned at this stage.  InputQueue1FilterCallback's caller always frees the message
// so we need to pass down a new copy.
static FILTERED_MESSAGE_INSTANCE* CreateFilteredMessageInstance(IOTHUB_MESSAGE_HANDLE message)
{
    FILTERED_MESSAGE_INSTANCE* filteredMessageInstance = (FILTERED_MESSAGE_INSTANCE*)malloc(sizeof(FILTERED_MESSAGE_INSTANCE));
    if (NULL == filteredMessageInstance)
    {
        (void)printf("Failed allocating 'FILTERED_MESSAGE_INSTANCE' for pipelined message\r\n");
    }
    else
    {
        memset(filteredMessageInstance, 0, sizeof(*filteredMessageInstance));

        if ((filteredMessageInstance->messageHandle = IoTHubMessage_Clone(message)) == NULL)
        {
            (void)printf("Cloning message for pipelined message\r\n");
            free(filteredMessageInstance);
            filteredMessageInstance = NULL;
        }
        filteredMessageInstance->messageTrackingId = messagesReceivedByInput1Queue;
    }

    return filteredMessageInstance;
}

// InputQueue1FilterCallback implements a very primitive filtering mechanism.  It will send the 1st, 3rd, 5th, etc. messages
// to the next step in the queue and will filter out the 2nd, 4th, 6th, etc. messages.
// The Azure IoT C SDK allows developers to acknowledge cloud-to-device messages
// (either sending DISPOSITION if using AMQP, or PUBACK if using MQTT) outside the following callback, in a separate function call.
// This would allow the user application to process the incoming message before acknowledging it to the Azure IoT Hub, since
// the callback below is supposed to return as fast as possible to unblock I/O.
// Please look for "IOTHUBMESSAGE_ASYNC_ACK" in iothub_ll_c2d_sample for more details.
static IOTHUBMESSAGE_DISPOSITION_RESULT InputQueue1FilterCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    IOTHUB_CLIENT_RESULT clientResult;
    IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle = (IOTHUB_MODULE_CLIENT_LL_HANDLE)userContextCallback;

    PrintMessageInformation(message);

    if ((messagesReceivedByInput1Queue % 2) == 0)
    {
        // This message should be sent to next stop in the pipeline, namely "output1".  What happens at "output1" is determined
        // by the configuration of the Edge routing table setup.
        FILTERED_MESSAGE_INSTANCE* filteredMessageInstance = CreateFilteredMessageInstance(message);
        if (NULL == filteredMessageInstance)
        {
            result = IOTHUBMESSAGE_ABANDONED;
        }
        else
        {
            // We filter out every other message.  Here we will send on.
            (void)printf("Sending message (%lu) to the next stage in pipeline\r\n", (unsigned long)messagesReceivedByInput1Queue);

            IOTHUB_MESSAGE_RESULT msgResult = IoTHubMessage_SetMessageId(message, "MSG_ID");
            if (msgResult != IOTHUB_MESSAGE_OK)
            {
                (void)printf("SetMesageId failed, id=%d\r\n", msgResult);
            }

            clientResult = IoTHubModuleClient_LL_SendEventToOutputAsync(iotHubModuleClientHandle, message, "output1", SendConfirmationCallbackFromFilter, (void*)filteredMessageInstance);
            if (clientResult != IOTHUB_CLIENT_OK)
            {
                IoTHubMessage_Destroy(filteredMessageInstance->messageHandle);
                free(filteredMessageInstance);
                (void)printf("IoTHubModuleClient_LL_SendEventToOutputAsync failed on sending msg#=%lu, err=%d\r\n", (unsigned long)messagesReceivedByInput1Queue, clientResult);
                result = IOTHUBMESSAGE_ABANDONED;
            }
            else
            {
                result = IOTHUBMESSAGE_ACCEPTED;
            }
        }
    }
    else
    {
        // No-op.  Swallow this message by not passing it onto the next stage in the pipeline.
        (void)printf("Not sending message (%lu) to the next stage in pipeline.\r\n", (unsigned long)messagesReceivedByInput1Queue);
        result = IOTHUBMESSAGE_ACCEPTED;
    }

    messagesReceivedByInput1Queue++;
    return result;
}

static IOTHUB_MODULE_CLIENT_LL_HANDLE InitializeConnectionForFilter()
{
    IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle;

    if (IoTHub_Init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
        iotHubModuleClientHandle = NULL;
    }
    // Note: You must use MQTT_Protocol as the argument below.  Using other protocols will result in undefined behavior.
    else if ((iotHubModuleClientHandle = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol)) == NULL)
    {
        (void)printf("ERROR: IoTHubModuleClient_LL_CreateFromEnvironment failed\r\n");
    }
    else
    {
        // Uncomment the following lines to enable verbose logging (e.g., for debugging).
        // bool traceOn = true;
        // IoTHubModuleClient_LL_SetOption(iotHubModuleClientHandle, OPTION_LOG_TRACE, &traceOn);
    }

    return iotHubModuleClientHandle;
}

static void DeInitializeConnectionForFilter(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle)
{
    if (iotHubModuleClientHandle != NULL)
    {
        IoTHubModuleClient_LL_Destroy(iotHubModuleClientHandle);
    }
    IoTHub_Deinit();
}

static int SetupCallbacksForInputQueues(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle)
{
    int ret;

    if (IoTHubModuleClient_LL_SetInputMessageCallback(iotHubModuleClientHandle, "input1", InputQueue1FilterCallback, (void*)iotHubModuleClientHandle) != IOTHUB_CLIENT_OK)
    {
        (void)printf("ERROR: IoTHubModuleClient_LL_SetInputMessageCallback(\"input1\")..........FAILED!\r\n");
        ret = MU_FAILURE;
    }
    else if (IoTHubModuleClient_LL_SetMessageCallback(iotHubModuleClientHandle, DefaultMessageCallback, (void*)iotHubModuleClientHandle) != IOTHUB_CLIENT_OK)
    {
        (void)printf("ERROR: IoTHubModuleClient_LL_SetMessageCallback(default)..........FAILED!\r\n");
        ret = MU_FAILURE;
    }
    else
    {
        ret = 0;
    }

    return ret;
}

int main(void)
{
    IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle;

    if ((iotHubModuleClientHandle = InitializeConnectionForFilter()) == NULL)
    {
        ;
    }
    else if (SetupCallbacksForInputQueues(iotHubModuleClientHandle) != 0)
    {
        ;
    }
    else
    {
        // The receiver just loops constantly waiting for messages.
        (void)printf("Waiting for incoming messages.\r\n");
        while (true)
        {
            IoTHubModuleClient_LL_DoWork(iotHubModuleClientHandle);
            ThreadAPI_Sleep(100);
        }
    }

    DeInitializeConnectionForFilter(iotHubModuleClientHandle);
    return 0;
}

