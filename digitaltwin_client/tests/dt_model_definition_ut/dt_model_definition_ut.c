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

static void* my_gballoc_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "real_parson.h"

#define ENABLE_MOCKS
#include "parson.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/strings.h"
#include "digitaltwin_device_client.h"
#include "digitaltwin_interface_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, JSON_Object *, json_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_clear, JSON_Object*, object);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);

#ifdef __cplusplus
}
#endif

#undef ENABLE_MOCKS

#include "digitaltwin_model_definition.h"
#include "real_map.h"
#include "real_strings.h"

typedef struct MODEL_DEFINITION_CLIENT_TAG *MODEL_DEFINITION_CLIENT_HANDLE;

TEST_DEFINE_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES);

static char* TEST_INTERFACE_NAME = "Test Interface";
static char* TEST_VALUE = "VALUE";
static const char* TEST_KEY = "KEY";
static char *TEST_READ_DATA = "TEST DATA TO PUBLISH";
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE temp_interface_handle = (DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1234;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES);
MU_DEFINE_ENUM_STRINGS(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);

BEGIN_TEST_SUITE(dt_model_definition_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_TYPE(MAP_RESULT, MAP_RESULT);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_FILTER_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);

    REGISTER_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_INTERFACE_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_COMMAND_EXECUTE_CALLBACK, void*);
    
    //Success and fail return
    REGISTER_GLOBAL_MOCK_RETURNS(DigitalTwin_InterfaceClient_Create, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY);
    REGISTER_GLOBAL_MOCK_RETURNS(DigitalTwin_InterfaceClient_SetCommandsCallback, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);

    REGISTER_GLOBAL_MOCK_HOOK(json_value_init_object, real_json_value_init_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object, real_json_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_serialize_to_string, real_json_serialize_to_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);   
    REGISTER_GLOBAL_MOCK_HOOK(json_value_get_object, real_json_value_get_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);   
    REGISTER_GLOBAL_MOCK_HOOK(json_object_set_string, real_json_object_set_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_string, JSONFailure);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_string, real_json_object_get_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, real_json_value_free);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_calloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_MAP_GLOBAL_MOCK_HOOK;

}

TEST_FUNCTION_INITIALIZE(TestMethodInit)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
}

static void set_expected_calls_for_DigitalTwin_ModelDefinition_Destroy()
{
    STRICT_EXPECTED_CALL(DigitalTwin_InterfaceClient_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_DigitalTwin_ModelDefinition_Create(void)
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DigitalTwin_InterfaceClient_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CopyOutArgumentBuffer_dtInterfaceClientHandle(&temp_interface_handle, 1);
    STRICT_EXPECTED_CALL(DigitalTwin_InterfaceClient_SetCommandsCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void set_expected_calls_for_DigitalTwin_ModelDefinition_Publish_Interface()
{
    STRICT_EXPECTED_CALL(Map_AddOrUpdate(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail().SetReturn(MAP_OK);
}

// test_allocate_and_register_DT_interface performs the required processing for creating a MODEL_DEFINITION_CLIENT_HANDLE mdHandle and connecting
// it to a (unit test) clientCore handle and also simulating its registration.  Interface is able to use the handle at this point.
static MODEL_DEFINITION_CLIENT_HANDLE create_test_MD_handle()
{
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR;
    set_expected_calls_for_DigitalTwin_ModelDefinition_Create();
    MODEL_DEFINITION_CLIENT_HANDLE h = NULL;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE ih = NULL;
    result = DigitalTwin_ModelDefinition_Create(&h, &ih);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);

    ASSERT_IS_NOT_NULL(h);
    ASSERT_IS_NOT_NULL(ih);
    umock_c_reset_all_calls();
    return h;
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_ModelDefinition_Destroy
///////////////////////////////////////////////////////////////////////////////

TEST_FUNCTION(DigitalTwin_ModelDefinition_Destroy_ok)
{
    // arrange
    MODEL_DEFINITION_CLIENT_HANDLE h = create_test_MD_handle();
    set_expected_calls_for_DigitalTwin_ModelDefinition_Destroy();

    //act
    DigitalTwin_ModelDefinition_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_ModelDefinition_Create
///////////////////////////////////////////////////////////////////////////////

TEST_FUNCTION(DigitalTwin_ModelDefinition_Create_ok)
{
    // arrange
    set_expected_calls_for_DigitalTwin_ModelDefinition_Create();

    //act
    MODEL_DEFINITION_CLIENT_HANDLE h = NULL;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE iHandle = NULL;
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_ModelDefinition_Create(&h, &iHandle);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);

    //assert
    ASSERT_IS_NOT_NULL(iHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    DigitalTwin_ModelDefinition_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_ModelDefinition_Create_fails)
{
    //arrange
    int nResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, nResult);
    MODEL_DEFINITION_CLIENT_HANDLE h = NULL;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE iHandle = NULL;
    DIGITALTWIN_CLIENT_RESULT result;

    set_expected_calls_for_DigitalTwin_ModelDefinition_Create();

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            result = DigitalTwin_ModelDefinition_Create(&h, &iHandle);

            //assert
            ASSERT_IS_NULL(h, "DigitalTwin_ModelDefinition_Create failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        }
    }

    //cleanup
    DigitalTwin_ModelDefinition_Destroy(h);
    umock_c_negative_tests_deinit();
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_ModelDefinition_Publish_Interface
///////////////////////////////////////////////////////////////////////////////

TEST_FUNCTION(DigitalTwin_ModelDefinition_Publish_Interface_ok)
{
    // arrange
    DIGITALTWIN_CLIENT_RESULT result;
    MODEL_DEFINITION_CLIENT_HANDLE h = create_test_MD_handle();
    set_expected_calls_for_DigitalTwin_ModelDefinition_Publish_Interface();

    //act
    result = DigitalTwin_ModelDefinition_Publish_Interface(TEST_INTERFACE_NAME, TEST_READ_DATA, h);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);

    // cleanup
    DigitalTwin_ModelDefinition_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_ModelDefinition_Publish_Interface_fails)
{
    //arrange
    int result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);
    
    DIGITALTWIN_CLIENT_RESULT dt_result;
    MODEL_DEFINITION_CLIENT_HANDLE h = create_test_MD_handle();
    set_expected_calls_for_DigitalTwin_ModelDefinition_Publish_Interface();

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            dt_result = DigitalTwin_ModelDefinition_Publish_Interface(TEST_INTERFACE_NAME, TEST_READ_DATA, h);

            //assert
            ASSERT_IS_NOT_NULL(h, "DigitalTwin_ModelDefinition_Publish_Interface failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        }
    }

    //cleanup
    DigitalTwin_ModelDefinition_Destroy(h);
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(dt_model_definition_ut)

