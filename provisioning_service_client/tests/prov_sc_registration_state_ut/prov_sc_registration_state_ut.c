// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

static void* real_malloc(size_t size)
{
    return malloc(size);
}

static void real_free(void* ptr)
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

#undef ENABLE_MOCKS

#include "prov_service_client/provisioning_sc_device_registration_state.h"
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

static const char* DUMMY_REGISTRATION_ID = "REGISTRATION_ID";
static const char* DUMMY_CREATED_DATE_TIME_UTC = "CREATED_DATE_TIME_UTC";
static const char* DUMMY_DEVICE_ID = "DEVICE_ID";
static const char* DUMMY_UPDATED_DATE_TIME_UTC = "UPDATED_DATE_TIME_UTC";
static const char* DUMMY_ERROR_MESSAGE = "ERROR_MESSAGE";
static const char* DUMMY_ETAG = "ETAG";
static int DUMMY_ERROR_CODE = 4747;

#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)real_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
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
}

BEGIN_TEST_SUITE(prov_sc_registration_state_ut)

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

static void expected_calls_deviceRegistrationState_fromJson()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_REGISTRATION_ID); //can't "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_CREATED_DATE_TIME_UTC); //can't "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_DEVICE_ID); //can't "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(REGISTRATION_STATUS_JSON_VALUE_ASSIGNED); //can't "fail"
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_UPDATED_DATE_TIME_UTC); //can't "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_ERROR_MESSAGE); //can't "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_ETAG); //can't "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_ERROR_CODE); //can't "fail"
}

static DEVICE_REGISTRATION_STATE_HANDLE make_dummy_drs()
{
    expected_calls_deviceRegistrationState_fromJson();
    DEVICE_REGISTRATION_STATE_HANDLE drs = deviceRegistrationState_fromJson(TEST_JSON_OBJECT);
    umock_c_reset_all_calls();
    return drs;
}

/* UNIT TESTS BEGIN */

TEST_FUNCTION(deviceRegistrationState_getRegistrationId_null)
{
    //arrange

    //act
    const char* result = deviceRegistrationState_getRegistrationId(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_getRegistrationId_success)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    //act
    const char* result = deviceRegistrationState_getRegistrationId(drs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_REGISTRATION_ID, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_getCreatedDateTime_null)
{
    //arrange

    //act
    const char* result = deviceRegistrationState_getCreatedDateTime(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_getCreatedDateTime_success)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    //act
    const char* result = deviceRegistrationState_getCreatedDateTime(drs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_CREATED_DATE_TIME_UTC, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_getDeviceId_null)
{
    //arrange

    //act
    const char* result = deviceRegistrationState_getDeviceId(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_getDeviceId_success)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    //act
    const char* result = deviceRegistrationState_getDeviceId(drs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_DEVICE_ID, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_getRegistrationStatus_null)
{
    //arrange

    //act
    REGISTRATION_STATUS result = deviceRegistrationState_getRegistrationStatus(NULL);

    //assert
    ASSERT_IS_TRUE(result == REGISTRATION_STATUS_ERROR);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_getRegistrationStatus_success)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    //act
    REGISTRATION_STATUS result = deviceRegistrationState_getRegistrationStatus(drs);

    //assert
    ASSERT_IS_TRUE(result == REGISTRATION_STATUS_ASSIGNED);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_getUpdatedDateTime_null)
{
    //arrange

    //act
    const char* result = deviceRegistrationState_getUpdatedDateTime(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_getUpdatedDateTime_success)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    //act
    const char* result = deviceRegistrationState_getUpdatedDateTime(drs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_UPDATED_DATE_TIME_UTC, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_getErrorCode_null)
{
    //arrange

    //act
    int result = deviceRegistrationState_getErrorCode(NULL);

    //assert
    ASSERT_ARE_EQUAL(int, -1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_getErrorCode_success)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    //act
    int result = deviceRegistrationState_getErrorCode(drs);

    //assert
    ASSERT_ARE_EQUAL(int, DUMMY_ERROR_CODE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_getErrorMessage_null)
{
    //arrange

    //act
    const char* result = deviceRegistrationState_getErrorMessage(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_getErrorMessage_success)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    //act
    const char* result = deviceRegistrationState_getErrorMessage(drs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ERROR_MESSAGE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_getEtag_null)
{
    //arrange

    //act
    const char* result = deviceRegistrationState_getEtag(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_getEtag_success)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    //act
    const char* result = deviceRegistrationState_getEtag(drs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_destroy_null)
{
    //arrange

    //act
    deviceRegistrationState_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_destroy_full)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs = make_dummy_drs();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    deviceRegistrationState_destroy(drs);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_fromJson_null)
{
    //arrange

    //act
    DEVICE_REGISTRATION_STATE_HANDLE drs = deviceRegistrationState_fromJson(NULL);

    //assert
    ASSERT_IS_NULL(drs);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceRegistrationState_fromJson_full)
{
    //arrange
    expected_calls_deviceRegistrationState_fromJson();

    //act
    DEVICE_REGISTRATION_STATE_HANDLE drs = deviceRegistrationState_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(drs);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_REGISTRATION_ID, deviceRegistrationState_getRegistrationId(drs));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_CREATED_DATE_TIME_UTC, deviceRegistrationState_getCreatedDateTime(drs));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_DEVICE_ID, deviceRegistrationState_getDeviceId(drs));
    ASSERT_IS_TRUE(deviceRegistrationState_getRegistrationStatus(drs) == REGISTRATION_STATUS_ASSIGNED);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_UPDATED_DATE_TIME_UTC, deviceRegistrationState_getUpdatedDateTime(drs));
    ASSERT_ARE_EQUAL(int, DUMMY_ERROR_CODE, deviceRegistrationState_getErrorCode(drs));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ERROR_MESSAGE, deviceRegistrationState_getErrorMessage(drs));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, deviceRegistrationState_getEtag(drs));

    //cleanup
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(deviceRegistrationState_fromJson_full_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    expected_calls_deviceRegistrationState_fromJson();
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 3, 5, 7, 8, 10, 12, 14 };
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
        sprintf(tmp_msg, "deviceRegistrationState_fromJson_full_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        DEVICE_REGISTRATION_STATE_HANDLE drs = deviceRegistrationState_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(drs, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(prov_sc_registration_state_ut);
