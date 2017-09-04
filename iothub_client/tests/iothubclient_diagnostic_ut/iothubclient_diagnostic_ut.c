// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_stdint.h"
#include "umocktypes_bool.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/map.h"
#include "iothub_message.h"

#undef ENABLE_MOCKS

#include "iothub_client_diagnostic.h"


static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(iothubclient_diagnostic_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetDiagnosticId, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetDiagnosticId, IOTHUB_MESSAGE_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetDiagnosticCreationTimeUtc, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetDiagnosticCreationTimeUtc, IOTHUB_MESSAGE_ERROR);


    REGISTER_GLOBAL_MOCK_RETURN(Map_Add, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Add, MAP_ERROR);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_001: [ IoTHubClient_Diagnostic_AddIfNecessary should return false if diagSetting or messageHandle is NULL. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_with_null_arguments_fails)
{
    //arrange

    //act
    bool result = IoTHubClient_Diagnostic_AddIfNecessary(NULL, NULL);

    //assert
    ASSERT_IS_FALSE(result);

    //cleanup
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_002: [ IoTHubClient_Diagnostic_AddIfNecessary should return false if failing to add diagnostic property. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_fails)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        100,	/*diagnostic sampling percentage*/
        0		/*message number*/
    };

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticId(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticCreationTimeUtc(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        bool result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, (IOTHUB_MESSAGE_HANDLE)0x12);

        //assert
        ASSERT_IS_FALSE(result);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_003: [ If diagSamplingPercentage is equal to 0, message number should not be increased and no diagnostic properties added ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_no_diag_info_with_percentage_0)
{
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        0,		/*diagnostic sampling percentage*/
        0		/*message number*/
    };

    umock_c_reset_all_calls();
    bool result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, (IOTHUB_MESSAGE_HANDLE)0x12);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_004: [ If diagSamplingPercentage is equal to 100, diagnostic properties should be added to all messages]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_no_diag_info_with_percentage_100)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        100,	/*diagnostic sampling percentage*/
        0		/*message number*/
    };

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticId(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticCreationTimeUtc(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //act
    bool result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, (IOTHUB_MESSAGE_HANDLE)0x12);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, 1);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_005: [ If diagSamplingPercentage is between(0, 100), diagnostic properties should be added based on percentage]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_no_diag_info_with_normal_percentage)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        50,		/*diagnostic sampling percentage*/
        0		/*message number*/
    };

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticId(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticCreationTimeUtc(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //act
    for (size_t index = 0; index < 2; ++index)
    {
        bool result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, (IOTHUB_MESSAGE_HANDLE)0x12);
        ASSERT_IS_TRUE(result);
    }

    //assert
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, 2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(iothubclient_diagnostic_ut)
