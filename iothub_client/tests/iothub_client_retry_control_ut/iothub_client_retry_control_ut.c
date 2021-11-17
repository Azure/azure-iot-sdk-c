// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#endif

void* real_malloc(size_t size)
{
    return malloc(size);
}

void real_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"
#include <limits.h>

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "iothub_client_core_ll.h"
#undef ENABLE_MOCKS

#include "internal/iothub_client_retry_control.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}


// Data definitions

#define INDEFINITE_TIME                     ((time_t)-1)
#define TEST_OPTIONHANDLER_HANDLE           (OPTIONHANDLER_HANDLE)0x7771


static time_t TEST_current_time;


// Helpers
static int saved_malloc_returns_count = 0;
static void* saved_malloc_returns[20];

static void* TEST_malloc(size_t size)
{
    saved_malloc_returns[saved_malloc_returns_count] = real_malloc(size);

    return saved_malloc_returns[saved_malloc_returns_count++];
}

static void TEST_free(void* ptr)
{
    int i, j;
    for (i = 0, j = 0; j < saved_malloc_returns_count; i++, j++)
    {
        if (saved_malloc_returns[i] == ptr) j++;

        saved_malloc_returns[i] = saved_malloc_returns[j];
    }

    if (i != j) saved_malloc_returns_count--;

    real_free(ptr);
}


static unsigned int TEST_OptionHandler_AddOption_saved_value;
static OPTIONHANDLER_RESULT TEST_OptionHandler_AddOption_result;
static OPTIONHANDLER_RESULT TEST_OptionHandler_AddOption(OPTIONHANDLER_HANDLE handle, const char* name, const void* value)
{
    (void)handle;
    (void)name;
    TEST_OptionHandler_AddOption_saved_value = *(const unsigned int*)value;
    return TEST_OptionHandler_AddOption_result;
}

static time_t add_seconds(time_t base_time, int seconds)
{
    time_t new_time;
    struct tm *bd_new_time;

    if ((bd_new_time = localtime(&base_time)) == NULL)
    {
        new_time = INDEFINITE_TIME;
    }
    else
    {
        bd_new_time->tm_sec += seconds;
        new_time = mktime(bd_new_time);
    }

    return new_time;
}

static void run_and_verify_should_retry(RETRY_CONTROL_HANDLE handle, time_t first_retry_time, time_t last_retry_time, time_t current_time, double secs_since_first_retry, double secs_since_last_retry, RETRY_ACTION expected_retry_action, bool is_first_check)
{
    // arrange
    umock_c_reset_all_calls();
    if (is_first_check)
    {
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
    }
    else
    {
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
        STRICT_EXPECTED_CALL(get_difftime(current_time, first_retry_time)).SetReturn(secs_since_first_retry);

        if (expected_retry_action != RETRY_ACTION_STOP_RETRYING)
        {
            STRICT_EXPECTED_CALL(get_difftime(current_time, last_retry_time)).SetReturn(secs_since_last_retry);
        }
    }

    if (expected_retry_action == RETRY_ACTION_RETRY_NOW)
    {
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
    }

    // act
    RETRY_ACTION retry_action;
    int result = retry_control_should_retry(handle, &retry_action);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, expected_retry_action, retry_action);
}

// @brief
//     The first element of 'expected_retry_times' shall be 0
static void run_and_verify_should_retry_times(RETRY_CONTROL_HANDLE handle, int* expected_retry_times, int number_of_elements, unsigned int max_retry_time_in_secs)
{
    time_t current_time = TEST_current_time;
    time_t first_time = INDEFINITE_TIME;
    time_t last_time = INDEFINITE_TIME;
    unsigned int secs_since_first_try = 0;
    unsigned int secs_since_last_try = 0;
    RETRY_ACTION expected_retry_action = RETRY_ACTION_RETRY_NOW;

    run_and_verify_should_retry(handle, first_time, last_time, current_time, secs_since_first_try, secs_since_last_try, expected_retry_action, true);

    first_time = current_time;
    last_time = current_time;

    int i;
    for (i = 1; i < number_of_elements; i++)
    {
        // RETRY_LATER time test
        int half_secs_since_last_try = (expected_retry_times[i] - expected_retry_times[i - 1]) / 2;

        if (half_secs_since_last_try > 0 && half_secs_since_last_try != expected_retry_times[i])
        {
            unsigned int half_secs_since_first_try = secs_since_first_try + half_secs_since_last_try;
            time_t half_current_time = add_seconds(current_time, half_secs_since_last_try);
            expected_retry_action = (half_secs_since_first_try < max_retry_time_in_secs ? RETRY_ACTION_RETRY_LATER : RETRY_ACTION_STOP_RETRYING);

            run_and_verify_should_retry(handle, first_time, last_time, half_current_time, half_secs_since_first_try, half_secs_since_last_try, expected_retry_action, false);
        }

        // RETRY_NOW/STOP_RETRYING time test
        secs_since_last_try = expected_retry_times[i];
        secs_since_first_try += expected_retry_times[i];
        current_time = add_seconds(current_time, secs_since_last_try);
        expected_retry_action = (secs_since_first_try < max_retry_time_in_secs ? RETRY_ACTION_RETRY_NOW : RETRY_ACTION_STOP_RETRYING);

        run_and_verify_should_retry(handle, first_time, last_time, current_time, secs_since_first_try, secs_since_last_try, expected_retry_action, false);

        if (expected_retry_action == RETRY_ACTION_RETRY_NOW)
        {
            last_time = current_time;
        }
    }
}

static void reset_test_data()
{
    TEST_current_time = time(NULL);

    TEST_OptionHandler_AddOption_saved_value = 0;
    TEST_OptionHandler_AddOption_result = OPTIONHANDLER_OK;

    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));
}

static void register_umock_alias_types()
{
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(pfCloneOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfDestroyOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfSetOption, void*);
}

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(OptionHandler_AddOption, TEST_OptionHandler_AddOption);
}

static void register_global_mock_returns()
{
    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_Create, TEST_OPTIONHANDLER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_AddOption, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_AddOption, OPTIONHANDLER_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_ERROR);
}


static RETRY_CONTROL_HANDLE create_retry_control(IOTHUB_CLIENT_RETRY_POLICY policy_name, unsigned int max_retry_time_in_secs)
{
    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    RETRY_CONTROL_HANDLE handle = retry_control_create(policy_name, max_retry_time_in_secs);

    return handle;
}


BEGIN_TEST_SUITE(iothub_client_retry_control_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    int result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    register_umock_alias_types();
    register_global_mock_returns();
    register_global_mock_hooks();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
    umock_c_negative_tests_deinit();

    reset_test_data();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(create_malloc_fails)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_fail_call(0);

    // act
    RETRY_CONTROL_HANDLE handle = retry_control_create(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    // assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();
}

TEST_FUNCTION(create_success)
{
    // arrange
    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));

    // act
    RETRY_CONTROL_HANDLE handle = retry_control_create(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(destroy_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    retry_control_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(destroy_success)
{
    // arrange
    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    RETRY_CONTROL_HANDLE handle = retry_control_create(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();
    EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    retry_control_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(is_timeout_reached_INDEFINITE_start_time)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    bool is_timed_out;
    int result = is_timeout_reached(INDEFINITE_TIME, 10, &is_timed_out);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(is_timeout_reached_NULL_is_timed_out)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = is_timeout_reached(TEST_current_time, 10, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(is_timeout_reached_TRUE_success)
{
    // arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(10);

    // act
    bool is_timed_out;
    int result = is_timeout_reached(TEST_current_time, 10, &is_timed_out);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(is_timed_out);

    // cleanup
}

TEST_FUNCTION(is_timeout_reached_FALSE_success)
{
    // arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(9);

    // act
    bool is_timed_out;
    int result = is_timeout_reached(TEST_current_time, 10, &is_timed_out);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_FALSE(is_timed_out);

    // cleanup
}

TEST_FUNCTION(is_timeout_reached_failure_checks)
{
    // arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL))
        .SetReturn(INDEFINITE_TIME);

    // act
    bool is_timed_out;
    int result = is_timeout_reached(TEST_current_time, 10, &is_timed_out);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(Retrieve_Options_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    OPTIONHANDLER_HANDLE result = retry_control_retrieve_options(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
}

TEST_FUNCTION(Retrieve_Options_success)
{
    // arrange
    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    RETRY_CONTROL_HANDLE handle = retry_control_create(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();
    EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, IGNORED_PTR_ARG))
        .IgnoreArgument_value();
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, RETRY_CONTROL_OPTION_MAX_JITTER_PERCENT, IGNORED_PTR_ARG))
        .IgnoreArgument_value();

    // act
    OPTIONHANDLER_HANDLE result = retry_control_retrieve_options(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_OPTIONHANDLER_HANDLE, result);
    ASSERT_ARE_EQUAL(int, 5, TEST_OptionHandler_AddOption_saved_value);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Retrieve_Options_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    RETRY_CONTROL_HANDLE handle = retry_control_create(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();
    EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, IGNORED_PTR_ARG))
        .IgnoreArgument_value();
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, RETRY_CONTROL_OPTION_MAX_JITTER_PERCENT, IGNORED_PTR_ARG))
        .IgnoreArgument_value();
    umock_c_negative_tests_snapshot();

    // act

    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        OPTIONHANDLER_HANDLE result = retry_control_retrieve_options(handle);

        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    retry_control_destroy(handle);
}

TEST_FUNCTION(Set_Options_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = retry_control_set_option(NULL, RETRY_CONTROL_OPTION_SAVED_OPTIONS, TEST_OPTIONHANDLER_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(Set_Options_NULL_name)
{
    // arrange
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();

    // act
    int result = retry_control_set_option(handle, NULL, TEST_OPTIONHANDLER_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Set_Options_NULL_value)
{
    // arrange
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();

    // act
    int result = retry_control_set_option(handle, RETRY_CONTROL_OPTION_SAVED_OPTIONS, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Set_Options_INVALID_initial_wait_time_in_secs)
{
    // arrange
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();

    // act
    unsigned int value = 0;
    int result = retry_control_set_option(handle, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Set_Options_INVALID_max_jitter_percent)
{
    // arrange
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();

    // act
    unsigned int value = 101;
    int result = retry_control_set_option(handle, RETRY_CONTROL_OPTION_MAX_JITTER_PERCENT, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Set_Options_success)
{
    // arrange
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(OptionHandler_FeedOptions(TEST_OPTIONHANDLER_HANDLE, handle));

    // act
    int result = retry_control_set_option(handle, RETRY_CONTROL_OPTION_SAVED_OPTIONS, TEST_OPTIONHANDLER_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Set_Options_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(OptionHandler_FeedOptions(TEST_OPTIONHANDLER_HANDLE, handle));
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        int result = retry_control_set_option(handle, RETRY_CONTROL_OPTION_SAVED_OPTIONS, TEST_OPTIONHANDLER_HANDLE);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    retry_control_destroy(handle);
}

TEST_FUNCTION(Set_Options_UNSUPPORTED_name)
{
    // arrange
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    umock_c_reset_all_calls();

    /*
    Cumpriu sua senten�a.
    Encontrou-se com o �nico mal irremedi�vel,
    aquilo que � a marca do nosso estranho destino sobre a terra,
    aquele fato sem explica��o que iguala tudo o que � vivo num s� rebanho de condenados,
    porque tudo o que � vivo, morre.
    (Ariano Suassuna)
    */

    // act
    int result = retry_control_set_option(handle, "initial_wait_time_in_sec", TEST_OPTIONHANDLER_HANDLE); // name is missing a final letter ('s')

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_NULL_handle)
{
    // arrange
    RETRY_ACTION retry_action;

    // act
    int result = retry_control_should_retry(NULL, &retry_action);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(Should_Retry_NULL_retry_action)
{
    // arrange
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    // act
    int result = retry_control_should_retry(handle, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_failure)
{
    // arrange
    unsigned int max_retry_time_in_secs = 15;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, max_retry_time_in_secs);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(INDEFINITE_TIME);

    // act
    RETRY_ACTION retry_action;
    int result = retry_control_should_retry(handle, &retry_action);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_evaluate_retry_action_failure)
{
    // arrange
    unsigned int max_retry_time_in_secs = 15;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, max_retry_time_in_secs);

    // This first call succeeds because retry_count is 0
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    RETRY_ACTION retry_action;
    (void)retry_control_should_retry(handle, &retry_action);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(INDEFINITE_TIME);

    // act
    int result = retry_control_should_retry(handle, &retry_action);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_INDEFINITE_last_retry_time)
{
    // arrange
    unsigned int max_retry_time_in_secs = 15;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, max_retry_time_in_secs);

    // This first call succeeds because retry_count is 0
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(INDEFINITE_TIME);
    RETRY_ACTION retry_action;
    (void)retry_control_should_retry(handle, &retry_action);

    umock_c_reset_all_calls();

    // act
    int result = retry_control_should_retry(handle, &retry_action);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_NO_MAX_RETRY_TIME_success)
{
    // arrange
    unsigned int max_retry_time_in_secs = 0;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_IMMEDIATE, max_retry_time_in_secs);

    time_t first_try_time = TEST_current_time;
    time_t next_try_time = add_seconds(first_try_time, 100);

    // This first call succeeds because retry_count is 0
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(first_try_time);
    RETRY_ACTION retry_action;
    (void)retry_control_should_retry(handle, &retry_action);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(next_try_time);
    // no get_difftime gets invoked.

    // act
    int result = retry_control_should_retry(handle, &retry_action);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, RETRY_ACTION_RETRY_NOW, retry_action);
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_EXPONENTIAL_BACKOFF_WITH_JITTER_success)
{
    // arrange
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 10);

    unsigned int option_value = 1;
    int set_option_result = retry_control_set_option(handle, RETRY_CONTROL_OPTION_MAX_JITTER_PERCENT, &option_value);

    int expected_retry_times[] = { 0, 1, 2, 4, 8, 16, 30, 30 };

    run_and_verify_should_retry_times(handle, expected_retry_times, 8, 10);

    // assert
    ASSERT_ARE_EQUAL(int, 0, set_option_result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_EXPONENTIAL_BACKOFF_success)
{
    // arrange
    unsigned int max_retry_time_in_secs = 15;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, max_retry_time_in_secs);

    unsigned int option_value = 2;
    int set_option_result = retry_control_set_option(handle, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, &option_value);

    int expected_retry_times[] = { 0, 2, 4, 8, 16, 30, 30 };

    run_and_verify_should_retry_times(handle, expected_retry_times, 7, max_retry_time_in_secs);

    // assert
    ASSERT_ARE_EQUAL(int, 0, set_option_result);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_INTERVAL_success)
{
    // arrange
    unsigned int max_retry_time_in_secs = 19;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_INTERVAL, max_retry_time_in_secs);

    int expected_retry_times[] = { 0, 5, 10, 15, 20, 25, 30, 30 };

    // act
    // assert
    run_and_verify_should_retry_times(handle, expected_retry_times, 8, max_retry_time_in_secs);

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_LINEAR_BACKOFF_success)
{
    // arrange
    unsigned int max_retry_time_in_secs = 19;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_LINEAR_BACKOFF, max_retry_time_in_secs);

    int expected_retry_times[] = { 0, 5, 15, 30, 50 };

    // act
    // assert
    run_and_verify_should_retry_times(handle, expected_retry_times, 5, max_retry_time_in_secs);

    // cleanup
    retry_control_destroy(handle);
}

// This test must be replaced. Create an auxiliary module for get_rand() in c-shared-utilities and test using that
/*
TEST_FUNCTION(Should_Retry_RANDOM_success)
{
    // arrange
    unsigned int max_retry_time_in_secs = RAND_MAX;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_RANDOM, max_retry_time_in_secs);

    time_t first_time = TEST_current_time;
    time_t last_time = TEST_current_time;
    time_t current_time = TEST_current_time;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
    RETRY_ACTION retry_action;
    int result = retry_control_should_retry(handle, &retry_action);

    int number_of_RETRY_ACTION_RETRY_LATER = 0;
    int number_of_RETRY_ACTION_RETRY_NOW = 0;
    int number_of_RETRY_ACTION_STOP_RETRYING = 0;

    unsigned int i, j; // start with 1 because retry_control_should_retry was invoked already
    for (i = 1, j = 1; i <= max_retry_time_in_secs; i++, j++)
    {
        // arrange
        current_time = add_seconds(current_time, 1);

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
        STRICT_EXPECTED_CALL(get_difftime(current_time, first_time)).SetReturn(i);
        STRICT_EXPECTED_CALL(get_difftime(current_time, last_time)).SetReturn(j);

        // this might happen or not, no way to predict at this point.
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);

        // act
        result = retry_control_should_retry(handle, &retry_action);

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);

        if (retry_action == RETRY_ACTION_RETRY_NOW)
        {
            number_of_RETRY_ACTION_RETRY_NOW++;
            last_time = current_time;
            j = 0;
        }
        else if (retry_action == RETRY_ACTION_RETRY_LATER)
        {
            number_of_RETRY_ACTION_RETRY_LATER++;
        }
        else if (retry_action == RETRY_ACTION_STOP_RETRYING)
        {
            number_of_RETRY_ACTION_STOP_RETRYING++;
        }
    }

    ASSERT_ARE_NOT_EQUAL(int, 0, number_of_RETRY_ACTION_RETRY_LATER);
    ASSERT_ARE_NOT_EQUAL(int, 0, number_of_RETRY_ACTION_RETRY_NOW);
    ASSERT_ARE_EQUAL(int, 1, number_of_RETRY_ACTION_STOP_RETRYING);

    // cleanup
    retry_control_destroy(handle);
}
*/

TEST_FUNCTION(Should_Retry_RETRY_IMMEDIATE_success)
{
    // arrange
    unsigned int max_retry_time_in_secs = 10;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_IMMEDIATE, max_retry_time_in_secs);

    time_t first_time = TEST_current_time;
    time_t current_time = TEST_current_time;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);

    unsigned int i;
    for (i = 0; i <= max_retry_time_in_secs; i++)
    {
        // arrange
        current_time = add_seconds(current_time, 1);

        if (i > 0)
        {
            // i.e., if it's not the first call to _should_retry.
            STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
            STRICT_EXPECTED_CALL(get_difftime(current_time, first_time)).SetReturn(i);
        }

        // act
        RETRY_ACTION retry_action;
        int result = retry_control_should_retry(handle, &retry_action);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);

        if (i < max_retry_time_in_secs)
        {
            ASSERT_ARE_EQUAL(int, RETRY_ACTION_RETRY_NOW, retry_action);
        }
        else
        {
            ASSERT_ARE_EQUAL(int, RETRY_ACTION_STOP_RETRYING, retry_action);
        }
    }

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Should_Retry_RETRY_NONE_success)
{
    // arrange
    unsigned int max_retry_time_in_secs = 10;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_NONE, max_retry_time_in_secs);

    umock_c_reset_all_calls();

    unsigned int i;
    for (i = 0; i <= max_retry_time_in_secs; i++)
    {
        // act
        RETRY_ACTION retry_action;
        int result = retry_control_should_retry(handle, &retry_action);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(int, RETRY_ACTION_STOP_RETRYING, retry_action);
    }

    // cleanup
    retry_control_destroy(handle);
}

TEST_FUNCTION(Reset_success)
{
    // arrange
    unsigned int max_retry_time_in_secs = 10;
    RETRY_CONTROL_HANDLE handle = create_retry_control(IOTHUB_CLIENT_RETRY_IMMEDIATE, max_retry_time_in_secs);

    time_t first_try_time = TEST_current_time;
    time_t next_try_time = add_seconds(first_try_time, max_retry_time_in_secs);

    // This first call succeeds because retry_count is 0
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(first_try_time);
    RETRY_ACTION retry_action;
    int result = retry_control_should_retry(handle, &retry_action);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, RETRY_ACTION_RETRY_NOW, retry_action);

    // At this point the retry control reached the max_retry_time_in_secs.
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(next_try_time);
    STRICT_EXPECTED_CALL(get_difftime(next_try_time, first_try_time)).SetReturn(max_retry_time_in_secs);
    result = retry_control_should_retry(handle, &retry_action);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, RETRY_ACTION_STOP_RETRYING, retry_action);

    // act
    retry_control_reset(handle);

    // assert
    umock_c_reset_all_calls();
    // notice "next_try_time" below.
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(next_try_time);
    // The return is RETRY_ACTION_RETRY_NOW because retry_count is 0.
    result = retry_control_should_retry(handle, &retry_action);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, RETRY_ACTION_RETRY_NOW, retry_action);
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    retry_control_destroy(handle);
}

END_TEST_SUITE(iothub_client_retry_control_ut)
