// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>
#include "testrunnerswitcher.h"

// Allow running a single test function by name (argv[1]).
// When no argument is given, all tests run (default behavior).
// This enables CTest to register each TEST_FUNCTION as an individual test.
static const char* g_test_filter = NULL;

const char* csr_test_get_filter(void)
{
    return g_test_filter;
}

int main(int argc, char* argv[])
{
    size_t failedTestCount = 0;
    if (argc > 1)
    {
        g_test_filter = argv[1];
    }
    RUN_TEST_SUITE(prov_x509_csr_client_e2e, failedTestCount);
    return (int)failedTestCount;
}
