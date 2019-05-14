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
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

#include "prov_service_client/provisioning_sc_device_capabilities.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "prov_service_client/provisioning_sc_shared_helpers.h"
#include "prov_service_client/provisioning_sc_models_serializer.h"
#include "parson.h"
#include "umock_c/umock_c_prod.h"

MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value*, value);
MOCKABLE_FUNCTION(, int, json_object_get_boolean, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_boolean, JSON_Object*, object, const char*, name, int, bool_val);
#undef ENABLE_MOCKS

static JSON_Object* TEST_JSON_CAPS = (JSON_Object*)0x2456;

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static JSON_Value* my_json_value_init_object(void)
{
    return (JSON_Value*)real_malloc(1);
}

static void my_json_value_free(JSON_Value *value)
{
    real_free(value);
}

static void register_global_mocks()
{
    (void)umocktypes_bool_register_types();
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_CAPABILITIES_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, real_free);

    REGISTER_GLOBAL_MOCK_HOOK(json_value_init_object, my_json_value_init_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, (JSON_Object*)11);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_boolean, 1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_boolean, -1);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_boolean, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_boolean, JSONFailure);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, my_json_value_free);
}

BEGIN_TEST_SUITE(prov_sc_dev_caps_ut)

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
TEST_FUNCTION(deviceCapabilities_create_malloc_NULL_fail)
{
    DEVICE_CAPABILITIES_HANDLE handle;

    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

    //act
    handle = deviceCapabilities_create();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(handle);

    //cleanup
}

TEST_FUNCTION(deviceCapabilities_create_success)
{
    DEVICE_CAPABILITIES_HANDLE handle;

    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    //act
    handle = deviceCapabilities_create();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);

    //cleanup
    deviceCapabilities_destroy(handle);
}

TEST_FUNCTION(deviceCapabilities_destroy_handle_NULL_success)
{
    //arrange

    //act
    deviceCapabilities_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceCapabilities_destroy_success)
{
    //arrange
    DEVICE_CAPABILITIES_HANDLE handle = deviceCapabilities_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    deviceCapabilities_destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceCapabilities_isIotEdgeCapable_success)
{
    bool is_edge;

    //arrange
    DEVICE_CAPABILITIES_HANDLE handle = deviceCapabilities_create();
    umock_c_reset_all_calls();

    //act
    is_edge = deviceCapabilities_isIotEdgeCapable(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(is_edge);

    //cleanup
    deviceCapabilities_destroy(handle);
}

TEST_FUNCTION(deviceCapabilities_isIotEdgeCapable_handle_NULL_success)
{
    //arrange

    //act
    bool is_edge = deviceCapabilities_isIotEdgeCapable(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(is_edge);

    //cleanup
}

TEST_FUNCTION(deviceCapabilities_setIotEdgeCapable_success)
{
    bool is_edge = true;

    //arrange
    DEVICE_CAPABILITIES_HANDLE handle = deviceCapabilities_create();
    umock_c_reset_all_calls();

    //act
    deviceCapabilities_setIotEdgeCapable(handle, is_edge);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(deviceCapabilities_isIotEdgeCapable(handle) );

    //cleanup
    deviceCapabilities_destroy(handle);
}

TEST_FUNCTION(deviceCapabilities_toJson_caps_NULL_fail)
{
    JSON_Value* json;

    //arrange
    umock_c_reset_all_calls();

    //act
    json = deviceCapabilities_toJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(deviceCapabilities_toJson_fail)
{
    JSON_Value* json;

    //arrange
    DEVICE_CAPABILITIES_HANDLE handle = deviceCapabilities_create();
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_boolean(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "deviceCapabilities_toJson failure in test %zu/%zu", index, count);

        json = deviceCapabilities_toJson(handle);

        //assert
        ASSERT_IS_NULL(json, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
    deviceCapabilities_destroy(handle);
}

TEST_FUNCTION(deviceCapabilities_toJson_success)
{
    JSON_Value* json;

    //arrange
    DEVICE_CAPABILITIES_HANDLE handle = deviceCapabilities_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_boolean(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    //act
    json = deviceCapabilities_toJson(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(json);

    //cleanup
    deviceCapabilities_destroy(handle);
    my_json_value_free(json);
}

TEST_FUNCTION(deviceCapabilities_fromJson_json_object_NULL_fail)
{
    //arrange
    DEVICE_CAPABILITIES_HANDLE handle;
    umock_c_reset_all_calls();

    //act
    handle = deviceCapabilities_fromJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(handle);

    //cleanup
    deviceCapabilities_destroy(handle);
}

TEST_FUNCTION(deviceCapabilities_fromJson_fail)
{
    //arrange
    DEVICE_CAPABILITIES_HANDLE handle;
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(true);
    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "deviceCapabilities_fromJson failure in test %zu/%zu", index, count);

        handle = deviceCapabilities_fromJson(TEST_JSON_CAPS);

        //assert
        ASSERT_IS_NULL(handle, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(deviceCapabilities_fromJson_success)
{
    //arrange
    DEVICE_CAPABILITIES_HANDLE handle;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_boolean(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(true);

    //act
    handle = deviceCapabilities_fromJson(TEST_JSON_CAPS);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_IS_TRUE(deviceCapabilities_isIotEdgeCapable(handle));

    //cleanup
    deviceCapabilities_destroy(handle);
}

END_TEST_SUITE(prov_sc_dev_caps_ut);
