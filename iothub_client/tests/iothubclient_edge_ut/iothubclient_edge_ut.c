// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef USE_EDGE_MODULES
#error "tryping to compile iothubclient_edge_ut.c while the symbol USE_EDGE_MODULES is not defined"
#else

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#endif

#include "testrunnerswitcher.h"
#include "azure_macro_utils/macro_utils.h"

#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"


static void* real_malloc(size_t size)
{
    return malloc(size);
}

static void real_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/envvariable.h"
#include "internal/iothub_client_authorization.h"
#include "parson.h"

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
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, double, json_value_get_number, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_array);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Status, json_array_append_value, JSON_Array*, array, JSON_Value*, value);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Object*, json_array_get_object, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);

#undef ENABLE_MOCKS

#include "internal/iothub_client_edge.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...);
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
    {
        (void)handle;
        (void)format;
        return 0;
    }

    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        return (STRING_HANDLE)real_malloc(1);
    }
#ifdef __cplusplus
}
#endif

static const IOTHUB_AUTHORIZATION_HANDLE TEST_AUTHORIZATION_HANDLE = (IOTHUB_AUTHORIZATION_HANDLE)0x0001;
static const IOTHUB_CLIENT_EDGE_HANDLE TEST_MODULE_CLIENT_METHOD_HANDLE = (IOTHUB_CLIENT_EDGE_HANDLE)0x0002;
static JSON_Object* DUMMY_JSON_OBJECT = (JSON_Object*)0x0003;
static JSON_Value* DUMMY_JSON_VALUE = (JSON_Value*)0x0004;

static const char* TEST_DEVICE_ID = "deviceId";
static const char* TEST_DEVICE_ID2 = "otherDeviceId";
static const char* TEST_MODULE_ID = "moduleId";
static const char* TEST_MODULE_ID2 = "otherModuleId";
static const char* TEST_GATEWAY_HOST_NAME = "GatewayHostName";
static const char* TEST_METHOD_NAME = "methodName";
static const char* TEST_METHOD_PAYLOAD = "{payload:payload}";
static unsigned int TEST_TIMEOUT = 47;

static const char* DUMMY_STRING = "string";
static unsigned char* DUMMY_USTRING = (unsigned char*)"unsigned-string";
static double DUMMY_NUMBER = 47;
static unsigned int DUMMY_UINT = 47;


static TEST_MUTEX_HANDLE g_testByTest;
MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)real_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static BUFFER_HANDLE my_BUFFER_create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (BUFFER_HANDLE)real_malloc(1);
}

static BUFFER_HANDLE my_BUFFER_new(void)
{
    return (BUFFER_HANDLE)real_malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE buffer)
{
    real_free(buffer);
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)real_malloc(1);
}

static void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE handle)
{
    real_free(handle);
}

static HTTPAPIEX_HANDLE my_HTTPAPIEX_Create(const char* hostName)
{
    (void)hostName;
    return (HTTPAPIEX_HANDLE)real_malloc(1);
}

static void my_HTTPAPIEX_Destroy(HTTPAPIEX_HANDLE handle)
{
    real_free(handle);
}

static HTTPAPIEX_RESULT my_HTTPAPIEX_ExecuteRequest(HTTPAPIEX_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
    HTTP_HEADERS_HANDLE requestHttpHeadersHandle, BUFFER_HANDLE requestContent, unsigned int* statusCode,
    HTTP_HEADERS_HANDLE responseHttpHeadersHandle, BUFFER_HANDLE responseContent)
{
    (void)handle;
    (void)requestType;
    (void)relativePath;
    (void)requestHttpHeadersHandle;
    (void)requestContent;
    *statusCode = 200;
    (void)responseHttpHeadersHandle;
    (void)responseContent;

    return HTTPAPIEX_OK;
}

static STRING_HANDLE my_STRING_from_byte_array(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (STRING_HANDLE)real_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    real_free(handle);
}

static JSON_Value* my_json_parse_string(const char* string)
{
    (void)string;
    return (JSON_Value*)real_malloc(1);
}

static void my_json_value_free(JSON_Value* value)
{
    real_free(value);
}

static char* my_json_serialize_to_string(const JSON_Value* value)
{
    (void)value;
    char* newstr;
    my_mallocAndStrcpy_s(&newstr, "test");
    return newstr;
}

static char* my_IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, size_t expiry_time_relative_seconds, const char* key_name)
{
    (void)handle;
    (void)expiry_time_relative_seconds;
    (void)key_name;
    (void)scope;
    return (char*)real_malloc(1);
}

static char* my_IoTHubClient_Auth_Get_TrustBundle(IOTHUB_AUTHORIZATION_HANDLE handle, const char* certificate_file_name)
{
    (void)handle;
    (void)certificate_file_name;
    return (char*)real_malloc(1);
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
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

static IOTHUB_CLIENT_EDGE_HANDLE create_module_client_method_handle()
{
    IOTHUB_CLIENT_CONFIG config;
    config.deviceId = TEST_DEVICE_ID;
    config.protocolGatewayHostName = TEST_GATEWAY_HOST_NAME;

    IOTHUB_CLIENT_EDGE_HANDLE handle = IoTHubClient_EdgeHandle_Create(&config, TEST_AUTHORIZATION_HANDLE, TEST_MODULE_ID);

    return handle;
}

static void createMethodPayloadExpectedCalls()
{
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));   //cannot fail
}

static void sendHttpRequestMethodExpectedCalls()
{
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));    //cannot fail

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));   //cannot fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));   //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));    //cannot fail

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_TrustBundle(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));    //cannot fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));       //cannot fail
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG));   //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));        //cannot fail
}

static void parseResponseJsonExpectedCalls()
{
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).CallCannotFail();       //cannot fail (returns length 0)
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_number(IGNORED_PTR_ARG)).CallCannotFail();   //cannot fail (returns -1, which isn't strictly failure)
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));           //cannot fail
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));         //cannot fail
}

BEGIN_TEST_SUITE(iothubclient_edge_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_EDGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JSON_Value*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JSON_Object*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPI_REQUEST_TYPE, int);


    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, real_free);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, DUMMY_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_u_char, DUMMY_USTRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_u_char, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_length, DUMMY_UINT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_length, 0);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Alloc, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Create, my_HTTPAPIEX_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Destroy, my_HTTPAPIEX_Destroy);
    REGISTER_GLOBAL_MOCK_RETURN(HTTPAPIEX_SetOption, HTTPAPIEX_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_SetOption, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_ExecuteRequest, my_HTTPAPIEX_ExecuteRequest);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(UniqueId_Generate, UNIQUEID_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UniqueId_Generate, UNIQUEID_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_from_byte_array, my_STRING_from_byte_array);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_from_byte_array, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, my_json_parse_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, DUMMY_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_value, DUMMY_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_value, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_serialize_to_string, my_json_serialize_to_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_number, DUMMY_NUMBER);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_number, -1);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, my_json_value_free);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 1);

    REGISTER_GLOBAL_MOCK_RETURN(environment_get_variable, "test_env_variable");
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(environment_get_variable, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_SasToken, my_IoTHubClient_Auth_Get_SasToken);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_SasToken, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_TrustBundle, my_IoTHubClient_Auth_Get_TrustBundle);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_TrustBundle, NULL);
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

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION(IoTHubClient_EdgeHandle_Create_FAIL_null_config)
{
    //arrange

    //act
    IOTHUB_CLIENT_EDGE_HANDLE handle = IoTHubClient_EdgeHandle_Create(NULL, TEST_AUTHORIZATION_HANDLE, TEST_MODULE_ID);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_EdgeHandle_Create_FAIL_null_auth)
{
    //arrange
    IOTHUB_CLIENT_CONFIG config;
    config.deviceId = TEST_DEVICE_ID;
    config.protocolGatewayHostName = TEST_GATEWAY_HOST_NAME;

    //act
    IOTHUB_CLIENT_EDGE_HANDLE handle = IoTHubClient_EdgeHandle_Create(&config, NULL, TEST_MODULE_ID);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_EdgeHandle_Create_FAIL_null_module)
{
    //arrange
    IOTHUB_CLIENT_CONFIG config;
    config.deviceId = TEST_DEVICE_ID;
    config.protocolGatewayHostName = TEST_GATEWAY_HOST_NAME;

    //act
    IOTHUB_CLIENT_EDGE_HANDLE handle = IoTHubClient_EdgeHandle_Create(&config, TEST_AUTHORIZATION_HANDLE, NULL);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_EdgeHandle_Create_SUCCESS)
{
    //arrange
    IOTHUB_CLIENT_CONFIG config;
    config.deviceId = TEST_DEVICE_ID;
    config.protocolGatewayHostName = TEST_GATEWAY_HOST_NAME;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_EDGE_HANDLE handle = IoTHubClient_EdgeHandle_Create(&config, TEST_AUTHORIZATION_HANDLE, TEST_MODULE_ID);

    //assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_EdgeHandle_Create_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CONFIG config;
    config.deviceId = TEST_DEVICE_ID;
    config.protocolGatewayHostName = TEST_GATEWAY_HOST_NAME;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
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
        sprintf(tmp_msg, "IoTHubClient_EdgeHandle_Create_FAIL failure in test %lu/%lu", (unsigned long)test_num, (unsigned long)test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        IOTHUB_CLIENT_EDGE_HANDLE handle = IoTHubClient_EdgeHandle_Create(&config, TEST_AUTHORIZATION_HANDLE, TEST_MODULE_ID);

        //assert
        ASSERT_IS_NULL(handle, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_EdgeHandle_Destroy_NULL_ARG)
{
    //arrange

    //act
    IoTHubClient_EdgeHandle_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_EdgeHandle_Destroy_SUCCESS)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClient_EdgeHandle_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_NULL_ARG_moduleMethodHandle)
{
    //arrange
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(NULL, TEST_DEVICE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_NULL_ARG_deviceId)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(handle, NULL, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_NULL_ARG_methodName)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(handle, TEST_DEVICE_ID2, NULL, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_NULL_ARG_methodPayload)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(handle, TEST_DEVICE_ID2, TEST_METHOD_NAME, NULL, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_NULL_ARG_responseStatus)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(handle, TEST_DEVICE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, NULL, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_NULL_ARG_responsePayload)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(handle, TEST_DEVICE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, NULL, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_NULL_ARG_responsePayloadSize)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(handle, TEST_DEVICE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_SUCCESS)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    createMethodPayloadExpectedCalls();
    STRICT_EXPECTED_CALL(BUFFER_new());
    sendHttpRequestMethodExpectedCalls();
    parseResponseJsonExpectedCalls();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));   //cannot fail
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));   //cannot fail

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(handle, TEST_DEVICE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    free(responsePayload);
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_DeviceMethodInvoke_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    createMethodPayloadExpectedCalls();
    STRICT_EXPECTED_CALL(BUFFER_new());
    sendHttpRequestMethodExpectedCalls();
    parseResponseJsonExpectedCalls();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            //act
            IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_DeviceMethodInvoke(handle, TEST_DEVICE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

            //assert
            ASSERT_IS_TRUE(result == IOTHUB_CLIENT_ERROR, "IoTHubClient_EdgeHandle_Create_FAIL failure in test %lu", (unsigned long)index);
        }
    }

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_NULL_ARG_moduleMethodHandle)
{
    //arrange
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(NULL, TEST_DEVICE_ID2, TEST_MODULE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_NULL_ARG_deviceId)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, NULL, TEST_MODULE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_NULL_ARG_moduleId)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, TEST_DEVICE_ID2, NULL, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_NULL_ARG_methodName)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, TEST_DEVICE_ID2, TEST_MODULE_ID2, NULL, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_NULL_ARG_methodPayload)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, TEST_DEVICE_ID2, TEST_MODULE_ID2, TEST_METHOD_NAME, NULL, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_NULL_ARG_responseStatus)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, TEST_DEVICE_ID2, TEST_MODULE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, NULL, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_NULL_ARG_responsePayload)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, TEST_DEVICE_ID2, TEST_MODULE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, NULL, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_NULL_ARG_responsePayloadSize)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, TEST_DEVICE_ID2, TEST_MODULE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_SUCCESS)
{
    //arrange
    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    createMethodPayloadExpectedCalls();
    STRICT_EXPECTED_CALL(BUFFER_new());
    sendHttpRequestMethodExpectedCalls();
    parseResponseJsonExpectedCalls();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));   //cannot fail
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));   //cannot fail

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, TEST_DEVICE_ID2, TEST_MODULE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    free(responsePayload);
    IoTHubClient_EdgeHandle_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Edge_ModuleMethodInvoke_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_EDGE_HANDLE handle = create_module_client_method_handle();
    int responseStatus;
    unsigned char* responsePayload;
    size_t responsePayloadSize;

    umock_c_reset_all_calls();

    createMethodPayloadExpectedCalls();
    STRICT_EXPECTED_CALL(BUFFER_new());
    sendHttpRequestMethodExpectedCalls();
    parseResponseJsonExpectedCalls();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));   //cannot fail
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));   //cannot fail

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            //act
            IOTHUB_CLIENT_RESULT result = IoTHubClient_Edge_ModuleMethodInvoke(handle, TEST_DEVICE_ID2, TEST_MODULE_ID2, TEST_METHOD_NAME, TEST_METHOD_PAYLOAD, TEST_TIMEOUT, &responseStatus, &responsePayload, &responsePayloadSize);

            //assert
            ASSERT_IS_TRUE(result == IOTHUB_CLIENT_ERROR, "IoTHubClient_Edge_ModuleMethodInvoke_FAIL failure in test %lu", (unsigned long)index);
        }
    }

    //cleanup
    IoTHubClient_EdgeHandle_Destroy(handle);
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(iothubclient_edge_ut)
#endif /* USE_EDGE_MODULES */
