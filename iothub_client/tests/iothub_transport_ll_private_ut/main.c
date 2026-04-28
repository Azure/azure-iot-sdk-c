// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include "testrunnerswitcher.h"
#include "c_logging/logger.h"

int main(void)
{
    size_t failedTestCount = 0;
    (void)logger_init();
    RUN_TEST_SUITE(iothub_transport_ll_private_ut, failedTestCount);
    logger_deinit();
    return (int)failedTestCount;
}
