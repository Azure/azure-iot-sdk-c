# dps_transport_amqp_ws_client Requirements

================================

## Overview

dps_transport_amqp_ws_client module implements the DPS amqp transport interface

## Dependencies

dps_transport_amqp_ws_common module

## Exposed API

```c
const DPS_TRANSPORT_PROVIDER* DPS_AMQP_WS_Protocol(void);

static DPS_TRANSPORT_PROVIDER dps_amqp_ws_func =
{
    dps_transport_amqp_ws_create,
    dps_transport_amqp_ws_destroy,
    dps_transport_amqp_ws_open,
    dps_transport_amqp_ws_close,
    dps_transport_amqp_ws_register_device,
    dps_transport_amqp_ws_get_operation_status,
    dps_transport_amqp_ws_dowork,
    dps_transport_amqp_ws_set_trace,
    dps_transport_amqp_ws_x509_cert,
    dps_transport_amqp_ws_set_trusted_cert,
    dps_transport_amqp_ws_set_proxy
};
```

### dps_transport_amqp_ws_create

Creates the DPS_TRANSPORT_HANDLE using the specified interface parameters

```c
DPS_TRANSPORT_HANDLE dps_transport_amqp_ws_create(const char* uri, DPS_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* dps_api_version)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_001: [** `dps_transport_amqp_ws_create` shall call the `dps_trans_common_amqp_create` function with `amqp_transport_ws_io` transport IO estabishment. **]**

### dps_transport_amqp_ws_destroy

Frees any resources created by the dps_transport_http module

```c
void dps_transport_amqp_ws_destroy(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_002: [** `dps_transport_amqp_ws_destroy` shall invoke the `dps_trans_common_amqp_destroy` method **]**

### dps_transport_amqp_ws_open

Opens the amqp transport for future communications

```c
int dps_transport_amqp_ws_open(DPS_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_003: [** `dps_transport_amqp_ws_open` shall invoke the `dps_trans_common_amqp_open` method **]**

### dps_transport_amqp_ws_close

Closes the amqp communication

```c
int dps_transport_amqp_ws_close(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_004: [** `dps_transport_amqp_ws_close` shall invoke the `dps_trans_common_amqp_close` method **]**

### dps_transport_amqp_ws_register_device

Begins the registration process for the device

```c
int dps_transport_amqp_ws_register_device(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_005: [** `dps_transport_amqp_ws_register_device` shall invoke the `dps_trans_common_amqp_register_device` method **]**

### dps_transport_amqp_ws_get_operation_status

Execute a get operation status for the DPS endpoint

```c
int dps_transport_amqp_ws_get_operation_status(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_006: [** `dps_transport_amqp_ws_get_operation_status` shall invoke the `dps_trans_common_amqp_get_operation_status` method **]**

### dps_transport_amqp_ws_dowork

```c
void dps_transport_amqp_ws_dowork(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_007: [** `dps_transport_amqp_ws_dowork` shall invoke the `dps_trans_common_amqp_dowork` method **]**

### dps_transport_amqp_ws_set_trace

```c
int dps_transport_amqp_ws_set_trace(DPS_TRANSPORT_HANDLE handle, bool trace_on)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_008: [** `dps_transport_amqp_ws_set_trace` shall invoke the `dps_trans_common_amqp_set_trace` method **]**

### dps_transport_amqp_ws_x509_cert

```c
static int dps_transport_amqp_ws_x509_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_009: [** `dps_transport_amqp_ws_x509_cert` shall invoke the `dps_trans_common_amqp_x509_cert` method **]**

### dps_transport_amqp_ws_set_trusted_cert

```c
static int dps_transport_amqp_ws_set_trusted_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_010: [** `dps_transport_amqp_ws_set_trusted_cert` shall invoke the `dps_trans_common_amqp_set_trusted_cert` method **]**

### dps_transport_amqp_ws_set_proxy

```c
static int dps_transport_amqp_ws_set_proxy(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_011: [** `dps_transport_amqp_ws_set_proxy` shall invoke the `dps_trans_common_amqp_set_proxy` method **]**

### amqp_transport_ws_io

```c
static XIO_HANDLE amqp_transport_ws_io(const char* fqdn, SASL_MECHANISM_HANDLE* sasl_mechanism, const HTTP_PROXY_OPTIONS* proxy_info)
```

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_012: [** If `proxy_info` is not NULL, `amqp_transport_ws_io` shall construct a `HTTP_PROXY_IO_CONFIG` object and assign it to `TLSIO_CONFIG` `underlying_io_parameters` **]**

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_013: [** If any failure is encountered `amqp_transport_ws_io` shall return NULL **]**

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_014: [** On success `amqp_transport_ws_io` shall return an allocated `XIO_HANDLE`. **]**

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_015: [** `amqp_transport_ws_io` shall allocate a `transfer_handle` by calling xio_create with the `ws_io_interface`. **]**

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_016: [** `amqp_transport_ws_io` shall allocate a `sasl_handle` by calling xio_create with the `saslio_interface`. **]**

**DPS_TRANSPORT_AMQP_WS_CLIENT_07_017: [** If `sasl_mechanism` is NULL `amqp_transport_ws_io` shall assign the `transfer_handle` to the result and return the value. **]**
