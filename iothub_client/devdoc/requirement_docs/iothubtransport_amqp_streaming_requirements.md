# iothubtransport_amqp_twin_messenger Requirements


## Overview

This module provides functionality for IoTHubTransportAMQP to work with Device Twin operations.  


## Dependencies

azure_c_shared_utility
azure_uamqp_c

   
## Exposed API

```c
extern AMQP_STREAMING_CLIENT_HANDLE amqp_streaming_client_create(const AMQP_STREAMING_CLIENT_CONFIG* client_config);
extern int amqp_streaming_client_start(AMQP_STREAMING_CLIENT_HANDLE client_handle, SESSION_HANDLE session_handle); 
extern int amqp_streaming_client_stop(AMQP_STREAMING_CLIENT_HANDLE client_handle);
extern void amqp_streaming_client_do_work(AMQP_STREAMING_CLIENT_HANDLE client_handle);
extern void amqp_streaming_client_destroy(AMQP_STREAMING_CLIENT_HANDLE client_handle);
extern int amqp_streaming_client_set_option(AMQP_STREAMING_CLIENT_HANDLE client_handle, const char* name, void* value);
extern OPTIONHANDLER_HANDLE amqp_streaming_client_retrieve_options(AMQP_STREAMING_CLIENT_HANDLE client_handle);
extern int amqp_streaming_client_set_stream_request_callback(AMQP_STREAMING_CLIENT_HANDLE client_handle, IOTHUB_CLIENT_INCOMING_STREAM_REQUEST_CALLBACK streamRequestCallback, const void* context);
```


### amqp_streaming_client_create

```c
extern AMQP_STREAMING_CLIENT_HANDLE amqp_streaming_client_create(const AMQP_STREAMING_CLIENT_CONFIG* client_config);
```

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_001: [**If parameter `client_config` is NULL, amqp_streaming_client_create() shall return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_002: [**If `client_config`'s `device_id`, `iothub_host_fqdn` or `client_version` is NULL, amqp_streaming_client_create() shall return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_003: [**amqp_streaming_client_create() shall allocate memory for the messenger instance structure (aka `instance`)**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_004: [**If malloc() fails, amqp_streaming_client_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_005: [**amqp_streaming_client_create() shall save a copy of `client_config` info into `instance`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_006: [**If any `client_config` info fails to be copied, amqp_streaming_client_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_007: [**`instance->amqp_msgr` shall be set using amqp_messenger_create(), passing a AMQP_MESSENGER_CONFIG instance `amqp_msgr_config`**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_008: [**`amqp_msgr_config->client_version` shall be set with `instance->client_version`**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_009: [**`amqp_msgr_config->device_id` shall be set with `instance->device_id`**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_010: [**`amqp_msgr_config->iothub_host_fqdn` shall be set with `instance->iothub_host_fqdn`**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_011: [**`amqp_msgr_config` shall have "streams/" as send link target suffix and receive link source suffix**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_012: [**`amqp_msgr_config` shall have send and receive link attach properties set as "com.microsoft:client-version" = `instance->client_version`, "com.microsoft:channel-correlation-id" = `stream:<UUID>`, "com.microsoft:api-version" = "2016-11-14"**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_013: [**`amqp_msgr_config` shall be set with `on_amqp_messenger_state_changed_callback` and `on_amqp_messenger_subscription_changed_callback` callbacks**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_014: [**If amqp_messenger_create() fails, amqp_streaming_client_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_015: [**If no failures occur, amqp_streaming_client_create() shall return a handle to `instance`**]**  


### amqp_streaming_client_start

```c
int amqp_streaming_client_start(AMQP_STREAMING_CLIENT_HANDLE instance_handle, SESSION_HANDLE session_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_016: [**If `instance_handle` or `session_handle` are NULL, amqp_streaming_client_start() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_017: [**amqp_messenger_start() shall be invoked passing `instance->amqp_msgr` and `session_handle`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_018: [**If amqp_messenger_start() fails, amqp_streaming_client_start() fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_019: [**If no failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_STARTING, and `instance->on_state_changed_callback` invoked if provided**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_020: [**If any failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_021: [**If no failures occur, amqp_streaming_client_start() shall return 0**]**


### amqp_streaming_client_stop

```c
int amqp_streaming_client_stop(AMQP_STREAMING_CLIENT_HANDLE instance_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_022: [**If `instance_handle` is NULL, amqp_streaming_client_stop() shall fail and return a non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_023: [**amqp_messenger_stop() shall be invoked passing `instance->amqp_msgr`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_024: [**If amqp_messenger_stop() fails, amqp_streaming_client_stop() fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_025: [**`instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_STOPPING, and `instance->on_state_changed_callback` invoked if provided**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_026: [**If any failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_027: [**If no failures occur, amqp_streaming_client_stop() shall return 0**]**


### amqp_streaming_client_do_work
```c
void amqp_streaming_client_do_work(AMQP_STREAMING_CLIENT_HANDLE instance_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_028: [**If `instance_handle` is NULL, amqp_streaming_client_do_work() shall return immediately**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_031: [**amqp_streaming_client_do_work() shall invoke amqp_messenger_do_work() passing `instance->amqp_msgr`**]**  


## amqp_streaming_client_destroy

```c
void amqp_streaming_client_destroy(AMQP_STREAMING_CLIENT_HANDLE instance_handle);
```

Destroys all the components within the messenger data structure (instance_handle) and releases any allocated memory.

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_032: [**If `instance_handle` is NULL, amqp_streaming_client_destroy() shall return immediately**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_033: [**`instance->amqp_messenger` shall be destroyed using amqp_messenger_destroy()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_034: [**amqp_streaming_client_destroy() shall release all memory allocated for and within `instance`**]**  


## amqp_streaming_client_set_option

```c
int amqp_streaming_client_set_option(AMQP_STREAMING_CLIENT_HANDLE instance_handle, const char* name, void* value);
```

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_035: [**If `instance_handle` or `name` or `value` are NULL, amqp_streaming_client_set_option() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_036: [**amqp_messenger_set_option() shall be invoked passing `name` and `option`**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_037: [**If amqp_messenger_set_option() fails, amqp_streaming_client_set_option() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_038: [**If no errors occur, amqp_streaming_client_set_option shall return zero**]**


## amqp_streaming_client_retrieve_options

```c
OPTIONHANDLER_HANDLE amqp_streaming_client_retrieve_options(AMQP_STREAMING_CLIENT_HANDLE instance_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_039: [**If `instance_handle` is NULL, amqp_streaming_client_retrieve_options shall fail and return NULL**]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_040: [**amqp_streaming_client_retrieve_options() shall return the result of amqp_messenger_retrieve_options()**]**


## amqp_streaming_client_set_stream_request_callback

```c
int amqp_streaming_client_set_stream_request_callback(AMQP_STREAMING_CLIENT_HANDLE client_handle, IOTHUB_CLIENT_INCOMING_STREAM_REQUEST_CALLBACK streamRequestCallback, const void* context);
```

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_041: [** If `instance_handle` is NULL, the function shall return a non-zero value (failure) **]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_042: [** `streamRequestCallback` and `context` shall be saved in `instance_handle` to be used in `on_amqp_message_received_callback` **]**

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_047: [** If no errors occur, the function shall return zero (success) **]**


#### on_amqp_message_received_callback
```c
static AMQP_MESSENGER_DISPOSITION_RESULT on_amqp_message_received_callback(MESSAGE_HANDLE message, AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_048: [**If `message` or `context` are NULL, on_amqp_message_received_callback shall return immediately**]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_049: [** `streamRequestCallback` shall be invoked passing the parsed STREAM_REQUEST_INFO instance **]**  

These are the AMQP annotations conveying information about the stream request:

Stream Name: "IoThub-streaming-name"
Ip Address: "IoThub-streaming-ip-address"
Streaming gateway Uri: "IoThub-streaming-url"
Authorization Token: "IoThub-streaming-auth-token"

The user custom data shall be extracted from the AMQP body (string) and AMQP properties content-type and content-encoding.


**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_050: [** The response from `streamRequestCallback` shall be saved to be sent on the next call to _do_work() **]**  

**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_051: [** If any parsing errors occur, `on_amqp_message_received_callback` shall return AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED **]
**  
**SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_052: [** If no parsing errors occur, `on_amqp_message_received_callback` shall return AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED **]**  


