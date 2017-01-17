// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportmqtt_websockets.h"

static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(iothubclient_mqtt_ws_e2e)

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

TEST_FUNCTION(IoTHub_MQTT_SendEvent_e2e)
{
    e2e_send_event_test(MQTT_WebSocket_Protocol);
}

TEST_FUNCTION(IoTHub_MQTT_RecvMessage_E2ETest)
{
    e2e_recv_message_test(MQTT_WebSocket_Protocol);
}

END_TEST_SUITE(iothubclient_mqtt_ws_e2e)
