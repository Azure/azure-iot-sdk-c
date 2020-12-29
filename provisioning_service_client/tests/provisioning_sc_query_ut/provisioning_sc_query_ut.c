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
#include "prov_service_client/provisioning_sc_shared_helpers.h"
#include "prov_service_client/provisioning_sc_enrollment.h"
#include "prov_service_client/provisioning_sc_device_registration_state.h"
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

#undef ENABLE_MOCKS

#include "prov_service_client/provisioning_sc_query.h"
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
static const char* DUMMY_REGISTRATION_ID = "REGISTRATION_ID";
static const char* DUMMY_DEVICE_ID = "DEVICE_ID";
static const char* DUMMY_GROUP_ID = "GROUP_ID";
static const char* DUMMY_IOTHUB_HOSTNAME = "IOTHUB_HOSTNAME";
static const char* DUMMY_ETAG = "ETAG";
static const char* DUMMY_CREATED_TIME = "CREATED TIME";
static const char* DUMMY_UPDATED_TIME = "UPDATED_TIME";
static const char* DUMMY_PROVISIONING_STATUS = "enabled";
static const char* QUERY_STRING = "*";
static int QUERY_RESP_SIZE = 5;


#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112
#define TEST_JSON_ARRAY (JSON_Array*)0x11111113

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)real_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static void** my_struct_array_fromJson(JSON_Array* json_arr, size_t len, FROM_JSON_FUNCTION fromJson)
{
    (void)json_arr;
    (void)fromJson;

    void** ret_arr = (void**)real_malloc(len * sizeof(void*));
    for (size_t i = 0; i < len; i++)
    {
        ret_arr[i] = (void*)real_malloc(sizeof(void*));
    }
    return ret_arr;
}

static void my_individualEnrollment_destroy(INDIVIDUAL_ENROLLMENT_HANDLE ie)
{
    real_free(ie);
}

static void my_enrollmentGroup_destroy(ENROLLMENT_GROUP_HANDLE eg)
{
    real_free(eg);
}

static void my_deviceRegistrationState_destroy(DEVICE_REGISTRATION_STATE_HANDLE drs)
{
    real_free(drs);
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
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_array, TEST_JSON_ARRAY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_array, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_number, DUMMY_NUM);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_number, 0);
    REGISTER_GLOBAL_MOCK_RETURN(json_array_get_count, QUERY_RESP_SIZE);

    //enrollments
    REGISTER_GLOBAL_MOCK_HOOK(individualEnrollment_destroy, my_individualEnrollment_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(enrollmentGroup_destroy, my_enrollmentGroup_destroy);

    //drs
    REGISTER_GLOBAL_MOCK_HOOK(deviceRegistrationState_destroy, my_deviceRegistrationState_destroy);

    //shared helpers
    REGISTER_GLOBAL_MOCK_RETURN(json_serialize_and_set_struct_array, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_and_set_struct_array, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(struct_array_fromJson, my_struct_array_fromJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(struct_array_fromJson, NULL);

    //types
    REGISTER_UMOCK_ALIAS_TYPE(TO_JSON_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(FROM_JSON_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(void**, void*);
    REGISTER_UMOCK_ALIAS_TYPE(INDIVIDUAL_ENROLLMENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ENROLLMENT_GROUP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_REGISTRATION_STATE_HANDLE, void*);
}

BEGIN_TEST_SUITE(provisioning_sc_query_ut)

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

static PROVISIONING_QUERY_RESPONSE* get_query_response(PROVISIONING_QUERY_TYPE type, size_t size)
{
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)).SetReturn(size);

    return queryResponse_deserializeFromJson(DUMMY_JSON, type);
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

TEST_FUNCTION(querySpecification_serializeToJson_null_qs)
{
    //arrange

    //act
    char* json = querySpecification_serializeToJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(json);

    //cleanup
}

TEST_FUNCTION(querySpecification_serializeToJson_invalid_version)
{
    //arrange
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.version = -1;
    qs.query_string = QUERY_STRING;

    //act
    char* json = querySpecification_serializeToJson(&qs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(json);

    //cleanup
}

TEST_FUNCTION(querySpecification_serializeToJson_golden)
{
    //arrange
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;
    qs.query_string = QUERY_STRING;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, QUERY_STRING));
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));

    //act
    char* json = querySpecification_serializeToJson(&qs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(json);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_JSON, json);

    //cleanup
    free(json);
}

TEST_FUNCTION(querySpecification_serializeToJson_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;
    qs.query_string = QUERY_STRING;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, QUERY_STRING));
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 5, 6 };
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
        sprintf(tmp_msg, "querySpecification_serializeToJson_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        char* json = querySpecification_serializeToJson(&qs);

        //assert
        ASSERT_IS_NULL(json, tmp_msg);
    }

    //cleanup
}

TEST_FUNCTION(queryResponse_deserializeFromJson_null_json)
{
    //arrange

    //act
    PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(NULL, QUERY_TYPE_INDIVIDUAL_ENROLLMENT);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(qr);

    //cleanup
}

TEST_FUNCTION(queryResponse_deserializeFromJson_invalid_query_type)
{
    //arrange

    //act
    PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_INVALID);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(qr);

    //cleanup
}

TEST_FUNCTION(queryResponse_deserializeFromJson_golden_individualEnrollment)
{
    //arrange
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(struct_array_fromJson(IGNORED_PTR_ARG, IGNORED_NUM_ARG, (FROM_JSON_FUNCTION)individualEnrollment_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));

    //act
    PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_INDIVIDUAL_ENROLLMENT);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(qr);
    ASSERT_ARE_EQUAL(size_t, QUERY_RESP_SIZE, qr->response_arr_size);
    ASSERT_IS_TRUE(qr->response_arr_type == QUERY_TYPE_INDIVIDUAL_ENROLLMENT);
    ASSERT_IS_NOT_NULL(qr->response_arr.ie);

    //cleanup
    queryResponse_free(qr);
}

TEST_FUNCTION(queryResponse_deserializeFromJson_error_individualEnrollment)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(struct_array_fromJson(IGNORED_PTR_ARG, IGNORED_NUM_ARG, (FROM_JSON_FUNCTION)individualEnrollment_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 5 };
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
        sprintf(tmp_msg, "queryResponse_deserializeFromJson_error_individualEnrollment failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_INDIVIDUAL_ENROLLMENT);

        //assert
        ASSERT_IS_NULL(qr, tmp_msg);
    }
}

TEST_FUNCTION(queryResponse_deserializeFromJson_golden_individualEnrollment_empty_arr)
{
    //arrange
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));

    //act
    PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_INDIVIDUAL_ENROLLMENT);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(qr);
    ASSERT_ARE_EQUAL(size_t, 0, qr->response_arr_size);
    ASSERT_IS_TRUE(qr->response_arr_type == QUERY_TYPE_INDIVIDUAL_ENROLLMENT);
    ASSERT_IS_NULL(qr->response_arr.eg);

    //cleanup
    queryResponse_free(qr);
}

TEST_FUNCTION(queryResponse_deserializeFromJson_error_individualEnrollment_empty_arr)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)).SetReturn(0); //cannot fail
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 4 };
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
        sprintf(tmp_msg, "queryResponse_deserializeFromJson_error_individualEnrollment_empty_arr failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_INDIVIDUAL_ENROLLMENT);

        //assert
        ASSERT_IS_NULL(qr, tmp_msg);
    }
}

TEST_FUNCTION(queryResponse_deserializeFromJson_golden_enrollmentGroup)
{
    //arrange
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(struct_array_fromJson(IGNORED_PTR_ARG, IGNORED_NUM_ARG, (FROM_JSON_FUNCTION)enrollmentGroup_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));

    //act
    PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_ENROLLMENT_GROUP);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(qr);
    ASSERT_ARE_EQUAL(size_t, QUERY_RESP_SIZE, qr->response_arr_size);
    ASSERT_IS_TRUE(qr->response_arr_type == QUERY_TYPE_ENROLLMENT_GROUP);
    ASSERT_IS_NOT_NULL(qr->response_arr.eg);

    //cleanup
    queryResponse_free(qr);
}

TEST_FUNCTION(queryResponse_deserializeFromJson_error_enrollmentGroup)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(struct_array_fromJson(IGNORED_PTR_ARG, IGNORED_NUM_ARG, (FROM_JSON_FUNCTION)enrollmentGroup_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 5 };
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
        sprintf(tmp_msg, "queryResponse_deserializeFromJson_error_enrollmentGroup failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_ENROLLMENT_GROUP);

        //assert
        ASSERT_IS_NULL(qr, tmp_msg);
    }
}

TEST_FUNCTION(queryResponse_deserializeFromJson_golden_enrollmentGroup_empty_arr)
{
    //arrange
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));

    //act
    PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_ENROLLMENT_GROUP);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(qr);
    ASSERT_ARE_EQUAL(size_t, 0, qr->response_arr_size);
    ASSERT_IS_TRUE(qr->response_arr_type == QUERY_TYPE_ENROLLMENT_GROUP);
    ASSERT_IS_NULL(qr->response_arr.eg);

    //cleanup
    queryResponse_free(qr);
}

TEST_FUNCTION(queryResponse_deserializeFromJson_error_enrollmentGroup_empty_arr)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)).SetReturn(0); //cannot fail
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 4 };
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
        sprintf(tmp_msg, "queryResponse_deserializeFromJson_error_enrollmentGroup_empty_arr failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_ENROLLMENT_GROUP);

        //assert
        ASSERT_IS_NULL(qr, tmp_msg);
    }
}

TEST_FUNCTION(queryResponse_deserializeFromJson_golden_deviceRegistrationState)
{
    //arrange
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(struct_array_fromJson(IGNORED_PTR_ARG, IGNORED_NUM_ARG, (FROM_JSON_FUNCTION)deviceRegistrationState_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));

    //act
    PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_DEVICE_REGISTRATION_STATE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(qr);
    ASSERT_ARE_EQUAL(size_t, QUERY_RESP_SIZE, qr->response_arr_size);
    ASSERT_IS_TRUE(qr->response_arr_type == QUERY_TYPE_DEVICE_REGISTRATION_STATE);
    ASSERT_IS_NOT_NULL(qr->response_arr.eg);

    //cleanup
    queryResponse_free(qr);
}

TEST_FUNCTION(queryResponse_deserializeFromJson_error_deviceRegistrationState)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(struct_array_fromJson(IGNORED_PTR_ARG, IGNORED_NUM_ARG, (FROM_JSON_FUNCTION)deviceRegistrationState_fromJson));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 5 };
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
        sprintf(tmp_msg, "queryResponse_deserializeFromJson_error_deviceRegistrationState failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_DEVICE_REGISTRATION_STATE);

        //assert
        ASSERT_IS_NULL(qr, tmp_msg);
    }
}

TEST_FUNCTION(queryResponse_deserializeFromJson_golden_deviceRegistrationState_empty_arr)
{
    //arrange
    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));

    //act
    PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_DEVICE_REGISTRATION_STATE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(qr);
    ASSERT_ARE_EQUAL(size_t, 0, qr->response_arr_size);
    ASSERT_IS_TRUE(qr->response_arr_type == QUERY_TYPE_DEVICE_REGISTRATION_STATE);
    ASSERT_IS_NULL(qr->response_arr.eg);

    //cleanup
    queryResponse_free(qr);
}

TEST_FUNCTION(queryResponse_deserializeFromJson_error_deviceRegistrationState_empty_arr)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(json_parse_string(DUMMY_JSON));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG)).SetReturn(0); //cannot fail
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 4 };
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
        sprintf(tmp_msg, "queryResponse_deserializeFromJson_error_deviceRegistrationState_empty_arr failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_QUERY_RESPONSE* qr = queryResponse_deserializeFromJson(DUMMY_JSON, QUERY_TYPE_DEVICE_REGISTRATION_STATE);

        //assert
        ASSERT_IS_NULL(qr, tmp_msg);
    }
}

TEST_FUNCTION(queryType_stringToEnum_null)
{
    //arrange

    //act
    PROVISIONING_QUERY_TYPE result = queryType_stringToEnum(NULL);

    //assert
    ASSERT_IS_TRUE(result == QUERY_TYPE_INVALID);

    //cleanup
}

TEST_FUNCTION(queryType_stringToEnum_individual_enrollment)
{
    //arrange

    //act
    PROVISIONING_QUERY_TYPE result = queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT);

    //assert
    ASSERT_IS_TRUE(result == QUERY_TYPE_INDIVIDUAL_ENROLLMENT);

    //cleanup
}

TEST_FUNCTION(queryType_stringToEnum_enrollment_group)
{
    //arrange

    //act
    PROVISIONING_QUERY_TYPE result = queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP);

    //assert
    ASSERT_IS_TRUE(result == QUERY_TYPE_ENROLLMENT_GROUP);

    //cleanup
}

TEST_FUNCTION(queryType_stringToEnum_device_registration_state)
{
    //arrange

    //act
    PROVISIONING_QUERY_TYPE result = queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE);

    //assert
    ASSERT_IS_TRUE(result == QUERY_TYPE_DEVICE_REGISTRATION_STATE);

    //cleanup
}

TEST_FUNCTION(queryType_stringToEnum_invalid_string)
{
    //arrange

    //act
    PROVISIONING_QUERY_TYPE result = queryType_stringToEnum("cannot-convert-to-enum");

    //assert
    ASSERT_IS_TRUE(result == QUERY_TYPE_INVALID);

    //cleanup
}

/*Tests_PROV_QUERY_22_001: [ queryResponse_free shall free all memory in the structure pointed to by query_resp ]*/
TEST_FUNCTION(queryResponse_free_null)
{
    //arrange

    //act
    queryResponse_free(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_PROV_QUERY_22_001: [ queryResponse_free shall free all memory in the structure pointed to by query_resp ]*/
TEST_FUNCTION(queryResponse_free_ie_with_results)
{
    //arrange
    PROVISIONING_QUERY_RESPONSE* qr = get_query_response(QUERY_TYPE_INDIVIDUAL_ENROLLMENT, 5);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    queryResponse_free(qr);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_PROV_QUERY_22_001: [ queryResponse_free shall free all memory in the structure pointed to by query_resp ]*/
TEST_FUNCTION(queryResponse_free_ie_no_results)
{
    //arrange
    PROVISIONING_QUERY_RESPONSE* qr = get_query_response(QUERY_TYPE_INDIVIDUAL_ENROLLMENT, 0);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    queryResponse_free(qr);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_PROV_QUERY_22_001: [ queryResponse_free shall free all memory in the structure pointed to by query_resp ]*/
TEST_FUNCTION(queryResponse_free_eg_with_results)
{
    //arrange
    PROVISIONING_QUERY_RESPONSE* qr = get_query_response(QUERY_TYPE_ENROLLMENT_GROUP, 5);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    queryResponse_free(qr);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_PROV_QUERY_22_001: [ queryResponse_free shall free all memory in the structure pointed to by query_resp ]*/
TEST_FUNCTION(queryResponse_free_eg_no_results)
{
    //arrange
    PROVISIONING_QUERY_RESPONSE* qr = get_query_response(QUERY_TYPE_ENROLLMENT_GROUP, 0);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    queryResponse_free(qr);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_PROV_QUERY_22_001: [ queryResponse_free shall free all memory in the structure pointed to by query_resp ]*/
TEST_FUNCTION(queryResponse_free_drs_with_results)
{
    //arrange
    PROVISIONING_QUERY_RESPONSE* qr = get_query_response(QUERY_TYPE_DEVICE_REGISTRATION_STATE, 5);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(deviceRegistrationState_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceRegistrationState_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceRegistrationState_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceRegistrationState_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceRegistrationState_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    queryResponse_free(qr);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_PROV_QUERY_22_001: [ queryResponse_free shall free all memory in the structure pointed to by query_resp ]*/
TEST_FUNCTION(queryResponse_free_drs_no_results)
{
    //arrange
    PROVISIONING_QUERY_RESPONSE* qr = get_query_response(QUERY_TYPE_DEVICE_REGISTRATION_STATE, 0);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    queryResponse_free(qr);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

END_TEST_SUITE(provisioning_sc_query_ut);
