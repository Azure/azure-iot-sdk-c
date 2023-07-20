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
#include "azure_c_shared_utility/singlylinkedlist.h"
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

/*helps when enums are not matched*/
#ifdef malloc
#undef malloc
#endif

#define TEST_HTTPAPIEX_HANDLE           (HTTPAPIEX_HANDLE)0x4343
#define TEST_SINGLYLINKEDLIST_HANDLE    (SINGLYLINKEDLIST_HANDLE)0x4444
#define TEST_BUFFER_HANDLE              (BUFFER_HANDLE)0x4445
#define TEST_LIST_ITEM_HANDLE           (LIST_ITEM_HANDLE)0x4446
#define TEST_STRING_HANDLE              (STRING_HANDLE)0x4447
#define HTTP_OK                 200
#define HTTP_UNAUTHORIZED       401

const unsigned int HTTP_STATUS_200 = HTTP_OK;
const unsigned int HTTP_STATUS_401 = HTTP_UNAUTHORIZED;

TEST_DEFINE_ENUM_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);

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

TEST_DEFINE_ENUM_TYPE(BLOB_RESULT, BLOB_RESULT_VALUES);

#define TEST_HTTPCOLONBACKSLASHBACKSLACH "http://"
#define TEST_RELATIVE_PATH_1 "/here/follows/something?param1=value1&param2=value2"
#define TEST_VALID_SASURI_1 TEST_HTTPCOLONBACKSLASHBACKSLACH TEST_HOSTNAME_1 TEST_RELATIVE_PATH_1

#define X_MS_BLOB_TYPE "x-ms-blob-type"
#define BLOCK_BLOB "BlockBlob"

const char* TEST_BLOB_STORAGE_HOSTNAME = "host.name";
const char* TEST_TRUSTED_CERTIFICATE = "my certificate";
const HTTP_PROXY_OPTIONS TEST_PROXY_OPTIONS = { "a", 8888, NULL, NULL };
const char* TEST_NETWORK_INTERFACE = "eth0";
const size_t TEST_TIMEOUT_MILLISECONDS = 5000;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

// Allocate test content during initial test setup only.  This buffer is very large,
// which means significant performance degradation on Valgrind tests if we were to
// allocate and free it per individual test-case.
static unsigned char * testUploadToBlobContent;
// Maximum size a test blob can be.  This is large (!), but needs of underlying
// test to verify that large buffers are properly chunked end up with this.
static const size_t testUploadToBlobContentMaxSize = 256 * 1024 * 1024;

BEGIN_TEST_SUITE(blob_ut)

TEST_SUITE_INITIALIZE(TestSuiteInitialize)
{
    (void)umock_c_init(on_umock_c_error);

    (void)umocktypes_charptr_register_types();

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPAPIEX_Create, TEST_HTTPAPIEX_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_Create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(Azure_Base64_Encode_Bytes, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Encode_Bytes, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURNS(singlylinkedlist_add, TEST_LIST_ITEM_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(singlylinkedlist_get_head_item, TEST_LIST_ITEM_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(singlylinkedlist_get_next_item, TEST_LIST_ITEM_HANDLE, NULL);

    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_SetOption, HTTPAPIEX_OK, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_RETURNS(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_OK, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, "a");

    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_CONDITION_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);

    REGISTER_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE);
    REGISTER_TYPE(HTTPAPIEX_RESULT, HTTPAPIEX_RESULT);
    REGISTER_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT);

    testUploadToBlobContent = gballoc_malloc(testUploadToBlobContentMaxSize);
    ASSERT_IS_NOT_NULL(testUploadToBlobContent);

    umock_c_reset_all_calls();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_free(testUploadToBlobContent);

    umock_c_deinit();
}

static void reset_test_data()
{
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

TEST_FUNCTION(Blob_CreateHttpConnection_with_NULL_hostname_fails)
{
    ///arrange

    ///act
    HTTPAPIEX_HANDLE result = Blob_CreateHttpConnection(NULL, NULL, NULL, NULL, 0);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);

    ///cleanup
}

TEST_FUNCTION(Blob_CreateHttpConnection_happy_path_no_options_succeeds)
{
    ///arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_BLOB_STORAGE_HOSTNAME));

    ///act
    HTTPAPIEX_HANDLE result = Blob_CreateHttpConnection(TEST_BLOB_STORAGE_HOSTNAME, NULL, NULL, NULL, 0);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_HTTPAPIEX_HANDLE, result);

    ///cleanup
}

TEST_FUNCTION(Blob_CreateHttpConnection_happy_path_succeeds)
{
    ///arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_BLOB_STORAGE_HOSTNAME));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_HTTP_TIMEOUT, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_TRUSTED_CERT, TEST_TRUSTED_CERTIFICATE));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_HTTP_PROXY, &TEST_PROXY_OPTIONS));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_CURL_INTERFACE, TEST_NETWORK_INTERFACE));

    ///act
    HTTPAPIEX_HANDLE result = Blob_CreateHttpConnection(
        TEST_BLOB_STORAGE_HOSTNAME, TEST_TRUSTED_CERTIFICATE, &TEST_PROXY_OPTIONS, TEST_NETWORK_INTERFACE, TEST_TIMEOUT_MILLISECONDS);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_HTTPAPIEX_HANDLE, result);

    ///cleanup
}

TEST_FUNCTION(Blob_CreateHttpConnection_failure_checks)
{
    ///arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_BLOB_STORAGE_HOSTNAME));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_HTTP_TIMEOUT, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_TRUSTED_CERT, TEST_TRUSTED_CERTIFICATE));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_HTTP_PROXY, &TEST_PROXY_OPTIONS));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, OPTION_CURL_INTERFACE, TEST_NETWORK_INTERFACE));
    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {
            // arrange
            char error_msg[64];
            HTTPAPIEX_HANDLE result;

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = Blob_CreateHttpConnection(
                TEST_BLOB_STORAGE_HOSTNAME, TEST_TRUSTED_CERTIFICATE, &TEST_PROXY_OPTIONS, TEST_NETWORK_INTERFACE, TEST_TIMEOUT_MILLISECONDS);

            ///assert
            sprintf(error_msg, "On failed call %lu", (unsigned long)i);
            ASSERT_IS_NULL(result, error_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Blob_DestroyHttpConnection_happy_path_succeeds)
{
    ///arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(TEST_HTTPAPIEX_HANDLE));

    ///act
    Blob_DestroyHttpConnection(TEST_HTTPAPIEX_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

TEST_FUNCTION(Blob_DestroyHttpConnection_NULL_handle_fails)
{
    ///arrange
    umock_c_reset_all_calls();

    ///act
    Blob_DestroyHttpConnection(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

TEST_FUNCTION(Blob_ClearBlockIdList_happy_path_succeeds)
{
    ///arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    ///act
    Blob_ClearBlockIdList(TEST_SINGLYLINKEDLIST_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

TEST_FUNCTION(Blob_ClearBlockIdList_NULL_handle_fails)
{
    ///arrange
    umock_c_reset_all_calls();

    ///act
    Blob_ClearBlockIdList(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_happy_path_succeeds)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, blockData, IGNORED_PTR_ARG, IGNORED_PTR_ARG, responseContent))
        .CopyOutArgumentBuffer_statusCode(&HTTP_STATUS_200, sizeof(unsigned int));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_401_fails)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, blockData, IGNORED_PTR_ARG, IGNORED_PTR_ARG, responseContent))
        .CopyOutArgumentBuffer_statusCode(&HTTP_STATUS_401, sizeof(unsigned int));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_HTTP_ERROR, result);
    ASSERT_ARE_EQUAL(int, HTTP_STATUS_401, responseHttpStatus);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_NULL_HTTPAPI_HANDLE_fails)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        NULL, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_NULL_relative_path_fails)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, NULL, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_NULL_blockData_fails)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = NULL;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_NULL_blockIdList_fails)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = NULL;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_NULL_response_status_succeeds)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, blockData, NULL, IGNORED_PTR_ARG, responseContent))
        .CopyOutArgumentBuffer_statusCode(&HTTP_STATUS_200, sizeof(unsigned int));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, NULL, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_NULL_response_content_succeeds)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = NULL;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, blockData, IGNORED_PTR_ARG, IGNORED_PTR_ARG, responseContent))
        .CopyOutArgumentBuffer_statusCode(&HTTP_STATUS_200, sizeof(unsigned int));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_failure_checks)
{
    ///arrange
    unsigned int blockId = 0;
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, blockData, &responseHttpStatus, IGNORED_PTR_ARG, responseContent));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
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

            // act
            BLOB_RESULT result = Blob_PutBlock(
                TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

            ///assert
            sprintf(error_msg, "On failed call %lu", (unsigned long)i);
            ASSERT_ARE_NOT_EQUAL(int, BLOB_OK, result, error_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Blob_PutBlock_blockId_max_succeeds)
{
    ///arrange
    unsigned int blockId = 49999; // Maximum 
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, 6));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, blockData, IGNORED_PTR_ARG, IGNORED_PTR_ARG, responseContent))
        .CopyOutArgumentBuffer_statusCode(&HTTP_STATUS_200, sizeof(unsigned int));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlock_blockId_above_max_fails)
{
    ///arrange
    unsigned int blockId = 50000; // Maximum 
    BUFFER_HANDLE blockData = TEST_BUFFER_HANDLE;
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlock(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockId, blockData, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_ERROR, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlockList_happy_path_succeeds)
{
    ///arrange
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    // createBlockIdListXml(1 block)
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(TEST_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // Back to Blob_PutBlockList
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(100);
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, responseContent))
        .CopyOutArgumentBuffer_statusCode(&HTTP_STATUS_200, sizeof(unsigned int));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    ///act
    BLOB_RESULT result = Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlockList_401_fails)
{
    ///arrange
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    // createBlockIdListXml(1 block)
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(TEST_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // Back to Blob_PutBlockList
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(100);
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, responseContent))
        .CopyOutArgumentBuffer_statusCode(&HTTP_STATUS_401, sizeof(unsigned int));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    ///act
    BLOB_RESULT result = Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_HTTP_ERROR, result);
    ASSERT_ARE_EQUAL(int, HTTP_STATUS_401, responseHttpStatus);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlockList_failure_checks)
{
    ///arrange
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .CallCannotFail();
    // createBlockIdListXml(1 block)
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(TEST_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // Back to Blob_PutBlockList
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(100);
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, &responseHttpStatus, IGNORED_PTR_ARG, responseContent));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();

        if (umock_c_negative_tests_can_call_fail(i))
        {
            // arrange
            char error_msg[64];

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            BLOB_RESULT result = Blob_PutBlockList(
                TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockIdList, &responseHttpStatus, responseContent);

            ///assert
            sprintf(error_msg, "On failed call %lu", (unsigned long)i);
            ASSERT_ARE_NOT_EQUAL(BLOB_RESULT, BLOB_OK, result, error_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Blob_PutBlockList_empty_blockIDList_fails)
{
    ///arrange
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .SetReturn(NULL);

    ///act
    BLOB_RESULT result = Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_ERROR, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlockList_NULL_HTTPAPIEX_HANDLE_fails)
{
    ///arrange
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlockList(
        NULL, TEST_RELATIVE_PATH_1, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_INVALID_ARG, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlockList_NULL_blockIdList_fails)
{
    ///arrange
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, NULL, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_INVALID_ARG, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlockList_NULL_relative_path_fails)
{
    ///arrange
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, NULL, blockIdList, &responseHttpStatus, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_INVALID_ARG, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlockList_NULL_http_status_fails)
{
    ///arrange
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    BUFFER_HANDLE responseContent = TEST_BUFFER_HANDLE;

    umock_c_reset_all_calls();

    ///act
    BLOB_RESULT result = Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockIdList, NULL, responseContent);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_INVALID_ARG, result);

    ///cleanup
}

TEST_FUNCTION(Blob_PutBlockList_no_response_content_succeeds)
{
    ///arrange
    SINGLYLINKEDLIST_HANDLE blockIdList = TEST_SINGLYLINKEDLIST_HANDLE;
    unsigned int responseHttpStatus = 0;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    // createBlockIdListXml(1 block)
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(TEST_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // Back to Blob_PutBlockList
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(100);
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest(
        TEST_HTTPAPIEX_HANDLE, HTTPAPI_REQUEST_PUT, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
        .CopyOutArgumentBuffer_statusCode(&HTTP_STATUS_200, sizeof(unsigned int));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    ///act
    BLOB_RESULT result = Blob_PutBlockList(
        TEST_HTTPAPIEX_HANDLE, TEST_RELATIVE_PATH_1, blockIdList, &responseHttpStatus, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, BLOB_OK, result);

    ///cleanup
}

END_TEST_SUITE(blob_ut);
