// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportmqtt_websockets.h"

BEGIN_TEST_SUITE(iothubclient_mqtt_ws_e2e)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    e2e_init(TEST_MQTT_WEBSOCKETS, false);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    e2e_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    g_e2e_test_options.use_special_chars = false;
}

TEST_FUNCTION(IoTHub_MQTT_SendEvent_e2e_sas)
{
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = true;
#endif
    e2e_send_event_test_sas(MQTT_WebSocket_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_SendEvent_e2e_sas_urlEncode)
{
#ifdef AZIOT_LINUX
    g_e2e_test_options.set_mac_address = true;
#endif
    g_e2e_test_options.use_special_chars = true;
    e2e_send_event_test_sas(MQTT_WebSocket_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_RecvMessage_E2ETest_sas)
{
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = false;
#endif
    e2e_recv_message_test_sas(MQTT_WebSocket_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_RecvMessage_E2ETest_sas_urlDecode)
{
#ifdef AZIOT_LINUX
    g_e2e_test_options.set_mac_address = false;
#endif
    g_e2e_test_options.use_special_chars = true;
    e2e_recv_message_test_sas(MQTT_WebSocket_Protocol);
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_MQTT_SendEvent_e2e_x509)
{
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = true;
#endif
    e2e_send_event_test_x509(MQTT_WebSocket_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_SendEvent_e2e_x509_urlEncode)
{
#ifdef AZIOT_LINUX
    g_e2e_test_options.set_mac_address = true;
#endif
    g_e2e_test_options.use_special_chars = true;
    e2e_send_event_test_x509(MQTT_WebSocket_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_RecvMessage_E2ETest_x509)
{
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = false;
#endif
    e2e_recv_message_test_x509(MQTT_WebSocket_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_RecvMessage_E2ETest_x509_urlDecode)
{
#ifdef AZIOT_LINUX
    g_e2e_test_options.set_mac_address = false;
#endif
    g_e2e_test_options.use_special_chars = true;
    e2e_recv_message_test_x509(MQTT_WebSocket_Protocol);
}
#endif
END_TEST_SUITE(iothubclient_mqtt_ws_e2e)
