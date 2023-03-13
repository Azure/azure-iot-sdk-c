// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportamqp.h"

BEGIN_TEST_SUITE(iothubclient_amqp_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        e2e_init(TEST_AMQP, false);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        e2e_deinit();
    }

    TEST_FUNCTION(IoTHub_AMQP_SendEvent_e2e_sas)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = true;
#endif
        e2e_send_event_test_sas(AMQP_Protocol);
    }

    TEST_FUNCTION(IoTHub_AMQP_SendBatchEvent_e2e_sas)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = true;
#endif
        IOTHUB_GATEWAY_VERSION iotHubVersion = IoTHubAccount_GetIoTHubVersion(g_iothubAcctInfo);

        ASSERT_ARE_NOT_EQUAL(int, IOTHUB_GATEWAY_VERSION_UNDEFINED, iotHubVersion);

        if (iotHubVersion == IOTHUB_GATEWAY_VERSION_2)
        {
            // Disbling this test on IoT Hub Gateway V2
            // batching size is no longer supported
            //e2e_send_batch_event_test_sas(AMQP_Protocol);
        }
        else
        {
            // Skipping the test if not running against IoT Hub Gateway V2.
        }
    }

    TEST_FUNCTION(IoTHub_AMQP_RecvMessage_E2ETest_sas)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = false;
#endif
        e2e_recv_message_test_sas(AMQP_Protocol);
    }

#ifndef __APPLE__
    TEST_FUNCTION(IoTHub_AMQP_SendEvent_e2e_x509)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = false;
#endif
        e2e_send_event_test_x509(AMQP_Protocol);
    }

    TEST_FUNCTION(IoTHub_AMQP_RecvMessage_E2ETest_x509)
    {
#ifdef AZIOT_LINUX
        g_e2e_test_options.set_mac_address = true;
#endif
        e2e_recv_message_test_x509(AMQP_Protocol);
    }
#endif

END_TEST_SUITE(iothubclient_amqp_e2e)
