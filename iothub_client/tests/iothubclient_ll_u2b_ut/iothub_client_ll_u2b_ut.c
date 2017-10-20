// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#ifdef DONT_USE_UPLOADTOBLOB
#error "trying to compile iothub_client_ll_u2b_ut.c while DONT_USE_UPLOADTOBLOB is #define'd"
#else

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

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umock_c_negative_tests.h"
#include "umocktypes.h"
#include "umocktypes_c.h"

#include "iothub_client_options.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "blob.h"
#include "parson.h"

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)malloc(1);
}

static STRING_HANDLE my_STRING_construct_n(const char* psz, size_t n)
{
    (void)psz;
    (void)n;
    return (STRING_HANDLE)malloc(1);
}

static STRING_HANDLE my_STRING_new(void)
{
    return (STRING_HANDLE)malloc(1);
}

static STRING_HANDLE my_STRING_from_byte_array(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (STRING_HANDLE)malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    free(handle);
}

static STRING_HANDLE my_URL_EncodeString(const char* textEncode)
{
	(void)textEncode;
	return (STRING_HANDLE)malloc(1);
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)malloc(1);
}

static void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE h)
{
    free(h);
}

static BUFFER_HANDLE my_BUFFER_new(void)
{
    return (BUFFER_HANDLE)malloc(1);
}

static BUFFER_HANDLE my_BUFFER_create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (BUFFER_HANDLE)malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    free(handle);
}

static HTTPAPIEX_HANDLE my_HTTPAPIEX_Create(const char* hostName)
{
    (void)hostName;
    return (HTTPAPIEX_HANDLE)malloc(1);
}

static void my_HTTPAPIEX_Destroy(HTTPAPIEX_HANDLE handle)
{
    free(handle);
}

static HTTPAPIEX_SAS_HANDLE my_HTTPAPIEX_SAS_Create(STRING_HANDLE key, STRING_HANDLE uriResource, STRING_HANDLE keyName)
{
    (void)key;
    (void)uriResource;
    (void)keyName;
    return (HTTPAPIEX_SAS_HANDLE)malloc(1);
}

static void my_HTTPAPIEX_SAS_Destroy(HTTPAPIEX_SAS_HANDLE handle)
{
    free(handle);
}

static JSON_Value * my_json_parse_string(const char *string)
{
    (void)string;
    return (JSON_Value *)malloc(1);
}

static void my_json_value_free(JSON_Value *value)
{
    free(value);
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

#include "azure_c_shared_utility/gballoc.h"

#undef ENABLE_MOCKS

#include "iothub_client_ll_uploadtoblob.h"

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


static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static const TRANSPORT_PROVIDER* provideFAKE(void);
#define TEST_DEVICE_ID "theidofTheDevice"
#define TEST_DEVICE_KEY "theKeyoftheDevice"
#define TEST_DEVICE_SAS "theSasOfTheDevice"
#define TEST_DEVICE_SAS_FAIL "fail_theSasOfTheDevice"
#define TEST_IOTHUBNAME "theNameoftheIotHub"
#define TEST_IOTHUBSUFFIX "theSuffixoftheIotHubHostname"
#define TEST_AUTHORIZATIONKEY "theAuthorizationKey"

#define TEST_STRING_HANDLE_DEVICE_ID ((STRING_HANDLE)0x1)
#define TEST_STRING_HANDLE_DEVICE_SAS ((STRING_HANDLE)0x2)

#define TEST_API_VERSION "?api-version=2016-11-14"
#define TEST_IOTHUB_SDK_VERSION "1.1.26"

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

BEGIN_TEST_SUITE(iothubclient_ll_uploadtoblob_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    umocktypes_charptr_register_types();

    REGISTER_TYPE(HTTPAPI_RESULT, HTTPAPI_RESULT);
    REGISTER_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT);
    REGISTER_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT);
    REGISTER_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE);
    REGISTER_TYPE(BLOB_RESULT, BLOB_RESULT);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(char **, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_SAS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const unsigned char*, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct_n, my_STRING_construct_n);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_from_byte_array, my_STRING_from_byte_array);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_DEFAULT_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, TEST_DEFAULT_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, __FAILURE__);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat_with_STRING, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, __FAILURE__);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_copy, __FAILURE__);
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

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, my_json_parse_string);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, (JSON_Object*)1);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, "a");
    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, my_json_value_free);
    

    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_ExecuteRequest, my_HTTPAPIEX_ExecuteRequest)
    REGISTER_GLOBAL_MOCK_RETURN(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_OK);
    
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_SetOption, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Blob_UploadFromSasUri, BLOB_ERROR);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __FAILURE__);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);

}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
    
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_happypath)
{
    ///arrange

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_ID));

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    
    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_SAS));

    ///act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);

    ///assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_unhappypaths)
{
    ///arrange

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_ID))
        .SetFailReturn(NULL);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_SAS))
        .SetFailReturn(NULL);

    umock_c_negative_tests_snapshot();

    ///act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        /// arrange
        char temp_str[128];
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        /// act
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);

        /// assert
        sprintf(temp_str, "On failed call %zu", i + 1);
        ASSERT_IS_NULL_WITH_MSG(h, temp_str);

        ///cleanup
    }
    
    umock_c_negative_tests_deinit();
    
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Destroy_with_NULL_handle_does_nothing)
{
    ///arrange

    ///act
    IoTHubClient_LL_UploadToBlob_Destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup

}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Destroy_happypath)
{
    ///arrange
    void* malloc1;
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .CaptureReturn(&malloc1);

    STRING_HANDLE s1;
    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_ID))
        .CaptureReturn(&s1);

    void* malloc2;
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .CaptureReturn(&malloc2);

    STRING_HANDLE s2;
    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_SAS))
        .CaptureReturn(&s2);

    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);

    STRICT_EXPECTED_CALL(STRING_delete(s2));
    STRICT_EXPECTED_CALL(gballoc_free(malloc2));
    STRICT_EXPECTED_CALL(STRING_delete(s1));
    STRICT_EXPECTED_CALL(gballoc_free(malloc1));
    
    ///act
    IoTHubClient_LL_UploadToBlob_Destroy(h);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_with_NULL_handle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(NULL, "text.txt", (const unsigned char*)"a", 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_with_NULL_destinationFileName_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, NULL, (const unsigned char*)"a", 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_with_NULL_source_and_non_zero_size_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", NULL, 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_064: [ IoTHubClient_LL_UploadToBlob shall create an HTTPAPIEX_HANDLE to the IoTHub hostname. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_066: [ IoTHubClient_LL_UploadToBlob shall create an HTTP relative path formed from "/devices/" + deviceId + "/files/" + destinationFileName + "?api-version=API_VERSION". ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_068: [ IoTHubClient_LL_UploadToBlob shall create an HTTP responseContent BUFFER_HANDLE. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_070: [ IoTHubClient_LL_UploadToBlob shall create request HTTP headers. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_072: [ IoTHubClient_LL_UploadToBlob shall add the following name:value to request HTTP headers: ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_073: [ If the credentials used to create iotHubClientHandle have "sasToken" then IoTHubClient_LL_UploadToBlob shall add the following HTTP request headers: ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_075: [ IoTHubClient_LL_UploadToBlob shall execute HTTPAPIEX_ExecuteRequest passing the following information for arguments: ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_081: [ Otherwise, IoTHubClient_LL_UploadToBlob shall use parson to extract and save the following information from the response buffer: correlationID and SasUri. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_085: [ IoTHubClient_LL_UploadToBlob shall use the same authorization as step 1. to prepare and perform a HTTP request with the following parameters: ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_088: [ Otherwise, IoTHubClient_LL_UploadToBlob shall succeed and return IOTHUB_CLIENT_OK. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_083: [ IoTHubClient_LL_UploadToBlob shall call Blob_UploadFromSasUri and capture the HTTP return code and HTTP body. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SAS_token_happypath)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);
    
    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*10*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", ""))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this fetches the SAS from under h (handle)*/
            .IgnoreArgument(1)
            .SetReturn(TEST_DEVICE_SAS);

        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", TEST_DEVICE_SAS))
            .IgnoreArgument(1);

        const char* iotHubHttpRelativePath1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpRelativePath1))
            .CaptureReturn(&iotHubHttpRelativePath1_as_const_char)
            .IgnoreArgument(1);



        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            iotHubHttpRelativePath1_as_const_char,
            iotHubHttpRequestHeaders1,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            iotHubHttpMessageBodyResponse1
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            .IgnoreArgument(8);

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size)) /*20*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId"))
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://")) /*30*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1)) /*40*/
            .IgnoreArgument(1);
    }
    
    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/
        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/"))
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        const char* relativePathNotification_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(relativePathNotification))
            .CaptureReturn(&relativePathNotification_as_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            relativePathNotification_as_char,
            iotHubHttpRequestHeaders1,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            NULL
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred));

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*60*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_106: [ - x509certificate and x509privatekey saved options shall be passed on the HTTPAPIEX_SetOption ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SAS_token_with_certificates_happypath)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, "TrustedCerts", "some certificates");
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, "TrustedCerts", IGNORED_PTR_ARG));

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*10*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", ""))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this fetches the SAS from under h (handle)*/
            .IgnoreArgument(1)
            .SetReturn(TEST_DEVICE_SAS);

        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", TEST_DEVICE_SAS))
            .IgnoreArgument(1);

        const char* iotHubHttpRelativePath1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpRelativePath1))
            .CaptureReturn(&iotHubHttpRelativePath1_as_const_char)
            .IgnoreArgument(1);



        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            iotHubHttpRelativePath1_as_const_char,
            iotHubHttpRequestHeaders1,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            iotHubHttpMessageBodyResponse1
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            .IgnoreArgument(8);

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size)) /*20*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId"))
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://")) /*30*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1)) /*40*/
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, "some certificates"))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/
        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/"))
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        const char* relativePathNotification_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(relativePathNotification))
            .CaptureReturn(&relativePathNotification_as_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            relativePathNotification_as_char,
            iotHubHttpRequestHeaders1,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            NULL
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred));

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*60*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);


    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_106: [ - x509certificate and x509privatekey saved options shall be passed on the HTTPAPIEX_SetOption ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SAS_token_with_certificates_unhappypath)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, "TrustedCerts", "some certificates");
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, "TrustedCerts", IGNORED_PTR_ARG))
        .SetReturn(HTTPAPIEX_ERROR);

    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);


    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SAS_token_when_step1_http_code_is400_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);


        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*10*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", ""))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this fetches the SAS from under h (handle)*/
            .IgnoreArgument(1)
            .SetReturn(TEST_DEVICE_SAS);

        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", TEST_DEVICE_SAS))
            .IgnoreArgument(1);

        const char* iotHubHttpRelativePath1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpRelativePath1))
            .CaptureReturn(&iotHubHttpRelativePath1_as_const_char)
            .IgnoreArgument(1);



        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            iotHubHttpRelativePath1_as_const_char,
            iotHubHttpRequestHeaders1,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            iotHubHttpMessageBodyResponse1
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&FourHundred, sizeof(FourHundred))
            .IgnoreArgument(8);

        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1)) /*40*/
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);


    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_077: [ If HTTP statusCode is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SAS_token_when_step2_httpStatusCode_is_bad_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);


        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*10*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", ""))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this fetches the SAS from under h (handle)*/
            .IgnoreArgument(1)
            .SetReturn(TEST_DEVICE_SAS);

        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", TEST_DEVICE_SAS))
            .IgnoreArgument(1);

        const char* iotHubHttpRelativePath1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpRelativePath1))
            .CaptureReturn(&iotHubHttpRelativePath1_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            iotHubHttpRelativePath1_as_const_char,
            iotHubHttpRequestHeaders1,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            iotHubHttpMessageBodyResponse1
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            .IgnoreArgument(8);

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size)) /*20*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId"))
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://")) /*30*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1)) /*40*/
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&FourHundred, sizeof(FourHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/
        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/"))
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        const char* relativePathNotification_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(relativePathNotification))
            .CaptureReturn(&relativePathNotification_as_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            relativePathNotification_as_char,
            iotHubHttpRequestHeaders1,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            NULL
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred));

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*60*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result); /*ERROR because upload failed*/


    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_087: [ If the statusCode of the HTTP request is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SAS_token_when_step3_httpStatusCode_is_bad_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*10*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", ""))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this fetches the SAS from under h (handle)*/
            .IgnoreArgument(1)
            .SetReturn(TEST_DEVICE_SAS);

        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", TEST_DEVICE_SAS))
            .IgnoreArgument(1);

        const char* iotHubHttpRelativePath1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpRelativePath1))
            .CaptureReturn(&iotHubHttpRelativePath1_as_const_char)
            .IgnoreArgument(1);



        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            iotHubHttpRelativePath1_as_const_char,
            iotHubHttpRequestHeaders1,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            iotHubHttpMessageBodyResponse1
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
			.IgnoreArgument(5)
            .IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            .IgnoreArgument(8);

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size)) /*20*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId"))
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://")) /*30*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1)) /*40*/
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/
        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/"))
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        const char* relativePathNotification_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(relativePathNotification))
            .CaptureReturn(&relativePathNotification_as_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            relativePathNotification_as_char,
            iotHubHttpRequestHeaders1,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            NULL
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_statusCode(&FourHundred, sizeof(FourHundred));

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*60*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result); /*ERROR because notification failed*/


    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_065: [ If creating the HTTPAPIEX_HANDLE fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_067: [ If creating the relativePath fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_069: [ If creating the HTTP response buffer handle fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_071: [ If creating the HTTP headers fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_074: [ If adding "Authorization" fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_076: [ If HTTPAPIEX_ExecuteRequest call fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_084: [ If Blob_UploadFromSasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_086: [ If performing the HTTP request fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SAS_token_unhappypaths)
{
    int result2;
    result2 = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result2);

    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*10*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", ""))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this fetches the SAS from under h (handle)*/
            .IgnoreArgument(1)
            .SetReturn(TEST_DEVICE_SAS);

        STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", TEST_DEVICE_SAS))
            .IgnoreArgument(1);

        const char* iotHubHttpRelativePath1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpRelativePath1))
            .CaptureReturn(&iotHubHttpRelativePath1_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            iotHubHttpRelativePath1_as_const_char,
            iotHubHttpRequestHeaders1,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            iotHubHttpMessageBodyResponse1
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            .IgnoreArgument(8);

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size)) /*20*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId"))
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://")) /*30*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1)) /*46*/
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/
        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/"))
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        const char* relativePathNotification_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(relativePathNotification))
            .CaptureReturn(&relativePathNotification_as_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            relativePathNotification_as_char,
            iotHubHttpRequestHeaders1,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            NULL
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred));

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*60*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    ///act

    size_t calls_that_cannot_fail[] = { 
		12, /*STRING_c_str*/
        19, /*STRING_c_str*/
        21, /*STRING_c_str*/
        23, /*BUFFER_u_char*/
        24, /*BUFFER_length*/
        26, /*STRING_c_str*/
		41, /*STRING_c_str*/
		44, /*STRING_delete*/
        45, /*json_value_free*/
        46, /*STRING_delete*/
        47, /*BUFFER_delete*/
        49, /*BUFFER_delete*/
        50, /*STRING_delete*/
        51, /*STRING_delete*/
        53, /*STRING_c_str*/
        55, /*BUFFER_u_char*/
        57, /*BUFFER_u_char*/
        66, /*STRING_c_str*/
        69, /*STRING_c_str*/
        70, /*BUFFER_delete*/
        71, /*STRING_delete*/
        72, /*STRING_delete*/
        73, /*BUFFER_delete*/
        74, /*gballoc_free*/
        75, /*BUFFER_delete*/
        76, /*HTTPHeaders_Free*/
        77, /*STRING_delete*/
        78, /*STRING_delete*/
        79, /*HTTPAPIEX_Destroy*/
    };

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        printf("this is call: %d\n", (int)i);
        /// arrange
        char temp_str[128];
        size_t j;
        umock_c_negative_tests_reset();
        for (size_t k = i;k < umock_c_negative_tests_call_count();k++) /*fail call "i" and all subsequent*/
        {
            umock_c_negative_tests_fail_call(k);
        }

        /// act
        for (j = 0;j<sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]);j++)
        {
            if (calls_that_cannot_fail[j] == i)
                break;
        }

        if (j == sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]))
        {
            /// arrange
            sprintf(temp_str, "On failed call %zu", i);

            ///act
            IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

            ///assert
			//ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, temp_str);
        }
    }

    umock_c_negative_tests_deinit();
   
    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}


TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_DeviceKey_happypath)
{
    ///arrange

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_ID));

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_KEY));

    ///act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);

    ///assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_DeviceKey_unhappypaths)
{
    ///arrange

    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_ID))
        .SetFailReturn(NULL);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .SetFailReturn(NULL);

    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_KEY))
        .SetFailReturn(NULL);

    umock_c_negative_tests_snapshot();

    ///act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        /// arrange
        char temp_str[128];
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        /// act
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);

        /// assert
        sprintf(temp_str, "On failed call %zu", i + 1);
        ASSERT_IS_NULL_WITH_MSG(h, temp_str);
    }

    umock_c_negative_tests_deinit();

}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Destroy_with_DeviceKey_happypath)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    ///act
    IoTHubClient_LL_UploadToBlob_Destroy(h);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_078: [ If the credentials used to create iotHubClientHandle have "deviceKey" then IoTHubClient_LL_UploadToBlob shall create an HTTPAPIEX_SAS_HANDLE passing as arguments: ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_090: [ IoTHubClient_LL_UploadToBlob shall call HTTPAPIEX_SAS_ExecuteRequest passing as arguments: ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_deviceKey_happypath)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*10*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", "")) /*14*/
            .IgnoreArgument(1);

       
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX)); /*this is starting to build the path that the SAS token authenticates*/
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/devices/")) /*this is building the path that the SAS token authenticates*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the path that the SAS token authenticates*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_new());/*this is needed for HTTPAPIEX_SAS_Create -it needs an empty STRING_HANDLE*/

        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest( /*20*/
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_POST,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG
        ))
            .IgnoreArgument_sasHandle()
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestHttpHeadersHandle()
            .IgnoreArgument_requestContent()
            .IgnoreArgument_statusCode()
            .IgnoreArgument_responseHeadersHandle()
            .IgnoreArgument_responseContent()
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
			.IgnoreArgument(6)
            ;
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*the empty STRING_new*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*the build the path that the SAS token authenticates*/
            .IgnoreArgument_handle();

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId")) /*30*/
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/")) /*40*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1))
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/

        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/")) /*60*/
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_new());
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_POST,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG
        ))
            .IgnoreArgument_sasHandle()
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestHttpHeadersHandle()
            .IgnoreArgument_requestContent()
            .IgnoreArgument_statusCode()
            .IgnoreArgument_responseHeadersHandle()
            .IgnoreArgument_responseContent()
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            ;
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
            
        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*70*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}


TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_deviceKey_when_step1_httpStatusCode_is_400_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);
        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", "")) /*14*/
            .IgnoreArgument(1);


        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX)); /*this is starting to build the path that the SAS token authenticates*/
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/devices/")) /*this is building the path that the SAS token authenticates*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the path that the SAS token authenticates*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_new());/*this is needed for HTTPAPIEX_SAS_Create -it needs an empty STRING_HANDLE*/

        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_POST,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG
        ))
            .IgnoreArgument_sasHandle()
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestHttpHeadersHandle()
            .IgnoreArgument_requestContent()
            .IgnoreArgument_statusCode()
            .IgnoreArgument_responseHeadersHandle()
            .IgnoreArgument_responseContent()
			.IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&FourHundred, sizeof(FourHundred))
            ;
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*the empty STRING_new*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*the build the path that the SAS token authenticates*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}


/*Tests_SRS_IOTHUBCLIENT_LL_02_080: [ If status code is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_deviceKey_when_step3_httpStatusCode_is_400_it_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

		BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*10*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", "")) /*14*/
            .IgnoreArgument(1);


        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX)); /*this is starting to build the path that the SAS token authenticates*/
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/devices/")) /*this is building the path that the SAS token authenticates*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the path that the SAS token authenticates*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_new());/*this is needed for HTTPAPIEX_SAS_Create -it needs an empty STRING_HANDLE*/

        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest( /*20*/
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_POST,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG
        ))
            .IgnoreArgument_sasHandle()
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestHttpHeadersHandle()
            .IgnoreArgument_requestContent()
            .IgnoreArgument_statusCode()
            .IgnoreArgument_responseHeadersHandle()
            .IgnoreArgument_responseContent()
			.IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            ;
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*the empty STRING_new*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*the build the path that the SAS token authenticates*/
            .IgnoreArgument_handle();

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId")) /*30*/
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/")) /*40*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1))
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/

        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/")) /*60*/
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_new());
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_POST,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG
        ))
            .IgnoreArgument_sasHandle()
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestHttpHeadersHandle()
            .IgnoreArgument_requestContent()
            .IgnoreArgument_statusCode()
            .IgnoreArgument_responseHeadersHandle()
            .IgnoreArgument_responseContent()
            .CopyOutArgumentBuffer_statusCode(&FourHundred, sizeof(FourHundred))
            ;
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*70*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_089: [ If creating the HTTPAPIEX_SAS_HANDLE fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_079: [ If HTTPAPIEX_SAS_ExecuteRequest fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_deviceKey_unhappypaths)
{
    ///arrange
    int umockc_result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, umockc_result);

    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*7*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json")) /*15*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Authorization", "")) /*18*/
            .IgnoreArgument(1);


        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX)); /*this is starting to build the path that the SAS token authenticates*/
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/devices/")) /*this is building the path that the SAS token authenticates*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the path that the SAS token authenticates*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_new());/*this is needed for HTTPAPIEX_SAS_Create -it needs an empty STRING_HANDLE*/

        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest( /*25*/
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_POST,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG
        ))
            .IgnoreArgument_sasHandle()
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestHttpHeadersHandle()
            .IgnoreArgument_requestContent()
            .IgnoreArgument_statusCode()
            .IgnoreArgument_responseHeadersHandle()
            .IgnoreArgument_responseContent()
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
			.IgnoreArgument(6)
            ;
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*the empty STRING_new*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*the build the path that the SAS token authenticates*/
            .IgnoreArgument_handle();

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))	/*30*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))/*32*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId")) /*30*/
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/")) /*40*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))/*48*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1))
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/

        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/")) /*60*/
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_new());
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_POST,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            NULL
        ))
            .IgnoreArgument_sasHandle()
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestHttpHeadersHandle()
            .IgnoreArgument_requestContent()
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            ;
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*70*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    size_t calls_that_cannot_fail[] = {
		11, /*STRING_length*/
        12, /*STRING_c_str*/
        24, /*STRING_c_str*/
        26, /*HTTPAPIEX_SAS_Destroy*/
        27, /*STRING_delete*/
        28, /*STRING_delete*/
        29, /*BUFFER_u_char*/
        30, /*BUFFER_length*/
        32, /*STRING_c_str*/
		47, /*STRING_c_str*/
		50, /*STRING_delete*/
        51, /*json_value_free*/
        52, /*STRING_delete*/
        53, /*BUFFER_delete*/
        54, /*BUFFER_delete*/
        55, /*STRING_delete*/
        56, /*STRING_delete*/
        58, /*STRING_c_str*/
        60, /*BUFFER_u_char*/
        62, /*BUFFER_u_char*/
        71, /*STRING_c_str*/
        76, /*STRING_c_str*/
        78, /*HTTPAPIEX_SAS_Destroy*/
        79, /*STRING_delete*/
        80, /*STRING_delete*/
        81, /*STRING_delete*/
        82, /*BUFFER_delete*/
        83, /*gballoc_free*/
        84, /*BUFFER_delete*/
        85, /*HTTPHeaders_Free*/
        86, /*STRING_delete*/
        87, /*STRING_delete*/
        88, /*HTTPAPIEX_Destroy*/
    };

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        /// arrange
        char temp_str[128];
        size_t j;
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        /// act
        for (j = 0;j<sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]);j++) /*not running the tests that have failed that cannot fail*/
        {
            if (calls_that_cannot_fail[j] == i)
                break;
        }

        if (j == sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]))
        {
            /// assert
            sprintf(temp_str, "On failed call %zu", i);
            IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, temp_str);
        }
    }

    umock_c_negative_tests_deinit();

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_106: [ - x509certificate and x509privatekey saved options shall be passed on the HTTPAPIEX_SetOption ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_107: [ - "Authorization" header shall not be build. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_108: [ IoTHubClient_LL_UploadToBlob shall execute HTTPAPIEX_ExecuteRequest passing the following information for arguments: ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_passes_x509_information_to_HTTPAPIEX_happyPath)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_X509_CERT, IGNORED_PTR_ARG))
        .ValidateArgumentValue_handle(&iotHubHttpApiExHandle)
        .IgnoreArgument(3)
        .SetReturn(HTTPAPIEX_OK);

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_X509_PRIVATE_KEY, IGNORED_PTR_ARG))
        .ValidateArgumentValue_handle(&iotHubHttpApiExHandle)
        .IgnoreArgument(3)
        .SetReturn(HTTPAPIEX_OK);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*10*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

			
        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);

        const char* iotHubHttpRelativePath1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpRelativePath1))
            .CaptureReturn(&iotHubHttpRelativePath1_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            iotHubHttpRelativePath1_as_const_char,
            iotHubHttpRequestHeaders1,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            iotHubHttpMessageBodyResponse1
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            .IgnoreArgument(8)
            .ValidateArgumentValue_handle(&iotHubHttpApiExHandle);

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)) /*20*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId"))
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://")) /*30*/
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1)) /*40*/
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/

        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/
        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG)) /*50*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/"))
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        const char* relativePathNotification_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(relativePathNotification))
            .CaptureReturn(&relativePathNotification_as_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            relativePathNotification_as_char,
            iotHubHttpRequestHeaders1,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            NULL
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred));

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) /*60*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);


    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_106: [ - x509certificate and x509privatekey saved options shall be passed on the HTTPAPIEX_SetOption ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_107: [ - "Authorization" header shall not be build. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_108: [ IoTHubClient_LL_UploadToBlob shall execute HTTPAPIEX_ExecuteRequest passing the following information for arguments: ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_passes_x509_information_to_HTTPAPIEX_unhappy_paths)
{
    ///arrange
    int umockc_result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, umockc_result);

    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509);
    unsigned char c = '3';
    umock_c_reset_all_calls();

    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
        .CaptureReturn(&iotHubHttpApiExHandle)
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_X509_CERT, IGNORED_PTR_ARG))
        .ValidateArgumentValue_handle(&iotHubHttpApiExHandle)
        .IgnoreArgument(3)
        .SetReturn(HTTPAPIEX_OK);

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_X509_PRIVATE_KEY, IGNORED_PTR_ARG))
        .ValidateArgumentValue_handle(&iotHubHttpApiExHandle)
        .IgnoreArgument(3)
        .SetReturn(HTTPAPIEX_OK);

    STRING_HANDLE correlationId;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&correlationId);

    STRING_HANDLE sasUri;
    STRICT_EXPECTED_CALL(STRING_new())
        .CaptureReturn(&sasUri);

    HTTP_HEADERS_HANDLE iotHubHttpRequestHeaders1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc())
        .CaptureReturn(&iotHubHttpRequestHeaders1);

    {
        STRING_HANDLE iotHubHttpRelativePath1;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&iotHubHttpRelativePath1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*IGNORED_PTR_ARG is the deviceId, which stays nicely tucked in h (handle)*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, "/files"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(iotHubHttpRelativePath1, TEST_API_VERSION)) /*7*/
            .IgnoreArgument(1);
			
		STRING_HANDLE blobJson;
        STRICT_EXPECTED_CALL(STRING_construct("{ \"blobName\": \""))
            .CaptureReturn(&blobJson);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, IGNORED_PTR_ARG))
			.IgnoreArgument(1)
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(STRING_concat(blobJson, "\" }"))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_length(blobJson))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(blobJson))
			.IgnoreArgument(1);
			
		BUFFER_HANDLE jsonBuffer;
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
			.CaptureReturn(&jsonBuffer);

        BUFFER_HANDLE iotHubHttpMessageBodyResponse1;
        STRICT_EXPECTED_CALL(BUFFER_new())
            .CaptureReturn(&iotHubHttpMessageBodyResponse1);

        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Content-Type", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "Accept", "application/json"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(iotHubHttpRequestHeaders1, "User-Agent", "iothubclient/" TEST_IOTHUB_SDK_VERSION))
            .IgnoreArgument(1);

        const char* iotHubHttpRelativePath1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpRelativePath1))
            .CaptureReturn(&iotHubHttpRelativePath1_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            iotHubHttpRelativePath1_as_const_char,
            iotHubHttpRequestHeaders1,
            jsonBuffer,
            IGNORED_PTR_ARG,
            NULL,
            iotHubHttpMessageBodyResponse1
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .IgnoreArgument(6)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            .IgnoreArgument(8)
            .ValidateArgumentValue_handle(&iotHubHttpApiExHandle);

        unsigned char* iotHubHttpMessageBodyResponse1_unsigned_char = (unsigned char*)TEST_DEFAULT_STRING_VALUE;
        size_t iotHubHttpMessageBodyResponse1_size;
        STRICT_EXPECTED_CALL(BUFFER_u_char(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_unsigned_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(iotHubHttpMessageBodyResponse1))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_size)
            .IgnoreArgument(1);

        STRING_HANDLE iotHubHttpMessageBodyResponse1_as_STRING_HANDLE;
        STRICT_EXPECTED_CALL(STRING_from_byte_array(iotHubHttpMessageBodyResponse1_unsigned_char, iotHubHttpMessageBodyResponse1_size))
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* iotHubHttpMessageBodyResponse1_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE)) /*20*/
            .CaptureReturn(&iotHubHttpMessageBodyResponse1_as_const_char)
            .IgnoreArgument(1);

        JSON_Value* allJson;
        STRICT_EXPECTED_CALL(json_parse_string(iotHubHttpMessageBodyResponse1_as_const_char))
            .CaptureReturn(&allJson)
            .IgnoreArgument(1);

        JSON_Object* jsonObject;
        STRICT_EXPECTED_CALL(json_value_get_object(allJson))
            .CaptureReturn(&jsonObject)
            .IgnoreArgument(1);

        const char* json_correlationId = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "correlationId"))
            .CaptureReturn(&json_correlationId)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(correlationId, json_correlationId))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        const char* json_hostName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "hostName"))
            .CaptureReturn(&json_hostName)
            .IgnoreArgument(1);

        const char* json_containerName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "containerName"))
            .CaptureReturn(&json_containerName)
            .IgnoreArgument(1);

        const char* json_blobName = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "blobName"))
            .CaptureReturn(&json_blobName)
            .IgnoreArgument(1);

        const char* json_sasToken = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(json_object_get_string(jsonObject, "sasToken"))
            .CaptureReturn(&json_sasToken)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_copy(sasUri, "https://"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(URL_EncodeString(json_blobName))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_hostName)) /*30*/
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_containerName))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, "/"))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(sasUri, json_sasToken))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

		STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(allJson))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpMessageBodyResponse1_as_STRING_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_delete(iotHubHttpMessageBodyResponse1))
            .IgnoreArgument(1);
		STRICT_EXPECTED_CALL(BUFFER_delete(jsonBuffer))
			.IgnoreArgument(1);
		STRICT_EXPECTED_CALL(STRING_delete(blobJson))
			.IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(iotHubHttpRelativePath1))
            .IgnoreArgument(1);
    }

    {/*step2*/
        STRICT_EXPECTED_CALL(BUFFER_new()); /*this is building the buffer that will contain the response from Blob_UploadFromSasUri*/ /*40*/
         
        const char* sasUri_as_const_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(sasUri))
            .CaptureReturn(&sasUri_as_const_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Blob_UploadFromSasUri(sasUri_as_const_char, &c, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_httpStatus(&TwoHundred, sizeof(TwoHundred))
            ;
        /*some snprintfs happen here... */
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_size()
            ;
    }

    {/*step3*/
        STRING_HANDLE uriResource;
        STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUBNAME "." TEST_IOTHUBSUFFIX))
            .CaptureReturn(&uriResource);

        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/devices/"))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(uriResource, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(uriResource, "/files/notifications"))/*50*/
            .IgnoreArgument(1);

        STRING_HANDLE relativePathNotification;
        STRICT_EXPECTED_CALL(STRING_construct("/devices/"))
            .CaptureReturn(&relativePathNotification);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(relativePathNotification, IGNORED_PTR_ARG)) 
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, "/files/notifications/"))
            .IgnoreArgument(1);

        const char* correlationId_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(correlationId))
            .CaptureReturn(&correlationId_as_char)
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, correlationId_as_char))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(relativePathNotification, TEST_API_VERSION))
            .IgnoreArgument(1);

        const char* relativePathNotification_as_char = TEST_DEFAULT_STRING_VALUE;
        STRICT_EXPECTED_CALL(STRING_c_str(relativePathNotification))
            .CaptureReturn(&relativePathNotification_as_char)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            iotHubHttpApiExHandle,
            HTTPAPI_REQUEST_POST,
            relativePathNotification_as_char,
            iotHubHttpRequestHeaders1,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            NULL
        ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(5)
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred));

        STRICT_EXPECTED_CALL(STRING_delete(relativePathNotification)) 
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(uriResource)) /*60*/
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(iotHubHttpRequestHeaders1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(sasUri))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(correlationId))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(iotHubHttpApiExHandle))
        .IgnoreArgument(1);

    umock_c_negative_tests_snapshot();

    size_t calls_that_cannot_fail[] = {
		13, /*STRING_length*/
		14, /*STRING_c_str*/
        20, /*STRING_c_str*/
//        22, /*STRING_c_str*/
        22, /*BUFFER_u_char*/
        23, /*BUFFER_length*/
        25, /*STRING_c_str*/
		40, /*STRING_c_str*/
		43, /*STRING_delete*/
        44, /*json_value_free*/
        45, /*STRING_delete*/
        46, /*BUFFER_delete*/
        47, /*BUFFER_delete*/
        48, /*STRING_delete*/
        49, /*STRING_delete*/
        51, /*STRING_c_str*/
        53, /*BUFFER_u_char*/
        55, /*BUFFER_u_char*/
        64, /*STRING_c_str*/
        67, /*STRING_c_str*/
        68, /*BUFFER_delete*/
        69, /*STRING_delete*/
        70, /*STRING_delete*/
        71, /*BUFFER_delete*/
        72, /*gballoc_free*/
        73, /*BUFFER_delete*/
        74, /*HTTPHeaders_Free*/
        75, /*STRING_delete*/
        76, /*STRING_delete*/
        77, /*HTTPAPIEX_Destroy*/
    };

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        /// arrange
        char temp_str[128];
        size_t j;


        /// act
        for (j = 0;j<sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]);j++) /*not running the tests that have failed that cannot fail*/
        {
            if (calls_that_cannot_fail[j] == i)
                break;
        }

        if (j == sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// assert
            sprintf(temp_str, "On failed call %zu", i);
            IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_Impl(h, "text.txt", &c, 1);
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, temp_str);
        }
    }

    umock_c_negative_tests_deinit();

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_110: [ If parameter handle is NULL then IoTHubClient_LL_UploadToBlob_SetOption shall fail and return IOTHUB_CLIENT_ERROR. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_with_NULL_handle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(NULL, OPTION_X509_CERT, "here be some certificate");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_100: [ x509certificate - then value then is a null terminated string that contains the x509 certificate. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_103: [ The options shall be saved. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509certificate_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "here be some certificate"))
        .IgnoreArgument_destination();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_CERT, "here be some certificate");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_101: [ x509privatekey - then value is a null terminated string that contains the x509 privatekey. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_103: [ The options shall be saved. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509privatekey_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "here be some pk"))
        .IgnoreArgument_destination();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_PRIVATE_KEY, "here be some pk");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_102: [ If an unknown option is presented then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509unknownoption_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, "x509unknownoption", "here be some pk");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_109: [ If the authentication scheme is NOT x509 then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509cerfiticate_with_devicekey_auth_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_CERT, "here be some cert");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_109: [ If the authentication scheme is NOT x509 then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509privatekey_with_devicekey_auth_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_DEVICE_KEY);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_PRIVATE_KEY, "here be some pk pk pk!");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_104: [ If saving fails, then IoTHubClient_LL_UploadToBlob_SetOption shall fail and return IOTHUB_CLIENT_ERROR. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509certificate_fails_when_saving_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "here be some certificate"))
        .IgnoreArgument_destination()
        .SetReturn(__FAILURE__);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_CERT, "here be some certificate");
    
    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_104: [ If saving fails, then IoTHubClient_LL_UploadToBlob_SetOption shall fail and return IOTHUB_CLIENT_ERROR. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509privatekey_fails_when_saving_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "here be some priv key"))
        .IgnoreArgument_destination()
        .SetReturn(__FAILURE__);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_PRIVATE_KEY, "here be some priv key");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}


END_TEST_SUITE(iothubclient_ll_uploadtoblob_ut)
#endif /*DONT_USE_UPLOADTOBLOB*/
