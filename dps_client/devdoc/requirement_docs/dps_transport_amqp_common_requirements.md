# dps_transport_amqp_common Requirements

================================

## Overview

dps_transport_amqp_common module implements the DPS amqp transport implementation

## Dependencies

uAmqp module

## Exposed API

```c
typedef struct DPS_TRANSPORT_IO_INFO_TAG
{
    XIO_HANDLE transport_handle;
    XIO_HANDLE sasl_handle;
} DPS_TRANSPORT_IO_INFO;

typedef DPS_TRANSPORT_IO_INFO*(*DPS_AMQP_TRANSPORT_IO)(const char* fully_qualified_name, SASL_MECHANISM_HANDLE sasl_mechanism, const HTTP_PROXY_OPTIONS* proxy_info);

DPS_TRANSPORT_HANDLE dps_transport_common_amqp_create(const char*, uri, DPS_HSM_TYPE, type, const char*, scope_id, const char*, registration_id, const char*, dps_api_version, DPS_AMQP_TRANSPORT_IO, transport_io);
void dps_transport_common_amqp_destroy(DPS_TRANSPORT_HANDLE handle);
int dps_transport_common_amqp_open(DPS_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx);
int dps_transport_common_amqp_close(DPS_TRANSPORT_HANDLE handle);
int dps_transport_common_amqp_register_device(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx);
int dps_transport_common_amqp_get_operation_status(DPS_TRANSPORT_HANDLE handle);
void dps_transport_common_amqp_dowork(DPS_TRANSPORT_HANDLE handle);
int dps_transport_common_amqp_set_trace(DPS_TRANSPORT_HANDLE handle, bool trace_on);
int dps_transport_common_amqp_set_proxy(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options);
int dps_transport_common_amqp_set_trusted_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate);
int dps_transport_common_amqp_x509_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key);
```

### dps_transport_common_amqp_create

Creates the DPS_TRANSPORT_HANDLE using the specified interface parameters

```c
DPS_TRANSPORT_HANDLE dps_transport_common_amqp_create(const char* uri, DPS_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* dps_api_version, DPS_AMQP_TRANSPORT_IO transport_io)
```

**DPS_TRANSPORT_AMQP_COMMON_07_001: [** If `uri`, `scope_id`, `registration_id`, `dps_api_version`, or `transport_io` is NULL, `dps_transport_common_amqp_create` shall return NULL. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_002: [** If any error is encountered, `dps_transport_common_amqp_create` shall return NULL. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_003: [** `dps_transport_common_amqp_create` shall obtain a `DPS_TRANSPORT_AMQP_INFO` structure from the parent module. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_004: [** On success `dps_transport_common_amqp_create` shall return a new instance of `DPS_TRANSPORT_HANDLE`. **]**

### dps_transport_common_amqp_destroy

Frees any resources created by the dps_transport_commonhttp module

```c
void dps_transport_common_amqp_destroy(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_AMQP_COMMON_07_005: [** If `handle` is NULL, `dps_transport_common_amqp_destroy` shall do nothing. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_006: [** `dps_transport_common_amqp_destroy` shall free all resources used in this module. **]**

### dps_transport_common_amqp_open

Opens the amqp transport for future communications

```c
int dps_transport_common_amqp_open(DPS_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
```

**DPS_TRANSPORT_AMQP_COMMON_07_007: [** If `handle`, `data_callback`, or `status_cb` is NULL, `dps_transport_common_amqp_open` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_008: [** If `hsm_type` is `DPS_HSM_TYPE_TPM` and `ek` or `srk` is NULL, `dps_transport_common_amqp_open` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_058: [** If `hsm_type` is `DPS_HSM_TYPE_TPM`, `dps_transport_common_amqp_open` shall store the `ek` and `srk` values. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_009: [** `dps_transport_common_amqp_open` shall clone the ek and srk values.**]**

**DPS_TRANSPORT_AMQP_COMMON_07_010: [** When complete `dps_transport_common_amqp_open` shall send a status callback of `DPS_TRANSPORT_STATUS_CONNECTED`.**]**

**DPS_TRANSPORT_AMQP_COMMON_07_041: [** If a failure is encountered, `dps_transport_common_amqp_open` shall return a non-zero value. **]**

### dps_transport_common_amqp_close

Closes the amqp communication

```c
int dps_transport_common_amqp_close(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_AMQP_COMMON_07_011: [** If `handle` is NULL, `dps_transport_common_amqp_close` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_012: [** `dps_transport_common_amqp_close` shall close all links and connection associated with amqp communication. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_013: [** On success `dps_transport_common_amqp_close` shall return a zero value. **]**

### dps_transport_common_amqp_register_device

Begins the registration process for the device

```c
int dps_transport_common_amqp_register_device(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx)
```

**DPS_TRANSPORT_AMQP_COMMON_07_014: [** If `handle` is NULL, `dps_transport_common_amqp_register_device` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_015: [** If `hsm_type` is of type `DPS_HSM_TYPE_TPM` and `reg_challenge_cb` is NULL, `dps_transport_common_amqp_register_device` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_016: [** If the `transport_state` is set to `TRANSPORT_CLIENT_STATE_ERROR` shall, `dps_transport_common_amqp_register_device` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_017: [** On success `dps_transport_common_amqp_register_device` shall return a zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_060: [** If this device in the registration process `dps_transport_common_amqp_register_device` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_061: [** If the `transport_state` is `TRANSPORT_CLIENT_STATE_REG_SEND` or the the `operation_id` is NULL, `dps_transport_common_amqp_register_device` shall return a non-zero value. **]**

### dps_transport_common_amqp_get_operation_status

Execute a get operation status for the DPS endpoint

```c
int dps_transport_common_amqp_get_operation_status(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_AMQP_COMMON_07_018: [** If `handle` is NULL, `dps_transport_common_amqp_get_operation_status` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_019: [** If the operation_id is NULL, `dps_transport_common_amqp_get_operation_status` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_020: [** If the `transport_state` is set to `TRANSPORT_CLIENT_STATE_ERROR` shall, `dps_transport_common_amqp_get_operation_status` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_021: [** `dps_transport_common_amqp_get_operation_status` shall set the `transport_state` to `TRANSPORT_CLIENT_STATE_STATUS_SEND`. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_022: [** On success `dps_transport_common_amqp_get_operation_status` shall return a zero value. **]**

### dps_transport_common_amqp_dowork

```c
void dps_transport_common_amqp_dowork(DPS_TRANSPORT_HANDLE handle)
```

**DPS_TRANSPORT_AMQP_COMMON_07_046: [** If `handle` is NULL, `dps_transport_common_amqp_dowork` shall do nothing. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_047: [** If the `amqp_state` is `AMQP_STATE_DISCONNECTED` `dps_transport_common_amqp_dowork` shall attempt to connect the amqp connections and links. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_048: [** If the `hsm_type` is `DPS_HSM_TYPE_TPM` `create_connection` shall create a `tpm_saslmechanism`. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_049: [** If any error is encountered `dps_transport_common_amqp_dowork` shall set the `amqp_state` to `AMQP_STATE_ERROR` and the `transport_state` to `TRANSPORT_CLIENT_STATE_ERROR`. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_050: [** The receiver and sender endpoints addresses shall be constructed in the following manner: `amqps://[hostname]/[scope_id]/registrations/[registration_id]` **]**

**DPS_TRANSPORT_AMQP_COMMON_07_052: [** Once the uamqp receiver and sender link are connected the `amqp_state` shall be set to `AMQP_STATE_CONNECTED` **]**

**DPS_TRANSPORT_AMQP_COMMON_07_051: [** Once connected `dps_transport_common_amqp_dowork` shall call uamqp connection dowork function and check the `transport_state` **]**

**DPS_TRANSPORT_AMQP_COMMON_07_053: [** When then `transport_state` is set to `TRANSPORT_CLIENT_STATE_REG_SEND`, `dps_transport_common_amqp_dowork` shall send a `AMQP_REGISTER_ME` message **]**

**DPS_TRANSPORT_AMQP_COMMON_07_054: [** Upon successful sending of a `TRANSPORT_CLIENT_STATE_REG_SEND` message, `dps_transport_common_amqp_dowork` shall set the `transport_state` to `TRANSPORT_CLIENT_STATE_REG_SENT` **]**

**DPS_TRANSPORT_AMQP_COMMON_07_055: [** When then `transport_state` is set to `TRANSPORT_CLIENT_STATE_STATUS_SEND`, `dps_transport_common_amqp_dowork` shall send a `AMQP_OPERATION_STATUS` message **]**

**DPS_TRANSPORT_AMQP_COMMON_07_056: [** Upon successful sending of a `AMQP_OPERATION_STATUS` message, `dps_transport_common_amqp_dowork` shall set the `transport_state` to `TRANSPORT_CLIENT_STATE_STATUS_SENT` **]**

**DPS_TRANSPORT_AMQP_COMMON_07_057: [** If `transport_state` is set to `TRANSPORT_CLIENT_STATE_ERROR`, `dps_transport_common_amqp_dowork` shall call the `register_data_cb` function with `DPS_TRANSPORT_RESULT_ERROR` setting the `transport_state` to `TRANSPORT_CLIENT_STATE_IDLE` **]**

### dps_transport_common_amqp_set_trace

```c
int dps_transport_common_amqp_set_trace(DPS_TRANSPORT_HANDLE handle, bool trace_on)
```

**DPS_TRANSPORT_AMQP_COMMON_07_023: [** If `handle` is NULL, `dps_transport_common_amqp_set_trace` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_024: [** `dps_transport_common_amqp_set_trace` shall set the `log_trace` variable to trace_on. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_025: [** On success `dps_transport_common_amqp_set_trace` shall return a zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_059: [** If the uamqp connection is not NULL, `dps_transport_common_amqp_set_trace` shall set the connection trace option on that connection. **]**

### dps_transport_common_amqp_x509_cert

```c
int dps_transport_common_amqp_x509_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
```

**DPS_TRANSPORT_AMQP_COMMON_07_026: [** If `handle` or `certificate` is NULL, `dps_transport_common_amqp_x509_cert` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_027: [** `dps_transport_common_amqp_x509_cert` shall copy the `certificate` and `private_key` values. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_028: [** On any failure `dps_transport_common_amqp_x509_cert`, shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_029: [** On success `dps_transport_common_amqp_x509_cert` shall return a zero value. **]**

### dps_transport_common_amqp_set_trusted_cert

```c
int dps_transport_common_amqp_set_trusted_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate)
```

**DPS_TRANSPORT_AMQP_COMMON_07_030: [** If `handle` or `certificate` is NULL, `dps_transport_common_amqp_set_trusted_cert` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_031: [** `dps_transport_common_amqp_set_trusted_cert` shall copy the `certificate` value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_032: [** On any failure `dps_transport_common_amqp_set_trusted_cert`, shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_033: [** On success `dps_transport_common_amqp_set_trusted_cert` shall return a zero value. **]**

### dps_transport_common_amqp_set_proxy

```c
int dps_transport_common_amqp_set_proxy(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
```

**DPS_TRANSPORT_AMQP_COMMON_07_034: [** If `handle` or `proxy_options` is NULL, `dps_transport_common_amqp_set_proxy` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_035: [** If `HTTP_PROXY_OPTIONS` `host_address` is NULL, `dps_transport_common_amqp_set_proxy` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_036: [** If `HTTP_PROXY_OPTIONS` password is not NULL and password is NULL, `dps_transport_common_amqp_set_proxy` shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_037: [** If any of the `host_addess`, `username`, or `password` variables are non-NULL, `dps_transport_common_amqp_set_proxy` shall free the memory. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_038: [** `dps_transport_common_amqp_set_proxy` shall copy the `host_addess`, `username`, or `password` variables **]**

**DPS_TRANSPORT_AMQP_COMMON_07_039: [** On any failure `dps_transport_common_amqp_set_proxy`, shall return a non-zero value. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_040: [** On success `dps_transport_common_amqp_set_proxy` shall return a zero value. **]**

### on_sasl_tpm_challenge_cb

```c
static char* on_sasl_tpm_challenge_cb(const unsigned char* data, size_t length, void* user_ctx)
```

**DPS_TRANSPORT_AMQP_COMMON_07_042: [** If `user_ctx` is NULL, `on_sasl_tpm_challenge_cb` shall return NULL. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_043: [** If the `challenge_cb` function is NULL, `on_sasl_tpm_challenge_cb` shall return NULL. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_044: [** If any failure is encountered `on_sasl_tpm_challenge_cb` shall return NULL. **]**

**DPS_TRANSPORT_AMQP_COMMON_07_045: [** `on_sasl_tpm_challenge_cb` shall call the `challenge_cb` returning the resulting value. **]**