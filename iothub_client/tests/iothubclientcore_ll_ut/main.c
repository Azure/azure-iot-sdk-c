// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include "testrunnerswitcher.h"

#ifdef WINCE
#include "windows.h"
#endif

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(iothub_client_core_ll_ut, failedTestCount);
    return failedTestCount;
}
