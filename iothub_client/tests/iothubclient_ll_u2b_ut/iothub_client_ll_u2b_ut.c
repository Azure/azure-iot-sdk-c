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
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"

#include "iothub_client_options.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/gballoc.h"
#include "internal/blob.h"
#include "internal/iothub_client_authorization.h"
#include "parson.h"

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
#undef ENABLE_MOCKS

#include "internal/iothub_client_ll_uploadtoblob.h"

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
#define TEST_IOTHUB_SDK_VERSION "1.11.0"

#define TEST_HTTP_HEADERS_HANDLE        (HTTP_HEADERS_HANDLE)0x4443
#define TEST_HTTPAPIEX_HANDLE           (HTTPAPIEX_HANDLE)0x4444
#define TEST_HTTPAPIEX_SAS_HANDLE       (HTTPAPIEX_SAS_HANDLE)0x4445
#define TEST_U2B_CONTEXT_HANDLE         (IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE)0x4446
#define TEST_JSON_VALUE                 (JSON_Value*)0x4447
#define TEST_SINGLYLINKEDLIST_HANDLE    (SINGLYLINKEDLIST_HANDLE)0x4448
#define TEST_BUFFER_HANDLE              (BUFFER_HANDLE)0x4449

static const char* const testUploadtrustedCertificates = "some certificates";
static const char* const TEST_IOTHUB_SAS_TOKEN = "test_sas_token";
static const char* const TEST_PRIVATE = "test_private";
static const char* const TEST_CERT = "test_cert";
static const unsigned char* TEST_SOURCE = (const unsigned char*)0x3;
static const size_t TEST_SOURCE_LENGTH = 3;
static const char* const TEST_DESTINATION_FILENAME = "text.txt";
static const char* const TEST_BLOB_NAME_CHAR_PTR = "{ \"blobName\": \"text.txt\" }";
static const char* const TEST_BLOB_SAS_URI = "https://mystorageaccount.blob.core.windows.net/uploadcontainer01/myDeviceId%2fsubdir%text.txt?sv=2018-03-28&sr=b&sig=BuvkhxiMqrFJDYoS4UCZq%2FMkS4rb47doCPvfdgx1iwM%3D&se=2023-07-16T05%3A31%3A08Z&sp=rw";
static const char* const TEST_UPLOAD_CORRELATION_ID = "MjAyMzA3MTYwNTQxXzVjMzg5NzMzLTlhZTgtNDVmZC1iNjQ0LTIzZDUzMDc5YzgyMF9zdWJkaXIvaGVsbG9fd29ybGRfY3VzdG9tX21iLnR4dF92ZXIyLjA=";

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

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    size_t l = strlen(source);
    *destination = (char*)malloc(l + 1);
    (void)memcpy(*destination, source, l+1);
    return 0;
}

static char* my_IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, uint64_t expiry_time_relative_seconds, const char* key_name)
{
    (void)handle;
    (void)scope;
    (void)expiry_time_relative_seconds;
    (void)key_name;
    size_t l = strlen(TEST_IOTHUB_SAS_TOKEN);
    char* result = (char*)my_gballoc_malloc(l+1);
    strcpy(result, TEST_IOTHUB_SAS_TOKEN);
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
    size_t maxBlockSize;
    size_t callbackCount;
    size_t abortOnCount;
}BLOB_UPLOAD_CONTEXT;

// By setting a very high block number below we guarantee
// our tests will never get to it and do not abort the uploads.
#define DO_NOT_ABORT_UPLOAD 1999999
BLOB_UPLOAD_CONTEXT blobUploadContext;

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT FileUpload_GetData_Callback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* _uploadContext)
{
    BLOB_UPLOAD_CONTEXT* uploadContext = (BLOB_UPLOAD_CONTEXT*) _uploadContext;

    uploadContext->callbackCount++;
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
    else if (uploadContext->callbackCount == uploadContext->abortOnCount)
    {
        return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT;
    }
    else
    {
        // Upload next block
        size_t thisBlockSize = (uploadContext->toUpload > uploadContext->maxBlockSize) ? uploadContext->maxBlockSize : uploadContext->toUpload;
        *data = (unsigned char*)uploadContext->source + (uploadContext->size - uploadContext->toUpload);
        *size = thisBlockSize;
        uploadContext->toUpload -= thisBlockSize;
    }

    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

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

static void* uploadCallbackTestContext = (void*)0x12344321;

// Mocks the final multiblock upload succeeding
static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT TestUploadMultiBlockTestSucceeds(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    ASSERT_ARE_EQUAL(void_ptr, context, uploadCallbackTestContext);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, FILE_UPLOAD_OK, result);
    ASSERT_IS_NULL(data);
    ASSERT_IS_NULL(size);
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

// Mocks the final multiblock upload failing
static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT TestUploadMultiBlockTestFails(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    ASSERT_ARE_EQUAL(void_ptr, context, uploadCallbackTestContext);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, FILE_UPLOAD_ERROR, result);
    ASSERT_IS_NULL(data);
    ASSERT_IS_NULL(size);
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT;
}

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

static void setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE cred_type)
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


static void setCreateIotHubHttpApiExHandleExpectedCalls(
    IOTHUB_CREDENTIAL_TYPE credentialType,
    size_t blobUploadTimeoutMillisecs,
    bool curlEnableVerboseLogging,
    const char* networkInterface,
    bool useTlsRenegotiation,
    int x509privatekeyType,
    const char* openSslEngine,
    const char* trustedCertificates,
    HTTP_PROXY_OPTIONS* proxyOptions)
{
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG));

    if (blobUploadTimeoutMillisecs != 0)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_HTTP_TIMEOUT, IGNORED_PTR_ARG));
    }

    if (curlEnableVerboseLogging)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_CURL_VERBOSE, IGNORED_PTR_ARG))
            .CallCannotFail();
    }

    if (networkInterface != NULL)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_CURL_INTERFACE, IGNORED_PTR_ARG));
    }

    if (credentialType == IOTHUB_CREDENTIAL_TYPE_X509 || credentialType == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
    {
        if (useTlsRenegotiation)
        {
            STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_SET_TLS_RENEGOTIATION, IGNORED_PTR_ARG));
        }

        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_X509_CERT, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_X509_PRIVATE_KEY, IGNORED_PTR_ARG));

        if (x509privatekeyType != 0)
        {
            STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_OPENSSL_PRIVATE_KEY_TYPE, IGNORED_PTR_ARG));
        }

        if (openSslEngine != NULL)
        {
            STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_OPENSSL_ENGINE, IGNORED_PTR_ARG));
        }
    }

    if (trustedCertificates != NULL)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_TRUSTED_CERT, IGNORED_PTR_ARG));
    }

    if (proxyOptions != NULL)
    {
        if (proxyOptions->host_address != NULL)
        {
            STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_HTTP_PROXY, IGNORED_PTR_ARG));
        }
        // TODO: username and password are not used... add to test if this is fixed.
    }
}

static void setCreateIotHubRequestHttpHeadersCalls(IOTHUB_CREDENTIAL_TYPE credentialType)
{
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    // Content-Type
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(TEST_HTTP_HEADERS_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // Accept
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(TEST_HTTP_HEADERS_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // User-Agent
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(TEST_HTTP_HEADERS_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    
    if (credentialType == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
    {
        // Empty Authorization
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(TEST_HTTP_HEADERS_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else if (credentialType == IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(TEST_HTTP_HEADERS_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else if (credentialType == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(TEST_HTTP_HEADERS_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void setParseResultFromIoTHubCalls()
{
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(TEST_UPLOAD_CORRELATION_ID);
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    // STRING_sprintf
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) // due to mallocAndStrcpy_s mock
        .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(TEST_BLOB_SAS_URI);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) // due to mallocAndStrcpy_s mock
        .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
}

static void setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
    IOTHUB_CREDENTIAL_TYPE credentialType,
    size_t blobUploadTimeoutMillisecs,
    bool curlEnableVerboseLogging,
    const char* networkInterface,
    bool useTlsRenegotiation,
    int x509privatekeyType,
    const char* openSslEngine,
    const char* trustedCertificates,
    HTTP_PROXY_OPTIONS* proxyOptions)
{
    // STRICT_EXPECTED_CALL(STRING_construct_sprintf(...)); // Not mocked.
    // STRICT_EXPECTED_CALL(STRING_construct_sprintf(...)); // Not mocked.
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
        .SetReturn(strlen(TEST_BLOB_NAME_CHAR_PTR))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn(TEST_BLOB_NAME_CHAR_PTR)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, strlen(TEST_BLOB_NAME_CHAR_PTR)));
    STRICT_EXPECTED_CALL(BUFFER_new());

    setCreateIotHubHttpApiExHandleExpectedCalls(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    setCreateIotHubRequestHttpHeadersCalls(credentialType);

    if (credentialType == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
    {
        // send_http_sas_request
        const unsigned int httpStatusCode = 200;

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(IGNORED_PTR_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create_From_String(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG,
            IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&httpStatusCode, sizeof(httpStatusCode));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else
    {
        // send_http_request
        const unsigned int httpStatusCode = 200;

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG,
            IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&httpStatusCode, sizeof(httpStatusCode));
    }

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    setParseResultFromIoTHubCalls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // responseAsString
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); // iotHubRequestHttpHeaders
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)); // iotHubHttpApiExHandle
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)); // responseContent
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)); // blobBuffer
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // blobName
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // relativePath
}

static void setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_NotifyCompletion(
    IOTHUB_CREDENTIAL_TYPE credentialType,
    size_t blobUploadTimeoutMillisecs,
    bool curlEnableVerboseLogging,
    const char* networkInterface,
    bool useTlsRenegotiation,
    int x509privatekeyType,
    const char* openSslEngine,
    const char* trustedCertificates,
    HTTP_PROXY_OPTIONS* proxyOptions)
{
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    setCreateIotHubHttpApiExHandleExpectedCalls(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    setCreateIotHubRequestHttpHeadersCalls(credentialType);

    if (credentialType == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
    {
        // send_http_sas_request
        const unsigned int httpStatusCode = 200;

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(IGNORED_PTR_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create_From_String(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG,
            IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&httpStatusCode, sizeof(httpStatusCode));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else
    {
        // send_http_request
        const unsigned int httpStatusCode = 200;

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG,
            IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_statusCode(&httpStatusCode, sizeof(httpStatusCode));
    }

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); // iotHubRequestHttpHeaders
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)); // iotHubHttpApiExHandle
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // relativePathNotification
    
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)); // responseToIoTHub
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // response
}

static void setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext()
{
    // createUploadToBlobContextInstance
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    // parseAzureBlobSasUri
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Blob_CreateHttpConnection(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
}

static void reset_test_data()
{
    memset(&blobUploadContext, 0, sizeof(blobUploadContext));
}

BEGIN_TEST_SUITE(iothubclient_ll_uploadtoblob_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    int result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_stdint_register_types");

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
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_SAS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const unsigned char*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const size_t, size_t);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE, void*);
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

    REGISTER_GLOBAL_MOCK_RETURNS(singlylinkedlist_create, TEST_SINGLYLINKEDLIST_HANDLE, NULL);

    REGISTER_GLOBAL_MOCK_RETURNS(HTTPHeaders_Alloc, TEST_HTTP_HEADERS_HANDLE, NULL);

    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_Create, TEST_HTTPAPIEX_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_SAS_Create, TEST_HTTPAPIEX_SAS_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_SAS_Create_From_String, TEST_HTTPAPIEX_SAS_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_SAS_ExecuteRequest, HTTPAPIEX_OK, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_OK, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_RETURNS(BUFFER_new, TEST_BUFFER_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(BUFFER_create, TEST_BUFFER_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(BUFFER_build, 0, 1);

    REGISTER_GLOBAL_MOCK_RETURNS(json_parse_string, TEST_JSON_VALUE, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, (JSON_Object*)1);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, "a");

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_SasToken, my_IoTHubClient_Auth_Get_SasToken);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_SasToken, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_x509_info, my_IoTHubClient_Auth_Get_x509_info);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_x509_info, __LINE__);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceKey, TEST_DEVICE_ID);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceId, TEST_DEVICE_ID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_DeviceId, NULL);

    REGISTER_GLOBAL_MOCK_RETURNS(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK, HTTP_HEADERS_ERROR);
    REGISTER_GLOBAL_MOCK_RETURNS(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_OK, HTTP_HEADERS_ERROR);

    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_OK, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_SAS_ExecuteRequest, HTTPAPIEX_OK, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_SetOption, HTTPAPIEX_OK, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_RETURNS(Blob_CreateHttpConnection, TEST_HTTPAPIEX_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(Blob_PutBlock, BLOB_OK, BLOB_ERROR);
    REGISTER_GLOBAL_MOCK_RETURNS(Blob_PutBlockList, BLOB_OK, BLOB_ERROR);

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

    reset_test_data();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_Create_sas_token_succeeds)
{
    //arrange
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);

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
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);

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
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509_ECC);

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

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

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
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    IoTHubClient_LL_UploadToBlob_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_InitializeUpload_SAS_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 1;
    char* openSslEngine = "pkcs11";
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_InitializeUpload_X509_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_X509;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_InitializeUpload_X509_ALL_OPTIONS_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_X509;
    size_t blobUploadTimeoutSecs = 5;
    bool curlEnableVerboseLogging = true;
    const char* networkInterface = "eth0";
    bool useTlsRenegotiation = true;
    int x509privatekeyType = 1;
    const char* openSslEngine = "pkcs11";
    const char* trustedCertificates = "test cert";
    HTTP_PROXY_OPTIONS proxyOptions;
    proxyOptions.host_address = "proxy.address.com";
    proxyOptions.username = "proxyusername";
    proxyOptions.password = "proxypassword";

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509, TEST_AUTH_HANDLE);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_BLOB_UPLOAD_TIMEOUT_SECS, &blobUploadTimeoutSecs);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_CURL_VERBOSE, &curlEnableVerboseLogging);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_BLOB_UPLOAD_TLS_RENEGOTIATION, &useTlsRenegotiation);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_OPENSSL_PRIVATE_KEY_TYPE, &x509privatekeyType);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_OPENSSL_ENGINE, openSslEngine);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_TRUSTED_CERT, trustedCertificates);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_NETWORK_INTERFACE_UPLOAD_TO_BLOB, networkInterface);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_HTTP_PROXY, &proxyOptions);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutSecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, &proxyOptions);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_InitializeUpload_SAS_failure_checks)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutSecs = 5;
    bool curlEnableVerboseLogging = true;
    const char* networkInterface = "eth0";
    bool useTlsRenegotiation = true;
    int x509privatekeyType = 1;
    const char* openSslEngine = "pkcs11";
    const char* trustedCertificates = "test cert";
    HTTP_PROXY_OPTIONS proxyOptions;
    proxyOptions.host_address = "proxy.address.com";
    proxyOptions.username = "proxyusername";
    proxyOptions.password = "proxypassword";

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_BLOB_UPLOAD_TIMEOUT_SECS, &blobUploadTimeoutSecs);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_CURL_VERBOSE, &curlEnableVerboseLogging);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_BLOB_UPLOAD_TLS_RENEGOTIATION, &useTlsRenegotiation);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_OPENSSL_PRIVATE_KEY_TYPE, &x509privatekeyType);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_OPENSSL_ENGINE, openSslEngine);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_TRUSTED_CERT, trustedCertificates);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_NETWORK_INTERFACE_UPLOAD_TO_BLOB, networkInterface);
    (void)IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_HTTP_PROXY, &proxyOptions);
    
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutSecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, &proxyOptions);
    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // arrange
            char error_msg[64];

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(
                h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);

            ///assert
            sprintf(error_msg, "On failed call %lu", (unsigned long)i);
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, error_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_InitializeUpload_NULL_handle_fails)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(NULL, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_InitializeUpload_NULL_destination_file_fails)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;
    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, NULL, &uploadCorrelationId, &azureBlobSasUri);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_InitializeUpload_NULL_correlation_arg_fails)
{
    //arrange
    char* azureBlobSasUri;
    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, NULL, &azureBlobSasUri);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_InitializeUpload_NULL_SAS_URI_arg_fails)
{
    //arrange
    char* uploadCorrelationId;
    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_CreateContext_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 1;
    char* openSslEngine = "pkcs11";
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(uploadContext);

    //cleanup
    IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_CreateContext_failure_checks)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();
    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // arrange
            char error_msg[64];

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext =
                IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);

            ///assert
            sprintf(error_msg, "On failed call %lu", (unsigned long)i);
            ASSERT_IS_NULL(uploadContext, error_msg);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_CreateContext_NULL_handle_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(NULL, TEST_BLOB_SAS_URI);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(uploadContext);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_CreateContext_NULL_SAS_URI_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext =
        IoTHubClient_LL_UploadToBlob_CreateContext((IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE)0x4567, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(uploadContext);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_DestroyContext_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(TEST_HTTPAPIEX_HANDLE));
    STRICT_EXPECTED_CALL(Blob_ClearBlockIdList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_DestroyContext_NULL_handle_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IoTHubClient_LL_UploadToBlob_DestroyContext(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_NotifyCompletion_SAS_Success_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_NotifyCompletion(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);

    //act
    result = IoTHubClient_LL_UploadToBlob_NotifyCompletion(h, uploadCorrelationId, true, 200, "All good");

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_NotifyCompletion_X509_Success_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_X509;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_X509, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_NotifyCompletion(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);

    //act
    result = IoTHubClient_LL_UploadToBlob_NotifyCompletion(h, uploadCorrelationId, true, 200, "All good");

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_NotifyCompletion_NULL_responseContent_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_NotifyCompletion(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);

    //act
    result = IoTHubClient_LL_UploadToBlob_NotifyCompletion(h, uploadCorrelationId, true, 200, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_NotifyCompletion_failure_checks)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    IOTHUB_CLIENT_RESULT result;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_NotifyCompletion(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // arrange
            char error_msg[64];

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = IoTHubClient_LL_UploadToBlob_NotifyCompletion(h, uploadCorrelationId, true, 200, "All good");

            ///assert
            sprintf(error_msg, "On failed call %lu", (unsigned long)i);
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, error_msg);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_NotifyCompletion_NULL_handle_fails)
{
    //arrange
    const char* uploadCorrelationId = "correlationid";

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_NotifyCompletion(NULL, uploadCorrelationId, true, 200, "All good");

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_NotifyCompletion_NULL_correlation_id_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_NotifyCompletion(
        (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE)0x4567, NULL, true, 200, "All good");

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_PutBlock_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext;

    #define blockDataSize 6
    const uint8_t blockData[blockDataSize] = { 'a', 'b', 'c', 'd', 'e', 'f' };

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();
    uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);
    ASSERT_IS_NOT_NULL(uploadContext);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(BUFFER_create(blockData, blockDataSize));
    STRICT_EXPECTED_CALL(Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));


    //act
    result = IoTHubClient_LL_UploadToBlob_PutBlock(uploadContext, 0 /* block number*/, blockData, blockDataSize);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_PutBlock_failure_checks)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext;

    #define blockDataSize 6
    const uint8_t blockData[blockDataSize] = { 'a', 'b', 'c', 'd', 'e', 'f' };

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();
    uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);
    ASSERT_IS_NOT_NULL(uploadContext);

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(BUFFER_create(blockData, blockDataSize));
    STRICT_EXPECTED_CALL(Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // arrange
            char error_msg[64];

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = IoTHubClient_LL_UploadToBlob_PutBlock(uploadContext, 0 /* block number*/, blockData, blockDataSize);

            ///assert
            sprintf(error_msg, "On failed call %lu", (unsigned long)i);
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, error_msg);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_PutBlock_NULL_handle_fails)
{
    //arrange
    IOTHUB_CLIENT_RESULT result;

    #define blockDataSize 6
    const uint8_t blockData[blockDataSize] = { 'a', 'b', 'c', 'd', 'e', 'f' };

    umock_c_reset_all_calls();

    //act
    result = IoTHubClient_LL_UploadToBlob_PutBlock(NULL, 0 /* block number*/, blockData, blockDataSize);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_PutBlock_NULL_data_fails)
{
    //arrange
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE)0x4567;

    #define blockDataSize 6

    umock_c_reset_all_calls();

    //act
    result = IoTHubClient_LL_UploadToBlob_PutBlock(uploadContext, 0 /* block number*/, NULL, blockDataSize);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_PutBlock_zero_dataSize_fails)
{
    //arrange
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE)0x4567;

    #define blockDataSize 6
    const uint8_t blockData[blockDataSize] = { 'a', 'b', 'c', 'd', 'e', 'f' };

    umock_c_reset_all_calls();

    //act
    result = IoTHubClient_LL_UploadToBlob_PutBlock(uploadContext, 0 /* block number*/, blockData, 0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_PutBlockList_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext;

    #define blockDataSize 6
    const uint8_t blockData[blockDataSize] = { 'a', 'b', 'c', 'd', 'e', 'f' };

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();
    uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);
    ASSERT_IS_NOT_NULL(uploadContext);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(BUFFER_create(blockData, blockDataSize));
    STRICT_EXPECTED_CALL(Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    result = IoTHubClient_LL_UploadToBlob_PutBlock(uploadContext, 0 /* block number*/, blockData, blockDataSize);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));

    //act
    result = IoTHubClient_LL_UploadToBlob_PutBlockList(uploadContext);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_PutBlockList_failure_checks)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext;

    #define blockDataSize 6
    const uint8_t blockData[blockDataSize] = { 'a', 'b', 'c', 'd', 'e', 'f' };

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();
    uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);
    ASSERT_IS_NOT_NULL(uploadContext);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(BUFFER_create(blockData, blockDataSize));
    STRICT_EXPECTED_CALL(Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    result = IoTHubClient_LL_UploadToBlob_PutBlock(uploadContext, 0 /* block number*/, blockData, blockDataSize);

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));
    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // arrange
            char error_msg[64];

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = IoTHubClient_LL_UploadToBlob_PutBlockList(uploadContext);

            ///assert
            sprintf(error_msg, "On failed call %lu", (unsigned long)i);
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, error_msg);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_PutBlockList_NULL_handle_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_PutBlockList(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks_handle_NULL_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks(NULL, FileUpload_GetData_Callback, &blobUploadContext);
    
    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks_NULL_callback_fails)
{
    //arrange
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks(TEST_U2B_CONTEXT_HANDLE, NULL, &blobUploadContext);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks_20x_succeeds)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext;

    #define numberOfUploads 20
    #define blockSize 10
    #define dataSize (numberOfUploads * blockSize)
    uint8_t data[dataSize];
    (void)memset(data, 1, dataSize);

    blobUploadContext.source = data; 
    blobUploadContext.size = dataSize;
    blobUploadContext.toUpload = dataSize;
    blobUploadContext.maxBlockSize = blockSize;
    blobUploadContext.abortOnCount = DO_NOT_ABORT_UPLOAD;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();
    uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);
    ASSERT_IS_NOT_NULL(uploadContext);

    umock_c_reset_all_calls();
    for (size_t i = 0; i < numberOfUploads; i++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(Blob_PutBlock(
            TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));

    //act
    result = IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks(uploadContext, FileUpload_GetData_Callback, &blobUploadContext);

    //assert
    #undef numberOfUploads
    #undef blockSize
    #undef dataSize
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks_abort_on_2nd_fails)
{
    //arrange
    char* uploadCorrelationId;
    char* azureBlobSasUri;
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext;

    #define numberOfUploads 3
    #define blockSize 10
    #define dataSize (numberOfUploads * blockSize)
    uint8_t data[dataSize];
    (void)memset(data, 1, dataSize);

    blobUploadContext.source = data; 
    blobUploadContext.size = dataSize;
    blobUploadContext.toUpload = dataSize;
    blobUploadContext.maxBlockSize = blockSize;
    blobUploadContext.abortOnCount = 2;

    IOTHUB_CREDENTIAL_TYPE credentialType = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
    size_t blobUploadTimeoutMillisecs = 0;
    bool curlEnableVerboseLogging = false;
    char* networkInterface = NULL;
    bool useTlsRenegotiation = false;
    int x509privatekeyType = 0;
    char* openSslEngine = NULL;
    char* trustedCertificates = NULL;
    HTTP_PROXY_OPTIONS* proxyOptions = NULL;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(credentialType);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_InitializeUpload(
        credentialType, blobUploadTimeoutMillisecs, curlEnableVerboseLogging, networkInterface,
        useTlsRenegotiation, x509privatekeyType, openSslEngine, trustedCertificates, proxyOptions);
    result = IoTHubClient_LL_UploadToBlob_InitializeUpload(h, TEST_DESTINATION_FILENAME, &uploadCorrelationId, &azureBlobSasUri);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_CreateContext();
    uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(h, azureBlobSasUri);
    ASSERT_IS_NOT_NULL(uploadContext);

    umock_c_reset_all_calls();
    for (size_t count = 1; count < blobUploadContext.abortOnCount; count++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(Blob_PutBlock(
            TEST_HTTPAPIEX_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, TEST_SINGLYLINKEDLIST_HANDLE, IGNORED_PTR_ARG, NULL));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    }

    //act
    result = IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks(uploadContext, FileUpload_GetData_Callback, &blobUploadContext);

    //assert
    #undef numberOfUploads
    #undef blockSize
    #undef dataSize
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);

    //cleanup
    IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
    IoTHubClient_LL_UploadToBlob_Destroy(h);
    my_gballoc_free(uploadCorrelationId);
    my_gballoc_free(azureBlobSasUri);
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
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // Within mallocAndStrcpy_s hook.

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_CERT, TEST_CERT);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_openssl_private_key_type_succeeds)
{
    int privateKeyType = 1;
    //arrange
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(privateKeyType)));
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_OPENSSL_PRIVATE_KEY_TYPE, &privateKeyType);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_openssl_engine_type_succeeds)
{
    const char* engine = "pkcs11";
    //arrange
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, engine));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // Within mallocAndStrcpy_s hook.

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_OPENSSL_ENGINE, engine);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_Network_Interface)
{
    //arrange
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    const char* networkInterface = "eth0";
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, networkInterface));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // Within mallocAndStrcpy_s hook.

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_NETWORK_INTERFACE_UPLOAD_TO_BLOB, networkInterface);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_x509_cert_twice_succeeds)
{
    //arrange
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_X509_CERT, TEST_CERT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // Within mallocAndStrcpy_s hook.
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
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_PRIVATE));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // Within mallocAndStrcpy_s hook.

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
    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // Within mallocAndStrcpy_s hook.

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

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
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

TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_SetOption_tls_renegotiation_succeeds)
{
    //arrange
    bool set_tls_renegotiaton = true;

    setExpectedCallsFor_IoTHubClient_LL_UploadToBlob_Create(IOTHUB_CREDENTIAL_TYPE_X509);
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE h = IoTHubClient_LL_UploadToBlob_Create(&TEST_CONFIG_SAS, TEST_AUTH_HANDLE);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob_SetOption(h, OPTION_BLOB_UPLOAD_TLS_RENEGOTIATION, &set_tls_renegotiaton);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_UploadToBlob_Destroy(h);
}

END_TEST_SUITE(iothubclient_ll_uploadtoblob_ut)
