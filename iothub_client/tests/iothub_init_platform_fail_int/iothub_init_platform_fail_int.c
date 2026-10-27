// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "c_logging/logger.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/platform.h"
#include "iothub.h"

/* Platform stub: fails, never initializes c-logging, counts platform_deinit calls. */
static int platform_deinit_calls = 0;

int platform_init(void)
{
    return 1;
}

void platform_deinit(void)
{
    platform_deinit_calls++;
}

const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{
    return NULL;
}

STRING_HANDLE platform_get_platform_info(PLATFORM_INFO_OPTION options)
{
    (void)options;
    return NULL;
}

/* Aborts if IoTHub_Init logs its platform failure without holding a logger reference. */
static int failed_IoTHub_Init_logs_and_IoTHub_Deinit_does_nothing(void)
{
    int result;
    if (IoTHub_Init() == 0)
    {
        (void)printf("FAILED: IoTHub_Init succeeded with a failing platform_init\r\n");
        result = 1;
    }
    else
    {
        IoTHub_Deinit();
        result = (platform_deinit_calls == 0) ? 0 : 1;
        if (result != 0)
        {
            (void)printf("FAILED: IoTHub_Deinit after a failed IoTHub_Init called platform_deinit\r\n");
        }
    }
    return result;
}

/* Aborts if the failed IoTHub_Init/IoTHub_Deinit pair released the application's logger reference. */
static int application_logger_reference_survives_failed_IoTHub_Init(void)
{
    int result;
    if (logger_init() != 0)
    {
        (void)printf("FAILED: logger_init\r\n");
        result = 1;
    }
    else
    {
        (void)IoTHub_Init();
        IoTHub_Deinit();
        LogError("iothub_init_platform_fail_int: application log after failed IoTHub_Init and IoTHub_Deinit");
        logger_deinit();
        result = 0;
    }
    return result;
}

int main(void)
{
    int failed = 0;
    failed += failed_IoTHub_Init_logs_and_IoTHub_Deinit_does_nothing();
    failed += application_logger_reference_survives_failed_IoTHub_Init();
    (void)printf("iothub_init_platform_fail_int: %d failed\r\n", failed);
    return failed;
}
