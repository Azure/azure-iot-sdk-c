// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <time.h>
#include <stdbool.h>
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#include "azure_c_shared_utility/platform.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"

#ifdef USE_MQTT
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"
#endif

#ifdef USE_AMQP
#include "azure_prov_client/prov_transport_amqp_client.h"
#include "azure_prov_client/prov_transport_amqp_ws_client.h"
#endif

#ifdef USE_HTTP
#include "azure_prov_client/prov_transport_http_client.h"
#endif

#define MAX_CONNECT_CALLBACK_WAIT_TIME        10

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);

typedef struct CONNECTION_STATUS_DATA_TAG
{
    bool status_set;
    PROV_DEVICE_RESULT current_result;
} CONNECTION_STATUS_INFO;

BEGIN_TEST_SUITE(prov_invalidcert_e2e)

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_ctx)
{
    CONNECTION_STATUS_INFO* conn_status = (CONNECTION_STATUS_INFO*)user_ctx;
    ASSERT_IS_NOT_NULL(conn_status, "connection status callback context is NULL");
    ASSERT_IS_NULL(iothub_uri, "iothub_uri is not NULL");
    ASSERT_IS_NULL(device_id, "device_id is not NULL");

    conn_status->status_set = true;
    conn_status->current_result = register_result;
}

static PROV_DEVICE_LL_HANDLE create_client(PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport, CONNECTION_STATUS_INFO* conn_status)
{
    const char* prov_uri = getenv("DPS_GLOBALDEVICEENDPOINT_INVALIDCERT");
    const char* id_scope = "dummy_id_scope";

    PROV_DEVICE_LL_HANDLE prov_handle;
    prov_handle = Prov_Device_LL_Create(prov_uri, id_scope, prov_transport);
    ASSERT_IS_NOT_NULL(prov_handle, "Could not create handle with Prov_Device_LL_Create");

    Prov_Device_LL_Register_Device(prov_handle, register_device_callback, conn_status, NULL, NULL);

    return prov_handle;
}

static void wait_for_unauthorized_connection(PROV_DEVICE_LL_HANDLE prov_handle, CONNECTION_STATUS_INFO* conn_status)
{
    time_t beginOperation, nowTime;

    beginOperation = time(NULL);
    do
    {
        Prov_Device_LL_DoWork(prov_handle);
    } while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CONNECT_CALLBACK_WAIT_TIME) && (!conn_status->status_set) // time box
        );
    ASSERT_IS_TRUE(conn_status->status_set, "Status callback did not get executed");
    ASSERT_ARE_NOT_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, conn_status->current_result, "Connection was successful and should not have been");
}

static void run_invalidcert_test(PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport)
{
    CONNECTION_STATUS_INFO conn_status;
    conn_status.status_set = false;
    conn_status.current_result = PROV_DEVICE_RESULT_OK;

    PROV_DEVICE_LL_HANDLE dev_handle = create_client(prov_transport, &conn_status);

    wait_for_unauthorized_connection(dev_handle, &conn_status);

    Prov_Device_LL_Destroy(dev_handle);
}

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    platform_init();
    prov_dev_security_init(SECURE_DEVICE_TYPE_X509);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    prov_dev_security_deinit();
    platform_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

#ifdef USE_MQTT
TEST_FUNCTION(IoTHub_MQTT_trusted_cert_e2e)
{
    run_invalidcert_test(Prov_Device_MQTT_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_WS_trusted_cert_e2e)
{
    run_invalidcert_test(Prov_Device_MQTT_WS_Protocol);
}
#endif

#ifdef USE_AMQP
TEST_FUNCTION(IoTHub_AMQP_trusted_cert_e2e)
{
    run_invalidcert_test(Prov_Device_AMQP_Protocol);
}

TEST_FUNCTION(IoTHub_AMQP_WS_trusted_cert_e2e)
{
    run_invalidcert_test(Prov_Device_AMQP_Protocol);
}
#endif

#ifdef USE_HTTP
TEST_FUNCTION(IoTHub_HTTP_trusted_cert_e2e)
{
    run_invalidcert_test(Prov_Device_HTTP_Protocol);
}
#endif

END_TEST_SUITE(prov_invalidcert_e2e)
