// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_macro_utils/macro_utils.h"
#include "iothub.h"

/* c-logging aborts on any log line until logger_init is called. IoTHub_Init holds its own reference, so its
   own log works when platform_init fails and platform adapters that do not initialize c-logging work.
   Unit tests predefine IOTHUB_LOGGER_INIT/IOTHUB_LOGGER_DEINIT to mock them. */
#ifndef IOTHUB_LOGGER_INIT
#if !defined(NO_LOGGING) && !defined(MINIMAL_LOGERROR) && !defined(ESP8266_RTOS)
#include "c_logging/logger.h"
#define IOTHUB_LOGGER_INIT() logger_init()
#define IOTHUB_LOGGER_DEINIT() logger_deinit()
#else
#define IOTHUB_LOGGER_INIT() 0
#define IOTHUB_LOGGER_DEINIT() ((void)0)
#endif
#endif

/* Successful IoTHub_Init calls not yet matched by IoTHub_Deinit. IoTHub_Deinit without one (e.g. after a failed
   IoTHub_Init) does nothing, so it never releases references it does not own. */
static uint32_t iothub_init_count = 0;

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
        iothub_init_count++;
        result = 0;
    }
    return result;
}

void IoTHub_Deinit(void)
{
    if (iothub_init_count > 0)
    {
        iothub_init_count--;
        platform_deinit();
        IOTHUB_LOGGER_DEINIT();
    }
}
