// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "testrunnerswitcher.h"

#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransporthttp.h"
#include "iothubtransport.h"
#include "iothub_messaging.h"

#include "iothub_account.h"
#include "iothubtest.h"

#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"

static TEST_MUTEX_HANDLE g_dllByDll;
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
#define IOTHUB_TIMEOUT_SEC           1000
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
    IOTHUB_MESSAGE_HANDLE msgHandle_fault;

    EXPECTED_SEND_DATA* sendData = EventData_Create();
    ASSERT_IS_NOT_NULL_WITH_MSG(sendData, "Failure creating data to be sent");

    // Send the Event
    IOTHUB_CLIENT_RESULT result;
    // Create the IoT Hub Data
    iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, HTTP_Protocol);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubClientHandle, "Failure creating IothubClient handle");

    msgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)sendData->expectedString, strlen(sendData->expectedString));
    ASSERT_IS_NOT_NULL_WITH_MSG(msgHandle, "Failure to create message handle");

    msgHandle_fault = IoTHubMessage_CreateFromByteArray((const unsigned char*)sendData->expectedString, strlen(sendData->expectedString));
    ASSERT_IS_NOT_NULL_WITH_MSG(msgHandle_fault, "Failure to create message handle");

    MAP_HANDLE msgMapHandle = IoTHubMessage_Properties(msgHandle_fault);

    MAP_RESULT propUpdateResult = Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationType", "KillTcp");
    ASSERT_ARE_EQUAL_WITH_MSG(MAP_RESULT, MAP_OK, propUpdateResult, "Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!\r\n");

    propUpdateResult = Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationCloseReason", "boom");
    ASSERT_ARE_EQUAL_WITH_MSG(MAP_RESULT, MAP_OK, propUpdateResult, "Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!\r\n");

    propUpdateResult = Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationDelayInSecs", "1");
    ASSERT_ARE_EQUAL_WITH_MSG(MAP_RESULT, MAP_OK, propUpdateResult, "Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!\r\n");

    if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
    }

    // act
    result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle, ReceiveConfirmationCallback, sendData);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure calling IoTHubClient_SendEventAsync");

    result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle_fault, ReceiveConfirmationCallback, sendData);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure sending fault control message");

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

        result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle_fault, ReceiveConfirmationCallback, sendData);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure sending fault control message");
    }

    if (Lock(sendData->lock) != LOCK_OK)
    {
        ASSERT_FAIL("unable to lock");
    }
    else
    {
        ASSERT_IS_TRUE_WITH_MSG(sendData->dataWasRecv, "Failure sending data to IotHub"); // was found is written by the callback...
        (void)Unlock(sendData->lock);
    }

    result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle_fault, ReceiveConfirmationCallback, sendData);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure sending fault control message");

    ThreadAPI_Sleep(5000);

    result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle, ReceiveConfirmationCallback, sendData);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure calling IoTHubClient_SendEventAsync");

    beginOperation, nowTime;
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
        ASSERT_IS_TRUE_WITH_MSG(sendData->dataWasRecv, "Failure sending data to IotHub"); // was found is written by the callback...
        (void)Unlock(sendData->lock);
    }

    IOTHUB_TEST_HANDLE iotHubTestHandle = IoTHubTest_Initialize(IoTHubAccount_GetEventHubConnectionString(g_iothubAcctInfo1), IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo1), deviceToUse->deviceId, IoTHubAccount_GetEventhubListenName(g_iothubAcctInfo1), IoTHubAccount_GetEventhubAccessKey(g_iothubAcctInfo1), IoTHubAccount_GetSharedAccessSignature(g_iothubAcctInfo1), IoTHubAccount_GetEventhubConsumerGroup(g_iothubAcctInfo1));
    ASSERT_IS_NOT_NULL(iotHubTestHandle);

    result = IoTHubTest_ListenForEventForMaxDrainTime(iotHubTestHandle, IoTHubCallback, IoTHubAccount_GetIoTHubPartitionCount(g_iothubAcctInfo1), sendData);
    ASSERT_ARE_EQUAL(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_OK, result);

    IoTHubTest_Deinit(iotHubTestHandle);

    // assert
    ASSERT_IS_TRUE_WITH_MSG(sendData->wasFound, "Failure receiving data from eventhub"); // was found is written by the callback...*/

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
    IOTHUB_MESSAGE_HANDLE msgHandle_fault;

    EXPECTED_SEND_DATA* sendData = EventData_Create();
    ASSERT_IS_NOT_NULL_WITH_MSG(sendData, "Failure creating data to be sent");

    // act
    platform_init();

    // Create Service Client
    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo1));
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubServiceClientHandle, "Could not initialize IoTHubServiceClient to send C2D messages to the device");

    iotHubMessagingHandle = IoTHubMessaging_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubMessagingHandle, "Could not initialize IoTHubMessaging to send C2D messages to the device");

    iotHubMessagingResult = IoTHubMessaging_Open(iotHubMessagingHandle, openCompleteCallback, (void*)"Context string for open");
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult);

    // Create user context and message
    receiveUserContext = ReceiveUserContext_Create();
    ASSERT_IS_NOT_NULL_WITH_MSG(receiveUserContext, "Could not create receive user context");

    messageHandle = IoTHubMessage_CreateFromString(MSG_CONTENT1);

    ASSERT_IS_NOT_NULL_WITH_MSG(messageHandle, "Could not create IoTHubMessage to send C2D messages to the device");

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
    ASSERT_ARE_EQUAL_WITH_MSG(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult, "IoTHubMessaging_SendAsync failed, could not send C2D message to the device");

    iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, HTTP_Protocol);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubClientHandle, "Failure creating Iothub Client");

    if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
        IOTHUB_CLIENT_RESULT result;
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
    }

    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, receiveUserContext);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure setting message callback");

    msgHandle_fault = IoTHubMessage_CreateFromByteArray((const unsigned char*)sendData->expectedString, strlen(sendData->expectedString));
    ASSERT_IS_NOT_NULL_WITH_MSG(msgHandle_fault, "Failure to create message handle");

    MAP_HANDLE msgMapHandle = IoTHubMessage_Properties(msgHandle_fault);

    MAP_RESULT propUpdateResult = Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationType", "KillTcp");
    ASSERT_ARE_EQUAL_WITH_MSG(MAP_RESULT, MAP_OK, propUpdateResult, "Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!\r\n");

    propUpdateResult = Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationCloseReason", "boom");
    ASSERT_ARE_EQUAL_WITH_MSG(MAP_RESULT, MAP_OK, propUpdateResult, "Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!\r\n");

    propUpdateResult = Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationDelayInSecs", "1");
    ASSERT_ARE_EQUAL_WITH_MSG(MAP_RESULT, MAP_OK, propUpdateResult, "Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!\r\n");

    unsigned int minimumPollingTime = 1; /*because it should not wait*/
    if (IoTHubClient_SetOption(iotHubClientHandle, OPTION_MIN_POLLING_TIME, &minimumPollingTime) != IOTHUB_CLIENT_OK)
    {
        printf("failure to set option \"MinimumPollingTime\"\r\n");
    }

    result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle_fault, ReceiveConfirmationCallback, sendData);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure sending fault control message");

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

        result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle_fault, ReceiveConfirmationCallback, sendData);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Failure sending fault control message");
    }

    // assert
    ASSERT_IS_TRUE_WITH_MSG(receiveUserContext->wasFound, "Failure retrieving message that was sent to IotHub."); // was found is written by the callback...

    // cleanup
    IoTHubMessage_Destroy(messageHandle);
    IoTHubMessaging_Close(iotHubMessagingHandle);
    IoTHubMessaging_Destroy(iotHubMessagingHandle);
    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);

    IoTHubClient_Destroy(iotHubClientHandle);
    ReceiveUserContext_Destroy(receiveUserContext);
}

BEGIN_TEST_SUITE(iothubclient_http_e2e_svc_fault_ctrl)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    platform_init();
    g_iothubAcctInfo1 = IoTHubAccount_Init();
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo1);
    g_iothubAcctInfo2 = IoTHubAccount_Init();
    ASSERT_IS_NOT_NULL_WITH_MSG(g_iothubAcctInfo2, "Failure to init 2nd IoTHubAccount information.");
    g_iothubAcctInfo3 = IoTHubAccount_Init();
    ASSERT_IS_NOT_NULL_WITH_MSG(g_iothubAcctInfo3, "Failure to init 3rd IoTHubAccount information.");
    platform_init();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    IoTHubAccount_deinit(g_iothubAcctInfo3);
    IoTHubAccount_deinit(g_iothubAcctInfo2);
    IoTHubAccount_deinit(g_iothubAcctInfo1);
    platform_deinit();
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    g_iotHubTestId++;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(IoTHub_HTTP_SendEvent_e2e_svc_fault_ctrl)
{
    SendEvent(IoTHubAccount_GetSASDevice(g_iothubAcctInfo1));
}

TEST_FUNCTION(IoTHub_HTTP_RecvMessage_e2e_svc_fault_ctrl)
{
    RecvMessage(IoTHubAccount_GetSASDevice(g_iothubAcctInfo1));
}

END_TEST_SUITE(iothubclient_http_e2e_svc_fault_ctrl)
