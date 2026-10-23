// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_macro_utils/macro_utils.h"
#include "iothub.h"

/* c-logging aborts on any log line until logger_init is called. IoTHub_Init holds its own reference, so its
   own log works when platform_init fails and platform adapters that do not initialize c-logging work. */
#if !defined(NO_LOGGING) && !defined(MINIMAL_LOGERROR) && !defined(ESP8266_RTOS)
#include "c_logging/logger.h"
#define IOTHUB_LOGGER_INIT() logger_init()
#define IOTHUB_LOGGER_DEINIT() logger_deinit()
#else
#define IOTHUB_LOGGER_INIT() 0
#define IOTHUB_LOGGER_DEINIT() ((void)0)
#endif

int IoTHub_Init(void)
{
    int result;
    if (IOTHUB_LOGGER_INIT() != 0)
    {
        (void)printf("IoTHub_Init: logger_init failed\r\n");
        result = MU_FAILURE;
    }
    else if (platform_init() != 0)
    {
        LogError("Platform initialization failed");
        IOTHUB_LOGGER_DEINIT();
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

void IoTHub_Deinit(void)
{
    platform_deinit();
    IOTHUB_LOGGER_DEINIT();
}
