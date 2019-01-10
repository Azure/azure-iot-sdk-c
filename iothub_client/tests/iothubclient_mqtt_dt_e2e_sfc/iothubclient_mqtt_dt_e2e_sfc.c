// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_dt_e2e.h"
#include "iothubtransportmqtt.h"

BEGIN_TEST_SUITE(iothubclient_mqtt_dt_e2e_sfc)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    dt_e2e_init(false);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    dt_e2e_deinit();
}

TEST_FUNCTION(IoTHub_MQTT_SendReported_e2e_svc_fault_ctrl_kill_Tcp)
{
    dt_e2e_send_reported_test_svc_fault_ctrl_kill_Tcp(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

TEST_FUNCTION(IoTHub_MQTT_GetFullDesired_e2e_svc_fault_ctrl_kill_Tcp)
{
    dt_e2e_get_complete_desired_test_svc_fault_ctrl_kill_Tcp(MQTT_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

END_TEST_SUITE(iothubclient_mqtt_dt_e2e_sfc)
