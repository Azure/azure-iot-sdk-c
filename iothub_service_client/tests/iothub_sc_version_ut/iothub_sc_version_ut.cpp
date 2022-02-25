 // Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "iothub_sc_version.h"

static MICROMOCK_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(iothub_sc_version_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = MicroMockCreateMutex();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    MicroMockDestroyMutex(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (!MicroMockAcquireMutex(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    if (!MicroMockReleaseMutex(g_testByTest))
    {
        ASSERT_FAIL("failure in test framework at ReleaseMutex");
    }
}

TEST_FUNCTION(the_version_constant_has_the_expected_value)
{
    // arrange
    // act
    // assert
    ASSERT_ARE_EQUAL(char_ptr, "1.1.0", IOTHUB_SERVICE_CLIENT_VERSION);
}

TEST_FUNCTION(IoTHubServiceClient_GetVersionString_returns_the_version)
{
    // arrange
    // act
    // assert
	ASSERT_ARE_EQUAL(char_ptr, IOTHUB_SERVICE_CLIENT_VERSION, IoTHubServiceClient_GetVersionString());
}

END_TEST_SUITE(iothub_sc_version_ut)

