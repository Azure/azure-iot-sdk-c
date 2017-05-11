// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportmqtt_websockets.h"

static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(iothubclient_mqtt_ws_e2e_sfc)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        e2e_init();
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        e2e_deinit();
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    //***********************************************************
    // D2C
    //***********************************************************
    //// FAIL - continous retry
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_d2c_svc_fault_ctrl_kill_Tcp)
    //{
    //    e2e_d2c_svc_fault_ctrl_kill_TCP_connection(MQTT_WebSocket_Protocol);
    //}

    //// FAIL - service alive
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_d2c_svc_fault_ctrl_throttling_reconnect)
    //{
    //    e2e_d2c_svc_fault_ctrl_MQTT_throttling_reconnect(MQTT_WebSocket_Protocol);
    //}

    //// FAIL - service alive
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_d2c_svc_fault_ctrl_message_quota_exceeded)
    //{
    //    e2e_d2c_svc_fault_ctrl_MQTT_message_quota_exceeded(MQTT_WebSocket_Protocol);
    //}

    //// FAIL - service alive
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_d2c_svc_fault_ctrl_auth_error)
    //{
    //    e2e_d2c_svc_fault_ctrl_MQTT_auth_error(MQTT_WebSocket_Protocol);
    //}

    //// FAIL - retry failing with error
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_d2c_svc_fault_ctrl_shut_down)
    //{
    //    e2e_d2c_svc_fault_ctrl_MQTT_shut_down(MQTT_WebSocket_Protocol);
    //}

    //***********************************************************
    // C2D
    //***********************************************************
    //// FAIL - continous retry
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_c2d_svc_fault_ctrl_kill_Tcp)
    //{
    //    e2e_c2d_svc_fault_ctrl_kill_TCP_connection(MQTT_WebSocket_Protocol);
    //}

    //// FAIL - service alive
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_c2d_svc_fault_ctrl_throttling_reconnect)
    //{
    //    e2e_c2d_svc_fault_ctrl_throttling_reconnect(MQTT_WebSocket_Protocol);
    //}

    //// FAIL - service alive
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_c2d_svc_fault_ctrl_message_quota_exceeded)
    //{
    //    e2e_c2d_svc_fault_ctrl_message_quota_exceeded(MQTT_WebSocket_Protocol);
    //}

    //// FAIL - service alive
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_c2d_svc_fault_ctrl_auth_error)
    //{
    //    e2e_c2d_svc_fault_ctrl_auth_error(MQTT_WebSocket_Protocol);
    //}

    //// FAIL - retry failing with error
    //TEST_FUNCTION(IoTHub_MQTT_WS_e2e_c2d_svc_fault_ctrl_shut_down)
    //{
    //    e2e_c2d_svc_fault_ctrl_MQTT_shut_down(MQTT_WebSocket_Protocol);
    //}

END_TEST_SUITE(iothubclient_mqtt_ws_e2e_sfc)
