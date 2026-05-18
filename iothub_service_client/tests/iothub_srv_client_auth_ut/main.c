// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "c_logging/logger.h"

int main(void)
{
    size_t failedTestCount = 0;
    logger_init();
    RUN_TEST_SUITE(iothub_service_client_auth_ut, failedTestCount);
    return failedTestCount;
}
