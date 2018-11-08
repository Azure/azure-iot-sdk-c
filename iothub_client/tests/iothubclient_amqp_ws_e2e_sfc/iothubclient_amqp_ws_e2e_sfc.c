// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportamqp_websockets.h"

BEGIN_TEST_SUITE(iothubclient_amqp_ws_e2e_sfc)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        e2e_init(TEST_AMQP_WEBSOCKETS, false);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        e2e_deinit();
    }

    //***********************************************************
    // D2C
    //***********************************************************
    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_kill_Tcp)
    {
        e2e_d2c_svc_fault_ctrl_kill_TCP_connection_with_transport_status_check(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_kill_AMQP_connection)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_kill_connection(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_kill_AMQP_session)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_kill_session(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_kill_AMQP_CBS_request_link)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_kill_CBS_request_link(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_kill_AMQP_CBS_response_link)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_kill_CBS_response_link(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_kill_AMQP_D2C_link)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_kill_D2C_link(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_kill_AMQP_C2D_link)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_kill_C2D_link(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_throttling_reconnect)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_throttling_reconnect(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_message_quota_exceeded)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_message_quota_exceeded(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_auth_error)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_auth_error(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_d2c_svc_fault_ctrl_kill_AMQP_shut_down)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_shut_down(AMQP_Protocol_over_WebSocketsTls);
    }

    //***********************************************************
    // C2D
    //***********************************************************
    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_kill_Tcp)
    {
        e2e_c2d_svc_fault_ctrl_kill_TCP_connection(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_kill_AMQP_connection)
    {
        e2e_c2d_svc_fault_ctrl_AMQP_kill_connection(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_kill_AMQP_session)
    {
        e2e_c2d_svc_fault_ctrl_AMQP_kill_session(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_kill_AMQP_CBS_request_link)
    {
        e2e_c2d_svc_fault_ctrl_AMQP_kill_CBS_request_link(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_kill_AMQP_CBS_response_link)
    {
        e2e_c2d_svc_fault_ctrl_AMQP_kill_CBS_response_link(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_kill_AMQP_D2C_link)
    {
        e2e_c2d_svc_fault_ctrl_AMQP_kill_D2C_link(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_kill_AMQP_C2D_link)
    {
        e2e_c2d_svc_fault_ctrl_AMQP_kill_C2D_link(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_throttling_reconnect)
    {
        e2e_c2d_svc_fault_ctrl_throttling_reconnect(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_message_quota_exceeded)
    {
        e2e_c2d_svc_fault_ctrl_message_quota_exceeded(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_auth_error)
    {
        e2e_c2d_svc_fault_ctrl_auth_error(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_WS_e2e_c2d_svc_fault_ctrl_kill_AMQP_shut_down)
    {
        e2e_d2c_svc_fault_ctrl_AMQP_shut_down(AMQP_Protocol_over_WebSocketsTls);
    }

END_TEST_SUITE(iothubclient_amqp_ws_e2e_sfc)
