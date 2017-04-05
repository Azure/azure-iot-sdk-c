// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#endif

#include "testrunnerswitcher.h"
#include "badnetwork.h"
#include "iothubtest.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportamqp.h"
#include "network_disconnect.h"

static TEST_MUTEX_HANDLE g_dllByDll;

IOTHUB_CLIENT_TRANSPORT_PROVIDER g_protocol = AMQP_Protocol;
const char *g_host = NULL;

void set_badnetwork_test_protocol(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    g_protocol = protocol;
}
    
BEGIN_TEST_SUITE(iothubclient_badnetwork_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
#if defined(__linux__)
        g_host = getenv("HOSTNAME");
        if (g_host== NULL || *g_host == 0)
        {
            ASSERT_FAIL("This test can only run inside a docker container");
        }
#endif
        e2e_init();
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        e2e_deinit();
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_CLEANUP(TestFunctionCleanup)
    {
        if (is_network_connected() == false)
        {
            printf("Function cleanup -- reconnecting\r\n");
            network_reconnect();
        }
    }

    TEST_FUNCTION(IotHub_BadNetwork_disconnect_create_send_reconnect_SAS)
    {
        disconnect_create_send_reconnect(IoTHubAccount_GetSASDevice(g_iothubAcctInfo),g_protocol);
    }

    TEST_FUNCTION(IotHub_BadNetwork_disconnect_after_first_confirmation_then_close_SAS)
    {
        disconnect_after_first_confirmation_then_close(IoTHubAccount_GetSASDevice(g_iothubAcctInfo),g_protocol);
    }

    TEST_FUNCTION(IotHub_BadNetwork_send_disconnect_send_reconnect_etc_SAS)
    {
        send_disconnect_send_reconnect_etc(IoTHubAccount_GetSASDevice(g_iothubAcctInfo),g_protocol);
    }
    
    TEST_FUNCTION(IotHub_BadNetwork_disconnect_create_send_reconnect_X509)
    {
        disconnect_create_send_reconnect(IoTHubAccount_GetX509Device(g_iothubAcctInfo),g_protocol);
    }

    TEST_FUNCTION(IotHub_BadNetwork_disconnect_after_first_confirmation_then_close_X509)
    {
        disconnect_after_first_confirmation_then_close(IoTHubAccount_GetX509Device(g_iothubAcctInfo),g_protocol);
    }

    TEST_FUNCTION(IotHub_BadNetwork_send_disconnect_send_reconnect_etc_X509)
    {
        send_disconnect_send_reconnect_etc(IoTHubAccount_GetX509Device(g_iothubAcctInfo),g_protocol);
    }

END_TEST_SUITE(iothubclient_badnetwork_e2e)

