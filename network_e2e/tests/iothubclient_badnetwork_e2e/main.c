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
#include "iothub_client_ll.h"
#include "iothubtransportamqp.h"
#include "iothubtransporthttp.h"
#include "iothubtransportmqtt.h"
#include "iothubtransportamqp_websockets.h"
#include "iothubtransportmqtt_websockets.h"

extern void set_badnetwork_test_protocol(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

IOTHUB_CLIENT_TRANSPORT_PROVIDER string_to_protocol(const char *pname)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    
    if (strcmp(pname, "AMQP") == 0)
    {
        printf("Using AMQP\r\n");
        protocol = AMQP_Protocol;
    }
    else if (strcmp(pname, "AMQP-WS") == 0)
    {
        printf("Using AMQP-WS\r\n");
        protocol = AMQP_Protocol_over_WebSocketsTls;
    }
    else if (strcmp(pname, "MQTT") == 0)
    {
        printf("Using MQTT\r\n");
        protocol = MQTT_Protocol;
    }
    else if (strcmp(pname, "MQTT-WS") == 0)
    {
        printf("Using MQTT-WS\r\n");
        protocol = MQTT_WebSocket_Protocol;
    }
    else if (strcmp(pname, "HTTP") == 0)
    {
        printf("Using HTTP\r\n");
        protocol = HTTP_Protocol;
    }
    else
    {
        printf("unknown protocol\n");
        protocol = NULL;
    }

    return protocol;
}


int main(int argc, char* argv[])
{
    // disable buffering on stdout.  It causes a bad intermix of printf from this process and output from child processes
    setbuf(stdout,NULL); 
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol = NULL;
    size_t failedTestCount = 0;
    if (argc == 1)
    {
        char *env = getenv("E2E_PROTOCOL");
        if (env && *env)
        {
            protocol = string_to_protocol(env);
        }
        else
        {
            protocol = string_to_protocol("AMQP");
        }
    }
    else if (argc == 2)
    {
        protocol = string_to_protocol(argv[1]);
    }


    if (protocol == NULL)
    {
        printf("Usage: iothubclient_badnetwork_e2e [protocol]\r\n");
        printf("protocol = one of [AMQP, AMQP-WS, MQTT, MQTT-WS, HTTP]\r\n");
        printf("protocol can also be specified in the E2E_PROTOCOL env variable\r\n");
        return -1;
    }
    else
    {
        set_badnetwork_test_protocol(protocol);
        RUN_TEST_SUITE(iothubclient_badnetwork_e2e, failedTestCount);
        return failedTestCount;
    }
}
