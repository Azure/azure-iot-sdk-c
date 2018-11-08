// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "iothubclient_common_dt_e2e.h"
#include "iothubtransportamqp.h"
#include "iothubtransportamqp_websockets.h"

BEGIN_TEST_SUITE(iothubclient_amqp_dt_e2e)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    dt_e2e_init(false);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    dt_e2e_deinit();
}

//
// AMQP tests.
//
TEST_FUNCTION(IoTHub_AMQP_SendReported_e2e_sas)
{
    dt_e2e_send_reported_test(AMQP_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

TEST_FUNCTION(IoTHub_AMQP_GetFullDesired_e2e_sas)
{
    dt_e2e_get_complete_desired_test(AMQP_Protocol, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_AMQP_SendReported_e2e_x509)
{
    dt_e2e_send_reported_test(AMQP_Protocol, IOTHUB_ACCOUNT_AUTH_X509);
}

TEST_FUNCTION(IoTHub_AMQP_GetFullDesired_e2e_x509)
{
    dt_e2e_get_complete_desired_test(AMQP_Protocol, IOTHUB_ACCOUNT_AUTH_X509);
}
#endif

#ifndef USE_WOLFSSL // Wolf doesn't run web socket tests
// AMQP_WS tests.
//
TEST_FUNCTION(IoTHub_AMQP_WS_SendReported_e2e_sas)
{
    dt_e2e_send_reported_test(AMQP_Protocol_over_WebSocketsTls, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

TEST_FUNCTION(IoTHub_AMQP_WS_GetFullDesired_e2e_sas)
{
    dt_e2e_get_complete_desired_test(AMQP_Protocol_over_WebSocketsTls, IOTHUB_ACCOUNT_AUTH_CONNSTRING);
}

#ifndef __APPLE__
TEST_FUNCTION(IoTHub_AMQP_WS_SendReported_e2e_x509)
{
    dt_e2e_send_reported_test(AMQP_Protocol_over_WebSocketsTls, IOTHUB_ACCOUNT_AUTH_X509);
}

TEST_FUNCTION(IoTHub_AMQP_WS_GetFullDesired_e2e_x509)
{
    dt_e2e_get_complete_desired_test(AMQP_Protocol_over_WebSocketsTls, IOTHUB_ACCOUNT_AUTH_X509);
}
#endif
#endif

END_TEST_SUITE(iothubclient_amqp_dt_e2e)

