// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>
#include <string.h>

#include "iothub_messaging_ll.h"

#include "umock_c.h"
#include "testrunnerswitcher.h"

#include "iothub_account.h"
#include "iothubtest.h"

#include "iothub_client_options.h"

#include "azure_c_shared_utility/lock.h"

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "iothub_service_client_auth.h"
#include "iothub_devicemethod.h"
#include "iothub_client.h"

#include "iothubclient_common_device_method_e2e.h"

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(IOTHUB_DEVICE_METHOD_RESULT, IOTHUB_DEVICE_METHOD_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);

typedef struct IOTHUB_CONN_INFO_TAG
{
    IOTHUB_CLIENT_CONNECTION_STATUS conn_status;
    LOCK_HANDLE lock;
} IOTHUB_CONN_INFO;

static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo = NULL;
static IOTHUB_CONN_INFO g_conn_info;

void device_method_e2e_init(void)
{
    ASSERT_ARE_EQUAL(int, 0, platform_init() );

    g_iothubAcctInfo = IoTHubAccount_Init(true);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo);

    // Initialize locking
    g_conn_info.lock = Lock_Init();
    ASSERT_IS_NOT_NULL(g_conn_info.lock);
}

void device_method_e2e_deinit(void)
{
    IoTHubAccount_deinit(g_iothubAcctInfo);

    // Need a double deinit
    platform_deinit();
    platform_deinit();

    Lock_Deinit(g_conn_info.lock);
}

static const char *METHOD_NAME = "MethodName";
static const int METHOD_RESPONSE_SUCCESS = 201;
static const int METHOD_RESPONSE_ERROR = 401;
static const unsigned int TIMEOUT = 60;
static const unsigned int IOTHUB_CONNECT_TIMEOUT_SEC = 30;

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    if (reason == IOTHUB_CLIENT_CONNECTION_OK)
    {
        IOTHUB_CONN_INFO* conn_info = (IOTHUB_CONN_INFO*)userContextCallback;
        if (Lock(conn_info->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            conn_info->conn_status = result;
            (void)Unlock(conn_info->lock);
        }
    }
}

static int DeviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
    int responseCode;
    const char * expectedMethodPayload = (const char*)userContextCallback;

    if (strcmp(METHOD_NAME, method_name))
    {
        LogError("Method name incorrect - expected %s but got %s", METHOD_NAME, method_name);
        responseCode = METHOD_RESPONSE_ERROR;
    }
    else if (size != strlen(expectedMethodPayload))
    {
        LogError("payload size incorect - expected %zu but got %zu", strlen(expectedMethodPayload), size);
        responseCode = METHOD_RESPONSE_ERROR;
    }
    else if (memcmp(payload, expectedMethodPayload, size))
    {
        LogError("Payload strings do not match");
        responseCode = METHOD_RESPONSE_ERROR;
    }
    else
    {
        *resp_size = size;
        if (size == 0)
        {
            *response = NULL;
            responseCode = METHOD_RESPONSE_SUCCESS;
        }
        else
        {
            if ((*response = (unsigned char*)malloc(*resp_size)) == NULL)
            {
                LogError("allocation failure");
                responseCode = METHOD_RESPONSE_ERROR;
            }
            else
            {
                memcpy(*response, payload, *resp_size);
                responseCode = METHOD_RESPONSE_SUCCESS;
            }
        }
    }
    return responseCode;
}

static IOTHUB_CLIENT_HANDLE iotHubClientHandle = NULL;
static IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = NULL;
static IOTHUB_SERVICE_CLIENT_DEVICE_METHOD_HANDLE serviceClientDeviceMethodHandle = NULL;

void test_device_method_with_string(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char *payload)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_CONFIG iotHubConfig = { 0 };
    time_t beginOperation, nowTime;
    bool trace = true;

    iotHubConfig.iotHubName = IoTHubAccount_GetIoTHubName(g_iothubAcctInfo);
    iotHubConfig.iotHubSuffix = IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo);
    iotHubConfig.deviceId = IoTHubAccount_GetDeviceId(g_iothubAcctInfo);
    iotHubConfig.deviceKey = IoTHubAccount_GetDeviceKey(g_iothubAcctInfo);
    iotHubConfig.protocol = protocol;
    
    g_conn_info.conn_status = IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED;

    iotHubClientHandle = IoTHubClient_Create(&iotHubConfig);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubClientHandle, "Could not create IoTHubClient");

    result = IoTHubClient_SetConnectionStatusCallback(iotHubClientHandle, connection_status_callback, &g_conn_info);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set connection Status Callback");

    // Turn on Log 
    (void)IoTHubClient_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &trace);

    result = IoTHubClient_SetDeviceMethodCallback(iotHubClientHandle, DeviceMethodCallback, (void*)payload);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device method callback");


    bool continue_running = true;
    beginOperation = time(NULL);
    do
    {
        if (Lock(g_conn_info.lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            if (g_conn_info.conn_status == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
            {
                continue_running = false;
            }
            (void)Unlock(g_conn_info.lock);
        }

        ThreadAPI_Sleep(10);
    } while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < IOTHUB_CONNECT_TIMEOUT_SEC) &&
        (continue_running)
      );
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, g_conn_info.conn_status, "Device Not connected");

    // Wait for the method to subscribe
    ThreadAPI_Sleep(1 * 1000);
    
    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubServiceClientHandle, "Could not create service client handle");
    
    serviceClientDeviceMethodHandle = IoTHubDeviceMethod_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL_WITH_MSG(serviceClientDeviceMethodHandle, "Could not create device method handle");

    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;
    IOTHUB_DEVICE_METHOD_RESULT invokeResult = IoTHubDeviceMethod_Invoke(serviceClientDeviceMethodHandle, iotHubConfig.deviceId, METHOD_NAME, payload, TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_DEVICE_METHOD_RESULT, IOTHUB_DEVICE_METHOD_OK, invokeResult, "Service Client IoTHubDeviceMethod_Invoke failed");
    ASSERT_ARE_EQUAL_WITH_MSG(int, METHOD_RESPONSE_SUCCESS, responseStatus, "response status is incorrect");

    ASSERT_ARE_EQUAL_WITH_MSG(size_t, strlen(payload), responsePayloadSize, "response size is incorrect");
    if (memcmp(payload, responsePayload, responsePayloadSize))
    {
        ASSERT_FAIL("response string does not match");
    }

    free(responsePayload);
}

void device_method_function_cleanup()
{
    if (serviceClientDeviceMethodHandle != NULL)
    {
        IoTHubDeviceMethod_Destroy(serviceClientDeviceMethodHandle);
        serviceClientDeviceMethodHandle = NULL;
    }

    if (iotHubServiceClientHandle != NULL)
    {
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
        iotHubServiceClientHandle = NULL;
    }

    if (iotHubClientHandle != NULL)
    {
        IoTHubClient_Destroy(iotHubClientHandle);
        iotHubClientHandle = NULL;
    }
}

void device_method_e2e_method_call_with_string(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(protocol, "\"I'm a happy little string\"");
}

void device_method_e2e_method_call_with_double_quoted_json(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(protocol, "{\"foo\":41,\"bar\":42,\"baz\":\"boo\"}");
}

void device_method_e2e_method_call_with_empty_json_object(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(protocol, "{}");
}

void device_method_e2e_method_call_with_null(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(protocol, "null");
}

extern void device_method_e2e_method_call_with_embedded_double_quote(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    (void)protocol;
    test_device_method_with_string(protocol, "\"this string has a double quote \\\" in the middle\"");
}

extern void device_method_e2e_method_call_with_embedded_single_quote(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    (void)protocol;
    test_device_method_with_string(protocol, "\"this string has a single quote ' in the middle\"");
}

