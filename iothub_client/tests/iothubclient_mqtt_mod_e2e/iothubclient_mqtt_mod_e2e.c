// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportmqtt.h"
#include "iothubtransportmqtt_websockets.h"


BEGIN_TEST_SUITE(iothubclient_mqtt_mod_e2e)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    e2e_init(TEST_MQTT, true);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    e2e_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    g_e2e_test_options.use_special_chars = false;
    g_e2e_test_options.set_mac_address = false;
}

TEST_FUNCTION(IoTHub_MQTT_Module_SendEvent_e2e_sas)
{
    e2e_send_event_test_sas(MQTT_Protocol);
}

// Unlike the device client equivalent E2E tests, there is no call e2e_recv_message_test_sas(MQTT_Protocol).
// Using C2D with modules is not supported.

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_MQTT_Module_SendEvent_e2e_x509)
{
    e2e_send_event_test_x509(MQTT_Protocol);
}
#endif

TEST_FUNCTION(IoTHub_MQTT_Module_SendSecurityEvent_e2e_sas)
{
    e2e_send_security_event_test_sas(MQTT_Protocol);
}

#ifndef USE_WOLFSSL // Wolf doesn't run web socket tests
TEST_FUNCTION(IoTHub_MQTT_WS_Module_SendEvent_e2e_sas)
{
    e2e_send_event_test_sas(MQTT_WebSocket_Protocol);
}

// Unlike the device client equivalent E2E tests, there is no call e2e_recv_message_test_sas(MQTT_Protocol).
// Using C2D with modules is not supported.

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_MQTT_WS_Module_SendEvent_e2e_x509)
{
    e2e_send_event_test_x509(MQTT_WebSocket_Protocol);
}
#endif // __APPLE__
#endif // USE_WOLFSSL


END_TEST_SUITE(iothubclient_mqtt_mod_e2e)
