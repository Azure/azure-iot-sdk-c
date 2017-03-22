// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "testrunnerswitcher.h"
#include "network_disconnect.h"

extern const char *g_host;


int network_disconnect()
{
    int result = __FAILURE__;
    printf("**** disconnecting from network ****\n");

#if defined(WIN32)
    // TODO
#elif defined(__linux__)
    result = system("docker network disconnect YoYoNet ${HOSTNAME}");
    if (result != 0)
    {
        printf("docker network disconnect returned %d\n",result);
    }
    bool connected = is_network_connected();
    ASSERT_IS_FALSE_WITH_MSG(connected, "ping succeeded even though we should be disconnected");
#else
    ASSERT_FAIL("Network disconnect not implemented on this OS\n");
#endif

    return result;
}

int network_reconnect()
{
    int result = __FAILURE__;
    printf("**** connecting to network ****\n");

#if defined(WIN32)
    // TODO
#elif defined(__linux__)
    result = system("docker network connect YoYoNet ${HOSTNAME}");
    if (result != 0)
    {
        printf("docker network connect returned %d\n",result);
    }
    bool connected = is_network_connected();
    ASSERT_IS_TRUE_WITH_MSG(connected, "ping failed even though we should be connected");
#else
    ASSERT_FAIL("Network connecting not implemented on this OS\n");
#endif

    return result;
}


bool is_network_connected()
{
    bool connected = true;
    
#if defined(WIN32)
    // TODO
#elif defined(__linux__)
    int result = system("ping -c 1 www.microsoft.com");
    if (result != 0)
    {
        connected = false;
    }
#else
    ASSERT_FAIL("Network connecting not implemented on this OS\n");
#endif

    return connected;
}

