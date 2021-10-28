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
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/buffer_.h"
#undef ENABLE_MOCKS

#include "azure_prov_client/prov_transport_http_client.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/tlsio.h"

#include "azure_prov_client/internal/prov_transport_private.h"
#include "azure_prov_client/prov_transport.h"
#include "azure_uhttp_c/uhttp.h"

#include "parson.h"

#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, on_transport_register_data_cb, PROV_DEVICE_TRANSPORT_RESULT, transport_result, BUFFER_HANDLE, iothub_key, const char*, assigned_hub, const char*, device_id, void*, user_ctx);
MOCKABLE_FUNCTION(, void, on_transport_status_cb, PROV_DEVICE_TRANSPORT_STATUS, transport_status, uint32_t, retry_interval, void*, user_ctx);
MOCKABLE_FUNCTION(, char*, on_transport_challenge_callback, const unsigned char*, nonce, size_t, nonce_len, const char*, key_name, void*, user_ctx);
MOCKABLE_FUNCTION(, char*, on_transport_create_json_payload, const char*, ek_value, const char*, srk_value, void*, user_ctx);
MOCKABLE_FUNCTION(, PROV_JSON_INFO*, on_transport_json_parse, const char*, json_document, void*, user_ctx);
MOCKABLE_FUNCTION(, void, on_transport_error, PROV_DEVICE_TRANSPORT_ERROR, transport_error, void*, user_context);

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, JSON_Status, json_serialize_to_file, const JSON_Value*, value, const char *, filename);
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_file, const char*, string);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object*, object, const char *, name);
MOCKABLE_FUNCTION(, const char*, json_value_get_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);

#undef ENABLE_MOCKS

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#define TEST_PARAMETER_VALUE            (void*)0x11111112
#define TEST_HTTP_HANDLE_VALUE          (HTTP_HEADERS_HANDLE)0x11111113
#define TEST_JSON_ROOT_VALUE            (JSON_Value*)0x11111112
#define TEST_JSON_OBJECT_VALUE          (JSON_Object*)0x11111113
#define TEST_JSON_STATUS_VALUE          (JSON_Value*)0x11111114
#define TEST_BUFFER_VALUE               (BUFFER_HANDLE)0x11111115
#define TEST_INTERFACE_DESC             (const IO_INTERFACE_DESCRIPTION*)0x11111115
#define TEST_OPTION_VALUE               (void*)0x1111111B
#define TEST_JSON_CONTENT_LEN           19
#define TEST_SUCCESS_STATUS_CODE        204
#define TEST_FAILURE_STATUS_CODE        501
#define TEST_THROTTLE_STATUS_CODE       429
#define TEST_STRING_VALUE_LEN           17
#define HTTP_STATUS_CODE_UNAUTHORIZED   401

static unsigned char TEST_DATA[] = { 'k', 'e', 'y' };
static const size_t TEST_DATA_LEN = 3;
static char* TEST_STRING_VALUE = "Test_String_Value";

static const char* TEST_JSON_REPLY = "{ json_reply }";
static const char* TEST_PARAM_INFO_VALUE = "Test Param Info";
static const char* TEST_URI_VALUE = "dps_uri";
static const char* TEST_SCOPE_ID_VALUE = "scope_id";
static const char* TEST_OPERATION_ID_VALUE = "operation_id";
static const char* TEST_REGISTRATION_ID_VALUE = "registration_id";
static const char* TEST_DEVICE_ID_VALUE = "device_id";
static const char* TEST_DPS_API_VALUE = "dps_api";
static const char* TEST_JSON_DATA_VALUE = "{outgoing json data}";
static const char* TEST_SAS_TOKEN_VALUE = "sas_token";
static const char* TEST_X509_CERT_VALUE = "x509_cert";
static const char* TEST_CERT_VALUE = "certificate";
static const char* TEST_PRIVATE_KEY_VALUE = "private_key";
static const char* TEST_DPS_ASSIGNED_VALUE = "assigned";
static const char* TEST_DPS_UNASSIGNED_VALUE = "unassigned";

static const char* TEST_HOST_ADDRESS_VALUE = "host_address";
static const char* TEST_USERNAME_VALUE = "username";
static const char* TEST_PASSWORD_VALUE = "password";
static const char* TEST_JSON_CONTENT = "{test json content}";
static const char* TEST_XIO_OPTION_NAME = "test_option";
static const char* TEST_CUSTOM_DATA = "custom data";

static pfprov_transport_create prov_dev_http_transport_create;
static pfprov_transport_destroy prov_dev_http_transport_destroy;
static pfprov_transport_open prov_dev_http_transport_open;
static pfprov_transport_close prov_dev_http_transport_close;
static pfprov_transport_register_device prov_dev_http_transport_register_device;
static pfprov_transport_get_operation_status prov_dev_http_transport_get_op_status;
static pfprov_transport_dowork prov_dev_http_transport_dowork;
static pfprov_transport_set_trace prov_dev_http_transport_set_trace;
static pfprov_transport_set_x509_cert prov_dev_http_transport_x509_cert;
static pfprov_transport_set_trusted_cert prov_dev_http_transport_trusted_cert;
static pfprov_transport_set_proxy prov_dev_http_transport_set_proxy;
static pfprov_transport_set_option prov_dev_http_transport_set_option;


static ON_HTTP_OPEN_COMPLETE_CALLBACK g_on_http_open;
static ON_HTTP_REQUEST_CALLBACK g_on_http_reply_recv;
static ON_HTTP_ERROR_CALLBACK g_on_http_error;
static void* g_http_open_ctx;
static void* g_http_execute_ctx;
static void* g_http_error_ctx;

static PROV_DEVICE_TRANSPORT_STATUS g_target_transport_status;

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_TRANSPORT_ERROR, PROV_DEVICE_TRANSPORT_ERROR_VALUE);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_TRANSPORT_ERROR, PROV_DEVICE_TRANSPORT_ERROR_VALUE);

static BUFFER_HANDLE my_BUFFER_clone(BUFFER_HANDLE handle)
{
    (void)handle;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    my_gballoc_free(handle);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static char* my_on_transport_challenge_callback(const unsigned char* nonce, size_t nonce_len, const char* key_name, void* user_ctx)
{
    (void)nonce;
    (void)nonce_len;
    (void)key_name;
    (void)user_ctx;
    char* result;
    size_t src_len = strlen(TEST_SAS_TOKEN_VALUE);
    result = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(result, TEST_SAS_TOKEN_VALUE);
    return result;
}

static PROV_JSON_INFO* my_on_transport_json_parse(const char* json_document, void* user_ctx)
{
    (void)json_document;
    (void)user_ctx;
    PROV_JSON_INFO* result = (PROV_JSON_INFO*)my_gballoc_malloc(sizeof(PROV_JSON_INFO));
    if (g_target_transport_status == PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED)
    {
        result->prov_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED;
        result->authorization_key = (BUFFER_HANDLE)my_gballoc_malloc(1);
        result->iothub_uri = (char*)my_gballoc_malloc(strlen(TEST_URI_VALUE)+1);
        strcpy(result->iothub_uri, TEST_URI_VALUE);

        result->device_id = (char*)my_gballoc_malloc(strlen(TEST_DEVICE_ID_VALUE)+1);
        strcpy(result->device_id, TEST_DEVICE_ID_VALUE);
    }
    else if (g_target_transport_status == PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING)
    {
        result->prov_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        result->operation_id = (char*)my_gballoc_malloc(strlen(TEST_OPERATION_ID_VALUE)+1);
        strcpy(result->operation_id, TEST_OPERATION_ID_VALUE);
    }
    else
    {
        result->prov_status = PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED;
        result->authorization_key = (BUFFER_HANDLE)my_gballoc_malloc(1);
        result->key_name = (char*)my_gballoc_malloc(strlen(TEST_STRING_VALUE)+1);
        strcpy(result->key_name, TEST_STRING_VALUE);
    }
    return result;
}

static char* my_on_transport_create_json_payload(const char* ek_value, const char* srk_value, void* user_ctx)
{
    (void)ek_value;
    (void)srk_value;
    (void)user_ctx;
    char* result;
    size_t src_len = strlen(TEST_CUSTOM_DATA);
    result = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(result, TEST_CUSTOM_DATA);
    return result;
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE handle)
{
    my_gballoc_free(handle);
}

static STRING_HANDLE my_Base64_Encode(const BUFFER_HANDLE source)
{
    (void)source;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_Azure_Base64_Decode(const char* source)
{
    (void)source;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_URL_EncodeString(const char* input)
{
    (void)input;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static HTTP_CLIENT_HANDLE my_uhttp_client_create(const IO_INTERFACE_DESCRIPTION* io_interface_desc, const void* xio_param, ON_HTTP_ERROR_CALLBACK on_http_error, void* callback_ctx)
{
    (void)io_interface_desc;
    (void)xio_param;

    g_on_http_error = on_http_error;
    g_http_error_ctx = callback_ctx;

    return (HTTP_CLIENT_HANDLE)my_gballoc_malloc(1);
}

static HTTP_CLIENT_RESULT my_uhttp_client_open(HTTP_CLIENT_HANDLE handle, const char* host, int port_num, ON_HTTP_OPEN_COMPLETE_CALLBACK on_connect, void* callback_ctx)
{
    (void)handle;
    (void)host;
    (void)port_num;
    g_on_http_open = on_connect;
    g_http_open_ctx = callback_ctx;
    return HTTP_CLIENT_OK;
}

static HTTP_CLIENT_RESULT my_uhttp_client_execute_request(HTTP_CLIENT_HANDLE handle, HTTP_CLIENT_REQUEST_TYPE request_type, const char* relative_path,
    HTTP_HEADERS_HANDLE http_header_handle, const unsigned char* content, size_t content_length, ON_HTTP_REQUEST_CALLBACK on_request_callback, void* callback_ctx)
{
    (void)handle;
    (void)request_type;
    (void)relative_path;
    (void)http_header_handle;
    (void)content;
    (void)content_length;

    g_on_http_reply_recv = on_request_callback;
    g_http_execute_ctx = callback_ctx;
    return HTTP_CLIENT_OK;
}

static void my_uhttp_client_destroy(HTTP_CLIENT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static XIO_HANDLE my_xio_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, const void* xio_create_parameters)
{
    (void)io_interface_description;
    (void)xio_create_parameters;
    return (XIO_HANDLE)my_gballoc_malloc(1);
}

static void my_xio_destroy(XIO_HANDLE xio)
{
    my_gballoc_free(xio);
}

static void my_STRING_delete(STRING_HANDLE h)
{
    my_gballoc_free((void*)h);
}

static JSON_Value* my_json_parse_string(const char* string)
{
    (void)string;
    return (JSON_Value*)my_gballoc_malloc(1);
}

static void my_json_value_free(JSON_Value* value)
{
    my_gballoc_free(value);
}

static void my_on_transport_register_data_cb(PROV_DEVICE_TRANSPORT_RESULT transport_result, const char* data, void* user_ctx)
{
    (void)transport_result;
    (void)data;
    (void)user_ctx;
}

static void my_on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS transport_status, uint32_t retry_after, void* user_ctx)
{
    (void)transport_status;
    (void)user_ctx;
    (void)retry_after;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(prov_transport_http_client_ut)

    TEST_SUITE_INITIALIZE(suite_init)
    {
        int result;

        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);
        (void)umocktypes_bool_register_types();
        result = umocktypes_charptr_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);
        result = umocktypes_stdint_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);

        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT);
        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS);
        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_ERROR, PROV_DEVICE_TRANSPORT_ERROR);

        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_REQUEST_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_ERROR_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_CLOSED_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_OPEN_COMPLETE_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_REQUEST_TYPE, int);
        REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_CREATE_JSON_PAYLOAD, void*);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Encode, my_Base64_Encode);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Encode, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_length, TEST_STRING_VALUE_LEN);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

        REGISTER_GLOBAL_MOCK_HOOK(on_transport_challenge_callback, my_on_transport_challenge_callback);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_challenge_callback, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(on_transport_json_parse, my_on_transport_json_parse);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_json_parse, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(on_transport_create_json_payload, my_on_transport_create_json_payload);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_create_json_payload, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_clone, my_BUFFER_clone);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_clone, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Decode, my_Azure_Base64_Decode);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Decode, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

        REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_INTERFACE_DESC);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(platform_get_default_tlsio, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_create, my_uhttp_client_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_open, my_uhttp_client_open);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_open, HTTP_CLIENT_ERROR);
        REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_execute_request, my_uhttp_client_execute_request);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_execute_request, HTTP_CLIENT_ERROR);
        REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_destroy, my_uhttp_client_destroy);
        REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_trusted_cert, HTTP_CLIENT_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_trusted_cert, HTTP_CLIENT_ERROR);
        REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_X509_cert, HTTP_CLIENT_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_X509_cert, HTTP_CLIENT_ERROR);
        REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_option, HTTP_CLIENT_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_option, HTTP_CLIENT_ERROR);

        REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Alloc, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_ERROR);
        REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);

        prov_dev_http_transport_create = Prov_Device_HTTP_Protocol()->prov_transport_create;
        prov_dev_http_transport_destroy = Prov_Device_HTTP_Protocol()->prov_transport_destroy;
        prov_dev_http_transport_open = Prov_Device_HTTP_Protocol()->prov_transport_open;
        prov_dev_http_transport_close = Prov_Device_HTTP_Protocol()->prov_transport_close;
        prov_dev_http_transport_register_device = Prov_Device_HTTP_Protocol()->prov_transport_register;
        prov_dev_http_transport_get_op_status = Prov_Device_HTTP_Protocol()->prov_transport_get_op_status;
        prov_dev_http_transport_dowork = Prov_Device_HTTP_Protocol()->prov_transport_dowork;
        prov_dev_http_transport_set_trace = Prov_Device_HTTP_Protocol()->prov_transport_set_trace;
        prov_dev_http_transport_x509_cert = Prov_Device_HTTP_Protocol()->prov_transport_x509_cert;
        prov_dev_http_transport_trusted_cert = Prov_Device_HTTP_Protocol()->prov_transport_trusted_cert;
        prov_dev_http_transport_set_proxy = Prov_Device_HTTP_Protocol()->prov_transport_set_proxy;
        prov_dev_http_transport_set_option = Prov_Device_HTTP_Protocol()->prov_transport_set_option;
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
        g_on_http_open = NULL;
        g_on_http_reply_recv = NULL;
        g_on_http_error = NULL;
        g_http_open_ctx = NULL;
        g_http_execute_ctx = NULL;
        g_http_error_ctx = NULL;
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
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

    static void setup_free_allocated_data_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_construct_header_mocks(bool use_sas)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (use_sas)
        {
            STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
    }

    static void setup_dps_send_challenge_response_mocks(void)
    {
        setup_construct_header_mocks(true);
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_NUM_ARG));

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(Azure_Base64_Encode(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Azure_Base64_Encode(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));

        STRICT_EXPECTED_CALL(on_transport_create_json_payload(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(uhttp_client_execute_request(IGNORED_PTR_ARG, HTTP_CLIENT_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    }

    static void setup_prov_dev_http_transport_register_device_mocks(void)
    {
        setup_construct_header_mocks(false);
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_execute_request(IGNORED_PTR_ARG, HTTP_CLIENT_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    }

    static void setup_prov_transport_http_create_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_URI_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_DPS_API_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SCOPE_ID_VALUE));
    }

    static void setup_prov_dev_http_transport_open_mocks(bool use_tpm)
    {
        if (use_tpm)
        {
            STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_NUM_ARG));
            STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_NUM_ARG));
        }
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_REGISTRATION_ID_VALUE));
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(uhttp_client_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_set_trace(IGNORED_PTR_ARG, false, true));
        if (!use_tpm)
        {
            STRICT_EXPECTED_CALL(uhttp_client_set_X509_cert(IGNORED_PTR_ARG, true, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(uhttp_client_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 443, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_prov_transport_http_register_device_mocks(void)
    {
        setup_construct_header_mocks(false);
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_execute_request(IGNORED_PTR_ARG, HTTP_CLIENT_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    /*static void setup_prov_transport_process_status_reply_mocks(bool status_complete, const char* status_string)
    {
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(status_string);
        if (status_complete)
        {
            STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(TEST_URI_VALUE);
            STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }*/

    static void setup_prov_dev_http_transport_get_op_status_mocks(void)
    {
        setup_construct_header_mocks(true);
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_execute_request(IGNORED_PTR_ARG, HTTP_CLIENT_REQUEST_GET, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    TEST_FUNCTION(prov_transport_http_create_uri_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(NULL, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_create_scope_id_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, NULL, TEST_DPS_API_VALUE, on_transport_error, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_http_create_api_version_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, NULL, on_transport_error, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_http_create_succeed)
    {
        //arrange
        setup_prov_transport_http_create_mocks();

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);

        //assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_create_fail)
    {
        //arrange
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_prov_transport_http_create_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "prov_dev_http_transport_create failure in test %zu/%zu", index, count);

            PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);

            //assert
            ASSERT_IS_NULL(handle, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_dev_http_transport_destroy_handle_NULL_succees)
    {
        //arrange

        //act
        prov_dev_http_transport_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_dev_http_transport_destroy_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        setup_free_allocated_data_mocks();

        //act
        prov_dev_http_transport_destroy(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_open_handle_NULL_fail)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_open(NULL, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_open_register_data_NULL_fail)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, NULL, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_open_tpm_status_cb_NULL_fail)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, NULL, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_open_tpm_ek_NULL_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_open_tpm_srk_NULL_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_open_registration_id_NULL_fail)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_open(handle, NULL, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_open_tpm_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        setup_prov_dev_http_transport_open_mocks(true);

        //act
        int result = prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_open_x509_certs_NULL_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_REGISTRATION_ID_VALUE));
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(uhttp_client_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_set_trace(IGNORED_PTR_ARG, false, true));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        int result = prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_open_x509_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_x509_cert(handle, TEST_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        umock_c_reset_all_calls();

        setup_prov_dev_http_transport_open_mocks(false);

        //act
        int result = prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_open_fail)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_prov_dev_http_transport_open_mocks(true);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 5 };

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            {
                continue;
            }

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "prov_dev_http_transport_open failure in test %zu/%zu", index, count);

            int result = prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);
        }

        //cleanup
        prov_dev_http_transport_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_transport_close_handle_NULL_succeed)
    {
        //arrange

        //act
        int result = prov_dev_http_transport_close(NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_dev_http_transport_close_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_close(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_destroy(IGNORED_PTR_ARG));

        //act
        int result = prov_dev_http_transport_close(handle);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_register_device_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_dev_http_transport_register_device(NULL, on_transport_json_parse, on_transport_create_json_payload, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_http_register_device_cb_NULL_succeed)
    {
        int result;

        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        umock_c_reset_all_calls();

        //act
        result = prov_dev_http_transport_register_device(handle, on_transport_json_parse, NULL, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_register_device_succeed)
    {
        int result;

        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        umock_c_reset_all_calls();

        //act
        result = prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_reply_recv_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        prov_dev_http_transport_dowork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL);

        //act
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, TEST_SUCCESS_STATUS_CODE, TEST_HTTP_HANDLE_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        (void)prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_reply_recv_transient_error_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        prov_dev_http_transport_dowork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn("2");

        //act
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, TEST_FAILURE_STATUS_CODE, TEST_HTTP_HANDLE_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        (void)prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_reply_recv_transient_throttle_error_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        prov_dev_http_transport_dowork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn("5");

        //act
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, TEST_THROTTLE_STATUS_CODE, TEST_HTTP_HANDLE_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        (void)prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_get_operation_status_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_dev_http_transport_get_op_status(NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_http_get_operation_status_sas_token_NULL_fail)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_get_op_status(handle);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_get_operation_status_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        prov_dev_http_transport_dowork(handle);
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, HTTP_STATUS_CODE_UNAUTHORIZED, TEST_HTTP_HANDLE_VALUE);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED;
        prov_dev_http_transport_dowork(handle);

        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, TEST_SUCCESS_STATUS_CODE, TEST_HTTP_HANDLE_VALUE);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        prov_dev_http_transport_dowork(handle);
        umock_c_reset_all_calls();

        setup_prov_dev_http_transport_get_op_status_mocks();

        //act
        int result = prov_dev_http_transport_get_op_status(handle);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

     TEST_FUNCTION(prov_transport_http_get_operation_status_fail)
    {
        size_t count;

        //arrange
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        prov_dev_http_transport_dowork(handle);
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, HTTP_STATUS_CODE_UNAUTHORIZED, TEST_HTTP_HANDLE_VALUE);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED;
        prov_dev_http_transport_dowork(handle);
        umock_c_reset_all_calls();

        setup_prov_dev_http_transport_get_op_status_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 8, 9, 11, 12, 13, 14, 16, 17 };

        //act
        count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            int result;
            if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            {
                continue;
            }

            if (index == 1)
                break;

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "prov_dev_http_transport_get_op_status failure in test %zu/%zu", index, count);

            result = prov_dev_http_transport_get_op_status(handle);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);
        }

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_transport_http_dowork_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        prov_dev_http_transport_dowork(handle);
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, TEST_SUCCESS_STATUS_CODE, TEST_HTTP_HANDLE_VALUE);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        prov_dev_http_transport_dowork(handle);
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, TEST_SUCCESS_STATUS_CODE, TEST_HTTP_HANDLE_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_json_parse(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        //setup_prov_transport_process_status_reply_mocks(true, TEST_DPS_ASSIGNED_VALUE);
        STRICT_EXPECTED_CALL(on_transport_register_data_cb(PROV_DEVICE_TRANSPORT_RESULT_OK, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED;

        //act
        prov_dev_http_transport_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_dowork_unauthorized_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        // Send Registration here
        prov_dev_http_transport_dowork(handle);
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, HTTP_STATUS_CODE_UNAUTHORIZED, TEST_HTTP_HANDLE_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_json_parse(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_challenge_callback(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        // Send Challege response
        setup_dps_send_challenge_response_mocks();

        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED;

        //act
        prov_dev_http_transport_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_dowork_assigning_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        // Send Registration here
        prov_dev_http_transport_dowork(handle);
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_OK, (const unsigned char*)TEST_JSON_CONTENT, TEST_JSON_CONTENT_LEN, TEST_SUCCESS_STATUS_CODE, TEST_HTTP_HANDLE_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_json_parse(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;

        //act
        prov_dev_http_transport_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_dowork_error)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_dev_http_transport_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
        prov_dev_http_transport_dowork(handle);
        g_on_http_reply_recv(g_http_execute_ctx, HTTP_CALLBACK_REASON_ERROR, NULL, 0, TEST_FAILURE_STATUS_CODE, TEST_HTTP_HANDLE_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_register_data_cb(PROV_DEVICE_TRANSPORT_RESULT_ERROR, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        prov_dev_http_transport_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_trace_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_dev_http_transport_set_trace(NULL, true);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_http_set_trace_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        (void)prov_dev_http_transport_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(uhttp_client_set_trace(IGNORED_PTR_ARG, true, true));

        //act
        int result = prov_dev_http_transport_set_trace(handle, true);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_trace_http_not_open_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_set_trace(handle, true);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_dev_http_transport_x509_cert_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_dev_http_transport_x509_cert(NULL, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_dev_http_transport_x509_cert_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        int result = prov_dev_http_transport_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_trusted_cert_succeed)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        int result = prov_dev_http_transport_trusted_cert(handle, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_trusted_cert_fail)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);

        //act
        int result = prov_dev_http_transport_trusted_cert(handle, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_proxy_handle_NULL_fail)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        umock_c_reset_all_calls();

        //act
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        int result = prov_dev_http_transport_set_proxy(NULL, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_http_set_proxy_proxy_opt_NULL_fail)
    {
        //arrange
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        int result = prov_dev_http_transport_set_proxy(handle, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_proxy_no_hostname_fail)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        proxy_options.port = 443;
        int result = prov_dev_http_transport_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_proxy_no_user_succeed)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        int result = prov_dev_http_transport_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_proxy_user_succeed)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.username = TEST_USERNAME_VALUE;
        proxy_options.password = TEST_PASSWORD_VALUE;
        int result = prov_dev_http_transport_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_proxy_user_no_pwd_fail)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //act
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.username = TEST_USERNAME_VALUE;
        int result = prov_dev_http_transport_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_http_set_proxy_fail)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        umock_c_negative_tests_snapshot();

        //act
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.username = TEST_USERNAME_VALUE;
        proxy_options.password = TEST_PASSWORD_VALUE;

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "prov_dev_http_transport_set_proxy failure in test %zu/%zu", index, count);

            int result = prov_dev_http_transport_set_proxy(handle, &proxy_options);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);
        }

        //cleanup
        prov_dev_http_transport_close(handle);
        prov_dev_http_transport_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_dev_http_transport_set_option_option_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_dev_http_transport_set_option(handle, NULL, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_dev_http_transport_set_option_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);

        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(uhttp_client_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(uhttp_client_set_option(IGNORED_PTR_ARG, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE));

        //act
        int result = prov_dev_http_transport_set_option(handle, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_dev_http_transport_set_option_twice_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        int result = prov_dev_http_transport_set_option(handle, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(uhttp_client_set_option(IGNORED_PTR_ARG, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE));

        //act
        result = prov_dev_http_transport_set_option(handle, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    TEST_FUNCTION(prov_dev_http_transport_set_option_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_dev_http_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);

        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(uhttp_client_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL);

        //act
        int result = prov_dev_http_transport_set_option(handle, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_dev_http_transport_destroy(handle);
    }

    END_TEST_SUITE(prov_transport_http_client_ut)
