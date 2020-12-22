// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
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
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "parson.h"

#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char*, string);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object*, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, double, json_object_get_number, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, int, json_object_has_value, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_value, JSON_Object*, object, const char*, name, JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_number, JSON_Object*, object, const char*, name, double, number);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_wrapping_value, const JSON_Object*, object);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_array);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Status, json_array_append_value, JSON_Array*, array, JSON_Value*, value);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Object*, json_array_get_object, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);

MOCKABLE_FUNCTION(, JSON_Value*, to_json_mock, void*, handle);
MOCKABLE_FUNCTION(, void*, from_json_mock, JSON_Object*, root_object);

#undef ENABLE_MOCKS

#include "prov_service_client/provisioning_sc_shared_helpers.h"

static TEST_MUTEX_HANDLE g_testByTest;
static int g_fromJson_calls;
static int g_toJson_calls;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

//Control Parameters
static char* DUMMY_JSON = "{\"json\":\"dummy\"}";
static const char* DUMMY_STRING = "dummy";
static int DUMMY_NUM = 4747;
static int ARRAY_SIZE = 3;

#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112
#define TEST_JSON_ARRAY (JSON_Array*)0x11111113
#define DUMMY_OBJECT (void*)0x11111116

#define FROM_JSON_MOCK_RETURN (void*)0x11111114
#define TO_JSON_MOCK_RETURN (JSON_Value*)0x11111115

//static JSON_Value* test_to_json(void* handle)
//{
//    (void)handle;
//    g_toJson_calls++;
//    return TEST_JSON_VALUE;
//}
//

static void* test_from_json(JSON_Object* root_object)
{
    (void)root_object;
    void* retval = (void*)real_malloc(1);
    return retval;
}

static void free_struct_arr(void* arr[], size_t len)
{
    if (len > 0)
    {
        for (size_t i = 0; i < len; i++)
        {
            free(arr[i]);
        }
        free(arr);
    }
}

//#define TEST_TO_JSON_FUNCTION (TO_JSON_FUNCTION)test_to_json
//#define TEST_FROM_JSON_FUNCTION (FROM_JSON_FUNCTION)test_from_json

static void register_global_mocks()
{
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, real_free);

    //parson
    REGISTER_GLOBAL_MOCK_RETURN(json_value_init_object, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, DUMMY_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_parse_string, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_value, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_value, JSONFailure);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_string, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_string, JSONFailure);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_wrapping_value, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_wrapping_value, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_serialize_to_string, DUMMY_JSON);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_number, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_number, JSONFailure);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_array, TEST_JSON_ARRAY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_array, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_number, DUMMY_NUM);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_number, 0);
    REGISTER_GLOBAL_MOCK_RETURN(json_array_get_count, ARRAY_SIZE);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_array, TEST_JSON_ARRAY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_array, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_array_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_array_get_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_init_array, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_array, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_array_append_value, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_array_append_value, JSONFailure);

    REGISTER_GLOBAL_MOCK_HOOK(from_json_mock, test_from_json);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(from_json_mock, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(to_json_mock, TO_JSON_MOCK_RETURN);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(to_json_mock, NULL);

    //types
    REGISTER_UMOCK_ALIAS_TYPE(TO_JSON_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(FROM_JSON_FUNCTION, void*);
}

BEGIN_TEST_SUITE(prov_sc_shared_helpers_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    register_global_mocks();
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

    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();

    g_fromJson_calls = 0;
    g_toJson_calls = 0;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
{
    int result = 0;
    if (skip_array != NULL)
    {
        for (size_t index = 0; index < length; index++)
        {
            if (current_index == skip_array[index])
            {
                result = __LINE__;
                break;
            }
        }
    }

    return result;
}

/* UNIT TESTS BEGIN */

/*NOTE WELL: These tests do not cover all shared helpers. They only cover functions that are not covered by other unittests
(i.e. many other prov_sc tests compile with the shared helpers and directly test their functionality in context of others) */

TEST_FUNCTION(json_deserialize_and_get_struct_array_NULL_dest_arr)
{
    //arrange
    size_t dest_len;

    //act
    int result = json_deserialize_and_get_struct_array(NULL, &dest_len, TEST_JSON_OBJECT, DUMMY_STRING, (FROM_JSON_FUNCTION)from_json_mock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

TEST_FUNCTION(json_deserialize_and_get_struct_array_NULL_dest_len)
{
    //arrange
    void** dest_arr;

    //act
    int result = json_deserialize_and_get_struct_array(&dest_arr, NULL, TEST_JSON_OBJECT, DUMMY_STRING, (FROM_JSON_FUNCTION)from_json_mock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

TEST_FUNCTION(json_deserialize_and_get_struct_array_GOLDEN)
{
    //arrange

    STRICT_EXPECTED_CALL(json_object_get_array(TEST_JSON_OBJECT, DUMMY_STRING));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(json_array_get_object(IGNORED_PTR_ARG, i));
        STRICT_EXPECTED_CALL(from_json_mock(IGNORED_PTR_ARG));
    }

    size_t dest_len;
    void** dest_arr;

    //act
    int result = json_deserialize_and_get_struct_array(&dest_arr, &dest_len, TEST_JSON_OBJECT, DUMMY_STRING, (FROM_JSON_FUNCTION)from_json_mock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(dest_arr);
    ASSERT_ARE_EQUAL(size_t, (size_t)ARRAY_SIZE, dest_len);

    //cleanup
    free_struct_arr(dest_arr, dest_len);
}

TEST_FUNCTION(json_deserialize_and_get_struct_array_ERROR)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    size_t dest_len;
    void** dest_arr;

    STRICT_EXPECTED_CALL(json_object_get_array(TEST_JSON_OBJECT, DUMMY_STRING));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(json_array_get_object(IGNORED_PTR_ARG, i));
        STRICT_EXPECTED_CALL(from_json_mock(IGNORED_PTR_ARG));
    }
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "json_serialize_and_get_struct_array_ERROR failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int result = json_deserialize_and_get_struct_array(&dest_arr, &dest_len, TEST_JSON_OBJECT, DUMMY_STRING, (FROM_JSON_FUNCTION)from_json_mock);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);
    }
}

TEST_FUNCTION(json_serialize_and_set_struct_array_NULL_arr)
{
    //arrange

    //act
    int result = json_serialize_and_set_struct_array(TEST_JSON_OBJECT, DUMMY_STRING, NULL, ARRAY_SIZE, (TO_JSON_FUNCTION)to_json_mock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

TEST_FUNCTION(json_serialize_and_set_struct_array_GOLDEN)
{
    //arrange
    void** arr = (void**)real_malloc(ARRAY_SIZE * sizeof(void*));
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        arr[i] = (void*)real_malloc(1);
    }

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_array());
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(to_json_mock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_array_append_value(IGNORED_PTR_ARG, TO_JSON_MOCK_RETURN));
    }
    STRICT_EXPECTED_CALL(json_object_set_value(TEST_JSON_OBJECT, DUMMY_STRING, IGNORED_PTR_ARG));

    //act
    int result = json_serialize_and_set_struct_array(TEST_JSON_OBJECT, DUMMY_STRING, arr, ARRAY_SIZE, (TO_JSON_FUNCTION)to_json_mock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    //cleanup
    free_struct_arr(arr, ARRAY_SIZE);
}

TEST_FUNCTION(json_serialize_and_set_struct_array_ERROR)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    void** arr = (void**)real_malloc(ARRAY_SIZE * sizeof(void*));
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        arr[i] = (void*)real_malloc(1);
    }

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_array());
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(to_json_mock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_array_append_value(IGNORED_PTR_ARG, TO_JSON_MOCK_RETURN));
    }
    STRICT_EXPECTED_CALL(json_object_set_value(TEST_JSON_OBJECT, DUMMY_STRING, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { 1 };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "json_serialize_and_set_struct_array_ERROR failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int result = json_serialize_and_set_struct_array(TEST_JSON_OBJECT, DUMMY_STRING, arr, ARRAY_SIZE, (TO_JSON_FUNCTION)to_json_mock);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);
    }

    //cleanup
    free_struct_arr(arr, ARRAY_SIZE);
}

END_TEST_SUITE(prov_sc_shared_helpers_ut);
