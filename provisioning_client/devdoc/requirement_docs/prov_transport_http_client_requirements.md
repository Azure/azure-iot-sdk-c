# prov_transport_http_client Requirements

================================

## Overview

prov_transport_http_client module implements the DPS http transport interface

## Exposed API

```c
    struct PROV_TRANSPORT_PROVIDER_TAG;
    typedef struct PROV_TRANSPORT_PROVIDER_TAG PROV_TRANSPORT_PROVIDER;

    typedef void* PROV_TRANSPORT_HANDLE;

#define PROV_TRANSPORT_RESULT_VALUES     \
    PROV_TRANSPORT_RESULT_OK,            \
    PROV_TRANSPORT_RESULT_UNAUTHORIZED,  \
    PROV_TRANSPORT_RESULT_ERROR          \

    DEFINE_ENUM(PROV_TRANSPORT_RESULT, PROV_TRANSPORT_RESULT_VALUES);

#define PROV_TRANSPORT_STATUS_VALUES     \
    PROV_TRANSPORT_STATUS_CONNECTED,     \
    PROV_TRANSPORT_STATUS_ERROR

    DEFINE_ENUM(PROV_TRANSPORT_STATUS, PROV_TRANSPORT_STATUS_VALUES);

    typedef void(*PROV_TRANSPORT_REGISTER_DATA_CALLBACK)(PROV_TRANSPORT_RESULT transport_result, const char* data, void* user_ctx);
    typedef void(*PROV_TRANSPORT_STATUS_CALLBACK)(PROV_TRANSPORT_STATUS transport_status, void* user_ctx);

    typedef PROV_TRANSPORT_HANDLE(*pfprov_transport_create)(const char* uri, const char* scope_id, const char* registration_id, const char* prov_api_version);
    typedef void(*pfprov_transport_destroy)(PROV_TRANSPORT_HANDLE handle);
    typedef int(*pfprov_transport_open)(PROV_TRANSPORT_HANDLE handle, PROV_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, PROV_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx);
    typedef int(*pfprov_transport_close)(PROV_TRANSPORT_HANDLE handle);

    typedef int(*pfprov_transport_register_device)(PROV_TRANSPORT_HANDLE handle, const char* json_data);
    typedef int(*pfprov_transport_challenge_response)(PROV_TRANSPORT_HANDLE handle, const char* json_data, const char* sas_token);
    typedef int(*pfprov_transport_get_operation_status)(PROV_TRANSPORT_HANDLE handle, const char* operation_id);
    typedef void(*pfprov_transport_dowork)(PROV_TRANSPORT_HANDLE handle);
    typedef int(*pfprov_transport_set_trace)(PROV_TRANSPORT_HANDLE handle, bool trace_on);
    typedef int(*pfprov_transport_set_x509_cert)(PROV_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key);
    typedef int(*pfprov_transport_set_trusted_cert)(PROV_TRANSPORT_HANDLE handle, const char* certificate);
    typedef int(*pfprov_transport_set_proxy)(PROV_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_option);

    const PROV_TRANSPORT_PROVIDER* PROV_HTTP_Protocol(void)
```

### prov_transport_http_create

Creates the PROV_TRANSPORT_HANDLE using the specified interface parameters

```c
PROV_TRANSPORT_HANDLE prov_transport_http_create(const char* uri, const char* scope_id, const char* registration_id, const char* prov_api_version)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_001: [** If `uri`, `scope_id`, `registration_id` or `prov_api_version` are NULL `prov_transport_http_create` shall return NULL. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_002: [** `prov_transport_http_create` shall allocate the memory for the `PROV_TRANSPORT_HANDLE` variables. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_003: [** If any error is encountered `prov_transport_http_create` shall return NULL. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_004: [** On success `prov_transport_http_create` shall return a PROV_TRANSPORT_HANDLE. **]**

### prov_transport_http_destroy

Frees any resources created by the prov_transport_http module

```c
void prov_transport_http_destroy(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_005: [** `prov_transport_http_destroy` shall free all resources associated with the `PROV_TRANSPORT_HANDLE` handle **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_007: [** If the argument handle is NULL, `prov_transport_http_destroy` shall do nothing **]**

### prov_transport_http_open

Opens the http transport for future communications

```c
int prov_transport_http_open(PROV_TRANSPORT_HANDLE handle, PROV_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, PROV_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_008: [** If the argument `handle` or `data_callback` is NULL, `prov_transport_http_open` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_009: [** `prov_transport_http_open` shall create the http client adding any proxy and certificate information that is presented. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_010: [** `prov_transport_http_open` shall opening the http communications with the DPS service. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_006: [** `prov_transport_http_close` shall call the `uhttp_client_destroy` function function associated with the http_client **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_011: [** If any error is encountered `prov_transport_http_open` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_012: [** If successful `prov_transport_http_open` shall return 0. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_013: [** If an error is encountered `prov_transport_http_open` shall return a non-zero value. **]**

### prov_transport_http_close

Closes the http communication

```c
int prov_transport_http_close(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_014: [** If the argument `handle` is NULL, `prov_transport_http_close` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_015: [** `prov_transport_http_close` shall attempt to close the http communication with the DPS service. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_016: [** On success `prov_transport_http_close` shall return 0. **]**

### prov_transport_http_register_device

Begins the registration process for the device

```c
int prov_transport_http_register_device(PROV_TRANSPORT_HANDLE handle, const char* json_data)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_017: [** If the argument `handle` or `json_data` is NULL, `prov_transport_http_register_device` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_018: [** `prov_transport_http_register_device` shall construct the http headers for anonymous communication to the DPS service. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_019: [** `prov_transport_http_register_device` shall construct the path to the DPS service in the following format: /<scope_id>/registrations/<url_encoded_registration_id>/register-me?api-version=<api_version> **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_020: [** `prov_transport_http_register_device` shall send the request using the http client. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_021: [** If any error is encountered `prov_transport_http_register_device` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_022: [** On success `prov_transport_http_register_device` shall return 0. **]**

### prov_transport_http_challenge_response

Execute the http challenge response for the DPS endpoint

```c
int prov_transport_http_challenge_response(PROV_TRANSPORT_HANDLE handle, const char* json_data, const char* sas_token)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_023: [** If the argument `handle`, `json_data` or `sas_token` is NULL, `prov_transport_http_challenge_response` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_024: [** `prov_transport_http_challenge_response` shall construct the http headers with SAS_TOKEN as `Authorization` header. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_025: [** `prov_transport_http_challenge_response` shall construct the path to the DPS service in the following format: /<scope_id>/registrations/<url_encoded_registration_id>/register-me?api-version=<api_version> **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_026: [** `prov_transport_http_challenge_response` shall send the request using the http client. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_027: [** If any error is encountered `prov_transport_http_challenge_response` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_028: [** On success `prov_transport_http_challenge_response` shall return 0. **]**

### prov_transport_http_get_operation_status

Execute a get operation status for the DPS endpoint

```c
int prov_transport_http_get_operation_status(PROV_TRANSPORT_HANDLE handle, const char* operation_id)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_029: [** If the argument `handle`, or `operation_id` is NULL, `prov_transport_http_get_operation_status` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_030: [** `prov_transport_http_get_operation_status` shall construct the http headers with SAS_TOKEN as Authorization header. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_031: [** `prov_transport_http_get_operation_status` shall construct the path to the DPS service in the following format: /<scope_id>/registrations/<url_encoded_registration_id>/operations<url_encoded_operation_id>?api-version=<api_version> **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_032: [** `prov_transport_http_get_operation_status` shall send the request using the http client. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_033: [** If any error is encountered `prov_transport_http_get_operation_status` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_034: [** On success `prov_transport_http_get_operation_status` shall return 0. **]**

### prov_transport_http_dowork

```c
void prov_transport_http_dowork(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_035: [** If the argument `handle`,  is NULL, `prov_transport_http_dowork` shall do nothing. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_036: [** `prov_transport_http_dowork` shall call the underlying http client do work method. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_037: [** If the `prov_transport_http_dowork` state is Message received `prov_transport_http_dowork` shall call the calling process registration_data callback **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_038: [** `prov_transport_http_dowork` shall free the payload_data **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_039: [** If the state is Error, `prov_transport_http_dowork` shall call the registration_data callback with `PROV_TRANSPORT_RESULT_ERROR` and NULL payload_data. **]**

### prov_transport_http_set_trace

```c
int prov_transport_http_set_trace(PROV_TRANSPORT_HANDLE handle, bool trace_on)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_040: [** If the argument `handle`,  is NULL, `prov_transport_http_set_trace` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_041: [** If the http client is not NULL, `prov_transport_http_set_trace` shall set the http client log trace function with the specified trace_on flag. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_042: [** On success `prov_transport_http_set_trace` shall return zero. **]**


### prov_transport_http_x509_cert

```c
static int prov_transport_http_x509_cert(PROV_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_043: [** If the argument `handle`, `private_key` or `certificate` is NULL, `prov_transport_http_x509_cert` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_044: [** If any error is encountered `prov_transport_http_x509_cert` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_045: [** `prov_transport_http_x509_cert` shall store the certificate and private_key for use when the http client connects. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_046: [** On success `prov_transport_http_set_trace` shall return zero. **]**

### prov_transport_http_set_trusted_cert

```c
static int prov_transport_http_set_trusted_cert(PROV_TRANSPORT_HANDLE handle, const char* certificate)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_047: [** If the argument `handle`, `certificate` is NULL, `prov_transport_http_set_trusted_cert` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_048: [** If any error is encountered `prov_transport_http_set_trusted_cert` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_049: [** `prov_transport_http_set_trusted_cert` shall store the `certificate` for use when the http client connects. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_050: [** On success `prov_transport_http_set_trusted_cert` shall return zero. **]**

### prov_transport_http_set_proxy

```c
static int prov_transport_http_set_proxy(PROV_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
```

**PROV_TRANSPORT_HTTP_CLIENT_07_051: [** If the argument `handle` or `proxy_options` is NULL, `prov_transport_http_set_proxy` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_052: [** If any error is encountered `prov_transport_http_set_proxy` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_055: [** If `proxy_options host_name` is NULL `prov_transport_http_set_proxy` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_053: [** If `proxy_options username` is NULL and `proxy_options password` is not NULL `prov_transport_http_set_proxy` shall return a non-zero value. **]**

**PROV_TRANSPORT_HTTP_CLIENT_07_054: [** On success `prov_transport_http_set_proxy` shall return zero. **]**
