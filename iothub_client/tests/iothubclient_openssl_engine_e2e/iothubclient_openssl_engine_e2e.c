// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <time.h>
#include <stdbool.h>
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "iothub_account.h"
#include "iothub_client_options.h"

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

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#define MAX_CONNECT_CALLBACK_WAIT_TIME        10
#define OPENSSL_ENGINE_ID "pkcs11"
#define PKCS11_PRIVATE_KEY_URI "pkcs11:object=test-privkey;type=private?pin-value=1234"

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

IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo = NULL;

BEGIN_TEST_SUITE(iothubclient_openssl_engine_e2e)

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
    IOTHUB_PROVISIONED_DEVICE* deviceToUse = IoTHubAccount_GetX509Device(g_iothubAcctInfo);

    static const char* x509privatekey = PKCS11_PRIVATE_KEY_URI;
    static const char* engineId = OPENSSL_ENGINE_ID;
    static const OPTION_OPENSSL_KEY_TYPE x509keyengine = KEY_TYPE_ENGINE;

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_client;
    device_client = IoTHubDeviceClient_LL_CreateFromConnectionString(deviceToUse->connectionString, protocol);
    ASSERT_IS_NOT_NULL(device_client, "Could not create IoTHubDeviceClient_LL_CreateFromConnectionString");

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    IoTHubDeviceClient_LL_SetOption(device_client, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_client, connection_status_callback, conn_status);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set connection Status Callback");

    result = IoTHubDeviceClient_LL_SetOption(device_client, OPTION_OPENSSL_ENGINE, engineId);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not load OpenSSL ENGINE");

    result = IoTHubDeviceClient_LL_SetOption(device_client, OPTION_OPENSSL_PRIVATE_KEY_TYPE, &x509keyengine);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not configure private key type");

    IoTHubDeviceClient_LL_SetOption(device_client, OPTION_X509_CERT, deviceToUse->certificate);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not configure certificate");

    IoTHubDeviceClient_LL_SetOption(device_client, OPTION_X509_PRIVATE_KEY, x509privatekey);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not configure private key");

    return device_client;
}

static void wait_for_connection(IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_handle, CONNECTION_STATUS_INFO* conn_status)
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

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, conn_status->current_status, "Connection using OpenSSL Engine " OPENSSL_ENGINE_ID " not successful");
}

static void run_openssl_engine_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    CONNECTION_STATUS_INFO conn_status;
    conn_status.status_set = false;
    conn_status.current_status = IOTHUB_CLIENT_CONNECTION_AUTHENTICATED;
    conn_status.currentStatusReason = IOTHUB_CLIENT_CONNECTION_OK;

    IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_handle = create_client(protocol, &conn_status);

    wait_for_connection(dev_handle, &conn_status);

    IoTHubDeviceClient_LL_Destroy(dev_handle);
}

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    int result = IoTHub_Init();
    ASSERT_ARE_EQUAL(int, 0, result, "Iothub init failed");
    g_iothubAcctInfo = IoTHubAccount_Init(false);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo, "Could not initialize IoTHubAccount");
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    IoTHub_Deinit();
    IoTHubAccount_deinit(g_iothubAcctInfo);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

#ifdef USE_MQTT
TEST_FUNCTION(IoTHub_MQTT_openssl_engine_e2e)
{
    run_openssl_engine_test(MQTT_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_WS_trusted_cert_e2e)
{
    run_openssl_engine_test(MQTT_WebSocket_Protocol);
}
#endif

#ifdef USE_AMQP
TEST_FUNCTION(IoTHub_AMQP_openssl_engine_e2e)
{
    run_openssl_engine_test(AMQP_Protocol);
}

TEST_FUNCTION(IoTHub_AMQP_WS_trusted_cert_e2e)
{
    run_openssl_engine_test(AMQP_Protocol_over_WebSocketsTls);
}
#endif

END_TEST_SUITE(iothubclient_openssl_engine_e2e)
