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
#include "azure_c_shared_utility/const_defines.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "prov_service_client/provisioning_sc_shared_helpers.h"
#include "prov_service_client/provisioning_sc_enrollment.h"
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
MOCKABLE_FUNCTION(, int, json_object_get_boolean, const JSON_Object*, object, const char*, name);

#undef ENABLE_MOCKS

#include "prov_service_client/provisioning_sc_bulk_operation.h"
#include "prov_service_client/provisioning_sc_models_serializer.h"
#include "prov_service_client/provisioning_sc_json_const.h"

static TEST_MUTEX_HANDLE g_testByTest;

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

//Toggles
static bool error_arr_is_empty = false;

#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112
#define TEST_ARRAY_SIZE (size_t)5

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)real_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static int my_copy_json_string_field(char** dest, JSON_Object* root_object, const char* json_key)
{
    AZURE_UNREFERENCED_PARAMETER(root_object);
    AZURE_UNREFERENCED_PARAMETER(json_key);

    my_mallocAndStrcpy_s(dest, DUMMY_STRING);

    return 0;
}

static int my_deserialize_and_get_struct_array(void*** dest_arr, size_t* dest_len, JSON_Object* root_object, const char* json_key, FROM_JSON_FUNCTION element_fromJson)
{
    AZURE_UNREFERENCED_PARAMETER(root_object);
    AZURE_UNREFERENCED_PARAMETER(json_key);
    AZURE_UNREFERENCED_PARAMETER(element_fromJson);

    if (error_arr_is_empty)
    {
        *dest_len = 0;
        *dest_arr = NULL;
    }
    else
    {
        *dest_len = TEST_ARRAY_SIZE;
        *dest_arr = (void**)real_malloc(*dest_len * sizeof(PROVISIONING_BULK_OPERATION_ERROR*));
        for (size_t i = 0; i < *dest_len; i++)
        {
            (*dest_arr)[i] = real_malloc(sizeof(PROVISIONING_BULK_OPERATION));
            memset((*dest_arr)[i], 0, sizeof(PROVISIONING_BULK_OPERATION));
        }
    }
    return 0;
}

static INDIVIDUAL_ENROLLMENT_HANDLE* create_dummy_enrollment_list(size_t len)
{
    INDIVIDUAL_ENROLLMENT_HANDLE* ret;
    ret = (INDIVIDUAL_ENROLLMENT_HANDLE*)real_malloc(len * sizeof(INDIVIDUAL_ENROLLMENT_HANDLE));
    return ret;
}

static void free_dummy_enrollment_list(INDIVIDUAL_ENROLLMENT_HANDLE* enrollments)
{
    real_free(enrollments);
}

static void register_global_mocks()
{
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, real_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

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
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_boolean, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_boolean, -1);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_number, DUMMY_NUM);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_number, 0);

    //shared helpers
    REGISTER_GLOBAL_MOCK_RETURN(json_serialize_and_set_struct_array, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_and_set_struct_array, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(json_deserialize_and_get_struct_array, my_deserialize_and_get_struct_array);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_deserialize_and_get_struct_array, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(copy_json_string_field, my_copy_json_string_field);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(copy_json_string_field, MU_FAILURE);

    //types
    REGISTER_UMOCK_ALIAS_TYPE(TO_JSON_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(FROM_JSON_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(void**, void*);
    REGISTER_UMOCK_ALIAS_TYPE(INDIVIDUAL_ENROLLMENT_HANDLE*, void**);
}

BEGIN_TEST_SUITE(prov_sc_bulk_operation_ut)

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

TEST_FUNCTION(bulkOperation_serializeToJson_null_handle)
{
    //arrange

    //act
    char* json = bulkOperation_serializeToJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(json);

    //cleanup
}

TEST_FUNCTION(bulkOperation_serializeToJson_null_enrollments)
{
    //arrange
    PROVISIONING_BULK_OPERATION bulk_op;
    bulk_op.version = PROVISIONING_BULK_OPERATION_VERSION_1;
    bulk_op.enrollments.ie = NULL;
    bulk_op.num_enrollments = 2;
    bulk_op.mode = BULK_OP_CREATE;
    bulk_op.type = BULK_OP_INDIVIDUAL_ENROLLMENT;

    //act
    char* json = bulkOperation_serializeToJson(&bulk_op);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(json);

    //cleanup
}

TEST_FUNCTION(bulkOperation_serializeToJson_invalid_size)
{
    //arrange
    PROVISIONING_BULK_OPERATION bulk_op;
    bulk_op.version = PROVISIONING_BULK_OPERATION_VERSION_1;
    bulk_op.enrollments.ie = create_dummy_enrollment_list(2);
    bulk_op.num_enrollments = 0;
    bulk_op.mode = BULK_OP_CREATE;
    bulk_op.type = BULK_OP_INDIVIDUAL_ENROLLMENT;

    //act
    char* json = bulkOperation_serializeToJson(&bulk_op);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(json);

    //cleanup
    free_dummy_enrollment_list(bulk_op.enrollments.ie);
}

TEST_FUNCTION(bulkOperation_serializeToJson_invalid_version)
{
    //arrange
    PROVISIONING_BULK_OPERATION bulk_op;
    bulk_op.version = -1;
    bulk_op.enrollments.ie = create_dummy_enrollment_list(2);
    bulk_op.num_enrollments = 2;
    bulk_op.mode = BULK_OP_CREATE;
    bulk_op.type = BULK_OP_INDIVIDUAL_ENROLLMENT;

    //act
    char* json = bulkOperation_serializeToJson(&bulk_op);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(json);

    //cleanup
    free_dummy_enrollment_list(bulk_op.enrollments.ie);
}

TEST_FUNCTION(bulkOperation_serializeToJson_success_ie)
{
    //arrange
    PROVISIONING_BULK_OPERATION bulk_op;
    bulk_op.version = PROVISIONING_BULK_OPERATION_VERSION_1;
    bulk_op.enrollments.ie = create_dummy_enrollment_list(2);
    bulk_op.num_enrollments = 2;
    bulk_op.mode = BULK_OP_CREATE;
    bulk_op.type = BULK_OP_INDIVIDUAL_ENROLLMENT;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_serialize_and_set_struct_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, (void**)bulk_op.enrollments.ie, bulk_op.num_enrollments, (TO_JSON_FUNCTION)individualEnrollment_toJson));
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));

    //act
    char* json = bulkOperation_serializeToJson(&bulk_op);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(json);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_JSON, json);

    //cleanup
    free_dummy_enrollment_list(bulk_op.enrollments.ie);
    free(json);
}

TEST_FUNCTION(bulkOperation_serializeToJson_error_ie)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_BULK_OPERATION bulk_op;
    bulk_op.version = -1;
    bulk_op.enrollments.ie = create_dummy_enrollment_list(2);
    bulk_op.num_enrollments = 2;
    bulk_op.mode = BULK_OP_CREATE;
    bulk_op.type = BULK_OP_INDIVIDUAL_ENROLLMENT;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_serialize_and_set_struct_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, (void**)bulk_op.enrollments.ie, bulk_op.num_enrollments, (TO_JSON_FUNCTION)individualEnrollment_toJson));
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 6, 7 };
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
        sprintf(tmp_msg, "BulkOperation_serializeToJson_error_ie failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        char* json = bulkOperation_serializeToJson(&bulk_op);

        //assert
        ASSERT_IS_NULL(json, tmp_msg);
    }

    //cleanup
    free_dummy_enrollment_list(bulk_op.enrollments.ie);
}

TEST_FUNCTION(bulkOperationError_fromJson_null)
{
    //arrange

    //act
    PROVISIONING_BULK_OPERATION_ERROR* err = bulkOperationError_fromJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(err);

    //cleanup
}

TEST_FUNCTION(bulkOperationError_fromJson_success)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(copy_json_string_field(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(copy_json_string_field(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    PROVISIONING_BULK_OPERATION_ERROR* err = bulkOperationError_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(err);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_STRING, err->registration_id);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_STRING, err->error_status);
    ASSERT_ARE_EQUAL(size_t, DUMMY_NUM, err->error_code);

    //cleanup
    free(err->registration_id);
    free(err->error_status);
    free(err);
}

TEST_FUNCTION(bulkOperationError_fromJson_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(copy_json_string_field(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(copy_json_string_field(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3 };
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
        sprintf(tmp_msg, "bulkOperationError_fromJson_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_BULK_OPERATION_ERROR* err = bulkOperationError_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(err, tmp_msg);
    }

    //cleanup
}

TEST_FUNCTION(bulkOperationResult_deserializeFromJson_null_json)
{
    //arrange

    //act
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res = bulkOperationResult_deserializeFromJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(bulk_res);

    //cleanup
}

TEST_FUNCTION(bulkOperationResult_deserializeFromJson_success_has_errors)
{
    //arrange
    error_arr_is_empty = false;
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(json_deserialize_and_get_struct_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, (FROM_JSON_FUNCTION)bulkOperationError_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));

    //act
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res = bulkOperationResult_deserializeFromJson(DUMMY_JSON);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(bulk_res);
    ASSERT_IS_NOT_NULL(bulk_res->errors);
    ASSERT_ARE_EQUAL(size_t, TEST_ARRAY_SIZE, bulk_res->num_errors);
    ASSERT_IS_FALSE(bulk_res->is_successful);

    //cleanup
    bulkOperationResult_free(bulk_res);
}

TEST_FUNCTION(bulkOperationResult_deserializeFromJson_error_has_errors)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    error_arr_is_empty = false;
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(json_deserialize_and_get_struct_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, (FROM_JSON_FUNCTION)bulkOperationError_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 5 };
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
        sprintf(tmp_msg, "bulkOperationResult_deserializeFromJson_error_has_errors failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_BULK_OPERATION_RESULT* bulk_res = bulkOperationResult_deserializeFromJson(DUMMY_JSON);

        //assert
        ASSERT_IS_NULL(bulk_res, tmp_msg);
    }

    //cleanup
}

TEST_FUNCTION(bulkOperationResult_deserializeFromJson_success_no_errors)
{
    //arrange
    error_arr_is_empty = true;
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(1);
    STRICT_EXPECTED_CALL(json_deserialize_and_get_struct_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, (FROM_JSON_FUNCTION)bulkOperationError_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));

    //act
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res = bulkOperationResult_deserializeFromJson(DUMMY_JSON);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(bulk_res);
    ASSERT_IS_NULL(bulk_res->errors);
    ASSERT_ARE_EQUAL(size_t, 0, bulk_res->num_errors);
    ASSERT_IS_TRUE(bulk_res->is_successful);

    //cleanup
    bulkOperationResult_free(bulk_res);
}

TEST_FUNCTION(bulkOperationResult_deserializeFromJson_error_no_errors)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    error_arr_is_empty = true;
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(1);
    STRICT_EXPECTED_CALL(json_deserialize_and_get_struct_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, (FROM_JSON_FUNCTION)bulkOperationError_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 5 };
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
        sprintf(tmp_msg, "bulkOperationResult_deserializeFromJson_error_no_errors failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_BULK_OPERATION_RESULT* bulk_res = bulkOperationResult_deserializeFromJson(DUMMY_JSON);

        //assert
        ASSERT_IS_NULL(bulk_res, tmp_msg);
    }

    //cleanup
}

TEST_FUNCTION(bulkOperationResult_free_null)
{
    //arrange

    //act
    bulkOperationResult_free(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(bulkOperationResult_free_with_errors)
{
    //arrange
    error_arr_is_empty = false;
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res = bulkOperationResult_deserializeFromJson(DUMMY_JSON);
    umock_c_reset_all_calls();

    for (size_t i = 0; i < bulk_res->num_errors; i++)
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    bulkOperationResult_free(bulk_res);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(bulkOperationResult_free_no_errors)
{
    //arrange
    error_arr_is_empty = true;
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res = bulkOperationResult_deserializeFromJson(DUMMY_JSON);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    bulkOperationResult_free(bulk_res);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(prov_sc_bulk_operation_ut);
