// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
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
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/strings.h"
#include "umock_c/umock_c_prod.h"
#undef ENABLE_MOCKS

#include "prov_service_client/provisioning_sc_twin.h"
#include "parson.h"
#include "real_strings.h"


static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

//Control Parameters
static const char* TEST_TAGS_JSON = "{\"a\":2}";
static const char* TEST_DESIRED_PROPERTIES_JSON = "{\"b\":1}";
static const char* TEST_JSON_NESTED = "{\"a\":2,\"b\":{\"c\":\"value\"}}";
static const char* TEST_JSON_NESTED2 = "{\"d\":1,\"e\":{\"f\":\"value\"}}";
static const char* TEST_EMPTY_JSON = "{}";

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static STRING_HANDLE create_twin_json(const char* tags, const char* desired_properties)
{
    STRING_HANDLE json = STRING_construct("{");
    STRING_HANDLE original = STRING_clone(json);

    if (tags != NULL)
    {
        STRING_concat(json, "\"tags\":");
        STRING_concat(json, tags);
    }
    if (desired_properties != NULL)
    {
        if (STRING_compare(json, original) != 0)
        {
            STRING_concat(json, ",");
        }
        STRING_concat(json, "\"properties\":{");
        STRING_concat(json, "\"desired\":");
        STRING_concat(json, desired_properties);
        STRING_concat(json, "}");
    }

    STRING_concat(json, "}");
    STRING_delete(original);
    return json;
}

static void register_global_mocks()
{
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_STRING_GLOBAL_MOCK_HOOK;
}


BEGIN_TEST_SUITE(provisioning_sc_twin_int)

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

/* INTEGRATION TESTS BEGIN */

TEST_FUNCTION(initialTwin_fromJson_only_tags)
{
    //arrange
    STRING_HANDLE json = create_twin_json(TEST_TAGS_JSON, NULL);
    JSON_Value* root_value = json_parse_string(STRING_c_str(json));
    JSON_Object* root_object = json_value_get_object(root_value);

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(root_object);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, TEST_TAGS_JSON, initialTwin_getTags(twin));
    ASSERT_IS_NULL(initialTwin_getDesiredProperties(twin));

    //cleanup
    STRING_delete(json);
    json_value_free(root_value);
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_fromJson_only_desired_properties)
{
    //arrange
    STRING_HANDLE json = create_twin_json(NULL, TEST_DESIRED_PROPERTIES_JSON);
    JSON_Value* root_value = json_parse_string(STRING_c_str(json));
    JSON_Object* root_object = json_value_get_object(root_value);

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(root_object);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, TEST_DESIRED_PROPERTIES_JSON, initialTwin_getDesiredProperties(twin));
    ASSERT_IS_NULL(initialTwin_getTags(twin));

    //cleanup
    STRING_delete(json);
    json_value_free(root_value);
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_fromJson_tags_and_desired_properties)
{
    //arrange
    STRING_HANDLE json = create_twin_json(TEST_TAGS_JSON, TEST_DESIRED_PROPERTIES_JSON);
    JSON_Value* root_value = json_parse_string(STRING_c_str(json));
    JSON_Object* root_object = json_value_get_object(root_value);

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(root_object);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, TEST_TAGS_JSON, initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, TEST_DESIRED_PROPERTIES_JSON, initialTwin_getDesiredProperties(twin));

    //cleanup
    STRING_delete(json);
    json_value_free(root_value);
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_fromJson_nested)
{
    //arrange
    STRING_HANDLE json = create_twin_json(TEST_JSON_NESTED, TEST_JSON_NESTED2);
    JSON_Value* root_value = json_parse_string(STRING_c_str(json));
    JSON_Object* root_object = json_value_get_object(root_value);

    //act
    INITIAL_TWIN_HANDLE twin = initialTwin_fromJson(root_object);

    //assert
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON_NESTED, initialTwin_getTags(twin));
    ASSERT_ARE_EQUAL(char_ptr, TEST_JSON_NESTED2, initialTwin_getDesiredProperties(twin));

    //cleanup
    STRING_delete(json);
    json_value_free(root_value);
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_toJson_only_tags)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_TAGS_JSON, NULL);
    STRING_HANDLE expected_json = create_twin_json(TEST_TAGS_JSON, NULL);

    //act
    JSON_Value* twin_val = initialTwin_toJson(twin);
    char* json = json_serialize_to_string(twin_val);

    //assert

    ASSERT_IS_NOT_NULL(json);
    ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(expected_json), json);

    //cleanup
    json_free_serialized_string(json);
    json_value_free(twin_val);
    STRING_delete(expected_json);
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_toJson_only_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(NULL, TEST_DESIRED_PROPERTIES_JSON);
    STRING_HANDLE expected_json = create_twin_json(NULL, TEST_DESIRED_PROPERTIES_JSON);

    //act
    JSON_Value* twin_val = initialTwin_toJson(twin);
    char* json = json_serialize_to_string(twin_val);

    //assert
    ASSERT_IS_NOT_NULL(json);
    ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(expected_json), json);

    //cleanup
    json_free_serialized_string(json);
    json_value_free(twin_val);
    STRING_delete(expected_json);
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_toJson_tags_and_desired_properties)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_TAGS_JSON, TEST_DESIRED_PROPERTIES_JSON);
    STRING_HANDLE expected_json = create_twin_json(TEST_TAGS_JSON, TEST_DESIRED_PROPERTIES_JSON);

    //act
    JSON_Value* twin_val = initialTwin_toJson(twin);
    char* json = json_serialize_to_string(twin_val);

    //assert
    ASSERT_IS_NOT_NULL(json);
    ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(expected_json), json);

    //cleanup
    json_free_serialized_string(json);
    json_value_free(twin_val);
    STRING_delete(expected_json);
    initialTwin_destroy(twin);
}

TEST_FUNCTION(initialTwin_toJson_nested)
{
    //arrange
    INITIAL_TWIN_HANDLE twin = initialTwin_create(TEST_JSON_NESTED, TEST_JSON_NESTED2);
    STRING_HANDLE expected_json = create_twin_json(TEST_JSON_NESTED, TEST_JSON_NESTED2);

    //act
    JSON_Value* twin_val = initialTwin_toJson(twin);
    char* json = json_serialize_to_string(twin_val);

    //assert
    ASSERT_IS_NOT_NULL(json);
    ASSERT_ARE_EQUAL(char_ptr, STRING_c_str(expected_json), json);

    //cleanup
    json_free_serialized_string(json);
    json_value_free(twin_val);
    STRING_delete(expected_json);
    initialTwin_destroy(twin);
}

END_TEST_SUITE(provisioning_sc_twin_int);
