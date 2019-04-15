// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#else
#include <stdlib.h>
#include <stdint.h>
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
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#if defined _MSC_VER
#pragma warning(disable: 4054) /* MSC incorrectly fires this */
#endif

static int bool_Compare(bool left, bool right)
{
    return left != right;
}

static void bool_ToString(char* string, size_t bufferSize, bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}

#ifndef __cplusplus
static int _Bool_Compare(_Bool left, _Bool right)
{
    return left != right;
}

static void _Bool_ToString(char* string, size_t bufferSize, _Bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}
#endif


#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/socketio.h"
#include "azure_uhttp_c/uhttp.h"
#include "azure_c_shared_utility/envvariable.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/urlencode.h"

#include "parson.h"

#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112
static const char* TEST_CONST_CHAR_PTR = "TestConstChar";
static JSON_Status TEST_JSON_STATUS = 0;
static char* TEST_CHAR_PTR = "TestString";

static const char * const TEST_ENV_MODULE_GENERATION_ID = "Test-ModuleGenerationId";
static const char * const TEST_ENV_EDGEMODULEID = "Test-ModuleId";
static const char * const TEST_ENV_WORKLOADURI_HTTP = "http://127.0.0.1:8080";
static const char * const TEST_ENV_WORKLOADURI_HTTP_BAD_PROTOCOL = "badprotocol://127.0.0.1:8080";
static const char * const TEST_ENV_WORKLOADURI_HTTP_NO_PORT = "http://127.0.0.1";
static const char * const TEST_ENV_WORKLOADURI_HTTP_NO_ADDRESS = "http://:8080";

static const char * const TEST_ENV_WORKLOADURI_DOMAIN_SOCKET = "unix:///test-socket";
static const char * const TEST_ENV_WORKLOADURI_DOMAIN_SOCKET_NO_NAME = "unix://";



static const unsigned char* TEST_SIGNING_DATA = (const unsigned char* )"Test/Data/To/Sign\nExpiry";
static const int TEST_SIGNING_DATA_LENGTH = sizeof(TEST_SIGNING_DATA) - 1;

typedef enum TEST_PROTOCOL_TAG
{
   TEST_HTTP_PROTOCOL,
   TEST_DOMAIN_SOCKET_PROTOCOL
} TEST_PROTOCOL;



MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, const char*, json_object_dotget_string, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Status, json_object_clear, JSON_Object*, object);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);


#undef ENABLE_MOCKS

#include "hsm_client_http_edge.h"

#define TEST_STRING_1 "TestData1"
#define TEST_BUFFER_1 (unsigned char*)"TestData2"
#define TEST_STRING_HANDLE1 (STRING_HANDLE)0x46
#define TEST_STRING_HANDLE2 (STRING_HANDLE)0x47
#define TEST_STRING_HANDLE3 (STRING_HANDLE)0x52
#define TEST_BUFFER_HANDLE (BUFFER_HANDLE)0x48

#define TEST_TIME ((double)3600)
#define TEST_TIME_T ((time_t)TEST_TIME)
#define TEST_TIME_FOR_TIMEOUT ((double)100000000)
#define TEST_TIME_FOR_TIMEOUT_T ((time_t)TEST_TIME_FOR_TIMEOUT)




#define TEST_HTTP_CLIENT_HANDLE (HTTP_CLIENT_HANDLE)0x49
#define TEST_HTTP_HEADERS_HANDLE (HTTP_HEADERS_HANDLE)0x50
#define TEST_SOCKETIO_INTERFACE_DESCRIPTION     (const IO_INTERFACE_DESCRIPTION*)0x51


static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
{
    (void)format;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static int test_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static void my_STRING_delete(STRING_HANDLE h)
{
    if ((h == TEST_STRING_HANDLE1) || (h == TEST_STRING_HANDLE2) || (h == TEST_STRING_HANDLE3))
    {
        return;
    }

    my_gballoc_free((void*)h);
}


HSM_HTTP_WORKLOAD_CONTEXT* workload_context;
static int g_uhttp_client_dowork_call_count;
static ON_HTTP_OPEN_COMPLETE_CALLBACK g_on_http_open;
static void* g_http_open_ctx;
static ON_HTTP_REQUEST_CALLBACK g_on_http_reply_recv;
static void* g_http_reply_recv_ctx;


static HTTP_CLIENT_HANDLE my_uhttp_client_create(const IO_INTERFACE_DESCRIPTION* io_interface_desc, const void* xio_param, ON_HTTP_ERROR_CALLBACK on_http_error, void* callback_ctx)
{
    (void)io_interface_desc;
    (void)xio_param;
    (void)on_http_error;
    workload_context = (HSM_HTTP_WORKLOAD_CONTEXT*)callback_ctx;

    ASSERT_ARE_EQUAL(bool, workload_context->continue_running, true, "Signing context not in running mode");
    ASSERT_IS_NULL(workload_context->http_response, "HTTP response not NULL during initialization");

    return (HTTP_CLIENT_HANDLE)my_gballoc_malloc(1);
}

static void my_uhttp_client_destroy(HTTP_CLIENT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static HTTP_CLIENT_RESULT my_uhttp_client_open(HTTP_CLIENT_HANDLE handle, const char* host, int port_num, ON_HTTP_OPEN_COMPLETE_CALLBACK on_connect, void* callback_ctx)
{
    (void)handle;
    (void)host;
    (void)port_num;
    g_on_http_open = on_connect;
    g_http_open_ctx = callback_ctx; //prov_client

    //note that a real malloc does occur in this fn, but it can't be mocked since it's in a field of handle

    return HTTP_CLIENT_OK;
}

static void my_uhttp_client_close(HTTP_CLIENT_HANDLE handle, ON_HTTP_CLOSED_CALLBACK on_close_callback, void* callback_ctx)
{
    (void)handle;
    (void)on_close_callback;
    (void)callback_ctx;

    //note that a real free does occur in this fn, but it can't be mocked since it's in a field of handle
}

static HTTP_CLIENT_RESULT my_uhttp_client_execute_request(HTTP_CLIENT_HANDLE handle, HTTP_CLIENT_REQUEST_TYPE request_type, const char* relative_path,
    HTTP_HEADERS_HANDLE http_header_handle, const unsigned char* content, size_t content_len, ON_HTTP_REQUEST_CALLBACK on_request_callback, void* callback_ctx)
{
    (void)handle;
    (void)request_type;
    (void)relative_path;
    (void)http_header_handle;
    (void)content;
    (void)content_len;
    g_on_http_reply_recv = on_request_callback;
    g_http_reply_recv_ctx = callback_ctx;

    return HTTP_CLIENT_OK;
}

static const unsigned char* TEST_REPLY_JSON = (const unsigned char*)"TestReplyJson";
static unsigned int test_status_code_in_callback;
HTTP_CALLBACK_REASON http_open_reason;
HTTP_CALLBACK_REASON http_reply_recv_reason;
static bool content_available;
static bool timed_out;

static void my_uhttp_client_dowork(HTTP_CLIENT_HANDLE handle)
{
    (void)handle;

    const unsigned char* content = content_available ? TEST_REPLY_JSON : NULL;

    if (g_uhttp_client_dowork_call_count == 0)
    {
        g_on_http_open(g_http_open_ctx, http_open_reason);
    }
    else if (g_uhttp_client_dowork_call_count == 1)
    {
        g_on_http_reply_recv(g_http_reply_recv_ctx, http_reply_recv_reason, content, 1, test_status_code_in_callback, TEST_HTTP_HEADERS_HANDLE);
    }
    else
    {
        ASSERT_FAIL("Invoked mock of do_work too many times");
    }
    g_uhttp_client_dowork_call_count++;
}

BEGIN_TEST_SUITE(hsm_client_http_edge_ut)
TEST_SUITE_INITIALIZE(suite_init)
{
    (void)umock_c_init(on_umock_c_error);

    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_ERROR_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_OPEN_COMPLETE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_REQUEST_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_REQUEST_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_CLOSED_CALLBACK, void*);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_close, my_uhttp_client_close);


    REGISTER_GLOBAL_MOCK_RETURN(environment_get_variable, "");
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(environment_get_variable, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, test_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 1);

    REGISTER_GLOBAL_MOCK_RETURN(json_parse_string, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_dotget_string, TEST_CONST_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_value_init_object, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_object, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_string, TEST_JSON_STATUS);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_string, -1);

    REGISTER_GLOBAL_MOCK_RETURN(json_serialize_to_string, TEST_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(json_object_clear, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_clear, JSONFailure);

    REGISTER_GLOBAL_MOCK_RETURN(URL_EncodeString, TEST_STRING_HANDLE1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(URL_Encode, TEST_STRING_HANDLE1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_Encode, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(Azure_Base64_Encode_Bytes, TEST_STRING_HANDLE2);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Encode_Bytes, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct_n, TEST_STRING_HANDLE3);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct_n, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_create, TEST_BUFFER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_create, my_uhttp_client_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_destroy, my_uhttp_client_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_open, my_uhttp_client_open);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_open, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_close, my_uhttp_client_close);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_Alloc, TEST_HTTP_HEADERS_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Alloc, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_execute_request, my_uhttp_client_execute_request);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_execute_request, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_option, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_option, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_dowork, my_uhttp_client_dowork);

    REGISTER_GLOBAL_MOCK_RETURN(socketio_get_interface_description, TEST_SOCKETIO_INTERFACE_DESCRIPTION);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, TEST_TIME_T);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, 1);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_u_char, TEST_BUFFER_1);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    g_http_open_ctx = NULL;
    g_on_http_reply_recv = NULL;
    g_http_reply_recv_ctx = NULL;
    g_uhttp_client_dowork_call_count = 0;

    test_status_code_in_callback = 200;
    http_reply_recv_reason = HTTP_CALLBACK_REASON_OK;
    http_open_reason =  HTTP_CALLBACK_REASON_OK;
    content_available = true;
    timed_out = false;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    ;
}

static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
{
    int result = 0;
    for (size_t index = 0; index < length; index++)
    {
        if (current_index == skip_array[index])
        {
            result = __LINE__;
            break;
        }
    }
    return result;
}


static void setup_hsm_client_http_edge_create_http_mock(const char* edge_uri_env, bool valid_edge_uri)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_MODULE_GENERATION_ID);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_EDGEMODULEID);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(edge_uri_env);

    if (valid_edge_uri == true)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
}

static void hsm_client_http_edge_create_http_Impl(const char* edge_uri_env, bool valid_edge_uri)
{
    //arrange
    setup_hsm_client_http_edge_create_http_mock(edge_uri_env, valid_edge_uri);

    //act
    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();

    //assert
    if (valid_edge_uri == true)
    {
        ASSERT_IS_NOT_NULL(sec_handle);
    }
    else
    {
        ASSERT_IS_NULL(sec_handle);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    hsm_client_http_edge_destroy(sec_handle);
}

static void hsm_client_http_edge_create_domain_socket_Impl(const char* edge_uri_env, bool valid_edge_uri)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_MODULE_GENERATION_ID);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_EDGEMODULEID);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(edge_uri_env);

    if (valid_edge_uri == true)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

TEST_FUNCTION(hsm_client_http_edge_create_http_succeed)
{
    hsm_client_http_edge_create_http_Impl(TEST_ENV_WORKLOADURI_HTTP, true);
}

TEST_FUNCTION(hsm_client_http_edge_create_http_fail_bad_protocol)
{
    hsm_client_http_edge_create_http_Impl(TEST_ENV_WORKLOADURI_HTTP_BAD_PROTOCOL, false);
}

TEST_FUNCTION(hsm_client_http_edge_create_http_fail_no_port_in_string)
{
    hsm_client_http_edge_create_http_Impl(TEST_ENV_WORKLOADURI_HTTP_NO_PORT, false);
}

TEST_FUNCTION(hsm_client_http_edge_create_http_fail_no_address_in_string)
{
    hsm_client_http_edge_create_http_Impl(TEST_ENV_WORKLOADURI_HTTP_NO_ADDRESS, false);
}

TEST_FUNCTION(hsm_client_http_edge_create_domain_socket_succeed)
{
    hsm_client_http_edge_create_domain_socket_Impl(TEST_ENV_WORKLOADURI_DOMAIN_SOCKET, true);
}

TEST_FUNCTION(hsm_client_http_edge_create_domain_socket_no_name_fail)
{
    hsm_client_http_edge_create_domain_socket_Impl(TEST_ENV_WORKLOADURI_DOMAIN_SOCKET_NO_NAME, true);
}


TEST_FUNCTION(hsm_client_http_edge_destroy_success)
{
    // setup
    setup_hsm_client_http_edge_create_http_mock(TEST_ENV_WORKLOADURI_HTTP, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();

    ASSERT_IS_NOT_NULL(sec_handle);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));


    // test
    hsm_client_http_edge_destroy(sec_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}



TEST_FUNCTION(hsm_client_http_edge_destroy_null_ptr)
{
    hsm_client_http_edge_destroy(NULL);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(hsm_client_http_edge_interface_succeed)
{
    //act
    const HSM_CLIENT_HTTP_EDGE_INTERFACE* http_edge_iface = hsm_client_http_edge_interface();

    //assert
    ASSERT_IS_NOT_NULL(http_edge_iface);
    ASSERT_IS_NOT_NULL(http_edge_iface->hsm_client_http_edge_create);
    ASSERT_IS_NOT_NULL(http_edge_iface->hsm_client_http_edge_destroy);
    ASSERT_IS_NOT_NULL(http_edge_iface->hsm_client_sign_with_identity);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


static void set_expected_calls_construct_json_signing_blob()
{
    STRICT_EXPECTED_CALL(STRING_construct_n(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_serialize_to_string(TEST_JSON_VALUE));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(TEST_CHAR_PTR));
    STRICT_EXPECTED_CALL(json_object_clear(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_NUM_ARG));
}

static void set_expected_calls_send_and_poll_http_signing_request(bool post_data)
{
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_NUM_ARG)).SetReturn(post_data ? (unsigned char*)TEST_STRING_1 : NULL);
    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).SetReturn(TEST_TIME_T);
    STRICT_EXPECTED_CALL(uhttp_client_execute_request(IGNORED_PTR_ARG, post_data ? HTTP_CLIENT_REQUEST_POST : HTTP_CLIENT_REQUEST_GET, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).SetReturn(timed_out ? TEST_TIME_FOR_TIMEOUT_T : TEST_TIME_T);
    STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).SetReturn(TEST_TIME_T);
}

static void set_expected_calls_send_http_workload_request(bool expect_success, bool post_data, TEST_PROTOCOL testProtocol)
{
    STRICT_EXPECTED_CALL(socketio_get_interface_description());
    STRICT_EXPECTED_CALL(uhttp_client_create(IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    if (testProtocol == TEST_DOMAIN_SOCKET_PROTOCOL)
    {
        STRICT_EXPECTED_CALL(uhttp_client_set_option(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(uhttp_client_open(IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    if (post_data)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    }
    set_expected_calls_send_and_poll_http_signing_request(post_data);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_close(IGNORED_NUM_ARG, NULL, NULL));
    STRICT_EXPECTED_CALL(uhttp_client_destroy(IGNORED_NUM_ARG));

    if (expect_success == false)
    {
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_NUM_ARG));
    }
}

static void set_expected_calls_parse_json_workload_response()
{
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_dotget_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_clear(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG));
}

static void set_expected_calls_hsm_client_http_edge_sign_data(TEST_PROTOCOL testProtocol)
{
    set_expected_calls_construct_json_signing_blob();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_1);
    set_expected_calls_send_http_workload_request(true, true, testProtocol);
    set_expected_calls_parse_json_workload_response();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_NUM_ARG));
}

TEST_FUNCTION(hsm_client_http_edge_sign_data_succeed)
{
    setup_hsm_client_http_edge_create_http_mock(TEST_ENV_WORKLOADURI_HTTP, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();
    unsigned char* signed_value = NULL;
    size_t signed_len;

    ASSERT_IS_NOT_NULL(sec_handle);

    umock_c_reset_all_calls();

    set_expected_calls_hsm_client_http_edge_sign_data(TEST_HTTP_PROTOCOL);

    int result = hsm_client_http_edge_sign_data(sec_handle, TEST_SIGNING_DATA, TEST_SIGNING_DATA_LENGTH, &signed_value, &signed_len);
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());


    hsm_client_http_edge_destroy(sec_handle);
    free(signed_value);
}

TEST_FUNCTION(hsm_client_http_edge_sign_data_domain_socket_succeed)
{
    hsm_client_http_edge_create_domain_socket_Impl(TEST_ENV_WORKLOADURI_DOMAIN_SOCKET, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();
    unsigned char* signed_value = NULL;
    size_t signed_len;

    ASSERT_IS_NOT_NULL(sec_handle);

    umock_c_reset_all_calls();

    set_expected_calls_hsm_client_http_edge_sign_data(TEST_DOMAIN_SOCKET_PROTOCOL);

    int result = hsm_client_http_edge_sign_data(sec_handle, TEST_SIGNING_DATA, TEST_SIGNING_DATA_LENGTH, &signed_value, &signed_len);
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());


    hsm_client_http_edge_destroy(sec_handle);
    free(signed_value);
}


static void test_http_failure_impl()
{
    setup_hsm_client_http_edge_create_http_mock(TEST_ENV_WORKLOADURI_HTTP, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();
    unsigned char* signed_value = NULL;
    size_t signed_len;

    ASSERT_IS_NOT_NULL(sec_handle);

    set_expected_calls_hsm_client_http_edge_sign_data(TEST_HTTP_PROTOCOL);

    int result = hsm_client_http_edge_sign_data(sec_handle, TEST_SIGNING_DATA, TEST_SIGNING_DATA_LENGTH, &signed_value, &signed_len);
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    hsm_client_http_edge_destroy(sec_handle);
}

TEST_FUNCTION(hsm_client_http_edge_sign_data_http_open_error)
{
    http_open_reason =  HTTP_CALLBACK_REASON_OPEN_FAILED;
    test_http_failure_impl();
}

TEST_FUNCTION(hsm_client_http_edge_sign_data_http_receive_error)
{
    http_reply_recv_reason = HTTP_CALLBACK_REASON_ERROR;
    test_http_failure_impl();
}

TEST_FUNCTION(hsm_client_http_edge_sign_data_http_receive_null_content)
{
    content_available = false;
    test_http_failure_impl();
}

TEST_FUNCTION(hsm_client_http_edge_sign_data_http_receive_bad_status_code)
{
    test_status_code_in_callback = 400;
    test_http_failure_impl();
}

TEST_FUNCTION(hsm_client_http_edge_sign_data_http_timeout)
{
    timed_out = true;
    test_http_failure_impl();
}

TEST_FUNCTION(hsm_client_http_edge_sign_data_http_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_hsm_client_http_edge_create_http_mock(TEST_ENV_WORKLOADURI_HTTP, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();
    unsigned char* signed_value = NULL;
    size_t signed_len;

    ASSERT_IS_NOT_NULL(sec_handle);

    umock_c_reset_all_calls();

    set_expected_calls_hsm_client_http_edge_sign_data(TEST_HTTP_PROTOCOL);

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = {
        3, // STRING_c_str
        4, // STRING_c_str
        10, // STRING_c_str
        14, // json_free_serialized_string
        15, // json_object_clear
        16 , // json_value_free
        17, // STRING_delete
        18, // STRING_delete
        19, // STRING_delete
        20, // STRING_c_str
        21, // socketio_get_interface_description
        26, // BUFFER_u_char
        27, // get_time
        29, // uhttp_client_dowork
        30, // get_time
        31, // uhttp_client_dowork
        33, // get_time
        34, // HTTPHeaders_Free
        35, // uhttp_client_close
        36, // uhttp_client_destroy
        42, // json_object_clear
        43, // json_value_free
        44, // BUFFER_delete
        45, // BUFFER_delete
        46 // STRING_delete
    };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }
        g_uhttp_client_dowork_call_count = 0;

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "hsm_client_http_edge_sign_data failure in test %zu/%zu", index, count);
        int result = hsm_client_http_edge_sign_data(sec_handle, TEST_SIGNING_DATA, TEST_SIGNING_DATA_LENGTH, &signed_value, &signed_len);
        ASSERT_ARE_NOT_EQUAL(int, result, 0, tmp_msg);
    }

    // cleanup

    hsm_client_http_edge_destroy(sec_handle);
    umock_c_negative_tests_deinit();
}


static void set_expected_calls_hsm_client_http_edge_get_trust_bundle(TEST_PROTOCOL testProtocol)
{
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_1);
    set_expected_calls_send_http_workload_request(true, false, testProtocol);
    set_expected_calls_parse_json_workload_response();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_NUM_ARG));
}

TEST_FUNCTION(hsm_client_http_edge_get_trust_bundle_success)
{
    setup_hsm_client_http_edge_create_http_mock(TEST_ENV_WORKLOADURI_HTTP, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();

    ASSERT_IS_NOT_NULL(sec_handle);

    umock_c_reset_all_calls();

    set_expected_calls_hsm_client_http_edge_get_trust_bundle(TEST_HTTP_PROTOCOL);
    const char* trusted_certificate = hsm_client_http_edge_get_trust_bundle(sec_handle);
    ASSERT_IS_NOT_NULL(trusted_certificate, "hsm_client_http_edge_get_trust_bundle fails");
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    hsm_client_http_edge_destroy(sec_handle);
    free((char*)trusted_certificate);
}

TEST_FUNCTION(hsm_client_http_edge_get_trust_bundle_NULL_sec_handle_fail)
{
    setup_hsm_client_http_edge_create_http_mock(TEST_ENV_WORKLOADURI_HTTP, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();

    ASSERT_IS_NOT_NULL(sec_handle);

    umock_c_reset_all_calls();

    const char* trusted_certificate = hsm_client_http_edge_get_trust_bundle(NULL);
    ASSERT_IS_NULL(trusted_certificate);

    hsm_client_http_edge_destroy(sec_handle);
    free((char*)trusted_certificate);
}

TEST_FUNCTION(hsm_client_http_edge_get_trust_bundle_domain_socket_success)
{
    hsm_client_http_edge_create_domain_socket_Impl(TEST_ENV_WORKLOADURI_DOMAIN_SOCKET, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();

    ASSERT_IS_NOT_NULL(sec_handle);

    umock_c_reset_all_calls();

    set_expected_calls_hsm_client_http_edge_get_trust_bundle(TEST_DOMAIN_SOCKET_PROTOCOL);
    const char* trusted_certificate = hsm_client_http_edge_get_trust_bundle(sec_handle);
    ASSERT_IS_NOT_NULL(trusted_certificate, "hsm_client_http_edge_get_trust_bundle fails");
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    hsm_client_http_edge_destroy(sec_handle);
    free((char*)trusted_certificate);
}


TEST_FUNCTION(hsm_client_http_edge_get_trust_bundle_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_hsm_client_http_edge_create_http_mock(TEST_ENV_WORKLOADURI_HTTP, true);

    HSM_CLIENT_HANDLE sec_handle = hsm_client_http_edge_create();

    ASSERT_IS_NOT_NULL(sec_handle);

    umock_c_reset_all_calls();

    set_expected_calls_hsm_client_http_edge_get_trust_bundle(TEST_HTTP_PROTOCOL);

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = {
        0, // STRING_c_str
        1, // socketio_get_interface_description
        4, // BUFFER_u_char
        5, // get_time
        7, // uhttp_client_dowork
        8, // get_time
        9, // uhttp_client_dowork
        11, // get_time
        12, // HTTPHeaders_Free
        14, // uhttp_client_destroy
        13, // uhttp_client_close
        20, // json_object_clear
        21, // json_value_free
        22, // BUFFER_delete
        23 // BUFFER_delete
    };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }
        g_uhttp_client_dowork_call_count = 0;

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "hsm_client_http_edge_get_trust_bundle failure in test %zu/%zu", index, count);
        const char* trusted_certificate = hsm_client_http_edge_get_trust_bundle(sec_handle);
        ASSERT_IS_NULL(trusted_certificate, tmp_msg);
    }

    // cleanup

    hsm_client_http_edge_destroy(sec_handle);
    umock_c_negative_tests_deinit();

}


END_TEST_SUITE(hsm_client_http_edge_ut)

