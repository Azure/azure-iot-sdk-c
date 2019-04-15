# IoTHubTransport_AMQP_Device Requirements
================

## Overview

This module encapsulates all the components that represent a logical device registered through the AMQP transport in Azure C SDK.
It is responsible for triggering the device authentication, messaging exchange.


## Dependencies

This module will depend on the following modules:

azure-c-shared-utility
azure-uamqp-c
iothubtransport_amqp_cbs_authentication
iothubtransport_amqp_telemetry_messenger


## Exposed API

```c
static const char* DEVICE_OPTION_SAVED_OPTIONS = "saved_device_options";
static const char* DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS = "event_send_timeout_secs";
static const char* DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS = "cbs_request_timeout_secs";
static const char* DEVICE_OPTION_SAS_TOKEN_REFRESH_TIME_SECS = "sas_token_refresh_time_secs";
static const char* DEVICE_OPTION_SAS_TOKEN_LIFETIME_SECS = "sas_token_lifetime_secs";

typedef enum DEVICE_STATE_TAG
{
	DEVICE_STATE_STOPPED,
	DEVICE_STATE_STOPPING,
	DEVICE_STATE_STARTING,
	DEVICE_STATE_STARTED,
	DEVICE_STATE_ERROR_AUTH,
	DEVICE_STATE_ERROR_AUTH_TIMEOUT,
	DEVICE_STATE_ERROR_MSG
} DEVICE_STATE;

typedef enum DEVICE_AUTH_MODE_TAG
{
	DEVICE_AUTH_MODE_CBS,
	DEVICE_AUTH_MODE_X509
} DEVICE_AUTH_MODE;

typedef enum DEVICE_SEND_STATUS_TAG
{
	DEVICE_SEND_STATUS_IDLE,
	DEVICE_SEND_STATUS_BUSY
} DEVICE_SEND_STATUS;

typedef enum D2C_EVENT_SEND_RESULT_TAG
{
	D2C_EVENT_SEND_COMPLETE_RESULT_OK,
	D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE,
	D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING,
	D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT,
	D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED,
	D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_UNKNOWN
} D2C_EVENT_SEND_RESULT;

typedef enum DEVICE_MESSAGE_DISPOSITION_RESULT_TAG
{
	DEVICE_MESSAGE_DISPOSITION_RESULT_NONE,
	DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED,
	DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED,
	DEVICE_MESSAGE_DISPOSITION_RESULT_ABANDONED
} DEVICE_MESSAGE_DISPOSITION_RESULT;

typedef enum DEVICE_TWIN_UPDATE_RESULT_TAG
{
    DEVICE_TWIN_UPDATE_RESULT_OK,
    DEVICE_TWIN_UPDATE_RESULT_ERROR
} DEVICE_TWIN_UPDATE_RESULT;

typedef enum DEVICE_TWIN_UPDATE_TYPE_TAG
{
    DEVICE_TWIN_UPDATE_TYPE_PARTIAL,
    DEVICE_TWIN_UPDATE_TYPE_COMPLETE
} DEVICE_TWIN_UPDATE_TYPE;

typedef void(*ON_DEVICE_STATE_CHANGED)(void* context, DEVICE_STATE previous_state, DEVICE_STATE new_state);
typedef DEVICE_MESSAGE_DISPOSITION_RESULT(*ON_DEVICE_C2D_MESSAGE_RECEIVED)(IOTHUB_MESSAGE_HANDLE message, DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info, void* context);
typedef void(*ON_DEVICE_D2C_EVENT_SEND_COMPLETE)(IOTHUB_MESSAGE_LIST* message, D2C_EVENT_SEND_RESULT result, void* context);
typedef void(*DEVICE_SEND_TWIN_UPDATE_COMPLETE_CALLBACK)(DEVICE_TWIN_UPDATE_RESULT result, int status_code, void* context);
typedef void(*DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK)(DEVICE_TWIN_UPDATE_TYPE update_type, const unsigned char* message, size_t length, void* context);

typedef struct DEVICE_CONFIG_TAG
{
    const char* device_id;
    char* iothub_host_fqdn;
    DEVICE_AUTH_MODE authentication_mode;
    ON_DEVICE_STATE_CHANGED on_state_changed_callback;
    void* on_state_changed_context;
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;
} AMQP_DEVICE_CONFIG;

typedef struct DEVICE_INSTANCE* DEVICE_HANDLE;

extern DEVICE_HANDLE amqp_device_create(AMQP_DEVICE_CONFIG* config);
extern void amqp_device_destroy(DEVICE_HANDLE handle);
extern int amqp_device_start_async(DEVICE_HANDLE handle, SESSION_HANDLE session_handle, CBS_HANDLE cbs_handle);
extern int amqp_device_stop(DEVICE_HANDLE handle);
extern void amqp_device_do_work(DEVICE_HANDLE handle);
extern int amqp_device_send_event_async(DEVICE_HANDLE handle, IOTHUB_MESSAGE_LIST* message, ON_DEVICE_D2C_EVENT_SEND_COMPLETE on_device_d2c_event_send_complete_callback, void* context);
extern int amqp_device_send_twin_update_async(DEVICE_HANDLE handle, CONSTBUFFER_HANDLE data, DEVICE_SEND_TWIN_UPDATE_COMPLETE_CALLBACK on_send_twin_update_complete_callback, void* context);
extern int amqp_device_subscribe_for_twin_updates(DEVICE_HANDLE handle, DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK on_device_twin_update_received_callback, void* context);
extern int amqp_device_unsubscribe_for_twin_updates(DEVICE_HANDLE handle);
extern int amqp_device_get_twin_async(AMQP_DEVICE_HANDLE handle, DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK on_device_get_twin_completed_callback, void* context);
extern int amqp_device_get_send_status(DEVICE_HANDLE handle, DEVICE_SEND_STATUS *send_status);
extern int amqp_device_subscribe_message(DEVICE_HANDLE handle, ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback, void* context);
extern int amqp_device_unsubscribe_message(DEVICE_HANDLE handle);
extern int amqp_device_send_message_disposition(DEVICE_HANDLE device_handle, DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info, DEVICE_MESSAGE_DISPOSITION_RESULT disposition_result);
extern int amqp_device_set_retry_policy(DEVICE_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY policy, size_t retry_timeout_limit_in_seconds);
extern int amqp_device_set_option(DEVICE_HANDLE handle, const char* name, void* value);
extern OPTIONHANDLER_HANDLE amqp_device_retrieve_options(DEVICE_HANDLE handle);

```

Note: `instance` refers to the structure that holds the current state and control parameters of the device.
In each function (other than amqp_device_create) it shall derive from the AMQP_DEVICE_HANDLE handle passed as argument.


### amqp_device_create

```c
extern DEVICE_HANDLE amqp_device_create(AMQP_DEVICE_CONFIG config);
```

**SRS_DEVICE_09_001: [**If `config`, `authorization_module` or `iothub_host_fqdn` or on_state_changed_callback are NULL then amqp_device_create shall fail and return NULL**]**
**SRS_DEVICE_09_002: [**amqp_device_create shall allocate memory for the device instance structure**]**
**SRS_DEVICE_09_003: [**If malloc fails, amqp_device_create shall fail and return NULL**]**
**SRS_DEVICE_09_004: [**All `config` parameters shall be saved into `instance`**]**
**SRS_DEVICE_09_005: [**If any `config` parameters fail to be saved into `instance`, amqp_device_create shall fail and return NULL**]**
**SRS_DEVICE_09_006: [**If `instance->authentication_mode` is DEVICE_AUTH_MODE_CBS, `instance->authentication_handle` shall be set using authentication_create()**]**
**SRS_DEVICE_09_007: [**If the AUTHENTICATION_HANDLE fails to be created, amqp_device_create shall fail and return NULL**]**
**SRS_DEVICE_09_008: [**`instance->messenger_handle` shall be set using telemetry_messenger_create()**]**
**SRS_DEVICE_09_009: [**If the MESSENGER_HANDLE fails to be created, amqp_device_create shall fail and return NULL**]**

**SRS_DEVICE_09_122: [**`instance->twin_messenger_handle` shall be set using twin_messenger_create()**]**
**SRS_DEVICE_09_123: [**If the TWIN_MESSENGER_HANDLE fails to be created, amqp_device_create shall fail and return NULL**]**

**SRS_DEVICE_09_010: [**If amqp_device_create fails it shall release all memory it has allocated**]**
**SRS_DEVICE_09_011: [**If amqp_device_create succeeds it shall return a handle to its `instance` structure**]**


### amqp_device_destroy

```c
extern void amqp_device_destroy(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_012: [**If `handle` is NULL, amqp_device_destroy shall return**]**
**SRS_DEVICE_09_013: [**If the device is in state DEVICE_STATE_STARTED or DEVICE_STATE_STARTING, amqp_device_stop() shall be invoked**]**
**SRS_DEVICE_09_014: [**`instance->messenger_handle shall be destroyed using telemetry_messenger_destroy()`**]**
**SRS_DEVICE_09_015: [**If created, `instance->authentication_handle` shall be destroyed using authentication_destroy()`**]**
**SRS_DEVICE_09_016: [**The contents of `instance->config` shall be detroyed and then it shall be freed**]**


### amqp_device_start_async

```c
extern int amqp_device_start_async(DEVICE_HANDLE handle, SESSION_HANDLE session_handle, CBS_HANDLE cbs_handle);
```

**SRS_DEVICE_09_017: [**If `handle` is NULL, amqp_device_start_async shall return a non-zero result**]**
**SRS_DEVICE_09_018: [**If the device state is not DEVICE_STATE_STOPPED, amqp_device_start_async shall return a non-zero result**]**
**SRS_DEVICE_09_019: [**If `session_handle` is NULL, amqp_device_start_async shall return a non-zero result**]**
**SRS_DEVICE_09_020: [**If using CBS authentication and `cbs_handle` is NULL, amqp_device_start_async shall return a non-zero result**]**
**SRS_DEVICE_09_021: [**`session_handle` and `cbs_handle` shall be saved into the `instance`**]**
**SRS_DEVICE_09_022: [**The device state shall be updated to DEVICE_STATE_STARTING, and state changed callback invoked**]**
**SRS_DEVICE_09_023: [**If no failures occur, amqp_device_start_async shall return 0**]**


### amqp_device_stop

```c
extern int amqp_device_stop(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_024: [**If `handle` is NULL, amqp_device_stop shall return a non-zero result**]**
**SRS_DEVICE_09_025: [**If the device state is already DEVICE_STATE_STOPPED or DEVICE_STATE_STOPPING, amqp_device_stop shall return a non-zero result**]**
**SRS_DEVICE_09_026: [**The device state shall be updated to DEVICE_STATE_STOPPING, and state changed callback invoked**]**
**SRS_DEVICE_09_027: [**If `instance->messenger_handle` state is not TELEMETRY_MESSENGER_STATE_STOPPED, telemetry_messenger_stop shall be invoked**]**
**SRS_DEVICE_09_028: [**If telemetry_messenger_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_MSG and the function shall return non-zero result**]**
**SRS_DEVICE_09_131: [**If `instance->twin_messenger_handle` state is not TWIN_MESSENGER_STATE_STOPPED, twin_messenger_stop shall be invoked**]**
**SRS_DEVICE_09_132: [**If twin_messenger_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_MSG and the function shall return non-zero result**]**
**SRS_DEVICE_09_029: [**If CBS authentication is used, if `instance->authentication_handle` state is not AUTHENTICATION_STATE_STOPPED, authentication_stop shall be invoked**]**
**SRS_DEVICE_09_030: [**If authentication_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_AUTH and the function shall return non-zero result**]**
**SRS_DEVICE_09_031: [**The device state shall be updated to DEVICE_STATE_STOPPED, and state changed callback invoked**]**
**SRS_DEVICE_09_032: [**If no failures occur, amqp_device_stop shall return 0**]**


### amqp_device_do_work

```c
extern void amqp_device_do_work(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_033: [**If `handle` is NULL, amqp_device_do_work shall return**]**

#### device state DEVICE_STATE_STARTING

##### Starting authentication instance

**SRS_DEVICE_09_034: [**If CBS authentication is used and authentication state is AUTHENTICATION_STATE_STOPPED, authentication_start shall be invoked**]**
**SRS_DEVICE_09_035: [**If authentication_start fails, the device state shall be updated to DEVICE_STATE_ERROR_AUTH**]**
**SRS_DEVICE_09_036: [**If authentication state is AUTHENTICATION_STATE_STARTING, the device shall track the time since last event change and timeout if needed**]**
**SRS_DEVICE_09_037: [**If authentication_start times out, the device state shall be updated to DEVICE_STATE_ERROR_AUTH_TIMEOUT**]**
**SRS_DEVICE_09_038: [**If authentication state is AUTHENTICATION_STATE_ERROR and error code is AUTH_FAILED, the device state shall be updated to DEVICE_STATE_ERROR_AUTH**]**
**SRS_DEVICE_09_039: [**If authentication state is AUTHENTICATION_STATE_ERROR and error code is TIMEOUT, the device state shall be updated to DEVICE_STATE_ERROR_AUTH_TIMEOUT**]**


##### Starting TELEMETRY messenger instance

**SRS_DEVICE_09_040: [**Messenger shall not be started if using CBS authentication and authentication start has not completed yet**]**
**SRS_DEVICE_09_041: [**If messenger state is TELEMETRY_MESSENGER_STATE_STOPPED, telemetry_messenger_start shall be invoked**]**
**SRS_DEVICE_09_042: [**If telemetry_messenger_start fails, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_043: [**If messenger state is TELEMETRY_MESSENGER_STATE_STARTING, the device shall track the time since last event change and timeout if needed**]**
**SRS_DEVICE_09_044: [**If telemetry_messenger_start times out, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_045: [**If messenger state is TELEMETRY_MESSENGER_STATE_ERROR, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_046: [**If messenger state is TELEMETRY_MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_STARTED**]**

##### Starting TWIN messenger instance

**SRS_DEVICE_09_124: [**TWIN Messenger shall not be started if using CBS authentication and authentication start has not completed yet**]**
**SRS_DEVICE_09_125: [**If TWIN messenger state is TWIN_MESSENGER_STATE_STOPPED, twin_messenger_start shall be invoked**]**
**SRS_DEVICE_09_126: [**If twin_messenger_start fails, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_127: [**If TWIN messenger state is TWIN_MESSENGER_STATE_STARTING, the device shall track the time since last event change and timeout if needed**]**
**SRS_DEVICE_09_128: [**If twin_messenger_start times out, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_129: [**If TWIN messenger state is TWIN_MESSENGER_STATE_ERROR, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_130: [**If TWIN messenger state is TWIN_MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_STARTED**]**

#### device state DEVICE_STATE_STARTED

**SRS_DEVICE_09_047: [**If CBS authentication is used and authentication state is not AUTHENTICATION_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_AUTH**]**
**SRS_DEVICE_09_048: [**If messenger state is not TELEMETRY_MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_133: [**If TWIN messenger state is not TWIN_MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**

#### Any device state

**SRS_DEVICE_09_049: [**If CBS is used for authentication and `instance->authentication_handle` state is not STOPPED or ERROR, authentication_do_work shall be invoked**]**
**SRS_DEVICE_09_050: [**If `instance->messenger_handle` state is not STOPPED or ERROR, telemetry_messenger_do_work shall be invoked**]**
**SRS_DEVICE_09_134: [**If `instance->twin_messenger_handle` state is not STOPPED or ERROR, twin_messenger_do_work shall be invoked**]**


### amqp_device_send_event_async

```c
extern int amqp_device_send_event_async(DEVICE_HANDLE handle, IOTHUB_MESSAGE_LIST* message, ON_DEVICE_D2C_EVENT_SEND_COMPLETE on_device_d2c_event_send_complete_callback, void* context);
```

**SRS_DEVICE_09_051: [**If `handle` are `message` are NULL, amqp_device_send_event_async shall return a non-zero result**]**
**SRS_DEVICE_09_052: [**A structure (`send_task`) shall be created to track the send state of the message**]**
**SRS_DEVICE_09_053: [**If `send_task` fails to be created, amqp_device_send_event_async shall return a non-zero value**]**
**SRS_DEVICE_09_054: [**`send_task` shall contain the user callback and the context provided**]**
**SRS_DEVICE_09_055: [**The message shall be sent using telemetry_messenger_send_async, passing `on_event_send_complete_messenger_callback` and `send_task`**]**
**SRS_DEVICE_09_056: [**If telemetry_messenger_send_async fails, amqp_device_send_event_async shall return a non-zero value**]**
**SRS_DEVICE_09_057: [**If any failures occur, amqp_device_send_event_async shall release all memory it has allocated**]**
**SRS_DEVICE_09_058: [**If no failures occur, amqp_device_send_event_async shall return 0**]**


#### on_event_send_complete_messenger_callback

```c
static void on_event_send_complete_messenger_callback(IOTHUB_MESSAGE_LIST* iothub_message, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT ev_send_comp_result, void* context)
```

**SRS_DEVICE_09_059: [**If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, D2C_EVENT_SEND_COMPLETE_RESULT_OK shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_060: [**If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_061: [**If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_062: [**If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_063: [**If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED, D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_064: [**If provided, the user callback and context saved in `send_task` shall be invoked passing the device `event_send_complete`**]**
**SRS_DEVICE_09_065: [**The memory allocated for `send_task` shall be released**]**


### amqp_device_send_twin_update_async
```c
extern int amqp_device_send_twin_update_async(DEVICE_HANDLE handle, CONSTBUFFER_HANDLE data, DEVICE_SEND_TWIN_UPDATE_COMPLETE_CALLBACK on_send_twin_update_complete_callback, void* context);
```

**SRS_DEVICE_09_135: [**If `handle` or `data` are NULL, amqp_device_send_twin_update_async shall return a non-zero result**]**

**SRS_DEVICE_09_136: [**A structure (`twin_ctx`) shall be created to track the send state of the twin report**]**

**SRS_DEVICE_09_137: [**If `twin_ctx` fails to be created, amqp_device_send_twin_update_async shall return a non-zero value**]**

**SRS_DEVICE_09_138: [**The twin report shall be sent using twin_messenger_report_state_async, passing `on_report_state_complete_callback` and `twin_ctx`**]**

**SRS_DEVICE_09_139: [**If twin_messenger_report_state_async fails, amqp_device_send_twin_update_async shall return a non-zero value**]**

**SRS_DEVICE_09_140: [**If no failures occur, amqp_device_send_twin_update_async shall return 0**]**


#### on_report_state_complete_callback
```c
static void on_report_state_complete_callback(TWIN_REPORT_STATE_RESULT result, TWIN_REPORT_STATE_REASON reason, int status_code, const void* context)
```

**SRS_DEVICE_09_141: [**on_send_twin_update_complete_callback (if provided by user) shall be invoked passing the corresponding device result and `status_code`**]**

**SRS_DEVICE_09_142: [**Memory allocated for `context` shall be released**]**



### amqp_device_subscribe_for_twin_updates
```c
extern int amqp_device_subscribe_for_twin_updates(DEVICE_HANDLE handle, DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK on_device_twin_update_received_callback, void* context);
```

**SRS_DEVICE_09_143: [**If `handle` or `on_device_twin_update_received_callback` are NULL, amqp_device_subscribe_for_twin_updates shall return a non-zero result**]**

**SRS_DEVICE_09_144: [**twin_messenger_subscribe shall be invoked passing `on_twin_state_update_callback`**]**

**SRS_DEVICE_09_145: [**If twin_messenger_subscribe fails, amqp_device_subscribe_for_twin_updates shall return a non-zero value**]**

**SRS_DEVICE_09_146: [**If no failures occur, amqp_device_subscribe_for_twin_updates shall return 0**]**


#### on_twin_state_update_callback
```c
static void on_twin_state_update_callback(TWIN_UPDATE_TYPE update_type, const char* payload, size_t size, const void* context)
```

**SRS_DEVICE_09_151: [**on_device_twin_update_received_callback (provided by user) shall be invoked passing the corresponding update type, `payload` and `size`**]**


### amqp_device_unsubscribe_for_twin_updates
```c
extern int amqp_device_unsubscribe_for_twin_updates(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_147: [**If `handle` is NULL, amqp_device_unsubscribe_for_twin_updates shall return a non-zero result**]**

**SRS_DEVICE_09_148: [**twin_messenger_unsubscribe shall be invoked passing `on_twin_state_update_callback`**]**

**SRS_DEVICE_09_149: [**If twin_messenger_unsubscribe fails, amqp_device_unsubscribe_for_twin_updates shall return a non-zero value**]**

**SRS_DEVICE_09_150: [**If no failures occur, amqp_device_unsubscribe_for_twin_updates shall return 0**]**


### amqp_device_get_twin_async
```c
extern int amqp_device_get_twin_async(AMQP_DEVICE_HANDLE handle, DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK on_device_get_twin_completed_callback, void* context);
```

**SRS_DEVICE_09_152: [**If `handle` or `on_device_get_twin_completed_callback` are NULL, amqp_device_get_twin_async shall return a non-zero result**]**

**SRS_DEVICE_09_153: [**twin_messenger_get_twin_async shall be invoked **]**

**SRS_DEVICE_09_154: [**If twin_messenger_get_twin_async fails, amqp_device_get_twin_async shall return a non-zero value**]**

**SRS_DEVICE_09_155: [**If no failures occur, amqp_device_get_twin_async shall return 0**]**


### amqp_device_subscribe_message

```c
extern int amqp_device_subscribe_message(DEVICE_HANDLE handle, ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback, void* context);
```

**SRS_DEVICE_09_066: [**If `handle` or `on_message_received_callback` or `context` is NULL, amqp_device_subscribe_message shall return a non-zero result**]**
**SRS_DEVICE_09_067: [**telemetry_messenger_subscribe_for_messages shall be invoked passing `on_messenger_message_received_callback` and the user callback and context**]**
**SRS_DEVICE_09_068: [**If telemetry_messenger_subscribe_for_messages fails, amqp_device_subscribe_message shall return a non-zero result**]**
**SRS_DEVICE_09_069: [**If no failures occur, amqp_device_subscribe_message shall return 0**]**


#### on_messenger_message_received_callback

```c
static TELEMETRY_MESSENGER_DISPOSITION_RESULT on_messenger_message_received_callback(IOTHUB_MESSAGE_HANDLE iothub_message_handle, TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
```

**SRS_DEVICE_09_070: [**If `iothub_message_handle` or `context` is NULL, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED**]**

**SRS_DEVICE_09_119: [**A DEVICE_MESSAGE_DISPOSITION_INFO instance shall be created containing a copy of `disposition_info->source` and `disposition_info->message_id`**]**

**SRS_DEVICE_09_120: [**If the DEVICE_MESSAGE_DISPOSITION_INFO instance fails to be created, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED**]**

**SRS_DEVICE_09_071: [**The user callback shall be invoked, passing the context it provided**]**
**SRS_DEVICE_09_072: [**If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED**]**
**SRS_DEVICE_09_073: [**If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED**]**
**SRS_DEVICE_09_074: [**If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED**]**

**SRS_DEVICE_09_121: [**on_messenger_message_received_callback shall release the memory allocated for DEVICE_MESSAGE_DISPOSITION_INFO**]**



### amqp_device_unsubscribe_message

```c
extern int amqp_device_unsubscribe_message(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_076: [**If `handle` is NULL, amqp_device_unsubscribe_message shall return a non-zero result**]**
**SRS_DEVICE_09_077: [**telemetry_messenger_unsubscribe_for_messages shall be invoked passing `instance->messenger_handle`**]**
**SRS_DEVICE_09_078: [**If telemetry_messenger_unsubscribe_for_messages fails, amqp_device_unsubscribe_message shall return a non-zero result**]**
**SRS_DEVICE_09_079: [**If no failures occur, amqp_device_unsubscribe_message shall return 0**]**


## amqp_device_send_message_disposition
```c
extern int amqp_device_send_message_disposition(DEVICE_HANDLE device_handle, DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info, DEVICE_MESSAGE_DISPOSITION_RESULT disposition_result);
```

**SRS_DEVICE_09_111: [**If `device_handle` or `disposition_info` are NULL, amqp_device_send_message_disposition() shall fail and return MU_FAILURE**]**

**SRS_DEVICE_09_112: [**If `disposition_info->source` is NULL, amqp_device_send_message_disposition() shall fail and return MU_FAILURE**]**

**SRS_DEVICE_09_113: [**A TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance shall be created with a copy of the `source` and `message_id` contained in `disposition_info`**]**

**SRS_DEVICE_09_114: [**If the TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO fails to be created, amqp_device_send_message_disposition() shall fail and return MU_FAILURE**]**

**SRS_DEVICE_09_115: [**`telemetry_messenger_send_message_disposition()` shall be invoked passing the TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance and the corresponding TELEMETRY_MESSENGER_DISPOSITION_RESULT**]**

**SRS_DEVICE_09_116: [**If `telemetry_messenger_send_message_disposition()` fails, amqp_device_send_message_disposition() shall fail and return MU_FAILURE**]**

**SRS_DEVICE_09_117: [**amqp_device_send_message_disposition() shall destroy the TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance**]**

**SRS_DEVICE_09_118: [**If no failures occurr, amqp_device_send_message_disposition() shall return 0**]**


### amqp_device_set_retry_policy

```c
extern int amqp_device_set_retry_policy(DEVICE_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY policy, size_t retry_timeout_limit_in_seconds);
```

Note: this API is not currently implemented or supported.

**SRS_DEVICE_09_080: [**If `handle` is NULL, amqp_device_set_retry_policy shall return a non-zero result**]**
**SRS_DEVICE_09_081: [**amqp_device_set_retry_policy shall return a non-zero result**]**


### amqp_device_set_option

```c
extern int amqp_device_set_option(DEVICE_HANDLE handle, const char* name, void* value);
```

**SRS_DEVICE_09_082: [**If `handle` or `name` or `value` are NULL, amqp_device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_083: [**If `name` refers to authentication but CBS authentication is not used, amqp_device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_084: [**If `name` refers to authentication, it shall be passed along with `value` to authentication_set_option**]**
**SRS_DEVICE_09_085: [**If authentication_set_option fails, amqp_device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_086: [**If `name` refers to messenger module, it shall be passed along with `value` to telemetry_messenger_set_option**]**
**SRS_DEVICE_09_087: [**If telemetry_messenger_set_option fails, amqp_device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_088: [**If `name` is DEVICE_OPTION_SAVED_AUTH_OPTIONS but CBS authentication is not being used, amqp_device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_089: [**If `name` is DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, `value` shall be fed to `instance->messenger_handle` using OptionHandler_FeedOptions**]**
**SRS_DEVICE_09_090: [**If `name` is DEVICE_OPTION_SAVED_OPTIONS, `value` shall be fed to `instance` using OptionHandler_FeedOptions**]**
**SRS_DEVICE_09_091: [**If any call to OptionHandler_FeedOptions fails, amqp_device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_092: [**If no failures occur, amqp_device_set_option shall return 0**]**

Note:
- Authentication-related options: DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, DEVICE_OPTION_SAS_TOKEN_REFRESH_TIME_SECS, DEVICE_OPTION_SAS_TOKEN_LIFETIME_SECS
- Messenger-related options: DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS


### amqp_device_retrieve_options

```c
extern OPTIONHANDLER_HANDLE amqp_device_retrieve_options(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_093: [**If `handle` is NULL, amqp_device_retrieve_options shall return NULL**]**

**SRS_DEVICE_09_094: [**A OPTIONHANDLER_HANDLE instance, aka `options` shall be created using OptionHandler_Create**]**
**SRS_DEVICE_09_095: [**If OptionHandler_Create fails, amqp_device_retrieve_options shall return NULL**]**
**SRS_DEVICE_09_096: [**If CBS authentication is used, `instance->authentication_handle` options shall be retrieved using authentication_retrieve_options**]**
**SRS_DEVICE_09_097: [**If authentication_retrieve_options fails, amqp_device_retrieve_options shall return NULL**]**
**SRS_DEVICE_09_098: [**The authentication options shall be added to `options` using OptionHandler_AddOption as DEVICE_OPTION_SAVED_AUTH_OPTIONS**]**
**SRS_DEVICE_09_099: [**`instance->messenger_handle` options shall be retrieved using telemetry_messenger_retrieve_options**]**
**SRS_DEVICE_09_100: [**If telemetry_messenger_retrieve_options fails, amqp_device_retrieve_options shall return NULL**]**
**SRS_DEVICE_09_101: [**The messenger options shall be added to `options` using OptionHandler_AddOption as DEVICE_OPTION_SAVED_MESSENGER_OPTIONS**]**
**SRS_DEVICE_09_102: [**If any call to OptionHandler_AddOption fails, amqp_device_retrieve_options shall return NULL**]**
**SRS_DEVICE_09_103: [**If any failure occurs, any memory allocated by amqp_device_retrieve_options shall be destroyed**]**
**SRS_DEVICE_09_104: [**If no failures occur, a handle to `options` shall be return**]**


### amqp_device_get_send_status

```c
extern int amqp_device_get_send_status(DEVICE_HANDLE handle, DEVICE_SEND_STATUS *send_status);
```

**SRS_DEVICE_09_105: [**If `handle` or `send_status` is NULL, amqp_device_get_send_status shall return a non-zero result**]**
**SRS_DEVICE_09_106: [**The status of `instance->messenger_handle` shall be obtained using telemetry_messenger_get_send_status**]**
**SRS_DEVICE_09_107: [**If telemetry_messenger_get_send_status fails, amqp_device_get_send_status shall return a non-zero result**]**
**SRS_DEVICE_09_108: [**If telemetry_messenger_get_send_status returns TELEMETRY_MESSENGER_SEND_STATUS_IDLE, amqp_device_get_send_status return status DEVICE_SEND_STATUS_IDLE**]**
**SRS_DEVICE_09_109: [**If telemetry_messenger_get_send_status returns TELEMETRY_MESSENGER_SEND_STATUS_BUSY, amqp_device_get_send_status return status DEVICE_SEND_STATUS_BUSY**]**
**SRS_DEVICE_09_110: [**If amqp_device_get_send_status succeeds, it shall return zero as result**]**
