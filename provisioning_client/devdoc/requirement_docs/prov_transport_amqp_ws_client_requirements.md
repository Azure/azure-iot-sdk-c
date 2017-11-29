# prov_transport_amqp_ws_client Requirements

================================

## Overview

prov_transport_amqp_ws_client module implements the DPS amqp transport interface

## Dependencies

prov_transport_amqp_ws_common module

## Exposed API

```c
const PROV_TRANSPORT_PROVIDER* PROV_AMQP_WS_Protocol(void);

static PROV_TRANSPORT_PROVIDER prov_amqp_ws_func =
{
    prov_transport_amqp_ws_create,
    prov_transport_amqp_ws_destroy,
    prov_transport_amqp_ws_open,
    prov_transport_amqp_ws_close,
    prov_transport_amqp_ws_register_device,
    prov_transport_amqp_ws_get_operation_status,
    prov_transport_amqp_ws_dowork,
    prov_transport_amqp_ws_set_trace,
    prov_transport_amqp_ws_x509_cert,
    prov_transport_amqp_ws_set_trusted_cert,
    prov_transport_amqp_ws_set_proxy
};
```

### prov_transport_amqp_ws_create

Creates the PROV_TRANSPORT_HANDLE using the specified interface parameters

```c
PROV_TRANSPORT_HANDLE prov_transport_amqp_ws_create(const char* uri, PROV_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* prov_api_version)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_001: [** `prov_transport_amqp_ws_create` shall call the `prov_trans_common_amqp_create` function with `amqp_transport_ws_io` transport IO estabishment. **]**

### prov_transport_amqp_ws_destroy

Frees any resources created by the prov_transport_http module

```c
void prov_transport_amqp_ws_destroy(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_002: [** `prov_transport_amqp_ws_destroy` shall invoke the `prov_trans_common_amqp_destroy` method **]**

### prov_transport_amqp_ws_open

Opens the amqp transport for future communications

```c
int prov_transport_amqp_ws_open(PROV_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, PROV_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, PROV_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_003: [** `prov_transport_amqp_ws_open` shall invoke the `prov_trans_common_amqp_open` method **]**

### prov_transport_amqp_ws_close

Closes the amqp communication

```c
int prov_transport_amqp_ws_close(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_004: [** `prov_transport_amqp_ws_close` shall invoke the `prov_trans_common_amqp_close` method **]**

### prov_transport_amqp_ws_register_device

Begins the registration process for the device

```c
int prov_transport_amqp_ws_register_device(PROV_TRANSPORT_HANDLE handle, PROV_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_005: [** `prov_transport_amqp_ws_register_device` shall invoke the `prov_trans_common_amqp_register_device` method **]**

### prov_transport_amqp_ws_get_operation_status

Execute a get operation status for the DPS endpoint

```c
int prov_transport_amqp_ws_get_operation_status(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_006: [** `prov_transport_amqp_ws_get_operation_status` shall invoke the `prov_trans_common_amqp_get_operation_status` method **]**

### prov_transport_amqp_ws_dowork

```c
void prov_transport_amqp_ws_dowork(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_007: [** `prov_transport_amqp_ws_dowork` shall invoke the `prov_trans_common_amqp_dowork` method **]**

### prov_transport_amqp_ws_set_trace

```c
int prov_transport_amqp_ws_set_trace(PROV_TRANSPORT_HANDLE handle, bool trace_on)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_008: [** `prov_transport_amqp_ws_set_trace` shall invoke the `prov_trans_common_amqp_set_trace` method **]**

### prov_transport_amqp_ws_x509_cert

```c
static int prov_transport_amqp_ws_x509_cert(PROV_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_009: [** `prov_transport_amqp_ws_x509_cert` shall invoke the `prov_trans_common_amqp_x509_cert` method **]**

### prov_transport_amqp_ws_set_trusted_cert

```c
static int prov_transport_amqp_ws_set_trusted_cert(PROV_TRANSPORT_HANDLE handle, const char* certificate)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_010: [** `prov_transport_amqp_ws_set_trusted_cert` shall invoke the `prov_trans_common_amqp_set_trusted_cert` method **]**

### prov_transport_amqp_ws_set_proxy

```c
static int prov_transport_amqp_ws_set_proxy(PROV_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_011: [** `prov_transport_amqp_ws_set_proxy` shall invoke the `prov_trans_common_amqp_set_proxy` method **]**

### amqp_transport_ws_io

```c
static XIO_HANDLE amqp_transport_ws_io(const char* fqdn, SASL_MECHANISM_HANDLE* sasl_mechanism, const HTTP_PROXY_OPTIONS* proxy_info)
```

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_012: [** If `proxy_info` is not NULL, `amqp_transport_ws_io` shall construct a `HTTP_PROXY_IO_CONFIG` object and assign it to `TLSIO_CONFIG` `underlying_io_parameters` **]**

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_013: [** If any failure is encountered `amqp_transport_ws_io` shall return NULL **]**

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_014: [** On success `amqp_transport_ws_io` shall return an allocated `XIO_HANDLE`. **]**

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_015: [** `amqp_transport_ws_io` shall allocate a `transfer_handle` by calling xio_create with the `ws_io_interface`. **]**

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_016: [** `amqp_transport_ws_io` shall allocate a `sasl_handle` by calling xio_create with the `saslio_interface`. **]**

**PROV_TRANSPORT_AMQP_WS_CLIENT_07_017: [** If `sasl_mechanism` is NULL `amqp_transport_ws_io` shall assign the `transfer_handle` to the result and return the value. **]**
