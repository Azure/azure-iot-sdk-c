// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(iothubclient_amqp_dt_e2e_sfc, failedTestCount);
    return (int)failedTestCount;
}
