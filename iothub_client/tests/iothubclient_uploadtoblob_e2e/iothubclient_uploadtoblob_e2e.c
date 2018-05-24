// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothub_messaging.h"

#include "iothub_account.h"
#include "iothubtest.h"

#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/lock.h"
#include "../../../certs/certs.h"
#include "azure_c_shared_utility/xlogging.h"

#ifdef TEST_MQTT
#include "iothubtransportmqtt.h"
#endif

#ifdef TEST_AMQP
#include "iothubtransportamqp.h"
#include "iothubtransportamqp_websockets.h"
#endif

#ifdef TEST_HTTP
#include "iothubtransporthttp.h"
#endif

typedef enum UPLOADTOBLOB_CALLBACK_STATUS
{
    UPLOADTOBLOB_CALLBACK_PENDING,
    UPLOADTOBLOB_CALLBACK_SUCCEEDED,
    UPLOADTOBLOB_CALLBACK_FAILED
} UPLOADTOBLOB_CALLBACK_STATUS;

static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo;

static bool uploadToBlobCauseAbort;
static int uploadBlobNumber;

static LOCK_HANDLE updateBlobTestLock;

#define IOTHUB_UPLOADTOBLOB_TIMEOUT_SEC 30
#define TEST_MAX_SIMULTANEOUS_UPLOADS 5


TEST_DEFINE_ENUM_TYPE(UPLOADTOBLOB_CALLBACK_STATUS, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE "hello_world.txt"
#define UPLOADTOBLOB_E2E_TEST_MULTI_BLOCK_DESTINATION_FILE "hello_world_multiblock.txt"
#define UPLOADTOBLOB_E2E_TEST_DATA (const unsigned char*)"e2e_UPLOADTOBLOB_CALLBACK test data"

#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE1 "hello_world1.txt"
#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE2 "hello_world2.txt"
#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE3 "hello_world3.txt"
#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE4 "hello_world4.txt"
#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE5 "hello_world5.txt"

#define UPLOADTOBLOB_E2E_TEST_MULTI_BLOCK_DESTINATION_FILE1 "hello_world_multiblock.txt"

static int bool_Compare(bool left, bool right)
{
    return left != right;
}

static void bool_ToString(char* string, size_t bufferSize, bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}

#ifndef __cplusplus
static int _Bool_Compare(_Bool left, _Bool right)
{
    return left != right;
}

static void _Bool_ToString(char* string, size_t bufferSize, _Bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}
#endif

void e2e_uploadblob_init()
{
    updateBlobTestLock = Lock_Init();
    ASSERT_IS_NOT_NULL(updateBlobTestLock);

    int result = platform_init();
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "Platform init failed");
    g_iothubAcctInfo = IoTHubAccount_Init();
    ASSERT_IS_NOT_NULL_WITH_MSG(g_iothubAcctInfo, "Could not initialize IoTHubAccount");
    platform_init();
}

void e2e_uploadblob_deinit()
{
    IoTHubAccount_deinit(g_iothubAcctInfo);
    // Need a double deinit
    platform_deinit();
    platform_deinit();

    Lock_Deinit(updateBlobTestLock);
}

// uploadToBlobCallback is invoked once upload succeeds or fails.  It updates context in either case so
// main waiting thread knows we're done.
void uploadToBlobCallback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT fileUploadResult, void* userContextCallback)
{
    if (Lock(updateBlobTestLock) != LOCK_OK)
    {
        ASSERT_FAIL("unable to lock in callback thread");
    }
    else
    {
        LogInfo("uploadToBlobCallback(%s), userContextCallback(%p)", ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, fileUploadResult), userContextCallback);

        UPLOADTOBLOB_CALLBACK_STATUS* callbackStatus = (UPLOADTOBLOB_CALLBACK_STATUS*)userContextCallback;
        ASSERT_ARE_EQUAL(UPLOADTOBLOB_CALLBACK_STATUS, *callbackStatus, UPLOADTOBLOB_CALLBACK_PENDING);

        if (fileUploadResult == FILE_UPLOAD_OK)
        {
            *callbackStatus = UPLOADTOBLOB_CALLBACK_SUCCEEDED;
        }
        else
        {
            *callbackStatus = UPLOADTOBLOB_CALLBACK_FAILED;
        }
        (void)Unlock(updateBlobTestLock);
    }
}

void poll_for_upload_completion(UPLOADTOBLOB_CALLBACK_STATUS *uploadToBlobStatus)
{
    time_t nowTime;
    time_t beginOperation = time(NULL);
    bool continue_running = true;

    // Loop below polls for completion (or failure or timeout) of upload, ultimately set by uploadToBlobCallback.
    do
    {
        if (Lock(updateBlobTestLock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock in main test thread");
        }
        else
        {
            if (*uploadToBlobStatus != UPLOADTOBLOB_CALLBACK_PENDING)
            {
                continue_running = false;
            }
            (void)Unlock(updateBlobTestLock);
        }

        ThreadAPI_Sleep(10);
    } while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < IOTHUB_UPLOADTOBLOB_TIMEOUT_SEC) &&
        (continue_running == true)
        );

}

static void check_upload_result(UPLOADTOBLOB_CALLBACK_STATUS uploadToBlobStatus)
{
    if (UPLOADTOBLOB_CALLBACK_FAILED == uploadToBlobStatus)
    {
        ASSERT_FAIL("IoTHubClient_UploadToBlobAsync failed in callback");
    }
    else if (UPLOADTOBLOB_CALLBACK_PENDING == uploadToBlobStatus)
    {
        ASSERT_FAIL("IoTHubClient_UploadToBlobAsync timed out performing file upload");
    }
    else if (UPLOADTOBLOB_CALLBACK_SUCCEEDED == uploadToBlobStatus)
    {
        LogInfo("IoTHubClient_UploadToBlobAsync succeeded");
    }
    else
    {
        ASSERT_FAIL("Unknown uploadToBlobStatus");
    }
}

void e2e_uploadtoblob_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_PROVISIONED_DEVICE* deviceToUse;
    if (accountAuthMethod == IOTHUB_ACCOUNT_AUTH_X509)
    {
        deviceToUse = IoTHubAccount_GetX509Device(g_iothubAcctInfo);
    }
    else
    {
        deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    }
    ASSERT_IS_NOT_NULL(deviceToUse);
    
    IOTHUB_CLIENT_HANDLE iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, protocol);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubClientHandle, "Could not invoke IoTHubClient_CreateFromConnectionString");

    if (accountAuthMethod == IOTHUB_ACCOUNT_AUTH_X509)
    {
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
    }

    UPLOADTOBLOB_CALLBACK_STATUS uploadToBlobStatus = UPLOADTOBLOB_CALLBACK_PENDING;
    result = IoTHubClient_UploadToBlobAsync(iotHubClientHandle, UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE, UPLOADTOBLOB_E2E_TEST_DATA, strlen((const char*)UPLOADTOBLOB_E2E_TEST_DATA), uploadToBlobCallback, &uploadToBlobStatus);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");

    poll_for_upload_completion(&uploadToBlobStatus);
    check_upload_result(uploadToBlobStatus);

    IoTHubClient_Destroy(iotHubClientHandle);
}


IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT uploadToBlobGetDataEx(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT callbackResult;
    LogInfo("uploadToBlobCallback(%s), call number(%d)", ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result), uploadBlobNumber);

    UPLOADTOBLOB_CALLBACK_STATUS* callbackStatus = (UPLOADTOBLOB_CALLBACK_STATUS*)context;
    ASSERT_ARE_EQUAL(UPLOADTOBLOB_CALLBACK_STATUS, *callbackStatus, UPLOADTOBLOB_CALLBACK_PENDING);
    ASSERT_ARE_EQUAL(int, (int)result, (int)FILE_UPLOAD_OK);

    static char* uploadData0 = "AAA-";
    static char* uploadData1 = "BBBBB-";
    static char* uploadData2 = "CCCCCC-";

    ASSERT_ARE_EQUAL(int, (int)Lock(updateBlobTestLock), (int)LOCK_OK);

    if (data == NULL)
    {
        // We're invoked for the final time when we're done uploading and with ultmitale result.
        ASSERT_IS_NULL(size);
        if (uploadToBlobCauseAbort == true)
        {
            // If we've aborted, expect to be called with final invocation sooner
            ASSERT_ARE_EQUAL(bool, true, (uploadBlobNumber == 3) ? true : false);
        }
        else
        {
            // If we're not completing, expect to be called with final invocation after all blocks have been passed up.
            ASSERT_ARE_EQUAL(bool, true, (uploadBlobNumber == 4) ? true : false);
        }
        *callbackStatus = UPLOADTOBLOB_CALLBACK_SUCCEEDED;
    }
    else
    {
        // We're invoked with a request for more data.
        ASSERT_ARE_EQUAL(bool, true, (uploadBlobNumber <= 3) ? true : false);
        if (uploadBlobNumber == 0)
        {
            *data = (unsigned char const *)uploadData0;
        }
        else if (uploadBlobNumber == 1)
        {
            *data = (unsigned char const *)uploadData1;
        }
        else if (uploadBlobNumber == 2)
        {
            *data = (unsigned char const *)uploadData2;
        }
        else if (uploadBlobNumber == 3)
        {
            // This will cause us to return 0 size to caller, which means no more data to read.
            *data = (unsigned char const *)"";
        }

        *size = strlen((const char*)*data);
        *callbackStatus = UPLOADTOBLOB_CALLBACK_PENDING;
    }

    if ((uploadToBlobCauseAbort == true) && (uploadBlobNumber == 2))
    {
        callbackResult = IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT;
    }
    else
    {
        callbackResult = IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
    }
    uploadBlobNumber++;
    
    (void)Unlock(updateBlobTestLock);
    return callbackResult;
}

// The non-Ex callback is identical to the Ex, except it is void instead of returning a value to caller.
void uploadToBlobGetData(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    (void)uploadToBlobGetDataEx(result, data, size, context);
}

void e2e_uploadtoblob_multiblock_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, bool useExMethod, bool causeAbort)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_PROVISIONED_DEVICE* deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    ASSERT_IS_NOT_NULL(deviceToUse);
    
    IOTHUB_CLIENT_HANDLE iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, protocol);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubClientHandle, "Could not invoke IoTHubClient_CreateFromConnectionString");

    uploadToBlobCauseAbort = causeAbort;
    UPLOADTOBLOB_CALLBACK_STATUS uploadToBlobStatus = UPLOADTOBLOB_CALLBACK_PENDING;
    uploadBlobNumber = 0;
    if (useExMethod)
    {
        result = IoTHubClient_UploadMultipleBlocksToBlobAsyncEx(iotHubClientHandle, UPLOADTOBLOB_E2E_TEST_MULTI_BLOCK_DESTINATION_FILE, uploadToBlobGetDataEx, &uploadToBlobStatus);
    }
    else
    {
        result = IoTHubClient_UploadMultipleBlocksToBlobAsync(iotHubClientHandle, UPLOADTOBLOB_E2E_TEST_MULTI_BLOCK_DESTINATION_FILE, uploadToBlobGetData, &uploadToBlobStatus);
    }
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");

    poll_for_upload_completion(&uploadToBlobStatus);
    check_upload_result(uploadToBlobStatus);

    IoTHubClient_Destroy(iotHubClientHandle);
}

void e2e_uploadtoblob_test_multiple_simultaneous_uploads(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_PROVISIONED_DEVICE* deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    ASSERT_IS_NOT_NULL(deviceToUse);
    
    IOTHUB_CLIENT_HANDLE iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, protocol);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubClientHandle, "Could not invoke IoTHubClient_CreateFromConnectionString");

    UPLOADTOBLOB_CALLBACK_STATUS uploadToBlobStatus[TEST_MAX_SIMULTANEOUS_UPLOADS];
    const char* uploadFileNameList[] = { UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE1, UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE2, UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE3, UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE4, UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE5 };

    // Fire off TEST_MAX_SIMULTANEOUS_UPLOADS simultaneous upload requests.  These will fire in parallel.
    for (int i = 0; i < TEST_MAX_SIMULTANEOUS_UPLOADS; i++)
    {
        const char* uploadFileName = uploadFileNameList[i];
        uploadToBlobStatus[i] = UPLOADTOBLOB_CALLBACK_PENDING;
        result = IoTHubClient_UploadToBlobAsync(iotHubClientHandle, uploadFileName, UPLOADTOBLOB_E2E_TEST_DATA, strlen(uploadFileName), uploadToBlobCallback, &uploadToBlobStatus[i]);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");
    }

    time_t beginOperation = time(NULL);

    // Poll for completion, looping one at a time.
    for (int i = 0; i < TEST_MAX_SIMULTANEOUS_UPLOADS; i++)
    {
        printf("waiting for context for context(%p), file(%s)\n", &uploadToBlobStatus[i], uploadFileNameList[i]);
        poll_for_upload_completion(&uploadToBlobStatus[i]);
        check_upload_result(uploadToBlobStatus[i]);
    }

    // Even though we had multiple simultaneous threads, expect them to complete in reasonable time (*2 to allow
    // for additional server load).
    time_t endOperation = time(NULL);
    ASSERT_ARE_EQUAL_WITH_MSG(bool, true, (difftime(endOperation, beginOperation) < IOTHUB_UPLOADTOBLOB_TIMEOUT_SEC * 2) ? true : false, "Multithreaded upload took longer than allowed");

    IoTHubClient_Destroy(iotHubClientHandle);
}


BEGIN_TEST_SUITE(iothubclient_uploadtoblob_e2e)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    e2e_uploadblob_init();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    e2e_uploadblob_deinit();
}

#ifdef TEST_MQTT
TEST_FUNCTION(IoTHub_MQTT_UploadToBlob_multithreaded)
{
    //Currently not working
    //e2e_uploadtoblob_test_multiple_simultaneous_uploads(MQTT_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_UploadToBlob_sas)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

TEST_FUNCTION(IoTHub_MQTT_WS_UploadToBlob_sas)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

TEST_FUNCTION(IoTHub_MQTT_UploadMultipleBlocksToBlobEx)
{
    e2e_uploadtoblob_multiblock_test(MQTT_Protocol, true, false);
}

TEST_FUNCTION(IoTHub_MQTT_UploadMultipleBlocksToBlob)
{
    e2e_uploadtoblob_multiblock_test(MQTT_Protocol, false, false);
}

TEST_FUNCTION(IoTHub_MQTT_UploadMultipleBlocksToBlobExWithAbort)
{
    e2e_uploadtoblob_multiblock_test(MQTT_Protocol, true, true);
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_MQTT_UploadToBlob_x509)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_X509);
}

TEST_FUNCTION(IoTHub_MQTT_WS_UploadToBlob_x509)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_X509);
}
#endif // __APPLE__

#endif // TEST_MQTT

#ifdef TEST_AMQP
TEST_FUNCTION(IoTHub_AMQP_UploadToBlob_sas)
{
    // Currently not working
    // e2e_uploadtoblob_test(AMQP_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

TEST_FUNCTION(IoTHub_AMQP_WS_UploadToBlob_sas)
{
    // Currently not working
    // e2e_uploadtoblob_test(AMQP_Protocol_over_WebSocketsTls, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_AMQP_UploadToBlob_x509)
{
    e2e_uploadtoblob_test(AMQP_Protocol, IOTHUB_ACCOUNT_AUTH_X509);
}

TEST_FUNCTION(IoTHub_AMQP_WS_UploadToBlob_x509)
{
    e2e_uploadtoblob_test(AMQP_Protocol_over_WebSocketsTls, IOTHUB_ACCOUNT_AUTH_X509);
}
#endif // __APPLE__

#endif // TEST_AMQP

#ifdef TEST_HTTP
TEST_FUNCTION(IoTHub_HTTP_UploadToBlob_sas)
{
    e2e_uploadtoblob_test(HTTP_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_HTTP_UploadToBlob_x509)
{
    e2e_uploadtoblob_test(HTTP_Protocol, IOTHUB_ACCOUNT_AUTH_X509);
}
#endif // __APPLE__


#endif // TEST_HTTP


END_TEST_SUITE(iothubclient_uploadtoblob_e2e)

