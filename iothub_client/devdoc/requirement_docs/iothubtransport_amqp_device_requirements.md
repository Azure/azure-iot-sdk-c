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
iothubtransport_amqp_messenger


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
	DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED,
	DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED,
	DEVICE_MESSAGE_DISPOSITION_RESULT_ABANDONED
} DEVICE_MESSAGE_DISPOSITION_RESULT;

typedef void(*ON_DEVICE_STATE_CHANGED)(void* context, DEVICE_STATE previous_state, DEVICE_STATE new_state);
typedef DEVICE_MESSAGE_DISPOSITION_RESULT(*ON_DEVICE_C2D_MESSAGE_RECEIVED)(IOTHUB_MESSAGE_HANDLE message, void* context);
typedef void(*ON_DEVICE_D2C_EVENT_SEND_COMPLETE)(IOTHUB_MESSAGE_LIST* message, D2C_EVENT_SEND_RESULT result, void* context);

typedef struct DEVICE_CONFIG_TAG
{
	char* device_id;
	char* iothub_host_fqdn;
	DEVICE_AUTH_MODE authentication_mode;
	ON_DEVICE_STATE_CHANGED on_state_changed_callback;
	void* on_state_changed_context;

	char* device_primary_key;
	char* device_secondary_key;
	char* device_sas_token;
} DEVICE_CONFIG;

typedef struct DEVICE_INSTANCE* DEVICE_HANDLE;

extern DEVICE_HANDLE device_create(DEVICE_CONFIG* config);
extern void device_destroy(DEVICE_HANDLE handle);
extern int device_start_async(DEVICE_HANDLE handle, SESSION_HANDLE session_handle, CBS_HANDLE cbs_handle);
extern int device_stop(DEVICE_HANDLE handle);
extern void device_do_work(DEVICE_HANDLE handle);
extern int device_send_event_async(DEVICE_HANDLE handle, IOTHUB_MESSAGE_LIST* message, ON_DEVICE_D2C_EVENT_SEND_COMPLETE on_device_d2c_event_send_complete_callback, void* context);
extern int device_get_send_status(DEVICE_HANDLE handle, DEVICE_SEND_STATUS *send_status);
extern int device_subscribe_message(DEVICE_HANDLE handle, ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback, void* context);
extern int device_unsubscribe_message(DEVICE_HANDLE handle);
extern int device_set_retry_policy(DEVICE_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY policy, size_t retry_timeout_limit_in_seconds);
extern int device_set_option(DEVICE_HANDLE handle, const char* name, void* value);
extern OPTIONHANDLER_HANDLE device_retrieve_options(DEVICE_HANDLE handle);

```

Note: `instance` refers to the structure that holds the current state and control parameters of the device. 
In each function (other than amqp_device_create) it shall derive from the AMQP_DEVICE_HANDLE handle passed as argument.  


### device_create

```c
extern DEVICE_HANDLE device_create(DEVICE_CONFIG config);
```

**SRS_DEVICE_09_001: [**If `config` or device_id or iothub_host_fqdn or on_state_changed_callback are NULL then device_create shall fail and return NULL**]**
**SRS_DEVICE_09_002: [**device_create shall allocate memory for the device instance structure**]**
**SRS_DEVICE_09_003: [**If malloc fails, device_create shall fail and return NULL**]**
**SRS_DEVICE_09_004: [**All `config` parameters shall be saved into `instance`**]**
**SRS_DEVICE_09_005: [**If any `config` parameters fail to be saved into `instance`, device_create shall fail and return NULL**]**
**SRS_DEVICE_09_006: [**If `instance->authentication_mode` is DEVICE_AUTH_MODE_CBS, `instance->authentication_handle` shall be set using authentication_create()**]**
**SRS_DEVICE_09_007: [**If the AUTHENTICATION_HANDLE fails to be created, device_create shall fail and return NULL**]**
**SRS_DEVICE_09_008: [**`instance->messenger_handle` shall be set using messenger_create()**]**
**SRS_DEVICE_09_009: [**If the MESSENGER_HANDLE fails to be created, device_create shall fail and return NULL**]**
**SRS_DEVICE_09_010: [**If device_create fails it shall release all memory it has allocated**]**
**SRS_DEVICE_09_011: [**If device_create succeeds it shall return a handle to its `instance` structure**]**


### device_destroy

```c
extern void device_destroy(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_012: [**If `handle` is NULL, device_destroy shall return**]**
**SRS_DEVICE_09_013: [**If the device is in state DEVICE_STATE_STARTED or DEVICE_STATE_STARTING, device_stop() shall be invoked**]**
**SRS_DEVICE_09_014: [**`instance->messenger_handle shall be destroyed using messenger_destroy()`**]**
**SRS_DEVICE_09_015: [**If created, `instance->authentication_handle` shall be destroyed using authentication_destroy()`**]**
**SRS_DEVICE_09_016: [**The contents of `instance->config` shall be detroyed and then it shall be freed**]**


### device_start_async
 
```c
extern int device_start_async(DEVICE_HANDLE handle, SESSION_HANDLE session_handle, CBS_HANDLE cbs_handle);
```

**SRS_DEVICE_09_017: [**If `handle` is NULL, device_start_async shall return a non-zero result**]**
**SRS_DEVICE_09_018: [**If the device state is not DEVICE_STATE_STOPPED, device_start_async shall return a non-zero result**]**
**SRS_DEVICE_09_019: [**If `session_handle` is NULL, device_start_async shall return a non-zero result**]**
**SRS_DEVICE_09_020: [**If using CBS authentication and `cbs_handle` is NULL, device_start_async shall return a non-zero result**]**
**SRS_DEVICE_09_021: [**`session_handle` and `cbs_handle` shall be saved into the `instance`**]**
**SRS_DEVICE_09_022: [**The device state shall be updated to DEVICE_STATE_STARTING, and state changed callback invoked**]**
**SRS_DEVICE_09_023: [**If no failures occur, device_start_async shall return 0**]**

 
### device_stop
 
```c
extern int device_stop(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_024: [**If `handle` is NULL, device_stop shall return a non-zero result**]**
**SRS_DEVICE_09_025: [**If the device state is already DEVICE_STATE_STOPPED or DEVICE_STATE_STOPPING, device_stop shall return a non-zero result**]**
**SRS_DEVICE_09_026: [**The device state shall be updated to DEVICE_STATE_STOPPING, and state changed callback invoked**]**
**SRS_DEVICE_09_027: [**If `instance->messenger_handle` state is not MESSENGER_STATE_STOPPED, messenger_stop shall be invoked**]**
**SRS_DEVICE_09_028: [**If messenger_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_MSG and the function shall return non-zero result**]**
**SRS_DEVICE_09_029: [**If CBS authentication is used, if `instance->authentication_handle` state is not AUTHENTICATION_STATE_STOPPED, authentication_stop shall be invoked**]**
**SRS_DEVICE_09_030: [**If authentication_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_AUTH and the function shall return non-zero result**]**
**SRS_DEVICE_09_031: [**The device state shall be updated to DEVICE_STATE_STOPPED, and state changed callback invoked**]**
**SRS_DEVICE_09_032: [**If no failures occur, device_stop shall return 0**]**


### device_do_work
 
```c
extern void device_do_work(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_033: [**If `handle` is NULL, device_do_work shall return**]**

#### device state DEVICE_STATE_STARTING

##### Starting authentication instance

**SRS_DEVICE_09_034: [**If CBS authentication is used and authentication state is AUTHENTICATION_STATE_STOPPED, authentication_start shall be invoked**]**
**SRS_DEVICE_09_035: [**If authentication_start fails, the device state shall be updated to DEVICE_STATE_ERROR_AUTH**]**
**SRS_DEVICE_09_036: [**If authentication state is AUTHENTICATION_STATE_STARTING, the device shall track the time since last event change and timeout if needed**]**
**SRS_DEVICE_09_037: [**If authentication_start times out, the device state shall be updated to DEVICE_STATE_ERROR_AUTH_TIMEOUT**]**
**SRS_DEVICE_09_038: [**If authentication state is AUTHENTICATION_STATE_ERROR and error code is AUTH_FAILED, the device state shall be updated to DEVICE_STATE_ERROR_AUTH**]**
**SRS_DEVICE_09_039: [**If authentication state is AUTHENTICATION_STATE_ERROR and error code is TIMEOUT, the device state shall be updated to DEVICE_STATE_ERROR_AUTH_TIMEOUT**]**


##### Starting messenger instance

**SRS_DEVICE_09_040: [**Messenger shall not be started if using CBS authentication and authentication start has not completed yet**]**
**SRS_DEVICE_09_041: [**If messenger state is MESSENGER_STATE_STOPPED, messenger_start shall be invoked**]**
**SRS_DEVICE_09_042: [**If messenger_start fails, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_043: [**If messenger state is MESSENGER_STATE_STARTING, the device shall track the time since last event change and timeout if needed**]**
**SRS_DEVICE_09_044: [**If messenger_start times out, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_045: [**If messenger state is MESSENGER_STATE_ERROR, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**
**SRS_DEVICE_09_046: [**If messenger state is MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_STARTED**]**


#### device state DEVICE_STATE_STARTED

**SRS_DEVICE_09_047: [**If CBS authentication is used and authentication state is not AUTHENTICATION_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_AUTH**]**
**SRS_DEVICE_09_048: [**If messenger state is not MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_MSG**]**


#### Any device state

**SRS_DEVICE_09_049: [**If CBS is used for authentication and `instance->authentication_handle` state is not STOPPED or ERROR, authentication_do_work shall be invoked**]**
**SRS_DEVICE_09_050: [**If `instance->messenger_handle` state is not STOPPED or ERROR, authentication_do_work shall be invoked**]**



### device_send_event_async
 
```c
extern int device_send_event_async(DEVICE_HANDLE handle, IOTHUB_MESSAGE_LIST* message, ON_DEVICE_D2C_EVENT_SEND_COMPLETE on_device_d2c_event_send_complete_callback, void* context);
```

**SRS_DEVICE_09_051: [**If `handle` are `message` are NULL, device_send_event_async shall return a non-zero result**]**
**SRS_DEVICE_09_052: [**A structure (`send_task`) shall be created to track the send state of the message**]**
**SRS_DEVICE_09_053: [**If `send_task` fails to be created, device_send_event_async shall return a non-zero value**]**
**SRS_DEVICE_09_054: [**`send_task` shall contain the user callback and the context provided**]**
**SRS_DEVICE_09_055: [**The message shall be sent using messenger_send_async, passing `on_event_send_complete_messenger_callback` and `send_task`**]**
**SRS_DEVICE_09_056: [**If messenger_send_async fails, device_send_event_async shall return a non-zero value**]**
**SRS_DEVICE_09_057: [**If any failures occur, device_send_event_async shall release all memory it has allocated**]**
**SRS_DEVICE_09_058: [**If no failures occur, device_send_event_async shall return 0**]**


#### on_event_send_complete_messenger_callback

```c
static void on_event_send_complete_messenger_callback(IOTHUB_MESSAGE_LIST* iothub_message, MESSENGER_EVENT_SEND_COMPLETE_RESULT ev_send_comp_result, void* context)
```

**SRS_DEVICE_09_059: [**If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, D2C_EVENT_SEND_COMPLETE_RESULT_OK shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_060: [**If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_061: [**If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_062: [**If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_063: [**If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED, D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED shall be reported as `event_send_complete`**]**
**SRS_DEVICE_09_064: [**If provided, the user callback and context saved in `send_task` shall be invoked passing the device `event_send_complete`**]**
**SRS_DEVICE_09_065: [**The memory allocated for `send_task` shall be released**]**


### device_subscribe_message
 
```c
extern int device_subscribe_message(DEVICE_HANDLE handle, ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback, void* context);
```

**SRS_DEVICE_09_066: [**If `handle` or `on_message_received_callback` or `context` is NULL, device_subscribe_message shall return a non-zero result**]**
**SRS_DEVICE_09_067: [**messenger_subscribe_for_messages shall be invoked passing `on_messenger_message_received_callback` and the user callback and context**]**
**SRS_DEVICE_09_068: [**If messenger_subscribe_for_messages fails, device_subscribe_message shall return a non-zero result**]**
**SRS_DEVICE_09_069: [**If no failures occur, device_subscribe_message shall return 0**]**


#### on_messenger_message_received_callback

```c
static MESSENGER_DISPOSITION_RESULT on_messenger_message_received_callback(IOTHUB_MESSAGE_HANDLE iothub_message_handle, void* context)
```

**SRS_DEVICE_09_070: [**If `iothub_message_handle` or `context` is NULL, on_messenger_message_received_callback shall return MESSENGER_DISPOSITION_RESULT_ABANDONED**]**
**SRS_DEVICE_09_071: [**The user callback shall be invoked, passing the context it provided**]**
**SRS_DEVICE_09_072: [**If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED, on_messenger_message_received_callback shall return MESSENGER_DISPOSITION_RESULT_ACCEPTED**]**
**SRS_DEVICE_09_073: [**If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED, on_messenger_message_received_callback shall return MESSENGER_DISPOSITION_RESULT_REJECTED**]**
**SRS_DEVICE_09_074: [**If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_ABANDONED, on_messenger_message_received_callback shall return MESSENGER_DISPOSITION_RESULT_ABANDONED**]**


### device_unsubscribe_message
 
```c
extern int device_unsubscribe_message(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_076: [**If `handle` is NULL, device_unsubscribe_message shall return a non-zero result**]**
**SRS_DEVICE_09_077: [**messenger_unsubscribe_for_messages shall be invoked passing `instance->messenger_handle`**]**
**SRS_DEVICE_09_078: [**If messenger_unsubscribe_for_messages fails, device_unsubscribe_message shall return a non-zero result**]**
**SRS_DEVICE_09_079: [**If no failures occur, device_unsubscribe_message shall return 0**]**


### device_set_retry_policy
 
```c
extern int device_set_retry_policy(DEVICE_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY policy, size_t retry_timeout_limit_in_seconds);
```

Note: this API is not currently implemented or supported.

**SRS_DEVICE_09_080: [**If `handle` is NULL, device_set_retry_policy shall return a non-zero result**]**
**SRS_DEVICE_09_081: [**device_set_retry_policy shall return a non-zero result**]**


### device_set_option
 
```c
extern int device_set_option(DEVICE_HANDLE handle, const char* name, void* value);
```

**SRS_DEVICE_09_082: [**If `handle` or `name` or `value` are NULL, device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_083: [**If `name` refers to authentication but CBS authentication is not used, device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_084: [**If `name` refers to authentication, it shall be passed along with `value` to authentication_set_option**]**
**SRS_DEVICE_09_085: [**If authentication_set_option fails, device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_086: [**If `name` refers to messenger module, it shall be passed along with `value` to messenger_set_option**]**
**SRS_DEVICE_09_087: [**If messenger_set_option fails, device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_088: [**If `name` is DEVICE_OPTION_SAVED_AUTH_OPTIONS but CBS authentication is not being used, device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_089: [**If `name` is DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, `value` shall be fed to `instance->messenger_handle` using OptionHandler_FeedOptions**]**
**SRS_DEVICE_09_090: [**If `name` is DEVICE_OPTION_SAVED_OPTIONS, `value` shall be fed to `instance` using OptionHandler_FeedOptions**]**
**SRS_DEVICE_09_091: [**If any call to OptionHandler_FeedOptions fails, device_set_option shall return a non-zero result**]**
**SRS_DEVICE_09_092: [**If no failures occur, device_set_option shall return 0**]**

Note: 
- Authentication-related options: DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, DEVICE_OPTION_SAS_TOKEN_REFRESH_TIME_SECS, DEVICE_OPTION_SAS_TOKEN_LIFETIME_SECS
- Messenger-related options: DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS


### device_retrieve_options
 
```c
extern OPTIONHANDLER_HANDLE device_retrieve_options(DEVICE_HANDLE handle);
```

**SRS_DEVICE_09_093: [**If `handle` is NULL, device_retrieve_options shall return NULL**]**

**SRS_DEVICE_09_094: [**A OPTIONHANDLER_HANDLE instance, aka `options` shall be created using OptionHandler_Create**]**
**SRS_DEVICE_09_095: [**If OptionHandler_Create fails, device_retrieve_options shall return NULL**]**
**SRS_DEVICE_09_096: [**If CBS authentication is used, `instance->authentication_handle` options shall be retrieved using authentication_retrieve_options**]**
**SRS_DEVICE_09_097: [**If authentication_retrieve_options fails, device_retrieve_options shall return NULL**]**
**SRS_DEVICE_09_098: [**The authentication options shall be added to `options` using OptionHandler_AddOption as DEVICE_OPTION_SAVED_AUTH_OPTIONS**]**
**SRS_DEVICE_09_099: [**`instance->messenger_handle` options shall be retrieved using messenger_retrieve_options**]**
**SRS_DEVICE_09_100: [**If messenger_retrieve_options fails, device_retrieve_options shall return NULL**]**
**SRS_DEVICE_09_101: [**The messenger options shall be added to `options` using OptionHandler_AddOption as DEVICE_OPTION_SAVED_MESSENGER_OPTIONS**]**
**SRS_DEVICE_09_102: [**If any call to OptionHandler_AddOption fails, device_retrieve_options shall return NULL**]**
**SRS_DEVICE_09_103: [**If any failure occurs, any memory allocated by device_retrieve_options shall be destroyed**]**
**SRS_DEVICE_09_104: [**If no failures occur, a handle to `options` shall be return**]**


### device_get_send_status

```c
extern int device_get_send_status(DEVICE_HANDLE handle, DEVICE_SEND_STATUS *send_status);
```

**SRS_DEVICE_09_105: [**If `handle` or `send_status` is NULL, device_get_send_status shall return a non-zero result**]**
**SRS_DEVICE_09_106: [**The status of `instance->messenger_handle` shall be obtained using messenger_get_send_status**]**
**SRS_DEVICE_09_107: [**If messenger_get_send_status fails, device_get_send_status shall return a non-zero result**]**
**SRS_DEVICE_09_108: [**If messenger_get_send_status returns MESSENGER_SEND_STATUS_IDLE, device_get_send_status return status DEVICE_SEND_STATUS_IDLE**]**
**SRS_DEVICE_09_109: [**If messenger_get_send_status returns MESSENGER_SEND_STATUS_BUSY, device_get_send_status return status DEVICE_SEND_STATUS_BUSY**]**
**SRS_DEVICE_09_110: [**If device_get_send_status succeeds, it shall return zero as result**]**
