// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <time.h>
#include <stdbool.h>
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_core_ll.h"
#include "azure_c_shared_utility/shared_util_options.h"

#ifdef USE_MQTT
#include "iothubtransportmqtt.h"
#include "iothubtransportmqtt_websockets.h"
#endif

#ifdef USE_AMQP
#include "iothubtransportamqp.h"
#include "iothubtransportamqp_websockets.h"
#endif

#ifdef USE_HTTP
#include "iothubtransporthttp.h"
#endif

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#define MAX_CONNECT_CALLBACK_WAIT_TIME        10

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

typedef struct CONNECTION_STATUS_DATA_TAG
{
    bool status_set;
    IOTHUB_CLIENT_CONNECTION_STATUS current_status;
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON currentStatusReason;
    IOTHUB_CLIENT_CONFIRMATION_RESULT current_confirmation;
} CONNECTION_STATUS_INFO;

BEGIN_TEST_SUITE(iothub_invalidcert_e2e)

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* user_ctx)
{
    CONNECTION_STATUS_INFO* conn_status = (CONNECTION_STATUS_INFO*)user_ctx;
    ASSERT_IS_NOT_NULL(conn_status, "connection status callback context is NULL");

    conn_status->status_set = true;
    conn_status->current_confirmation = result;
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_ctx)
{
    CONNECTION_STATUS_INFO* conn_status = (CONNECTION_STATUS_INFO*)user_ctx;
    ASSERT_IS_NOT_NULL(conn_status, "connection status callback context is NULL");

    conn_status->status_set = true;
    conn_status->current_status = status;
    conn_status->currentStatusReason = reason;
}

static IOTHUB_DEVICE_CLIENT_LL_HANDLE create_client(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, CONNECTION_STATUS_INFO* conn_status)
{
    const char* connection_string = getenv("IOTHUB_DEVICE_CONN_STRING_INVALIDCERT");

    IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_handle;
    iothub_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connection_string, protocol);
    ASSERT_IS_NOT_NULL(iothub_handle, "Could not create IoTHubDeviceClient_LL_CreateFromConnectionString");

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    IoTHubDeviceClient_LL_SetOption(iothub_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothub_handle, connection_status_callback, conn_status);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set connection Status Callback");

    return iothub_handle;
}

static void wait_for_unauthorized_send(IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_handle, CONNECTION_STATUS_INFO* conn_status)
{
    time_t beginOperation, nowTime;

    beginOperation = time(NULL);
    do
    {
        IoTHubDeviceClient_LL_DoWork(dev_handle);
    } while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CONNECT_CALLBACK_WAIT_TIME) && (!conn_status->status_set) // time box
        );
    ASSERT_IS_TRUE(conn_status->status_set, "Status callback did not get executed");
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_ERROR, conn_status->current_confirmation, "Sending message was successful and should not have been");
}

static void wait_for_unauthorized_connection(IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_handle, CONNECTION_STATUS_INFO* conn_status)
{
    time_t beginOperation, nowTime;

    beginOperation = time(NULL);
    do
    {
        IoTHubDeviceClient_LL_DoWork(dev_handle);
        if (conn_status->status_set)
        {
            printf("Status has been set\r\n");
        }
    } while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CONNECT_CALLBACK_WAIT_TIME) && (!conn_status->status_set) // time box
        );
    ASSERT_IS_TRUE(conn_status->status_set, "Status callback did not get executed");
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, conn_status->current_status, "Connection was successful and should not have been");
}

static void run_invalidcert_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    CONNECTION_STATUS_INFO conn_status;
    conn_status.status_set = false;
    conn_status.current_status = IOTHUB_CLIENT_CONNECTION_AUTHENTICATED;
    conn_status.currentStatusReason = IOTHUB_CLIENT_CONNECTION_OK;

    IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_handle = create_client(protocol, &conn_status);

    if (protocol == HTTP_Protocol)
    {
        IOTHUB_MESSAGE_HANDLE message_handle;

        // HTTP must send a message to connect
        message_handle = IoTHubMessage_CreateFromString("dummy message");
        IoTHubDeviceClient_LL_SendEventAsync(dev_handle, message_handle, send_confirm_callback, &conn_status);
        IoTHubMessage_Destroy(message_handle);

        wait_for_unauthorized_send(dev_handle, &conn_status);
    }
    else
    {
        wait_for_unauthorized_connection(dev_handle, &conn_status);
    }

    IoTHubDeviceClient_LL_Destroy(dev_handle);
}

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    int result = IoTHub_Init();
    ASSERT_ARE_EQUAL(int, 0, result, "Iothub init failed");
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    IoTHub_Deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

#ifdef USE_MQTT
TEST_FUNCTION(IoTHub_MQTT_trusted_cert_e2e)
{
    run_invalidcert_test(MQTT_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_WS_trusted_cert_e2e)
{
    run_invalidcert_test(MQTT_WebSocket_Protocol);
}
#endif

/*#ifdef USE_AMQP
TEST_FUNCTION(IoTHub_AMQP_trusted_cert_e2e)
{
    run_invalidcert_test(AMQP_Protocol);
}

TEST_FUNCTION(IoTHub_AMQP_WS_trusted_cert_e2e)
{
    run_invalidcert_test(AMQP_Protocol_over_WebSocketsTls);
}
#endif*/

#ifdef USE_HTTP
TEST_FUNCTION(IoTHub_HTTP_trusted_cert_e2e)
{
    run_invalidcert_test(HTTP_Protocol);
}
#endif

END_TEST_SUITE(iothub_invalidcert_e2e)
