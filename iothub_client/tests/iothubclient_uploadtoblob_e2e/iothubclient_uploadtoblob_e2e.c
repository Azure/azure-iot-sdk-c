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
#include "../../../certs/certs.h"

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

static UPLOADTOBLOB_CALLBACK_STATUS uploadToBlobStatus;

static LOCK_HANDLE updateBlobTestLock;

#define IOTHUB_UPLOADTOBLOB_TIMEOUT_SEC 30

DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(UPLOADTOBLOB_CALLBACK_STATUS, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

#define UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE "hello_world.txt"
#define UPLOADTOBLOB_E2E_TEST_DATA (const unsigned char*)"e2e_UPLOADTOBLOB_CALLBACK test data"

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
        LogInfo("uploadToBlobCallback(%s)", ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, fileUploadResult));

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

    uploadToBlobStatus = UPLOADTOBLOB_CALLBACK_PENDING;
    result = IoTHubClient_UploadToBlobAsync(iotHubClientHandle, UPLOADTOBLOB_E2E_TEST_DESTINATION_FILE, UPLOADTOBLOB_E2E_TEST_DATA, sizeof(UPLOADTOBLOB_E2E_TEST_DATA) - 1, uploadToBlobCallback, &uploadToBlobStatus);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubClient_UploadToBlobAsync");

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
            if (uploadToBlobStatus != UPLOADTOBLOB_CALLBACK_PENDING)
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
TEST_FUNCTION(IoTHub_MQTT_UploadToBlob_sas)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

TEST_FUNCTION(IoTHub_MQTT_WS_UploadToBlob_sas)
{
    e2e_uploadtoblob_test(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
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

