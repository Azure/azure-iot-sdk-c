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

    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_OK, HTTPAPIEX_ERROR);

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

static void set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup()
{
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG)) /*this is the HTTPAPIEX handle*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))/*this is the XML string used for Put Block List operation*/
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the copy of hte hostname*/
        .IgnoreArgument_ptr();
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

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_with_NULL_SasUri_fails)
{
    ///arrange

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(NULL, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);

    ///cleanup

}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_with_NULL_getDataCallBack_and_non_NULL_context_fails)
{
    ///arrange

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, NULL, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);

    ///cleanup

}

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
    for (size_t blockNumber = 0;blockNumber < (size - 1) / (100 * 1024 * 1024) + 1;blockNumber++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(&c + blockNumber * 100 * 1024 * 1024,
            (blockNumber != (size - 1) / (100 * 1024 * 1024)) ? 100 * 1024 * 1024 : (size - 1) % (100 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
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

    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

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
    for (size_t blockNumber = 0;blockNumber < (size - 1) / (100 * 1024 * 1024) + 1;blockNumber++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(&c + blockNumber * 100 * 1024 * 1024,
            (blockNumber != (size - 1) / (100 * 1024 * 1024)) ? 100 * 1024 * 1024 : (size - 1) % (100 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
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

    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_HTTP_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}


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

        }
        set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();
    }

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ERROR, result);

    ///cleanup
}

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
    }

    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

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

    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri(TEST_VALID_SASURI_1, FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_when_SasUri_is_wrong_fails_1)
{
    ///arrange
    unsigned char c = '3';
    context.size = 1;
    context.source = &c;
    context.toUpload = context.size;

    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https:/h.h/doms", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL); /*wrong format for protocol, notice it is actually http:\h.h\doms (missing a \ from http)*/

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_when_SasUri_is_wrong_fails_2)
{
    ///arrange
    unsigned char c = '3';
    context.size = 1;
    context.source = &c;
    context.toUpload = context.size;

    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL); /*there's no relative path here*/

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

static void Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path_Impl(HTTP_PROXY_OPTIONS *proxyOptions, const char * networkInterface)
{
    /*the following sizes have been identified as "important to be tested*/
    /*1B, 100MB-1, 100MB, 100MB+1, 256MB, 256MB+1, 260MB-1, 260MB, 260MB+1*/
    size_t sizes[] = {
        1,

        100 * 1024 * 1024 - 1,
        100 * 1024 * 1024,
        100 * 1024 * 1024 + 1,

        256 * 1024 * 1024 - 1,
        256 * 1024 * 1024,
        256 * 1024 * 1024 + 1,

        260 * 1024 * 1024 - 1,
        260 * 1024 * 1024,
        260 * 1024 * 1024 + 1,
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

        if (networkInterface != NULL)
        {
            STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_CURL_INTERFACE, networkInterface));
        }
        STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

        /*uploading blocks (Put Block)*/
        for (size_t blockNumber = 0;blockNumber < (sizes[iSize] - 1) / (100 * 1024 * 1024) + 1;blockNumber++)
        {
            STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 100 * 1024 * 1024,
                (blockNumber != (sizes[iSize] - 1) / (100 * 1024 * 1024)) ? 100 * 1024 * 1024 : (sizes[iSize] - 1) % (100 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
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
        set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

        ///act
        BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, proxyOptions, networkInterface);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

        ///cleanup
        gballoc_free(content);
    }


}


TEST_FUNCTION(Blob_UploadFromSasUri_with_proxy_happy_path)
{
    HTTP_PROXY_OPTIONS proxyOptions = { "a", 8888, NULL, NULL };
    Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path_Impl(&proxyOptions, NULL);
}


TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path)
{
    Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path_Impl(NULL, NULL);
}


TEST_FUNCTION(Blob_UploadFromSasUri_with_network_interface)
{
    const char* networkInterface = "eth0";
    Blob_UploadMultipleBlocksFromSasUri_various_sizes_happy_path_Impl(NULL, networkInterface);
}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_various_sizes_with_certificates_happy_path)
{
    /*the following sizes have been identified as "important to be tested*/
    /*1B, 100MB-1, 100MB, 100MB+1, 256MB, 256MB+1, 260MB-1, 260MB, 260MB+1*/
    size_t sizes[] = {
        1,

        100 * 1024 * 1024 - 1,
        100 * 1024 * 1024,
        100 * 1024 * 1024 + 1,

        256 * 1024 * 1024 - 1,
        256 * 1024 * 1024,
        256 * 1024 * 1024 + 1,

        260 * 1024 * 1024 - 1,
        260 * 1024 * 1024,
        260 * 1024 * 1024 + 1,
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
        for (size_t blockNumber = 0;blockNumber < (sizes[iSize] - 1) / (100 * 1024 * 1024) + 1;blockNumber++)
        {
            STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 100 * 1024 * 1024,
                (blockNumber != (sizes[iSize] - 1) / (100 * 1024 * 1024)) ? 100 * 1024 * 1024 : (sizes[iSize] - 1) % (100 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
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
        set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

        ///act
        BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, "a", NULL, NULL);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

        ///cleanup
        gballoc_free(content);
    }
}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_64MB_unhappy_paths)
{
    size_t size = 256 * 1024 * 1024;
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
    for (size_t blockNumber = 0;blockNumber < (size - 1) / (100 * 1024 * 1024) + 1;blockNumber++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 100 * 1024 * 1024,
            (blockNumber != (size - 1) / (100 * 1024 * 1024)) ? 100 * 1024 * 1024 : (size - 1) % (100 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
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
            .IgnoreArgument_handle()
            .CallCannotFail();

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
        .IgnoreArgument_handle()
        .CallCannotFail();

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)) /*this is creating the XML body as BUFFER_HANDLE*/
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path*/
        .IgnoreArgument_handle()
        .CallCannotFail();

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
    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {

            umock_c_negative_tests_fail_call(i);
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            ///act
            context.toUpload = context.size; /* Reinit context */
            BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

            ///assert
            ASSERT_ARE_NOT_EQUAL(BLOB_RESULT, BLOB_OK, result, temp_str);
        }
    }

    umock_c_negative_tests_deinit();

    ///cleanup
    gballoc_free(content);

}

TEST_FUNCTION(Blob_UploadMultipleBlocksFromSasUri_64MB_with_certificate_and_network_interface_unhappy_paths)
{
    size_t size = 256 * 1024 * 1024;

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
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(IGNORED_PTR_ARG, OPTION_CURL_INTERFACE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>")); /*this is starting to build the XML used in Put Block List operation*/

                                                                                                         /*uploading blocks (Put Block)*/
    for (size_t blockNumber = 0;blockNumber < (size - 1) / (100 * 1024 * 1024) + 1;blockNumber++)
    {
        STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 100 * 1024 * 1024,
            (blockNumber != (size - 1) / (100 * 1024 * 1024)) ? 100 * 1024 * 1024 : (size - 1) % (100 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
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
            .IgnoreArgument_handle()
            .CallCannotFail();

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
        .IgnoreArgument_handle()
        .CallCannotFail();

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)) /*this is creating the XML body as BUFFER_HANDLE*/
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the relative path*/
        .IgnoreArgument_handle()
        .CallCannotFail();

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
    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {

            umock_c_negative_tests_fail_call(i);
            const char* interfaceName = "eth0";
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            ///act
            context.toUpload = context.size; /* Reinit context */
            BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, "a", NULL, interfaceName);

            ///assert
            ASSERT_ARE_NOT_EQUAL(BLOB_RESULT, BLOB_OK, result, temp_str);
        }
    }

    umock_c_negative_tests_deinit();

    ///cleanup
    gballoc_free(content);

}

TEST_FUNCTION(Blob_UploadFromSasUri_when_http_code_is_404_it_immediately_succeeds)
{
    size_t size = 256 * 1024 * 1024;

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
        STRICT_EXPECTED_CALL(BUFFER_create(content + blockNumber * 100 * 1024 * 1024,
            (blockNumber != (size - 1) / (100 * 1024 * 1024)) ? 100 * 1024 * 1024 : (size - 1) % (100 * 1024 * 1024) + 1 /*condition to take care of "the size of the last block*/
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

    set_expected_calls_for_Blob_UploadMultipleBlocksFromSasUri_cleanup();

    ///act
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetData_Callback, &context, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

    ///cleanup
    gballoc_free(content);

}

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
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

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
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

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
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_OK, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

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
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_INVALID_ARG, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

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
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ABORTED, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

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
    BLOB_RESULT result = Blob_UploadMultipleBlocksFromSasUri("https://h.h/something?a=b", FileUpload_GetFakeData_Callback, &fakeContext, &httpResponse, testValidBufferHandle, NULL, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(BLOB_RESULT, BLOB_ABORTED, result);

    ///cleanup
    gballoc_free(fakeContext.fakeData);
}

END_TEST_SUITE(blob_ut);
