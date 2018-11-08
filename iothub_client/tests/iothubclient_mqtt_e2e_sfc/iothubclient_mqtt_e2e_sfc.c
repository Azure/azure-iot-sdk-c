// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportmqtt.h"

BEGIN_TEST_SUITE(iothubclient_mqtt_e2e_sfc)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        e2e_init(TEST_MQTT, false);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        e2e_deinit();
    }

    TEST_FUNCTION(IoTHub_MQTT_e2e_d2c_svc_fault_ctrl_message_quota_exceeded)
    {
        e2e_d2c_svc_fault_ctrl_MQTT_message_quota_exceeded(MQTT_Protocol);
    }

    TEST_FUNCTION(IoTHub_MQTT_e2e_d2c_svc_fault_ctrl_auth_error)
    {
        e2e_d2c_svc_fault_ctrl_MQTT_auth_error(MQTT_Protocol);
    }

    TEST_FUNCTION(IoTHub_MQTT_e2e_d2c_svc_fault_ctrl_throttling_reconnect)
    {
        e2e_d2c_svc_fault_ctrl_MQTT_throttling_reconnect(MQTT_Protocol);
    }

    // ***********************************************************
    // D2C
    // ***********************************************************
    TEST_FUNCTION(IoTHub_MQTT_e2e_d2c_svc_fault_ctrl_kill_Tcp)
    {
        e2e_d2c_svc_fault_ctrl_kill_TCP_connection(MQTT_Protocol);
    }

    TEST_FUNCTION(IoTHub_MQTT_e2e_d2c_svc_fault_ctrl_shut_down)
    {
        e2e_d2c_svc_fault_ctrl_MQTT_shut_down(MQTT_Protocol);
    }

    // ***********************************************************
    // C2D
    // ***********************************************************
    TEST_FUNCTION(IoTHub_MQTT_e2e_c2d_svc_fault_ctrl_kill_Tcp)
    {
        e2e_c2d_svc_fault_ctrl_kill_TCP_connection(MQTT_Protocol);
    }

    TEST_FUNCTION(IoTHub_MQTT_e2e_c2d_svc_fault_ctrl_throttling_reconnect)
    {
        e2e_c2d_svc_fault_ctrl_throttling_reconnect(MQTT_Protocol);
    }

    TEST_FUNCTION(IoTHub_MQTT_e2e_c2d_svc_fault_ctrl_message_quota_exceeded)
    {
        e2e_c2d_svc_fault_ctrl_message_quota_exceeded(MQTT_Protocol);
    }

    TEST_FUNCTION(IoTHub_MQTT_e2e_c2d_svc_fault_ctrl_auth_error)
    {
        e2e_c2d_svc_fault_ctrl_auth_error(MQTT_Protocol);
    }

    TEST_FUNCTION(IoTHub_MQTT_e2e_c2d_svc_fault_ctrl_shut_down)
    {
        e2e_c2d_svc_fault_ctrl_MQTT_shut_down(MQTT_Protocol);
    }

END_TEST_SUITE(iothubclient_mqtt_e2e_sfc)
