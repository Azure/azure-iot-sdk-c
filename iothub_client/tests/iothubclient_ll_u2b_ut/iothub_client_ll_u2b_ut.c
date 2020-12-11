// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

static void* my_gballoc_malloc(size_t size)
{
    void *result = malloc(size);
    return result;
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

static void* my_gballoc_calloc(size_t nmemb, size_t size)
{
    void *result = calloc(nmemb, size);
    return result;
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
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "internal/blob.h"
#include "internal/iothub_client_authorization.h"

#include "parson.h"

#define BLOCK_SIZE (4*1024*1024)

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);

#define TEST_DEVICE_ID "theidofTheDevice"
#define TEST_DEVICE_KEY "theKeyoftheDevice"
#define TEST_DEVICE_SAS     "theSasOfTheDevice"
#define TEST_DEVICE_SAS_FAIL "fail_theSasOfTheDevice"
#define TEST_IOTHUBNAME "theNameoftheIotHub"
#define TEST_IOTHUBSUFFIX "theSuffixoftheIotHubHostname"
#define TEST_AUTHORIZATIONKEY "theAuthorizationKey"

#define TEST_STRING_HANDLE_DEVICE_ID ((STRING_HANDLE)0x1)
#define TEST_STRING_HANDLE_DEVICE_SAS ((STRING_HANDLE)0x2)

#define TEST_API_VERSION "?api-version=2016-11-14"
#define TEST_IOTHUB_SDK_VERSION "1.4.1"

static const char* const testUploadtrustedCertificates = "some certificates";
static const char* const TEST_SAS_TOKEN = "test_sas_token";
static const char* const TEST_PRIVATE = "test_private";
static const char* const TEST_CERT = "test_cert";

static const unsigned char* TEST_SOURCE = (const unsigned char*)0x3;
static const size_t TEST_SOURCE_LENGTH = 3;
static const char* const TEST_DESTINATION_FILENAME = "text.txt";

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
        return (STRING_HANDLE)my_gballoc_malloc(1);
    }

#ifdef __cplusplus
    }
#endif

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

static HTTPAPIEX_HANDLE my_HTTPAPIEX_Create(const char* hostName)
{
    (void)hostName;
    return (HTTPAPIEX_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPAPIEX_Destroy(HTTPAPIEX_HANDLE handle)
{
    my_gballoc_free(handle);
}

static HTTPAPIEX_SAS_HANDLE my_HTTPAPIEX_SAS_Create(STRING_HANDLE key, STRING_HANDLE uriResource, STRING_HANDLE keyName)
{
    (void)key;
    (void)uriResource;
    (void)keyName;
    return (HTTPAPIEX_SAS_HANDLE)my_gballoc_malloc(1);
}

static HTTPAPIEX_SAS_HANDLE my_HTTPAPIEX_SAS_Create_From_String(const char* key, const char* uriResource, const char* keyName)
{
    (void)key;
    (void)uriResource;
    (void)keyName;
    return (HTTPAPIEX_SAS_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPAPIEX_SAS_Destroy(HTTPAPIEX_SAS_HANDLE handle)
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

static HTTPAPIEX_RESULT my_HTTPAPIEX_ExecuteRequest(HTTPAPIEX_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
    HTTP_HEADERS_HANDLE requestHttpHeadersHandle, BUFFER_HANDLE requestContent, unsigned int* statusCode,
    HTTP_HEADERS_HANDLE responseHttpHeadersHandle, BUFFER_HANDLE responseContent)
{
    (void)handle;
    (void)requestType;
    (void)relativePath;
    (void)requestHttpHeadersHandle;
    (void)requestContent;
    (void)responseHttpHeadersHandle;
    (void)responseContent;
    if (statusCode != NULL)
    {
        *statusCode = 200; /*success*/
    }
    return HTTPAPIEX_OK;
}

static HTTPAPIEX_RESULT my_HTTPAPIEX_SAS_ExecuteRequest(HTTPAPIEX_SAS_HANDLE sasHandle, HTTPAPIEX_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath, HTTP_HEADERS_HANDLE requestHttpHeadersHandle, BUFFER_HANDLE requestContent, unsigned int* statusCode, HTTP_HEADERS_HANDLE responseHeadersHandle, BUFFER_HANDLE responseContent)
{
    (void)sasHandle;
    (void)handle;
    (void)requestType;
    (void)relativePath;
    (void)requestHttpHeadersHandle;
    (void)requestContent;
    (void)responseHeadersHandle;
    (void)responseContent;
    if (statusCode != NULL)
    {
        *statusCode = 200;/*success*/
    }
    return HTTPAPIEX_OK;
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    size_t l = strlen(source);
    *destination = (char*)malloc(l + 1);
    (void)memcpy(*destination, source, l+1);
    return 0;
}

static char* my_IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, size_t expiry_time_relative_seconds, const char* key_name)
{
    (void)handle;
    (void)scope;
    (void)expiry_time_relative_seconds;
    (void)key_name;
    size_t l = strlen(TEST_SAS_TOKEN);
    char* result = (char*)my_gballoc_malloc(l+1);
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
    if (data == NULL || size == NULL)
    {
        // This is the last call
    }
    else if (result != FILE_UPLOAD_OK)
    {
        // Last call failed
        *data = NULL;
        *size = 0;
    }
    else if (uploadContext->toUpload == 0)
    {
        // Everything has been uploaded
        *data = NULL;
        *size = 0;
    }
    else
    {
        // Upload next block
        size_t thisBlockSize = (uploadContext->toUpload > BLOCK_SIZE) ? BLOCK_SIZE : uploadContext->toUpload;
        *data = (unsigned char*)uploadContext->source + (uploadContext->size - uploadContext->toUpload);
        *size = thisBlockSize;
        uploadContext->toUpload -= thisBlockSize;
    }

    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

#include "azure_c_shared_utility/gballoc.h"

#undef ENABLE_MOCKS

#include "internal/iothub_client_ll_uploadtoblob.h"

TEST_DEFINE_ENUM_TYPE       (HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE       (HTTPAPIEX_RESULT, HTTPAPIEX_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (HTTPAPIEX_RESULT, HTTPAPIEX_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE       (HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE       (HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE       (IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE       (BLOB_RESULT, BLOB_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (BLOB_RESULT, BLOB_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE       (IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE       (IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);

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

// We store many return values during run of UploadToBlob UT to make sure they're processed correctly later.
// We need these to exist outside the scope of setup_upload_to_blob_happypath, which is deleted prior to invoking UT itself.
typedef struct UPLOADTOBLOB_TEST_UT_CONTEXT_TAG
{
    int i;
    STRING_HANDLE correlationId;
    STRING_HANDLE sasUri;
    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRING_HANDLE iotHubHttpRelativePath1;
    BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
    STRING_HANDLE blobJson;
    BUFFER_HANDLE jsonBuffer;
    const char* sasUri_as_const_char;
    STRING_HANDLE uriResource;
    const char* json_hostName;
    const char* json_containerName;
    const char* json_blobName;
    const char* json_sasToken;
    STRING_HANDLE relativePathNotification;
    const char* correlationId_as_char;
    const char* relativePathNotification_as_char;
    unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char;
    size_t iotHubHttpMessageBodyResponse1_size;
    const char* iotHubHttpRelativePath1_as_const_char;
    STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
    const char* iotHubHttpMessageBodyResponse1_as_const_char;
    JSON_Value* allJson;
    JSON_Object* jsonObject;
    const char* json_correlationId;
} UPLOADTOBLOB_TEST_UT_CONTEXT;

static UPLOADTOBLOB_TEST_UT_CONTEXT uploadtoblobTest_Context;

BEGIN_TEST_SUITE(iothubclient_ll_uploadtoblob_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    umocktypes_charptr_register_types();

    REGISTER_TYPE(HTTPAPI_RESULT, HTTPAPI_RESULT);
    REGISTER_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT);
    REGISTER_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT);
    REGISTER_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE);
    REGISTER_TYPE(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE);
    REGISTER_TYPE(BLOB_RESULT, BLOB_RESULT);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(char **, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_SAS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const unsigned char*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);

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

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Create, my_HTTPAPIEX_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Destroy, my_HTTPAPIEX_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_Create, my_HTTPAPIEX_SAS_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_SAS_ExecuteRequest, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_ExecuteRequest, my_HTTPAPIEX_SAS_ExecuteRequest)
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_Destroy, my_HTTPAPIEX_SAS_Destroy);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_Create_From_String, my_HTTPAPIEX_SAS_Create_From_String);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);

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
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_ExecuteRequest, my_HTTPAPIEX_ExecuteRequest)
    REGISTER_GLOBAL_MOCK_RETURN(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_OK);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(HTTPAPIEX_SetOption, HTTPAPIEX_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_SetOption, HTTPAPIEX_ERROR);

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

static void reset_test_data()
{
    memset(&context, 0, sizeof(context));
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    reset_test_data();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

static void setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE cred_type)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceId(TEST_AUTH_HANDLE));
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
}

static void setup_steps_1_and_2_mocks(IOTHUB_CREDENTIAL_TYPE cred_type)
{
    int status_code = 200;

    // Step 1
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_new());

    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); // 11
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC || cred_type == IOTHUB_CREDENTIAL_TYPE_X509)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&status_code, sizeof(status_code));
    }
    else if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(TEST_AUTH_HANDLE)).CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create_From_String(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(IGNORED_PTR_ARG, IGNORED_PTR_ARG, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&status_code, sizeof(status_code));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(get_time(NULL)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(TEST_AUTH_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&status_code, sizeof(status_code));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else if (cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&status_code, sizeof(status_code));
    }
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();

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

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setup_steps_3(IOTHUB_CREDENTIAL_TYPE cred_type)
{
    int status_code = 200;

    if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(TEST_AUTH_HANDLE)).CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create_From_String(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(IGNORED_PTR_ARG, IGNORED_PTR_ARG, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&status_code, sizeof(status_code));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_POST, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&status_code, sizeof(status_code));
    }
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setup_Blob_UploadMultipleBlocksFromSasUri_mocks(IOTHUB_CREDENTIAL_TYPE cred_type, BLOB_RESULT blob_result, bool null_buffer)
{
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();

    unsigned int status_code;
    if (BLOB_OK != blob_result)
    {
        status_code = 404;
        STRICT_EXPECTED_CALL(Blob_UploadMultipleBlocksFromSasUri(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_httpStatus(&status_code, sizeof(status_code))
            .SetReturn(blob_result);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        setup_steps_3(cred_type);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else
    {
        status_code = 200;
        STRICT_EXPECTED_CALL(Blob_UploadMultipleBlocksFromSasUri(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_httpStatus(&status_code, sizeof(status_code)).CallCannotFail();

        if (null_buffer)
        {
            STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).SetReturn(NULL);
        }
        else
        {
            STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();
        }
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        setup_steps_3(cred_type);
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
}

static void setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE cred_type, bool proxy, bool set_timeout, bool trusted_cert, BLOB_RESULT blob_result, bool null_buffer)
{
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG));
    if (set_timeout)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_HTTP_TIMEOUT, IGNORED_PTR_ARG));
    }
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC || cred_type == IOTHUB_CREDENTIAL_TYPE_X509)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_X509_CERT, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_X509_PRIVATE_KEY, IGNORED_PTR_ARG));
    }
    if (trusted_cert)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_TRUSTED_CERT, IGNORED_PTR_ARG));
    }
    if (proxy)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_HTTP_PROXY, IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());

    setup_steps_1_and_2_mocks(cred_type);

    setup_Blob_UploadMultipleBlocksFromSasUri_mocks(cred_type, blob_result, null_buffer);

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG));
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

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_x509_ecc_succeeds)
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

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_config_NULL_fails)
{
    //arrange

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(NULL, TEST_AUTH_HANDLE);

    //assert
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_auth_handle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, NULL);

    //assert
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_fails)
{
    //arrange
    int result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

            //assert
            ASSERT_IS_NULL(h, "IoTHubClient_LL_UploadToBlob_Create failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Destroy_handle_NULL_fail)
{
    //arrange

    //act
    IoTHubClient_LL_UploadToBlob_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Destroy_handle_sas_token_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    IoTHubClient_LL_UploadToBlob_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Destroy_handle_x509_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(TEST_AUTH_HANDLE)).SetReturn(IOTHUB_CREDENTIAL_TYPE_X509_ECC);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    IoTHubClient_LL_UploadToBlob_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_handle_NULL_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(NULL, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_source_NULL_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, NULL, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_with_proxy_succeeds)
{
    //arrange
    HTTP_PROXY_OPTIONS proxy_options = { 0 };
    proxy_options.host_address = "127.0.0.1";
    proxy_options.port = 8888;

    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_HTTP_PROXY, &proxy_options);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN, true, false, false, BLOB_OK, false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    (void)result;
    //ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_TRUSTED_CERT, TEST_CERT);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN, false, false, true, BLOB_OK, false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_device_key_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY, false, false, false, BLOB_OK, false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_x509_succeeds)
{
    //arrange

    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509_ECC);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_X509, false, false, false, BLOB_OK, false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_device_auth_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH, false, false, false, BLOB_OK, false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_fails)
{
    //arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN, false, false, false, BLOB_OK, false);

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHubClient_LL_UploadToBlob_Impl failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        }
    }

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_dev_key_fails)
{
    //arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY, false, false, false, BLOB_OK, false);

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHubClient_LL_UploadToBlob_Impl failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        }
    }

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_dev_auth_fails)
{
    //arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH, false, false, false, BLOB_OK, false);

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHubClient_LL_UploadToBlob_Impl failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        }
    }

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_blob_aborted_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN, false, false, false, BLOB_ABORTED, false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_blob_error_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN, false, false, false, BLOB_ERROR, false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Impl_null_response_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    setup_upload_blocks_mocks(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN, false, false, false, BLOB_OK, true);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, TEST_DESTINATION_FILENAME, TEST_SOURCE, TEST_SOURCE_LENGTH);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_handle_NULL_fails)
{
    bool curlVerbosity = true;
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(NULL, OPTION_CURL_VERBOSE, &curlVerbosity);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_verbose_succeeds)
{
    bool curlVerbosity = true;
    //arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_CURL_VERBOSE, &curlVerbosity);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509_cert_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_CERT, TEST_CERT);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509_cert_twice_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_CERT, TEST_CERT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_CERT, TEST_CERT);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509_key_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_PRIVATE));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_PRIVATE_KEY, TEST_PRIVATE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509_trusted_cert_succeeds)
{
    //arrange
    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_TRUSTED_CERT, TEST_CERT);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_unknown_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, "unknown", TEST_CERT);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509_timeout_succeeds)
{
    //arrange
    size_t timeout = 10;

    setup_uploadtoblob_create_mocks(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_BLOB_UPLOAD_TIMEOUT_SECS, &timeout);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

END_TEST_SUITE(iothubclient_ll_uploadtoblob_ut)
