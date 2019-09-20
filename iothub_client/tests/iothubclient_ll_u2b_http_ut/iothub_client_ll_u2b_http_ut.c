// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#include <stdbool.h>
#endif

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

static void* my_gballoc_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"

#include "iothub_client_options.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/platform.h"

#include "internal/blob.h"
#include "internal/iothub_client_authorization.h"
#include "azure_uhttp_c/uhttp.h"

#include "parson.h"

#include "azure_c_shared_utility/gballoc.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...);
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

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

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);

#undef ENABLE_MOCKS

#include "internal/iothub_client_ll_uploadtoblob.h"

#define BLOCK_SIZE (4*1024*1024)

#define TEST_DEVICE_ID "theidofTheDevice"
#define TEST_DEVICE_KEY "theKeyoftheDevice"
#define TEST_DEVICE_SAS     "theSasOfTheDevice"
#define TEST_DEVICE_SAS_FAIL "fail_theSasOfTheDevice"
#define TEST_IOTHUBNAME "theNameoftheIotHub"
#define TEST_IOTHUBSUFFIX "theSuffixoftheIotHubHostname"
#define TEST_AUTHORIZATIONKEY "theAuthorizationKey"

#define TEST_STRING_HANDLE_DEVICE_ID ((STRING_HANDLE)0x1)
#define TEST_STRING_HANDLE_DEVICE_SAS ((STRING_HANDLE)0x2)

/*#define TEST_API_VERSION "?api-version=2016-11-14"
#define TEST_IOTHUB_SDK_VERSION "1.3.3"*/

static const char* const testUploadtrustedCertificates = "some certificates";
static const char* const TEST_SAS_TOKEN = "test_sas_token";
static const char* const TEST_PRIVATE = "test_private";
static const char* const TEST_CERT = "test_cert";

static const char* const TEST_DESTINASTION_FILENAME = "test/filename.txt";
static const unsigned char* TEST_SOURCE = (const unsigned char*)0x3;
static const size_t TEST_SOURCE_LENGTH = 3;
static IO_INTERFACE_DESCRIPTION* TEST_INTERFACE_DESC = (IO_INTERFACE_DESCRIPTION*)0x4;
static XIO_HANDLE TEST_XIO_HANDLE = (XIO_HANDLE)0x5;

static ON_HTTP_REQUEST_CALLBACK g_on_request_callback = NULL;
static void* g_on_request_callback_ctx = NULL;
static bool g_call_http_reply = false;

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_construct_n(const char* psz, size_t n)
{
    (void)psz;
    (void)n;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_new(void)
{
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_from_byte_array(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    my_gballoc_free(handle);
}

static STRING_HANDLE my_URL_EncodeString(const char* textEncode)
{
(void)textEncode;
return (STRING_HANDLE)my_gballoc_malloc(1);
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE h)
{
    my_gballoc_free(h);
}

static int my_BUFFER_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size)
{
    (void)handle;
    (void)source;
    (void)size;
    return 0;
}

static BUFFER_HANDLE my_BUFFER_new(void)
{
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_BUFFER_create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    my_gballoc_free(handle);
}

static JSON_Value * my_json_parse_string(const char *string)
{
    (void)string;
    return (JSON_Value *)my_gballoc_malloc(1);
}

static void my_json_value_free(JSON_Value *value)
{
    my_gballoc_free(value);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    size_t l = strlen(source);
    *destination = (char*)malloc(l + 1);
    (void)memcpy(*destination, source, l + 1);
    return 0;
}

static char* my_IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, size_t expiry_time_relative_seconds, const char* key_name)
{
    (void)handle;
    (void)scope;
    (void)expiry_time_relative_seconds;
    (void)key_name;
    size_t l = strlen(TEST_SAS_TOKEN);
    char* result = (char*)my_gballoc_malloc(l + 1);
    strcpy(result, TEST_SAS_TOKEN);
    return result;
}

static int my_IoTHubClient_Auth_Get_x509_info(IOTHUB_AUTHORIZATION_HANDLE handle, char** x509_cert, char** x509_key)
{
    (void)handle;

    size_t l = strlen(TEST_PRIVATE);
    *x509_key = (char*)my_gballoc_malloc(l + 1);
    strcpy(*x509_key, TEST_PRIVATE);

    l = strlen(TEST_CERT);
    *x509_cert = (char*)my_gballoc_malloc(l + 1);
    strcpy(*x509_cert, TEST_CERT);

    return 0;
}

static HTTP_CLIENT_HANDLE my_uhttp_client_create(const IO_INTERFACE_DESCRIPTION* io_interface_desc, const void* xio_param, ON_HTTP_ERROR_CALLBACK on_http_error, void* callback_ctx)
{
    (void)io_interface_desc;
    (void)xio_param;
    (void)on_http_error;
    (void)callback_ctx;
    return (HTTP_CLIENT_HANDLE)my_gballoc_malloc(1);
}

static void my_uhttp_client_destroy(HTTP_CLIENT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static void my_uhttp_client_close(HTTP_CLIENT_HANDLE handle, ON_HTTP_CLOSED_CALLBACK on_close_callback, void* callback_ctx)
{
    (void)handle;
    on_close_callback(callback_ctx);
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
    g_on_request_callback = on_request_callback;
    g_on_request_callback_ctx = callback_ctx;

    g_call_http_reply = true;
    return HTTP_CLIENT_OK;
}

static void my_uhttp_client_dowork(HTTP_CLIENT_HANDLE handle)
{
    (void)handle;
    if (g_call_http_reply)
    {
        g_on_request_callback(g_on_request_callback_ctx, HTTP_CALLBACK_REASON_OK, TEST_SOURCE, TEST_SOURCE_LENGTH, 201, NULL);
        g_call_http_reply = false;
    }
}

static TICK_COUNTER_HANDLE my_tickcounter_create(void)
{
    return (TICK_COUNTER_HANDLE)my_gballoc_malloc(1);
}

static void my_tickcounter_destroy(TICK_COUNTER_HANDLE handle)
{
    my_gballoc_free(handle);
}

/**
 * BLOB_UPLOAD_CONTEXT and FileUpload_GetData_Callback
 * allow to simulate a user who wants to upload
 * a source of size "size".
 * It also allows to access the latest values of result, data and size, for testing purpose
 */
typedef struct BLOB_UPLOAD_CONTEXT_TAG
{
    const unsigned char* source; /* source to upload */
    size_t size; /* size of the source */
    size_t toUpload; /* size not yet uploaded */
    IOTHUB_CLIENT_FILE_UPLOAD_RESULT lastResult; /* last result received */
    unsigned char const ** lastData;
    size_t* lastSize;
}BLOB_UPLOAD_CONTEXT;

BLOB_UPLOAD_CONTEXT context;

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT FileUpload_GetData_Callback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* _uploadContext)
{
    BLOB_UPLOAD_CONTEXT* uploadContext = (BLOB_UPLOAD_CONTEXT*) _uploadContext;

    uploadContext->lastResult = result;
    uploadContext->lastData = data;
    uploadContext->lastSize = size;

    if (result == FILE_UPLOAD_OK)
    {
        if (data != NULL && size != NULL)
        {
            // Upload next block
            size_t thisBlockSize = (uploadContext->toUpload > BLOCK_SIZE) ? BLOCK_SIZE : uploadContext->toUpload;
            *data = (unsigned char*)uploadContext->source;
            *size = thisBlockSize;
            uploadContext->toUpload -= thisBlockSize;
        }
        else
        {
            // Last call failed
            *data = NULL;
            *size = 0;
        }
    }
    else
    {
        // Check failure
    }
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

TEST_DEFINE_ENUM_TYPE       (IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE       (BLOB_RESULT, BLOB_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (BLOB_RESULT, BLOB_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE       (IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE       (IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE       (HTTP_CLIENT_REQUEST_TYPE, HTTP_CLIENT_REQUEST_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (HTTP_CLIENT_REQUEST_TYPE, HTTP_CLIENT_REQUEST_TYPE_VALUES);

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}
static const TRANSPORT_PROVIDER* provideFAKE(void);

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_SAS =
{
    provideFAKE,            /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    NULL,                   /* const char* deviceKey;                       */
    TEST_DEVICE_SAS,        /* const char* deviceSasToken;                  */
    TEST_IOTHUBNAME,        /* const char* iotHubName;                      */
    TEST_IOTHUBSUFFIX,      /* const char* iotHubSuffix;                    */
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_DEVICE_KEY =
{
    provideFAKE,            /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    TEST_DEVICE_KEY,        /* const char* deviceKey;                       */
    NULL,                   /* const char* deviceSasToken;                  */
    TEST_IOTHUBNAME,        /* const char* iotHubName;                      */
    TEST_IOTHUBSUFFIX,      /* const char* iotHubSuffix;                    */
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_X509 =
{
    provideFAKE,            /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    NULL,                   /* const char* deviceKey;                       */
    NULL,                   /* const char* deviceSasToken;                  */
    TEST_IOTHUBNAME,        /* const char* iotHubName;                      */
    TEST_IOTHUBSUFFIX,      /* const char* iotHubSuffix;                    */
};

static const TRANSPORT_PROVIDER* provideFAKE(void)
{
    return NULL;
}

static const unsigned int TwoHundred = 200;
static const unsigned int FourHundred = 400;

static unsigned char TestValid_BUFFER_u_char[] = { '3', '\0' };

static char TEST_DEFAULT_STRING_VALUE[2] = { '3', '\0' };

static IOTHUB_AUTHORIZATION_HANDLE TEST_AUTH_HANDLE = (IOTHUB_AUTHORIZATION_HANDLE)0x123456;

BEGIN_TEST_SUITE(iothubclient_ll_u2b_http_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    umocktypes_charptr_register_types();

    REGISTER_TYPE(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE);
    REGISTER_TYPE(BLOB_RESULT, BLOB_RESULT);
    REGISTER_TYPE(HTTP_CLIENT_REQUEST_TYPE, HTTP_CLIENT_REQUEST_TYPE);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(char **, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const unsigned char*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_ERROR_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_OPEN_COMPLETE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_REQUEST_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_CLOSED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);

    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_INTERFACE_DESC);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(platform_get_default_tlsio, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_create, my_uhttp_client_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_destroy, my_uhttp_client_destroy);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_trusted_cert, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_trusted_cert, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_dowork, my_uhttp_client_dowork);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_get_underlying_xio, TEST_XIO_HANDLE);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_close, my_uhttp_client_close);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_execute_request, my_uhttp_client_execute_request);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_execute_request, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Set_xio_Certificate, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Set_xio_Certificate, __LINE__);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_trace, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_trace, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_open, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_open, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct_n, my_STRING_construct_n);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_from_byte_array, my_STRING_from_byte_array);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_DEFAULT_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, TEST_DEFAULT_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat_with_STRING, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_copy, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
    REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, (time_t)123456);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(get_time, (time_t)-1);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_length, 3);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_create, my_tickcounter_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_destroy, my_tickcounter_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_build, my_BUFFER_build);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_build, 1);

    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, my_json_parse_string);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, (JSON_Object*)1);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, "a");
    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, my_json_value_free);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_SasToken, my_IoTHubClient_Auth_Get_SasToken);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_SasToken, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_x509_info, my_IoTHubClient_Auth_Get_x509_info);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_x509_info, __LINE__);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceKey, TEST_DEVICE_ID);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceId, TEST_DEVICE_ID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_DeviceId, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(Blob_UploadMultipleBlocksFromSasUri, BLOB_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Blob_UploadMultipleBlocksFromSasUri, BLOB_ERROR);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
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

    umock_c_reset_all_calls();
    g_on_request_callback = NULL;
    g_on_request_callback_ctx = NULL;
    g_call_http_reply = false;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

static void setup_http_reply_recv_mocks(void)
{
    STRICT_EXPECTED_CALL(BUFFER_unbuild(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

}

static void setup_send_http_request_mocks(void)
{
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(uhttp_client_execute_request(IGNORED_PTR_ARG, HTTP_CLIENT_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG));

    setup_http_reply_recv_mocks();

    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(TEST_SOURCE_LENGTH);
}

static void setup_create_http_client_mocks(bool use_proxy, bool use_cert, IOTHUB_CREDENTIAL_TYPE cred_type)
{
    if (use_proxy)
    {

    }

    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(uhttp_client_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (use_cert)
    {
        STRICT_EXPECTED_CALL(uhttp_client_set_trusted_cert(IGNORED_PTR_ARG, TEST_CERT));
    }
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC || cred_type == IOTHUB_CREDENTIAL_TYPE_X509)
    {
        STRICT_EXPECTED_CALL(uhttp_client_get_underlying_xio(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Set_xio_Certificate(TEST_AUTH_HANDLE, IGNORED_PTR_ARG));
    }
    //STRICT_EXPECTED_CALL(uhttp_client_set_trace(IGNORED_PTR_ARG, true, true));
    STRICT_EXPECTED_CALL(uhttp_client_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 443, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void setup_http_header_mocks(IOTHUB_CREDENTIAL_TYPE cred_type)
{
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_X509 || cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void setup_parse_result_json_mocks(void)
{
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_copy(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
}

static void setup_close_http_client(void)
{
    STRICT_EXPECTED_CALL(uhttp_client_close(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void setup_step3_mocks(IOTHUB_CREDENTIAL_TYPE cred_type)
{
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(uhttp_client_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 443, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(TEST_AUTH_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        setup_send_http_request_mocks();

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        setup_send_http_request_mocks();
    }
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setup_initiate_multiple_blob_upload_mocks(bool use_proxy, bool use_cert, IOTHUB_CREDENTIAL_TYPE cred_type)
{
    setup_create_http_client_mocks(use_proxy, use_cert, cred_type);

    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    setup_http_header_mocks(cred_type);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    setup_send_http_request_mocks();

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();

    setup_parse_result_json_mocks();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    setup_close_http_client();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(Blob_UploadMultipleBlocksFromSasUri(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();

    // Upload step 3
    setup_step3_mocks(cred_type);

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_destroy(IGNORED_PTR_ARG));
}

static void setup_initiate_blob_upload_mocks(bool use_proxy, bool use_cert, IOTHUB_CREDENTIAL_TYPE cred_type)
{
    setup_create_http_client_mocks(use_proxy, use_cert, cred_type);

    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    setup_http_header_mocks(cred_type);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    setup_send_http_request_mocks();

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();

    setup_parse_result_json_mocks();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    setup_close_http_client();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(Blob_UploadMultipleBlocksFromSasUri(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();

    // Upload step 3
    setup_step3_mocks(cred_type);

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_destroy(IGNORED_PTR_ARG));
}

static void setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE cred_type)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceId(TEST_AUTH_HANDLE));
    STRICT_EXPECTED_CALL(tickcounter_create());
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(TEST_AUTH_HANDLE))
        .SetReturn(cred_type)
        .CallCannotFail();
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_x509_info(TEST_AUTH_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else if (cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(TEST_AUTH_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    }
    else if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(TEST_AUTH_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_auth_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, NULL);

    //assert
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_config_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(NULL, TEST_AUTH_HANDLE);

    //assert
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_sas_token_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_x509_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509);

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_ecc_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509_ECC);

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_device_key_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CREDENTIAL_TYPE cred_type_list[] = {
        IOTHUB_CREDENTIAL_TYPE_UNKNOWN,
        IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY,
        IOTHUB_CREDENTIAL_TYPE_X509,
        IOTHUB_CREDENTIAL_TYPE_X509_ECC,
        IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN,
        IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH
    };

    size_t type_count = sizeof(cred_type_list)/sizeof(cred_type_list[0]);
    for (size_t type_index = 0; type_index < type_count; type_index++)
    {
        umock_c_reset_all_calls();
        setup_uploadtoblob_create_mocks(cred_type_list[type_index]);

        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                //act
                IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

                //assert
                ASSERT_IS_NULL(handle, "IoTHubClient_LL_UploadToBlob_Impl failure in test %lu/%lu with cred type %lu", (unsigned long)index, (unsigned long)count, (unsigned long)type_index);
            }
        }
    }
    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Destroy_handle_NULL_success)
{
    //arrange

    //act
    IoTHubClient_LL_UploadToBlob_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Destroy_x509_success)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClient_LL_UploadToBlob_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_handle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(NULL, TEST_DESTINASTION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_destination_filename_NULL_fail)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(handle, NULL, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_destination_source_NULL_size_valid_fail)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(handle, TEST_DESTINASTION_FILENAME, NULL, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_success)
{
    //arrange
    IOTHUB_CREDENTIAL_TYPE cred_type_list[] = {
        IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN,
        IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY,
        IOTHUB_CREDENTIAL_TYPE_X509,
        IOTHUB_CREDENTIAL_TYPE_X509_ECC,
        IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH
    };

    size_t type_count = sizeof(cred_type_list) / sizeof(cred_type_list[0]);
    for (size_t type_index = 0; type_index < type_count; type_index++)
    {
        setup_uploadtoblob_create_mocks(cred_type_list[type_index]);
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
        umock_c_reset_all_calls();

        setup_initiate_blob_upload_mocks(false, false, cred_type_list[type_index]);

        //act
        IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(handle, TEST_DESTINASTION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

        //assert
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHubClient_LL_UploadToBlob_Impl failure in test cred type %s", MU_ENUM_TO_STRING(IOTHUB_CREDENTIAL_TYPE, cred_type_list[type_index]));
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubClient_LL_UploadToBlob_Destroy(handle);
        umock_c_reset_all_calls();
    }
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CREDENTIAL_TYPE cred_type_list[] = {
        IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN//, 
        //IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY
        //IOTHUB_CREDENTIAL_TYPE_X509,
        //IOTHUB_CREDENTIAL_TYPE_X509_ECC,
        //IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH
    };

    size_t type_count = sizeof(cred_type_list) / sizeof(cred_type_list[0]);
    for (size_t type_index = 0; type_index < type_count; type_index++)
    {
        umock_c_reset_all_calls();
        setup_uploadtoblob_create_mocks(cred_type_list[type_index]);
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
        umock_c_reset_all_calls();

        setup_initiate_blob_upload_mocks(false, false, cred_type_list[type_index]);

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(handle, TEST_DESTINASTION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

                //assert
                ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHubClient_LL_UploadToBlob_Impl failure in test %lu/%lu with cred type %s", (unsigned long)index, (unsigned long)count, MU_ENUM_TO_STRING(IOTHUB_CREDENTIAL_TYPE, cred_type_list[type_index]));
            }
        }

        //cleanup
        IoTHubClient_LL_UploadToBlob_Destroy(handle);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl_success)
{
    //arrange
    IOTHUB_CREDENTIAL_TYPE cred_type_list[] = {
        IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN/*,
        IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY,
        IOTHUB_CREDENTIAL_TYPE_X509,
        IOTHUB_CREDENTIAL_TYPE_X509_ECC,
        IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH*/
    };

    size_t type_count = sizeof(cred_type_list) / sizeof(cred_type_list[0]);
    for (size_t type_index = 0; type_index < type_count; type_index++)
    {
        BLOB_UPLOAD_CONTEXT blob_ctx = { 0 };
        blob_ctx.toUpload = 1024;
        blob_ctx.source = (unsigned char*)0x2345;

        setup_uploadtoblob_create_mocks(cred_type_list[type_index]);
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
        umock_c_reset_all_calls();

        setup_initiate_multiple_blob_upload_mocks(false, false, cred_type_list[type_index]);

        //act
        IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl(handle, TEST_DESTINASTION_FILENAME, FileUpload_GetData_Callback, &blob_ctx);

        //assert
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHubClient_LL_UploadToBlob_Impl failure in test cred type %s", MU_ENUM_TO_STRING(IOTHUB_CREDENTIAL_TYPE, cred_type_list[type_index]));
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubClient_LL_UploadToBlob_Destroy(handle);
        umock_c_reset_all_calls();
    }
}

END_TEST_SUITE(iothubclient_ll_u2b_http_ut)
