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
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/shared_util_options.h"
#undef ENABLE_MOCKS

#include "internal/blob.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "internal/iothub_client_ll_uploadtoblob.h"
#undef ENABLE_MOCKS

/*helps when enums are not matched*/
#ifdef malloc
#undef malloc
#endif

TEST_DEFINE_ENUM_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);

static HTTPAPIEX_HANDLE my_HTTPAPIEX_Create(const char* hostName)
{
    (void)hostName;
    return (HTTPAPIEX_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPAPIEX_Destroy(HTTPAPIEX_HANDLE handle)
{
    my_gballoc_free(handle);
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

TEST_DEFINE_ENUM_TYPE(BLOB_RESULT, BLOB_RESULT_VALUES);

#define TEST_HTTPCOLONBACKSLASHBACKSLACH "http://"
#define TEST_HOSTNAME_1 "host.name"
#define TEST_RELATIVE_PATH_1 "/here/follows/something?param1=value1&param2=value2"
#define TEST_VALID_SASURI_1 TEST_HTTPCOLONBACKSLASHBACKSLACH TEST_HOSTNAME_1 TEST_RELATIVE_PATH_1

#define X_MS_BLOB_TYPE "x-ms-blob-type"
#define BLOCK_BLOB "BlockBlob"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static BUFFER_HANDLE testValidBufferHandle; /*assigned in TEST_SUITE_INITIALIZE*/
static unsigned int httpResponse; /*used as out parameter in every call to Blob_....*/
static const unsigned int TwoHundred = 200;
static const unsigned int FourHundredFour = 404;


/**
 * BLOB_UPLOAD_CONTEXT and FileUpload_GetData_Callback
 * allow to simulate a user who wants to upload
 * a source of size "size".
 */
typedef struct BLOB_UPLOAD_CONTEXT_TAG
{
    const unsigned char* source; /* source to upload */
    size_t size; /* size of the source */
    size_t toUpload; /* size not yet uploaded */
}BLOB_UPLOAD_CONTEXT;

BLOB_UPLOAD_CONTEXT context;

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT FileUpload_GetData_Callback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* _uploadContext)
{
    BLOB_UPLOAD_CONTEXT* uploadContext = (BLOB_UPLOAD_CONTEXT*) _uploadContext;

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

/**
 * BLOB_UPLOAD_CONTEXT_FAKE and FileUpload_GetFakeData_Callback
 * allow to simulate a user who wants to sent
 * blocksCount blocks of size blockSize
 */
typedef struct BLOB_UPLOAD_CONTEXT_FAKE_TAG
{
    unsigned char* fakeData; /* fake data allocated */
    size_t blockSize; /* size of the block */
    unsigned int blocksCount; /* number of block wanted */
    unsigned int blockSent; /* number block already sent */
    int abortOnBlockNumber; /* the callback shall abort if the block asked is equal to this value */
}BLOB_UPLOAD_CONTEXT_FAKE;

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT FileUpload_GetFakeData_Callback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* _uploadContext)
{
    BLOB_UPLOAD_CONTEXT_FAKE* uploadContext = (BLOB_UPLOAD_CONTEXT_FAKE*) _uploadContext;

    if (uploadContext->fakeData == NULL)
    {
        // Allocate the fake data exactly once (actual LL layer just reads through existing buffer)
        uploadContext->fakeData = (unsigned char*)gballoc_malloc(uploadContext->blockSize);
    }

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
    else if (uploadContext->blockSent >= uploadContext->blocksCount)
    {
        // Everything has been uploaded
        *data = NULL;
        *size = 0;
    }
    else
    {
        // Upload next block
        uploadContext->blockSent++;
        *data = uploadContext->fakeData;
        *size = (uploadContext->fakeData != NULL) ? uploadContext->blockSize : 0;
    }

    if (uploadContext->abortOnBlockNumber + 1 == (int)uploadContext->blockSent)
        return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT;

    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

BEGIN_TEST_SUITE(blob_ut)

TEST_SUITE_INITIALIZE(TestSuiteInitialize)
{
    (void)umock_c_init(on_umock_c_error);

    (void)umocktypes_charptr_register_types();

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Create, my_HTTPAPIEX_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_Create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Destroy, my_HTTPAPIEX_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Encode_Bytes, my_Azure_Base64_Encode_Bytes);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Encode_Bytes, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_SetOption, HTTPAPIEX_OK, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, "a");
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE);
    REGISTER_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT);
    REGISTER_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT);

    testValidBufferHandle = BUFFER_create((const unsigned char*)"a", 1);
    ASSERT_IS_NOT_NULL(testValidBufferHandle);
    umock_c_reset_all_calls();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{

    BUFFER_delete(testValidBufferHandle);

    umock_c_deinit();
}

static void reset_test_data()
{
    memset(&context, 0, sizeof(context));
}

TEST_FUNCTION_INITIALIZE(Setup)
{
    umock_c_reset_all_calls();
    reset_test_data();
}

TEST_FUNCTION_CLEANUP(Cleanup)
{
    reset_test_data();
}

/*Tests_SRS_BLOB_02_001: [ If SASURI is NULL then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_with_NULL_SasUri_fails)
{
    ///arrange

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(NULL, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);

    ///cleanup

}

/*Tests_SRS_BLOB_02_002: [ If getDataCallback is NULL then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_with_NULL_getDataCallBack_and_non_NULL_context_fails)
{
    ///arrange

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, NULL, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);

    ///cleanup

}

/*Tests_SRS_BLOB_02_032: [ Otherwise, `Blob_UploadMultipleBlocksFromSasUri` shall succeed and return `BLOB_OK`. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_succeeds_when_HTTP_status_code_is_404)
{
    ///arrange
    unsigned char c = '3';
    size_t size = 1;
    context.size = size;
    context.source = &c;
    context.toUpload = context.size;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a copy of the hostname */
        .IgnoreArgument_size();

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create("host.name")); /*this is creating the httpapiex handle to storage (it is always the same host)*/
    STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

    /*uploading blocks (Put Block)*/
    for (size_t blockNumber = 0;blockNumber < (size - 1) / (4 * 1024 * 1024) + 1;blockNumber++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(&c + blockNumber * 4 * 1024 * 1024,
            (blockNumber != (size - 1) / (4 * 1024 * 1024)) ? 4 * 1024 * 1024 : (size - 1) % (4 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
        )); /*this is the content to be uploaded by this call*/

        /*here some sprintf happens and that produces a string in the form: 000000...049999*/
        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6)) /*this is converting the produced blockID string to a base64 representation*/
            .IgnoreArgument_source();

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "<Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the XML*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_construct("/here/follows/something?param1=value1&param2=value2")); /*this is building the relativePath*/

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=block&blockid=")) /*this is building the relativePath*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the relativePath by adding the blockId (base64 encoded_*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path as const char* */
            .IgnoreArgument_handle();

        int responseCode = 404; /*not found*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, &httpResponse, NULL, testValidBufferHandle))
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestContent()
            .CopyOutArgumentBuffer_statusCode(&responseCode, sizeof(responseCode));

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the relativePath*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the blockID string to a base64 representation*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*this was the content to be uploaded*/
            .IgnoreArgument_handle();
    }

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)) /*this is the HTTPAPIEX handle*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the copy of hte hostname*/
        .IgnoreArgument_ptr();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_BLOB_02_025: [ If HTTPAPIEX_ExecuteRequest fails, then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_HTTP_ERROR. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_fails_when_HTTPAPIEX_ExecuteRequest_fails)
{
    ///arrange
    unsigned char c = '3';
    size_t size = 1;
    context.size = size;
    context.source = &c;
    context.toUpload = context.size;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a copy of the hostname */
        .IgnoreArgument_size();

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create("host.name")); /*this is creating the httpapiex handle to storage (it is always the same host)*/
    STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

    /*uploading blocks (Put Block)*/
    for (size_t blockNumber = 0;blockNumber < (size - 1) / (4 * 1024 * 1024) + 1;blockNumber++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(&c + blockNumber * 4 * 1024 * 1024,
            (blockNumber != (size - 1) / (4 * 1024 * 1024)) ? 4 * 1024 * 1024 : (size - 1) % (4 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
        )); /*this is the content to be uploaded by this call*/

        /*here some sprintf happens and that produces a string in the form: 000000...049999*/
        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6)) /*this is converting the produced blockID string to a base64 representation*/
            .IgnoreArgument_source();

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "<Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the XML*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_construct("/here/follows/something?param1=value1&param2=value2")); /*this is building the relativePath*/

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=block&blockid=")) /*this is building the relativePath*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the relativePath by adding the blockId (base64 encoded_*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path as const char* */
            .IgnoreArgument_handle();

        int responseCode = 200; /*ok*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, &httpResponse, NULL, testValidBufferHandle))
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestContent()
            .CopyOutArgumentBuffer_statusCode(&responseCode, sizeof(responseCode))
            .SetReturn(HTTPAPIEX_ERROR);

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the relativePath*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the blockID string to a base64 representation*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*this was the content to be uploaded*/
            .IgnoreArgument_handle();
    }

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)) /*this is the HTTPAPIEX handle*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the copy of hte hostname*/
        .IgnoreArgument_ptr();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_HTTP_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}


/*Tests_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_ERROR` ]  */
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_fails_when_BUFFER_create_fails)
{
    ///arrange
    unsigned char c = '3';
    context.size = 1;
    context.source = &c;
    context.toUpload = context.size;

    STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_HOSTNAME_1) + 1));
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_HOSTNAME_1));
        {
            STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

            STRICT_EXPECTED_CALL(BUFFER_create(&c, 1))
                .SetReturn(NULL);

            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
                 .IgnoreArgument_handle();

            STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG))
                .IgnoreArgument_handle();
        }
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument_ptr();
    }

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ERROR, result);

    ///cleanup
}

/*Tests_SRS_BLOB_02_007: [ If HTTPAPIEX_Create fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_fails_when_HTTPAPIEX_Create_fails)
{
    ///arrange
    unsigned char c = '3';
    context.size = 1;
    context.source = &c;
    context.toUpload = context.size;

    STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_HOSTNAME_1) + 1));
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_HOSTNAME_1))
            .SetReturn(NULL)
            ;
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument_ptr();
    }

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_BLOB_02_016: [ If the hostname copy cannot be made then then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_fails_when_malloc_fails)
{
    ///arrange
    unsigned char c = '3';
    context.size = 1;
    context.source = &c;
    context.toUpload = context.size;

    STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_HOSTNAME_1) + 1))
        .SetReturn(NULL)
        ;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_when_SasUri_is_wrong_fails_1)
{
    ///arrange
    unsigned char c = '3';
    context.size = 1;
    context.source = &c;
    context.toUpload = context.size;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https:/h.h/doms", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL); /*wrong format for protocol, notice it is actually http:\h.h\doms (missing a \ from http)*/

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_when_SasUri_is_wrong_fails_2)
{
    ///arrange
    unsigned char c = '3';
    context.size = 1;
    context.source = &c;
    context.toUpload = context.size;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL); /*there's no relative path here*/

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

static void Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path_Impl(HTTP_PROXY_OPTIONS *proxyOptions)
{
    /*the following sizes have been identified as "important to be tested*/
    /*1B, 4MB-1, 4MB, 4MB+1, 64MB, 64MB+1, 68MB-1, 68MB, 68MB+1*/
    size_t sizes[] = {
        1,

        4 * 1024 * 1024 - 1,
        4 * 1024 * 1024,
        4 * 1024 * 1024 + 1,

        64 * 1024 * 1024 - 1,
        64 * 1024 * 1024,
        64 * 1024 * 1024 + 1,

        68 * 1024 * 1024 - 1,
        68 * 1024 * 1024,
        68 * 1024 * 1024 + 1,
    };

    for (size_t iSize = 0; iSize < sizeof(sizes) / sizeof(sizes[0]);iSize++)
    {
        umock_c_reset_all_calls();
        ///arrange
        unsigned char * content = (unsigned char*)gballoc_malloc(sizes[iSize]);
        ASSERT_IS_NOT_NULL(content);

        umock_c_reset_all_calls();

        memset(content, '3', sizes[iSize]);
        content[0] = '0';
        content[sizes[iSize] - 1] = '4';
        context.size = sizes[iSize];
        context.source = content;
        context.toUpload = context.size;

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a copy of the hostname */
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(HTTPAPIEX_Create("h.h")); /*this is creating the httpapiex handle to storage (it is always the same host)*/
        if (proxyOptions != NULL)
        {
            STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_HTTP_PROXY, proxyOptions));
        }
        STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

        /*uploading blocks (Put Block)*/
        for (size_t blockNumber = 0;blockNumber < (sizes[iSize] - 1) / (4 * 1024 * 1024) + 1;blockNumber++)
        {
            STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 4 * 1024 * 1024,
                (blockNumber != (sizes[iSize] - 1) / (4 * 1024 * 1024)) ? 4 * 1024 * 1024 : (sizes[iSize] - 1) % (4 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
            )); /*this is the content to be uploaded by this call*/

            /*here some sprintf happens and that produces a string in the form: 000000...049999*/
            STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6)) /*this is converting the produced blockID string to a base64 representation*/
                .IgnoreArgument_source();

            STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "<Latest>")) /*this is building the XML*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the XML*/
                .IgnoreArgument_s1()
                .IgnoreArgument_s2();
            STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</Latest>")) /*this is building the XML*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relativePath*/

            STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=block&blockid=")) /*this is building the relativePath*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the relativePath by adding the blockId (base64 encoded_*/
                .IgnoreArgument_s1()
                .IgnoreArgument_s2();

            STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path as const char* */
                .IgnoreArgument_handle();

            STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, &httpResponse, NULL, testValidBufferHandle))
                .IgnoreArgument_handle()
                .IgnoreArgument_relativePath()
                .IgnoreArgument_requestContent();

            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the relativePath*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the blockID string to a base64 representation*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*this was the content to be uploaded*/
                .IgnoreArgument_handle();
        }

        /*this part is Put Block list*/
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</BlockList>")) /*This is closing the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relative path for the Put BLock list*/

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=blocklist")) /*This is still building relative path for Put Block list*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the XML as const char* so it can be passed to _ExecuteRequest*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)) /*this is creating the XML body as BUFFER_HANDLE*/
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_PUT,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG,
            &httpResponse,
            NULL,
            testValidBufferHandle
        ))
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestContent()
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            ;

        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*This is the XML as BUFFER_HANDLE*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is destroying the relative path for Put Block List*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)) /*this is the HTTPAPIEX handle*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the copy of hte hostname*/
            .IgnoreArgument_ptr();

        ///act
        BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, proxyOptions);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

        ///cleanup
        gballoc_free(content);
    }


}


/*Tests_SRS_BLOB_02_004: [ Blob_UploadFromSasUri shall copy from SASURI the hostname to a new const char*. ]*/
/*Tests_SRS_BLOB_02_006: [ Blob_UploadFromSasUri shall create a new HTTPAPI_EX_HANDLE by calling HTTPAPIEX_Create passing the hostname. ]*/
/*Tests_SRS_BLOB_02_008: [ Blob_UploadFromSasUri shall compute the relative path of the request from the SASURI parameter. ]*/
/*Tests_SRS_BLOB_02_009: [ Blob_UploadFromSasUri shall create an HTTP_HEADERS_HANDLE for the request HTTP headers carrying the following headers: ]*/
/*Tests_SRS_BLOB_02_010: [ Blob_UploadFromSasUri shall create a BUFFER_HANDLE from source and size parameters. ]*/
/*Tests_SRS_BLOB_02_012: [ Blob_UploadFromSasUri shall call HTTPAPIEX_ExecuteRequest passing the parameters previously build, httpStatus and httpResponse ]*/
/*Tests_SRS_BLOB_02_015: [ Otherwise, HTTPAPIEX_ExecuteRequest shall succeed and return BLOB_OK. ]*/
/*Tests_SRS_BLOB_32_001: [ If proxy was provided then Blob_UploadFromSasUri shall pass proxy information to HTTPAPI_EX_HANDLE by calling HTTPAPIEX_SetOption with the option name OPTION_HTTP_PROXY. ]*/
TEST_FUNCTION(Blob_UploadFromSasUri_with_proxy_happy_path)
{
    HTTP_PROXY_OPTIONS proxyOptions = { "a", 8888, NULL, NULL };
    Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path_Impl(&proxyOptions);
}


/*Tests_SRS_BLOB_02_017: [ Blob_UploadMultipleBlocksFromSasUri shall copy from SASURI the hostname to a new const char* ]*/
/*Tests_SRS_BLOB_02_018: [ Blob_UploadMultipleBlocksFromSasUri shall create a new HTTPAPI_EX_HANDLE by calling HTTPAPIEX_Create passing the hostname. ]*/
/*Tests_SRS_BLOB_02_019: [ Blob_UploadMultipleBlocksFromSasUri shall compute the base relative path of the request from the SASURI parameter. ]*/
/*Tests_SRS_BLOB_02_021: [ For every block returned by getDataCallback the following operations shall happen: ]*/
/*Tests_SRS_BLOB_99_002: [ If the size of the block returned by `getDataCallback` is 0 or if the data is NULL, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop. ]*/
/*Tests_SRS_BLOB_02_020: [ Blob_UploadMultipleBlocksFromSasUri shall construct a BASE64 encoded string from the block ID (000000... 049999) ]*/
/*Tests_SRS_BLOB_02_022: [ Blob_UploadMultipleBlocksFromSasUri shall construct a new relativePath from following string: base relativePath + "&comp=block&blockid=BASE64 encoded string of blockId" ]*/
/*Tests_SRS_BLOB_02_023: [ Blob_UploadMultipleBlocksFromSasUri shall create a BUFFER_HANDLE from source and size parameters. ]*/
/*Tests_SRS_BLOB_02_024: [ Blob_UploadMultipleBlocksFromSasUri shall call HTTPAPIEX_ExecuteRequest with a PUT operation, passing httpStatus and httpResponse. ]*/
/*Tests_SRS_BLOB_02_025: [ If HTTPAPIEX_ExecuteRequest fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_HTTP_ERROR. ]*/
/*Tests_SRS_BLOB_02_027: [ Otherwise Blob_UploadMultipleBlocksFromSasUri shall continue execution. ]*/
/*Tests_SRS_BLOB_02_028: [ Blob_UploadMultipleBlocksFromSasUri shall construct an XML string with the following content: ]*/
/*Tests_SRS_BLOB_02_029: [ Blob_UploadMultipleBlocksFromSasUri shall construct a new relativePath from following string: base relativePath + "&comp=blocklist" ]*/
/*Tests_SRS_BLOB_02_030: [ Blob_UploadMultipleBlocksFromSasUri shall call HTTPAPIEX_ExecuteRequest with a PUT operation, passing the new relativePath, httpStatus and httpResponse and the XML string as content. ]*/
/*Tests_SRS_BLOB_02_032: [ Otherwise, Blob_UploadMultipleBlocksFromSasUri shall succeed and return BLOB_OK. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path)
{
    Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path_Impl(NULL);
}



/*Tests_SRS_BLOB_02_037: [ If certificates is non-NULL then Blob_UploadMultipleBlocksFromSasUri shall pass certificates to HTTPAPI_EX_HANDLE by calling HTTPAPIEX_SetOption with the option name "TrustedCerts". ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_various_sizes_with_certificates_happy_path)
{
    /*the following sizes have been identified as "important to be tested*/
    /*1B, 4MB-1, 4MB, 4MB+1, 64MB, 64MB+1, 68MB-1, 68MB, 68MB+1*/
    size_t sizes[] = {
        1,

        4 * 1024 * 1024 - 1,
        4 * 1024 * 1024,
        4 * 1024 * 1024 + 1,

        64 * 1024 * 1024 - 1,
        64 * 1024 * 1024,
        64 * 1024 * 1024 + 1,

        68 * 1024 * 1024 - 1,
        68 * 1024 * 1024,
        68 * 1024 * 1024 + 1,
    };

    for (size_t iSize = 0; iSize < sizeof(sizes) / sizeof(sizes[0]);iSize++)
    {
        umock_c_reset_all_calls();
        ///arrange
        unsigned char * content = (unsigned char*)gballoc_malloc(sizes[iSize]);
        ASSERT_IS_NOT_NULL(content);

        umock_c_reset_all_calls();

        memset(content, '3', sizes[iSize]);
        content[0] = '0';
        content[sizes[iSize] - 1] = '4';
        context.size = sizes[iSize];
        context.source = content;
        context.toUpload = context.size;

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a copy of the hostname */
            .IgnoreArgument_size();

        STRICT_EXPECTED_CALL(HTTPAPIEX_Create("h.h")); /*this is creating the httpapiex handle to storage (it is always the same host)*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, "TrustedCerts", IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

                                                                                                             /*uploading blocks (Put Block)*/
        for (size_t blockNumber = 0;blockNumber < (sizes[iSize] - 1) / (4 * 1024 * 1024) + 1;blockNumber++)
        {
            STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 4 * 1024 * 1024,
                (blockNumber != (sizes[iSize] - 1) / (4 * 1024 * 1024)) ? 4 * 1024 * 1024 : (sizes[iSize] - 1) % (4 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
            )); /*this is the content to be uploaded by this call*/

            /*here some sprintf happens and that produces a string in the form: 000000...049999*/
            STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6)) /*this is converting the produced blockID string to a base64 representation*/
                .IgnoreArgument_source();

            STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "<Latest>")) /*this is building the XML*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the XML*/
                .IgnoreArgument_s1()
                .IgnoreArgument_s2();
            STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</Latest>")) /*this is building the XML*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relativePath*/

            STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=block&blockid=")) /*this is building the relativePath*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the relativePath by adding the blockId (base64 encoded_*/
                .IgnoreArgument_s1()
                .IgnoreArgument_s2();

            STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path as const char* */
                .IgnoreArgument_handle();

            STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, &httpResponse, NULL, testValidBufferHandle))
                .IgnoreArgument_handle()
                .IgnoreArgument_relativePath()
                .IgnoreArgument_requestContent();

            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the relativePath*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the blockID string to a base64 representation*/
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*this was the content to be uploaded*/
                .IgnoreArgument_handle();
        }

        /*this part is Put Block list*/
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</BlockList>")) /*This is closing the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relative path for the Put BLock list*/

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=blocklist")) /*This is still building relative path for Put Block list*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the XML as const char* so it can be passed to _ExecuteRequest*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)) /*this is creating the XML body as BUFFER_HANDLE*/
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_PUT,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG,
            &httpResponse,
            NULL,
            testValidBufferHandle
        ))
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestContent()
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            ;

        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*This is the XML as BUFFER_HANDLE*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is destroying the relative path for Put Block List*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)) /*this is the HTTPAPIEX handle*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the copy of hte hostname*/
            .IgnoreArgument_ptr();

        ///act
        BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, "a", NULL);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

        ///cleanup
        gballoc_free(content);
    }
}

/*Tests_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_64MB_unhappy_paths)
{
    size_t size = 64 * 1024 * 1024;

    size_t calls_that_cannot_fail[] =
    {
        13   ,/*STRING_delete*/
        26   ,/*STRING_delete*/
        39   ,/*STRING_delete*/
        52   ,/*STRING_delete*/
        65   ,/*STRING_delete*/
        78   ,/*STRING_delete*/
        91   ,/*STRING_delete*/
        104  ,/*STRING_delete*/
        117  ,/*STRING_delete*/
        130  ,/*STRING_delete*/
        143  ,/*STRING_delete*/
        156  ,/*STRING_delete*/
        169  ,/*STRING_delete*/
        182  ,/*STRING_delete*/
        195  ,/*STRING_delete*/
        208  ,/*STRING_delete*/
        11   ,/*STRING_c_str*/
        24   ,/*STRING_c_str*/
        37   ,/*STRING_c_str*/
        50   ,/*STRING_c_str*/
        63   ,/*STRING_c_str*/
        76   ,/*STRING_c_str*/
        89   ,/*STRING_c_str*/
        102  ,/*STRING_c_str*/
        115  ,/*STRING_c_str*/
        128  ,/*STRING_c_str*/
        141  ,/*STRING_c_str*/
        154  ,/*STRING_c_str*/
        167  ,/*STRING_c_str*/
        180  ,/*STRING_c_str*/
        193  ,/*STRING_c_str*/
        206  ,/*STRING_c_str*/
        14   ,/*STRING_delete*/
        27   ,/*STRING_delete*/
        40   ,/*STRING_delete*/
        53   ,/*STRING_delete*/
        66   ,/*STRING_delete*/
        79   ,/*STRING_delete*/
        92   ,/*STRING_delete*/
        105  ,/*STRING_delete*/
        118  ,/*STRING_delete*/
        131  ,/*STRING_delete*/
        144  ,/*STRING_delete*/
        157  ,/*STRING_delete*/
        170  ,/*STRING_delete*/
        183  ,/*STRING_delete*/
        196  ,/*STRING_delete*/
        209  ,/*STRING_delete*/
        15   ,/*BUFFER_delete*/
        28   ,/*BUFFER_delete*/
        41   ,/*BUFFER_delete*/
        54   ,/*BUFFER_delete*/
        67   ,/*BUFFER_delete*/
        80   ,/*BUFFER_delete*/
        93   ,/*BUFFER_delete*/
        106  ,/*BUFFER_delete*/
        119  ,/*BUFFER_delete*/
        132  ,/*BUFFER_delete*/
        145  ,/*BUFFER_delete*/
        158  ,/*BUFFER_delete*/
        171  ,/*BUFFER_delete*/
        184  ,/*BUFFER_delete*/
        197  ,/*BUFFER_delete*/
        210  ,/*BUFFER_delete*/


        214, /*STRING_c_str*/
        216, /*STRING_c_str*/
        218, /*BUFFER_delete*/
        219, /*STRING_delete*/
        220, /*STRING_delete*/
        221, /*HTTPAPIEX_Destroy*/
        222, /*gballoc_free*/
    };

    (void)umock_c_negative_tests_init();


    umock_c_reset_all_calls();
    ///arrange
    unsigned char * content = (unsigned char*)gballoc_malloc(size);
    ASSERT_IS_NOT_NULL(content);

    umock_c_reset_all_calls();

    memset(content, '3', size);
    content[0] = '0';
    content[size - 1] = '4';
    context.size = size;
    context.source = content;
    context.toUpload = context.size;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a copy of the hostname */
        .IgnoreArgument_size();

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create("h.h")); /*this is creating the httpapiex handle to storage (it is always the same host)*/
    STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

    /*uploading blocks (Put Block)*/
    for (size_t blockNumber = 0;blockNumber < (size - 1) / (4 * 1024 * 1024) + 1;blockNumber++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 4 * 1024 * 1024,
            (blockNumber != (size - 1) / (4 * 1024 * 1024)) ? 4 * 1024 * 1024 : (size - 1) % (4 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
        )); /*this is the content to be uploaded by this call*/

        /*here some sprintf happens and that produces a string in the form: 000000...049999*/
        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6)) /*this is converting the produced blockID string to a base64 representation*/ /*3, 16, 29... (16 numbers)*/
            .IgnoreArgument_source();

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "<Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the XML*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relativePath*/

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=block&blockid=")) /*this is building the relativePath*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the relativePath by adding the blockId (base64 encoded_*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path as const char* */ /*11, 24, 27...*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, &httpResponse, NULL, testValidBufferHandle))
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestContent()
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            ;

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the relativePath*/ /*13, 26, 39...*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the blockID string to a base64 representation*/ /*14, 27, 40...*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*this was the content to be uploaded*/ /*15, 28, 41...210 (16 numbers)*/
            .IgnoreArgument_handle();
    }

    /*this part is Put Block list*/
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</BlockList>")) /*This is closing the XML*/ /*211*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relative path for the Put BLock list*/

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=blocklist")) /*This is still building relative path for Put Block list*/
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the XML as const char* so it can be passed to _ExecuteRequest*/
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)) /*this is creating the XML body as BUFFER_HANDLE*/
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path*/
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_PUT,
        IGNORED_PTR_ARG,
        NULL,
        IGNORED_PTR_ARG,
        &httpResponse,
        NULL,
        testValidBufferHandle
    ))
        .IgnoreArgument_handle()
        .IgnoreArgument_relativePath()
        .IgnoreArgument_requestContent()
        .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
        ;

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*This is the XML as BUFFER_HANDLE*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is destroying the relative path for Put Block List*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)) /*this is the HTTPAPIEX handle*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the copy of hte hostname*/
        .IgnoreArgument_ptr();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        size_t j;
        umock_c_negative_tests_reset();

        for (j = 0;j<sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]);j++) /*not running the tests that cannot fail*/
        {
            if (calls_that_cannot_fail[j] == i)
                break;
        }

        if (j == sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]))
        {

            umock_c_negative_tests_fail_call(i);
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            ///act
            context.toUpload = context.size; /* Reinit context */
            BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

            ///assert
            ASSERT_ARE_NOT_EQUAL(BLOB_RESULT, BLOB_OK, result, temp_str);
        }
    }

    umock_c_negative_tests_deinit();

    ///cleanup
    gballoc_free(content);

}

/*Tests_SRS_BLOB_02_038: [ If HTTPAPIEX_SetOption fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_64MB_with_certificate_unhappy_paths)
{
    size_t size = 64 * 1024 * 1024;

    size_t calls_that_cannot_fail[] =
    {
        13  + 1 ,/*STRING_delete*/
        26  + 1 ,/*STRING_delete*/
        39  + 1 ,/*STRING_delete*/
        52  + 1 ,/*STRING_delete*/
        65  + 1 ,/*STRING_delete*/
        78  + 1 ,/*STRING_delete*/
        91  + 1 ,/*STRING_delete*/
        104 + 1 ,/*STRING_delete*/
        117 + 1 ,/*STRING_delete*/
        130 + 1 ,/*STRING_delete*/
        143 + 1 ,/*STRING_delete*/
        156 + 1 ,/*STRING_delete*/
        169 + 1 ,/*STRING_delete*/
        182 + 1 ,/*STRING_delete*/
        195 + 1 ,/*STRING_delete*/
        208 + 1 ,/*STRING_delete*/
        11  + 1 ,/*STRING_c_str*/
        24  + 1 ,/*STRING_c_str*/
        37  + 1 ,/*STRING_c_str*/
        50  + 1 ,/*STRING_c_str*/
        63  + 1 ,/*STRING_c_str*/
        76  + 1 ,/*STRING_c_str*/
        89  + 1 ,/*STRING_c_str*/
        102 + 1 ,/*STRING_c_str*/
        115 + 1 ,/*STRING_c_str*/
        128 + 1 ,/*STRING_c_str*/
        141 + 1 ,/*STRING_c_str*/
        154 + 1 ,/*STRING_c_str*/
        167 + 1 ,/*STRING_c_str*/
        180 + 1 ,/*STRING_c_str*/
        193 + 1 ,/*STRING_c_str*/
        206 + 1 ,/*STRING_c_str*/
        14  + 1 ,/*STRING_delete*/
        27  + 1 ,/*STRING_delete*/
        40  + 1 ,/*STRING_delete*/
        53  + 1 ,/*STRING_delete*/
        66  + 1 ,/*STRING_delete*/
        79  + 1 ,/*STRING_delete*/
        92  + 1 ,/*STRING_delete*/
        105 + 1 ,/*STRING_delete*/
        118 + 1 ,/*STRING_delete*/
        131 + 1 ,/*STRING_delete*/
        144 + 1 ,/*STRING_delete*/
        157 + 1 ,/*STRING_delete*/
        170 + 1 ,/*STRING_delete*/
        183 + 1 ,/*STRING_delete*/
        196 + 1 ,/*STRING_delete*/
        209 + 1 ,/*STRING_delete*/
        15  + 1 ,/*BUFFER_delete*/
        28  + 1 ,/*BUFFER_delete*/
        41  + 1 ,/*BUFFER_delete*/
        54  + 1 ,/*BUFFER_delete*/
        67  + 1 ,/*BUFFER_delete*/
        80  + 1 ,/*BUFFER_delete*/
        93  + 1 ,/*BUFFER_delete*/
        106 + 1 ,/*BUFFER_delete*/
        119 + 1 ,/*BUFFER_delete*/
        132 + 1 ,/*BUFFER_delete*/
        145 + 1 ,/*BUFFER_delete*/
        158 + 1 ,/*BUFFER_delete*/
        171 + 1 ,/*BUFFER_delete*/
        184 + 1 ,/*BUFFER_delete*/
        197 + 1 ,/*BUFFER_delete*/
        210 + 1 ,/*BUFFER_delete*/


        214+1, /*STRING_c_str*/
        216+1, /*STRING_c_str*/
        218+1, /*BUFFER_delete*/
        219+1, /*STRING_delete*/
        220+1, /*STRING_delete*/
        221+1, /*HTTPAPIEX_Destroy*/
        222+1, /*gballoc_free*/
    };

    (void)umock_c_negative_tests_init();


    umock_c_reset_all_calls();
    ///arrange
    unsigned char * content = (unsigned char*)gballoc_malloc(size);
    ASSERT_IS_NOT_NULL(content);

    umock_c_reset_all_calls();

    memset(content, '3', size);
    content[0] = '0';
    content[size - 1] = '4';
    context.size = size;
    context.source = content;
    context.toUpload = context.size;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a copy of the hostname */
        .IgnoreArgument_size();

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create("h.h")); /*this is creating the httpapiex handle to storage (it is always the same host)*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, "TrustedCerts", IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

                                                                                                         /*uploading blocks (Put Block)*/
    for (size_t blockNumber = 0;blockNumber < (size - 1) / (4 * 1024 * 1024) + 1;blockNumber++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 4 * 1024 * 1024,
            (blockNumber != (size - 1) / (4 * 1024 * 1024)) ? 4 * 1024 * 1024 : (size - 1) % (4 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
        )); /*this is the content to be uploaded by this call*/

        /*here some sprintf happens and that produces a string in the form: 000000...049999*/
        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6)) /*this is converting the produced blockID string to a base64 representation*/ /*3, 16, 29... (16 numbers)*/
            .IgnoreArgument_source(); /* 5 */

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "<Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the XML*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relativePath*/

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=block&blockid=")) /*this is building the relativePath*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the relativePath by adding the blockId (base64 encoded_*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path as const char* */ /*12, 25, 38...*/
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, &httpResponse, NULL, testValidBufferHandle))
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestContent()
            .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
            ; /* 13 */

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the relativePath*/ /*14, 27, 40...*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the blockID string to a base64 representation*/ /*15, 28, 41... */
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*this was the content to be uploaded*/ /*16, 29, 42...211 (16 numbers)*/
                    .IgnoreArgument_handle();
    }

    /*this part is Put Block list*/
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</BlockList>")) /*This is closing the XML*/ /*212*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relative path for the Put BLock list*/

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=blocklist")) /*This is still building relative path for Put Block list*/
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the XML as const char* so it can be passed to _ExecuteRequest*/
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)) /*this is creating the XML body as BUFFER_HANDLE*/
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path*/
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_PUT,
        IGNORED_PTR_ARG,
        NULL,
        IGNORED_PTR_ARG,
        &httpResponse,
        NULL,
        testValidBufferHandle
    ))
        .IgnoreArgument_handle()
        .IgnoreArgument_relativePath()
        .IgnoreArgument_requestContent()
        .CopyOutArgumentBuffer_statusCode(&TwoHundred, sizeof(TwoHundred))
        ;

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*This is the XML as BUFFER_HANDLE*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is destroying the relative path for Put Block List*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)) /*this is the HTTPAPIEX handle*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the copy of the hostname*/ /* 223 */
        .IgnoreArgument_ptr();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        size_t j;
        umock_c_negative_tests_reset();

        for (j = 0;j<sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]);j++) /*not running the tests that cannot fail*/
        {
            if (calls_that_cannot_fail[j] == i)
                break;
        }

        if (j == sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]))
        {

            umock_c_negative_tests_fail_call(i);
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            ///act
            context.toUpload = context.size; /* Reinit context */
            BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, "a", NULL);

            ///assert
            ASSERT_ARE_NOT_EQUAL(BLOB_RESULT, BLOB_OK, result, temp_str);
        }
    }

    umock_c_negative_tests_deinit();

    ///cleanup
    gballoc_free(content);

}

/*Tests_SRS_BLOB_02_026: [ Otherwise, if HTTP response code is >=300 then Blob_UploadFromSasUri shall succeed and return BLOB_OK. ]*/
TEST_FUNCTION(Blob_UploadFromSasUri_when_http_code_is_404_it_immediately_succeeds)
{
    size_t size = 64 * 1024 * 1024;

    ///arrange
    unsigned char * content = (unsigned char*)gballoc_malloc(size);
    ASSERT_IS_NOT_NULL(content);

    umock_c_reset_all_calls();

    memset(content, '3', size);
    content[0] = '0';
    content[size - 1] = '4';
    context.size = size;
    context.source = content;
    context.toUpload = context.size;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a copy of the hostname */
        .IgnoreArgument_size();

    STRICT_EXPECTED_CALL(HTTPAPIEX_Create("h.h")); /*this is creating the httpapiex handle to storage (it is always the same host)*/
    STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

    /*uploading blocks (Put Block)*/ /*this simply fails first block*/
    size_t blockNumber = 0;
    {
        STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 4 * 1024 * 1024,
            (blockNumber != (size - 1) / (4 * 1024 * 1024)) ? 4 * 1024 * 1024 : (size - 1) % (4 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
        )); /*this is the content to be uploaded by this call*/

        /*here some sprintf happens and that produces a string in the form: 000000...049999*/
        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6)) /*this is converting the produced blockID string to a base64 representation*/
            .IgnoreArgument_source();

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "<Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the XML*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "</Latest>")) /*this is building the XML*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_construct("/something?a=b")); /*this is building the relativePath*/

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "&comp=block&blockid=")) /*this is building the relativePath*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is building the relativePath by adding the blockId (base64 encoded_*/
            .IgnoreArgument_s1()
            .IgnoreArgument_s2();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path as const char* */
            .IgnoreArgument_handle();

        STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(IGNORED_PTR_ARG, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, &httpResponse, NULL, testValidBufferHandle))
            .IgnoreArgument_handle()
            .IgnoreArgument_relativePath()
            .IgnoreArgument_requestContent()
            .CopyOutArgumentBuffer_statusCode(&FourHundredFour, sizeof(FourHundredFour))
            ;

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the relativePath*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)) /*this is unbuilding the blockID string to a base64 representation*/
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG)) /*this was the content to be uploaded*/
        .IgnoreArgument_handle();
    }

    /*this part is Put Block list*/ /*notice: no op because it failed before with 404*/

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)) /*this is the HTTPAPIEX handle*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the copy of hte hostname*/
        .IgnoreArgument_ptr();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

    ///cleanup
    gballoc_free(content);

}

/*Tests_SRS_BLOB_99_001: [ If the size of the block returned by `getDataCallback` is bigger than 4MB, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_when_blockSize_too_big_fails)
{
    ///arrange
    BLOB_UPLOAD_CONTEXT_FAKE fakeContext;
    fakeContext.blockSent = 0;
    fakeContext.blockSize = BLOCK_SIZE + 1;
    fakeContext.blocksCount = 1;
    fakeContext.fakeData = NULL;
    fakeContext.abortOnBlockNumber = -1;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

/*Tests_SRS_BLOB_99_001: [ If the size of the block returned by `getDataCallback` is bigger than 4MB, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_when_blockSize_is_4MB_succeeds)
{
    ///arrange
    BLOB_UPLOAD_CONTEXT_FAKE fakeContext;
    fakeContext.blockSent = 0;
    fakeContext.blockSize = BLOCK_SIZE;
    fakeContext.blocksCount = 1;
    fakeContext.fakeData = NULL;
    fakeContext.abortOnBlockNumber = -1;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

/*Tests_SRS_BLOB_99_003: [ If `getDataCallback` returns more than 50000 blocks, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_when_blockCount_is_maximum_succeeds)
{
    ///arrange
    BLOB_UPLOAD_CONTEXT_FAKE fakeContext;
    fakeContext.blockSent = 0;
    fakeContext.blockSize = 1;
    fakeContext.blocksCount = MAX_BLOCK_COUNT;
    fakeContext.fakeData = NULL;
    fakeContext.abortOnBlockNumber = -1;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

/*Tests_SRS_BLOB_99_003: [ If `getDataCallback` returns more than 50000 blocks, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_when_blockCount_is_one_over_maximum_fails)
{
    ///arrange
    BLOB_UPLOAD_CONTEXT_FAKE fakeContext;
    fakeContext.blockSent = 0;
    fakeContext.blockSize = 1;
    fakeContext.blocksCount = MAX_BLOCK_COUNT + 1;
    fakeContext.fakeData = NULL;
    fakeContext.abortOnBlockNumber = -1;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

/*Tests_SRS_BLOB_99_004: [ If `getDataCallback` returns `IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_ABORT`, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop and return `BLOB_ABORTED`. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_returns_BLOB_ABORTED_when_callback_aborts_immediately)
{
    ///arrange
    BLOB_UPLOAD_CONTEXT_FAKE fakeContext;
    fakeContext.blockSent = 0;
    fakeContext.blockSize = 1;
    fakeContext.blocksCount = 10;
    fakeContext.fakeData = NULL;
    fakeContext.abortOnBlockNumber = 0;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ABORTED, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

/*Tests_SRS_BLOB_99_004: [ If `getDataCallback` returns `IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_ABORT`, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop and return `BLOB_ABORTED`. ]*/
TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_returns_BLOB_ABORTED_when_callback_aborts_after_5_blocks)
{
    ///arrange
    BLOB_UPLOAD_CONTEXT_FAKE fakeContext;
    fakeContext.blockSent = 0;
    fakeContext.blockSize = 1;
    fakeContext.blocksCount = 10;
    fakeContext.fakeData = NULL;
    fakeContext.abortOnBlockNumber = 5;

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ABORTED, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

END_TEST_SUITE(blob_ut);
