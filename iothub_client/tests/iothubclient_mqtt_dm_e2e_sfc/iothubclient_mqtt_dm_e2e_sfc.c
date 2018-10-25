// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothubclient_common_device_method_e2e.h"
#include "testrunnerswitcher.h"
#include "iothubtransportmqtt.h"
#include "iothub_devicemethod.h"

BEGIN_TEST_SUITE(iothubclient_mqtt_dm_e2e_sfc)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        device_method_e2e_init(false);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        device_method_e2e_deinit();
    }

    TEST_FUNCTION_CLEANUP(TestFunctionCleanup)
    {
        device_method_function_cleanup();
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_svc_fault_ctrl_kill_Tcp)
    {
        device_method_e2e_method_call_svc_fault_ctrl_kill_Tcp(MQTT_Protocol);
    }

    END_TEST_SUITE(iothubclient_mqtt_dm_e2e_sfc)

