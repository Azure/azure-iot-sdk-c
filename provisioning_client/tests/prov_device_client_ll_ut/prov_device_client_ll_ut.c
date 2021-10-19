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
MOCKABLE_FUNCTION(, void, on_transport_error, PROV_DEVICE_TRANSPORT_ERROR, transport_error, void*, user_context);

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
MOCKABLE_FUNCTION(, int, prov_transport_set_option, PROV_DEVICE_TRANSPORT_HANDLE, handle, const char*, option_name, const void*, value);
#undef ENABLE_MOCKS

static TEST_MUTEX_HANDLE g_testByTest;
static PROV_TRANSPORT_CHALLENGE_CALLBACK g_challenge_callback;
static void* g_challenge_ctx;
static PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK g_registration_callback;
static void* g_registration_ctx;
PROV_DEVICE_TRANSPORT_STATUS_CALLBACK g_status_callback;
PROV_TRANSPORT_ERROR_CALLBACK g_error_callback;
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
    prov_transport_set_proxy,
    prov_transport_set_option
};

static const PROV_DEVICE_TRANSPORT_PROVIDER* trans_provider(void)
{
    return &g_prov_transport_func;
}

static const BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x11111116;

static const char* TEST_JSON_ASSIGNED_REPLY = 
"{" \
"    \"operationId\": \"string\"," \
"    \"status\": \"assigned\"," \
"    \"registrationState\": {" \
"        \"tpm\": {" \
"            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
"        }," \
"        \"registrationId\": \"string\"," \
"        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
"        \"assignedHub\": \"testiothub.azure-devices.net\"," \
"        \"deviceId\": \"testdevice\"," \
"        \"status\": \"assigned\"," \
"        \"substatus\": \"initialAssignment\"," \
"        \"errorCode\": -2147483648," \
"        \"errorMessage\": \"string\"," \
"        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
"        \"etag\": \"string\"" \
"    }" \
"}";

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

static const char* TEST_CUSTOM_DATA = "{\"json_cust_data\":123456}";
#define TEST_TRUST_BUNDLE_ETAG "\"5301e830-0000-0300-0000-613864190000\""

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

static PROV_DEVICE_TRANSPORT_HANDLE my_prov_transport_create(const char* uri, TRANSPORT_HSM_TYPE type, const char* scope_id, const char* prov_api_version, PROV_TRANSPORT_ERROR_CALLBACK error_cb, void* error_ctx)
{
    (void)type;
    (void)uri;
    (void)scope_id;
    (void)prov_api_version;
    (void)error_cb;
    (void)error_ctx;

    g_error_callback = error_cb;
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
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
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
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(Azure_Base64_Decode(IGNORED_PTR_ARG));
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
    }

    static void setup_parse_json_assigning_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_parse_json_disabled_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
    }

    static void setup_parse_json_error_mocks(double return_err_num)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_parse_json_assigned_mocks(bool use_tpm)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        if (use_tpm)
        {
            STRICT_EXPECTED_CALL(Azure_Base64_Decode(IGNORED_PTR_ARG));
        }
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
        setup_retrieve_json_item_mocks(TEST_STRING_VALUE);
    }

    static void setup_parse_json_nohub_mocks(bool use_tpm)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        if (use_tpm)
        {
            STRICT_EXPECTED_CALL(Azure_Base64_Decode(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_001: [If dev_auth_handle or prov_uri is NULL Prov_Device_LL_Create shall return NULL.] */
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

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_002: [ Prov_Device_LL_Create shall allocate a PROV_DEVICE_LL_HANDLE and initialize all members. ] */
    /* Tests_SRS_PROV_CLIENT_CLIENT_07_028: [ PROV_CLIENT_STATE_READY is the initial state after the object is created which will send a uhttp_client_open call to the http endpoint. ] */
    /* Tests_SRS_PROV_CLIENT_CLIENT_07_034: [ Prov_Device_LL_Create shall construct a scope_id by base64 encoding the prov_uri. ] */
    /* Tests_SRS_PROV_CLIENT_CLIENT_07_035: [ Prov_Device_LL_Create shall store the registration_id from the security module. ] */
    TEST_FUNCTION(Prov_Device_LL_Create_success)
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

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_003: [ If any error is encountered, Prov_Device_LL_Create shall return NULL. ] */
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

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_005: [ If handle is NULL Prov_Device_LL_Destroy shall do nothing. ] */
    TEST_FUNCTION(Prov_Device_LL_Destroy_handle_NULL)
    {
        //arrange

        //act
        Prov_Device_LL_Destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_006: [ Prov_Device_LL_Destroy shall destroy resources associated with the IoTHub_prov_client ] */
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

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_007: [ If handle, device_id or register_callback is NULL, Prov_Device_LL_Register_Device shall return PROV_DEVICE_RESULT_INVALID_ARG. ] */
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

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_007: [ If handle, device_id or register_callback is NULL, Prov_Device_LL_Register_Device shall return PROV_DEVICE_RESULT_INVALID_ARG. ] */
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

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_008: [ Prov_Device_LL_Register_Device shall set the state to send the registration request to on subsequent DoWork calls. ] */
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

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_010: [ If handle is NULL, Prov_Device_LL_DoWork shall do nothing. ] */
    TEST_FUNCTION(Prov_Device_LL_DoWork_handle_NULL_fail)
    {
        //arrange

        //act
        Prov_Device_LL_DoWork(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_030: [ PROV_CLIENT_STATE_REGISTER_SENT state shall retrieve the endorsement_key, auth_type and hsm_type from a call to the dev_auth modules function. ] */
    /* Tests_SRS_PROV_CLIENT_CLIENT_07_013: [ The PROV_CLIENT_STATE_REGISTER_SENTstate shall construct http request using uhttp_client_execute_request to the service with the following endorsement information: ] */
    /* Tests_SRS_PROV_CLIENT_CLIENT_07_011: [ Prov_Device_LL_DoWork shall call the underlying http_client_dowork function ] */
    /* Tests_SRS_PROV_CLIENT_CLIENT_07_19: [ Upon successfully sending the message iothub_prov_client shall transition to the PROV_CLIENT_STATE_REGISTER_SENT state ] */
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

    TEST_FUNCTION(Prov_Device_LL_DoWork_register_transport_error_fail)
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
        int error_type = 0;
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);

                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                //act
                Prov_Device_LL_DoWork(handle);

                switch (error_type)
                {
                    case 0:
                        g_error_callback(PROV_DEVICE_ERROR_KEY_FAIL, g_registration_ctx);
                        break;
                    case 1:
                        g_error_callback(PROV_DEVICE_ERROR_KEY_UNAUTHORIZED, g_registration_ctx);
                        break;
                    default:
                        g_error_callback(PROV_DEVICE_ERROR_MEMORY, g_registration_ctx);
                }
                
                error_type++;
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
        static const char* TEST_JSON_REPLY_UNASSIGNED = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"unassigned\"" \
        "INVALID" \
        "}";
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                result = g_json_parse_cb(TEST_JSON_REPLY_UNASSIGNED, g_json_ctx);

                //assert
                ASSERT_IS_NULL(result, "g_json_parse_cb failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_unassigned_succeed)
    {
        PROV_JSON_INFO* result;
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_disabled_mocks();

        //act
        static const char* TEST_JSON_REPLY_UNASSIGNED = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"unassigned\"" \
        "}";
        result = g_json_parse_cb(TEST_JSON_REPLY_UNASSIGNED, g_json_ctx);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(result);
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_parse_json_disabled_succeed)
    {
        PROV_JSON_INFO* result;
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_disabled_mocks();

        //act
        static const char* TEST_JSON_REPLY_DISABLED = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"disabled\"" \
        "}";
        result = g_json_parse_cb(TEST_JSON_REPLY_DISABLED, g_json_ctx);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(result);
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
        static const char* TEST_JSON_REPLY_ASSIGNING = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigning\"" \
        "}";
        result = g_json_parse_cb(TEST_JSON_REPLY_ASSIGNING, g_json_ctx);

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
        static const char* TEST_JSON_REPLY_ERROR = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"failed\"," \
        "    \"registrationState\": {" \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"" \
        "    }" \
        "}";
        result = g_json_parse_cb(TEST_JSON_REPLY_ERROR, g_json_ctx);

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

        setup_parse_json_nohub_mocks(true);

        //act
        static const char* TEST_JSON_REPLY_NO_HUB = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"payload\": { \"json_cust_data\": 123456 }" \
        "    }" \
        "}";

        result = g_json_parse_cb(TEST_JSON_REPLY_NO_HUB, g_json_ctx);

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
        static const char* TEST_JSON_REPLY_ASSIGNING_FAIL = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigning\"" \
        "INVALID" \
        "}";

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                result = g_json_parse_cb(TEST_JSON_REPLY_ASSIGNING_FAIL, g_json_ctx);

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

        setup_parse_json_assigned_mocks(true);

        //act
        result = g_json_parse_cb(TEST_JSON_ASSIGNED_REPLY, g_json_ctx);

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

        setup_parse_json_assigned_mocks(false);

        umock_c_negative_tests_snapshot();

        //act
        static const char* TEST_JSON_ASSIGNED_INVALID_REPLY = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "INVALID" \
        "}";
       
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                result = g_json_parse_cb(TEST_JSON_ASSIGNED_INVALID_REPLY, g_json_ctx);

                //assert
                ASSERT_IS_NULL(result, "g_json_parse_cb failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
        Prov_Device_LL_Destroy(handle);
    }

    /* Tests_SRS_PROV_CLIENT_CLIENT_07_009: [ Upon success Prov_Device_LL_Register_Device shall return PROV_DEVICE_RESULT_OK. ] */
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

    TEST_FUNCTION(Prov_Device_LL_SetOption_TransportOption_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();
        const char* engine = "pkcs11";
        STRICT_EXPECTED_CALL(prov_transport_set_option(IGNORED_PTR_ARG, OPTION_OPENSSL_ENGINE, &engine));

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_SetOption(handle, OPTION_OPENSSL_ENGINE, &engine);

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
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
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

    TEST_FUNCTION(Prov_Device_LL_Register_With_Provisioning_Payload_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        const char* EXPECTED_REQUEST_PAYLOAD = "{\"registrationId\":\"A87FA22F-828B-46CA-BA37-D574C32E423E\","
        "\"tpm\":{\"endorsementKey\":\"keykey\",\"storageRootKey\":\"keykey\"},\"payload\":{\"json_cust_data\":123456}}";

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Set_Provisioning_Payload(handle, TEST_CUSTOM_DATA);
        char* request_payload = g_json_create_cb(TEST_ENDORSMENT_KEY, TEST_ENDORSMENT_KEY, g_json_ctx);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, EXPECTED_REQUEST_PAYLOAD, request_payload);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
        free(request_payload);
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

        setup_parse_json_assigned_mocks(true);
        static const char* TEST_JSON_PAYLOAD_REPLY = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"payload\": { \"json_cust_data\": 123456 }" \
        "    }" \
        "}";

        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_JSON_PAYLOAD_REPLY, g_json_ctx);
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

        setup_parse_json_assigned_mocks(true);
        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_JSON_ASSIGNED_REPLY, g_json_ctx);
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

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_TrustBundle_single_certificate_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true);
        static const char* TEST_TRUST_BUNDLE_REGISTER_RESPONSE_SINGLE_CERT = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"payload\": {}," \
        "        \"trustBundle\": {" \
        "            \"certificates\": [" \
        "                {" \
        "                    \"certificate\": \"-----BEGIN CERTIFICATE-----\\r\\nMIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2\\r\\nOWMxYS04MzlmLTQ2NjgtOGU4Yy04NjhlOGI0MGRkNGMxETAPBgNVBAsMCG1hcm9oZXJhMRcwFQYD\\r\\nVQQKDA5tYXJvaGVyYWxvYW5lcjEOMAwGA1UEBhMFMTYxNDgwHhcNMjEwOTA1MDcxOTQ4WhcNMjEw\\r\\nOTExMDcxOTQ4WjBwMTIwMAYDVQQDDClyb290LTZhNDY5YzFhLTgzOWYtNDY2OC04ZThjLTg2OGU4\\r\\nYjQwZGQ0YzERMA8GA1UECwwIbWFyb2hlcmExFzAVBgNVBAoMDm1hcm9oZXJhbG9hbmVyMQ4wDAYD\\r\\nVQQGEwUxNjE0ODBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOPnvxg7TfTGkfIDyJZgdkAWrhZ4\\r\\nkNCzZoBdujF9BdIe9yQKrM3sskfKBMJTAp0Mx7Hr6PM4qU0UNxispFW6uRyjFjAUMBIGA1UdEwEB\\r\\n/wQIMAYBAf8CAQEwCgYIKoZIzj0EAwIDRwAwRAIgFlF0vGuAIWOVEhZ0jYmA4IO0agbFbq1KOTfN\\r\\nq4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm\\r\\n-----END CERTIFICATE-----\\r\\n\"," \
        "                    \"metadata\": {" \
        "                        \"subjectName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"sha1Thumbprint\": \"5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0\"," \
        "                        \"sha256Thumbprint\": \"1EA3870D991E432B5895FD9C8845D859AFDA933569B7FA5855050CC1C2DFD1E5\"," \
        "                        \"issuerName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"notBeforeUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"notAfterUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"serialNumber\": \"6620A198A418AD45\"," \
        "                        \"version\": 3" \
        "                    }" \
        "                }" \
        "            ]," \
        "            \"id\": \"TestTrustBundle-4e697b1f-1b59-4587-bdc7-e8645eef0a13\"," \
        "            \"createdDateTime\": \"2019-08-24T14:15:22Z\"," \
        "            \"lastModifiedDateTime\": \"2019-08-24T14:15:22Z\"," \
        "             \"etag\": \"\\\"5301e830-0000-0300-0000-613864190000\\\"\"" \
        "        }" \
        "    }" \
        "}";

        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_TRUST_BUNDLE_REGISTER_RESPONSE_SINGLE_CERT, g_json_ctx);
        umock_c_reset_all_calls();

        //act
        const char* trustbundle_json = Prov_Device_LL_Get_Trust_Bundle(handle);

        //assert
        ASSERT_IS_NOT_NULL(trustbundle_json);

        JSON_Value* root_value = NULL;
        JSON_Object* root_obj = NULL;

        JSON_Array* json_ca_certificates = NULL;
        const JSON_Object* json_certificate = NULL;

        const char* etag = NULL;
        int certificate_count = 0;
        const char* pem_certificate = NULL;

        root_value = json_parse_string(trustbundle_json);
        ASSERT_IS_NOT_NULL(root_value);
        
        root_obj = json_value_get_object(root_value);
        ASSERT_IS_NOT_NULL(root_obj);

        etag = json_object_get_string(root_obj, "etag");
        const char* expected = TEST_TRUST_BUNDLE_ETAG;

        ASSERT_ARE_EQUAL(char_ptr, TEST_TRUST_BUNDLE_ETAG, etag);
        
        json_ca_certificates = json_object_get_array(root_obj, "certificates");
        ASSERT_IS_NOT_NULL(json_ca_certificates);
        certificate_count = json_array_get_count(json_ca_certificates);
        ASSERT_ARE_EQUAL(int, 1, certificate_count);

        json_certificate = json_array_get_object(json_ca_certificates, 0);
        ASSERT_IS_NOT_NULL(json_certificate);
        pem_certificate = json_object_get_string(json_certificate, "certificate");
        ASSERT_IS_NOT_NULL(pem_certificate);

        JSON_Value* sn_json = json_object_dotget_value(json_certificate, "metadata.subjectName");
        ASSERT_IS_NOT_NULL(sn_json);
        const char* sn = json_value_get_string(sn_json);
        ASSERT_IS_NOT_NULL(sn);
        size_t sn_len = json_value_get_string_len(sn_json);

        const char* issuer = json_object_dotget_string(json_certificate, "metadata.issuerName");
        ASSERT_IS_NOT_NULL(issuer);

        bool selfsigned_certificate = false;
        if (strncmp(sn, issuer, sn_len) == 0)
        {
            selfsigned_certificate = true;
        }

        ASSERT_IS_TRUE(selfsigned_certificate);

        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(parse_info);
        json_value_free(root_value);
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_TrustBundle_null_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true);
        static const char* TEST_TRUST_BUNDLE_REGISTER_RESPONSE_NULL = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"payload\": {}," \
        "        \"trustBundle\": null" \
        "    }" \
        "}";
        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_TRUST_BUNDLE_REGISTER_RESPONSE_NULL, g_json_ctx);
        umock_c_reset_all_calls();

        //act
        const char* trustbundle_json = Prov_Device_LL_Get_Trust_Bundle(handle);

        //assert
        ASSERT_IS_NOT_NULL(trustbundle_json);

        JSON_Value* root_value = NULL;
        JSON_Object* root_obj = NULL;

        root_value = json_parse_string(trustbundle_json);
        ASSERT_IS_NOT_NULL(root_value);
        
        root_obj = json_value_get_object(root_value);
        ASSERT_IS_NULL(root_obj);

        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(parse_info);
        json_value_free(root_value);
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_TrustBundle_multiple_certificates_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true);

        static const char* TEST_TRUST_BUNDLE_REGISTER_RESPONSE_MULTIPLE_CERTS = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"payload\": {}," \
        "        \"trustBundle\": {" \
        "            \"certificates\": [" \
        "                {" \
        "                    \"certificate\": \"-----BEGIN CERTIFICATE-----\\r\\nMIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2\\r\\nOWMxYS04MzlmLTQ2NjgtOGU4Yy04NjhlOGI0MGRkNGMxETAPBgNVBAsMCG1hcm9oZXJhMRcwFQYD\\r\\nVQQKDA5tYXJvaGVyYWxvYW5lcjEOMAwGA1UEBhMFMTYxNDgwHhcNMjEwOTA1MDcxOTQ4WhcNMjEw\\r\\nOTExMDcxOTQ4WjBwMTIwMAYDVQQDDClyb290LTZhNDY5YzFhLTgzOWYtNDY2OC04ZThjLTg2OGU4\\r\\nYjQwZGQ0YzERMA8GA1UECwwIbWFyb2hlcmExFzAVBgNVBAoMDm1hcm9oZXJhbG9hbmVyMQ4wDAYD\\r\\nVQQGEwUxNjE0ODBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOPnvxg7TfTGkfIDyJZgdkAWrhZ4\\r\\nkNCzZoBdujF9BdIe9yQKrM3sskfKBMJTAp0Mx7Hr6PM4qU0UNxispFW6uRyjFjAUMBIGA1UdEwEB\\r\\n/wQIMAYBAf8CAQEwCgYIKoZIzj0EAwIDRwAwRAIgFlF0vGuAIWOVEhZ0jYmA4IO0agbFbq1KOTfN\\r\\nq4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm\\r\\n-----END CERTIFICATE-----\\r\\n\"," \
        "                    \"metadata\": {" \
        "                        \"subjectName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"sha1Thumbprint\": \"5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0\"," \
        "                        \"sha256Thumbprint\": \"1EA3870D991E432B5895FD9C8845D859AFDA933569B7FA5855050CC1C2DFD1E5\"," \
        "                        \"issuerName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"notBeforeUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"notAfterUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"serialNumber\": \"6620A198A418AD45\"," \
        "                        \"version\": 3" \
        "                    }" \
        "                }," \
        "                {" \
        "                    \"certificate\": \"-----BEGIN CERTIFICATE-----\\r\\nMIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2\\r\\nOWMxYS04MzlmLTQ2NjgtOGU4Yy04NjhlOGI0MGRkNGMxETAPBgNVBAsMCG1hcm9oZXJhMRcwFQYD\\r\\nVQQKDA5tYXJvaGVyYWxvYW5lcjEOMAwGA1UEBhMFMTYxNDgwHhcNMjEwOTA1MDcxOTQ4WhcNMjEw\\r\\nOTExMDcxOTQ4WjBwMTIwMAYDVQQDDClyb290LTZhNDY5YzFhLTgzOWYtNDY2OC04ZThjLTg2OGU4\\r\\nYjQwZGQ0YzERMA8GA1UECwwIbWFyb2hlcmExFzAVBgNVBAoMDm1hcm9oZXJhbG9hbmVyMQ4wDAYD\\r\\nVQQGEwUxNjE0ODBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOPnvxg7TfTGkfIDyJZgdkAWrhZ4\\r\\nkNCzZoBdujF9BdIe9yQKrM3sskfKBMJTAp0Mx7Hr6PM4qU0UNxispFW6uRyjFjAUMBIGA1UdEwEB\\r\\n/wQIMAYBAf8CAQEwCgYIKoZIzj0EAwIDRwAwRAIgFlF0vGuAIWOVEhZ0jYmA4IO0agbFbq1KOTfN\\r\\nq4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm\\r\\n-----END CERTIFICATE-----\\r\\n\"," \
        "                    \"metadata\": {" \
        "                        \"subjectName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"sha1Thumbprint\": \"5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0\"," \
        "                        \"sha256Thumbprint\": \"1EA3870D991E432B5895FD9C8845D859AFDA933569B7FA5855050CC1C2DFD1E5\"," \
        "                        \"issuerName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"notBeforeUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"notAfterUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"serialNumber\": \"6620A198A418AD45\"," \
        "                        \"version\": 3" \
        "                    }" \
        "                }," \
        "                {" \
        "                    \"certificate\": \"-----BEGIN CERTIFICATE-----\\r\\nMIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2\\r\\nOWMxYS04MzlmLTQ2NjgtOGU4Yy04NjhlOGI0MGRkNGMxETAPBgNVBAsMCG1hcm9oZXJhMRcwFQYD\\r\\nVQQKDA5tYXJvaGVyYWxvYW5lcjEOMAwGA1UEBhMFMTYxNDgwHhcNMjEwOTA1MDcxOTQ4WhcNMjEw\\r\\nOTExMDcxOTQ4WjBwMTIwMAYDVQQDDClyb290LTZhNDY5YzFhLTgzOWYtNDY2OC04ZThjLTg2OGU4\\r\\nYjQwZGQ0YzERMA8GA1UECwwIbWFyb2hlcmExFzAVBgNVBAoMDm1hcm9oZXJhbG9hbmVyMQ4wDAYD\\r\\nVQQGEwUxNjE0ODBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOPnvxg7TfTGkfIDyJZgdkAWrhZ4\\r\\nkNCzZoBdujF9BdIe9yQKrM3sskfKBMJTAp0Mx7Hr6PM4qU0UNxispFW6uRyjFjAUMBIGA1UdEwEB\\r\\n/wQIMAYBAf8CAQEwCgYIKoZIzj0EAwIDRwAwRAIgFlF0vGuAIWOVEhZ0jYmA4IO0agbFbq1KOTfN\\r\\nq4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm\\r\\n-----END CERTIFICATE-----\\r\\n\"," \
        "                    \"metadata\": {" \
        "                        \"subjectName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"sha1Thumbprint\": \"5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0\"," \
        "                        \"sha256Thumbprint\": \"1EA3870D991E432B5895FD9C8845D859AFDA933569B7FA5855050CC1C2DFD1E5\"," \
        "                        \"issuerName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"notBeforeUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"notAfterUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"serialNumber\": \"6620A198A418AD45\"," \
        "                        \"version\": 3" \
        "                    }" \
        "                }" \
        "            ]," \
        "            \"id\": \"TestTrustBundle-4e697b1f-1b59-4587-bdc7-e8645eef0a13\"," \
        "            \"createdDateTime\": \"2019-08-24T14:15:22Z\"," \
        "            \"lastModifiedDateTime\": \"2019-08-24T14:15:22Z\"," \
        "             \"etag\": \"\\\"5301e830-0000-0300-0000-613864190000\\\"\"" \
        "        }" \
        "    }" \
        "}";
        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_TRUST_BUNDLE_REGISTER_RESPONSE_MULTIPLE_CERTS, g_json_ctx);
        umock_c_reset_all_calls();

        //act
        const char* trustbundle_json = Prov_Device_LL_Get_Trust_Bundle(handle);

        //assert
        ASSERT_IS_NOT_NULL(trustbundle_json);

        JSON_Value* root_value = NULL;
        JSON_Object* root_obj = NULL;

        JSON_Array* json_ca_certificates = NULL;
        const JSON_Object* json_certificate = NULL;

        const char* etag = NULL;
        int certificate_count = 0;
        const char* pem_certificate = NULL;

        root_value = json_parse_string(trustbundle_json);
        ASSERT_IS_NOT_NULL(root_value);
        
        root_obj = json_value_get_object(root_value);
        ASSERT_IS_NOT_NULL(root_obj);

        etag = json_object_get_string(root_obj, "etag");
        const char* expected = TEST_TRUST_BUNDLE_ETAG;

        ASSERT_ARE_EQUAL(char_ptr, TEST_TRUST_BUNDLE_ETAG, etag);

        json_ca_certificates = json_object_get_array(root_obj, "certificates");
        ASSERT_IS_NOT_NULL(json_ca_certificates);
        certificate_count = json_array_get_count(json_ca_certificates);
        ASSERT_ARE_EQUAL(int, 3, certificate_count);

        for (int i = 0; i < certificate_count; i++)
        {
            json_certificate = json_array_get_object(json_ca_certificates, i);
            ASSERT_IS_NOT_NULL(json_certificate);
            pem_certificate = json_object_get_string(json_certificate, "certificate");
            ASSERT_IS_NOT_NULL(pem_certificate);

            JSON_Value* sn_json = json_object_dotget_value(json_certificate, "metadata.subjectName");
            ASSERT_IS_NOT_NULL(sn_json);
            const char* sn = json_value_get_string(sn_json);
            ASSERT_IS_NOT_NULL(sn);
            size_t sn_len = json_value_get_string_len(sn_json);

            const char* issuer = json_object_dotget_string(json_certificate, "metadata.issuerName");
            ASSERT_IS_NOT_NULL(issuer);

            bool selfsigned_certificate = false;
            if (strncmp(sn, issuer, sn_len) == 0)
            {
                selfsigned_certificate = true;
            }

            ASSERT_IS_TRUE(selfsigned_certificate);
        }

        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(parse_info);
        json_value_free(root_value);
        Prov_Device_LL_Destroy(handle);
    }

    
    TEST_FUNCTION(Prov_Device_LL_Set_Provisioning_CSR_handle_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Set_Certificate_Signing_Request(NULL, TEST_CERTIFICATE_VAL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_Set_Provisioning_CSR_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERTIFICATE_VAL));

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Set_Certificate_Signing_Request(handle, TEST_CERTIFICATE_VAL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Register_With_Provisioning_CSR_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        const char* EXPECTED_REQUEST_PAYLOAD = "{\"registrationId\":\"A87FA22F-828B-46CA-BA37-D574C32E423E\"," \
        "\"tpm\":{\"endorsementKey\":\"keykey\",\"storageRootKey\":\"keykey\"},\"clientCertificateCsr\":""\"--BEGIN_CERT 12345 END_CERT--\"}";

        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Set_Certificate_Signing_Request(handle, TEST_CERTIFICATE_VAL);
        char* request_payload = g_json_create_cb(TEST_ENDORSMENT_KEY, TEST_ENDORSMENT_KEY, g_json_ctx);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, EXPECTED_REQUEST_PAYLOAD, request_payload);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
        free(request_payload);
    }

    TEST_FUNCTION(Prov_Device_LL_Set_Provisioning_CSR_set_twice_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERTIFICATE_VAL));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERTIFICATE_VAL));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Set_Certificate_Signing_Request(handle, TEST_CERTIFICATE_VAL);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        prov_result = Prov_Device_LL_Set_Certificate_Signing_Request(handle, TEST_CERTIFICATE_VAL);

        //assert
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_CSR_handle_NULL_success)
    {
        //arrange

        //act
        const char* result = Prov_Device_LL_Get_Issued_Client_Certificate(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_CSR_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true);
        static const char* TEST_JSON_CSR_REPLY = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"issuedClientCertificate\": \"--BEGIN_CERT 12345 END_CERT--\"" \
        "    }" \
        "}";

        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_JSON_CSR_REPLY, g_json_ctx);
        umock_c_reset_all_calls();

        //act
        const char* result = Prov_Device_LL_Get_Issued_Client_Certificate(handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, TEST_CERTIFICATE_VAL, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(parse_info);
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_CSR_no_data_success)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true);
                static const char* TEST_JSON_CSR_REPLY = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"issuedClientCertificate\": \"\"" \
        "    }" \
        "}";

        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_JSON_CSR_REPLY, g_json_ctx);
        umock_c_reset_all_calls();

        //act
        const char* result = Prov_Device_LL_Get_Issued_Client_Certificate(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, "", result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(parse_info);
        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(Prov_Device_LL_Get_Provisioning_CSR_data_wrong_type_fails)
    {
        //arrange
        PROV_DEVICE_LL_HANDLE handle = Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, trans_provider);
        (void)Prov_Device_LL_Register_Device(handle, on_prov_register_device_callback, NULL, on_prov_register_status_callback, NULL);
        g_status_callback(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, DEFAULT_RETRY_AFTER, g_status_ctx);
        Prov_Device_LL_DoWork(handle);
        umock_c_reset_all_calls();

        setup_parse_json_assigned_mocks(true);
                static const char* TEST_JSON_CSR_REPLY = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"issuedClientCertificate\": 5" \
        "    }" \
        "}";

        PROV_JSON_INFO* parse_info = g_json_parse_cb(TEST_JSON_CSR_REPLY, g_json_ctx);
        umock_c_reset_all_calls();

        //act
        const char* result = Prov_Device_LL_Get_Issued_Client_Certificate(handle);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        free_prov_json_info(parse_info);
        Prov_Device_LL_Destroy(handle);
    }

END_TEST_SUITE(prov_device_client_ll_ut)
