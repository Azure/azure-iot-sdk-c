# dps_transport_http_client Requirements

================================

## Overview

dps_transport_http_client module implements the DPS http transport interface

## Exposed API

```c
    struct DPS_TRANSPORT_PROVIDER_TAG;
    typedef struct DPS_TRANSPORT_PROVIDER_TAG DPS_TRANSPORT_PROVIDER;

    typedef void* DPS_TRANSPORT_HANDLE;

#define DPS_TRANSPORT_RESULT_VALUES     \
    DPS_TRANSPORT_RESULT_OK,            \
    DPS_TRANSPORT_RESULT_UNAUTHORIZED,  \
    DPS_TRANSPORT_RESULT_ERROR          \

    DEFINE_ENUM(DPS_TRANSPORT_RESULT, DPS_TRANSPORT_RESULT_VALUES);

#define DPS_TRANSPORT_STATUS_VALUES     \
    DPS_TRANSPORT_STATUS_CONNECTED,     \
    DPS_TRANSPORT_STATUS_ERROR

    DEFINE_ENUM(DPS_TRANSPORT_STATUS, DPS_TRANSPORT_STATUS_VALUES);

    typedef void(*DPS_TRANSPORT_REGISTER_DATA_CALLBACK)(DPS_TRANSPORT_RESULT transport_result, const char* data, void* user_ctx);
    typedef void(*DPS_TRANSPORT_STATUS_CALLBACK)(DPS_TRANSPORT_STATUS transport_status, void* user_ctx);

    typedef DPS_TRANSPORT_HANDLE(*pfdps_transport_create)(const char* uri, const char* scope_id, const char* registration_id, const char* dps_api_version);
    typedef void(*pfdps_transport_destroy)(DPS_TRANSPORT_HANDLE handle);
    typedef int(*pfdps_transport_open)(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx);
    typedef int(*pfdps_transport_close)(DPS_TRANSPORT_HANDLE handle);

    typedef int(*pfdps_transport_register_device)(DPS_TRANSPORT_HANDLE handle, const char* json_data);
    typedef int(*pfdps_transport_challenge_response)(DPS_TRANSPORT_HANDLE handle, const char* json_data, const char* sas_token);
    typedef int(*pfdps_transport_get_operation_status)(DPS_TRANSPORT_HANDLE handle, const char* operation_id);
    typedef void(*pfdps_transport_dowork)(DPS_TRANSPORT_HANDLE handle);
    typedef int(*pfdps_transport_set_trace)(DPS_TRANSPORT_HANDLE handle, bool trace_on);
    typedef int(*pfdps_transport_set_x509_cert)(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key);
    typedef int(*pfdps_transport_set_trusted_cert)(DPS_TRANSPORT_HANDLE handle, const char* certificate);
    typedef int(*pfdps_transport_set_proxy)(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_option);

    const DPS_TRANSPORT_PROVIDER* DPS_HTTP_Protocol(void)
```

### dps_transport_http_create

Creates the DPS_TRANSPORT_HANDLE using the specified interface parameters

```c
DPS_TRANSPORT_HANDLE dps_transport_http_create(const char* uri, const char* scope_id, const char* registration_id, const char* dps_api_version)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_001: [** If `uri`, `scope_id`, `registration_id` or `dps_api_version` are NULL `dps_transport_http_create` shall return NULL. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_002: [** `dps_transport_http_create` shall allocate the memory for the `DPS_TRANSPORT_HANDLE` variables. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_003: [** If any error is encountered `dps_transport_http_create` shall return NULL. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_004: [** On success `dps_transport_http_create` shall return a DPS_TRANSPORT_HANDLE. **]**

### dps_transport_http_destroy

Frees any resources created by the dps_transport_http module

```c
void dps_transport_http_destroy(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_005: [** `dps_transport_http_destroy` shall free all resources associated with the `DPS_TRANSPORT_HANDLE` handle **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_007: [** If the argument handle is NULL, `dps_transport_http_destroy` shall do nothing **]**

### dps_transport_http_open

Opens the http transport for future communications

```c
int dps_transport_http_open(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_008: [** If the argument `handle` or `data_callback` is NULL, `dps_transport_http_open` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_009: [** `dps_transport_http_open` shall create the http client adding any proxy and certificate information that is presented. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_010: [** `dps_transport_http_open` shall opening the http communications with the DPS service. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_006: [** `dps_transport_http_close` shall call the `uhttp_client_destroy` function function associated with the http_client **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_011: [** If any error is encountered `dps_transport_http_open` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_012: [** If successful `dps_transport_http_open` shall return 0. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_013: [** If an error is encountered `dps_transport_http_open` shall return a non-zero value. **]**

### dps_transport_http_close

Closes the http communication

```c
int dps_transport_http_close(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_014: [** If the argument `handle` is NULL, `dps_transport_http_close` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_015: [** `dps_transport_http_close` shall attempt to close the http communication with the DPS service. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_016: [** On success `dps_transport_http_close` shall return 0. **]**

### dps_transport_http_register_device

Begins the registration process for the device

```c
int dps_transport_http_register_device(DPS_TRANSPORT_HANDLE handle, const char* json_data)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_017: [** If the argument `handle` or `json_data` is NULL, `dps_transport_http_register_device` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_018: [** `dps_transport_http_register_device` shall construct the http headers for anonymous communication to the DPS service. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_019: [** `dps_transport_http_register_device` shall construct the path to the DPS service in the following format: /<scope_id>/registrations/<url_encoded_registration_id>/register-me?api-version=<api_version> **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_020: [** `dps_transport_http_register_device` shall send the request using the http client. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_021: [** If any error is encountered `dps_transport_http_register_device` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_022: [** On success `dps_transport_http_register_device` shall return 0. **]**

### dps_transport_http_challenge_response

Execute the http challenge response for the DPS endpoint

```c
int dps_transport_http_challenge_response(DPS_TRANSPORT_HANDLE handle, const char* json_data, const char* sas_token)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_023: [** If the argument `handle`, `json_data` or `sas_token` is NULL, `dps_transport_http_challenge_response` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_024: [** `dps_transport_http_challenge_response` shall construct the http headers with SAS_TOKEN as `Authorization` header. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_025: [** `dps_transport_http_challenge_response` shall construct the path to the DPS service in the following format: /<scope_id>/registrations/<url_encoded_registration_id>/register-me?api-version=<api_version> **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_026: [** `dps_transport_http_challenge_response` shall send the request using the http client. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_027: [** If any error is encountered `dps_transport_http_challenge_response` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_028: [** On success `dps_transport_http_challenge_response` shall return 0. **]**

### dps_transport_http_get_operation_status

Execute a get operation status for the DPS endpoint

```c
int dps_transport_http_get_operation_status(DPS_TRANSPORT_HANDLE handle, const char* operation_id)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_029: [** If the argument `handle`, or `operation_id` is NULL, `dps_transport_http_get_operation_status` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_030: [** `dps_transport_http_get_operation_status` shall construct the http headers with SAS_TOKEN as Authorization header. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_031: [** `dps_transport_http_get_operation_status` shall construct the path to the DPS service in the following format: /<scope_id>/registrations/<url_encoded_registration_id>/operations<url_encoded_operation_id>?api-version=<api_version> **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_032: [** `dps_transport_http_get_operation_status` shall send the request using the http client. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_033: [** If any error is encountered `dps_transport_http_get_operation_status` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_034: [** On success `dps_transport_http_get_operation_status` shall return 0. **]**

### dps_transport_http_dowork

```c
void dps_transport_http_dowork(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_035: [** If the argument `handle`,  is NULL, `dps_transport_http_dowork` shall do nothing. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_036: [** `dps_transport_http_dowork` shall call the underlying http client do work method. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_037: [** If the `dps_transport_http_dowork` state is Message received `dps_transport_http_dowork` shall call the calling process registration_data callback **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_038: [** `dps_transport_http_dowork` shall free the payload_data **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_039: [** If the state is Error, `dps_transport_http_dowork` shall call the registration_data callback with `DPS_TRANSPORT_RESULT_ERROR` and NULL payload_data. **]**

### dps_transport_http_set_trace

```c
int dps_transport_http_set_trace(DPS_TRANSPORT_HANDLE handle, bool trace_on)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_040: [** If the argument `handle`,  is NULL, `dps_transport_http_set_trace` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_041: [** If the http client is not NULL, `dps_transport_http_set_trace` shall set the http client log trace function with the specified trace_on flag. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_042: [** On success `dps_transport_http_set_trace` shall return zero. **]**


### dps_transport_http_x509_cert

```c
static int dps_transport_http_x509_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_043: [** If the argument `handle`, `private_key` or `certificate` is NULL, `dps_transport_http_x509_cert` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_044: [** If any error is encountered `dps_transport_http_x509_cert` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_045: [** `dps_transport_http_x509_cert` shall store the certificate and private_key for use when the http client connects. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_046: [** On success `dps_transport_http_set_trace` shall return zero. **]**

### dps_transport_http_set_trusted_cert

```c
static int dps_transport_http_set_trusted_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_047: [** If the argument `handle`, `certificate` is NULL, `dps_transport_http_set_trusted_cert` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_048: [** If any error is encountered `dps_transport_http_set_trusted_cert` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_049: [** `dps_transport_http_set_trusted_cert` shall store the `certificate` for use when the http client connects. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_050: [** On success `dps_transport_http_set_trusted_cert` shall return zero. **]**

### dps_transport_http_set_proxy

```c
static int dps_transport_http_set_proxy(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
```

**DPS_TRANSPORT_HTTP_CLIENT_07_051: [** If the argument `handle` or `proxy_options` is NULL, `dps_transport_http_set_proxy` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_052: [** If any error is encountered `dps_transport_http_set_proxy` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_055: [** If `proxy_options host_name` is NULL `dps_transport_http_set_proxy` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_053: [** If `proxy_options username` is NULL and `proxy_options password` is not NULL `dps_transport_http_set_proxy` shall return a non-zero value. **]**

**DPS_TRANSPORT_HTTP_CLIENT_07_054: [** On success `dps_transport_http_set_proxy` shall return zero. **]**
