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
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/tickcounter.h"

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
#define TEST_JITTER_BETWEEN_UPLOAD_TO_BLOB_E2E_TESTS_MS 1500
#define TEST_SLEEP_BETWEEN_UPLOAD_TO_BLOB_E2E_TESTS_MS 7500

#define TEST_SLEEP_BETWEEN_MULTIBLOCK_UPLOADS 2000

#define TEST_SLEEP_BETWEEN_NEW_UPLOAD_THREAD 150

// After starting an upload, we close the handle after TEST_SLEEP_BEFORE_EARLY_HANDLE_CLOSE milliseconds
// but have a worker thread sleep for TEST_SLEEP_SLOW_WORKER_THREAD to simulate an active worker thread during close.
#define TEST_SLEEP_BEFORE_EARLY_HANDLE_CLOSE 1000
#define TEST_SLEEP_SLOW_WORKER_THREAD 5000

// In normal operation we should only need a few seconds for the workers to complete.
// On Valgrind test runs, however, there's a significant performance degradation because
// of all the simultaneous threads the test creates.  Allow ample time (but not forever).
#define TEST_MAXIMUM_TIME_FOR_DESTROY_ON_BLOCKED_THREADS_TO_COMPLETE_SECONDS 30

#define INDEFINITE_TIME ((time_t)(-1))

TEST_DEFINE_ENUM_TYPE(UPLOADTOBLOB_CALLBACK_STATUS, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

static const char TEST_UPLOADTOBLOB_DESTINATION_FILE[] = "upload_destination_file.txt";
static const char TEST_UPLOADTOBLOB_DESTINATION_FILE_MULTIBLOCK[] = "upload_destination_file_multiblock.txt";
static const char TEST_UPLOADTOBLOB_DESTINATION_FILE_CLOSE_THREAD[] = "close_thread_test.txt";
static const unsigned char* UPLOADTOBLOB_E2E_TEST_DATA = (const unsigned char*)"e2e_UPLOADTOBLOB_CALLBACK test data";
static const char* uploadDataFromCallback = "AAA-";
static bool testSlowThreadContext;

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

static void e2e_uploadblob_init()
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

static void e2e_uploadblob_deinit()
{
    IoTHubAccount_deinit(g_iothubAcctInfo);
    // Need a double deinit
    platform_deinit();
    platform_deinit();

    Lock_Deinit(updateBlobTestLock);
}

// uploadToBlobCallback is invoked once upload succeeds or fails.  It updates context in either case so
// main waiting thread knows we're done.
static void uploadToBlobCallback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT fileUploadResult, void* userContextCallback)
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

static void poll_for_upload_completion(UPLOADTOBLOB_CALLBACK_STATUS *uploadToBlobStatus)
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
    // https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-quotas-throttling
    //
    // On gate runs, we have many instances of this test executable running at the same time and we cannot orchestrate
    // them.  Most individual testcases take <1 second to run.  Without a sleep, this amount of traffic
    // will end up going over throttling maximums and causing test case failures.  
    // And because the tests run very quickly, we need additional jitter to keep different test executables from getting
    // "in phase" on when they run.
    //
    unsigned int jitter = (rand() % TEST_JITTER_BETWEEN_UPLOAD_TO_BLOB_E2E_TESTS_MS);
    unsigned int sleepTime = TEST_SLEEP_BETWEEN_UPLOAD_TO_BLOB_E2E_TESTS_MS + jitter;
        
    LogInfo("Invoking sleep for %d milliseconds before test case", sleepTime);
    ThreadAPI_Sleep(sleepTime);
}

static void e2e_uploadtoblob_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod)
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
    result = IoTHubClient_UploadToBlobAsync(iotHubClientHandle, TEST_UPLOADTOBLOB_DESTINATION_FILE, UPLOADTOBLOB_E2E_TEST_DATA, strlen((const char*)UPLOADTOBLOB_E2E_TEST_DATA), uploadToBlobCallback, &g_uploadToBlobStatus);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");

    poll_for_upload_completion(&g_uploadToBlobStatus);
    check_upload_result(g_uploadToBlobStatus);

    IoTHubClient_Destroy(iotHubClientHandle);
}

// Callback invoked by the SDK to retrieve data.
static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT uploadToBlobGetDataEx(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
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
            ASSERT_ARE_EQUAL(bool, true, (uploadBlobNumber == 2) ? true : false);
        }
        else
        {
            // If we're not completing, expect to be called with final invocation after all blocks have been passed up.
            ASSERT_ARE_EQUAL(bool, true, (uploadBlobNumber == 2) ? true : false);
        }
        *callbackStatus = UPLOADTOBLOB_CALLBACK_SUCCEEDED;
    }
    else
    {
        // We're invoked with a request for more data.
        ASSERT_ARE_EQUAL(bool, true, (uploadBlobNumber <= 2) ? true : false);
        if (uploadBlobNumber == 0)
        {
            *data = (unsigned char const *)uploadDataFromCallback;
        }
        else if (uploadBlobNumber == 1)
        {
            // This will cause us to return 0 size to caller, which means no more data to read.
            *data = (unsigned char const *)"";
        }

        *size = strlen((const char*)*data);
        *callbackStatus = UPLOADTOBLOB_CALLBACK_PENDING;
    }

    if ((uploadToBlobCauseAbort == true) && (uploadBlobNumber == 1))
    {
        callbackResult = IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT;
    }
    else
    {
        callbackResult = IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
    }
    uploadBlobNumber++;

    (void)Unlock(updateBlobTestLock);

    if (data != NULL)
    {
        // Sleep to not send too much data to server in gated runs.  When data==NULL we don't need this because this callback doesn't result in network I/O
        LogInfo("Sleeping %d milliseconds between upload calls", TEST_SLEEP_BETWEEN_MULTIBLOCK_UPLOADS);
        ThreadAPI_Sleep(TEST_SLEEP_BETWEEN_MULTIBLOCK_UPLOADS);
    }
    return callbackResult;
}

// The non-Ex callback is identical to the Ex, except it is void instead of returning a value to caller.
static void uploadToBlobGetData(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    (void)uploadToBlobGetDataEx(result, data, size, context);
}

static void e2e_uploadtoblob_multiblock_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, bool useExMethod, bool causeAbort)
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
        result = IoTHubClient_UploadMultipleBlocksToBlobAsyncEx(iotHubClientHandle, TEST_UPLOADTOBLOB_DESTINATION_FILE_MULTIBLOCK, uploadToBlobGetDataEx, &g_uploadToBlobStatus);
    }
    else
    {
        result = IoTHubClient_UploadMultipleBlocksToBlobAsync(iotHubClientHandle, TEST_UPLOADTOBLOB_DESTINATION_FILE_MULTIBLOCK, uploadToBlobGetData, &g_uploadToBlobStatus);
    }
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");

    poll_for_upload_completion(&g_uploadToBlobStatus);
    check_upload_result(g_uploadToBlobStatus);

    IoTHubClient_Destroy(iotHubClientHandle);
}

// uploadToBlobTestEarlyClose is the callback passed to SDK that will retrieve uploadToBlob data.  For this test, however, we simply
// will wait a long time before returning.  The main test thread will simultaneously be closing the underlying handle but it'll need to block until we return from this.
static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT uploadToBlobBlockForALongTime(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    if (data != NULL)
    {
        // When data is non-NULL, the SDK is requesting this callback to fill a buffer for it.
        ASSERT_ARE_EQUAL(int, (int)FILE_UPLOAD_OK, (int)result);   
        ASSERT_IS_NOT_NULL(size);
        ASSERT_IS_NOT_NULL(context);

        ASSERT_ARE_EQUAL(int, (int)LOCK_OK, (int)Lock(updateBlobTestLock));
        bool* slowThreadContext = context;
        ASSERT_IS_FALSE(*slowThreadContext);
        (void)Unlock(updateBlobTestLock);

        LogInfo("Slow worker thread created, context=%p.  It will sleep %d milliseconds before return", context, TEST_SLEEP_SLOW_WORKER_THREAD);
        ThreadAPI_Sleep(TEST_SLEEP_SLOW_WORKER_THREAD);
        LogInfo("Slow worker thread sleep finished, context=%p.  Returning to caller.", context);

        // Set the context to indicate to test that we have actually executed this thread.
        ASSERT_ARE_EQUAL(int, (int)LOCK_OK, (int)Lock(updateBlobTestLock));
        *slowThreadContext = true;
        (void)Unlock(updateBlobTestLock);

        // It doesn't matter for the test what code we return, as the worker thread can continue to execute indefinitely as the Destroy() call will block.
        // Since we're testing the SDK close here and not the Hub, just terminate the request.
        return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT;
    }
    else
    {
        // When data is NULL, it indicates the final call of status.  No need to sleep here, just return to finish up test case.
        return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
    }
}

// e2e_uploadtoblob_close_handle_with_active_thread tests that if the application closes the IOTHUB_CLIENT_HANDLE while the worker thread
// is still active, that we block on the destroy until workers have completed.
static void e2e_uploadtoblob_close_handle_with_active_thread(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_PROVISIONED_DEVICE* deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    ASSERT_IS_NOT_NULL(deviceToUse);

    IOTHUB_CLIENT_HANDLE iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, protocol);
    ASSERT_IS_NOT_NULL(iotHubClientHandle, "Could not invoke IoTHubClient_CreateFromConnectionString");

    // If this is not true, it means the test is meaningless (namely the worker thread would be done before the close happens).
    ASSERT_IS_TRUE(TEST_SLEEP_BEFORE_EARLY_HANDLE_CLOSE < TEST_SLEEP_SLOW_WORKER_THREAD);

    // Make the worker thread less aggressive on polling.  We do this during shutdown with a worker thread still active,
    // the default polling interval (1 ms) ends up creating substantial overhead on Valgrind runs in IoTHubClient_Destroy().
    tickcounter_ms_t doWorkFrequency = 100;
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, IoTHubClient_SetOption(iotHubClientHandle, OPTION_DO_WORK_FREQUENCY_IN_MS, &doWorkFrequency));

    testSlowThreadContext = false;

    result = IoTHubClient_UploadMultipleBlocksToBlobAsyncEx(iotHubClientHandle, TEST_UPLOADTOBLOB_DESTINATION_FILE_CLOSE_THREAD, uploadToBlobBlockForALongTime, &testSlowThreadContext);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");
    ThreadAPI_Sleep(TEST_SLEEP_BETWEEN_NEW_UPLOAD_THREAD);
    
    LogInfo("Sleeping %d milliseconds before calling IoTHubClient_Destroy", TEST_SLEEP_BEFORE_EARLY_HANDLE_CLOSE);
    ThreadAPI_Sleep(TEST_SLEEP_BEFORE_EARLY_HANDLE_CLOSE);

    LogInfo("Invoking call to IoTHubClient_Destroy before the worker thread has finished");

    time_t timeWhenDestroyCalled;
    time_t timeWhenDestroyComplete;

    ASSERT_ARE_NOT_EQUAL(int, INDEFINITE_TIME, get_time(&timeWhenDestroyCalled));
    
    // Note that this will block for approximately TEST_SLEEP_SLOW_WORKER_THREAD milliseconds.
    // The SDK cannot return from this call until all the worker threads have finished execution (or else there would be a crash when worker accessed handle).
    // This test simply makes sure this there's no crashes in this and we eventually unblock from this.
    IoTHubClient_Destroy(iotHubClientHandle);

    ASSERT_ARE_NOT_EQUAL(int, INDEFINITE_TIME, get_time(&timeWhenDestroyComplete));

    double timeBetweenStartAndComplete = get_difftime(timeWhenDestroyComplete, timeWhenDestroyCalled);
    ASSERT_ARE_EQUAL(bool, true, timeBetweenStartAndComplete < TEST_MAXIMUM_TIME_FOR_DESTROY_ON_BLOCKED_THREADS_TO_COMPLETE_SECONDS, "It took %f seconds for IoTHubClient_Destroy to complete but maximum allowed is %d",
                                 timeBetweenStartAndComplete, TEST_MAXIMUM_TIME_FOR_DESTROY_ON_BLOCKED_THREADS_TO_COMPLETE_SECONDS);

    // Checking these contexts were set to true by the running thread verifies that the threads run and that they indeed have returned.
    ASSERT_ARE_EQUAL(int, (int)LOCK_OK, (int)Lock(updateBlobTestLock));
    ASSERT_IS_TRUE(testSlowThreadContext, "Worker thread's callback uploadToBlobBlockForALongTime never signaled its context");
    (void)Unlock(updateBlobTestLock);

    LogInfo("Returned from IoTHubClient_Destroy");
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
TEST_FUNCTION(IoTHub_MQTT_UploadMultipleBlocksToBlobEx)
{
    e2e_uploadtoblob_multiblock_test(MQTT_Protocol, true, false);
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_MQTT_UploadToBlob_x509)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_X509);
}
#endif

TEST_FUNCTION(IoTHub_MQTT_UploadCloseHandle_Before_WorkersComplete)
{
    e2e_uploadtoblob_close_handle_with_active_thread(MQTT_Protocol);
}
#endif // TEST_MQTT

END_TEST_SUITE(iothubclient_uploadtoblob_e2e)

