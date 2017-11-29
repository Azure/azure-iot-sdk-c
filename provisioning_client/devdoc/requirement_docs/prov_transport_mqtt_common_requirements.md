# prov_transport_mqtt_common Requirements

================================

## Overview

prov_transport_mqtt_common module implements the DPS amqp transport implementation

## Dependencies

uAmqp module

## Exposed API

```c
typedef XIO_HANDLE(*PROV_MQTT_TRANSPORT_IO)(const char* fully_qualified_name, const HTTP_PROXY_OPTIONS* proxy_info);

MOCKABLE_FUNCTION(, PROV_TRANSPORT_HANDLE, prov_transport_common_mqtt_create, const char*, uri, PROV_HSM_TYPE, type, const char*, scope_id, const char*, registration_id, const char*, prov_api_version, PROV_MQTT_TRANSPORT_IO, transport_io);
MOCKABLE_FUNCTION(, void, prov_transport_common_mqtt_destroy, PROV_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, prov_transport_common_mqtt_open, PROV_TRANSPORT_HANDLE, handle, BUFFER_HANDLE, ek, BUFFER_HANDLE, srk, PROV_TRANSPORT_REGISTER_DATA_CALLBACK, data_callback, void*, user_ctx, PROV_TRANSPORT_STATUS_CALLBACK, status_cb, void*, status_ctx);
MOCKABLE_FUNCTION(, int, prov_transport_common_mqtt_close, PROV_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, prov_transport_common_mqtt_register_device, PROV_TRANSPORT_HANDLE, handle, PROV_TRANSPORT_CHALLENGE_CALLBACK, reg_challenge_cb, void*, user_ctx, PROV_TRANSPORT_JSON_PARSE, json_parse_cb, void*, json_ctx);
MOCKABLE_FUNCTION(, int, prov_transport_common_mqtt_get_operation_status, PROV_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, void, prov_transport_common_mqtt_dowork, PROV_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, prov_transport_common_mqtt_set_trace, PROV_TRANSPORT_HANDLE, handle, bool, trace_on);
MOCKABLE_FUNCTION(, int, prov_transport_common_mqtt_set_proxy, PROV_TRANSPORT_HANDLE, handle, const HTTP_PROXY_OPTIONS*, proxy_options);
MOCKABLE_FUNCTION(, int, prov_transport_common_mqtt_set_trusted_cert, PROV_TRANSPORT_HANDLE, handle, const char*, certificate);
MOCKABLE_FUNCTION(, int, prov_transport_common_mqtt_x509_cert, PROV_TRANSPORT_HANDLE, handle, const char*, certificate, const char*, private_key);
```

### prov_transport_common_mqtt_create

Creates the PROV_TRANSPORT_HANDLE using the specified interface parameters

```c
PROV_TRANSPORT_HANDLE prov_transport_common_mqtt_create(const char* uri, PROV_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* prov_api_version, PROV_AMQP_TRANSPORT_IO transport_io)
```

**PROV_TRANSPORT_MQTT_COMMON_07_001: [** If `uri`, `scope_id`, `registration_id`, `prov_api_version`, or `transport_io` is NULL, `prov_transport_common_mqtt_create` shall return NULL. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_002: [** If any error is encountered, `prov_transport_common_mqtt_create` shall return NULL. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_003: [** `prov_transport_common_mqtt_create` shall allocate a `PROV_TRANSPORT_MQTT_INFO` and initialize the containing fields. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_004: [** On success `prov_transport_common_mqtt_create` shall return a new instance of `PROV_TRANSPORT_HANDLE`. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_062: [** If `PROV_HSM_TYPE` is `PROV_HSM_TYPE_TPM` `prov_transport_common_mqtt_create` shall return NULL (currently TPM is not supported). **]**

### prov_transport_common_mqtt_destroy

Frees any resources created by the prov_transport_commonhttp module

```c
void prov_transport_common_mqtt_destroy(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_MQTT_COMMON_07_005: [** If `handle` is NULL, `prov_transport_common_mqtt_destroy` shall do nothing. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_006: [** `prov_transport_common_mqtt_destroy` shall free all resources used in this module. **]**

### prov_transport_common_mqtt_open

Opens the amqp transport for future communications

```c
int prov_transport_common_mqtt_open(PROV_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, PROV_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, PROV_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
```

**PROV_TRANSPORT_MQTT_COMMON_07_007: [** If `handle`, `data_callback`, or `status_cb` is NULL, `prov_transport_common_mqtt_open` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_008: [** If `hsm_type` is `PROV_HSM_TYPE_TPM` and `ek` or `srk` is NULL, `prov_transport_common_mqtt_open` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_058: [** If `hsm_type` is `PROV_HSM_TYPE_TPM`, `prov_transport_common_mqtt_open` shall store the `ek` and `srk` values. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_009: [** `prov_transport_common_mqtt_open` shall clone the ek and srk values.**]**

**PROV_TRANSPORT_MQTT_COMMON_07_041: [** If a failure is encountered, `prov_transport_common_mqtt_open` shall return a non-zero value. **]**

### prov_transport_common_mqtt_close

Closes the amqp communication

```c
int prov_transport_common_mqtt_close(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_MQTT_COMMON_07_011: [** If `handle` is NULL, `prov_transport_common_mqtt_close` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_012: [** `prov_transport_common_mqtt_close` shall close all connection associated with mqtt communication. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_013: [** On success `prov_transport_common_mqtt_close` shall return a zero value. **]**

### prov_transport_common_mqtt_register_device

Begins the registration process for the device

```c
int prov_transport_common_mqtt_register_device(PROV_TRANSPORT_HANDLE handle, PROV_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx)
```

**PROV_TRANSPORT_MQTT_COMMON_07_014: [** If `handle` is NULL, `prov_transport_common_mqtt_register_device` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_015: [** If `hsm_type` is of type `PROV_HSM_TYPE_TPM` and `reg_challenge_cb` is NULL, `prov_transport_common_mqtt_register_device` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_016: [** If the `transport_state` is set to `TRANSPORT_CLIENT_STATE_ERROR` shall, `prov_transport_common_mqtt_register_device` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_017: [** On success `prov_transport_common_mqtt_register_device` shall return a zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_060: [** If this device in the registration process `prov_transport_common_mqtt_register_device` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_061: [** If the `transport_state` is `TRANSPORT_CLIENT_STATE_REG_SEND` or the the `operation_id` is NULL, `prov_transport_common_mqtt_register_device` shall return a non-zero value. **]**

### prov_transport_common_mqtt_get_operation_status

Execute a get operation status for the DPS endpoint

```c
int prov_transport_common_mqtt_get_operation_status(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_MQTT_COMMON_07_018: [** If `handle` is NULL, `prov_transport_common_mqtt_get_operation_status` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_019: [** If the operation_id is NULL, `prov_transport_common_mqtt_get_operation_status` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_020: [** If the `transport_state` is set to `TRANSPORT_CLIENT_STATE_ERROR` shall, `prov_transport_common_mqtt_get_operation_status` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_021: [** `prov_transport_common_mqtt_get_operation_status` shall set the `transport_state` to `TRANSPORT_CLIENT_STATE_STATUS_SEND`. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_022: [** On success `prov_transport_common_mqtt_get_operation_status` shall return a zero value. **]**

### prov_transport_common_mqtt_dowork

```c
void prov_transport_common_mqtt_dowork(PROV_TRANSPORT_HANDLE handle)
```

**PROV_TRANSPORT_MQTT_COMMON_07_046: [** If `handle` is NULL, `prov_transport_common_mqtt_dowork` shall do nothing. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_047: [** If the `mqtt_state` is `MQTT_STATE_DISCONNECTED` `prov_transport_common_mqtt_dowork` shall attempt to connect the mqtt connections. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_049: [** If any error is encountered `prov_transport_common_mqtt_dowork` shall set the `mqtt_state` to `MQTT_STATE_ERROR` and the `transport_state` to `TRANSPORT_CLIENT_STATE_ERROR`. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_050: [** When the `mqtt_state` is `MQTT_STATE_CONNECTED`, `prov_transport_common_mqtt_dowork` shall subscribe to the topic `$dps/registrations/res/#` **]**

**PROV_TRANSPORT_MQTT_COMMON_07_052: [** Once the mqtt CONNACK is recieved `prov_transport_common_mqtt_dowork` shall set `mqtt_state` to `MQTT_STATE_CONNECTED` **]**

**PROV_TRANSPORT_MQTT_COMMON_07_053: [** When then `transport_state` is set to `TRANSPORT_CLIENT_STATE_REG_SEND`, `prov_transport_common_mqtt_dowork` shall send a `REGISTER_ME` message **]**

**PROV_TRANSPORT_MQTT_COMMON_07_054: [** Upon successful sending of a `TRANSPORT_CLIENT_STATE_REG_SEND` message, `prov_transport_common_mqtt_dowork` shall set the `transport_state` to `TRANSPORT_CLIENT_STATE_REG_SENT` **]**

**PROV_TRANSPORT_MQTT_COMMON_07_055: [** When then `transport_state` is set to `TRANSPORT_CLIENT_STATE_STATUS_SEND`, `prov_transport_common_mqtt_dowork` shall send a `AMQP_OPERATION_STATUS` message **]**

**PROV_TRANSPORT_MQTT_COMMON_07_056: [** Upon successful sending of a `AMQP_OPERATION_STATUS` message, `prov_transport_common_mqtt_dowork` shall set the `transport_state` to `TRANSPORT_CLIENT_STATE_STATUS_SENT` **]**

**PROV_TRANSPORT_MQTT_COMMON_07_057: [** If `transport_state` is set to `TRANSPORT_CLIENT_STATE_ERROR`, `prov_transport_common_mqtt_dowork` shall call the `register_data_cb` function with `PROV_TRANSPORT_RESULT_ERROR` setting the `transport_state` to `TRANSPORT_CLIENT_STATE_IDLE` **]**

### prov_transport_common_mqtt_set_trace

```c
int prov_transport_common_mqtt_set_trace(PROV_TRANSPORT_HANDLE handle, bool trace_on)
```

**PROV_TRANSPORT_MQTT_COMMON_07_023: [** If `handle` is NULL, `prov_transport_common_mqtt_set_trace` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_024: [** `prov_transport_common_mqtt_set_trace` shall set the `log_trace` variable to trace_on. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_025: [** On success `prov_transport_common_mqtt_set_trace` shall return a zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_059: [** If the umqtt connection is not NULL, `prov_transport_common_mqtt_set_trace` shall set the trace option on that connection. **]**

### prov_transport_common_mqtt_x509_cert

```c
int prov_transport_common_mqtt_x509_cert(PROV_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
```

**PROV_TRANSPORT_MQTT_COMMON_07_026: [** If `handle` or `certificate` is NULL, `prov_transport_common_mqtt_x509_cert` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_027: [** `prov_transport_common_mqtt_x509_cert` shall copy the `certificate` and `private_key` values. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_028: [** On any failure `prov_transport_common_mqtt_x509_cert`, shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_029: [** On success `prov_transport_common_mqtt_x509_cert` shall return a zero value. **]**

### prov_transport_common_mqtt_set_trusted_cert

```c
int prov_transport_common_mqtt_set_trusted_cert(PROV_TRANSPORT_HANDLE handle, const char* certificate)
```

**PROV_TRANSPORT_MQTT_COMMON_07_030: [** If `handle` or `certificate` is NULL, `prov_transport_common_mqtt_set_trusted_cert` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_031: [** `prov_transport_common_mqtt_set_trusted_cert` shall copy the `certificate` value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_032: [** On any failure `prov_transport_common_mqtt_set_trusted_cert`, shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_033: [** On success `prov_transport_common_mqtt_set_trusted_cert` shall return a zero value. **]**

### prov_transport_common_mqtt_set_proxy

```c
int prov_transport_common_mqtt_set_proxy(PROV_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
```

**PROV_TRANSPORT_MQTT_COMMON_07_034: [** If `handle` or `proxy_options` is NULL, `prov_transport_common_mqtt_set_proxy` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_035: [** If `HTTP_PROXY_OPTIONS` `host_address` is NULL, `prov_transport_common_mqtt_set_proxy` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_036: [** If `HTTP_PROXY_OPTIONS` password is not NULL and username is NULL, `prov_transport_common_mqtt_set_proxy` shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_037: [** If any of the `host_addess`, `username`, or `password` variables are non-NULL, `prov_transport_common_mqtt_set_proxy` shall free the memory. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_038: [** `prov_transport_common_mqtt_set_proxy` shall copy the `host_addess`, `username`, or `password` variables **]**

**PROV_TRANSPORT_MQTT_COMMON_07_039: [** On any failure `prov_transport_common_mqtt_set_proxy`, shall return a non-zero value. **]**

**PROV_TRANSPORT_MQTT_COMMON_07_040: [** On success `prov_transport_common_mqtt_set_proxy` shall return a zero value. **]**
