// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "iothub.h"
#include "iothub_device_client_ll.h"
#ifdef TEST_MQTT
#include "iothubtransportmqtt.h"
#endif
#ifdef TEST_AMQP
#include "iothubtransportamqp.h"
#endif
#ifdef TEST_HTTP
#include "iothubtransporthttp.h"
#endif

/* Uses the SDK as documented (IoTHub_Init, no logger_init) and makes it log: an invalid argument logs an error.
   The process aborts if the SDK logs while c-logging is not initialized. */
static int create_with_null_connection_string_fails(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char* name)
{
    int result;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE handle = IoTHubDeviceClient_LL_CreateFromConnectionString(NULL, protocol);
    if (handle != NULL)
    {
        (void)printf("FAILED: %s: create with a NULL connection string returned a handle\r\n", name);
        IoTHubDeviceClient_LL_Destroy(handle);
        result = 1;
    }
    else
    {
        result = 0;
    }
    return result;
}

int main(void)
{
    int failed = 0;

    if (IoTHub_Init() != 0)
    {
        (void)printf("FAILED: IoTHub_Init\r\n");
        failed++;
    }
    else
    {
#ifdef TEST_MQTT
        failed += create_with_null_connection_string_fails(MQTT_Protocol, "MQTT");
#endif
#ifdef TEST_AMQP
        failed += create_with_null_connection_string_fails(AMQP_Protocol, "AMQP");
#endif
#ifdef TEST_HTTP
        failed += create_with_null_connection_string_fails(HTTP_Protocol, "HTTP");
#endif
        IoTHub_Deinit();
    }

    (void)printf("iothub_init_logging_int: %d failed\r\n", failed);
    return failed;
}
