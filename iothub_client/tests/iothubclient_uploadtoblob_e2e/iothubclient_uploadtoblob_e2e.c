// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"


// Note: Even though iothub_client.h is deprecated, we are using it as basis of our tests and NOT the recommended
// device_client.h.  In practice for most tests there's no difference (both APIs quickly pass through to core implementation).
// However, the iothub_client.h has both an Ex and non-Ex version of IoTHubClient_UploadMultipleBlocksToBlobAsync.
// The device_client did NOT bring this Ex/non split forward.
// We must maintain back-compat with the non-Ex function, so we need to #include "iothub_client.h" as it is only way to access it.
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
#include "azure_c_shared_utility/shared_util_options.h"


#ifdef TEST_MQTT
#include "iothubtransportmqtt.h"
#include "iothubtransportmqtt_websockets.h"
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

// Even though g_uploadToBlobStatus is only used in scope of a function, there are test cases
// where test function may exit prior to the SDK callback completing.  So the variable needs to be at global scope.
UPLOADTOBLOB_CALLBACK_STATUS g_uploadToBlobStatus;

#define IOTHUB_UPLOADTOBLOB_TIMEOUT_SEC 120
#define TEST_MAX_SIMULTANEOUS_UPLOADS 3
#define TEST_JITTER_BETWEEN_UPLOAD_TO_BLOB_E2E_TESTS_MS 1500
#define TEST_SLEEP_BETWEEN_UPLOAD_TO_BLOB_E2E_TESTS_MS 7500

#define TEST_SLEEP_BETWEEN_MULTIBLOCK_UPLOADS 2000

TEST_DEFINE_ENUM_TYPE(UPLOADTOBLOB_CALLBACK_STATUS, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE "hello_world.txt"
#define UPLOADTOBLOB_E2E_TEST_MULTI_BLOCK_DESTINATION_FILE "hello_world_multiblock.txt"
#define UPLOADTOBLOB_E2E_TEST_DATA (const unsigned char*)"e2e_UPLOADTOBLOB_CALLBACK test data"

#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE1 "hello_world1.txt"
#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE2 "hello_world2.txt"
#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE3 "hello_world3.txt"

#define UPLOADTOBLOB_E2E_TEST_MULTI_BLOCK_DESTINATION_FILE1 "hello_world_multiblock.txt"

static char* uploadData0 = "AAA-";
static char* uploadData1 = "BBBBB-";
static char* uploadData2 = "CCCCCC-";

static char* uploadDataMultiThread = "Upload data from multi-thread test";

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
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    updateBlobTestLock = Lock_Init();
    ASSERT_IS_NOT_NULL(updateBlobTestLock);

    srand((unsigned int)time(NULL));

    int result = platform_init();
    ASSERT_ARE_EQUAL(int, 0, result, "Platform init failed");
    g_iothubAcctInfo = IoTHubAccount_Init(false);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo, "Could not initialize IoTHubAccount");
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
        LogInfo("uploadToBlobCallback(%s), userContextCallback(%p)", MU_ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, fileUploadResult), userContextCallback);

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
        ASSERT_FAIL("Upload failed in callback");
    }
    else if (UPLOADTOBLOB_CALLBACK_PENDING == uploadToBlobStatus)
    {
        ASSERT_FAIL("Upload timed out performing file upload");
    }
    else if (UPLOADTOBLOB_CALLBACK_SUCCEEDED == uploadToBlobStatus)
    {
        LogInfo("Upload succeeded");
    }
    else
    {
        ASSERT_FAIL("Unknown uploadToBlobStatus");
    }
}

static void sleep_between_upload_blob_e2e_tests(void)
{
    // We need a Sleep() between each E2E test.  Current (as of October, 2020) throttling
    // rules limit an S1 hub to 1.67 upload initiations per second for the entire IoT Hub (not just per device).
    // https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-quotas-throttling
    //
    // On gate runs, we have many instances of this test executable running at the same time and we cannot orchestrate
    // them.  Most individual testcases take <1 second to run.  Without a sleep, this amount of traffic
    // will end up going over throttling maximums and causing test case failures.  
    // And because the tests run very quickly, we need additional jitter to keep different test executables from getting
    // "in phase" on when they run.
    //
    unsigned int jitter = (rand() % TEST_JITTER_BETWEEN_UPLOAD_TO_BLOB_E2E_TESTS_MS);
    unsigned int sleepTime = TEST_SLEEP_BETWEEN_UPLOAD_TO_BLOB_E2E_TESTS_MS + jitter;
        
    LogInfo("Invoking sleep for %d milliseconds after test case", sleepTime);
    ThreadAPI_Sleep(sleepTime);
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
    ASSERT_IS_NOT_NULL(iotHubClientHandle, "Could not invoke IoTHubClient_CreateFromConnectionString");

    if (accountAuthMethod == IOTHUB_ACCOUNT_AUTH_X509)
    {
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
    }

#if defined(__APPLE__) || defined(AZIOT_LINUX)
    bool curl_verbose = true;
    result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_CURL_VERBOSE, &curl_verbose);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set curl_verbose opt");
#endif
	
    g_uploadToBlobStatus = UPLOADTOBLOB_CALLBACK_PENDING;
    result = IoTHubClient_UploadToBlobAsync(iotHubClientHandle, UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE, UPLOADTOBLOB_E2E_TEST_DATA, strlen((const char*)UPLOADTOBLOB_E2E_TEST_DATA), uploadToBlobCallback, &g_uploadToBlobStatus);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");

    poll_for_upload_completion(&g_uploadToBlobStatus);
    check_upload_result(g_uploadToBlobStatus);

    IoTHubClient_Destroy(iotHubClientHandle);
}

IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT uploadToBlobGetDataEx(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT callbackResult;

    UPLOADTOBLOB_CALLBACK_STATUS* callbackStatus = (UPLOADTOBLOB_CALLBACK_STATUS*)context;
    ASSERT_ARE_EQUAL(UPLOADTOBLOB_CALLBACK_STATUS, *callbackStatus, UPLOADTOBLOB_CALLBACK_PENDING);
    ASSERT_ARE_EQUAL(int, (int)FILE_UPLOAD_OK, (int)result);

    LogInfo("uploadToBlobCallback(%s), call number(%d)", MU_ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result), uploadBlobNumber);

    ASSERT_ARE_EQUAL(int, (int)LOCK_OK, (int)Lock(updateBlobTestLock));

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

    LogInfo("Sleeping %d seconds between upload calls", TEST_SLEEP_BETWEEN_MULTIBLOCK_UPLOADS);
    ThreadAPI_Sleep(TEST_SLEEP_BETWEEN_MULTIBLOCK_UPLOADS);
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
    ASSERT_IS_NOT_NULL(iotHubClientHandle, "Could not invoke IoTHubClient_CreateFromConnectionString");

    uploadToBlobCauseAbort = causeAbort;
    g_uploadToBlobStatus = UPLOADTOBLOB_CALLBACK_PENDING;
    uploadBlobNumber = 0;
    if (useExMethod)
    {
        result = IoTHubClient_UploadMultipleBlocksToBlobAsyncEx(iotHubClientHandle, UPLOADTOBLOB_E2E_TEST_MULTI_BLOCK_DESTINATION_FILE, uploadToBlobGetDataEx, &g_uploadToBlobStatus);
    }
    else
    {
        result = IoTHubClient_UploadMultipleBlocksToBlobAsync(iotHubClientHandle, UPLOADTOBLOB_E2E_TEST_MULTI_BLOCK_DESTINATION_FILE, uploadToBlobGetData, &g_uploadToBlobStatus);
    }
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");

    poll_for_upload_completion(&g_uploadToBlobStatus);
    check_upload_result(g_uploadToBlobStatus);

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

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    // We need to sleep to avoid triggering IoT Hub upload threshold limits in our E2E tests.
    sleep_between_upload_blob_e2e_tests();
}

#ifdef TEST_MQTT
#if 0 
// TODO: Re-enable.  See https://github.com/Azure/azure-iot-sdk-c/issues/1770.
// Once these are re-enabled, remove IoTHub_MQTT_UploadToBlob_ConnString as its duplicative.
// It's needed for now to get Apple coverage however.
TEST_FUNCTION(IoTHub_MQTT_UploadMultipleBlocksToBlobEx)
{
    e2e_uploadtoblob_multiblock_test(MQTT_Protocol, true, false);
}

TEST_FUNCTION(IoTHub_MQTT_UploadMultipleBlocksToBlobExWithAbort)
{
    e2e_uploadtoblob_multiblock_test(MQTT_Protocol, true, true);
}

TEST_FUNCTION(IoTHub_MQTT_UploadToBlob_x509)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_X509);
}
#else
TEST_FUNCTION(IoTHub_MQTT_UploadToBlob_ConnString)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}
#endif

#endif // TEST_MQTT


END_TEST_SUITE(iothubclient_uploadtoblob_e2e)

