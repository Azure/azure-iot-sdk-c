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

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_stdint.h"
#include "umocktypes_bool.h"
#include "parson.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/map.h"
#include "iothub_message.h"

#undef ENABLE_MOCKS

#include "iothub_client_diagnostic.h"

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

bool g_fail_string_sprintf;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

int STRING_sprintf(STRING_HANDLE handle, const char* psz, ...)
{
    (void)handle;
    (void)psz;
    return g_fail_string_sprintf ? -1 : 0;
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static IOTHUB_MESSAGE_HANDLE TEST_MESSAGE_HANDLE = (IOTHUB_MESSAGE_HANDLE)0x12;

#define TEST_STRING_HANDLE (STRING_HANDLE)0x46
#define INDEFINITE_TIME ((time_t)-1)
static time_t g_current_time;

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

    g_current_time = time(NULL);

    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetDiagnosticPropertyData, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetDiagnosticPropertyData, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(Map_Add, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Add, MAP_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, g_current_time);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(get_time, INDEFINITE_TIME);
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
    g_fail_string_sprintf = false;
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_001: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if diagSetting or messageHandle is NULL. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_with_null_messageHanlde_fails)
{
    //arrange

    //act
    int result = IoTHubClient_Diagnostic_AddIfNecessary((IOTHUB_DIAGNOSTIC_SETTING_DATA*)0x1, NULL);

    //assert
    ASSERT_IS_FALSE(result == 0);

    //cleanup
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_001: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if diagSetting or messageHandle is NULL. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_with_null_setting_fails)
{
    //arrange

    //act
    int result = IoTHubClient_Diagnostic_AddIfNecessary(NULL, TEST_MESSAGE_HANDLE);

    //assert
    ASSERT_IS_FALSE(result == 0);

    //cleanup
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_002: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if failing to add diagnostic property. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_fails)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        100,	/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_LOCAL /*use local setting*/
    };

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticPropertyData(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        int result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_IS_FALSE(result == 0);
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
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_LOCAL /*use local setting*/
    };

    umock_c_reset_all_calls();
    int result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, TEST_MESSAGE_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(result == 0);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, 0);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_004: [ If diagSamplingPercentage is equal to 100, diagnostic properties should be added to all messages]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_no_diag_info_with_percentage_100)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        100,	/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_LOCAL /*use local setting*/
    };

    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticPropertyData(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, TEST_MESSAGE_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(result == 0);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, 1);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_005: [ If diagSamplingPercentage is between(0, 100), diagnostic properties should be added based on percentage]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_no_diag_info_with_normal_percentage)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        50,		/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_LOCAL /*use local setting*/
    };

    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticPropertyData(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    for (uint32_t index = 0; index < 2; ++index)
    {
        int result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, index + 1);
        ASSERT_IS_TRUE(result == 0);
    }
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_006: [ IoTHubClient_Diagnostic_UpdateFromTwin should return nonezero if arguments are NULL. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_UpdateFromTwin_with_null_args_fails)
{
    //arrange

    //act
    int result = IoTHubClient_Diagnostic_UpdateFromTwin(NULL, false, NULL, NULL);

    //assert
    ASSERT_IS_FALSE(result == 0);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_007: [ IoTHubClient_Diagnostic_UpdateFromTwin should return nonezero if payLoad is not a valid json string. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_UpdateFromTwin_with_invalid_twin_fails)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        0,		/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_REMOTE /*use remote setting*/
    };

    //act
    int result = IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, false, (const unsigned char*)"abc", TEST_STRING_HANDLE);

    //assert
    ASSERT_IS_FALSE(result == 0);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_009: [ IoTHubClient_Diagnostic_UpdateFromTwin should set diagSamplingPercentage = 0 when sampling rate in twin is null. ]*/
/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_012: [ IoTHubClient_Diagnostic_UpdateFromTwin should set diagSamplingPercentage correctly if sampling rate is valid. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_UpdateFromTwin_update_twin)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        0,		/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_REMOTE /*use remote setting*/
    };

    //act
    int result = IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, false, (const unsigned char*)"{ \"desired\": {\"__e2e_diag_sample_rate\": 100,\"$version\": 11}}", TEST_STRING_HANDLE);

    ASSERT_ARE_EQUAL(uint32_t, result, 0);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.diagSamplingPercentage, 100);

    result = IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, true, (const unsigned char*)"{\"__e2e_diag_sample_rate\":99}", TEST_STRING_HANDLE);

    ASSERT_ARE_EQUAL(uint32_t, result, 0);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.diagSamplingPercentage, 99);

    result = IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, true, (const unsigned char*)"{\"__e2e_diag_sample_rate\":null}", TEST_STRING_HANDLE);

    ASSERT_ARE_EQUAL(uint32_t, result, 0);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.diagSamplingPercentage, 0);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_008: [ IoTHubClient_Diagnostic_UpdateFromTwin should return nonezero if device twin json doesn't contains a valid desired property. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_UpdateFromTwin_with_invalid_complete_twin_update)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        50,		/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_REMOTE /*use remote setting*/
    };

    //act
    IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, false, (const unsigned char*)"{ \"nodesired\": {\"__e2e_diag_sample_rate\": 100,\"$version\": 11} }", TEST_STRING_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.diagSamplingPercentage, 50);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_011: [ IoTHubClient_Diagnostic_UpdateFromTwin should set diagSamplingPercentage = 0 if sampling rate parsed from twin is not between [0,100]. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_UpdateFromTwin_with_diag_setting_out_of_range)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        50,		/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_REMOTE /*use remote setting*/
    };

    //act
    IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, true, (const unsigned char*)"{\"__e2e_diag_sample_rate\":101}", TEST_STRING_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.diagSamplingPercentage, 50);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_010: [ IoTHubClient_Diagnostic_UpdateFromTwin should set diagSamplingPercentage = 0 when cannot parse sampling rate from twin. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_UpdateFromTwin_with_diag_setting_invalid_type)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        50,		/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_REMOTE /*use remote setting*/
    };

    //act
    IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, true, (const unsigned char*)"{\"__e2e_diag_sample_rate\": \"19\"}", TEST_STRING_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.diagSamplingPercentage, 50);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_013: [ IoTHubClient_Diagnostic_UpdateFromTwin should report diagnostic property not existed if there is no sampling rate in complete twin. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_UpdateFromTwin_when_no_sampling_rate_in_complete_twin)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        50,		/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_REMOTE /*use remote setting*/
    };

    //act
    IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, false, (const unsigned char*)"{ \"desired\": {}}", TEST_STRING_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.diagSamplingPercentage, 50);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_014: [ IoTHubClient_Diagnostic_UpdateFromTwin should return nonzero if STRING_sprintf failed. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_UpdateFromTwin_when_no_sampling_rate_in_complete_twin_fails)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        50,		/*diagnostic sampling percentage*/
        0,		/*message number*/
        E2E_DIAG_SETTING_USE_REMOTE /*use remote setting*/
    };
    g_fail_string_sprintf = true;

    //act
    int result = IoTHubClient_Diagnostic_UpdateFromTwin(&diag_setting, false, (const unsigned char*)"{ \"desired\": {}}", TEST_STRING_HANDLE);

    //assert
    ASSERT_IS_TRUE(result != 0);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.diagSamplingPercentage, 50);
}


END_TEST_SUITE(iothubclient_diagnostic_ut)
