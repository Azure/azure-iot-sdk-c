// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportamqp_websockets.h"

BEGIN_TEST_SUITE(iothubclient_amqp_ws_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        e2e_init(TEST_AMQP_WEBSOCKETS, false);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        e2e_deinit();
    }

    TEST_FUNCTION(IoTHub_AMQP_SendEvent_e2e_sas)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = false;
#endif
        e2e_send_event_test_sas(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_RecvMessage_E2ETest_sas)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = true;
#endif
        e2e_recv_message_test_sas(AMQP_Protocol_over_WebSocketsTls);
    }

#ifndef __APPLE__
    TEST_FUNCTION(IoTHub_AMQP_SendEvent_e2e_x509)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = true;
#endif
        e2e_send_event_test_x509(AMQP_Protocol_over_WebSocketsTls);
    }

    TEST_FUNCTION(IoTHub_AMQP_RecvMessage_E2ETest_x509)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = false;
#endif
        e2e_recv_message_test_x509(AMQP_Protocol_over_WebSocketsTls);
    }
#endif
END_TEST_SUITE(iothubclient_amqp_ws_e2e)
