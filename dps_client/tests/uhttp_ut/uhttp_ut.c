// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstdbool>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_bool.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

static void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/singlylinkedlist.h"

#include "azure_c_shared_utility/crt_abstractions.h"

#undef ENABLE_MOCKS

#include "azure_hub_modules/uhttp.h"

#define ENABLE_MOCKS

#ifdef __cplusplus
extern "C" {
#endif
#include "azure_c_shared_utility/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, on_connection_callback, void*, callback_ctx, HTTP_CALLBACK_REASON, connect_result);
MOCKABLE_FUNCTION(, void, on_error_callback, void*, callback_ctx, HTTP_CALLBACK_REASON, error_result);
MOCKABLE_FUNCTION(, void, on_msg_recv_callback, void*, callback_ctx, HTTP_CALLBACK_REASON, request_result, const unsigned char*, content, size_t, content_length, unsigned int, status_code,
    HTTP_HEADERS_HANDLE, response_header);
MOCKABLE_FUNCTION(, void, on_closed_callback, void*, callback_ctx);

#ifdef __cplusplus
}
#endif

#undef ENABLE_MOCKS

#ifdef __cplusplus
extern "C" {
#endif

    extern BUFFER_HANDLE real_BUFFER_new(void);
    extern void real_BUFFER_delete(BUFFER_HANDLE handle);
    extern unsigned char* real_BUFFER_u_char(BUFFER_HANDLE handle);
    extern size_t real_BUFFER_length(BUFFER_HANDLE handle);
    extern int real_BUFFER_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size);
    extern int real_BUFFER_append_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size);
    extern BUFFER_HANDLE real_BUFFER_clone(BUFFER_HANDLE handle);
    extern int real_BUFFER_shrink(BUFFER_HANDLE handle, size_t decreaseSize, bool fromEnd);
    extern BUFFER_HANDLE real_BUFFER_create(const unsigned char* source, size_t size);

#ifdef __cplusplus
}
#endif

static ON_BYTES_RECEIVED g_onBytesRecv = NULL;
static void* g_onBytesRecv_ctx = NULL;
static ON_SEND_COMPLETE g_on_xio_send_complete;
static ON_IO_ERROR g_on_io_error = NULL;
static void* g_on_io_error_ctx = NULL;

static HTTP_CALLBACK_REASON g_http_cb_reason;
static size_t g_http_cb_content_len;
static unsigned int g_http_cb_status_code;
static size_t g_header_count = 0;
static ON_IO_OPEN_COMPLETE g_on_open_complete = NULL;
static void* g_on_open_complete_context = NULL;
static ON_IO_CLOSE_COMPLETE g_on_close_complete = NULL;
static void* g_on_close_complete_context = NULL;

static const void** g_list_items = NULL;
static size_t g_list_item_count = 0;
static bool g_list_add_called = false;

static void* TEST_CREATE_PARAM = (void*)0x1210;
static HTTP_HEADERS_HANDLE TEST_HTTP_HEADERS_HANDLE = (HTTP_HEADERS_HANDLE)0x1211;
static IO_INTERFACE_DESCRIPTION* TEST_INTERFACE_DESC = (IO_INTERFACE_DESCRIPTION*)0x1212;
static BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x1214;
static STRING_HANDLE TEST_STRING_HANDLE = (STRING_HANDLE)0x1215;
static LIST_ITEM_HANDLE TEST_LIST_HANDLE_ITEM = (LIST_ITEM_HANDLE)0x1216;
static void* TEST_EXECUTE_CONTEXT = (void*)0x1217;
static void* TEST_CONNECT_CONTEXT = (void*)0x1218;
static void* TEST_CLOSE_CONTEXT = (void*)0x1219;
static void* TEST_ERROR_CONTEXT = (void*)0x1219;

static const char* TEST_SEND_DATA = "TEST_SEND_DATA";
static char* TEST_STRING_VALUE = "Test_String_Value";
static size_t TEST_STRING_LENGTH = sizeof("Test_String_Value");

static const char* TEST_HOST_NAME = "HTTP_HOST";
static const char* TEST_HEADER_STRING = "HEADER_VALUE";
#define TEST_HTTP_CONTENT    "http_content"
static size_t TEST_HTTP_CONTENT_LENGTH = sizeof(TEST_HTTP_CONTENT);
static const unsigned char TEST_HTTP_CONTENT_CHUNK[] = { 0x52, 0x49, 0x46, 0x46, 0x0A, 0x31, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74 };
static size_t TEST_HTTP_CONTENT_CHUNK_LEN = 15;
static size_t HEADER_COUNT = 9;
static int TEST_PORT_NUM = 8080;

static const char* TEST_HTTP_EXAMPLE[] = 
{
    "HTTP/1.1 200 OK\r\nDate: Mon, 23 May 2005 22:38:34 GMT\r\nContent-Type: tex",
    "t/html; charset=UTF-8\r\nContent-Encoding: UTF-8\r\nContent-Leng",
    "th: 118\r\nLast-Modified: Wed, 08 Jan 2003 23:11:55 GMT\r\nServer: Apache/1.3.3.7 (Unix)(Red-Hat/Linux)\r\n",
    "ETag: \"3f80f-1b6-3e1cb03b\"\r\nAccept-",
    "Ranges: bytes\r\nConnection: close\r\n\r\n<html><head><title>An Example Page</title>",
    "</head><body>Hello World, this is a very simple HTML document.</body></html>\r\n\r\n" 
};

static const char* TEST_HTTP_NO_CONTENT_EXAMPLE[] =
{
    "HTTP/1.1 204 No Content\r\nContent-Length:",
    "0\r\nServer: Microsoft-HTTPAPI/2.0\r\nDate: Wed",
    ", 25 May 2016 00:07:51 GMT\r\n\r\n"
};

static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL_2[] =
{
    "H",
    "TTP/1.1 200 OK",
    "\r\nDate: Thu, 11 May 2017 21:52:38 GMT\r\nContent-Typ",
    "e: application/json; charset=utf-8\r\nTransfer-Encoding: chunked\r\n",
    "\r\n",
    "a",
    "5\r\n",
    "{\"enrollmentGroupId\":\"RIoT_Device\",\"attestation\":{\"x509\":{}},\"etag\":\"\\\"14009e7f-0000-0000-0000-5914dd230000\\\"\",\"generationId\":\"9d546a85-5e8a-44c3-8946-c8ce4f54e5b6\"}\r\n",
    "0\r\n\r\n"
};
#define CHUNK_IRL_LENGTH_2_1      165

static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL_3[] =
{
    "HTTP/1.1 200 OK\r\nDate: Tue, 09 May 2017 00:04:30 GMT\r\nContent-Type: application/json; charset=utf-8\r\nTransfer-Encoding: chunked\r\n\r\n29d\r\n",
    "{\"registrationId\":\"AAaVpgGlHlHY0zcAitlUv%2bkcvkVsd6bZ5XrcEm%2bzEdw%3d\",\"deviceId\":\"AFake_do_dev\",\"attestation\":{\"tpm\":{\"endorsementKey\":\"AToaAQALAqMqsqAqgqGqZqSqs/gqkqyqRqXqJq1q1q4qUqtq8qHqGqMqaqoABgCAAEMAEAgAAAAAAAEAzKJy5Ar/TZrRNU3jAq1knxz/6QkDAD3SIs8yf87D9560p4ikwbwE63RD1mghFnenLUexfAikMOqCZBOQ0Hcn6rVQMqhO8vTCg+0eKkb2KoqP7OwlZoLx5ZTsOav2fpX09PRtnlGV/E2u6Ih9lNDYlYcFyM3zJ7UKWBSkLx9Api3xfCUtzN4rhvbJVepzGxrrBvzR8b4QP4UVUTcO1Ptsr3LnXAw8xcI1c64vsFIdALcLZrEgJhEB9CCG9wSuBnr9SwRF7c+hVYrX2ffn+JGjUyexra7MjDTPDEREMwERmskmjHxXo8kKxbxwClBnMa+B4hFCwMfjtmvBITfe+YnHSw==\"}},\"etag\":\"\\\"0900ae54-0000-0000-0000-5911078f0000\\\"\",\"generationId\":\"1e9523ce-732d-4920-96eb-abf9e2e27b75\"}\r\n0\r\n\r\n"
};
#define CHUNK_IRL_LENGTH_3_1      669

static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL[] =
{
    "HTTP/1.1 200 OK\r\nDate: Thu, 04 May 2017 18:03:35 GMT\r\n",
    "Content-Type: application/json; charset=utf-8\r\nTransfer-Encoding: ",
    "chunked\r\n\r\n1A\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n10\r\n",
    "1234567890123456\r",
    "\n100\r\n1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF\r\n0\r\n\r\n"
};
#define CHUNK_IRL_LENGTH_1      26
#define CHUNK_IRL_LENGTH_2      16
#define CHUNK_IRL_LENGTH_3      256

/*static const char* TEST_HTTP_CHUNK_EXAMPLE_IRL_2[] =
{
    "HTTP/1.1 401 Unauthorized\r\nDate: Tue, 09 May 2017 18:28:44 GMT\r\nContent-Type: application/json; charset=utf-8\r\nTransfer-Encoding: chunked\r\n\r\n400\r\n{\"message\":\"Authorization required, resend request using supplied key\",\"authenticationKey\":\"ADQAIIvHKlrqd0xE1r7NQXDB/IrVsCBXBjvQxhkR+DLUngtYEL36OLX3Id8pN2N4pIGDj8FDAQA6P1J6aDVixNB5TMiO5D9Kx7tvsqjju31eoSTIfIn37Ud2tm2lvv3FeAzauG40YEUgEymyLgzw74I0rIJYhWhSzKgacFx7q05TCh0NvaZ8X28cRPMlhhmsYzfNzGnNMDQgj/5n5XZkvznkf7MLh+USrFS6Q0mkZrUWrMl2iHuYn7HUIxRaEmb5Srk4YIRyXbdUwKP9fiUPPIk984ufKxy7kbrod9xFV7PDL2eX9Hb12XWDxIUP2V2IIgUviyCbXKsuMrM3v0jgD88VJfSpjQcvxBN/lm7Q3IUjgsgaDCmod98XH/7Rj/yBiv3nTK93gi91bayAim3+1MbvD7UfQALtAI4AIOeBYtB+qk+2vbILTHVw8Rv6e+jgnkiZhQyxoahFBQ5EbEbEZQgwo9voYrYLuLbPf4GU2yl0Hqvlu5MC5gfhzSAFF26TsPT1AyPbWTsDwG3EKceHJGNe1WgzmL3TMB2wcSw3d/rI/qORtw6EiCE6REVav5i8rnAWmVB7G2Pap/TW+j54terM0CbsS+0ZAQBAHTflOCaVGPpKP/PwKdzOhMKh/FVW0MfNtJoN1bvP+eYEgbOnAPX7nG7bum2UktgDiZx7rx75tOFUQ6HlUgnQmwrr+CAdGQK+hOu9KjUgzAbOdnCtGNBE5W2ufEpByCp3g7kBBh6duJN5B69xP+FtdMZ/l2FbLeYPjIKt3C5d55rlsyfuiTZw414u4DQngSdAvjFUs1UH62CBJcX+gJLQ6B3w0vnE9Mih1mrnuVwoz5e6JWaacG70IZu3SWj0SQbAapK3GxkGxQAXU/WtVHpgNDxrkiLG1yQiqISkXCMVeqoZVolcwF5fSwgzbppMlLY+\r\n1c1\r\nsDGuAwU8xqNVyn6nPcaXADAACAALAAQEQAAAAAUACwAg/yMycoNJTM2h1DGjguQccuxhy+pHvv4XNTdZYizhwX4AvrHKBGZ78zk2Wlk+7R3OS6mY2S92u9wjDU4N4PMWS4CdT5sxXH7dkYp7jbvhN+8GiYmsoPePQLg30Eqv9wf9DJaHBbJOKHTyKQbMiavospzvOxQj/46KRRxgaHadzAwwjaF9Z8VqxqjhgruNXzySjdmRyDXliAqPgGbEPl1cj1zLjegUCch+8PYTUnukiF7SR5tVAYeOO/W2L9qyYBtsLpOWHPIRXj0AvnC/eZuxaWAif9R/ulcC41nH9S8feWA=\",\"unencryptedAuthenticationKey\":\"x/xWHUmzcm5NAY6vGZwMZ3vl7ylPfnGHcQN9RGkf1HM=\",\"keyName\":\"registration\"}\r\n0\r\n\r\n"
};
#define CHUNK_IRL_2_LENGTH_1      1024
#define CHUNK_IRL_2_LENGTH_2      449*/

static const char* TEST_HTTP_CHUNK_EXAMPLE[] =
{
    "HTTP/1.1 200 OK\r\nDate: Thu, 04 May 2017 18:03:35 GMT\r\n",
    "Content-Type: application/json; charset=utf-8\r\nTransfer-Encoding: ",
    "chunked\r\n\r\n12;this is junk\r\n1234567890ABCDEFGH\r\n9\r\nIJKLMNOPQ\r\n0\r\n\r\n"
};
#define CHUNK_EXAMPLE_LENGTH_1      18
#define CHUNK_EXAMPLE_LENGTH_2      9

static const char* TEST_HTTP_CHUNK_SPLIT_EXAMPLE[] =
{
    "HTTP/1.1 200 OK\r\nDate: Thu, 04 May 2017 18:03:35 GMT\r\n",
    "Content-Type: application/json; charset=utf-8\r\nTransfer-Encoding: ",
    "chunked\r\n\r\n12\r\n1234567890ABCDEF",
    "GH\r\n0f\r\nIJKLMNOPQRSTUVW\r\n0\r\n\r\n"
};
#define CHUNK_SPLIT_EXAMPLE_LENGTH_1      18
#define CHUNK_SPLIT_EXAMPLE_LENGTH_2      15

static const char* TEST_SMALL_HTTP_EXAMPLE = "HTTP/1.1 200 OK\r\nDate: Mon, 23 May 2005 22:38:34 GMT\r\nContent-Type: text/html; charset=UTF-8\r\nAccept-Ranges: bytes\r\n\r\n<html><head><title>An Example Page</title></head><body>Hello World, this is a very simple HTML document.</body></html>\r\n\r\n";

static const char* TEST_HTTP_BODY = "<html><head><title>An Example Page</title></head><body>Hello World, this is a very simple HTML document.</body></html>\r\n\r\n";

void my_on_msg_recv_callback(void* callback_ctx, HTTP_CALLBACK_REASON request_result, const unsigned char* content, size_t content_length, unsigned int status_code,
    HTTP_HEADERS_HANDLE response_header)
{
    (void)callback_ctx;
    (void)content;
    (void)response_header;

    g_http_cb_reason = request_result;
    g_http_cb_content_len = content_length;
    g_http_cb_status_code = status_code;
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len+1);
    strcpy(*destination, source);
    return 0;
}

static XIO_HANDLE my_xio_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, const void* io_create_parameters)
{
    (void)io_interface_description;
    (void)io_create_parameters;
    return (XIO_HANDLE)my_gballoc_malloc(1);
}

static void my_xio_destroy(XIO_HANDLE xio)
{
    my_gballoc_free(xio);
}

static int my_xio_open(XIO_HANDLE xio, ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_context, ON_BYTES_RECEIVED on_bytes_received, void* on_bytes_received_context, ON_IO_ERROR on_io_error, void* on_io_error_context)
{
    (void)xio;
    g_on_open_complete = on_io_open_complete;
    g_on_open_complete_context = on_io_open_complete_context;
    g_onBytesRecv = on_bytes_received;
    g_onBytesRecv_ctx = on_bytes_received_context;
    g_on_io_error = on_io_error;
    g_on_io_error_ctx = on_io_error_context;
    return 0;
}

static int my_xio_send(XIO_HANDLE xio, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context)
{
    (void)xio;
    (void)buffer;
    (void)size;
    g_on_xio_send_complete = on_send_complete;
    (void)callback_context;
    return 0;
}

static int my_xio_close(XIO_HANDLE xio, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context)
{
    (void)xio;
    g_on_close_complete = on_io_close_complete;
    g_on_close_complete_context = callback_context;
    return 0;
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)my_gballoc_malloc(1);
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Clone(HTTP_HEADERS_HANDLE handle)
{
    (void)handle;
    return (HTTP_HEADERS_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE handle)
{
    my_gballoc_free(handle);
}

static HTTP_HEADERS_RESULT my_HTTPHeaders_GetHeaderCount(HTTP_HEADERS_HANDLE handle, size_t* headerCount)
{
    (void)handle;
    *headerCount = g_header_count;
    return HTTP_HEADERS_OK;
}

static HTTP_HEADERS_RESULT my_HTTPHeaders_GetHeader(HTTP_HEADERS_HANDLE handle, size_t index, char** destination)
{
    (void)handle;
    (void)index;
    size_t len = strlen(TEST_HEADER_STRING);
    *destination = (char*)my_gballoc_malloc(len+1);
    strcpy(*destination, TEST_HEADER_STRING);
    return HTTP_HEADERS_OK;
}

static STRING_HANDLE my_STRING_new(void)
{
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    my_gballoc_free(handle);
}

static SINGLYLINKEDLIST_HANDLE my_singlylinkedlist_create(void)
{
    return (SINGLYLINKEDLIST_HANDLE)my_gballoc_malloc(1);
}

static void my_singlylinkedlist_destroy(SINGLYLINKEDLIST_HANDLE list)
{
    my_gballoc_free(list);
    my_gballoc_free((void*)g_list_items);
}

static LIST_ITEM_HANDLE my_singlylinkedlist_get_head_item(SINGLYLINKEDLIST_HANDLE list)
{
    LIST_ITEM_HANDLE listHandle = NULL;
    (void)list;
    if (g_list_item_count > 0)
    {
        listHandle = (LIST_ITEM_HANDLE)g_list_items[0];
        g_list_item_count--;
    }
    return listHandle;
}

static LIST_ITEM_HANDLE my_singlylinkedlist_add(SINGLYLINKEDLIST_HANDLE list, const void* item)
{
    const void** items = (const void**)my_gballoc_realloc((void*)g_list_items, (g_list_item_count + 1) * sizeof(item));
    (void)list;
    if (items != NULL)
    {
        g_list_items = items;
        g_list_items[g_list_item_count++] = item;
    }
    g_list_add_called = true;
    return (LIST_ITEM_HANDLE)g_list_item_count;
}

static const void* my_singlylinkedlist_item_get_value(LIST_ITEM_HANDLE item_handle)
{
    const void* resultPtr = NULL;
    if (g_list_add_called)
    {
        resultPtr = item_handle;
    }
    if (g_list_item_count == 0)
    {
        g_list_add_called = false;
    }
    return (const void*)resultPtr;
}

static int my_singlylinkedlist_remove(SINGLYLINKEDLIST_HANDLE list, LIST_ITEM_HANDLE item)
{
    (void)list;
    (void)item;

    if (g_list_add_called)
    {
        if (g_list_item_count > 0)
        {
            g_list_item_count--;
        }
    }
    if (g_list_item_count == 0)
    {
        g_list_add_called = false;
    }
    return 0;
}

TEST_DEFINE_ENUM_TYPE(HTTP_CLIENT_RESULT, HTTP_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTP_CLIENT_RESULT, HTTP_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_VALUES);

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(uhttp_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT);
    REGISTER_TYPE(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON);

    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_OPEN_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_BYTES_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_ERROR, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_CLOSE_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_SEND_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_RESULT, int);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

    REGISTER_GLOBAL_MOCK_HOOK(on_msg_recv_callback, my_on_msg_recv_callback);

    REGISTER_GLOBAL_MOCK_HOOK(xio_create, my_xio_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(xio_open, my_xio_open);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_open, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(xio_send, my_xio_send);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_send, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(xio_close, my_xio_close);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_close, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(xio_destroy, my_xio_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, real_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, real_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, real_BUFFER_delete);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_build, real_BUFFER_build);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_build, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_u_char, real_BUFFER_u_char);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_length, real_BUFFER_length);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_append_build, real_BUFFER_append_build);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_append_build, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_shrink, real_BUFFER_shrink);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_shrink, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_clone, real_BUFFER_clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_clone, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_length, TEST_STRING_LENGTH);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, __LINE__);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Alloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_GetHeaderCount, my_HTTPHeaders_GetHeaderCount);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_GetHeaderCount, HTTP_HEADERS_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_GetHeader, my_HTTPHeaders_GetHeader);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_GetHeader, HTTP_HEADERS_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Clone, my_HTTPHeaders_Clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Clone, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_create, my_singlylinkedlist_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_destroy, my_singlylinkedlist_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_add, my_singlylinkedlist_add);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_remove, my_singlylinkedlist_remove);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_head_item, my_singlylinkedlist_get_head_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_get_head_item, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_item_get_value, my_singlylinkedlist_item_get_value);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_item_get_value, NULL);
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

    g_http_cb_reason = HTTP_CALLBACK_REASON_DISCONNECTED;
    g_http_cb_content_len = 0;
    g_http_cb_status_code = 0;

    g_onBytesRecv = NULL;
    g_onBytesRecv_ctx = NULL;
    g_on_io_error = NULL;
    g_on_io_error_ctx = NULL;
    g_header_count = 0;

    g_list_items = NULL;
    g_list_item_count = 0;
    g_list_add_called = false;
    g_onBytesRecv = NULL;
    g_onBytesRecv_ctx = NULL;
    g_on_open_complete = NULL;
    g_on_open_complete_context = NULL;
    g_on_close_complete = NULL;
    g_on_close_complete_context = NULL;
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

static void setup_uhttp_client_execute_request_no_content_mocks()
{
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_headersCount()
        .IgnoreArgument_httpHeadersHandle();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_item()
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument_psz();
}

static void setup_uhttp_client_execute_request_with_content_mocks()
{
    g_header_count = 1;

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
}

static void setup_uhttp_client_dowork_no_msg_mocks()
{
    EXPECTED_CALL(xio_dowork(IGNORED_PTR_ARG))
        .IgnoreArgument_xio();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument_item_handle();
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_s1()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_xio()
        .IgnoreArgument_buffer()
        .IgnoreArgument_callback_context()
        .IgnoreArgument_on_send_complete()
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG,IGNORED_PTR_ARG))
        .IgnoreArgument_item_handle()
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument_list();
}

static void setup_uhttp_client_dowork_msg_mocks()
{
    EXPECTED_CALL(xio_dowork(IGNORED_PTR_ARG))
        .IgnoreArgument_xio();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument_item_handle();
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(TEST_HTTP_CONTENT_LENGTH);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_s1()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_xio()
        .IgnoreArgument_buffer()
        .IgnoreArgument_callback_context()
        .IgnoreArgument_on_send_complete()
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn((unsigned char*)TEST_HTTP_CONTENT);
    STRICT_EXPECTED_CALL(xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_xio()
        .IgnoreArgument_buffer()
        .IgnoreArgument_callback_context()
        .IgnoreArgument_on_send_complete()
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG,IGNORED_PTR_ARG))
        .IgnoreArgument_item_handle()
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument_list();
}

static void SetupProcessHeader()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void setup_uhttp_client_onBytesReceived_small_ex()
{
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    SetupProcessHeader();
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(on_msg_recv_callback(IGNORED_PTR_ARG, HTTP_CALLBACK_REASON_OK, IGNORED_PTR_ARG, IGNORED_NUM_ARG, 200, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
}

/* uhttp_client_create */
/* Tests_SRS_UHTTP_07_003: [If uhttp_client_create encounters any error then it shall return NULL] */
TEST_FUNCTION(uhttp_client_create_fails)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "uhttp_client_create failure in test %zu/%zu", index, count);

        HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);

        // assert
        ASSERT_IS_NULL(clientHandle);
    }

    // Cleanup
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_UHTTP_07_002: [If io_interface_desc is NULL, uhttp_client_create shall return NULL.] */
TEST_FUNCTION(uhttp_client_create_interface_NULL_fail)
{
    // arrange

    // act
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(NULL, TEST_CREATE_PARAM, on_error_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(clientHandle);

    // Cleanup
}

/* Tests_SRS_UHTTP_07_001: [uhttp_client_create shall return an initialize the http client handle.] */
TEST_FUNCTION(uhttp_client_create_succeeds)
{
    // arrange
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(clientHandle);

    // Cleanup
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_050: [ if context is NULL on_io_error shall do nothing. ] */
/* Tests_SRS_UHTTP_07_051: [ if on_error callback is not NULL, on_io_error shall call on_error callback. ] */
TEST_FUNCTION(uhttp_client_on_error_succeeds)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, TEST_ERROR_CONTEXT);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(on_error_callback(TEST_ERROR_CONTEXT, HTTP_CALLBACK_REASON_ERROR) );

    // act
    ASSERT_IS_NOT_NULL(g_on_io_error);
    g_on_io_error(g_on_io_error_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* uhttp_client_destroy */
/* Tests_SRS_UHTTP_07_004: [if hanlde is NULL then http_client_destroy shall do nothing] */
TEST_FUNCTION(uhttp_client_destroy_http_handle_NULL_succeeds)
{
    // arrange

    // act
    uhttp_client_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_UHTTP_07_005: [uhttp_client_destroy shall free any memory that is allocated in this translation unit] */
TEST_FUNCTION(uhttp_client_destroy_succeeds)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    uhttp_client_destroy(clientHandle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_UHTTP_07_006: [If handle, xio or host is NULL then `uhttp_client_open` shall return HTTP_CLIENT_INVALID_ARG] */
TEST_FUNCTION(uhttp_client_open_HTTP_CLIENT_HANDLE_NULL_fail)
{
    // arrange

    // act
    HTTP_CLIENT_RESULT httpResult = uhttp_client_open(NULL, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(HTTP_CLIENT_RESULT, HTTP_CLIENT_INVALID_ARG, httpResult);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_UHTTP_07_008: [If uhttp_client_open succeeds then it shall return HTTP_CLIENT_OK] */
TEST_FUNCTION(uhttp_client_open_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#ifdef USE_OPENSSL
    STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, "tls_version", IGNORED_PTR_ARG));
#endif
    EXPECTED_CALL(xio_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    HTTP_CLIENT_RESULT httpResult = uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls() );
    ASSERT_ARE_EQUAL(HTTP_CLIENT_RESULT, HTTP_CLIENT_OK, httpResult);

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_049: [ If not NULL uhttp_client_open shall call the on_connect callback with the callback_ctx, once the underlying xio's open is complete. ] */
/* Tests_SRS_UHTTP_07_042: [ If the underlying socket opens successfully the on_connect callback shall be call with HTTP_CALLBACK_REASON_OK... ] */
/* Tests_SRS_UHTTP_07_008: [If uhttp_client_open succeeds then it shall return HTTP_CLIENT_OK] */
/* Tests_SRS_UHTTP_07_007: [http_client_connect shall attempt to open the xio_handle. ] */
TEST_FUNCTION(uhttp_client_open_complete_called_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(on_connection_callback(TEST_CONNECT_CONTEXT, HTTP_CALLBACK_REASON_OK));

    // act
    ASSERT_IS_NOT_NULL(g_on_open_complete);
    g_on_open_complete(g_on_open_complete_context, IO_OPEN_OK);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls() );

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_049: [ If not NULL uhttp_client_open shall call the on_connect callback with the callback_ctx, once the underlying xio's open is complete. ] */
/* Tests_SRS_UHTTP_07_043: [ Otherwise on_connect callback shall be call with HTTP_CLIENT_OPEN_REQUEST_FAILED. ] */
TEST_FUNCTION(uhttp_client_open_complete_called_with_error_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(on_connection_callback(TEST_CONNECT_CONTEXT, HTTP_CALLBACK_REASON_OPEN_FAILED));

    // act
    ASSERT_IS_NOT_NULL(g_on_open_complete);
    g_on_open_complete(g_on_open_complete_context, IO_OPEN_ERROR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls() );

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_044: [ if a failure is encountered on xio_open uhttp_client_open shall return HTTP_CLIENT_OPEN_REQUEST_FAILED. ] */
TEST_FUNCTION(uhttp_client_open_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_destination()
        .IgnoreArgument_source();
    EXPECTED_CALL(xio_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "uhttp_client_open failure in test %zu/%zu", index, count);
        
        HTTP_CLIENT_RESULT httpResult = uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);

        //assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(HTTP_CLIENT_RESULT, HTTP_CLIENT_OK, httpResult, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_009: [If handle is NULL then uhttp_client_close shall do nothing] */
TEST_FUNCTION(uhttp_client_close_HTTP_CLIENT_HANDLE_NULL_fail)
{
    // arrange

    // act
    uhttp_client_close(NULL, on_closed_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_UHTTP_07_010: [If the xio_handle is NOT NULL uhttp_client_close shall call xio_close to close the handle] */
/* Tests_SRS_UHTTP_07_011: [uhttp_client_close shall free any HTTPHeader object that has not been freed] */
TEST_FUNCTION(uhttp_client_close_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    EXPECTED_CALL(xio_close(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    // act
    uhttp_client_close(clientHandle, on_closed_callback, TEST_CLOSE_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_045: [ If on_close_callback is not NULL, on_close_callback shall be called once the underlying xio is closed. ] */
TEST_FUNCTION(uhttp_client_close_callback_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    uhttp_client_close(clientHandle, on_closed_callback, TEST_CLOSE_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(on_closed_callback(TEST_CLOSE_CONTEXT));

    // act
    g_on_close_complete(g_on_close_complete_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_049: [ If the state has been previously set to state_message_closed, uhttp_client_close shall do nothing. ] */
TEST_FUNCTION(uhttp_client_close_called_twice_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    uhttp_client_close(clientHandle, on_closed_callback, TEST_CLOSE_CONTEXT);
    umock_c_reset_all_calls();

    // act
    uhttp_client_close(clientHandle, on_closed_callback, TEST_CLOSE_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_010: [If the xio_handle is NOT NULL uhttp_client_close shall call xio_close]*/
TEST_FUNCTION(uhttp_client_close_free_headers_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_POST, "/", NULL, NULL, 0, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();

    EXPECTED_CALL(xio_close(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .IgnoreArgument_item_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG,IGNORED_PTR_ARG))
        .IgnoreArgument_item_handle()
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .IgnoreArgument_list();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument_httpHeadersHandle();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    uhttp_client_close(clientHandle, on_closed_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_012: [If handle, or on_http_reply_recv is NULL then `uhttp_client_execute_request` shall return HTTP_CLIENT_INVALID_ARG] */
TEST_FUNCTION(uhttp_client_execute_request_http_client_handle_NULL_fail)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    HTTP_CLIENT_RESULT httpResult = uhttp_client_execute_request(NULL, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, NULL, 0, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(HTTP_CLIENT_RESULT, HTTP_CLIENT_INVALID_ARG, httpResult);

    // Cleanup
}

/* Tests_SRS_UHTTP_07_017: [If any failure encountered uhttp_client_execute_request shall return HTTP_CLIENT_ERROR] */
TEST_FUNCTION(uhttp_client_execute_request_no_content_fails)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_uhttp_client_execute_request_no_content_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 7, 10, 12, 13, 15 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "uhttp_client_execute_request failure in test %zu/%zu", index, count);

        HTTP_CLIENT_RESULT httpResult = uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, NULL, 0, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(HTTP_CLIENT_RESULT, HTTP_CLIENT_OK, httpResult, tmp_msg);
    }

    // Cleanup
    umock_c_negative_tests_deinit();
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_016: [http_client_dowork shall iterate through the queued http data and send them to the http endpoint.] */
/* Tests_SRS_UHTTP_07_018: [upon success uhttp_client_execute_request shall then transmit the content data, if supplied, through a call to xio_send.] */
/* Tests_SRS_UHTTP_07_015: [uhttp_client_execute_request shall add the Content-Length http header item to the request if the contentLength is > 0] */
/* Tests_SRS_UHTTP_07_046: [ http_client_dowork shall free resouces queued to send to the http endpoint. ] */
TEST_FUNCTION(uhttp_client_execute_request_no_content_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    setup_uhttp_client_execute_request_no_content_mocks();

    // act
    HTTP_CLIENT_RESULT httpResult = uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, NULL, 0, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(HTTP_CLIENT_RESULT, HTTP_CLIENT_OK, httpResult);

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_016: [http_client_dowork shall iterate through the queued http data and send them to the http endpoint.] */
/* Tests_SRS_UHTTP_07_018: [upon success uhttp_client_execute_request shall then transmit the content data, if supplied, through a call to xio_send.] */
/* Tests_SRS_UHTTP_07_015: [uhttp_client_execute_request shall add the Content-Length http header item to the request if the contentLength is > 0] */
/* Tests_SRS_UHTTP_07_046: [ http_client_dowork shall free resouces queued to send to the http endpoint. ] */
TEST_FUNCTION(uhttp_client_execute_request_with_content_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    setup_uhttp_client_execute_request_with_content_mocks();

    // act
    HTTP_CLIENT_RESULT httpResult = uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_POST, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(HTTP_CLIENT_RESULT, HTTP_CLIENT_OK, httpResult);

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_046: [ http_client_dowork shall free resouces queued to send to the http endpoint. ] */
TEST_FUNCTION(uhttp_client_execute_request_with_content_fails)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_uhttp_client_execute_request_with_content_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 6, 9, 10, 11, 13, 16 };

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "uhttp_client_execute_request failure in test %zu/%zu", index, count);

        HTTP_CLIENT_RESULT httpResult = uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_POST, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(HTTP_CLIENT_RESULT, HTTP_CLIENT_OK, httpResult, tmp_msg);
    }

    // Cleanup
    umock_c_negative_tests_deinit();
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_036: [If handle is NULL then http_client_dowork shall do nothing] */
TEST_FUNCTION(uhttp_client_dowork_http_client_handle_null_fail)
{
    // arrange

    // act
    uhttp_client_dowork(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
}

/* Tests_SRS_UHTTP_07_016: [http_client_dowork shall iterate through the queued Data using the xio interface to send the http request in the following ways...] */
/* Tests_SRS_UHTTP_07_052: [uhttp_client_dowork shall call xio_send to transmits the header information... ] */
/* Tests_SRS_UHTTP_07_053: [Then uhttp_client_dowork shall use xio_send to transmit the content of the http request if supplied. ] */
TEST_FUNCTION(uhttp_client_dowork_msg_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();

    setup_uhttp_client_dowork_msg_mocks();

    // act
    uhttp_client_dowork(clientHandle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_046: [ http_client_dowork shall free resouces queued to send to the http endpoint. ] */
TEST_FUNCTION(uhttp_client_dowork_msg_fails)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_uhttp_client_dowork_msg_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 0, 1, 3, 4, 8, 9, 11, 12, 14, 15, 16, 17, 18, 19 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        // We fail singlylinkedlist_item_get_value function which will cause a 
        // memory leak if this is called
        if (index != 2)
        {
            (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
            (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "uhttp_client_dowork failure in test %zu/%zu", index, count);

        uhttp_client_dowork(clientHandle);

        uhttp_client_close(clientHandle, on_closed_callback, NULL);
        g_on_close_complete(g_on_close_complete_context);
    }

    // Cleanup
    umock_c_negative_tests_deinit();
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_037: [http_client_dowork shall call the underlying xio_dowork function. ] */
TEST_FUNCTION(uhttp_client_dowork_no_msg_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, NULL, 0, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();

    setup_uhttp_client_dowork_no_msg_mocks();

    // act
    uhttp_client_dowork(clientHandle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_037: [http_client_dowork shall call the underlying xio_dowork function. ] */
TEST_FUNCTION(uhttp_client_dowork_no_msg_fails)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_uhttp_client_dowork_no_msg_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 0, 1, 2, 3, 4, 8, 9, 11, 12, 13, 14, 15, 16 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, NULL, 0, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "uhttp_client_dowork failure in test %zu/%zu", index, count);

        uhttp_client_dowork(clientHandle);
    }

    // Cleanup
    umock_c_negative_tests_deinit();
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_047: [ If context or buffer is NULL on_bytes_received shall do nothing. ] */
TEST_FUNCTION(uhttp_client_onBytesReceived_context_NULL_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();

    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    g_onBytesRecv(NULL, (const unsigned char*)TEST_HTTP_BODY, strlen(TEST_HTTP_BODY));

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_047: [ If context or buffer is NULL on_bytes_received shall do nothing. ] */
TEST_FUNCTION(uhttp_client_onBytesReceived_buffer_NULL_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();

    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    g_onBytesRecv(g_onBytesRecv_ctx, NULL, 0);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_015: [on_bytes_received shall add the Content-Length http header item to the request.] */
/* Tests_SRS_UHTTP_07_047: [ If context or buffer is NULL on_bytes_received shall do nothing. ] */
/* Tests_SRS_UHTTP_07_048: [ If any error is encountered on_bytes_received shall set the state to error. ] */
/* Tests_SRS_UHTTP_07_015: [on_bytes_received shall add the Content-Length http header item to the request.] */
/* Tests_SRS_UHTTP_07_011: [on_bytes_received shall add the HOST http header item to the request if not supplied (RFC 7230 - 5.4 ).] */
TEST_FUNCTION(uhttp_client_onBytesReceived_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();
    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));

    SetupProcessHeader();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));

    SetupProcessHeader();
    SetupProcessHeader();

    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));

    SetupProcessHeader();
    SetupProcessHeader();
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(on_msg_recv_callback(IGNORED_PTR_ARG, HTTP_CALLBACK_REASON_OK, IGNORED_PTR_ARG, IGNORED_NUM_ARG, 200, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    size_t count = sizeof(TEST_HTTP_EXAMPLE)/sizeof(TEST_HTTP_EXAMPLE[0]);
    for (size_t index = 0; index < count; index++)
    {
        g_onBytesRecv(g_onBytesRecv_ctx, (const unsigned char*)TEST_HTTP_EXAMPLE[index], strlen(TEST_HTTP_EXAMPLE[index]));
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_015: [on_bytes_received shall add the Content-Length http header item to the request.] */
/* Tests_SRS_UHTTP_07_047: [ If context or buffer is NULL on_bytes_received shall do nothing. ] */
/* Tests_SRS_UHTTP_07_048: [ If any error is encountered on_bytes_received shall set the state to error. ] */
/* Tests_SRS_UHTTP_07_015: [on_bytes_received shall add the Content-Length http header item to the request.] */
/* Tests_SRS_UHTTP_07_011: [on_bytes_received shall add the HOST http header item to the request if not supplied (RFC 7230 - 5.4 ).] */
TEST_FUNCTION(uhttp_client_onBytesReceived_small_ex_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();
    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    setup_uhttp_client_onBytesReceived_small_ex();

    size_t http_len = strlen(TEST_SMALL_HTTP_EXAMPLE);
    g_onBytesRecv(g_onBytesRecv_ctx, (const unsigned char*)TEST_SMALL_HTTP_EXAMPLE, http_len);

    // assert
    ASSERT_ARE_EQUAL(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_OK, g_http_cb_reason);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_054: [ If the http header does not include a content length then it indicates a chunk response. ] */
/* Tests_SRS_UHTTP_07_055: [ on_bytes_received shall convert the hexs length supplied in the response to the data length of the chunked data. ] */
/* Tests_SRS_UHTTP_07_056: [ After the response chunk is parsed it shall be placed in a BUFFER_HANDLE. ] */
/* Tests_SRS_UHTTP_07_057: [ Once the response is stored on_bytes_received shall free the bytes that are stored and shrink the stored bytes buffer. ] */
/* Tests_SRS_UHTTP_07_058: [ Once a chunk size value of 0 is encountered on_bytes_received shall call the on_request_callback with the http message ] */
TEST_FUNCTION(uhttp_client_onBytesReceived_chunk_response_1_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();
    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(on_msg_recv_callback(IGNORED_PTR_ARG, HTTP_CALLBACK_REASON_OK, IGNORED_PTR_ARG, IGNORED_NUM_ARG, 200, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    size_t count = sizeof(TEST_HTTP_CHUNK_EXAMPLE) / sizeof(TEST_HTTP_CHUNK_EXAMPLE[0]);
    for (size_t index = 0; index < count; index++)
    {
        g_onBytesRecv(g_onBytesRecv_ctx, (const unsigned char*)TEST_HTTP_CHUNK_EXAMPLE[index], strlen(TEST_HTTP_CHUNK_EXAMPLE[index]));
    }

    // assert
    ASSERT_ARE_EQUAL(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_OK, g_http_cb_reason);
    ASSERT_ARE_EQUAL(size_t, CHUNK_EXAMPLE_LENGTH_1 + CHUNK_EXAMPLE_LENGTH_2, g_http_cb_content_len);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_054: [ If the http header does not include a content length then it indicates a chunk response. ] */
/* Tests_SRS_UHTTP_07_055: [ on_bytes_received shall convert the hexs length supplied in the response to the data length of the chunked data. ] */
/* Tests_SRS_UHTTP_07_056: [ After the response chunk is parsed it shall be placed in a BUFFER_HANDLE. ] */
/* Tests_SRS_UHTTP_07_057: [ Once the response is stored on_bytes_received shall free the bytes that are stored and shrink the stored bytes buffer. ] */
/* Tests_SRS_UHTTP_07_058: [ Once a chunk size value of 0 is encountered on_bytes_received shall call the on_request_callback with the http message ] */
TEST_FUNCTION(uhttp_client_onBytesReceived_chunk_response_2_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();
    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(on_msg_recv_callback(IGNORED_PTR_ARG, HTTP_CALLBACK_REASON_OK, IGNORED_PTR_ARG, IGNORED_NUM_ARG, 200, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    size_t count = sizeof(TEST_HTTP_CHUNK_SPLIT_EXAMPLE) / sizeof(TEST_HTTP_CHUNK_SPLIT_EXAMPLE[0]);
    for (size_t index = 0; index < count; index++)
    {
        g_onBytesRecv(g_onBytesRecv_ctx, (const unsigned char*)TEST_HTTP_CHUNK_SPLIT_EXAMPLE[index], strlen(TEST_HTTP_CHUNK_SPLIT_EXAMPLE[index]));
    }

    // assert
    ASSERT_ARE_EQUAL(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_OK, g_http_cb_reason);
    ASSERT_ARE_EQUAL(size_t, CHUNK_SPLIT_EXAMPLE_LENGTH_1+CHUNK_SPLIT_EXAMPLE_LENGTH_2, g_http_cb_content_len);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_054: [ If the http header does not include a content length then it indicates a chunk response. ] */
/* Tests_SRS_UHTTP_07_055: [ on_bytes_received shall convert the hexs length supplied in the response to the data length of the chunked data. ] */
/* Tests_SRS_UHTTP_07_056: [ After the response chunk is parsed it shall be placed in a BUFFER_HANDLE. ] */
/* Tests_SRS_UHTTP_07_057: [ Once the response is stored on_bytes_received shall free the bytes that are stored and shrink the stored bytes buffer. ] */
/* Tests_SRS_UHTTP_07_058: [ Once a chunk size value of 0 is encountered on_bytes_received shall call the on_request_callback with the http message ] */
TEST_FUNCTION(uhttp_client_onBytesReceived_chunk_response_IRL_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();
    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(on_msg_recv_callback(IGNORED_PTR_ARG, HTTP_CALLBACK_REASON_OK, IGNORED_PTR_ARG, IGNORED_NUM_ARG, 200, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    size_t count = sizeof(TEST_HTTP_CHUNK_EXAMPLE_IRL) / sizeof(TEST_HTTP_CHUNK_EXAMPLE_IRL[0]);
    for (size_t index = 0; index < count; index++)
    {
        g_onBytesRecv(g_onBytesRecv_ctx, (const unsigned char*)TEST_HTTP_CHUNK_EXAMPLE_IRL[index], strlen(TEST_HTTP_CHUNK_EXAMPLE_IRL[index]));
    }

    // assert
    ASSERT_ARE_EQUAL(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_OK, g_http_cb_reason);
    ASSERT_ARE_EQUAL(size_t, g_http_cb_content_len, CHUNK_IRL_LENGTH_1+CHUNK_IRL_LENGTH_2+CHUNK_IRL_LENGTH_3);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_054: [ If the http header does not include a content length then it indicates a chunk response. ] */
/* Tests_SRS_UHTTP_07_055: [ on_bytes_received shall convert the hexs length supplied in the response to the data length of the chunked data. ] */
/* Tests_SRS_UHTTP_07_056: [ After the response chunk is parsed it shall be placed in a BUFFER_HANDLE. ] */
/* Tests_SRS_UHTTP_07_057: [ Once the response is stored on_bytes_received shall free the bytes that are stored and shrink the stored bytes buffer. ] */
/* Tests_SRS_UHTTP_07_058: [ Once a chunk size value of 0 is encountered on_bytes_received shall call the on_request_callback with the http message ] */
/* Tests_SRS_UHTTP_07_059: [ on_bytes_received shall loop throught the stored data to find the /r/n separator. ] */
/* Tests_SRS_UHTTP_07_060: [ if the data_length specified in the chunk is beyond the amound of data recieved, the parsing shall end it with an error. ] */
TEST_FUNCTION(uhttp_client_onBytesReceived_chunk_response_IRL_2_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();
    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    SetupProcessHeader();
    SetupProcessHeader();
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_shrink(IGNORED_PTR_ARG, IGNORED_NUM_ARG, false));
    STRICT_EXPECTED_CALL(BUFFER_append_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(on_msg_recv_callback(IGNORED_PTR_ARG, HTTP_CALLBACK_REASON_OK, IGNORED_PTR_ARG, IGNORED_NUM_ARG, 200, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    size_t count = sizeof(TEST_HTTP_CHUNK_EXAMPLE_IRL_2) / sizeof(TEST_HTTP_CHUNK_EXAMPLE_IRL_2[0]);
    for (size_t index = 0; index < count; index++)
    {
        g_onBytesRecv(g_onBytesRecv_ctx, (const unsigned char*)TEST_HTTP_CHUNK_EXAMPLE_IRL_2[index], strlen(TEST_HTTP_CHUNK_EXAMPLE_IRL_2[index]));
    }

    // assert
    ASSERT_ARE_EQUAL(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_OK, g_http_cb_reason);
    ASSERT_ARE_EQUAL(size_t, CHUNK_IRL_LENGTH_2_1, g_http_cb_content_len);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_048: [ If any error is encountered on_bytes_received shall set the state to error. ] */
TEST_FUNCTION(uhttp_client_onBytesReceived_small_ex_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, (void*)TEST_HTTP_EXAMPLE);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();
    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    // act
    setup_uhttp_client_onBytesReceived_small_ex();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 2, 6, 7, 11, 12, 14, 16, 17, 19, 20, 21, 22 };

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "g_onBytesRecv failure in test %zu/%zu", index, count);

        size_t http_len = strlen(TEST_SMALL_HTTP_EXAMPLE);
        g_onBytesRecv(g_onBytesRecv_ctx, (const unsigned char*)TEST_SMALL_HTTP_EXAMPLE, http_len);

        // assert
    }

    // Cleanup
    umock_c_negative_tests_deinit();
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_048: [ If any error is encountered on_bytes_received shall set the state to error. ] */
TEST_FUNCTION(uhttp_client_onBytesReceived_len_0_fail)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    (void)uhttp_client_open(clientHandle, TEST_HOST_NAME, TEST_PORT_NUM, on_connection_callback, TEST_CONNECT_CONTEXT);
    (void)uhttp_client_execute_request(clientHandle, HTTP_CLIENT_REQUEST_GET, "/", TEST_HTTP_HEADERS_HANDLE, (const unsigned char*)TEST_HTTP_CONTENT, TEST_HTTP_CONTENT_LENGTH, on_msg_recv_callback, TEST_EXECUTE_CONTEXT);
    umock_c_reset_all_calls();
    ASSERT_IS_NOT_NULL(g_onBytesRecv);

    // act
    g_onBytesRecv(g_onBytesRecv_ctx, (const unsigned char*)TEST_HTTP_EXAMPLE[0], 0);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_close(clientHandle, on_closed_callback, NULL);
    uhttp_client_destroy(clientHandle);
}

/* Tests_SRS_UHTTP_07_038: [If handle is NULL then http_client_set_trace shall return HTTP_CLIENT_INVALID_ARG] */
TEST_FUNCTION(uhttp_client_set_trace_handle_NULL_fail)
{
    // arrange

    // act
    HTTP_CLIENT_RESULT httpResult = uhttp_client_set_trace(NULL, true, true);

    // assert
    ASSERT_ARE_EQUAL(HTTP_CLIENT_RESULT, HTTP_CLIENT_INVALID_ARG, httpResult);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
}

/* Tests_SRS_UHTTP_07_039: [http_client_set_trace shall set the HTTP tracing to the trace_on variable.] */
/* Tests_SRS_UHTTP_07_040: [if http_client_set_trace finishes successfully then it shall return HTTP_CLIENT_OK.] */
TEST_FUNCTION(uhttp_client_set_trace_succeed)
{
    // arrange
    HTTP_CLIENT_HANDLE clientHandle = uhttp_client_create(TEST_INTERFACE_DESC, TEST_CREATE_PARAM, on_error_callback, NULL);
    umock_c_reset_all_calls();

    // act
    HTTP_CLIENT_RESULT httpResult = uhttp_client_set_trace(clientHandle, true, true);

    // assert
    ASSERT_ARE_EQUAL(HTTP_CLIENT_RESULT, HTTP_CLIENT_OK, httpResult);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    uhttp_client_destroy(clientHandle);
}

END_TEST_SUITE(uhttp_ut)
