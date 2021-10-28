// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
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
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/tickcounter.h"

#include "azure_prov_client/internal/iothub_auth_client.h"
#include "azure_prov_client/internal/prov_auth_client.h"
#include "azure_prov_client/internal/prov_transport_private.h"
#include "parson.h"

#undef ENABLE_MOCKS

#include "azure_prov_client/prov_device_ll_client.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, on_prov_register_device_callback, PROV_DEVICE_RESULT, register_result, const char*, iothub_uri, const char*, device_id, void*, user_context);
MOCKABLE_FUNCTION(, void, on_prov_register_status_callback, PROV_DEVICE_REG_STATUS, reg_status, void*, user_context);
MOCKABLE_FUNCTION(, char*, on_prov_transport_challenge_cb, const unsigned char*, nonce, size_t, nonce_len, const char*, key_name, void*, user_ctx);

MOCKABLE_FUNCTION(, PROV_DEVICE_TRANSPORT_HANDLE, prov_transport_create, const char*, uri, TRANSPORT_HSM_TYPE, type, const char*, scope_id, const char*, prov_api_version, PROV_TRANSPORT_ERROR_CALLBACK, error_cb, void*, error_ctx);
MOCKABLE_FUNCTION(, void, prov_transport_destroy, PROV_DEVICE_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, prov_transport_open, PROV_DEVICE_TRANSPORT_HANDLE, handle, const char*, registration_id, BUFFER_HANDLE, ek, BUFFER_HANDLE, srk, PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK, data_callback, void*, user_ctx, PROV_DEVICE_TRANSPORT_STATUS_CALLBACK, status_cb, void*, status_ctx, PROV_TRANSPORT_CHALLENGE_CALLBACK, reg_challenge_cb, void*, challenge_ctx);
MOCKABLE_FUNCTION(, int, prov_transport_close, PROV_DEVICE_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, prov_transport_register_device, PROV_DEVICE_TRANSPORT_HANDLE, handle, PROV_TRANSPORT_JSON_PARSE, json_parse_cb, PROV_TRANSPORT_CREATE_JSON_PAYLOAD, json_create_cb, void*, json_ctx);
MOCKABLE_FUNCTION(, int, prov_transport_get_operation_status, PROV_DEVICE_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, void, prov_transport_dowork, PROV_DEVICE_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, prov_transport_set_trace, PROV_DEVICE_TRANSPORT_HANDLE, handle, bool, trace_on);
MOCKABLE_FUNCTION(, int, prov_transport_x509_cert, PROV_DEVICE_TRANSPORT_HANDLE, handle, const char*, certificate, const char*, private_key);
MOCKABLE_FUNCTION(, int, prov_transport_set_trusted_cert, PROV_DEVICE_TRANSPORT_HANDLE, handle, const char*, certificate);
MOCKABLE_FUNCTION(, int, prov_transport_set_proxy, PROV_DEVICE_TRANSPORT_HANDLE, handle, const HTTP_PROXY_OPTIONS*, proxy_option);

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, JSON_Status, json_serialize_to_file, const JSON_Value*, value, const char *, filename);
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_file, const char*, string);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object*, object, const char *, name);
MOCKABLE_FUNCTION(, const char*, json_value_get_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);
MOCKABLE_FUNCTION(, void, on_transport_error, PROV_DEVICE_TRANSPORT_ERROR, transport_error, void*, user_context);
MOCKABLE_FUNCTION(, double, json_value_get_number, const JSON_Value*, value);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_value, JSON_Object*, object, const char*, name, JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);

#undef ENABLE_MOCKS

static TEST_MUTEX_HANDLE g_testByTest;
static PROV_TRANSPORT_CHALLENGE_CALLBACK g_challenge_callback;
static void* g_challenge_ctx;
static PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK g_registration_callback;
static void* g_registration_ctx;
PROV_DEVICE_TRANSPORT_STATUS_CALLBACK g_status_callback;
static void* g_status_ctx;
static void* g_http_error_ctx;
PROV_TRANSPORT_JSON_PARSE g_json_parse_cb;
PROV_TRANSPORT_CREATE_JSON_PAYLOAD g_json_create_cb;
void* g_json_ctx;

#ifdef __cplusplus
extern "C"
{
#endif

    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...);
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

#ifdef __cplusplus
}
#endif

static PROV_DEVICE_TRANSPORT_PROVIDER g_prov_transport_func =
{
    prov_transport_create,
    prov_transport_destroy,
    prov_transport_open,
    prov_transport_close,
    prov_transport_register_device,
    prov_transport_get_operation_status,
    prov_transport_dowork,
    prov_transport_set_trace,
    prov_transport_x509_cert,
    prov_transport_set_trusted_cert,
    prov_transport_set_proxy
};

static const PROV_DEVICE_TRANSPORT_PROVIDER* trans_provider(void)
{
    return &g_prov_transport_func;
}

static const BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x11111116;

static const char* TEST_JSON_REPLY = "{ json_reply }";
static const char* TEST_PROV_URI = "www.prov_uri.com";
static const char* TEST_SCOPE_ID = "scope_id";
static const char* TEST_STRING_HANDLE_VALUE = "string_value";
static size_t TEST_STRING_LENGTH = sizeof("string_value");
static const char* TEST_DEVICE_ID = "device_id";
static const char* TEST_IOTHUB = "iothub.value.test";
static const char* TEST_KEY = "AD87D58E-4E52-411B-9C63-49BD02C84F0D";
static const char* TEST_REGISTRATION_ID = "A87FA22F-828B-46CA-BA37-D574C32E423E";
static const char* TEST_PROV_ASSIGNED_STATUS = "assigned";
static const char* TEST_HTTP_PROXY_VAL = "127.0.0.1";
static const char* TEST_CERTIFICATE_VAL = "--BEGIN_CERT 12345 END_CERT--";
static const char* TEST_HTTP_USERNAME = "username";
static const char* TEST_HTTP_PASSWORD = "password";
static const char* TEST_TRUSTED_CERT = "trusted_cert";
static char* TEST_STRING_VALUE = "Test_String_Value";
static const char* TEST_CUSTOM_DATA = "{ \"json_cust_data\": 123456 }";

static const char* PROV_DISABLED_STATUS = "disabled";
static const char* PROV_FAILURE_STATUS = "failure";

static int TEST_STATUS_CODE_OK = 200;
static int TEST_STATUS_CODE_2 = 299;
static int TEST_UNAUTHORIZED_CODE = 401;
static int TEST_ERROR_STATUS_CODE = 500;
#define TEST_JSON_ROOT_VALUE (JSON_Value*)0x11111112
#define TEST_JSON_OBJECT_VALUE (JSON_Object*)0x11111113
#define TEST_JSON_STATUS_VALUE (JSON_Value*)0x11111114
#define TEST_BUFFER_HANDLE_VALUE (BUFFER_HANDLE)0x11111115

#define TEST_DPS_HUB_ERROR_NO_HUB       400208
#define TEST_DPS_HUB_ERROR_UNAUTH       400209
#define DEFAULT_RETRY_AFTER             2

static unsigned char TEST_ENDORSMENT_KEY[] = { 'k', 'e', 'y' };

static unsigned char TEST_DATA[] = { 'k', 'e', 'y' };
static const size_t TEST_DATA_LEN = 3;
static const char* TEST_DECRYPTED_NONCE = "decrypted_nonce";
static const size_t TEST_DECRYPTED_NONCE_LEN = sizeof("decrypted_nonce");

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);

TEST_DEFINE_ENUM_TYPE(PROV_AUTH_TYPE, PROV_AUTH_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_AUTH_TYPE, PROV_AUTH_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(PROV_AUTH_RESULT, PROV_AUTH_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_AUTH_RESULT, PROV_AUTH_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE_VALUES);

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
        return (STRING_HANDLE)my_gballoc_malloc(1);
    }
#ifdef __cplusplus
}
#endif

static void free_prov_json_info(PROV_JSON_INFO* parse_info)
{
    if (parse_info != NULL)
    {
        switch (parse_info->prov_status)
        {
            case PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED:
                BUFFER_delete(parse_info->authorization_key);
                free(parse_info->key_name);
                break;
            case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED:
                BUFFER_delete(parse_info->authorization_key);
                free(parse_info->iothub_uri);
                free(parse_info->device_id);
                break;
            case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING:
                free(parse_info->operation_id);
                break;
            default:
                break;
        }
        my_gballoc_free(parse_info);
    }
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

static STRING_HANDLE my_URL_EncodeString(const char* textEncode)
{
    (void)textEncode;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static TICK_COUNTER_HANDLE my_tickcounter_create(void)
{
    return (TICK_COUNTER_HANDLE)my_gballoc_malloc(1);
}

static void my_tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)
{
    my_gballoc_free(tick_counter);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    my_gballoc_free(handle);
}

char* my_json_serialize_to_string(const JSON_Value* value)
{
    (void)value;

    size_t len = strlen(TEST_CUSTOM_DATA);
    char* result = (char*)my_gballoc_malloc(len + 1);
    strcpy(result, TEST_CUSTOM_DATA);
    return result;
}

void my_json_free_serialized_string(char* string)
{
    my_gballoc_free(string);
}

static PROV_DEVICE_TRANSPORT_HANDLE my_prov_transport_create(const char* uri, TRANSPORT_HSM_TYPE type, const char* scope_id, const char* prov_api_version, PROV_TRANSPORT_ERROR_CALLBACK error_cb, void* error_ctx)
{
    (void)type;
    (void)uri;
    (void)scope_id;
    (void)prov_api_version;
    (void)error_cb;
    (void)error_ctx;

    return (PROV_DEVICE_TRANSPORT_HANDLE)my_gballoc_malloc(1);
}

static void my_prov_transport_destroy(PROV_DEVICE_TRANSPORT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static int my_prov_transport_open(PROV_DEVICE_TRANSPORT_HANDLE handle, const char* registration_id, BUFFER_HANDLE ek, BUFFER_HANDLE srk, PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK data_callback, void* user_ctx, PROV_DEVICE_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx, PROV_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* challenge_ctx)
{
    (void)handle;
    (void)ek;
    (void)srk;
    (void)registration_id;
    g_registration_callback = data_callback;
    g_registration_ctx = user_ctx;
    g_status_callback = status_cb;
    g_status_ctx = status_ctx;
    g_challenge_callback = reg_challenge_cb;
    g_challenge_ctx = challenge_ctx;
    return 0;
}

static int my_prov_transport_register_device(PROV_DEVICE_TRANSPORT_HANDLE handle, PROV_TRANSPORT_JSON_PARSE json_parse_cb, PROV_TRANSPORT_CREATE_JSON_PAYLOAD json_create_cb, void* json_ctx)
{
    (void)handle;
    g_json_parse_cb = json_parse_cb;
    g_json_ctx = json_ctx;
    g_json_create_cb = json_create_cb;
    return 0;
}

static PROV_AUTH_HANDLE my_prov_auth_create(void)
{
    return (PROV_AUTH_HANDLE)my_gballoc_malloc(1);
}

static void my_prov_auth_destroy(PROV_AUTH_HANDLE handle)
{
    my_gballoc_free(handle);
}

static char* my_prov_auth_get_registration_id(PROV_AUTH_HANDLE handle)
{
    (void)handle;
    char* result;
    size_t data_len = strlen(TEST_REGISTRATION_ID);
    result = (char*)my_gballoc_malloc(data_len+1);
    strcpy(result, TEST_REGISTRATION_ID);
    return result;
}

static BUFFER_HANDLE my_prov_auth_get_endorsement_key(PROV_AUTH_HANDLE handle)
{
    (void)handle;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_prov_auth_get_storage_key(PROV_AUTH_HANDLE handle)
{
    (void)handle;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static char* my_prov_auth_get_certificate(PROV_AUTH_HANDLE handle)
{
    (void)handle;
    char* result;
    size_t data_len = strlen(TEST_CERTIFICATE_VAL);
    result = (char*)my_gballoc_malloc(data_len + 1);
    strcpy(result, TEST_CERTIFICATE_VAL);
    return result;
}

static char* my_prov_auth_get_alias_key(PROV_AUTH_HANDLE handle)
{
    (void)handle;
    char* result;
    size_t data_len = strlen(TEST_CERTIFICATE_VAL);
    result = (char*)my_gballoc_malloc(data_len + 1);
    strcpy(result, TEST_CERTIFICATE_VAL);
    return result;
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_SASToken_Create(STRING_HANDLE key, STRING_HANDLE scope, STRING_HANDLE keyName, uint64_t expiry)
{
    (void)key;
    (void)scope;
    (void)keyName;
    (void)expiry;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    my_gballoc_free(handle);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len+1);
    strcpy(*destination, source);
    return 0;
}

static STRING_HANDLE my_Azure_Base64_Encode_Bytes(const unsigned char* source, size_t size)
{
    (void)source;(void)size;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_Azure_Base64_Decode(const char* source)
{
    (void)source;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
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

BEGIN_TEST_SUITE(prov_device_client_ll_ut)

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

        REGISTER_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT);
        REGISTER_TYPE(PROV_AUTH_TYPE, PROV_AUTH_TYPE);
        REGISTER_TYPE(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS);
        REGISTER_TYPE(PROV_AUTH_RESULT, PROV_AUTH_RESULT);
        REGISTER_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE);

        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_STATUS_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_CHALLENGE_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_AUTH_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_JSON_PARSE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_CREATE_JSON_PAYLOAD, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_ERROR_CALLBACK, void*);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(tickcounter_create, my_tickcounter_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(tickcounter_destroy, my_tickcounter_destroy);

        REGISTER_GLOBAL_MOCK_HOOK(prov_transport_create, my_prov_transport_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_transport_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(prov_transport_destroy, my_prov_transport_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(prov_transport_open, my_prov_transport_open);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_transport_open, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(prov_transport_register_device, my_prov_transport_register_device);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_transport_register_device, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_x509_cert, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_transport_x509_cert, __LINE__);

        REGISTER_GLOBAL_MOCK_RETURN(prov_auth_get_type, PROV_AUTH_TYPE_TPM);

        REGISTER_GLOBAL_MOCK_HOOK(prov_auth_create, my_prov_auth_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_auth_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(prov_auth_destroy, my_prov_auth_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(prov_auth_get_registration_id, my_prov_auth_get_registration_id);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_auth_get_registration_id, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(prov_auth_get_certificate, my_prov_auth_get_certificate);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_auth_get_certificate, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(prov_auth_get_alias_key, my_prov_auth_get_alias_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_auth_get_alias_key, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(prov_auth_construct_sas_token, "Sas_token");
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_auth_construct_sas_token, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, my_json_parse_string);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(json_value_free, my_json_value_free);
        REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, TEST_JSON_OBJECT_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(json_object_get_value, TEST_JSON_STATUS_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_value, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(json_value_get_string, TEST_STRING_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_string, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(json_object_get_object, TEST_JSON_OBJECT_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(prov_auth_get_endorsement_key, my_prov_auth_get_endorsement_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_auth_get_endorsement_key, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(prov_auth_get_storage_key, my_prov_auth_get_storage_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_auth_get_storage_key, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(prov_auth_import_key, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_auth_import_key, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_HANDLE_VALUE);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_length, TEST_STRING_LENGTH);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_create, TEST_BUFFER_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

        REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(SASToken_Create, my_SASToken_Create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(SASToken_Create, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Encode_Bytes, my_Azure_Base64_Encode_Bytes);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Encode_Bytes, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Decode, my_Azure_Base64_Decode);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Decode, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(json_serialize_to_string, my_json_serialize_to_string);
        REGISTER_GLOBAL_MOCK_HOOK(json_free_serialized_string, my_json_free_serialized_string);

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
        g_registration_callback = NULL;
        g_registration_ctx = NULL;
        g_status_callback = NULL;
        g_status_ctx = NULL;
        g_challenge_callback = NULL;
        g_challenge_ctx = NULL;
        g_json_parse_cb = NULL;
        g_json_ctx = NULL;
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    static void setup_retrieve_json_item_mocks(const char* return_item)
    {
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(return_item);
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_Prov_Device_LL_on_registration_data_mocks(void)
    {
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(prov_auth_import_key(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(on_prov_register_device_callback(PROV_DEVICE_RESULT_OK, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_Prov_Device_LL_Create_mocks(PROV_AUTH_TYPE type)
    {
        STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_auth_create());
        STRICT_EXPECTED_CALL(tickcounter_create());
        STRICT_EXPECTED_CALL(prov_auth_get_type(IGNORED_PTR_ARG)).SetReturn(type).CallCannotFail();
        STRICT_EXPECTED_CALL(prov_transport_create(IGNORED_PTR_ARG, type == PROV_AUTH_TYPE_TPM ? TRANSPORT_HSM_TYPE_TPM : TRANSPORT_HSM_TYPE_X509, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
    }

    static size_t setup_Prov_Device_LL_Register_Device_mocks(bool tpm)
    {
        size_t transport_error = 1;
        STRICT_EXPECTED_CALL(prov_auth_get_registration_id(IGNORED_PTR_ARG));
        if (tpm)
        {
            STRICT_EXPECTED_CALL(prov_auth_get_endorsement_key(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(prov_auth_get_storage_key(IGNORED_PTR_ARG));
            transport_error += 2;
        }
        else
        {
            STRICT_EXPECTED_CALL(prov_auth_get_certificate(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(prov_auth_get_alias_key(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(prov_transport_x509_cert(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
            transport_error += 5;
        }
        STRICT_EXPECTED_CALL(prov_transport_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        return transport_error;
    }

    static void setup_cleanup_prov_info_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_destroy_prov_info_mocks(void)
    {
        setup_cleanup_prov_info_mocks();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(prov_transport_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(prov_auth_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_Prov_Device_LL_DoWork_register_send_mocks()
    {
        STRICT_EXPECTED_CALL(prov_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(prov_transport_register_device(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_challenge_callback_mocks(void)
    {
        STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_auth_import_key(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(prov_auth_construct_sas_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_parse_json_unassigned_mocks(void)
    {
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(NULL);

        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Azure_Base64_Decode(IGNORED_PTR_ARG));
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }

    static void setup_parse_json_assigning_mocks(void)
    {
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(PROV_ASSIGNING_STATUS);

        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }

    static void setup_parse_json_disabled_mocks(void)
    {
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(PROV_DISABLED_STATUS);
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }

    static void setup_parse_json_error_mocks(double return_err_num)
    {
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(PROV_FAILURE_STATUS);

        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(json_value_get_number(IGNORED_PTR_ARG)).SetReturn(return_err_num);
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }

    static void setup_parse_json_assigned_mocks(bool use_tpm, bool data_return)
    {
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(PROV_ASSIGNED_STATUS);

        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        if (use_tpm)
        {
            STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(PROV_ASSIGNED_STATUS);
            STRICT_EXPECTED_CALL(Azure_Base64_Decode(IGNORED_PTR_ARG));
        }
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);

        if (data_return)
        {
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
        }
        else
        {
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL).CallCannotFail();
        }

        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }

    TEST_FUNCTION(Prov_Device_LL_Create_uri_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_LL_HANDLE result = Prov_Device_LL_Create(NULL, TEST_SCOPE_ID, trans_provider);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_Create_succees)
    {
        //arrange
        setup_Prov_Device_LL_Create_mocks(PROV_AUTH_TYPE_TPM);

        //act
        PROV_DEVICE_LL_HANDLE result = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(result);
    }

    TEST_FUNCTION(Prov_Device_LL_Create_fail)
    {
        //arrange
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_Prov_Device_LL_Create_mocks(PROV_AUTH_TYPE_TPM);

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                PROV_DEVICE_LL_HANDLE result = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);

                // assert
                ASSERT_IS_NULL(result, "Prov_Device_LL_Create failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(Prov_Device_LL_Destroy_handle_NULL)
    {
        //arrange

        //act
        Prov_Device_LL_Destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_Destroy_succeed)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        setup_destroy_prov_info_mocks();

        //act
        Prov_Device_LL_Destroy(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_Register_Device_handle_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(NULL, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_Register_Device_register_callback_NULL_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, NULL, NULL, on_prov_register_status_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Register_Device_tpm_succeed)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        setup_Prov_Device_LL_Register_Device_mocks(true);
        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Register_Device_x509_succeed)
    {
        //arrange
        setup_Prov_Device_LL_Create_mocks(PROV_AUTH_TYPE_X509);
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        setup_Prov_Device_LL_Register_Device_mocks(false);

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Register_Device_tpm_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        size_t transport_error = setup_Prov_Device_LL_Register_Device_mocks(true);

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                //act
                PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);

                //assert
                ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, index < transport_error ? PROV_DEVICE_RESULT_ERROR : PROV_DEVICE_RESULT_TRANSPORT, prov_result, "Prov_Device_LL_Register_Device failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        Prov_Device_LL_Destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(Prov_Device_LL_Register_Device_riot_fail)
    {
        //arrange
        setup_Prov_Device_LL_Create_mocks(PROV_AUTH_TYPE_X509);
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        size_t transport_error = setup_Prov_Device_LL_Register_Device_mocks(false);

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                //act
                PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);

                //assert
                ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, index < transport_error ? PROV_DEVICE_RESULT_ERROR : PROV_DEVICE_RESULT_TRANSPORT, prov_result, "Prov_Device_LL_Register_Device failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        Prov_Device_LL_Destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(Prov_Device_LL_DoWork_handle_NULL_fail)
    {
        //arrange

        //act
        Prov_Device_LL_DoWork(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_DoWork_register_send_succeed)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        umock_c_reset_all_calls();

        setup_Prov_Device_LL_DoWork_register_send_mocks();

        //act
        Prov_Device_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_DoWork_register_send_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_Prov_Device_LL_DoWork_register_send_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);

                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                //act
                Prov_Device_LL_DoWork(handle);
                // To Get the device back in registration mode
                Prov_Device_LL_DoWork(handle);

                //assert
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_unassigned_fail)
    {
        PROV_JSON_INFO* result;

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_unassigned_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

                //assert
                ASSERT_IS_NULL(result, "g_json_parse_cb failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_assigning_succeed)
    {
        PROV_JSON_INFO* result;
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigning_mocks();

        //act
        result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_IS_NOT_NULL(result->operation_id);
        ASSERT_IS_NULL(result->key_name);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(result);
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_error_succeed)
    {
        PROV_JSON_INFO* result;
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_error_mocks(0);

        //act
        result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_no_hub_succeed)
    {
        PROV_JSON_INFO* result;
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_error_mocks(TEST_DPS_HUB_ERROR_NO_HUB);

        //act
        result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_assigning_fail)
    {
        PROV_JSON_INFO* result;
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_Prov_Device_LL_Create_mocks(PROV_AUTH_TYPE_X509);
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigning_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

                //assert
                ASSERT_IS_NULL(result, "g_json_parse_cb failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_assigned_succeed)
    {
        PROV_JSON_INFO* result;
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true, false);

        //act
        result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_IS_NOT_NULL(result->authorization_key);
        ASSERT_IS_NOT_NULL(result->iothub_uri);
        ASSERT_IS_NOT_NULL(result->device_id);
        ASSERT_IS_NULL(result->operation_id);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(result);
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_assigned_fail)
    {
        PROV_JSON_INFO* result;
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_Prov_Device_LL_Create_mocks(PROV_AUTH_TYPE_X509);
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(false, false);

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

                //assert
                ASSERT_IS_NULL(result, "g_json_parse_cb failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_DoWork_no_connection_succeed)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(prov_transport_dowork(IGNORED_PTR_ARG));

        //act
        Prov_Device_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_DoWork_get_operation_status_send_succeed)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_AUTHENTICATED, DEFAULT_RETRY_AFTER, g_status_ctx);
        umock_c_reset_all_calls();

        size_t t1_ms = (size_t)(DEFAULT_RETRY_AFTER * 1.5 * 1000);  // Some random time after the first registration message.
        size_t t2_ms = t1_ms + 300;                                 // Some random time after the first get-status message is sent.

        STRICT_EXPECTED_CALL(prov_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_current_ms(&t1_ms, sizeof(t1_ms));
        STRICT_EXPECTED_CALL(prov_transport_get_operation_status(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_current_ms(&t2_ms, sizeof(t2_ms));

        //act
        Prov_Device_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_DoWork_get_operation_status_send_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_AUTHENTICATED, DEFAULT_RETRY_AFTER, g_status_ctx);
        umock_c_reset_all_calls();

        size_t t1_ms = (size_t)(DEFAULT_RETRY_AFTER * 1.5 * 1000);  // Some random time after the first registration message.

        STRICT_EXPECTED_CALL(prov_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_current_ms(&t1_ms, sizeof(t1_ms));
        STRICT_EXPECTED_CALL(prov_transport_get_operation_status(IGNORED_PTR_ARG)).SetReturn(__LINE__);
        STRICT_EXPECTED_CALL(on_prov_register_device_callback(PROV_DEVICE_RESULT_TRANSPORT, NULL, NULL, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(prov_transport_close(IGNORED_PTR_ARG));
        setup_cleanup_prov_info_mocks();

        //act
        Prov_Device_LL_DoWork(handle);
        Prov_Device_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_challenge_cb_nonce_NULL_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        char* result = g_challenge_callback(NULL, TEST_DATA_LEN, TEST_STRING_HANDLE_VALUE, g_challenge_ctx);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_challenge_cb_user_ctx_NULL_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        char* result = g_challenge_callback(TEST_DATA, TEST_DATA_LEN, TEST_STRING_HANDLE_VALUE, NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_challenge_cb_succeed)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_challenge_callback_mocks();

        //act
        char* result = g_challenge_callback(TEST_DATA, TEST_DATA_LEN, TEST_STRING_HANDLE_VALUE, g_challenge_ctx);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_challenge_cb_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_challenge_callback_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                char* result = g_challenge_callback(TEST_DATA, TEST_DATA_LEN, TEST_STRING_HANDLE_VALUE, g_challenge_ctx);

                //assert
                ASSERT_IS_NULL(result, "g_challenge_callback failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_on_registration_data_succeed)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_Prov_Device_LL_on_registration_data_mocks();
        STRICT_EXPECTED_CALL(prov_transport_close(IGNORED_PTR_ARG));
        setup_cleanup_prov_info_mocks();

        //act
        g_registration_callback(PROV_DEVICE_TRANSPORT_RESULT_OK, TEST_BUFFER_HANDLE_VALUE, TEST_IOTHUB, TEST_DEVICE_ID, g_registration_ctx);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_on_registration_data_iothub_key_NULL_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        g_registration_callback(PROV_DEVICE_TRANSPORT_RESULT_OK, NULL, TEST_IOTHUB, TEST_DEVICE_ID, g_registration_ctx);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_on_registration_data_prov_auth_import_key_0_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        g_registration_callback(PROV_DEVICE_TRANSPORT_RESULT_OK, NULL, TEST_IOTHUB, TEST_DEVICE_ID, g_registration_ctx);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_on_registration_data_user_ctx_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        g_registration_callback(PROV_DEVICE_TRANSPORT_RESULT_OK, TEST_BUFFER_HANDLE_VALUE, TEST_IOTHUB, TEST_DEVICE_ID, NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_on_registration_data_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_Prov_Device_LL_on_registration_data_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                //act
                g_registration_callback(PROV_DEVICE_TRANSPORT_RESULT_OK, TEST_BUFFER_HANDLE_VALUE, TEST_IOTHUB, TEST_DEVICE_ID, g_registration_ctx);

                //assert
            }
        }

        //cleanup
        Prov_Device_LL_Destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(Prov_Device_LL_on_registration_data_error_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(on_prov_register_device_callback(PROV_DEVICE_RESULT_TRANSPORT, NULL, NULL, NULL));
        STRICT_EXPECTED_CALL(prov_transport_close(IGNORED_PTR_ARG));
        setup_cleanup_prov_info_mocks();
        STRICT_EXPECTED_CALL(prov_transport_dowork(IGNORED_PTR_ARG));

        //act
        g_registration_callback(PROV_DEVICE_TRANSPORT_RESULT_ERROR, NULL, NULL, NULL, g_registration_ctx);
        Prov_Device_LL_DoWork(handle);
        Prov_Device_LL_DoWork(handle);  // Second DoWork should not call the callback again.

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_on_registration_unauthorized_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(on_prov_register_device_callback(PROV_DEVICE_RESULT_DEV_AUTH_ERROR, NULL, NULL, NULL));
        STRICT_EXPECTED_CALL(prov_transport_close(IGNORED_PTR_ARG));
        setup_cleanup_prov_info_mocks();

        //act
        g_registration_callback(PROV_DEVICE_TRANSPORT_RESULT_UNAUTHORIZED, NULL, NULL, NULL, g_registration_ctx);
        Prov_Device_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_handle_NULL_fail)
    {
        //arrange

        //act
        HTTP_PROXY_OPTIONS http_proxy;
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
        http_proxy.host_address = TEST_HTTP_PROXY_VAL;
        http_proxy.port = 80;
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(NULL, OPTION_HTTP_PROXY, &http_proxy);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_option_name_NULL_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        //act
        HTTP_PROXY_OPTIONS http_proxy;
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
        http_proxy.host_address = TEST_HTTP_PROXY_VAL;
        http_proxy.port = 80;
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(NULL, NULL, &http_proxy);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_value_NULL_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(NULL, OPTION_HTTP_PROXY, NULL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_http_proxy_succeed)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(prov_transport_set_proxy(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        HTTP_PROXY_OPTIONS http_proxy;
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, OPTION_HTTP_PROXY, &http_proxy);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_http_proxy_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(prov_transport_set_proxy(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);

        //act
        HTTP_PROXY_OPTIONS http_proxy;
        http_proxy.host_address = TEST_HTTP_PROXY_VAL;
        http_proxy.port = 80;
        http_proxy.username = TEST_HTTP_USERNAME;
        http_proxy.password = TEST_HTTP_PASSWORD;
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, OPTION_HTTP_PROXY, &http_proxy);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_ERROR, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_logtrace_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(prov_transport_set_trace(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        //act
        bool trace = true;
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, "logtrace", &trace);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_logtrace_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(prov_transport_set_trace(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);

        //act
        bool trace = true;
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, "logtrace", &trace);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_ERROR, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_TrustedCerts_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(prov_transport_set_trusted_cert(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, "TrustedCerts", TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_TrustedCerts_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(prov_transport_set_trusted_cert(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, "TrustedCerts", TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_ERROR, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_Registration_id_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(prov_auth_set_registration_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, PROV_REGISTRATION_ID, TEST_REGISTRATION_ID);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_Registration_id_set_reg_id_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(prov_auth_set_registration_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, PROV_REGISTRATION_ID, TEST_REGISTRATION_ID);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_ERROR, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_Registration_id_reg_id_NULL_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, PROV_REGISTRATION_ID, NULL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_ERROR, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_Registration_id_after_register_fail)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        umock_c_reset_all_calls();

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, PROV_REGISTRATION_ID, TEST_REGISTRATION_ID);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_ERROR, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_Timeout_NULL_timeout_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, PROV_OPTION_TIMEOUT, NULL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_ERROR, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_SetOption_Timeout_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        uint8_t prov_timeout = 10;

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, PROV_OPTION_TIMEOUT, &prov_timeout);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Set_Provisioning_Payload_handle_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Set_Provisioning_Payload(NULL, TEST_CUSTOM_DATA);

        //assert
        ASSERT_ARE_NOT_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_Set_Provisioning_Payload_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CUSTOM_DATA));

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Set_Provisioning_Payload(handle, TEST_CUSTOM_DATA);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Set_Provisioning_Payload_set_twice_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CUSTOM_DATA));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CUSTOM_DATA));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Set_Provisioning_Payload(handle, TEST_CUSTOM_DATA);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        prov_result = Prov_Device_LL_Set_Provisioning_Payload(handle, TEST_CUSTOM_DATA);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_Payload_handle_NULL_success)
    {
        //arrange

        //act
        const char* result = Prov_Device_LL_Get_Provisioning_Payload(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_Payload_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true, true);
        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);
        umock_c_reset_all_calls();

        //act
        const char* result = Prov_Device_LL_Get_Provisioning_Payload(handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, TEST_CUSTOM_DATA, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(parse_info);
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_Payload_no_data_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true, false);
        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);
        umock_c_reset_all_calls();

        //act
        const char* result = Prov_Device_LL_Get_Provisioning_Payload(handle);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(parse_info);
        Prov_Device_LL_Destroy(handle);
    }

    END_TEST_SUITE(prov_device_client_ll_ut)
