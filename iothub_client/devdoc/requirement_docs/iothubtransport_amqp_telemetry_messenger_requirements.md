# iothubtransport_amqp_telemetry_messenger Requirements


## Overview

This module provides an abstraction for the IoTHubTransportAMQP to send telemetry events and receive command/messages.


## Dependencies

uamqp_messaging
azure_c_shared_utility
azure_uamqp_c


## Exposed API

```c
	static const char* TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS = "telemetry_event_send_timeout_secs";
	static const char* TELEMETRY_MESSENGER_OPTION_SAVED_OPTIONS = "saved_telemetry_messenger_options";

	typedef struct TELEMETRY_MESSENGER_INSTANCE* TELEMETRY_MESSENGER_HANDLE;

	typedef enum TELEMETRY_MESSENGER_SEND_STATUS_TAG
	{
		TELEMETRY_MESSENGER_SEND_STATUS_IDLE,
		TELEMETRY_MESSENGER_SEND_STATUS_BUSY
	} TELEMETRY_MESSENGER_SEND_STATUS;

	typedef enum TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_TAG
	{
		TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK,
		TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE,
		TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING,
		TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT,
		TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED
	} TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT;

	typedef enum TELEMETRY_MESSENGER_DISPOSITION_RESULT_TAG
	{
		TELEMETRY_MESSENGER_DISPOSITION_RESULT_NONE,
		TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED,
		TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED,
		TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED
	} TELEMETRY_MESSENGER_DISPOSITION_RESULT;

	typedef enum TELEMETRY_MESSENGER_STATE_TAG
	{
		TELEMETRY_MESSENGER_STATE_STARTING,
		TELEMETRY_MESSENGER_STATE_STARTED,
		TELEMETRY_MESSENGER_STATE_STOPPING,
		TELEMETRY_MESSENGER_STATE_STOPPED,
		TELEMETRY_MESSENGER_STATE_ERROR
	} TELEMETRY_MESSENGER_STATE;

	typedef struct TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO_TAG
	{
		delivery_number message_id;
		char* source;
	} TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO;

	typedef void(*ON_TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE)(IOTHUB_MESSAGE_LIST* iothub_message_list, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT messenger_event_send_complete_result, void* context);
	typedef void(*ON_TELEMETRY_MESSENGER_STATE_CHANGED_CALLBACK)(void* context, TELEMETRY_MESSENGER_STATE previous_state, TELEMETRY_MESSENGER_STATE new_state);
	typedef TELEMETRY_MESSENGER_DISPOSITION_RESULT(*ON_TELEMETRY_MESSENGER_MESSAGE_RECEIVED)(IOTHUB_MESSAGE_HANDLE message, TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context);

	typedef struct TELEMETRY_MESSENGER_CONFIG_TAG
	{
		const char* device_id;
		char* iothub_host_fqdn;
		ON_TELEMETRY_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
		void* on_state_changed_context;
	} TELEMETRY_MESSENGER_CONFIG;

	extern TELEMETRY_MESSENGER_HANDLE telemetry_messenger_create(const TELEMETRY_MESSENGER_CONFIG* messenger_config);
	extern int telemetry_messenger_send_async(TELEMETRY_MESSENGER_HANDLE messenger_handle, IOTHUB_MESSAGE_LIST* message, ON_EVENT_SEND_COMPLETE on_event_send_complete_callback, const void* context);
	extern int telemetry_messenger_subscribe_for_messages(TELEMETRY_MESSENGER_HANDLE messenger_handle, ON_MESSAGE_RECEIVED on_message_received_callback, void* context);
	extern int telemetry_messenger_unsubscribe_for_messages(TELEMETRY_MESSENGER_HANDLE messenger_handle);
	extern int telemetry_messenger_send_message_disposition(TELEMETRY_MESSENGER_HANDLE messenger_handle, TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT disposition_result);
	extern int telemetry_messenger_get_send_status(TELEMETRY_MESSENGER_HANDLE messenger_handle, TELEMETRY_MESSENGER_SEND_STATUS* send_status);
	extern int telemetry_messenger_start(TELEMETRY_MESSENGER_HANDLE messenger_handle, SESSION_HANDLE session_handle);
	extern int telemetry_messenger_stop(TELEMETRY_MESSENGER_HANDLE messenger_handle);
	extern void telemetry_messenger_do_work(TELEMETRY_MESSENGER_HANDLE messenger_handle);
	extern void telemetry_messenger_destroy(TELEMETRY_MESSENGER_HANDLE messenger_handle);
	extern int telemetry_messenger_set_option(TELEMETRY_MESSENGER_HANDLE messenger_handle, const char* name, void* value);
	extern OPTIONHANDLER_HANDLE telemetry_messenger_retrieve_options(TELEMETRY_MESSENGER_HANDLE messenger_handle);
```


## telemetry_messenger_create

```c
	extern TELEMETRY_MESSENGER_HANDLE telemetry_messenger_create(const TELEMETRY_MESSENGER_CONFIG* messenger_config);
```

Summary: creates struct instance to store data, checks and stores copies of parameters; no other components are created not started.

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_001: [**If parameter `messenger_config` is NULL, telemetry_messenger_create() shall return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_002: [**If `messenger_config->device_id` is NULL, telemetry_messenger_create() shall return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_003: [**If `messenger_config->iothub_host_fqdn` is NULL, telemetry_messenger_create() shall return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_006: [**telemetry_messenger_create() shall allocate memory for the messenger instance structure (aka `instance`)**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_007: [**If malloc() fails, telemetry_messenger_create() shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_008: [**telemetry_messenger_create() shall save a copy of `messenger_config->device_id` into `instance->device_id`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_009: [**If STRING_construct() fails, telemetry_messenger_create() shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_010: [**telemetry_messenger_create() shall save a copy of `messenger_config->iothub_host_fqdn` into `instance->iothub_host_fqdn`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_011: [**If STRING_construct() fails, telemetry_messenger_create() shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_165: [**`instance->wait_to_send_list` shall be set using singlylinkedlist_create()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_166: [**If singlylinkedlist_create() fails, telemetry_messenger_create() shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_132: [**`instance->in_progress_list` shall be set using singlylinkedlist_create()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_133: [**If singlylinkedlist_create() fails, telemetry_messenger_create() shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_013: [**`messenger_config->on_state_changed_callback` shall be saved into `instance->on_state_changed_callback`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_014: [**`messenger_config->on_state_changed_context` shall be saved into `instance->on_state_changed_context`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_015: [**If no failures occurr, telemetry_messenger_create() shall return a handle to `instance`**]**


### telemetry_messenger_send_async
```c
int telemetry_messenger_send_async(TELEMETRY_MESSENGER_HANDLE messenger_handle, IOTHUB_MESSAGE_LIST* message, ON_EVENT_SEND_COMPLETE on_event_send_complete_callback, const void* context);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_134: [**If `messenger_handle` is NULL, telemetry_messenger_send_async() shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_135: [**If `message` is NULL, telemetry_messenger_send_async() shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_136: [**If `on_event_send_complete_callback` is NULL, telemetry_messenger_send_async() shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_137: [**telemetry_messenger_send_async() shall allocate memory for a MESSENGER_SEND_EVENT_CALLER_INFORMATION structure**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_138: [**If malloc() fails, telemetry_messenger_send_async() shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_100: [**`task` shall be added to `instance->wait_to_send_list` using singlylinkedlist_add()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_139: [**If singlylinkedlist_add() fails, telemetry_messenger_send_async() shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_142: [**If any failure occurs, telemetry_messenger_send_async() shall free any memory it has allocated**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_143: [**If no failures occur, telemetry_messenger_send_async() shall return zero**]**


### telemetry_messenger_get_send_status

```c
int telemetry_messenger_get_send_status(TELEMETRY_MESSENGER_HANDLE messenger_handle, TELEMETRY_MESSENGER_SEND_STATUS* send_status);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_144: [**If `messenger_handle` is NULL, telemetry_messenger_get_send_status() shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_145: [**If `send_status` is NULL, telemetry_messenger_get_send_status() shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_147: [**If `instance->in_progress_list` and `instance->wait_to_send_list` are empty, send_status shall be set to TELEMETRY_MESSENGER_SEND_STATUS_IDLE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_148: [**Otherwise, send_status shall be set to TELEMETRY_MESSENGER_SEND_STATUS_BUSY**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_149: [**If no failures occur, telemetry_messenger_get_send_status() shall return 0**]**


## telemetry_messenger_subscribe_for_messages

```c
	extern int telemetry_messenger_subscribe_for_messages(TELEMETRY_MESSENGER_HANDLE messenger_handle, ON_MESSAGE_RECEIVED on_message_received_callback, void* context);
```

Summary: informs the messenger instance that a uAMQP messagereceiver shall be created/started (delayed, done on telemetry_messenger_do_work()), stores the callback information.

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_016: [**If `messenger_handle` is NULL, telemetry_messenger_subscribe_for_messages() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_017: [**If `instance->receive_messages` is already true, telemetry_messenger_subscribe_for_messages() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_018: [**If `on_message_received_callback` is NULL, telemetry_messenger_subscribe_for_messages() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_019: [**`on_message_received_callback` shall be saved on `instance->on_message_received_callback`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_020: [**`context` shall be saved on `instance->on_message_received_context`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_021: [**telemetry_messenger_subscribe_for_messages() shall set `instance->receive_messages` to true**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_022: [**If no failures occurr, telemetry_messenger_subscribe_for_messages() shall return 0**]**


## telemetry_messenger_unsubscribe_for_messages

```c
	extern int telemetry_messenger_unsubscribe_for_messages(TELEMETRY_MESSENGER_HANDLE messenger_handle);
```

Summary: informs the messenger instance that an existing uAMQP messagereceiver shall be stopped/destroyed (delayed, done on telemetry_messenger_do_work()).

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_023: [**If `messenger_handle` is NULL, telemetry_messenger_unsubscribe_for_messages() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_024: [**If `instance->receive_messages` is already false, telemetry_messenger_unsubscribe_for_messages() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_025: [**telemetry_messenger_unsubscribe_for_messages() shall set `instance->receive_messages` to false**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_026: [**telemetry_messenger_unsubscribe_for_messages() shall set `instance->on_message_received_callback` to NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_027: [**telemetry_messenger_unsubscribe_for_messages() shall set `instance->on_message_received_context` to NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_028: [**If no failures occurr, telemetry_messenger_unsubscribe_for_messages() shall return 0**]**


## telemetry_messenger_send_message_disposition
```c
extern int telemetry_messenger_send_message_disposition(TELEMETRY_MESSENGER_HANDLE messenger_handle, TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT disposition_result);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_179: [**If `messenger_handle` or `disposition_info` are NULL, telemetry_messenger_send_message_disposition() shall fail and return MU_FAILURE**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_180: [**If `disposition_info->source` is NULL, telemetry_messenger_send_message_disposition() shall fail and return MU_FAILURE**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_189: [**If `messenger_handle->message_receiver` is NULL, telemetry_messenger_send_message_disposition() shall fail and return MU_FAILURE**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_181: [**An AMQP_VALUE disposition result shall be created corresponding to the `disposition_result` provided**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_182: [**`messagereceiver_send_message_disposition()` shall be invoked passing `disposition_info->source`, `disposition_info->message_id` and the AMQP_VALUE disposition result**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_183: [**If `messagereceiver_send_message_disposition()` fails, telemetry_messenger_send_message_disposition() shall fail and return MU_FAILURE**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_184: [**telemetry_messenger_send_message_disposition() shall destroy the AMQP_VALUE disposition result**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_185: [**If no failures occurr, telemetry_messenger_send_message_disposition() shall return 0**]**



## telemetry_messenger_start

```c
	extern int telemetry_messenger_start(TELEMETRY_MESSENGER_HANDLE messenger_handle, SESSION_HANDLE session_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_029: [**If `messenger_handle` is NULL, telemetry_messenger_start() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_030: [**If `session_handle` is NULL, telemetry_messenger_start() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_031: [**If `instance->state` is not TELEMETRY_MESSENGER_STATE_STOPPED, telemetry_messenger_start() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_032: [**`session_handle` shall be saved on `instance->session_handle`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_115: [**If no failures occurr, `instance->state` shall be set to TELEMETRY_MESSENGER_STATE_STARTING, and `instance->on_state_changed_callback` invoked if provided**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_056: [**If no failures occurr, telemetry_messenger_start() shall return 0**]**


## telemetry_messenger_stop

```c
	extern int telemetry_messenger_stop(TELEMETRY_MESSENGER_HANDLE messenger_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_057: [**If `messenger_handle` is NULL, telemetry_messenger_stop() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_058: [**If `instance->state` is TELEMETRY_MESSENGER_STATE_STOPPED, telemetry_messenger_stop() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_116: [**`instance->state` shall be set to TELEMETRY_MESSENGER_STATE_STOPPING, and `instance->on_state_changed_callback` invoked if provided**]**



## telemetry_messenger_do_work

```c
	extern void telemetry_messenger_do_work(TELEMETRY_MESSENGER_HANDLE messenger_handle);
```

Summary: creates/destroys the uAMQP messagesender, messagereceiver according to current subscription (telemetry_messenger_subscribe_for_messages/telemetry_messenger_unsubscribe_for_messages), sends pending D2C events.

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_065: [**If `messenger_handle` is NULL, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_151: [**If `instance->state` is TELEMETRY_MESSENGER_STATE_STARTING, telemetry_messenger_do_work() shall create and open `instance->message_sender`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_152: [**If `instance->state` is TELEMETRY_MESSENGER_STATE_STOPPING, telemetry_messenger_do_work() shall close and destroy `instance->message_sender` and `instance->message_receiver`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_162: [**If `instance->state` is TELEMETRY_MESSENGER_STATE_STOPPING, telemetry_messenger_do_work() shall move all items from `instance->in_progress_list` to the beginning of `instance->wait_to_send_list`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_163: [**If not all items from `instance->in_progress_list` can be moved back to `instance->wait_to_send_list`, `instance->state` shall be set to TELEMETRY_MESSENGER_STATE_ERROR, and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_164: [**If all items get successfuly moved back to `instance->wait_to_send_list`, `instance->state` shall be set to TELEMETRY_MESSENGER_STATE_STOPPED, and `instance->on_state_changed_callback` invoked**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_066: [**If `instance->state` is not TELEMETRY_MESSENGER_STATE_STARTED, telemetry_messenger_do_work() shall return**]**


### Create/Open the message sender

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_033: [**A variable, named `devices_and_modules_path`, shall be created concatenating `instance->iothub_host_fqdn`, "/devices/" and `instance->device_id` (and "/modules/" and `instance->module_id` if modules are present)**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_034: [**If `devices_path` fails to be created, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_035: [**A variable, named `event_send_address`, shall be created concatenating "amqps://", `devices_path` and "/messages/events"**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_036: [**If `event_send_address` fails to be created, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_037: [**A `link_name` variable shall be created using an unique string label per AMQP session**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_038: [**If `link_name` fails to be created, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_039: [**A `source` variable shall be created with messaging_create_source() using an unique string label per AMQP session**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_040: [**If `source` fails to be created, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_041: [**A `target` variable shall be created with messaging_create_target() using `event_send_address`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_042: [**If `target` fails to be created, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_043: [**`instance->sender_link` shall be set using link_create(), passing `instance->session_handle`, `link_name`, "role_sender", `source` and `target` as parameters**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_044: [**If link_create() fails, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_047: [**`instance->sender_link` maximum message size shall be set to UINT64_MAX using link_set_max_message_size()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_048: [**If link_set_max_message_size() fails, it shall be logged and ignored.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_049: [**`instance->sender_link` should have a property "com.microsoft:client-version" set as `CLIENT_DEVICE_TYPE_PREFIX/IOTHUB_SDK_VERSION`, using amqpvalue_set_map_value() and link_set_attach_properties()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_050: [**If amqpvalue_set_map_value() or link_set_attach_properties() fail, the failure shall be ignored**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_051: [**`instance->message_sender` shall be created using messagesender_create(), passing the `instance->sender_link` and `on_event_sender_state_changed_callback`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_052: [**If messagesender_create() fails, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_053: [**`instance->message_sender` shall be opened using messagesender_open()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_054: [**If messagesender_open() fails, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_055: [**Before returning, telemetry_messenger_do_work() shall release all the temporary memory it has allocated**]**


#### on_event_sender_state_changed_callback

```c
static void on_event_sender_state_changed_callback(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_118: [**If the messagesender new state is MESSAGE_SENDER_STATE_OPEN, `instance->state` shall be set to TELEMETRY_MESSENGER_STATE_STARTED, and `instance->on_state_changed_callback` invoked if provided**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_119: [**If the messagesender new state is MESSAGE_SENDER_STATE_ERROR, `instance->state` shall be set to TELEMETRY_MESSENGER_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided**]**


### Destroy the message sender
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_060: [**`instance->message_sender` shall be destroyed using messagesender_destroy()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_061: [**`instance->message_receiver` shall be closed using messagereceiver_close()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_062: [**`instance->message_receiver` shall be destroyed using messagereceiver_destroy()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_063: [**`instance->sender_link` shall be destroyed using link_destroy()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_064: [**`instance->receiver_link` shall be destroyed using link_destroy()**]**


### Create a message receiver

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_067: [**If `instance->receive_messages` is true and `instance->message_receiver` is NULL, a message_receiver shall be created**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_068: [**A variable, named `devices_and_modules_path`, shall be created concatenating `instance->iothub_host_fqdn`, "/devices/" and `instance->device_id` (and "/modules/" and `instance->module_id` if modules are present)**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_069: [**If `devices_path` fails to be created, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_070: [**A variable, named `message_receive_address`, shall be created concatenating "amqps://", `devices_path` and "/messages/devicebound"**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_071: [**If `message_receive_address` fails to be created, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_072: [**A `link_name` variable shall be created using an unique string label per AMQP session**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_073: [**If `link_name` fails to be created, telemetry_messenger_do_work() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_074: [**A `target` variable shall be created with messaging_create_target() using an unique string label per AMQP session**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_075: [**If `target` fails to be created, telemetry_messenger_do_work() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_076: [**A `source` variable shall be created with messaging_create_source() using `message_receive_address`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_077: [**If `source` fails to be created, telemetry_messenger_do_work() shall fail and return MU_FAILURE**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_078: [**`instance->receiver_link` shall be set using link_create(), passing `instance->session_handle`, `link_name`, "role_receiver", `source` and `target` as parameters**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_079: [**If link_create() fails, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_080: [**`instance->receiver_link` settle mode shall be set to "receiver_settle_mode_first" using link_set_rcv_settle_mode(), **]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_081: [**If link_set_rcv_settle_mode() fails, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_082: [**`instance->receiver_link` maximum message size shall be set to 65536 using link_set_max_message_size()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_083: [**If link_set_max_message_size() fails, it shall be logged and ignored.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_084: [**`instance->receiver_link` should have a property "com.microsoft:client-version" set as `CLIENT_DEVICE_TYPE_PREFIX/IOTHUB_SDK_VERSION`, using amqpvalue_set_map_value() and link_set_attach_properties()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_085: [**If amqpvalue_set_map_value() or link_set_attach_properties() fail, the failure shall be ignored**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_086: [**`instance->message_receiver` shall be created using messagereceiver_create(), passing the `instance->receiver_link` and `on_messagereceiver_state_changed_callback`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_087: [**If messagereceiver_create() fails, telemetry_messenger_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_088: [**`instance->message_receiver` shall be opened using messagereceiver_open(), passing `on_message_received_internal_callback`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_089: [**If messagereceiver_open() fails, telemetry_messenger_do_work() shall fail and return**]**


#### on_messagereceiver_state_changed_callback

```c
static void on_message_receiver_state_changed(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_120: [**If the messagereceiver new state is MESSAGE_RECEIVER_STATE_ERROR, `instance->state` shall be set to TELEMETRY_MESSENGER_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided**]**


#### on_message_received_internal_callback

```c
static AMQP_VALUE on_message_received_internal_callback(const void* context, MESSAGE_HANDLE message)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_121: [**An IOTHUB_MESSAGE_HANDLE shall be obtained from MESSAGE_HANDLE using message_create_IoTHubMessage_from_uamqp_message()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_122: [**If message_create_IoTHubMessage_from_uamqp_message() fails, on_message_received_internal_callback shall return the result of messaging_delivery_rejected()**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_186: [**A TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance shall be created containing the source link name and message delivery ID**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_187: [**If the TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance fails to be created, on_message_received_internal_callback shall return messaging_delivery_released()**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_123: [**`instance->on_message_received_callback` shall be invoked passing the IOTHUB_MESSAGE_HANDLE and TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_188: [**The memory allocated for the TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance shall be released**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_125: [**If `instance->on_message_received_callback` returns TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED, on_message_received_internal_callback shall return the result of messaging_delivery_accepted()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_126: [**If `instance->on_message_received_callback` returns TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED, on_message_received_internal_callback shall return the result of messaging_delivery_released()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_127: [**If `instance->on_message_received_callback` returns TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED, on_message_received_internal_callback shall return the result of messaging_delivery_rejected()**]**


### Destroy the message receiver

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_092: [**If `instance->receive_messages` is false and `instance->message_receiver` is not NULL, it shall be destroyed**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_093: [**`instance->message_receiver` shall be closed using messagereceiver_close()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_094: [**If messagereceiver_close() fails, it shall be logged and ignored**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_095: [**`instance->message_receiver` shall be destroyed using messagereceiver_destroy()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_096: [**`instance->message_receiver` shall be set to NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_097: [**`instance->receiver_link` shall be destroyed using link_destroy()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_098: [**`instance->receiver_link` shall be set to NULL**]**


### Send pending events

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_161: [**If telemetry_messenger_do_work() fail sending events for `instance->event_send_retry_limit` times in a row, it shall invoke `instance->on_state_changed_callback`, if provided, with error code TELEMETRY_MESSENGER_STATE_ERROR**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_192: [**Enumerate through all messages waiting to send, building up AMQP message to send and sending when size will be greater than link max size.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_193: [**If (length of current user AMQP message) + (length of user messages pending for this batched message) + (1KB reserve buffer) > maximum link send, send pending messages and create new batched message.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_194: [**When message is ready to send, invoke AMQP's `messagesender_send` and free temporary values associated with this batch.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_195: [**Append the current message's encoded data to the batched message tracked by uAMQP layer.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_197: [**If a single message is greater than our maximum AMQP send size, the message will be ignored.  Invoke the callback but continue send loop; this is NOT a fatal error.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_198: [**While processing pending messages, errors shall result in user callback being invoked.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_199: [**Errors specific to a message (e.g. failure to encode) are NOT fatal but we'll keep processing.  More general errors (e.g. out of memory) will stop processing.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_200: [**Retrieve an AMQP encoded representation of this message for later appending to main batched message.  On error, invoke callback but continue send loop; this is NOT a fatal error.**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_201: [**If message_create_uamqp_encoding_from_iothub_message fails, invoke callback with TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE**]**

#### internal_on_event_send_complete_callback
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_128: [**`task` shall be removed from `instance->in_progress_list`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_130: [**`task` shall be destroyed()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_201: [**Freeing a `task` will free callback items associated with it and free the data itself**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_189: [**If no failure occurs, `on_event_send_complete_callback` shall be invoked with result TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK for all callers associated with this task**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_31_190: [**If a failure occured, `on_event_send_complete_callback` shall be invoked with result TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING for all callers associated with this task**]**



NOTE: the IOTHUB_MESSAGE_HANDLE must be destroyed by the upper layer, it is not freed here since this module doesn't own (i.e., create) it.


## telemetry_messenger_destroy

```c
	extern void telemetry_messenger_destroy(TELEMETRY_MESSENGER_HANDLE messenger_handle);
```

Summary: stops the messenger if needed, destroys all the components within the messenger data structure (messenger_handle) and releases any allocated memory.

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_109: [**If `messenger_handle` is NULL, telemetry_messenger_destroy() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_110: [**If the `instance->state` is not TELEMETRY_MESSENGER_STATE_STOPPED, telemetry_messenger_destroy() shall invoke telemetry_messenger_stop() and telemetry_messenger_do_work() once**]**

Note: when events are moved from wait_to_send_list to in_progress_list, they are moved from beginning (oldest element) to end (newest).
Due to the logic used by telemetry_messenger_do_work(), all elements of in_progress_list are always older than any current element on wait_to_send_list.
In that case, when they are rolled back, they need to go to the beginning of the wait_to_send_list.

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_111: [**All elements of `instance->in_progress_list` and `instance->wait_to_send_list` shall be removed, invoking `task->on_event_send_complete_callback` for each with EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_150: [**`instance->in_progress_list` and `instance->wait_to_send_list` shall be destroyed using singlylinkedlist_destroy()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_112: [**`instance->iothub_host_fqdn` shall be destroyed using STRING_delete()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_113: [**`instance->device_id` shall be destroyed using STRING_delete()**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_114: [**telemetry_messenger_destroy() shall destroy `instance` with free()**]**


## telemetry_messenger_set_option

```c
	extern int telemetry_messenger_set_option(TELEMETRY_MESSENGER_HANDLE messenger_handle, const char* name, void* value);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_167: [**If `messenger_handle` or `name` or `value` is NULL, telemetry_messenger_set_option shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_168: [**If name matches TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, `value` shall be saved on `instance->event_send_timeout_secs`**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_169: [**If name matches TELEMETRY_MESSENGER_OPTION_SAVED_OPTIONS, `value` shall be applied using OptionHandler_FeedOptions**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_170: [**If OptionHandler_FeedOptions fails, telemetry_messenger_set_option shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_171: [**If no errors occur, telemetry_messenger_set_option shall return 0**]**


## telemetry_messenger_retrieve_options

```c
	extern OPTIONHANDLER_HANDLE telemetry_messenger_retrieve_options(TELEMETRY_MESSENGER_HANDLE messenger_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_172: [**If `messenger_handle` is NULL, telemetry_messenger_retrieve_options shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_173: [**An OPTIONHANDLER_HANDLE instance shall be created using OptionHandler_Create**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_174: [**If an OPTIONHANDLER_HANDLE instance fails to be created, telemetry_messenger_retrieve_options shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_175: [**Each option of `instance` shall be added to the OPTIONHANDLER_HANDLE instance using OptionHandler_AddOption**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_176: [**If OptionHandler_AddOption fails, telemetry_messenger_retrieve_options shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_177: [**If telemetry_messenger_retrieve_options fails, any allocated memory shall be freed**]**
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_178: [**If no failures occur, telemetry_messenger_retrieve_options shall return the OPTIONHANDLER_HANDLE instance**]**
