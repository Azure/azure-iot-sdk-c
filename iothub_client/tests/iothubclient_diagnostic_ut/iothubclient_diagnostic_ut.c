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

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"

#include "real_parson.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/map.h"
#include "iothub_message.h"
#include "parson.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, double, json_object_get_number, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, const char*, json_object_dotget_string, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Status, json_object_dotset_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, JSON_Array*, json_array_get_array, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Object*, json_array_get_object, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value*, value);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_array_clear, JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_object_clear, JSON_Object*, object);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, char *, json_serialize_to_string_pretty, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_dotset_value, JSON_Object *, object, const char *, name, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object *, json_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_dotget_value, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, double, json_object_dotget_number, const JSON_Object*, object, const char*, name);

#ifdef __cplusplus
}
#endif


#undef ENABLE_MOCKS

#include "internal/iothub_client_diagnostic.h"

bool g_fail_string_construct_sprintf;
bool g_fail_string_concat_with_string;

static const char* TEST_STRING_VALUE = "Test string value";

#define TEST_STRING_HANDLE (STRING_HANDLE)0x46

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static STRING_HANDLE my_STRING_new(void)
{
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
{
    (void)handle;
    (void)format;
    return 0;
}

STRING_HANDLE STRING_construct_sprintf(const char* psz, ...)
{
    (void)psz;
    STRING_HANDLE result;
    if (g_fail_string_construct_sprintf)
    {
        result = (STRING_HANDLE)NULL;
    }
    else
    {
        result = (STRING_HANDLE)my_gballoc_malloc(1);
    }
    return result;
}

static int m_STRING_concat_with_STRING(STRING_HANDLE handle, STRING_HANDLE arg1)
{
    (void)handle;
    (void)arg1;
    int result;
    if (g_fail_string_concat_with_string)
    {
        result = -1;
    }
    else
    {
        result = 0;
    }
    return result;
}

static STRING_HANDLE my_STRING_clone(STRING_HANDLE handle)
{
    (void)handle;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    (void)handle;

    if (handle != TEST_STRING_HANDLE)
    {
        my_gballoc_free(handle);
    }
}

int my_STRING_concat(STRING_HANDLE handle, const char* s2)
{
    (void)handle;
    (void)s2;
    return 0;
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

static IOTHUB_MESSAGE_HANDLE TEST_MESSAGE_HANDLE = (IOTHUB_MESSAGE_HANDLE)0x12;


#define INDEFINITE_TIME ((time_t)-1)
static time_t g_current_time;

BEGIN_TEST_SUITE(iothubclient_diagnostic_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    g_current_time = time(NULL);

    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetDiagnosticPropertyData, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetDiagnosticPropertyData, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(Map_Add, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Add, MAP_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, g_current_time);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(get_time, INDEFINITE_TIME);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat, my_STRING_concat);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat_with_STRING, m_STRING_concat_with_STRING);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, my_STRING_clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_clone, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    umock_c_reset_all_calls();
    g_fail_string_construct_sprintf = false;
    g_fail_string_concat_with_string = false;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_001: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if diagSetting or messageHandle is NULL. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_with_null_messageHanlde_fails)
{
    //arrange

    //act
    int result = IoTHubClient_Diagnostic_AddIfNecessary((IOTHUB_DIAGNOSTIC_SETTING_DATA*)0x1, NULL);

    //assert
    ASSERT_IS_FALSE(result == 0);

    //cleanup
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_001: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if diagSetting or messageHandle is NULL. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_with_null_setting_fails)
{
    //arrange

    //act
    int result = IoTHubClient_Diagnostic_AddIfNecessary(NULL, TEST_MESSAGE_HANDLE);

    //assert
    ASSERT_IS_FALSE(result == 0);

    //cleanup
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_002: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if failing to add diagnostic property. ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_fails)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        100,    /*diagnostic sampling percentage*/
        0        /*message number*/
    };

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticPropertyData(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        int result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_IS_FALSE(result == 0);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_003: [ If diagSamplingPercentage is equal to 0, message number should not be increased and no diagnostic properties added ]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_no_diag_info_with_percentage_0)
{
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        0,        /*diagnostic sampling percentage*/
        0        /*message number*/
    };

    umock_c_reset_all_calls();
    int result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, TEST_MESSAGE_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(result == 0);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, 0);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_004: [ If diagSamplingPercentage is equal to 100, diagnostic properties should be added to all messages]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_diag_info_with_percentage_100)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        100,    /*diagnostic sampling percentage*/
        0        /*message number*/
    };

    umock_c_reset_all_calls();


    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticPropertyData(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, TEST_MESSAGE_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(result == 0);
    ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, 1);
}

/* Tests_SRS_IOTHUB_DIAGNOSTIC_13_005: [ If diagSamplingPercentage is between(0, 100), diagnostic properties should be added based on percentage]*/
TEST_FUNCTION(IoTHubClient_Diagnostic_AddIfNecessary_diag_info_with_normal_percentage)
{
    //arrange
    IOTHUB_DIAGNOSTIC_SETTING_DATA diag_setting =
    {
        50,        /*diagnostic sampling percentage*/
        0        /*message number*/
    };

    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDiagnosticPropertyData(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    for (uint32_t index = 0; index < 2; ++index)
    {
        int result = IoTHubClient_Diagnostic_AddIfNecessary(&diag_setting, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(uint32_t, diag_setting.currentMessageNumber, index + 1);
        ASSERT_IS_TRUE(result == 0);
    }
}

/* Tests_SRS_IOTHUB_DISTRIBUTED_38_004: [ If samplingRate is equal to 100, distributed properties should be added to all messages]*/
TEST_FUNCTION(IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary_distributedtracing_setting_with_percentage_100)
{
    //arrange
    IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA distributed_tracing_setting =
    {
        true,   /* policyEnabled */
        IOTHUB_DISTRIBUTED_TRACING_SAMPLING_MODE_ON,   /* samplingMode */
        100,    /* samplingRate */
        0       /* currentMessageNumber */
    };

    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(get_time(NULL));
    EXPECTED_CALL(STRING_construct("timestamp="));
    EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDistributedTracingSystemProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    int result = IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary(&distributed_tracing_setting, TEST_MESSAGE_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(result == 0);
    ASSERT_ARE_EQUAL(uint32_t, distributed_tracing_setting.currentMessageNumber, 1);
}

END_TEST_SUITE(iothubclient_diagnostic_ut)
