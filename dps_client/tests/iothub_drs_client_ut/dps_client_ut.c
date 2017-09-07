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
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/tickcounter.h"

#include "azure_hub_modules/iothub_device_auth.h"
#include "azure_hub_modules/dps_sec_client.h"
#include "parson.h"

#undef ENABLE_MOCKS

#include "azure_hub_modules/dps_client.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, on_dps_register_device_callback, DPS_RESULT, register_result, const char*, iothub_uri, const char*, device_id, void*, user_context);
MOCKABLE_FUNCTION(, void, on_dps_error_callback, DPS_ERROR, error_type, void*, user_context);
MOCKABLE_FUNCTION(, void, on_dps_register_status_callback, DPS_REGISTRATION_STATUS, reg_status, void*, user_context);
MOCKABLE_FUNCTION(, char*, on_dps_transport_challenge_cb, const unsigned char*, nonce, size_t, nonce_len, const char*, key_name, void*, user_ctx);

MOCKABLE_FUNCTION(, DPS_TRANSPORT_HANDLE, dps_transport_create, const char*, uri, DPS_HSM_TYPE, type, const char*, scope_id, const char*, registration_id, const char*, dps_api_version);
MOCKABLE_FUNCTION(, void, dps_transport_destroy, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_transport_open, DPS_TRANSPORT_HANDLE, handle, BUFFER_HANDLE, ek, BUFFER_HANDLE, srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK, data_callback, void*, user_ctx, DPS_TRANSPORT_STATUS_CALLBACK, status_cb, void*, status_ctx);
MOCKABLE_FUNCTION(, int, dps_transport_close, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_transport_register_device, DPS_TRANSPORT_HANDLE, handle, DPS_TRANSPORT_CHALLENGE_CALLBACK, reg_challenge_cb, void*, user_ctx, DPS_TRANSPORT_JSON_PARSE, json_parse_cb, void*, json_ctx);
MOCKABLE_FUNCTION(, int, dps_transport_get_operation_status, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, void, dps_transport_dowork, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_transport_set_trace, DPS_TRANSPORT_HANDLE, handle, bool, trace_on);
MOCKABLE_FUNCTION(, int, dps_transport_x509_cert, DPS_TRANSPORT_HANDLE, handle, const char*, certificate, const char*, private_key);
MOCKABLE_FUNCTION(, int, dps_transport_set_trusted_cert, DPS_TRANSPORT_HANDLE, handle, const char*, certificate);
MOCKABLE_FUNCTION(, int, dps_transport_set_proxy, DPS_TRANSPORT_HANDLE, handle, const HTTP_PROXY_OPTIONS*, proxy_option);

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

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;
static DPS_TRANSPORT_CHALLENGE_CALLBACK g_challenge_callback;
static void* g_challenge_ctx;
static DPS_TRANSPORT_REGISTER_DATA_CALLBACK g_registration_callback;
static void* g_registration_ctx;
DPS_TRANSPORT_STATUS_CALLBACK g_status_callback;
static void* g_status_ctx;
static void* g_http_error_ctx;
DPS_TRANSPORT_JSON_PARSE g_json_parse_cb;
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

static DPS_TRANSPORT_PROVIDER g_dps_transport_func =
{
    dps_transport_create,
    dps_transport_destroy,
    dps_transport_open,
    dps_transport_close,
    dps_transport_register_device,
    dps_transport_get_operation_status,
    dps_transport_dowork,
    dps_transport_set_trace,
    dps_transport_x509_cert,
    dps_transport_set_trusted_cert,
    dps_transport_set_proxy
};

static const DPS_TRANSPORT_PROVIDER* trans_provider(void)
{
    return &g_dps_transport_func;
}

static const BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x11111116;

static const char* TEST_JSON_REPLY = "{ json_reply }";
static const char* TEST_DPS_URI = "www.dps_uri.com";
static const char* TEST_SCOPE_ID = "scope_id";
static const char* TEST_STRING_HANDLE_VALUE = "string_value";
static size_t TEST_STRING_LENGTH = sizeof("string_value");
static const char* TEST_DEVICE_ID = "device_id";
static const char* TEST_IOTHUB = "iothub.value.test";
static const char* TEST_KEY = "AD87D58E-4E52-411B-9C63-49BD02C84F0D";
static const char* TEST_REGISTRATION_ID = "A87FA22F-828B-46CA-BA37-D574C32E423E";
static const char* TEST_DPS_ASSIGNED_STATUS = "assigned";
static const char* TEST_HTTP_PROXY_VAL = "127.0.0.1";
static const char* TEST_CERTIFICATE_VAL = "--BEGIN_CERT 12345 END_CERT--";
static const char* TEST_HTTP_USERNAME = "username";
static const char* TEST_HTTP_PASSWORD = "password";
static const char* TEST_TRUSTED_CERT = "trusted_cert";
static char* TEST_STRING_VALUE = "Test_String_Value";

static const char* DPS_ASSIGNED_STATUS = "assigned";
static const char* DPS_ASSIGNING_STATUS = "assigning";
static const char* DPS_UNASSIGNED_STATUS = "unassigned";
static const char* DPS_FAILURE_STATUS = "failure";

static int TEST_STATUS_CODE_OK = 200;
static int TEST_STATUS_CODE_2 = 299;
static int TEST_UNAUTHORIZED_CODE = 401;
static int TEST_ERROR_STATUS_CODE = 500;
#define TEST_JSON_ROOT_VALUE (JSON_Value*)0x11111112
#define TEST_JSON_OBJECT_VALUE (JSON_Object*)0x11111113
#define TEST_JSON_STATUS_VALUE (JSON_Value*)0x11111114
#define TEST_BUFFER_HANDLE_VALUE (BUFFER_HANDLE)0x11111115

static unsigned char TEST_ENDORSMENT_KEY[] = { 'k', 'e', 'y' };

static unsigned char TEST_DATA[] = { 'k', 'e', 'y' };
static const size_t TEST_DATA_LEN = 3;
static const char* TEST_DECRYPTED_NONCE = "decrypted_nonce";
static const size_t TEST_DECRYPTED_NONCE_LEN = sizeof("decrypted_nonce");

TEST_DEFINE_ENUM_TYPE(DPS_RESULT, DPS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DPS_RESULT, DPS_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(DPS_ERROR, DPS_ERROR_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DPS_ERROR, DPS_ERROR_VALUES);

TEST_DEFINE_ENUM_TYPE(DPS_SEC_TYPE, DPS_SEC_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DPS_SEC_TYPE, DPS_SEC_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(DPS_REGISTRATION_STATUS, DPS_REGISTRATION_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DPS_REGISTRATION_STATUS, DPS_REGISTRATION_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(DPS_SEC_RESULT, DPS_SEC_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DPS_SEC_RESULT, DPS_SEC_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(DPS_HSM_TYPE, DPS_HSM_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DPS_HSM_TYPE, DPS_HSM_TYPE_VALUES);

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

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
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

static DPS_TRANSPORT_HANDLE my_dps_transport_create(const char* uri, DPS_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* dps_api_version)
{
    (void)type;
    (void)uri;
    (void)scope_id;
    (void)registration_id;
    (void)dps_api_version;
    return (DPS_TRANSPORT_HANDLE)my_gballoc_malloc(1);
}

static void my_dps_transport_destroy(DPS_TRANSPORT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static int my_dps_transport_open(DPS_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
{
    (void)handle;
    (void)ek;
    (void)srk;
    g_registration_callback = data_callback;
    g_registration_ctx = user_ctx;
    g_status_callback = status_cb;
    g_status_ctx = status_ctx;
    return 0;
}

static int my_dps_transport_register_device(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx, DPS_TRANSPORT_JSON_PARSE json_parse_cb, void* json_ctx)
{
    (void)handle;
    g_challenge_callback = reg_challenge_cb;
    g_challenge_ctx = user_ctx;
    g_json_parse_cb = json_parse_cb;
    g_json_ctx = json_ctx;
    return 0;
}

static DPS_SEC_HANDLE my_dps_sec_create(void)
{
    return (DPS_SEC_HANDLE)my_gballoc_malloc(1);
}

static void my_dps_sec_destroy(DPS_SEC_HANDLE handle)
{
    my_gballoc_free(handle);
}

static char* my_dps_sec_get_registration_id(DPS_SEC_HANDLE handle)
{
    (void)handle;
    char* result;
    size_t data_len = strlen(TEST_REGISTRATION_ID);
    result = (char*)my_gballoc_malloc(data_len+1);
    strcpy(result, TEST_REGISTRATION_ID);
    return result;
}

static BUFFER_HANDLE my_dps_sec_get_endorsement_key(DPS_SEC_HANDLE handle)
{
    (void)handle;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_dps_sec_get_storage_key(DPS_SEC_HANDLE handle)
{
    (void)handle;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static char* my_dps_sec_get_certificate(DPS_SEC_HANDLE handle)
{
    (void)handle;
    char* result;
    size_t data_len = strlen(TEST_CERTIFICATE_VAL);
    result = (char*)my_gballoc_malloc(data_len + 1);
    strcpy(result, TEST_CERTIFICATE_VAL);
    return result;
}

static char* my_dps_sec_get_alias_key(DPS_SEC_HANDLE handle)
{
    (void)handle;
    char* result;
    size_t data_len = strlen(TEST_CERTIFICATE_VAL);
    result = (char*)my_gballoc_malloc(data_len + 1);
    strcpy(result, TEST_CERTIFICATE_VAL);
    return result;
}

static char* my_dps_sec_get_signer_cert(DPS_SEC_HANDLE handle)
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

static STRING_HANDLE my_SASToken_Create(STRING_HANDLE key, STRING_HANDLE scope, STRING_HANDLE keyName, size_t expiry)
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

static STRING_HANDLE my_Base64_Encode_Bytes(const unsigned char* source, size_t size)
{
    (void)source;(void)size;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_Base64_Decoder(const char* source)
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

BEGIN_TEST_SUITE(dps_client_ut)

    TEST_SUITE_INITIALIZE(suite_init)
    {
        int result;

        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);
        (void)umocktypes_bool_register_types();

        result = umocktypes_charptr_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);
        result = umocktypes_stdint_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);

        REGISTER_TYPE(DPS_RESULT, DPS_RESULT);
        REGISTER_TYPE(DPS_ERROR, DPS_ERROR);
        REGISTER_TYPE(DPS_SEC_TYPE, DPS_SEC_TYPE);
        REGISTER_TYPE(DPS_REGISTRATION_STATUS, DPS_REGISTRATION_STATUS);
        REGISTER_TYPE(DPS_SEC_RESULT, DPS_SEC_RESULT);
        REGISTER_TYPE(DPS_HSM_TYPE, DPS_HSM_TYPE);

        REGISTER_UMOCK_ALIAS_TYPE(XDA_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(DPS_TRANSPORT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(JSON_Value, void*);
        REGISTER_UMOCK_ALIAS_TYPE(JSON_Object, void*);
        REGISTER_UMOCK_ALIAS_TYPE(DPS_TRANSPORT_STATUS_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(DPS_TRANSPORT_REGISTER_DATA_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(DPS_TRANSPORT_CHALLENGE_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(DPS_SEC_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SEC_HANDLE, void*); 
        REGISTER_UMOCK_ALIAS_TYPE(DPS_TRANSPORT_JSON_PARSE, void*);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(tickcounter_create, my_tickcounter_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(tickcounter_destroy, my_tickcounter_destroy);

        REGISTER_GLOBAL_MOCK_HOOK(dps_transport_create, my_dps_transport_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_transport_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_transport_destroy, my_dps_transport_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(dps_transport_open, my_dps_transport_open);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_transport_open, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(dps_transport_register_device, my_dps_transport_register_device);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_transport_register_device, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(dps_transport_x509_cert, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_transport_x509_cert, __LINE__);

        REGISTER_GLOBAL_MOCK_RETURN(dps_sec_get_type, DPS_SEC_TYPE_TPM);

        REGISTER_GLOBAL_MOCK_HOOK(dps_sec_create, my_dps_sec_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_sec_destroy, my_dps_sec_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(dps_sec_get_registration_id, my_dps_sec_get_registration_id);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_get_registration_id, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_sec_get_certificate, my_dps_sec_get_certificate);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_get_certificate, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_sec_get_alias_key, my_dps_sec_get_alias_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_get_alias_key, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_sec_get_signer_cert, my_dps_sec_get_signer_cert);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_get_signer_cert, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(dps_sec_construct_sas_token, "Sas_token");
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_construct_sas_token, NULL);

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

        REGISTER_GLOBAL_MOCK_HOOK(dps_sec_get_endorsement_key, my_dps_sec_get_endorsement_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_get_endorsement_key, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_sec_get_storage_key, my_dps_sec_get_storage_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_get_storage_key, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(dps_sec_import_key, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_sec_import_key, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_HANDLE_VALUE);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_length, TEST_STRING_LENGTH);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_create, TEST_BUFFER_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
        //REGISTER_GLOBAL_MOCK_RETURN(BUFFER_delete, TEST_BUFFER_HANDLE);

        REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(SASToken_Create, my_SASToken_Create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(SASToken_Create, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(Base64_Encode_Bytes, my_Base64_Encode_Bytes);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Base64_Encode_Bytes, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(Base64_Decoder, my_Base64_Decoder);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Base64_Decoder, NULL);
}

    TEST_SUITE_CLEANUP(suite_cleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
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

    static void setup_retrieve_json_item_mocks(const char* return_item)
    {
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(return_item);
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_DPS_LL_on_registration_data_mocks(void)
    {
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(dps_sec_import_key(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(on_dps_register_device_callback(DPS_CLIENT_OK, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_DPS_LL_Create_mocks(DPS_SEC_TYPE type)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(dps_sec_create());
        STRICT_EXPECTED_CALL(dps_sec_get_registration_id(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_create());
        STRICT_EXPECTED_CALL(dps_sec_get_type(IGNORED_PTR_ARG)).SetReturn(type);
        STRICT_EXPECTED_CALL(dps_transport_create(IGNORED_PTR_ARG, type == DPS_SEC_TYPE_TPM ? DPS_HSM_TYPE_TPM : DPS_HSM_TYPE_X509, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_DPS_LL_Register_Device_mocks(bool tpm)
    {
        if (tpm)
        {
            STRICT_EXPECTED_CALL(dps_sec_get_endorsement_key(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(dps_sec_get_storage_key(IGNORED_PTR_ARG));
        }
        else
        {
            STRICT_EXPECTED_CALL(dps_sec_get_certificate(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(dps_sec_get_alias_key(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(dps_transport_x509_cert(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(dps_transport_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    }

    static void setup_cleanup_dps_info_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_destroy_dps_info_mocks(void)
    {
        STRICT_EXPECTED_CALL(dps_transport_destroy(IGNORED_PTR_ARG));
        setup_cleanup_dps_info_mocks();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(dps_sec_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_DPS_LL_DoWork_register_send_mocks()
    {
        STRICT_EXPECTED_CALL(dps_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(dps_transport_register_device(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_challenge_callback_mocks(void)
    {
        STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(dps_sec_import_key(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(dps_sec_construct_sas_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
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
        STRICT_EXPECTED_CALL(Base64_Decoder(IGNORED_PTR_ARG));
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }

    static void setup_parse_json_assigning_mocks(void)
    {
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(DPS_ASSIGNING_STATUS);

        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }

    static void setup_parse_json_assigned_mocks(void)
    {
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(DPS_ASSIGNED_STATUS);

        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG)).SetReturn(DPS_ASSIGNED_STATUS);
        STRICT_EXPECTED_CALL(Base64_Decoder(IGNORED_PTR_ARG));

        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_001: [If dev_auth_handle or dps_uri is NULL DPS_LL_Create shall return NULL.] */
    TEST_FUNCTION(DPS_LL_Create_uri_NULL_fail)
    {
        //arrange

        //act
        DPS_LL_HANDLE result = DPS_LL_Create(NULL, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_002: [ DPS_LL_Create shall allocate a DPS_LL_HANDLE and initialize all members. ] */
    /* Tests_SRS_DPS_CLIENT_CLIENT_07_028: [ DPS_CLIENT_STATE_READY is the initial state after the DPS object is created which will send a uhttp_client_open call to the DPS http endpoint. ] */
    /* Tests_SRS_DPS_CLIENT_CLIENT_07_034: [ DPS_LL_Create shall construct a scope_id by base64 encoding the dps_uri. ] */
    /* Tests_SRS_DPS_CLIENT_CLIENT_07_035: [ DPS_LL_Create shall store the registration_id from the security module. ] */
    TEST_FUNCTION(DPS_LL_Create_succees)
    {
        //arrange
        setup_DPS_LL_Create_mocks(DPS_SEC_TYPE_TPM);

        //act
        DPS_LL_HANDLE result = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(result);
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_003: [ If any error is encountered, DPS_LL_Create shall return NULL. ] */
    TEST_FUNCTION(DPS_LL_Create_fail)
    {
        //arrange
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_DPS_LL_Create_mocks(DPS_SEC_TYPE_TPM);

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

            char tmp_msg[64];
            sprintf(tmp_msg, "DPS_LL_Create failure in test %zu/%zu", index, count);
            
            DPS_LL_HANDLE result = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);

            // assert
            ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_005: [ If handle is NULL DPS_LL_Destroy shall do nothing. ] */
    TEST_FUNCTION(DPS_LL_Destroy_handle_NULL)
    {
        //arrange

        //act
        DPS_LL_Destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_006: [ DPS_LL_Destroy shall destroy resources associated with the IoTHub_dps_client ] */
    TEST_FUNCTION(DPS_LL_Destroy_succeed)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_close(IGNORED_PTR_ARG));
        setup_destroy_dps_info_mocks();

        //act
        DPS_LL_Destroy(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_007: [ If handle, device_id or register_callback is NULL, DPS_LL_Register_Device shall return DPS_CLIENT_INVALID_ARG. ] */
    TEST_FUNCTION(DPS_LL_Register_Device_handle_NULL_fail)
    {
        //arrange

        //act
        DPS_RESULT dps_result = DPS_LL_Register_Device(NULL, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_INVALID_ARG, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_007: [ If handle, device_id or register_callback is NULL, DPS_LL_Register_Device shall return DPS_CLIENT_INVALID_ARG. ] */
    TEST_FUNCTION(DPS_LL_Register_Device_register_callback_NULL_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        //act
        DPS_RESULT dps_result = DPS_LL_Register_Device(handle, NULL, NULL, on_dps_register_status_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_INVALID_ARG, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_008: [ DPS_LL_Register_Device shall set the state to send the registration request to DPS on subsequent DoWork calls. ] */
    TEST_FUNCTION(DPS_LL_Register_Device_tpm_succeed)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        setup_DPS_LL_Register_Device_mocks(true);
        //act
        DPS_RESULT dps_result = DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_OK, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_Register_Device_x509_succeed)
    {
        //arrange
        setup_DPS_LL_Create_mocks(DPS_SEC_TYPE_X509);
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        setup_DPS_LL_Register_Device_mocks(false);

        //act
        DPS_RESULT dps_result = DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_OK, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_Register_Device_tpm_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_DPS_LL_Register_Device_mocks(true);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 3, 4 };

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

            char tmp_msg[64];
            sprintf(tmp_msg, "DPS_LL_Register_Device failure in test %zu/%zu", index, count);

            //act
            DPS_RESULT dps_result = DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);

            //assert
            ASSERT_ARE_EQUAL_WITH_MSG(DPS_RESULT, DPS_CLIENT_ERROR, dps_result, tmp_msg);
        }

        //cleanup
        DPS_LL_Destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(DPS_LL_Register_Device_riot_fail)
    {
        //arrange
        setup_DPS_LL_Create_mocks(DPS_SEC_TYPE_X509);
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_DPS_LL_Register_Device_mocks(false);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 3, 4, 6, 7 };

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

            char tmp_msg[64];
            sprintf(tmp_msg, "DPS_LL_Register_Device failure in test %zu/%zu", index, count);

            //act
            DPS_RESULT dps_result = DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);

            //assert
            ASSERT_ARE_EQUAL_WITH_MSG(DPS_RESULT, DPS_CLIENT_ERROR, dps_result, tmp_msg);
        }

        //cleanup
        DPS_LL_Destroy(handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_010: [ If handle is NULL, DPS_LL_DoWork shall do nothing. ] */
    TEST_FUNCTION(DPS_LL_DoWork_handle_NULL_fail)
    {
        //arrange

        //act
        DPS_LL_DoWork(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_030: [ DPS_CLIENT_STATE_REGISTER_SENT state shall retrieve the endorsement_key, auth_type and hsm_type from a call to the dev_auth modules function. ] */
    /* Tests_SRS_DPS_CLIENT_CLIENT_07_013: [ The DPS_CLIENT_STATE_REGISTER_SENTstate shall construct http request using uhttp_client_execute_request to the DPS service with the following endorsement information: ] */
    /* Tests_SRS_DPS_CLIENT_CLIENT_07_011: [ DPS_LL_DoWork shall call the underlying http_client_dowork function ] */
    /* Tests_SRS_DPS_CLIENT_CLIENT_07_19: [ Upon successfully sending the messge iothub_dps_client shall transition to the DPS_CLIENT_STATE_REGISTER_SENT state ] */
    TEST_FUNCTION(DPS_LL_DoWork_register_send_succeed)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        umock_c_reset_all_calls();

        setup_DPS_LL_DoWork_register_send_mocks();

        //act
        DPS_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_DoWork_register_send_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_DPS_LL_DoWork_register_send_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 0 };

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            {
                continue;
            }

            (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "DPS_LL_DoWork failure in test %zu/%zu", index, count);

            //act
            DPS_LL_DoWork(handle);
            // To Get the device back in registration mode
            DPS_LL_DoWork(handle);

            //assert
        }

        //cleanup
        umock_c_negative_tests_deinit();
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_parse_json_unassigned_fail)
    {
        DPS_JSON_INFO* result;

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_unassigned_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 3, 4, 6, 9, 11 };

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

            char tmp_msg[64];
            sprintf(tmp_msg, "g_json_parse_cb failure in test %zu/%zu", index, count);


            result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

            //assert
            ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_parse_json_assigning_succeed)
    {
        DPS_JSON_INFO* result;
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
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
        DPS_LL_Destroy(handle);
        my_gballoc_free(result->operation_id);
        my_gballoc_free(result);
    }

    TEST_FUNCTION(DPS_LL_parse_json_assigning_fail)
    {
        DPS_JSON_INFO* result;
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigning_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 3, 4, 6, 8 };

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

            char tmp_msg[64];
            sprintf(tmp_msg, "g_json_parse_cb failure in test %zu/%zu", index, count);


            result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

            //assert
            ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_parse_json_assigned_succeed)
    {
        DPS_JSON_INFO* result;
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks();

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
        DPS_LL_Destroy(handle);
        my_gballoc_free(result->authorization_key);
        my_gballoc_free(result->iothub_uri);
        my_gballoc_free(result->device_id);
        my_gballoc_free(result);
    }

    TEST_FUNCTION(DPS_LL_parse_json_assigned_fail)
    {
        DPS_JSON_INFO* result;
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 3, 4, 8, 11, 14, 16 };

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

            char tmp_msg[64];
            sprintf(tmp_msg, "g_json_parse_cb failure in test %zu/%zu", index, count);
            
            result = g_json_parse_cb(TEST_JSON_REPLY, g_json_ctx);

            //assert
            ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
        DPS_LL_Destroy(handle);
    }

    /* Tests_SRS_DPS_CLIENT_CLIENT_07_009: [ Upon success DPS_LL_Register_Device shall return DPS_CLIENT_OK. ] */
    TEST_FUNCTION(DPS_LL_DoWork_no_connection_succeed)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        DPS_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_DoWork_get_operation_status_send_succeed)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        g_status_callback(DPS_TRANSPORT_STATUS_AUTHENTICATED, g_status_ctx);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(dps_transport_get_operation_status(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        DPS_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_DoWork_get_operation_status_send_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        g_status_callback(DPS_TRANSPORT_STATUS_AUTHENTICATED, g_status_ctx);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(dps_transport_get_operation_status(IGNORED_PTR_ARG)).SetReturn(__LINE__);
        STRICT_EXPECTED_CALL(dps_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_dps_error_callback(DPS_ERROR_PARSING, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        DPS_LL_DoWork(handle);
        DPS_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_challenge_cb_nonce_NULL_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        char* result = g_challenge_callback(NULL, TEST_DATA_LEN, TEST_STRING_HANDLE_VALUE, g_challenge_ctx);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_challenge_cb_user_ctx_NULL_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        char* result = g_challenge_callback(TEST_DATA, TEST_DATA_LEN, TEST_STRING_HANDLE_VALUE, NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_challenge_cb_succeed)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_challenge_callback_mocks();

        //act
        char* result = g_challenge_callback(TEST_DATA, TEST_DATA_LEN, TEST_STRING_HANDLE_VALUE, g_challenge_ctx);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_challenge_cb_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_challenge_callback_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 0, 4, 6, 7 };

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

            char tmp_msg[64];
            sprintf(tmp_msg, "g_challenge_callback failure in test %zu/%zu", index, count);
            
            char* result = g_challenge_callback(TEST_DATA, TEST_DATA_LEN, TEST_STRING_HANDLE_VALUE, g_challenge_ctx);

            //assert
            ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_on_registration_data_succeed)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_DPS_LL_on_registration_data_mocks();

        //act
        g_registration_callback(DPS_TRANSPORT_RESULT_OK, TEST_BUFFER_HANDLE_VALUE, TEST_IOTHUB, TEST_DEVICE_ID, g_registration_ctx);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_on_registration_data_iothub_key_NULL_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        g_registration_callback(DPS_TRANSPORT_RESULT_OK, NULL, TEST_IOTHUB, TEST_DEVICE_ID, g_registration_ctx);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_on_registration_data_dps_sec_import_key_0_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        g_registration_callback(DPS_TRANSPORT_RESULT_OK, NULL, TEST_IOTHUB, TEST_DEVICE_ID, g_registration_ctx);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_on_registration_data_user_ctx_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        //act
        g_registration_callback(DPS_TRANSPORT_RESULT_OK, TEST_BUFFER_HANDLE_VALUE, TEST_IOTHUB, TEST_DEVICE_ID, NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_on_registration_data_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_DPS_LL_on_registration_data_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 1, 2, 5 };

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

            char tmp_msg[64];
            sprintf(tmp_msg, "g_registration_callback failure in test %zu/%zu", index, count);

            //act
            g_registration_callback(DPS_TRANSPORT_RESULT_OK, TEST_BUFFER_HANDLE_VALUE, TEST_IOTHUB, TEST_DEVICE_ID, g_registration_ctx);

            //assert
        }

        //cleanup
        DPS_LL_Destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(DPS_LL_on_registration_data_error_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_dps_error_callback(DPS_ERROR_TRANSPORT, IGNORED_PTR_ARG));
        setup_cleanup_dps_info_mocks();

        //act
        g_registration_callback(DPS_TRANSPORT_RESULT_ERROR, NULL, NULL, NULL, g_registration_ctx);
        DPS_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_on_registration_unauthorized_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        DPS_LL_DoWork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_dps_error_callback(DPS_ERROR_DEV_AUTH_ERROR, IGNORED_PTR_ARG));
        setup_cleanup_dps_info_mocks();

        //act
        g_registration_callback(DPS_TRANSPORT_RESULT_UNAUTHORIZED, NULL, NULL, NULL, g_registration_ctx);
        DPS_LL_DoWork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_SetOption_handle_NULL_fail)
    {
        //arrange

        //act
        HTTP_PROXY_OPTIONS http_proxy;
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
        http_proxy.host_address = TEST_HTTP_PROXY_VAL;
        http_proxy.port = 80;
        DPS_RESULT dps_result = DPS_LL_SetOption(NULL, OPTION_HTTP_PROXY, &http_proxy);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_INVALID_ARG, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(DPS_LL_SetOption_option_name_NULL_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        //act
        HTTP_PROXY_OPTIONS http_proxy;
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
        http_proxy.host_address = TEST_HTTP_PROXY_VAL;
        http_proxy.port = 80;
        DPS_RESULT dps_result = DPS_LL_SetOption(NULL, NULL, &http_proxy);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_INVALID_ARG, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_SetOption_value_NULL_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        //act
        DPS_RESULT dps_result = DPS_LL_SetOption(NULL, OPTION_HTTP_PROXY, NULL);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_INVALID_ARG, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_SetOption_http_proxy_succeed)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_set_proxy(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        HTTP_PROXY_OPTIONS http_proxy;
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
        DPS_RESULT dps_result = DPS_LL_SetOption(handle, OPTION_HTTP_PROXY, &http_proxy);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_OK, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_SetOption_http_proxy_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_set_proxy(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);

        //act
        HTTP_PROXY_OPTIONS http_proxy;
        http_proxy.host_address = TEST_HTTP_PROXY_VAL;
        http_proxy.port = 80;
        http_proxy.username = TEST_HTTP_USERNAME;
        http_proxy.password = TEST_HTTP_PASSWORD;
        DPS_RESULT dps_result = DPS_LL_SetOption(handle, OPTION_HTTP_PROXY, &http_proxy);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_ERROR, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_SetOption_logtrace_success)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_set_trace(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        //act
        bool trace = true;
        DPS_RESULT dps_result = DPS_LL_SetOption(handle, "logtrace", &trace);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_OK, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_SetOption_logtrace_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        (void)DPS_LL_Register_Device(handle, on_dps_register_device_callback, NULL, on_dps_register_status_callback, NULL);
        g_status_callback(DPS_TRANSPORT_STATUS_CONNECTED, g_status_ctx);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_set_trace(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);

        //act
        bool trace = true;
        DPS_RESULT dps_result = DPS_LL_SetOption(handle, "logtrace", &trace);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_ERROR, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_SetOption_TrustedCerts_success)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_set_trusted_cert(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        DPS_RESULT dps_result = DPS_LL_SetOption(handle, "TrustedCerts", TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_OK, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(DPS_LL_SetOption_TrustedCerts_fail)
    {
        //arrange
        DPS_LL_HANDLE handle = DPS_LL_Create(TEST_DPS_URI, TEST_SCOPE_ID, trans_provider, on_dps_error_callback, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(dps_transport_set_trusted_cert(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);

        //act
        DPS_RESULT dps_result = DPS_LL_SetOption(handle, "TrustedCerts", TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(DPS_RESULT, DPS_CLIENT_ERROR, dps_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        DPS_LL_Destroy(handle);
    }

    END_TEST_SUITE(dps_client_ut)
