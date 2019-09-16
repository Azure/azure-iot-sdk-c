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

#undef ENABLE_MOCKS

#include "prov_service_client/provisioning_sc_twin.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

//Control Parameters
static const char* TEST_JSON = "{\"a\": 2}";
static const char* TEST_JSON2 = "{\"b\": 1}";
static const char* TEST_JSON_NESTED = "{\"a\": 2, \"b\": {\"c\": \"value\"}}";
static const char* TEST_JSON_NESTED2 = "{\"d\": 1, \"e\": {\"f\": \"value\"}}";
static const char* TEST_EMPTY_JSON = "{}";
static char* DUMMY_JSON_STRING = "{\"json\":\"dummy\"}";
static const char* DUMMY_JSON_VALUE = "dummy";

#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112

//TEST_DEFINE_ENUM_TYPE(NECESSITY, NECESSITY_VALUES);
//IMPLEMENT_UMOCK_C_ENUM_TYPE(NECESSITY, NECESSITY_VALUES);

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
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, DUMMY_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_RETURN(json_parse_string, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_value, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_value, JSONFailure);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_wrapping_value, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_wrapping_value, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_serialize_to_string, DUMMY_JSON_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);
}

BEGIN_TEST_SUITE(provisioning_sc_twin_ut)

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

static void expected_calls_twinCollection_toJson()
{
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
}

static void expected_calls_twinProperties_toJson()
{
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    expected_calls_twinCollection_toJson();
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void expected_calls_initialTwin_toJson(bool tags, bool desired_properties)
{
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    if (tags)
    {
        expected_calls_twinCollection_toJson();
        STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    if (desired_properties)
    {
        expected_calls_twinProperties_toJson();
        STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void expected_calls_twinCollection_fromJson()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_wrapping_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG)); //cannot fail
}

static void expected_calls_twinProperties_fromJson(bool desired)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    if (desired)
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        expected_calls_twinCollection_fromJson();
    }
    else
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL);
    }
}

static void expected_calls_initialTwin_fromJson(bool tags, bool desired_properties)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    if (tags)
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        expected_calls_twinCollection_fromJson();
    }
    else
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL);
    }
    if (desired_properties)
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        expected_calls_twinProperties_fromJson(desired_properties);
    }
    else
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL);
    }
}

/* UNIT TESTS BEGIN */

/*Tests_PROV_TWIN_22_004: [ Upon successful creation of a new initialTwin, initialTwin_create shall return a INITIAL_TWIN_HANDLE to access the model ]*/
TEST_FUNCTION(initialTwin_create_desired_properties_null)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON, initialTwin_getTags(twin));
    ASSERT_IS_NULL(initialTwin_getDesiredProperties(twin));

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_002: [ Passing a value of "{}" for tags or desired_properties is the same as passing NULL ]*/
/*Tests_PROV_TWIN_22_004: [ Upon successful creation of a new initialTwin, initialTwin_create shall return a INITIAL_TWIN_HANDLE to access the model ]*/
TEST_FUNCTION(initialTwin_create_desired_properties_empty)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, TEST_EMPTY_JSON);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON, initialTwin_getTags(twin));
    ASSERT_IS_NULL(initialTwin_getDesiredProperties(twin));

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_004: [ Upon successful creation of a new initialTwin, initialTwin_create shall return a INITIAL_TWIN_HANDLE to access the model ]*/
TEST_FUNCTION(initialTwin_create_tags_null)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, NULL, initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON, initialTwin_getDesiredProperties(twin));

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_002: [ Passing a value of "{}" for tags or desired_properties is the same as passing NULL ]*/
/*Tests_PROV_TWIN_22_004: [ Upon successful creation of a new initialTwin, initialTwin_create shall return a INITIAL_TWIN_HANDLE to access the model ]*/
TEST_FUNCTION(initialTwin_create_tags_empty)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_EMPTY_JSON, TEST_JSON);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, NULL, initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON, initialTwin_getDesiredProperties(twin));

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_004: [ Upon successful creation of a new initialTwin, initialTwin_create shall return a INITIAL_TWIN_HANDLE to access the model ]*/
TEST_FUNCTION(initialTwin_create_both_args)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON2));

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, TEST_JSON2);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON, initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON2, initialTwin_getDesiredProperties(twin));

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_001: [ If both tags and desired_properties are NULL, initialTwin_create shall fail and return NULL ]*/
TEST_FUNCTION(initialTwin_create_both_null)
{
    //arrange

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, NULL);

    //assert
    ASSERT_IS_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_PROV_TWIN_22_001: [ If both tags and desired_properties are NULL, initialTwin_create shall fail and return NULL ]*/
/*Tests_PROV_TWIN_22_002: [ Passing a value of "{}" for tags or desired_properties is the same as passing NULL ]*/
TEST_FUNCTION(initialTwin_create_both_empty)
{
    //arrange

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_EMPTY_JSON, TEST_EMPTY_JSON);

    //assert
    ASSERT_IS_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_PROV_TWIN_22_003: [ If allocating memory for the new initialTwin fails, initialTwin_create shall fail and return NULL ]*/
TEST_FUNCTION(initialTwin_create_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON2));

    umock_c_negative_tests_snapshot();
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count;

    for (size_t index = 0; index < count; index++)
    {
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "initialTwin_create failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, TEST_JSON2);

        //assert
        ASSERT_IS_NULL(twin, tmp_msg);

        //cleanup
        initialTwin_destroy(twin);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

/*Tests_PROV_TWIN_22_005: [ initialTwin_destroy shall free all memory contained within the twin handle ]*/
TEST_FUNCTION(initialTwin_destroy_full)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, TEST_JSON2);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->tags->json
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->tags
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->properties->desired->json
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->properties->desired
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->properties
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin

    //act
    initialTwin_destroy(twin);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_PROV_TWIN_22_005: [ initialTwin_destroy shall free all memory contained within the twin handle ]*/
TEST_FUNCTION(initialTwin_destroy_no_tags)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON2);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->properties->desired->json
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->properties->desired
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->properties
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin

    //act
    initialTwin_destroy(twin);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_PROV_TWIN_22_005: [ initialTwin_destroy shall free all memory contained within the twin handle ]*/
TEST_FUNCTION(initialTwin_destroy_no_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->tags->json
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin->tags
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //twin

    //act
    initialTwin_destroy(twin);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_PROV_TWIN_22_005: [ initialTwin_destroy shall free all memory contained within the twin handle ]*/
TEST_FUNCTION(initialTwin_destroy_null)
{
    //act
    initialTwin_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_PROV_TWIN_22_007: [ Otherwise, initialTwin_getTags shall return the tags contained in twin ]*/
/*Tests_PROV_TWIN_22_014: [ Otherwise, initialTwin_getDesiredProperties shall return the desired properties contained in twin ]*/
TEST_FUNCTION(initialTwin_getters_success)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, TEST_JSON2);
    umock_c_reset_all_calls();

    //act
    const char* tags = initialTwin_getTags(twin);
    const char* desired_properties = initialTwin_getDesiredProperties(twin);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON, tags);
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON2, desired_properties);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_006: [ If the given handle, twin, is NULL, or there are no tags in twin, initialTwin_getTags shall return NULL ]*/
/*Tests_PROV_TWIN_22_013: [ If the given handle, twin, is NULL, or there are no desired properties in twin, initialTwin_getDesiredProperties shall return NULL ]*/
TEST_FUNCTION(initialTwin_getters_null_twin)
{
    //act
    const char* tags = initialTwin_getTags(NULL);
    const char* desired_properties = initialTwin_getDesiredProperties(NULL);

    //assert
    ASSERT_IS_NULL(tags);
    ASSERT_IS_NULL(desired_properties);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_PROV_TWIN_22_006: [ If the given handle, twin, is NULL, or there are no tags in twin, initialTwin_getTags shall return NULL ]*/
TEST_FUNCTION(initialTwin_getTags_no_tags)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON2);
    umock_c_reset_all_calls();

    //act
    const char* tags = initialTwin_getTags(twin);

    //assert
    ASSERT_IS_NULL(tags);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_013: [ If the given handle, twin, is NULL, or there are no desired properties in twin, initialTwin_getDesiredProperties shall return NULL ]*/
TEST_FUNCTION(initialTwin_getDesiredProperties_no_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);
    umock_c_reset_all_calls();

    //act
    const char* desired_properties = initialTwin_getDesiredProperties(twin);

    //assert
    ASSERT_IS_NULL(desired_properties);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_008: [ If the given handle, twin is NULL, initialTwin_setTags shall fail and return a non-zero value ]*/
TEST_FUNCTION(initialTwin_setTags_null_twin)
{
    //act
    int result = initialTwin_setTags(NULL, TEST_JSON);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_PROV_TWIN_22_009: [ The tags value of twin shall be set to the given value tags ]*/
/*Tests_PROV_TWIN_22_010: [ Passing a value of "{}" for tags is the same as passing NULL, which will clear any already set value ]*/
/*Tests_PROV_TWIN_22_012: [ Upon success, initialTwin_setTags shall return 0 ]*/
TEST_FUNCTION(initialTwin_setTags_null_tag)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = initialTwin_setTags(twin, NULL);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_009: [ The tags value of twin shall be set to the given value tags ]*/
/*Tests_PROV_TWIN_22_010: [ Passing a value of "{}" for tags is the same as passing NULL, which will clear any already set value ]*/
/*Tests_PROV_TWIN_22_012: [ Upon success, initialTwin_setTags shall return 0 ]*/
TEST_FUNCTION(initialTwin_setTags_empty_tag)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = initialTwin_setTags(twin, NULL);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_009: [ The tags value of twin shall be set to the given value tags ]*/
/*Tests_PROV_TWIN_22_012: [ Upon success, initialTwin_setTags shall return 0 ]*/
TEST_FUNCTION(initialTwin_setTags_no_existing_tags)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON2);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));

    //act
    int result = initialTwin_setTags(twin, TEST_JSON);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON, initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_011: [ If allocating memory in twin for the new value fails, initialTwin_setTags shall fail and return a non-zero value ]*/
TEST_FUNCTION(initialTwin_setTags_no_existing_tags_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON2);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));

    umock_c_negative_tests_snapshot();
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count;

    for (size_t index = 0; index < count; index++)
    {
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "initialTwin_setTags (no existing tags) failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int result = initialTwin_setTags(twin, TEST_JSON);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);

        //cleanup
        initialTwin_setTags(twin, NULL);
    }

    //cleanup
    initialTwin_destroy(twin);
    umock_c_negative_tests_deinit();
}

/*Tests_PROV_TWIN_22_009: [ The tags value of twin shall be set to the given value tags ]*/
/*Tests_PROV_TWIN_22_012: [ Upon success, initialTwin_setTags shall return 0 ]*/
TEST_FUNCTION(initialTwin_setTags_overwrite_existing_tags)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON2));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = initialTwin_setTags(twin, TEST_JSON2);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON2, initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Tests_PROV_TWIN_22_011: [ If allocating memory in twin for the new value fails, initialTwin_setTags shall fail and return a non-zero value ]*/
TEST_FUNCTION(initialTwin_setTags_overwrite_existing_tags_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON2));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
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
        sprintf(tmp_msg, "initialTwin_setTags (overwrite existing tags) failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int result = initialTwin_setTags(twin, TEST_JSON2);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);

        //cleanup
        initialTwin_setTags(twin, TEST_JSON);
    }

    //cleanup
    initialTwin_destroy(twin);
    umock_c_negative_tests_deinit();
}

/*Test_PROV_TWIN_22_015: [ If the given handle, twin is NULL, initialTwin_setDesiredProperties shall fail and return a non-zero value ]*/
TEST_FUNCTION(initialTwin_setDesiredProperties_null_twin)
{
    //act
    int result = initialTwin_setDesiredProperties(NULL, TEST_JSON);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Test_PROV_TWIN_22_016: [ The desired properties value of twin shall be set to the given value desired_properties ]*/
/*Test_PROV_TWIN_22_017: [ Passing a value of "{}" for desired_properties is the same as passing NULL, which will clear any already set value ]*/
/*Test_PROV_TWIN_22_019: [ Upon success, initialTwin_setDesiredProperties shall return 0 ]*/
TEST_FUNCTION(initialTwin_setDesiredProperties_null_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = initialTwin_setDesiredProperties(twin, NULL);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(initialTwin_getDesiredProperties(twin));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Test_PROV_TWIN_22_016: [ The desired properties value of twin shall be set to the given value desired_properties ]*/
/*Test_PROV_TWIN_22_017: [ Passing a value of "{}" for desired_properties is the same as passing NULL, which will clear any already set value ]*/
/*Test_PROV_TWIN_22_019: [ Upon success, initialTwin_setDesiredProperties shall return 0 ]*/
TEST_FUNCTION(initialTwin_setDesiredProperties_empty_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = initialTwin_setDesiredProperties(twin, TEST_EMPTY_JSON);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(initialTwin_getDesiredProperties(twin));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Test_PROV_TWIN_22_016: [ The desired properties value of twin shall be set to the given value desired_properties ]*/
/*Test_PROV_TWIN_22_019: [ Upon success, initialTwin_setDesiredProperties shall return 0 ]*/
TEST_FUNCTION(initialTwin_setDesiredProperties_no_existing_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON2));

    //act
    int result = initialTwin_setDesiredProperties(twin, TEST_JSON2);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON2, initialTwin_getDesiredProperties(twin));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Test_PROV_TWIN_22_018: [ If allocating memory in twin for the new value fails, initialTwin_setDesiredProperties shall fail and return a non-zero value ]*/
TEST_FUNCTION(initialTwin_setDesiredProperties_no_existing_desired_properties_fails)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON2, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON));

    umock_c_negative_tests_snapshot();
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count;

    for (size_t index = 0; index < count; index++)
    {
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "initialTwin_setDesiredProperties (no existing desired properties) failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int result = initialTwin_setDesiredProperties(twin, TEST_JSON);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);

        //cleanup
        initialTwin_setDesiredProperties(twin, NULL);
    }

    //cleanup
    initialTwin_destroy(twin);
    umock_c_negative_tests_deinit();
}

/*Test_PROV_TWIN_22_016: [ The desired properties value of twin shall be set to the given value desired_properties ]*/
/*Test_PROV_TWIN_22_019: [ Upon success, initialTwin_setDesiredProperties shall return 0 ]*/
TEST_FUNCTION(initialTwin_setDesiredProperties_overwrite_existing_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON2));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = initialTwin_setDesiredProperties(twin, TEST_JSON2);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON2, initialTwin_getDesiredProperties(twin));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

/*Test_PROV_TWIN_22_018: [ If allocating memory in twin for the new value fails, initialTwin_setDesiredProperties shall fail and return a non-zero value ]*/
TEST_FUNCTION(initialTwin_setDesiredProperties_overwrite_existing_desired_properties_fails)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_JSON2));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

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
        sprintf(tmp_msg, "initialTwin_setDesiredProperties (overwrite existing desired properties) failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int result = initialTwin_setDesiredProperties(twin, TEST_JSON2);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);

        //cleanup
        initialTwin_setDesiredProperties(twin, NULL);
    }

    //cleanup
    initialTwin_destroy(twin);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(initialTwin_toJson_null)
{
    //arrange

    //act
    JSON_Value* result = initialTwin_toJson(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(initialTwin_toJson_only_tags)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, NULL);
    umock_c_reset_all_calls();
    expected_calls_initialTwin_toJson(true, false);

    //act
    JSON_Value* result = initialTwin_toJson(twin);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_IS_TRUE(result == TEST_JSON_VALUE);

    //cleanup
    //cleanup
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_toJson_only_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_JSON);
    umock_c_reset_all_calls();
    expected_calls_initialTwin_toJson(false, true);

    //act
    JSON_Value* result = initialTwin_toJson(twin);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_IS_TRUE(result == TEST_JSON_VALUE);

    //cleanup
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_toJson_tags_and_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, TEST_JSON2);
    umock_c_reset_all_calls();
    expected_calls_initialTwin_toJson(true, true);

    //act
    JSON_Value* result = initialTwin_toJson(twin);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_IS_TRUE(result == TEST_JSON_VALUE);

    //cleanup
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_toJson_failure)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON, TEST_JSON2);
    umock_c_reset_all_calls();
    expected_calls_initialTwin_toJson(true, true);

    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0;
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "initialTwin_toJson failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* result = initialTwin_toJson(twin);

        //assert
        ASSERT_IS_NULL(result);
    }

    //cleanup
    initialTwin_destroy(twin);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(initialTwin_fromJson_null)
{
    //arrange

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(NULL);

    //assert
    ASSERT_IS_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(initialTwin_fromJson_only_tags)
{
    //arrange
    expected_calls_initialTwin_fromJson(true, false);

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_fromJson_only_desired_properties)
{
    //arrange
    expected_calls_initialTwin_fromJson(false, true);

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_fromJson_tags_and_desired_properties)
{
    //arrange
    expected_calls_initialTwin_fromJson(true, true);

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_fromJson_tags_and_desired_properties_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    expected_calls_initialTwin_fromJson(true, true);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 6, 7, 9, 14 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);;
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "initialTwin_FromJson_tags_and_desired_properties_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(twin, tmp_msg);

        //cleanup
        initialTwin_destroy(twin);
    }

    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(provisioning_sc_twin_ut);
