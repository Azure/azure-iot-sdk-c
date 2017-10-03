# uhttp Requirements

================

## Overview

The uhttp module provides a platform independent http implementation

## References

RFC7230 - Hypertext Transfer Protocol

## RFC 7230 features not yet supported

4.1   Sending Chunking message
8.1.0 Persistent Connections
8.2.3 100 Continue Response
14.33 Proxy Authentication
execute_request Timeout

## Exposed API

```c
typedef struct HTTP_CLIENT_HANDLE_DATA_TAG* HTTP_CLIENT_HANDLE;

#define HTTP_CLIENT_RESULT_VALUES       \
    HTTP_CLIENT_OK,                     \
    HTTP_CLIENT_INVALID_ARG,            \
    HTTP_CLIENT_ERROR,                  \
    HTTP_CLIENT_OPEN_FAILED,            \
    HTTP_CLIENT_SEND_FAILED,            \
    HTTP_CLIENT_ALREADY_INIT,           \
    HTTP_CLIENT_HTTP_HEADERS_FAILED

/** @brief Enumeration specifying the possible return values for the APIs in
*		   this module.
*/
DEFINE_ENUM(HTTP_CLIENT_RESULT, HTTP_CLIENT_RESULT_VALUES);

#define HTTP_CLIENT_REQUEST_TYPE_VALUES \
    HTTP_CLIENT_REQUEST_OPTIONS,        \
    HTTP_CLIENT_REQUEST_GET,            \
    HTTP_CLIENT_REQUEST_POST,           \
    HTTP_CLIENT_REQUEST_PUT,            \
    HTTP_CLIENT_REQUEST_DELETE,         \
    HTTP_CLIENT_REQUEST_PATCH

DEFINE_ENUM(HTTP_CLIENT_REQUEST_TYPE, HTTP_CLIENT_REQUEST_TYPE_VALUES);

#define HTTP_CALLBACK_REASON_VALUES     \
    HTTP_CALLBACK_REASON_OK,            \
    HTTP_CALLBACK_REASON_DESTROY,       \
    HTTP_CALLBACK_REASON_DISCONNECTED

DEFINE_ENUM(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_VALUES);

typedef void(*ON_HTTP_OPEN_COMPLETE_CALLBACK)(void* callback_ctx, HTTP_CALLBACK_REASON open_result);
typedef void(*ON_HTTP_ERROR_CALLBACK)(void* callback_ctx, HTTP_CALLBACK_REASON error_result);
typedef void(*ON_HTTP_REQUEST_CALLBACK)(void* callback_ctx, HTTP_CALLBACK_REASON request_result, const unsigned char* content, size_t content_length, unsigned int status_code,
    HTTP_HEADERS_HANDLE response_headers);
typedef void(*ON_HTTP_CLOSED_CALLBACK)(void* callback_ctx);

MOCKABLE_FUNCTION(, HTTP_CLIENT_HANDLE, uhttp_client_create, const IO_INTERFACE_DESCRIPTION*, io_interface_desc, const void*, xio_param, ON_HTTP_ERROR_CALLBACK, on_http_error, void*, callback_ctx);

MOCKABLE_FUNCTION(, void, uhttp_client_destroy, HTTP_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, HTTP_CLIENT_RESULT, uhttp_client_open, HTTP_CLIENT_HANDLE, handle, const char*, host, int, port_num, ON_HTTP_OPEN_COMPLETE_CALLBACK, on_connect, void*, callback_ctx);

MOCKABLE_FUNCTION(, void, uhttp_client_close, HTTP_CLIENT_HANDLE, handle, ON_HTTP_CLOSED_CALLBACK, on_close_callback, void*, callback_ctx);

MOCKABLE_FUNCTION(, HTTP_CLIENT_RESULT, uhttp_client_execute_request, HTTP_CLIENT_HANDLE, handle, HTTP_CLIENT_REQUEST_TYPE, request_type, const char*, relative_path,
    HTTP_HEADERS_HANDLE, http_header_handle, const unsigned char*, content, size_t, content_length, ON_HTTP_REQUEST_CALLBACK, on_request_callback, void*, callback_ctx);

MOCKABLE_FUNCTION(, void, uhttp_client_dowork, HTTP_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, HTTP_CLIENT_RESULT, uhttp_client_set_trace, HTTP_CLIENT_HANDLE, handle, bool, trace_on);
```

### uhttp_client_create

```c
HTTP_CLIENT_HANDLE uhttp_client_create(const IO_INTERFACE_DESCRIPTION* io_interface_desc, const void* xio_param, ON_HTTP_ERROR_CALLBACK on_http_error, void* callback_ctx)
```

http_client_create initializes the http client object.

**SRS_UHTTP_07_002: [**If io_interface_desc is NULL, `uhttp_client_create` shall return NULL.**]**

**SRS_UHTTP_07_001: [**`uhttp_client_create` shall return and initialize the http client handle.**]**

**SRS_UHTTP_07_003: [**If `uhttp_client_create` encounters any error then it shall return NULL**]**

### uhttp_client_destroy

```c
void uhttp_client_destroy(HTTP_CLIENT_HANDLE handle);
```

uhttp_client_destroy cleans up all items that have been allocated.

**SRS_UHTTP_07_004: [** If `handle` is NULL then `uhttp_client_destroy` shall do nothing **]**

**SRS_UHTTP_07_005: [** `http_client_destroy` shall free any resource that is associated with `handle`. **]**

### uhttp_client_open

```c
HTTP_CLIENT_RESULT uhttp_client_open(HTTP_CLIENT_HANDLE handle, const char* host, int port_num, ON_HTTP_OPEN_COMPLETE_CALLBACK on_connect, void* callback_ctx)
```

uhttp_client_open opens the xio_handle calling the on_open callback if supplied.

**SRS_UHTTP_07_006: [**If handle or host is NULL then `uhttp_client_open` shall return HTTP_CLIENT_INVALID_ARG**]**

**SRS_UHTTP_07_007: [**`uhttp_client_open` shall call xio_open on the xio_handle.**]**

**SRS_UHTTP_07_044: [** if a failure is encountered on xio_open `uhttp_client_open` shall return `HTTP_CLIENT_OPEN_REQUEST_FAILED`. **]**

**SRS_UHTTP_07_008: [**If `uhttp_client_open` succeeds then it shall return HTTP_CLIENT_OK**]**

### uhttp_client_close

```c
void uhttp_client_close(HTTP_CLIENT_HANDLE handle, ON_HTTP_CLOSED_CALLBACK on_close_callback, void* callback_ctx)
```

uhttp_client_close closes the xioHandle connection.

**SRS_UHTTP_07_009: [**If handle is NULL then `uhttp_client_close` shall do nothing**]**

**SRS_UHTTP_07_010: [**If the xio_handle is NOT NULL `uhttp_client_close` shall call xio_close**]**

**SRS_UHTTP_07_049: [** If the state has been previously set to `state_closed`, `uhttp_client_close` shall do nothing. **]**

### uhttp_client_execute_request

```c
HTTP_CLIENT_RESULT uhttp_client_execute_request(HTTP_CLIENT_HANDLE handle, HTTP_CLIENT_REQUEST_TYPE request_type, const char* relative_path,
    HTTP_HEADERS_HANDLE http_header_handle, const unsigned char* content, size_t content_len, ON_HTTP_REPLY_RECV_CALLBACK on_http_reply_recv, void* callback_ctx)
```

uhttp_client_execute_request allocates data to be sent to the http endpoint.

**SRS_UHTTP_07_012: [**If `handle`, or `on_http_reply_recv` is NULL then `uhttp_client_execute_request` shall return HTTP_CLIENT_INVALID_ARG.**]**

**SRS_UHTTP_07_013: [**if content is not NULL and `contentLength` is 0 or content is NULL and contentLength is not 0 then `uhttp_client_execute_request` shall return HTTP_CLIENT_INVALID_ARG.**]**

**SRS_UHTTP_07_041: [**HTTP_CLIENT_REQUEST_TYPE shall support all request types specified under section 9.1.2 in the spec.**]**

**SRS_UHTTP_07_016: [**`uhttp_client_execute_request` shall queue the http headers data and content to be sent to the http endpoint.**]**

**SRS_UHTTP_07_015: [**`uhttp_client_execute_request` shall add the Content-Length to the request if the contentLength is > 0.**]**

**SRS_UHTTP_07_011: [**`uhttp_client_execute_request` shall add the HOST http header item to the request if not supplied (RFC 7230 - 5.4).**]**

**SRS_UHTTP_07_018: [**upon success `uhttp_client_execute_request` shall return HTTP_CLIENT_OK.**]**

**SRS_UHTTP_07_017: [**If any failure is encountered `uhttp_client_execute_request` shall return HTTP_CLIENT_ERROR.**]**

### uhttp_client_dowork

```c
void uhttp_client_dowork(HTTP_CLIENT_HANDLE handle);
```

uhttp_client_dowork executes the http work that includes sends and receives.

**SRS_UHTTP_07_036: [**If handle is NULL then `uhttp_client_dowork` shall do nothing.**]**

**SRS_UHTTP_07_037: [**`uhttp_client_dowork` shall call the underlying xio_dowork function. **]**

**SRS_UHTTP_07_016: [**`uhttp_client_dowork` shall iterate through the queued Data using the xio interface to send the http request in the following ways...**]**

**SRS_UHTTP_07_052: [**`uhttp_client_dowork` shall call xio_send to transmits the header information... **]**

**SRS_UHTTP_07_053: [** Then `uhttp_client_dowork` shall use xio_send to transmit the content of the http request if supplied. **]**

**SRS_UHTTP_07_046: [** `uhttp_client_dowork` shall free resouces queued to send to the http endpoint. **]**

### http_client_set_trace

```c
HTTP_CLIENT_RESULT uhttp_client_set_trace(HTTP_CLIENT_HANDLE handle, bool trace_on)
```

http_client_set_trace turns on or off log tracing.

**SRS_UHTTP_07_038: [**If handle is NULL then `http_client_set_trace` shall return HTTP_CLIENT_INVALID_ARG**]**

**SRS_UHTTP_07_039: [**`http_client_set_trace` shall set the HTTP tracing to the trace_on variable.**]**

**SRS_UHTTP_07_040: [**if `http_client_set_trace` finishes successfully then it shall return HTTP_CLIENT_OK.**]**

### on_bytes_received

```c
static void on_bytes_received(void* context, const unsigned char* buffer, size_t len)
```

**SRS_UHTTP_07_047: [** If context or buffer is NULL `on_bytes_received` shall do nothing. **]**

**SRS_UHTTP_07_048: [** If any error is encountered `on_bytes_received` shall set the stop processing the request. **]**

### Chunk Response

**SRS_UHTTP_07_054: [** If the http header does not include a content length then it indicates a chunk response. **]**

**SRS_UHTTP_07_055: [** `on_bytes_received` shall convert the hexs length supplied in the response to the data length of the chunked data. **]**

**SRS_UHTTP_07_056: [** After the response chunk is parsed it shall be placed in a `BUFFER_HANDLE`. **]**

**SRS_UHTTP_07_057: [** Once the response is stored `on_bytes_received` shall free the bytes that are stored and shrink the stored bytes buffer. **]**

**SRS_UHTTP_07_058: [** Once a chunk size value of 0 is encountered `on_bytes_received` shall call the on_request_callback with the http message **]**

**SRS_UHTTP_07_059: [** `on_bytes_received` shall loop throught the stored data to find the /r/n separator. **]**

**SRS_UHTTP_07_060: [** if the `data_length` specified in the chunk is beyond the amound of data recieved, the parsing shall end and wait for more data. **]**

### on_xio_close_complete

```c
static void on_xio_close_complete(void* context)
```

**SRS_UHTTP_07_045: [** If `on_close_callback` is not NULL, `on_close_callback` shall be called once the underlying xio is closed. **]**

### on_xio_open_complete

```c
static void on_xio_open_complete(void* context, IO_OPEN_RESULT open_result)
```

**SRS_UHTTP_07_049: [** If not NULL `uhttp_client_open` shall call the `on_connect` callback with the `callback_ctx`, once the underlying xio's open is complete. **]**

**SRS_UHTTP_07_042: [** If the underlying XIO object opens successfully the `on_connect` callback shall be call with HTTP_CLIENT_OK... **]**

**SRS_UHTTP_07_043: [** Otherwise `on_connect` callback shall be call with `HTTP_CLIENT_OPEN_REQUEST_FAILED`. **]**

### on_io_error

```c
static void on_io_error(void* context)
```

**SRS_UHTTP_07_050: [** if `context` is NULL `on_io_error` shall do nothing. **]**

**SRS_UHTTP_07_051: [** if on_error callback is not NULL, `on_io_error` shall call on_error callback. **]**