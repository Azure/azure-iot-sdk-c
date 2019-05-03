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
#include "azure_c_shared_utility/crt_abstractions.h"
#undef ENABLE_MOCKS

#include "iothub_client_streaming.h"

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}


// Data definitions

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

static void reset_test_data()
{
    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));
}

static void register_umock_alias_types() 
{
    //REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
}

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, TEST_free);
}

static void register_global_mock_returns() 
{ 
    REGISTER_GLOBAL_MOCK_RETURN(mallocAndStrcpy_s, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 1);
}

BEGIN_TEST_SUITE(iothub_client_streaming_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
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
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
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

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_017: [ The function shall allocate memory for a new instance of DEVICE_STREAM_C2D_REQUEST (aka `clone`) ]
// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_019: [ All fields of `request` shall be copied into `clone` ]
TEST_FUNCTION(stream_c2d_request_clone_succeeds)
{
    // arrange
    DEVICE_STREAM_C2D_REQUEST* result;
    DEVICE_STREAM_C2D_REQUEST request;

    request.request_id = "1";
    request.name = "TestStream";
    request.url = "someurl.com";
    request.authorization_token = "123";

    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));

    // act
    result = stream_c2d_request_clone(&request);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    stream_c2d_request_destroy(result);
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_018: [ If malloc fails, the function shall return NULL ]
// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_020: [ If any field fails to be copied, the function shall release the memory allocated and return NULL ]
TEST_FUNCTION(stream_c2d_request_clone_negative_tests)
{
    // arrange
    DEVICE_STREAM_C2D_REQUEST request;
    size_t count;

    request.request_id = "1";
    request.name = "TestStream";
    request.url = "someurl.com";
    request.authorization_token = "123";

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    umock_c_negative_tests_snapshot();

    count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        DEVICE_STREAM_C2D_REQUEST* result;
        char tmp_msg[128];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        (void)sprintf(tmp_msg, "Failed in test %zu/%zu", index, count);

        //act
        result = stream_c2d_request_clone(&request);

        // assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_016: [ If `request` is NULL, the function shall return NULL ]
TEST_FUNCTION(stream_c2d_request_clone_NULL_request)
{
    // arrange
    DEVICE_STREAM_C2D_REQUEST* result;

    umock_c_reset_all_calls();

    // act
    result = stream_c2d_request_clone(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_011: [ The function shall allocate memory for a new instance of DEVICE_STREAM_C2D_RESPONSE (aka `response`) ]
// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_013: [ `request->request_id` shall be copied into `response->request_id` ]
TEST_FUNCTION(stream_c2d_response_create_succeeds)
{
    // arrange
    DEVICE_STREAM_C2D_RESPONSE* result;
    DEVICE_STREAM_C2D_REQUEST request;

    request.request_id = "1";
    request.name = "TestStream";
    request.url = "someurl.com";
    request.authorization_token = "123";

    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));

    // act
    result = stream_c2d_response_create(&request, false);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    stream_c2d_response_destroy(result);
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_012: [ If malloc fails, the function shall return NULL ]
// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_015: [ If any values fail to be copied, the function shall release the memory allocated and return NULL ]
TEST_FUNCTION(stream_c2d_response_create_negative_tests)
{
    // arrange
    DEVICE_STREAM_C2D_REQUEST request;
    size_t count;

    request.request_id = "1";

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    umock_c_negative_tests_snapshot();

    count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        DEVICE_STREAM_C2D_RESPONSE* result;
        char tmp_msg[128];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        (void)sprintf(tmp_msg, "Failed in test %zu/%zu", index, count);

        //act
        result = stream_c2d_response_create(&request, false);

        // assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_010: [ If `request` is NULL, the function shall return NULL ]
TEST_FUNCTION(stream_c2d_response_create_NULL_request)
{
    // arrange
    DEVICE_STREAM_C2D_RESPONSE* result;

    umock_c_reset_all_calls();

    // act
    result = stream_c2d_response_create(NULL, true);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_024: [ The memory allocated for `request` shall be released ]
TEST_FUNCTION(stream_c2d_request_destroy_succeeds)
{
    // arrange
    DEVICE_STREAM_C2D_REQUEST* result;

    result = (DEVICE_STREAM_C2D_REQUEST*)real_malloc(sizeof(DEVICE_STREAM_C2D_REQUEST));
    ASSERT_IS_NOT_NULL(result);

    result->request_id = (char*)TEST_malloc(sizeof(char) * 2);
    (void)sprintf(result->request_id, "1");
    result->name = (char*)malloc(sizeof(char) * 4);
    (void)sprintf(result->name, "abc");
    result->url = (char*)malloc(sizeof(char) * 12);
    (void)sprintf(result->url, "someurl.com");
    result->authorization_token = (char*)malloc(sizeof(char) * 4);
    (void)sprintf(result->authorization_token, "123");

    umock_c_reset_all_calls();
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    stream_c2d_request_destroy(result);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_023: [ If `request` is NULL, the function shall return ]
TEST_FUNCTION(stream_c2d_request_destroy_NULL_request)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    stream_c2d_request_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_022: [ The memory allocated for `response` shall be released ]
TEST_FUNCTION(stream_c2d_response_destroy_succeeds)
{
    // arrange
    DEVICE_STREAM_C2D_RESPONSE* result;

    result = (DEVICE_STREAM_C2D_RESPONSE*)real_malloc(sizeof(DEVICE_STREAM_C2D_RESPONSE));
    ASSERT_IS_NOT_NULL(result);

    result->request_id = (char*)TEST_malloc(sizeof(char) * 2);
    (void)sprintf(result->request_id, "1");

    umock_c_reset_all_calls();
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    stream_c2d_response_destroy(result);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUB_CLIENT_STREAMING_09_021: [ If `request` is NULL, the function shall return ]
TEST_FUNCTION(stream_c2d_response_destroy_NULL_request)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    stream_c2d_response_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

END_TEST_SUITE(iothub_client_streaming_ut)
