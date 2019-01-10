// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "testrunnerswitcher.h"

#include "iothub.h"
#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransporthttp.h"
#include "internal/iothubtransport.h"
#include "iothub_messaging.h"

#include "iothub_account.h"
#include "iothubtest.h"

#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/threadapi.h"

static bool g_callbackRecv = false;

const char* TEST_EVENT_DATA_FMT = "{\"data\":\"%.24s\",\"id\":\"%d\"}";

const char* MSG_ID1 = "MessageIdForE2E_1";
const char* MSG_ID2 = "MessageIdForE2E_2";
const char* MSG_CORRELATION_ID1 = "MessageCorrelationIdForE2E_1";
const char* MSG_CORRELATION_ID2 = "MessageCorrelationIdForE2E_2";
const char* MSG_CONTENT1 = "Message content for E2E_1";
const char* MSG_CONTENT2 = "Message content for E2E_2";

#define MSG_PROP_COUNT 3
const char* MSG_PROP_KEYS[MSG_PROP_COUNT] = { "Key1", "Key2", "Key3" };
const char* MSG_PROP_VALS[MSG_PROP_COUNT] = { "Val1", "Val2", "Val3" };

static size_t g_iotHubTestId = 0;
static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo1 = NULL;
static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo2 = NULL;
static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo3 = NULL;

#define IOTHUB_COUNTER_MAX           10
#define MAX_CLOUD_TRAVEL_TIME        60.0

TEST_DEFINE_ENUM_TYPE(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES);

typedef struct EXPECTED_SEND_DATA_TAG
{
    const char* expectedString;
    bool wasFound;
    bool dataWasRecv;
    LOCK_HANDLE lock;
} EXPECTED_SEND_DATA;

typedef struct EXPECTED_RECEIVE_DATA_TAG
{
    bool wasFound;
    LOCK_HANDLE lock; /*needed to protect this structure*/
} EXPECTED_RECEIVE_DATA;

static size_t IoTHub_HTTP_LL_CanSend_2000_smallest_messages_batched_nCalls;

static void openCompleteCallback(void* context)
{
    (void)printf("Open completed, context: %s\n", (char*)context);
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
}

static void IoTHub_HTTP_LL_CanSend_2000_smallest_messages_batched_Message(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)result;
    (void)userContextCallback;
    IoTHub_HTTP_LL_CanSend_2000_smallest_messages_batched_nCalls++;
}

static size_t IoTHub_HTTP_LL_CanSend_2000_smallest_messages_batched_with_properties_nCalls;

static void IoTHub_HTTP_LL_CanSend_2000_smallest_messages_batched_with_properties_Message(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)result;
    (void)userContextCallback;
    IoTHub_HTTP_LL_CanSend_2000_smallest_messages_batched_with_properties_nCalls++;
}

BEGIN_TEST_SUITE(iothubclient_http_e2e)

static int IoTHubCallback(void* context, const char* data, size_t size)
{
    EXPECTED_SEND_DATA* expectedData = (EXPECTED_SEND_DATA*)context;
    int result = 0; // 0 means "keep processing"

    if (expectedData != NULL)
    {
        if (Lock(expectedData->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            if (
                (strlen(expectedData->expectedString) == size) &&
                (memcmp(expectedData->expectedString, data, size) == 0)
                )
            {
                expectedData->wasFound = true;
                result = 1;
            }
            (void)Unlock(expectedData->lock);
        }
    }
    return result;
}

static int IoTHubCallbackMultipleEvents(void* context, const char* data, size_t size)
{
    EXPECTED_SEND_DATA** expectedData = (EXPECTED_SEND_DATA**)context;
    int result = 0; // 0 means "keep processing"
    bool allDataFound = false;

    if (expectedData != NULL)
    {
        allDataFound = true;
        for (size_t i = 0; i < 2; i++)
        {
            EXPECTED_SEND_DATA* expected = expectedData[i];
            if (Lock(expected->lock) != LOCK_OK)
            {
                allDataFound = false;
                ASSERT_FAIL("unable to lock");
            }
            else
            {
                if (
                    (strlen(expected->expectedString) == size) &&
                    (memcmp(expected->expectedString, data, size) == 0)
                    )
                {
                    expected->wasFound = true;
                }

                allDataFound &= expected->wasFound;
                (void)Unlock(expected->lock);
            }
        }
    }

    if (allDataFound == true)
    {
        result = 1;
    }

    return result;
}

static void ReceiveConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EXPECTED_SEND_DATA* expectedData = (EXPECTED_SEND_DATA*)userContextCallback;
    (void)result;
    if (expectedData != NULL)
    {
        if (Lock(expectedData->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            expectedData->dataWasRecv = true;
            (void)Unlock(expectedData->lock);
        }
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE messageHandle, void* userContextCallback)
{
    EXPECTED_RECEIVE_DATA* receiveUserContext = (EXPECTED_RECEIVE_DATA*)userContextCallback;
    if (receiveUserContext == NULL)
    {
        ASSERT_FAIL("User context is NULL");
    }
    else
    {
        if (Lock(receiveUserContext->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Unable to lock"); /*because the test must absolutely be terminated*/
        }
        else
        {
            const char* messageId;
            const char* correlationId;
            const unsigned char* content;
            size_t contentSize;

            if ((messageId = IoTHubMessage_GetMessageId(messageHandle)) == NULL)
            {
                ASSERT_FAIL("Message ID is NULL.");
            }
            if ((strcmp(messageId, MSG_ID1) != 0) && (strcmp(messageId, MSG_ID2) != 0))
            {
                ASSERT_FAIL("Message ID mismatch.");
            }

            if ((correlationId = IoTHubMessage_GetCorrelationId(messageHandle)) == NULL)
            {
                ASSERT_FAIL("Message correlation ID is NULL.");
            }
            if ((strcmp(correlationId, MSG_CORRELATION_ID1) != 0) && (strcmp(correlationId, MSG_CORRELATION_ID2) != 0))
            {
                ASSERT_FAIL("Message correlation ID mismatch.");
            }

            IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(messageHandle);
            if (contentType != IOTHUBMESSAGE_BYTEARRAY)
            {
                ASSERT_FAIL("Message content type mismatch.");
            }
            else
            {
                if (IoTHubMessage_GetByteArray(messageHandle, &content, &contentSize) != IOTHUB_MESSAGE_OK)
                {
                    ASSERT_FAIL("Unable to get the content of the message.");
                }
                else
                {
                    (void)printf("Received new message from IoT Hub :\r\nMessage-id: %s\r\nCorrelation-id: %s\r\n", messageId, correlationId);
                    (void)printf("\r\n");
                }

                receiveUserContext->wasFound = true;
                MAP_HANDLE mapHandle = IoTHubMessage_Properties(messageHandle);
                if (mapHandle != NULL)
                {
                    const char*const* keys;
                    const char*const* values;
                    size_t propertyCount = 0;
                    if (Map_GetInternals(mapHandle, &keys, &values, &propertyCount) == MAP_OK)
                    {
                        receiveUserContext->wasFound = true;
                        if (propertyCount == MSG_PROP_COUNT)
                        {
                            (void)printf("Message Properties:\r\n");
                            for (size_t index = 0; index < propertyCount; index++)
                            {
                                (void)printf("\tKey: %s Value: %s\r\n", keys[index], values[index]);
                                if (strcmp(keys[index], MSG_PROP_KEYS[index]) != 0)
                                {
                                    receiveUserContext->wasFound = false;
                                }
                                if (strcmp(values[index], MSG_PROP_VALS[index]) != 0)
                                {
                                    receiveUserContext->wasFound = false;
                                }
                            }
                            (void)printf("\r\n");
                        }
                        else
                        {
                            receiveUserContext->wasFound = false;
                        }
                    }
                }
            }
            Unlock(receiveUserContext->lock);
        }
    }
    return IOTHUBMESSAGE_ACCEPTED;
}

static EXPECTED_RECEIVE_DATA* ReceiveUserContext_Create(void)
{
    EXPECTED_RECEIVE_DATA* result = (EXPECTED_RECEIVE_DATA*)malloc(sizeof(EXPECTED_RECEIVE_DATA));
    if (result != NULL)
    {
        result->wasFound = false;
        if ((result->lock = Lock_Init()) == NULL)
        {
            free(result);
            result = NULL;
        }
    }
    return result;
}

static void ReceiveUserContext_Destroy(EXPECTED_RECEIVE_DATA* data)
{
    if (data != NULL)
    {
        (void)Lock_Deinit(data->lock);

        free(data);
    }
}

static EXPECTED_SEND_DATA* EventData_Create(void)
{
    EXPECTED_SEND_DATA* result = (EXPECTED_SEND_DATA*)malloc(sizeof(EXPECTED_SEND_DATA));
    if (result != NULL)
    {
        if ((result->lock = Lock_Init()) == NULL)
        {
            ASSERT_FAIL("unable to Lock_Init");
        }
        else
        {
            char temp[1000];
            char* tempString;
            time_t t = time(NULL);
            sprintf(temp, TEST_EVENT_DATA_FMT, ctime(&t), g_iotHubTestId);
            if ((tempString = (char*)malloc(strlen(temp) + 1)) == NULL)
            {
                Lock_Deinit(result->lock);
                free(result);
                result = NULL;
            }
            else
            {
                strcpy(tempString, temp);
                result->expectedString = tempString;
                result->wasFound = false;
                result->dataWasRecv = false;
            }
        }
    }
    return result;
}

static void EventData_Destroy(EXPECTED_SEND_DATA* data)
{
    if (data != NULL)
    {
        (void)Lock_Deinit(data->lock);
        if (data->expectedString != NULL)
        {
            free((void*)data->expectedString);
        }
        free(data);
    }
}

static void SendEvent(IOTHUB_PROVISIONED_DEVICE* deviceToUse)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    IOTHUB_MESSAGE_HANDLE msgHandle;

    EXPECTED_SEND_DATA* sendData = EventData_Create();
    ASSERT_IS_NOT_NULL(sendData, "Failure creating data to be sent");

    // Send the Event
    {
        IOTHUB_CLIENT_RESULT result;
        // Create the IoT Hub Data
        iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, HTTP_Protocol);
        ASSERT_IS_NOT_NULL(iotHubClientHandle, "Failure creating IothubClient handle");

        msgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)sendData->expectedString, strlen(sendData->expectedString));
        ASSERT_IS_NOT_NULL(msgHandle, "Failure to create message handle");

        if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
            result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
            result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
        }

        // act
        result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle, ReceiveConfirmationCallback, sendData);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure calling IoTHubClient_SendEventAsync");
    }

    time_t beginOperation, nowTime;
    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(sendData->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            if (sendData->dataWasRecv)
            {
                Unlock(sendData->lock);
                break;
            }
            Unlock(sendData->lock);
        }
        ThreadAPI_Sleep(100);
    }

    if (Lock(sendData->lock) != LOCK_OK)
    {
        ASSERT_FAIL("unable to lock");
    }
    else
    {
        ASSERT_IS_TRUE(sendData->dataWasRecv, "Failure sending data to IotHub"); // was found is written by the callback...
        (void)Unlock(sendData->lock);
    }

    {
        IOTHUB_TEST_HANDLE iotHubTestHandle = IoTHubTest_Initialize(IoTHubAccount_GetEventHubConnectionString(g_iothubAcctInfo1), IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo1), deviceToUse->deviceId, IoTHubAccount_GetEventhubListenName(g_iothubAcctInfo1), IoTHubAccount_GetEventhubAccessKey(g_iothubAcctInfo1), IoTHubAccount_GetSharedAccessSignature(g_iothubAcctInfo1), IoTHubAccount_GetEventhubConsumerGroup(g_iothubAcctInfo1));
        ASSERT_IS_NOT_NULL(iotHubTestHandle);

        IOTHUB_TEST_CLIENT_RESULT result = IoTHubTest_ListenForEventForMaxDrainTime(iotHubTestHandle, IoTHubCallback, IoTHubAccount_GetIoTHubPartitionCount(g_iothubAcctInfo1), sendData);
        ASSERT_ARE_EQUAL(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_OK, result);

        IoTHubTest_Deinit(iotHubTestHandle);
    }

    // assert
    ASSERT_IS_TRUE(sendData->wasFound, "Failure receiving data from eventhub"); // was found is written by the callback...*/

                                                                                         // cleanup
    IoTHubMessage_Destroy(msgHandle);

    IoTHubClient_Destroy(iotHubClientHandle);
    EventData_Destroy(sendData);
}

static void RecvMessage(IOTHUB_PROVISIONED_DEVICE* deviceToUse)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;

    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle;
    IOTHUB_MESSAGING_CLIENT_HANDLE iotHubMessagingHandle;
    IOTHUB_MESSAGING_RESULT iotHubMessagingResult;
    IOTHUB_MESSAGE_RESULT iotHubMessageResult;

    EXPECTED_RECEIVE_DATA* receiveUserContext;
    IOTHUB_MESSAGE_HANDLE messageHandle;

    // act
    IoTHub_Init();

    // Create Service Client
    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo1));
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "Could not initialize IoTHubServiceClient to send C2D messages to the device");

    iotHubMessagingHandle = IoTHubMessaging_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL(iotHubMessagingHandle, "Could not initialize IoTHubMessaging to send C2D messages to the device");

    iotHubMessagingResult = IoTHubMessaging_Open(iotHubMessagingHandle, openCompleteCallback, (void*)"Context string for open");
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult);

    // Create user context and message
    receiveUserContext = ReceiveUserContext_Create();
    ASSERT_IS_NOT_NULL(receiveUserContext, "Could not create receive user context");

    messageHandle = IoTHubMessage_CreateFromString(MSG_CONTENT1);

    ASSERT_IS_NOT_NULL(messageHandle, "Could not create IoTHubMessage to send C2D messages to the device");

    iotHubMessageResult = IoTHubMessage_SetMessageId(messageHandle, MSG_ID1);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessageResult);

    iotHubMessageResult = IoTHubMessage_SetCorrelationId(messageHandle, MSG_CORRELATION_ID1);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessageResult);

    MAP_HANDLE mapHandle = IoTHubMessage_Properties(messageHandle);
    for (size_t i = 0; i < MSG_PROP_COUNT; i++)
    {
        if (Map_AddOrUpdate(mapHandle, MSG_PROP_KEYS[i], MSG_PROP_VALS[i]) != MAP_OK)
        {
            (void)printf("ERROR: Map_AddOrUpdate failed for property %zu!\r\n", i);
        }
    }

    iotHubMessagingResult = IoTHubMessaging_SendAsync(iotHubMessagingHandle, deviceToUse->deviceId, messageHandle, sendCompleteCallback, receiveUserContext);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult, "IoTHubMessaging_SendAsync failed, could not send C2D message to the device");
    iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, HTTP_Protocol);
    ASSERT_IS_NOT_NULL(iotHubClientHandle, "Failure creating Iothub Client");

    if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
        IOTHUB_CLIENT_RESULT result;
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
    }

    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, receiveUserContext);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure setting message callback");

    unsigned int minimumPollingTime = 1; /*because it should not wait*/
    if (IoTHubClient_SetOption(iotHubClientHandle, OPTION_MIN_POLLING_TIME, &minimumPollingTime) != IOTHUB_CLIENT_OK)
    {
        printf("failure to set option \"MinimumPollingTime\"\r\n");
    }

    time_t beginOperation, nowTime;
    beginOperation = time(NULL);
    while (
        (
        (nowTime = time(NULL)),
            (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) //time box
            )
        )
    {
        if (Lock(receiveUserContext->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable ot lock");
        }
        else
        {
            if (receiveUserContext->wasFound)
            {
                (void)Unlock(receiveUserContext->lock);
                break;
            }
            (void)Unlock(receiveUserContext->lock);
        }
        ThreadAPI_Sleep(100);
    }

    // assert
    ASSERT_IS_TRUE(receiveUserContext->wasFound, "Failure retrieving message that was sent to IotHub."); // was found is written by the callback...

    // cleanup
    IoTHubMessage_Destroy(messageHandle);
    IoTHubMessaging_Close(iotHubMessagingHandle);
    IoTHubMessaging_Destroy(iotHubMessagingHandle);
    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);

    IoTHubClient_Destroy(iotHubClientHandle);
    ReceiveUserContext_Destroy(receiveUserContext);
}

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    IoTHub_Init();
    g_iothubAcctInfo1 = IoTHubAccount_Init(false);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo1);
    g_iothubAcctInfo2 = IoTHubAccount_Init(false);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo2, "Failure to init 2nd IoTHubAccount information.");
    g_iothubAcctInfo3 = IoTHubAccount_Init(false);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo3, "Failure to init 3rd IoTHubAccount information.");
    IoTHub_Init();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    IoTHubAccount_deinit(g_iothubAcctInfo3);
    IoTHubAccount_deinit(g_iothubAcctInfo2);
    IoTHubAccount_deinit(g_iothubAcctInfo1);
    IoTHub_Deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    g_iotHubTestId++;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(IoTHub_HTTP_SendEvent_e2e_sas)
{
    SendEvent(IoTHubAccount_GetSASDevice(g_iothubAcctInfo1));
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_HTTP_SendEvent_e2e_x509)
{
    SendEvent(IoTHubAccount_GetX509Device(g_iothubAcctInfo1));
}
#endif

TEST_FUNCTION(IoTHub_HTTP_SendEvent_Shared_e2e)
{
    // arrange

    TRANSPORT_HANDLE     transportHandle;


    IOTHUB_CLIENT_CONFIG iotHubConfig1 = { 0 };
    IOTHUB_CLIENT_CONFIG iotHubConfig2 = { 0 };
    IOTHUB_CLIENT_HANDLE iotHubClientHandle1;
    IOTHUB_CLIENT_HANDLE iotHubClientHandle2;
    IOTHUB_MESSAGE_HANDLE msgHandle1;
    IOTHUB_MESSAGE_HANDLE msgHandle2;

    iotHubConfig1.iotHubName = IoTHubAccount_GetIoTHubName(g_iothubAcctInfo3);
    iotHubConfig1.iotHubSuffix = IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo3);
    iotHubConfig1.deviceId = IoTHubAccount_GetSASDevice(g_iothubAcctInfo3)->deviceId;
    iotHubConfig1.deviceKey = IoTHubAccount_GetSASDevice(g_iothubAcctInfo3)->primaryAuthentication;
    iotHubConfig1.protocol = HTTP_Protocol;

    iotHubConfig2.iotHubName = IoTHubAccount_GetIoTHubName(g_iothubAcctInfo2);
    iotHubConfig2.iotHubSuffix = IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo2);
    iotHubConfig2.deviceId = IoTHubAccount_GetSASDevice(g_iothubAcctInfo2)->deviceId;
    iotHubConfig2.deviceKey = IoTHubAccount_GetSASDevice(g_iothubAcctInfo2)->primaryAuthentication;
    iotHubConfig2.protocol = HTTP_Protocol;

    EXPECTED_SEND_DATA* sendData1 = EventData_Create();
    EXPECTED_SEND_DATA* sendData2 = EventData_Create();
    ASSERT_IS_NOT_NULL(sendData1, "Failure creating set 1 data to be sent");
    ASSERT_IS_NOT_NULL(sendData2, "Failure creating set 2 data to be sent");

    // Create the transport
    {
        transportHandle = IoTHubTransport_Create(HTTP_Protocol, IoTHubAccount_GetIoTHubName(g_iothubAcctInfo3), IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo3));
        ASSERT_IS_NOT_NULL(transportHandle, "Failure creating transport handle.");
    }

    // Send the Event device 1
    {
        IOTHUB_CLIENT_RESULT result;
        // Create the IoT Hub Data
        iotHubClientHandle1 = IoTHubClient_CreateWithTransport(transportHandle, &iotHubConfig1);
        ASSERT_IS_NOT_NULL(iotHubClientHandle1, "Failure creating IothubClient handle device 1");

        msgHandle1 = IoTHubMessage_CreateFromByteArray((const unsigned char*)sendData1->expectedString, strlen(sendData1->expectedString));
        ASSERT_IS_NOT_NULL(msgHandle1, "Failure to create message handle");

        // act
        result = IoTHubClient_SendEventAsync(iotHubClientHandle1, msgHandle1, ReceiveConfirmationCallback, sendData1);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure calling IoTHubClient_SendEventAsync");
    }

    // Send the Event device 2
    {
        IOTHUB_CLIENT_RESULT result;
        // Create the IoT Hub Data
        iotHubClientHandle2 = IoTHubClient_CreateWithTransport(transportHandle, &iotHubConfig2);
        ASSERT_IS_NOT_NULL(iotHubClientHandle2, "Failure creating IothubClient handle device 2");

        msgHandle2 = IoTHubMessage_CreateFromByteArray((const unsigned char*)sendData2->expectedString, strlen(sendData2->expectedString));
        ASSERT_IS_NOT_NULL(msgHandle2, "Failure to create message handle");

        // act
        result = IoTHubClient_SendEventAsync(iotHubClientHandle2, msgHandle2, ReceiveConfirmationCallback, sendData2);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure calling IoTHubClient_SendEventAsync");
    }

    time_t beginOperation, nowTime;
    beginOperation = time(NULL);
    bool dataWasRecv1 = false;
    bool dataWasRecv2 = false;

    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(sendData1->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock send data 1");
        }
        else
        {
            dataWasRecv1 = sendData1->dataWasRecv; // was found is written by the callback...
            Unlock(sendData1->lock);
        }
        if (Lock(sendData2->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock send data 2");
        }
        else
        {
            dataWasRecv2 = sendData2->dataWasRecv; // was found is written by the callback...
            Unlock(sendData2->lock);
        }

        if (dataWasRecv1 && dataWasRecv2)
        {
            break;
        }
        ThreadAPI_Sleep(100);
    }

    ASSERT_IS_TRUE(dataWasRecv1, "Failure sending data to IotHub, device 1");
    ASSERT_IS_TRUE(dataWasRecv2, "Failure sending data to IotHub, device 2");


    {
        EXPECTED_SEND_DATA* sendList[2];
        sendList[0] = sendData1;
        sendList[1] = sendData2;
        IOTHUB_TEST_HANDLE iotHubTestHandle1 = IoTHubTest_Initialize(IoTHubAccount_GetEventHubConnectionString(g_iothubAcctInfo3), IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo3), IoTHubAccount_GetSASDevice(g_iothubAcctInfo3)->deviceId, IoTHubAccount_GetEventhubListenName(g_iothubAcctInfo1), IoTHubAccount_GetEventhubAccessKey(g_iothubAcctInfo1), IoTHubAccount_GetSharedAccessSignature(g_iothubAcctInfo1), IoTHubAccount_GetEventhubConsumerGroup(g_iothubAcctInfo1));
        ASSERT_IS_NOT_NULL(iotHubTestHandle1);

        IOTHUB_TEST_CLIENT_RESULT result = IoTHubTest_ListenForEventForMaxDrainTime(iotHubTestHandle1, IoTHubCallbackMultipleEvents, IoTHubAccount_GetIoTHubPartitionCount(g_iothubAcctInfo1), sendList);
        ASSERT_ARE_EQUAL(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_OK, result);

        IoTHubTest_Deinit(iotHubTestHandle1);
    }

    // assert
    ASSERT_IS_TRUE(sendData1->wasFound, "Failure receiving client 1 data from eventhub"); // was found is written by the callback...*/
    ASSERT_IS_TRUE(sendData2->wasFound, "Failure receiving client 2 data from eventhub"); // was found is written by the callback...*/

    // cleanup
    IoTHubMessage_Destroy(msgHandle2);
    IoTHubMessage_Destroy(msgHandle1);

    IoTHubClient_Destroy(iotHubClientHandle2);
    IoTHubClient_Destroy(iotHubClientHandle1);
    IoTHubTransport_Destroy(transportHandle);
    EventData_Destroy(sendData2);
    EventData_Destroy(sendData1);

}

TEST_FUNCTION(IoTHub_HTTP_RecvMessage_E2ETest_sas)
{
    RecvMessage(IoTHubAccount_GetSASDevice(g_iothubAcctInfo1));
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_HTTP_RecvMessage_E2ETest_x509)
{
    RecvMessage(IoTHubAccount_GetX509Device(g_iothubAcctInfo1));
}
#endif

TEST_FUNCTION(IoTHub_HTTP_RecvMessage_shared_E2ETest)
{
    // arrange

    TRANSPORT_HANDLE     transportHandle;
    IOTHUB_CLIENT_CONFIG iotHubConfig1 = { 0 };
    IOTHUB_CLIENT_CONFIG iotHubConfig2 = { 0 };
    IOTHUB_CLIENT_HANDLE iotHubClientHandle1;
    IOTHUB_CLIENT_HANDLE iotHubClientHandle2;

    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle1;
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle2;
    IOTHUB_MESSAGING_CLIENT_HANDLE iotHubMessagingHandle1;
    IOTHUB_MESSAGING_CLIENT_HANDLE iotHubMessagingHandle2;
    IOTHUB_MESSAGING_RESULT iotHubMessagingResult;
    IOTHUB_MESSAGE_RESULT iotHubMessageResult;

    EXPECTED_RECEIVE_DATA* receiveUserContext1;
    EXPECTED_RECEIVE_DATA* receiveUserContext2;
    IOTHUB_MESSAGE_HANDLE messageHandle1;
    IOTHUB_MESSAGE_HANDLE messageHandle2;
    MAP_HANDLE mapHandle1;
    MAP_HANDLE mapHandle2;

    // act
    iotHubConfig1.iotHubName = IoTHubAccount_GetIoTHubName(g_iothubAcctInfo3);
    iotHubConfig1.iotHubSuffix = IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo3);
    iotHubConfig1.deviceId = IoTHubAccount_GetSASDevice(g_iothubAcctInfo3)->deviceId;
    iotHubConfig1.deviceKey = IoTHubAccount_GetSASDevice(g_iothubAcctInfo3)->primaryAuthentication;
    iotHubConfig1.protocol = HTTP_Protocol;

    iotHubConfig2.iotHubName = IoTHubAccount_GetIoTHubName(g_iothubAcctInfo2);
    iotHubConfig2.iotHubSuffix = IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo2);
    iotHubConfig2.deviceId = IoTHubAccount_GetSASDevice(g_iothubAcctInfo2)->deviceId;
    iotHubConfig2.deviceKey = IoTHubAccount_GetSASDevice(g_iothubAcctInfo2)->primaryAuthentication;
    iotHubConfig2.protocol = HTTP_Protocol;

    IoTHub_Init();

    // Create the transport
    {
        transportHandle = IoTHubTransport_Create(HTTP_Protocol, IoTHubAccount_GetIoTHubName(g_iothubAcctInfo3), IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo3));
        ASSERT_IS_NOT_NULL(transportHandle, "Failure creating transport handle.");
    }

    // Create Service Client1
    iotHubServiceClientHandle1 = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo3));
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle1, "Could not initialize IoTHubServiceClient to send C2D messages to the device");

    // Create Messaging1
    iotHubMessagingHandle1 = IoTHubMessaging_Create(iotHubServiceClientHandle1);
    ASSERT_IS_NOT_NULL(iotHubMessagingHandle1, "Could not initialize IoTHubMessaging to send C2D messages to the device");

    iotHubMessagingResult = IoTHubMessaging_Open(iotHubMessagingHandle1, openCompleteCallback, (void*)"Context string for open");
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult);

    // Create user context and message
    receiveUserContext1 = ReceiveUserContext_Create();
    ASSERT_IS_NOT_NULL(receiveUserContext1, "Could not create receive user context");

    messageHandle1 = IoTHubMessage_CreateFromString(MSG_CONTENT1);

    ASSERT_IS_NOT_NULL(messageHandle1, "Could not create IoTHubMessage to send C2D messages to the device");

    iotHubMessageResult = IoTHubMessage_SetMessageId(messageHandle1, MSG_ID1);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessageResult);

    iotHubMessageResult = IoTHubMessage_SetCorrelationId(messageHandle1, MSG_CORRELATION_ID1);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessageResult);

    mapHandle1 = IoTHubMessage_Properties(messageHandle1);
    for (size_t i = 0; i < MSG_PROP_COUNT; i++)
    {
        if (Map_AddOrUpdate(mapHandle1, MSG_PROP_KEYS[i], MSG_PROP_VALS[i]) != MAP_OK)
        {
            (void)printf("ERROR: Map_AddOrUpdate failed for property %zu!\r\n", i);
        }
    }

    iotHubMessagingResult = IoTHubMessaging_SendAsync(iotHubMessagingHandle1, iotHubConfig1.deviceId, messageHandle1, sendCompleteCallback, receiveUserContext1);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult, "IoTHubMessaging_SendAsync failed, could not send C2D message to the device");

    // Create Service Client2
    iotHubServiceClientHandle2 = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo2));
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle2, "Could not initialize IoTHubServiceClient to send C2D messages to the device");

    // Create Messaging2
    iotHubMessagingHandle2 = IoTHubMessaging_Create(iotHubServiceClientHandle2);
    ASSERT_IS_NOT_NULL(iotHubMessagingHandle2, "Could not initialize IoTHubMessaging to send C2D messages to the device");

    iotHubMessagingResult = IoTHubMessaging_Open(iotHubMessagingHandle2, openCompleteCallback, (void*)"Context string for open");
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult);

    // Create user context and message
    receiveUserContext2 = ReceiveUserContext_Create();
    ASSERT_IS_NOT_NULL(receiveUserContext2, "Could not create receive user context");

    messageHandle2 = IoTHubMessage_CreateFromString(MSG_CONTENT2);

    ASSERT_IS_NOT_NULL(messageHandle2, "Could not create IoTHubMessage to send C2D messages to the device");

    iotHubMessageResult = IoTHubMessage_SetMessageId(messageHandle2, MSG_ID2);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessageResult);

    iotHubMessageResult = IoTHubMessage_SetCorrelationId(messageHandle2, MSG_CORRELATION_ID2);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessageResult);

    mapHandle2 = IoTHubMessage_Properties(messageHandle2);
    for (size_t i = 0; i < MSG_PROP_COUNT; i++)
    {
        if (Map_AddOrUpdate(mapHandle2, MSG_PROP_KEYS[i], MSG_PROP_VALS[i]) != MAP_OK)
        {
            (void)printf("ERROR: Map_AddOrUpdate failed for property %zu!\r\n", i);
        }
    }

    iotHubMessagingResult = IoTHubMessaging_SendAsync(iotHubMessagingHandle2, iotHubConfig2.deviceId, messageHandle2, sendCompleteCallback, receiveUserContext1);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult, "IoTHubMessaging_SendAsync failed, could not send C2D message to the device");

    // Create Device Client
    iotHubClientHandle1 = IoTHubClient_CreateWithTransport(transportHandle, &iotHubConfig1);
    ASSERT_IS_NOT_NULL(iotHubClientHandle1, "Failure creating Iothub Client, device 1");
    iotHubClientHandle2 = IoTHubClient_CreateWithTransport(transportHandle, &iotHubConfig2);
    ASSERT_IS_NOT_NULL(iotHubClientHandle2, "Failure creating Iothub Client, device 2");

    IOTHUB_CLIENT_RESULT result1 = IoTHubClient_SetMessageCallback(iotHubClientHandle1, ReceiveMessageCallback, receiveUserContext1);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result1, "Failure setting message callback, device 1");
    IOTHUB_CLIENT_RESULT result2 = IoTHubClient_SetMessageCallback(iotHubClientHandle2, ReceiveMessageCallback, receiveUserContext2);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result2, "Failure setting message callback device 2");

    unsigned int minimumPollingTime = 1; /*because it should not wait*/
    if (IoTHubClient_SetOption(iotHubClientHandle1, OPTION_MIN_POLLING_TIME, &minimumPollingTime) != IOTHUB_CLIENT_OK)
    {
        printf("failure to set option \"MinimumPollingTime\"\r\n");
    }
    if (IoTHubClient_SetOption(iotHubClientHandle2, OPTION_MIN_POLLING_TIME, &minimumPollingTime) != IOTHUB_CLIENT_OK)
    {
        printf("failure to set option \"MinimumPollingTime\"\r\n");
    }

    time_t beginOperation, nowTime;
    bool wasFound1 = false;
    bool wasFound2 = false;
    beginOperation = time(NULL);
    while (
        (
            (nowTime = time(NULL)),
            (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) //time box
            )
        )
    {
        if (Lock(receiveUserContext1->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            wasFound1 = receiveUserContext1->wasFound; // was found is written by the callback...
            (void)Unlock(receiveUserContext1->lock);
        }
        if (Lock(receiveUserContext2->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            wasFound2 = receiveUserContext2->wasFound; // was found is written by the callback...
            (void)Unlock(receiveUserContext2->lock);
        }

        if (wasFound1 && wasFound2)
        {
            break;
        }
        ThreadAPI_Sleep(100);
    }

    // assert
    ASSERT_IS_TRUE(wasFound1, "Failure retrieving message from client 1 that was sent to IotHub.");
    ASSERT_IS_TRUE(wasFound2, "Failure retrieving message from client 2 that was sent to IotHub.");

    // cleanup
    IoTHubClient_Destroy(iotHubClientHandle2);
    IoTHubClient_Destroy(iotHubClientHandle1);

    IoTHubMessage_Destroy(messageHandle2);
    ReceiveUserContext_Destroy(receiveUserContext2);

    IoTHubMessaging_Close(iotHubMessagingHandle2); //sync
    IoTHubMessaging_Destroy(iotHubMessagingHandle2);

    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle2);

    IoTHubMessage_Destroy(messageHandle1);
    ReceiveUserContext_Destroy(receiveUserContext1);

    IoTHubMessaging_Close(iotHubMessagingHandle1);
    IoTHubMessaging_Destroy(iotHubMessagingHandle1);

    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle1);

    IoTHubTransport_Destroy(transportHandle);
}

END_TEST_SUITE(iothubclient_http_e2e)
