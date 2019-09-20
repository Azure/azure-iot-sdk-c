// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstring>
#else
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#endif

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* s)
{
    free(s);
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "azure_uhttp_c/uhttp.h"
#undef ENABLE_MOCKS

#include "internal/blob.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "internal/iothub_client_ll_uploadtoblob.h"

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

#undef ENABLE_MOCKS

TEST_DEFINE_ENUM_TYPE       (HTTP_CLIENT_REQUEST_TYPE, HTTP_CLIENT_REQUEST_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (HTTP_CLIENT_REQUEST_TYPE, HTTP_CLIENT_REQUEST_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE       (BLOB_RESULT, BLOB_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE (BLOB_RESULT, BLOB_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);

static const char* const TEST_SAS_URI = "http://test_uri.address.com/here/follows/something?param1=value1&param2=value2";
static const char* const TEST_CERTIFICATE = "test_certificate";

static const unsigned char* TEST_SOURCE = (const unsigned char*)0x3;
static const size_t TEST_SOURCE_LENGTH = 3;

static IO_INTERFACE_DESCRIPTION* TEST_INTERFACE_DESC = (IO_INTERFACE_DESCRIPTION*)0x4;
static XIO_HANDLE TEST_XIO_HANDLE = (XIO_HANDLE)0x5;

static ON_HTTP_REQUEST_CALLBACK g_on_request_callback = NULL;
static void* g_on_request_callback_ctx = NULL;
static bool g_call_http_reply = false;

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

static void my_BUFFER_delete(BUFFER_HANDLE h)
{
    my_gballoc_free(h);
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE h)
{
    my_gballoc_free(h);
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    char* temp = (char*)my_gballoc_malloc(strlen(psz) + 1);
    ASSERT_IS_NOT_NULL(temp);
    (void)memcpy(temp, psz, strlen(psz) + 1);
    return (STRING_HANDLE)temp;
}

static void my_STRING_delete(STRING_HANDLE h)
{
    my_gballoc_free((void*)h);
}

static STRING_HANDLE my_Azure_Base64_Encode_Bytes(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (STRING_HANDLE)my_gballoc_malloc(1);
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

#define TEST_HTTPCOLONBACKSLASHBACKSLACH "http://"
#define TEST_HOSTNAME_1 "host.name"
#define TEST_RELATIVE_PATH_1 "/here/follows/something?param1=value1&param2=value2"
#define TEST_VALID_SASURI_1 TEST_HTTPCOLONBACKSLASHBACKSLACH TEST_HOSTNAME_1 TEST_RELATIVE_PATH_1

#define X_MS_BLOB_TYPE "x-ms-blob-type"
#define BLOCK_BLOB "BlockBlob"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

static const unsigned int TwoHundred = 200;
static const unsigned int FourHundredFour = 404;

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
    size_t iterations;
} BLOB_UPLOAD_CONTEXT;

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT FileUpload_GetData_Callback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* _uploadContext)
{
    BLOB_UPLOAD_CONTEXT* uploadContext = (BLOB_UPLOAD_CONTEXT*)_uploadContext;

    uploadContext->lastResult = result;
    uploadContext->lastData = data;
    uploadContext->lastSize = size;

    if (result == FILE_UPLOAD_OK)
    {
        if (data != NULL && size != NULL)
        {
            if (uploadContext->iterations > 0)
            {
                // Upload next block
                *data = (unsigned char*)uploadContext->source;
                *size = uploadContext->toUpload;
                uploadContext->iterations--;
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
            // Last call failed
            *data = NULL;
            *size = 0;
        }
    }
    else
    {
        // Check failure
        *data = NULL;
        *size = 0;
    }
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

BEGIN_TEST_SUITE(blob_uhttp_ut)

TEST_SUITE_INITIALIZE(TestSuiteInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    (void)umocktypes_charptr_register_types();
    (void)umocktypes_bool_register_types();
    REGISTER_TYPE(HTTP_CLIENT_REQUEST_TYPE, HTTP_CLIENT_REQUEST_TYPE);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_INTERFACE_DESC);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(platform_get_default_tlsio, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_create, my_uhttp_client_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_destroy, my_uhttp_client_destroy);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_trusted_cert, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_trusted_cert, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_dowork, my_uhttp_client_dowork);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_trace, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_trace, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_open, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_open, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_get_underlying_xio, TEST_XIO_HANDLE);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_close, my_uhttp_client_close);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_execute_request, my_uhttp_client_execute_request);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_execute_request, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);
    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_unbuild, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_unbuild, __LINE__);
    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_build, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_build, __LINE__);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Encode_Bytes, my_Azure_Base64_Encode_Bytes);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Encode_Bytes, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, "a");
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_ERROR_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_OPEN_COMPLETE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_REQUEST_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_CLOSED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT);

    //testValidBufferHandle = BUFFER_create((const unsigned char*)"a", 1);
    //ASSERT_IS_NOT_NULL(testValidBufferHandle);
    umock_c_reset_all_calls();
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

static void setup_close_http_client_mocks(void)
{
    STRICT_EXPECTED_CALL(uhttp_client_close(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void setup_create_http_client_mocks(bool use_proxy, bool use_cert)
{
    if (use_proxy)
    {
    }

    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(uhttp_client_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (use_cert)
    {
        STRICT_EXPECTED_CALL(uhttp_client_set_trusted_cert(IGNORED_PTR_ARG, TEST_CERTIFICATE));
    }
    STRICT_EXPECTED_CALL(uhttp_client_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 443, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_set_trace(IGNORED_PTR_ARG, true, true)).CallCannotFail();
}

static void setup_send_http_data_mocks(void)
{
    STRICT_EXPECTED_CALL(uhttp_client_execute_request(IGNORED_PTR_ARG, HTTP_CLIENT_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_unbuild(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
}

static void setup_blob_upload_block_mocks(void)
{
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();

    // Send http handle mocks
    setup_send_http_data_mocks();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setup_upload_multiple_blocks_mocks(size_t iterations)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    setup_create_http_client_mocks(false, true);
    STRICT_EXPECTED_CALL(BUFFER_new());
    for (size_t index = 0; index < iterations; index++)
    {
        setup_blob_upload_block_mocks();
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_NUM_ARG));
    }
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    setup_send_http_data_mocks();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    setup_close_http_client_mocks();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_uri_NULL_fail)
{
    //arrange
    unsigned int httpStatus = 0;
    BUFFER_HANDLE httpResponse = (BUFFER_HANDLE)0x1234;
    BLOB_UPLOAD_CONTEXT blob_ctx = { 0 };
    blob_ctx.toUpload = 1024;
    blob_ctx.source = (unsigned char*)0x2345;

    //act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(NULL, TEST_CERTIFICATE, NULL, FileUpload_GetData_Callback, &blob_ctx, &httpStatus, httpResponse);

    //assert
    ASSERT_ARE_NOT_EQUAL(BLOB_RESULT, BLOB_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_success)
{
    //arrange
    unsigned int httpStatus = 0;
    BUFFER_HANDLE httpResponse = (BUFFER_HANDLE)0x1234;
    BLOB_UPLOAD_CONTEXT blob_ctx = { 0 };
    blob_ctx.toUpload = 1024;
    blob_ctx.source = (unsigned char*)0x2345;
    blob_ctx.iterations = 1;

    setup_upload_multiple_blocks_mocks(1);

    //act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_SAS_URI, TEST_CERTIFICATE, NULL, FileUpload_GetData_Callback, &blob_ctx, &httpStatus, httpResponse);

    //assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    unsigned int httpStatus = 0;
    BUFFER_HANDLE httpResponse = (BUFFER_HANDLE)0x1234;

    setup_upload_multiple_blocks_mocks(1);
    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        BLOB_UPLOAD_CONTEXT blob_ctx = { 0 };
        blob_ctx.toUpload = 1024;
        blob_ctx.source = (unsigned char*)0x2345;
        blob_ctx.iterations = 1;

        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_SAS_URI, TEST_CERTIFICATE, NULL, FileUpload_GetData_Callback, &blob_ctx, &httpStatus, httpResponse);

            //assert
            ASSERT_ARE_NOT_EQUAL(BLOB_RESULT, BLOB_OK, result, "IoTHubClient_LL_UploadToBlob_Impl failure in test %lu/%lu", index, count);
        }
    }
    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_10_iterations_success)
{
    //arrange
    unsigned int httpStatus = 0;
    size_t iterations = 2;
    BUFFER_HANDLE httpResponse = (BUFFER_HANDLE)0x1234;
    BLOB_UPLOAD_CONTEXT blob_ctx = { 0 };
    blob_ctx.toUpload = 1024 * iterations;
    blob_ctx.source = (unsigned char*)0x2345;
    blob_ctx.iterations = iterations;

    setup_upload_multiple_blocks_mocks(iterations);

    //act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_SAS_URI, TEST_CERTIFICATE, NULL, FileUpload_GetData_Callback, &blob_ctx, &httpStatus, httpResponse);

    //assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup 
}

END_TEST_SUITE(blob_uhttp_ut);
