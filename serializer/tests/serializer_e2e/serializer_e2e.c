// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <ctime>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#endif

#include "azure_macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#include "iothub_account.h"
#include "iothubtest.h"
#include "iothub_messaging.h"

#include "serializer.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/threadapi.h"
#include "iothubtransportamqp.h"
#include "iothubtransporthttp.h"
#include "iothubtransportmqtt.h"
#include "MacroE2EModelAction.h"
#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_devicemethod.h"
#include "parson.h"
#include "methodreturn.h"

#include "azure_c_shared_utility/platform.h"
#include "iothub_service_client_auth.h"
#include "iothub_devicetwin.h"

static TEST_MUTEX_HANDLE g_testByTest;
static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo = NULL;

/*the following time expressed in seconds denotes the maximum time to read all the events available in an event hub*/
#define MAX_DRAIN_TIME 100.0

/*the following time expressed in seconds denotes the maximum "cloud" travel time - the time from a moment some data reaches the cloud until that data is available for a consumer*/
#define MAX_CLOUD_TRAVEL_TIME 60.0
#define MAX_TWIN_CLOUD_TRAVEL_TIME 120.0
#define TIME_DATA_LENGTH    32

TEST_DEFINE_ENUM_TYPE(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(CODEFIRST_RESULT, CODEFIRST_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CODEFIRST_RESULT, CODEFIRST_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_DEVICE_METHOD_RESULT, IOTHUB_DEVICE_METHOD_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_DEVICE_METHOD_RESULT, IOTHUB_DEVICE_METHOD_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_RESULT_VALUES);

static const char* TEST_SEND_DATA_FMT = "{\"ExampleData\": { \"SendDate\": \"%.24s\", \"UniqueId\":%d} }";
static const char* TEST_RECV_DATA_FMT = "{\\\"Name\\\": \\\"testaction\\\", \\\"Parameters\\\": { \\\"property1\\\": \\\"%.24s\\\", \\\"UniqueId\\\":%d}}";
static const char* TEST_CMP_DATA_FMT = "{\"Name\": \"testaction\", \"Parameters\": { \"property1\": \"%.24s\", \"UniqueId\":%d} }";
static const char* TEST_MACRO_CMP_DATA_FMT = "{\"UniqueId\":%d, \"property1\":\"%.24s\"}";
static const char* TEST_MACRO_RECV_DATA_FMT = "{\"Name\":\"dataMacroCallback\", \"Parameters\":{\"property1\":\"%.24s\", \"UniqueId\": %d}}";

static size_t g_uniqueTestId = 0;

typedef struct _tagEXPECTED_SEND_DATA
{
    const char* expectedString;
    bool wasFound;
    bool dataWasSent;
    LOCK_HANDLE lock; /*needed to protect this structure*/
} EXPECTED_SEND_DATA;

typedef struct _tagEXPECTED_RECEIVE_DATA
{
    const char* toBeSend;
    size_t toBeSendSize;
    const char* compareData;
    size_t compareDataSize;
    bool wasFound;
    LOCK_HANDLE lock; /*needed to protect this structure*/
} EXPECTED_RECEIVE_DATA;


static EXPECTED_RECEIVE_DATA* g_recvMacroData;

static LOCK_HANDLE lock; /*this protects the data written in the below callback from the main thread that is polling it*/
static int was_my_IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_called;
void my_IOTHUB_CLIENT_REPORTED_STATE_CALLBACK(int status_code, void* userContextCallback)
{
    (void)(userContextCallback);
    ASSERT_ARE_EQUAL(int, status_code, 204);
    (void)Lock(lock);
    was_my_IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_called = 1;
    Unlock(lock);

}

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

// This is a Static function called from the macro
EXECUTE_COMMAND_RESULT dataMacroCallback(deviceModel* device, ascii_char_ptr property1, int UniqueId)
{
    (void)device;
    if (g_recvMacroData != NULL)
    {
        if (Lock(g_recvMacroData->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            if ( (g_uniqueTestId == (size_t)UniqueId) && (strcmp(g_recvMacroData->compareData, property1) == 0) )
            {
                g_recvMacroData->wasFound = true;
            }
            (void)Unlock(g_recvMacroData->lock);
        }
    }
    return EXECUTE_COMMAND_SUCCESS;
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        ASSERT_FAIL("unable to IoTHubMessage_GetByteArray");
    }
    else
    {
        /*buffer is not zero terminated*/
        char* buffer_string = (char*)malloc(size+1);
        ASSERT_IS_NOT_NULL(buffer_string);

        if (memcpy(buffer_string, buffer, size) == 0)
        {
            ASSERT_FAIL("memcpy failed for buffer");
        }
        else
        {
            buffer_string[size] = '\0';
            EXECUTE_COMMAND(userContextCallback, buffer_string);
        }
        free(buffer_string);
    }
    return IOTHUBMESSAGE_ACCEPTED;
}

METHODRETURN_HANDLE theSumOfThings(deviceModel* device, int a, int b)
{
    (void)(device);
    int a_plus_b = a + b;
    size_t needed = snprintf(NULL, 0, "%u", a_plus_b);
    char* jsonValue = (char*)malloc(needed + 1);
    ASSERT_IS_NOT_NULL(jsonValue);
    (void)snprintf(jsonValue, needed + 1, "%u", a_plus_b);
    METHODRETURN_HANDLE result = MethodReturn_Create(10, jsonValue);
    ASSERT_IS_NOT_NULL(result);
    free(jsonValue);
    return result;
}

/*AT NO TIME, CAN THERE BE 2 TRANSPORTS OVER OPENSSL IN THE SAME TIME.*/
/*If there are, nasty Linux bugs will happen*/
BEGIN_TEST_SUITE(serializer_e2e)

    static void iotHubMacroCallBack(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
    {
        (void)result;
        EXPECTED_SEND_DATA* expectedData = (EXPECTED_SEND_DATA*)userContextCallback;
        if (expectedData != NULL)
        {
            if (Lock(expectedData->lock) != LOCK_OK)
            {
                ASSERT_FAIL("unable to lock");
            }
            else
            {
                if (expectedData != NULL)
                {
                    expectedData->dataWasSent = true;
                }
                (void)Unlock(expectedData->lock);
                //printf("was sent: %s\n", expectedData->expectedString);
            }
        }
    }

    static int IoTHubCallback(void* context, const char* data, size_t size)
    {
        int result = 0; // 0 means "keep processing"
        EXPECTED_SEND_DATA* expectedData = (EXPECTED_SEND_DATA*)context;
        //printf("Received: %*.*s\n", (int)size, (int)size, data);
        if (expectedData != NULL)
        {
            if (Lock(expectedData->lock) != LOCK_OK)
            {
                ASSERT_FAIL("unable to lock");
            }
            else
            {
                if (size != strlen(expectedData->expectedString))
                {
                    result = 0;
                }
                else
                {
                    if (memcmp(expectedData->expectedString, data, size) == 0)
                    {
                        expectedData->wasFound = true;
                        result = 1;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                (void)Unlock(expectedData->lock);
            }
        }
        return result;
    }

    static EXPECTED_RECEIVE_DATA* RecvMacroTestData_Create(void)
    {
        EXPECTED_RECEIVE_DATA* result;
        result = (EXPECTED_RECEIVE_DATA*)malloc(sizeof(EXPECTED_RECEIVE_DATA));
        if (result != NULL)
        {
            if ((result->lock = Lock_Init()) == NULL)
            {
                free(result);
                result = NULL;
            }
            else
            {
                char* tempString;
                result->wasFound = false;

                tempString = (char*)malloc(TIME_DATA_LENGTH);
                if (tempString == NULL)
                {
                    (void)Lock_Deinit(result->lock);
                    free(result);
                    result = NULL;
                }
                else
                {
                    time_t t = time(NULL);
                    (void)sprintf_s(tempString, TIME_DATA_LENGTH, "%.24s", ctime(&t) );
                    result->compareData = tempString;
                    result->compareDataSize = strlen(result->compareData);
                    size_t nLen = result->compareDataSize+strlen(TEST_MACRO_RECV_DATA_FMT)+4;
                    tempString = (char*)malloc(nLen);
                    if (tempString == NULL)
                    {
                        (void)Lock_Deinit(result->lock);
                        free((void*)result->compareData);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        (void)sprintf_s(tempString, nLen, TEST_MACRO_RECV_DATA_FMT, result->compareData, g_uniqueTestId);
                        result->toBeSendSize = strlen(tempString);
                        result->toBeSend = tempString;
                    }
                }
            }
        }
        return result;
    }

    static void RecvTestData_Destroy(EXPECTED_RECEIVE_DATA* data)
    {
        if (data != NULL)
        {
            (void)Lock_Deinit(data->lock);
            if (data->compareData != NULL)
            {
                free((void*)(data->compareData));
            }
            if (data->toBeSend != NULL)
            {
                free((void*)data->toBeSend);
            }
            free(data);
        }
    }

    static EXPECTED_SEND_DATA* SendMacroTestData_Create(const char* pszTime)
    {
        EXPECTED_SEND_DATA* result = (EXPECTED_SEND_DATA*)malloc(sizeof(EXPECTED_SEND_DATA));
        if (result != NULL)
        {
            if ((result->lock = Lock_Init()) == NULL)
            {
                free(result);
                result = NULL;
            }
            else
            {
                char temp[1000];
                char* tempString;
                sprintf(temp, TEST_MACRO_CMP_DATA_FMT, g_uniqueTestId, pszTime);
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
                    result->dataWasSent = false;
                }
            }
        }
        return result;
    }

    static void SendTestData_Destroy(EXPECTED_SEND_DATA* data)
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


    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        ASSERT_ARE_EQUAL(int, 0, platform_init() );
        ASSERT_ARE_EQUAL(int, 0, serializer_init(NULL));

        g_iothubAcctInfo = IoTHubAccount_Init(false);
        ASSERT_IS_NOT_NULL(g_iothubAcctInfo);

        g_uniqueTestId = 0;
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        IoTHubAccount_deinit(g_iothubAcctInfo);

        platform_deinit();
        serializer_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }
        g_uniqueTestId++;
        was_my_IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_called = 0;
        lock = Lock_Init();
        ASSERT_IS_NOT_NULL(lock);
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        Lock_Deinit(lock);
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    static void AMQP_MacroRecv(IOTHUB_PROVISIONED_DEVICE* deviceToUse)
    {
        // arrange
        IOTHUB_CLIENT_HANDLE iotHubClientHandle;
        bool continue_run;

        IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientAuthHandle;
        IOTHUB_MESSAGING_CLIENT_HANDLE iotHubMessagingHandle;
        IOTHUB_MESSAGING_RESULT iotHubMessagingResult;
        IOTHUB_MESSAGE_HANDLE messageHandle;

        // act

        //step 1: data is retrieved by device using AMQP
        time_t beginOperation, nowTime;

        // step 2: data is created
        g_recvMacroData = RecvMacroTestData_Create();
        ASSERT_IS_NOT_NULL(g_recvMacroData);

        // Create message
        messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)g_recvMacroData->toBeSend, g_recvMacroData->toBeSendSize);

        // Create Service Client
        iotHubServiceClientAuthHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
        ASSERT_IS_NOT_NULL(iotHubServiceClientAuthHandle, "Could not initialize IoTHubServiceClient to send C2D messages to the device");

        iotHubMessagingHandle = IoTHubMessaging_Create(iotHubServiceClientAuthHandle);
        ASSERT_IS_NOT_NULL(iotHubMessagingHandle, "Could not initialize IoTHubMessaging to send C2D messages to the device");

        iotHubMessagingResult = IoTHubMessaging_Open(iotHubMessagingHandle, openCompleteCallback, "Context string for open");
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, iotHubMessagingResult);

        iotHubMessagingResult = IoTHubMessaging_SendAsync(iotHubMessagingHandle, deviceToUse->deviceId, messageHandle, sendCompleteCallback, NULL);
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, iotHubMessagingResult, "IoTHubMessaging_SendAsync failed, could not send C2D message to the device");

        iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, AMQP_Protocol);
        ASSERT_IS_NOT_NULL(iotHubClientHandle);

        if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
            IOTHUB_CLIENT_RESULT result;
            result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
            result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
        }

        deviceModel* devModel = CREATE_MODEL_INSTANCE(MacroE2EModelAction, deviceModel);
        ASSERT_IS_NOT_NULL(devModel);

        IOTHUB_CLIENT_RESULT setMessageResult = IoTHubClient_SetMessageCallback(iotHubClientHandle, IoTHubMessage, devModel);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, setMessageResult);

        beginOperation = time(NULL);
        continue_run = true;
        while (
            ((nowTime = time(NULL)),
            (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME)) &&
            continue_run)
        {
            if (Lock(g_recvMacroData->lock) != LOCK_OK)
            {
                ASSERT_FAIL("unable to lock macro not found");
            }
            else
            {
                if (g_recvMacroData->wasFound)
                {
                    continue_run = false;
                }
                (void)Unlock(g_recvMacroData->lock);
            }
            ThreadAPI_Sleep(100);
        }
        // assert
        ASSERT_IS_TRUE(g_recvMacroData->wasFound);

        ///cleanup
        DESTROY_MODEL_INSTANCE(devModel);
        IoTHubClient_Destroy(iotHubClientHandle);
        RecvTestData_Destroy(g_recvMacroData);

        IoTHubMessage_Destroy(messageHandle);
        IoTHubMessaging_Close(iotHubMessagingHandle);
        IoTHubMessaging_Destroy(iotHubMessagingHandle);
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientAuthHandle);
    }

    TEST_FUNCTION(IoTClient_AMQP_MacroRecv_e2e_sas)
    {
        AMQP_MacroRecv(IoTHubAccount_GetSASDevice(g_iothubAcctInfo));
    }

#ifndef __APPLE__
    TEST_FUNCTION(IoTClient_AMQP_MacroRecv_e2e_x509)
    {
        AMQP_MacroRecv(IoTHubAccount_GetX509Device(g_iothubAcctInfo));
    }
#endif

    static void AMQP_MacroSend(IOTHUB_PROVISIONED_DEVICE* deviceToUse)
    {
        // arrange
        IOTHUB_CLIENT_HANDLE iotHubClientHandle;
        deviceModel* devModel;
        time_t beginOperation, nowTime;
        bool continue_run;

        // step 1: prepare data
        time_t t = time(NULL);
        char sztimeText[64];
        (void)sprintf_s(sztimeText, sizeof(sztimeText), "%.24s", ctime(&t));

        EXPECTED_SEND_DATA* expectedData = SendMacroTestData_Create(sztimeText);
        ASSERT_IS_NOT_NULL(expectedData);

        /// act
        // step 2: send data with AMQP
        {
            iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, AMQP_Protocol);
            ASSERT_IS_NOT_NULL(iotHubClientHandle);

            if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
                IOTHUB_CLIENT_RESULT result;
                result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
                ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
                result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
                ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
            }

            devModel = CREATE_MODEL_INSTANCE(MacroE2EModelAction, deviceModel);
            ASSERT_IS_NOT_NULL(devModel);

            devModel->property1 = sztimeText;
            devModel->UniqueId = (int)g_uniqueTestId;
            unsigned char* destination;
            size_t destinationSize;
            CODEFIRST_RESULT nResult = SERIALIZE(&destination, &destinationSize, *devModel);
            ASSERT_ARE_EQUAL(CODEFIRST_RESULT, CODEFIRST_OK, nResult);
            IOTHUB_MESSAGE_HANDLE iothubMessageHandle = IoTHubMessage_CreateFromByteArray(destination, destinationSize);
            ASSERT_IS_NOT_NULL(iothubMessageHandle);
            free(destination);

            IOTHUB_CLIENT_RESULT iothubClientResult = IoTHubClient_SendEventAsync(iotHubClientHandle, iothubMessageHandle, iotHubMacroCallBack, expectedData);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, iothubClientResult);

            IoTHubMessage_Destroy(iothubMessageHandle);
            // Wait til the data gets sent to the callback
            beginOperation = time(NULL);
            continue_run = true;
            while (
                ((nowTime = time(NULL)),
                (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME)) && //time box
                continue_run)
            {
                if (Lock(expectedData->lock) != LOCK_OK)
                {
                    ASSERT_FAIL("unable to lock data was sent");
                }
                else
                {
                    if (expectedData->dataWasSent)
                    {
                        continue_run = false;
                    }
                    (void)Unlock(expectedData->lock);
                }
                ThreadAPI_Sleep(100);
            }
        }
        ASSERT_IS_TRUE(expectedData->dataWasSent);

        ///assert
        //step3: get the data from the other side
        {
            IOTHUB_TEST_HANDLE devhubTestHandle = IoTHubTest_Initialize(IoTHubAccount_GetEventHubConnectionString(g_iothubAcctInfo), IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo), deviceToUse->deviceId, IoTHubAccount_GetEventhubListenName(g_iothubAcctInfo), IoTHubAccount_GetEventhubAccessKey(g_iothubAcctInfo), IoTHubAccount_GetSharedAccessSignature(g_iothubAcctInfo), IoTHubAccount_GetEventhubConsumerGroup(g_iothubAcctInfo));
            ASSERT_IS_NOT_NULL(devhubTestHandle);

            IOTHUB_TEST_CLIENT_RESULT result = IoTHubTest_ListenForEventForMaxDrainTime(devhubTestHandle, IoTHubCallback, IoTHubAccount_GetIoTHubPartitionCount(g_iothubAcctInfo), expectedData);
            ASSERT_ARE_EQUAL(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_OK, result);

            IoTHubTest_Deinit(devhubTestHandle);
        }

        // Sent Send is Async we need to make sure all the Data has been sent
        continue_run = true;
        beginOperation = time(NULL);
        while (
            ((nowTime = time(NULL)),
            (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME)) && //time box
            continue_run)
        {
            if (Lock(expectedData->lock) != LOCK_OK)
            {
                ASSERT_FAIL("unable to lock found data");
            }
            else
            {
                if (expectedData->wasFound)
                {
                    continue_run = false;
                }
                (void)Unlock(expectedData->lock);
            }
            ThreadAPI_Sleep(100);
        }
        ASSERT_IS_TRUE(expectedData->wasFound); // was found is written by the callback...

                                                ///cleanup
        DESTROY_MODEL_INSTANCE(devModel);
        IoTHubClient_Destroy(iotHubClientHandle);
        SendTestData_Destroy(expectedData); //cleanup

    }

    TEST_FUNCTION(IoTClient_AMQP_MacroSend_e2e_sas)
    {
        AMQP_MacroSend(IoTHubAccount_GetSASDevice(g_iothubAcctInfo));
    }

#ifndef __APPLE__
    TEST_FUNCTION(IoTClient_AMQP_MacroSend_e2e_x509)
    {
        AMQP_MacroSend(IoTHubAccount_GetX509Device(g_iothubAcctInfo));
    }
#endif

    static void Http_MacroRecv(IOTHUB_PROVISIONED_DEVICE* deviceToUse)
    {
        IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
        deviceModel* devModel;

        IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientAuthHandle;
        IOTHUB_MESSAGING_CLIENT_HANDLE iotHubMessagingHandle;
        IOTHUB_MESSAGING_RESULT iotHubMessagingResult;
        IOTHUB_MESSAGE_HANDLE messageHandle;
        //step 1: data is created
        g_recvMacroData = RecvMacroTestData_Create();
        ASSERT_IS_NOT_NULL(g_recvMacroData);

        // Create Service Client
        iotHubServiceClientAuthHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
        ASSERT_IS_NOT_NULL(iotHubServiceClientAuthHandle, "Could not initialize IoTHubServiceClient to send C2D messages to the device");

        iotHubMessagingHandle = IoTHubMessaging_Create(iotHubServiceClientAuthHandle);
        ASSERT_IS_NOT_NULL(iotHubMessagingHandle, "Could not initialize IoTHubMessaging to send C2D messages to the device");

        iotHubMessagingResult = IoTHubMessaging_Open(iotHubMessagingHandle, openCompleteCallback, "Context string for open");
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, iotHubMessagingResult);

        // Create message
        messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)g_recvMacroData->toBeSend, g_recvMacroData->toBeSendSize);

        // act
        iotHubMessagingResult = IoTHubMessaging_SendAsync(iotHubMessagingHandle, deviceToUse->deviceId, messageHandle, sendCompleteCallback, NULL);
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, iotHubMessagingResult, "IoTHubMessaging_SendAsync failed, could not send C2D message to the device");

        //step 3: data is retrieved by HTTP
        {
            time_t beginOperation, nowTime;

            iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(deviceToUse->connectionString, HTTP_Protocol);
            ASSERT_IS_NOT_NULL(iotHubClientHandle);

            if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
                IOTHUB_CLIENT_RESULT result;
                result = IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
                ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
                result = IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
                ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
            }

            unsigned int minimumPollingTime = 0; /*because it should not wait*/
            if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
            {
                printf("failure to set option \"MinimumPollingTime\"\r\n");
            }

            devModel = CREATE_MODEL_INSTANCE(MacroE2EModelAction, deviceModel);
            ASSERT_IS_NOT_NULL(devModel);

            IOTHUB_CLIENT_RESULT setMessageResult = IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, IoTHubMessage, devModel);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, setMessageResult);

            beginOperation = time(NULL);
            while (
                (
                (nowTime = time(NULL)),
                    (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) //time box
                    ) &&
                    (!g_recvMacroData->wasFound) //condition box
                )
            {
                //just go on;
                IoTHubClient_LL_DoWork(iotHubClientHandle);
                ThreadAPI_Sleep(100);
            }
        }

        // assert
        ASSERT_IS_TRUE(g_recvMacroData->wasFound);

        ///cleanup
        DESTROY_MODEL_INSTANCE(devModel);
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        RecvTestData_Destroy(g_recvMacroData);

        IoTHubMessage_Destroy(messageHandle);
        IoTHubMessaging_Close(iotHubMessagingHandle);
        IoTHubMessaging_Destroy(iotHubMessagingHandle);
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientAuthHandle);
    }

#ifndef __APPLE__
    TEST_FUNCTION(IoTClient_Http_MacroRecv_e2e_x509)
    {
        Http_MacroRecv(IoTHubAccount_GetX509Device(g_iothubAcctInfo));
    }
#endif

    static void Http_MacroSend(IOTHUB_PROVISIONED_DEVICE* deviceToUse)
    {
        // arrange
        IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
        deviceModel* devModel;
        time_t beginOperation, nowTime;

        // step 1: prepare data
        time_t t = time(NULL);
        char sztimeText[64];
        (void)sprintf_s(sztimeText, sizeof(sztimeText), "%.24s", ctime(&t));

        EXPECTED_SEND_DATA* expectedData = SendMacroTestData_Create(sztimeText);
        ASSERT_IS_NOT_NULL(expectedData);

        ///act
        // step 2: send data with HTTP
        {
            iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(deviceToUse->connectionString, HTTP_Protocol);
            ASSERT_IS_NOT_NULL(iotHubClientHandle);

            if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
                IOTHUB_CLIENT_RESULT result;
                result = IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
                ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
                result = IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
                ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
            }

            devModel = CREATE_MODEL_INSTANCE(MacroE2EModelAction, deviceModel);
            ASSERT_IS_NOT_NULL(devModel);

            devModel->property1 = sztimeText;
            devModel->UniqueId = (int)g_uniqueTestId;
            unsigned char* destination;
            size_t destinationSize;
            CODEFIRST_RESULT nResult = SERIALIZE(&destination, &destinationSize, *devModel);
            ASSERT_ARE_EQUAL(CODEFIRST_RESULT, CODEFIRST_OK, nResult);
            IOTHUB_MESSAGE_HANDLE iothubMessageHandle = IoTHubMessage_CreateFromByteArray(destination, destinationSize);
            ASSERT_IS_NOT_NULL(iothubMessageHandle);
            free(destination);

            IOTHUB_CLIENT_RESULT iothubClientResult = IoTHubClient_LL_SendEventAsync(iotHubClientHandle, iothubMessageHandle, iotHubMacroCallBack, expectedData);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, iothubClientResult);

            IoTHubMessage_Destroy(iothubMessageHandle);

            // Make sure all the Data has been sent
            beginOperation = time(NULL);
            while (
                (
                (nowTime = time(NULL)),
                    (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) //time box
                    ) &&
                    (!expectedData->dataWasSent)
                ) //condition box
            {
                //just go on;
                IoTHubClient_LL_DoWork(iotHubClientHandle);
                ThreadAPI_Sleep(100);
            }
        }
        ASSERT_IS_TRUE(expectedData->dataWasSent);

        ///assert
        //step3: get the data from the other side
        {
            IOTHUB_TEST_HANDLE devhubTestHandle = IoTHubTest_Initialize(IoTHubAccount_GetEventHubConnectionString(g_iothubAcctInfo), IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo), deviceToUse->deviceId, IoTHubAccount_GetEventhubListenName(g_iothubAcctInfo), IoTHubAccount_GetEventhubAccessKey(g_iothubAcctInfo), IoTHubAccount_GetSharedAccessSignature(g_iothubAcctInfo), IoTHubAccount_GetEventhubConsumerGroup(g_iothubAcctInfo));
            ASSERT_IS_NOT_NULL(devhubTestHandle);

            IOTHUB_TEST_CLIENT_RESULT result = IoTHubTest_ListenForEventForMaxDrainTime(devhubTestHandle, IoTHubCallback, IoTHubAccount_GetIoTHubPartitionCount(g_iothubAcctInfo), expectedData);
            ASSERT_ARE_EQUAL(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_OK, result);

            IoTHubTest_Deinit(devhubTestHandle);
        }

        ASSERT_IS_TRUE(expectedData->wasFound); // was found is written by the callback...

                                                ///cleanup
        DESTROY_MODEL_INSTANCE(devModel);
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        SendTestData_Destroy(expectedData); //cleanup*/
    }

    TEST_FUNCTION(IoTClient_Http_MacroSend_e2e_sas)
    {
        Http_MacroSend(IoTHubAccount_GetSASDevice(g_iothubAcctInfo));
    }

#ifndef __APPLE__
    TEST_FUNCTION(IoTClient_Http_MacroSend_e2e_x509)
    {
        Http_MacroSend(IoTHubAccount_GetX509Device(g_iothubAcctInfo));
    }
#endif



    static int DeviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
    {
        /*userContextCallback is of type deviceModel*/

        /*this is step  3: receive the method and push that payload into serializer (from below)*/
        char* payloadZeroTerminated = (char*)malloc(size + 1);
        ASSERT_IS_NOT_NULL(payloadZeroTerminated);
        (void)memcpy(payloadZeroTerminated, payload, size);
        payloadZeroTerminated[size] = '\0';

        METHODRETURN_HANDLE result = EXECUTE_METHOD(userContextCallback, method_name, payloadZeroTerminated);
        free(payloadZeroTerminated);
        ASSERT_IS_NOT_NULL(result);

        /*step 4: get the serializer answer and push it in the networking stack*/
        const METHODRETURN_DATA* data = MethodReturn_GetReturn(result);

        int statusCode = data->statusCode;

        ASSERT_IS_NOT_NULL(data->jsonValue);

        *resp_size = strlen(data->jsonValue);

        *response = (unsigned char*)malloc(*resp_size);
        ASSERT_IS_NOT_NULL(*response);
        (void)memcpy(*response, data->jsonValue, *resp_size);

        MethodReturn_Destroy(result);
        return statusCode;
    }


    /*the following test shall perform the following
    1: create a model instance that has a method
    2: create/start the device + start listening on method
    3: receive the method and push that payload into serializer
    4: get the serializer answer and push it in the networking stack
    5: verify on the service side that the answer matches the expectations
    6: destroy all things*/

    /*
    by convention, it shall call a function that receives 2 ints and produces the sum of the ints in the answer.
    by convention, the request JSON looks like
    {
        "a": 3,
        "b": 33
    }

    by convention, the answer shall have "36" (or whatever the function computes) as only payload, status will be set to 10.

    */

    static void MQTT_can_receive_a_method(IOTHUB_PROVISIONED_DEVICE* deviceToUse)
    {

        /*step 1: create a model instance that has a method*/
        IOTHUB_CLIENT_RESULT result;

        deviceModel* device = CREATE_MODEL_INSTANCE(MacroE2EModelAction, deviceModel);
        ASSERT_IS_NOT_NULL(device);

        /*step 2: create/start the device + start listening on method*/
        IOTHUB_CLIENT_HANDLE iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, MQTT_Protocol);
        ASSERT_IS_NOT_NULL(iotHubClientHandle, "Could not create IoTHubClient");

        if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
            result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
            result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
        }

        result = IoTHubClient_SetDeviceMethodCallback(iotHubClientHandle, DeviceMethodCallback, device);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device method callback");

        // Wait for the method subscription to go through
        ThreadAPI_Sleep(3 * 1000);

        IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
        ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "Could not create service client handle");

        IOTHUB_SERVICE_CLIENT_DEVICE_METHOD_HANDLE serviceClientDeviceMethodHandle = IoTHubDeviceMethod_Create(iotHubServiceClientHandle);
        ASSERT_IS_NOT_NULL(serviceClientDeviceMethodHandle, "Could not create device method handle");

        /*step 5: verify on the service side that the answer matches the expectations*/
        int responseStatus;
        unsigned char* responsePayload;
        size_t responsePayloadSize;
        IOTHUB_DEVICE_METHOD_RESULT invokeResult = IoTHubDeviceMethod_Invoke(serviceClientDeviceMethodHandle, deviceToUse->deviceId, "theSumOfThings", "{\"a\":3, \"b\":33}", 120, &responseStatus, &responsePayload, &responsePayloadSize);

        ASSERT_ARE_EQUAL(IOTHUB_DEVICE_METHOD_RESULT, IOTHUB_DEVICE_METHOD_OK, invokeResult, "IoTHubDeviceMethod_Invoke failed");
        ASSERT_ARE_EQUAL(int, 10, responseStatus, "response status is incorrect");

        /*make sure the response is null terminated*/

        char* responseAsString = (char*)malloc(responsePayloadSize + 1);
        ASSERT_IS_NOT_NULL(responseAsString);
        (void)memcpy(responseAsString, responsePayload, responsePayloadSize);
        responseAsString[responsePayloadSize] = '\0';

        /*parse the response*/
        JSON_Value *jsonValue = json_parse_string(responseAsString);
        free(responseAsString);
        ASSERT_IS_NOT_NULL(jsonValue);

        double d = json_value_get_number(jsonValue);
        json_value_free(jsonValue);
        ASSERT_ARE_EQUAL(int, 36, (int)d);

        /*cleanup*/
        /*step 6: destroy all things*/
        free(responsePayload);
        IoTHubDeviceMethod_Destroy(serviceClientDeviceMethodHandle);
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
        IoTHubClient_Destroy(iotHubClientHandle);
        DESTROY_MODEL_INSTANCE(device);
    }

    TEST_FUNCTION(IoTClient_MQTT_can_receive_a_method_sas)
    {
        MQTT_can_receive_a_method(IoTHubAccount_GetSASDevice(g_iothubAcctInfo));
    }

#ifndef __APPLE__
    TEST_FUNCTION(IoTClient_MQTT_can_receive_a_method_x509)
    {
        MQTT_can_receive_a_method(IoTHubAccount_GetX509Device(g_iothubAcctInfo));
    }
#endif


    /*the following tests IotHubDeviceTwin_SendReportedState*/
    /*these are steps the test will make*/
    /*step 1: import the schema; create an IoTHubClient handle
      step 2: create an instance of the model
      step 3: set the model instance's value to something; send it!
      step 4: wait for the callback to be called (it needs to indicate success)
      step 5: use the C service SDK to pick up the reported properties from the service(they need to indicate the same value)
      step 6: succeed
   */

    static void DeviceTwin_SendReportedState_succeeds(IOTHUB_PROVISIONED_DEVICE* deviceToUse)
    {

        /*step 1: import the schema; create an IoTHubClient handle*/
        SERIALIZER_REGISTER_NAMESPACE(MacroE2EModelAction);

        IOTHUB_CLIENT_HANDLE iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, MQTT_Protocol);
        ASSERT_IS_NOT_NULL(iotHubClientHandle);

        bool traceOn = true;
        IoTHubClient_SetOption(iotHubClientHandle, "logtrace", &traceOn);

        if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
            IOTHUB_CLIENT_RESULT result;
            result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
            result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
        }

        /*step 2: create an instance of the model*/
        deviceModel* model = IoTHubDeviceTwin_CreatedeviceModel(iotHubClientHandle);

        /* step 3: set the model instance's value to something; send it!*/
        model->reported_int = 199;
        IOTHUB_CLIENT_RESULT res = IoTHubDeviceTwin_SendReportedStatedeviceModel(model, my_IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, NULL);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, res);

        /*step 4: wait for the callback to be called (it needs to indicate success)*/
        /*time bound it to 1 minute*/
        time_t begin = time(NULL);
        while (difftime(time(NULL), begin)<MAX_TWIN_CLOUD_TRAVEL_TIME)
        {
            (void)Lock(lock);
            if (was_my_IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_called)
            {
                (void)Unlock(lock);
                /*can proceed to next step, callback was called*/
                break;
            }
            (void)Unlock(lock);
            ThreadAPI_Sleep(1);
        }

        ASSERT_ARE_EQUAL(int, was_my_IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_called, 1);

        /*step 5: use the C service SDK to pick up the reported properties from the service(they need to indicate the same value)*/
        IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
        ASSERT_IS_NOT_NULL(iotHubServiceClientHandle);

        IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceClientDeviceTwinHandle = IoTHubDeviceTwin_Create(iotHubServiceClientHandle);
        ASSERT_IS_NOT_NULL(serviceClientDeviceTwinHandle);

        char* theTwin = IoTHubDeviceTwin_GetTwin(serviceClientDeviceTwinHandle, deviceToUse->deviceId);

        JSON_Value* twinJson = json_parse_string(theTwin);
        JSON_Object* twinJsonObject = json_value_get_object(twinJson);
        double reported_by_the_service = json_object_dotget_number(twinJsonObject, "properties.reported.reported_int");
        ASSERT_ARE_EQUAL(double, reported_by_the_service, 199); /*same like model->reported_int = 199;*/

                                                                ///cleanup
        json_value_free(twinJson);
        free(theTwin);
        IoTHubDeviceTwin_Destroy(serviceClientDeviceTwinHandle);
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
        IoTHubDeviceTwin_DestroydeviceModel(model);
        IoTHubClient_Destroy(iotHubClientHandle);
    }

    TEST_FUNCTION(IotHubDeviceTwin_SendReportedState_succeeds_sas)
    {
        DeviceTwin_SendReportedState_succeeds(IoTHubAccount_GetSASDevice(g_iothubAcctInfo));
    }

#ifndef __APPLE__
    TEST_FUNCTION(IotHubDeviceTwin_SendReportedState_succeeds_x509)
    {
        DeviceTwin_SendReportedState_succeeds(IoTHubAccount_GetX509Device(g_iothubAcctInfo));
    }
#endif

END_TEST_SUITE(serializer_e2e)
