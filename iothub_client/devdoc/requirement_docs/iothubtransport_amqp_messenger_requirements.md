# iothubtransport_amqp_messenger Requirements

Aka iothubtransport_amqp_messenger

## Overview

This module provides an abstraction for the IoTHubTransportAMQP to send and receive AMQP messages.  


## Dependencies

azure_c_shared_utility
azure_uamqp_c

   
## Exposed API

```c
	typedef struct AMQP_MESSENGER_INSTANCE* AMQP_MESSENGER_HANDLE;

	typedef enum AMQP_MESSENGER_SEND_STATUS
	{
		AMQP_MESSENGER_SEND_STATUS_IDLE,
		AMQP_MESSENGER_SEND_STATUS_BUSY
	} AMQP_MESSENGER_SEND_STATUS;

	typedef enum AMQP_MESSENGER_SEND_RESULT_TAG
	{
		AMQP_MESSENGER_SEND_RESULT_SUCCESS,
		AMQP_MESSENGER_SEND_RESULT_ERROR,
		AMQP_MESSENGER_SEND_RESULT_CANCELLED
	} AMQP_MESSENGER_SEND_RESULT;

	typedef enum AMQP_MESSENGER_REASON_TAG
	{
		AMQP_MESSENGER_REASON_NONE,
		AMQP_MESSENGER_REASON_CANNOT_PARSE,
		AMQP_MESSENGER_REASON_FAIL_SENDING,
		AMQP_MESSENGER_REASON_TIMEOUT,
		AMQP_MESSENGER_REASON_MESSENGER_DESTROYED
	} AMQP_MESSENGER_REASON;

	typedef enum AMQP_MESSENGER_DISPOSITION_RESULT_TAG
	{
		AMQP_MESSENGER_DISPOSITION_RESULT_NONE,
		AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED,
		AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED,
		AMQP_MESSENGER_DISPOSITION_RESULT_RELEASED 
	}
	AMQP_MESSENGER_DISPOSITION_RESULT;

	typedef enum AMQP_MESSENGER_STATE
	{
		AMQP_MESSENGER_STATE_STARTING,
		AMQP_MESSENGER_STATE_STARTED,
		AMQP_MESSENGER_STATE_STOPPING,
		AMQP_MESSENGER_STATE_STOPPED,
		AMQP_MESSENGER_STATE_ERROR
	} AMQP_MESSENGER_STATE;

	typedef struct AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO_TAG
	{
		delivery_number message_id;
		char* source;
	} AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO;

	typedef void(*AMQP_MESSENGER_SEND_COMPLETE_CALLBACK)(AMQP_MESSENGER_SEND_RESULT result, AMQP_MESSENGER_REASON reason, void* context);
	typedef void(*AMQP_MESSENGER_STATE_CHANGED_CALLBACK)(void* context, AMQP_MESSENGER_STATE previous_state, AMQP_MESSENGER_STATE new_state);
	typedef AMQP_MESSENGER_DISPOSITION_RESULT(*ON_AMQP_MESSENGER_MESSAGE_RECEIVED)(MESSAGE_HANDLE message, AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context);

	typedef struct AMQP_MESSENGER_LINK_CONFIG_TAG
	{
		/**
		* @brief Sample format: "/messages/devicebound"
		*/
		const char* source_suffix;

		/**
		* @brief Sample format: "/messages/events"
		*/
		const char* target_suffix;
		
		receiver_settle_mode rcv_settle_mode;
		sender_settle_mode snd_settle_mode;
		
		MAP_HANDLE attach_properties;
	} AMQP_MESSENGER_LINK_CONFIG;

	typedef struct AMQP_MESSENGER_CONFIG_TAG
	{
		const char* client_version;
		const char* device_id;
		const char* iothub_host_fqdn;

		AMQP_MESSENGER_LINK_CONFIG send_link;
		AMQP_MESSENGER_LINK_CONFIG receive_link;

		AMQP_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
		void* on_state_changed_context;
	} AMQP_MESSENGER_CONFIG;

	AMQP_MESSENGER_HANDLE amqp_messenger_create(const AMQP_MESSENGER_CONFIG* messenger_config);
	int amqp_messenger_send_async(AMQP_MESSENGER_HANDLE messenger_handle, MESSAGE_HANDLE message, AMQP_MESSENGER_SEND_COMPLETE_CALLBACK on_messenger_event_send_complete_callback, void* context);
	int amqp_messenger_subscribe_for_messages(AMQP_MESSENGER_HANDLE messenger_handle, ON_AMQP_MESSENGER_MESSAGE_RECEIVED on_message_received_callback, void* context);
	int amqp_messenger_unsubscribe_for_messages(AMQP_MESSENGER_HANDLE messenger_handle);
	int amqp_messenger_send_message_disposition(AMQP_MESSENGER_HANDLE messenger_handle, AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT disposition_result);
	int amqp_messenger_get_send_status, AMQP_MESSENGER_HANDLE, messenger_handle, AMQP_MESSENGER_SEND_STATUS*, send_status);
	int amqp_messenger_start(AMQP_MESSENGER_HANDLE messenger_handle, SESSION_HANDLE session_handle);
	int amqp_messenger_stop(AMQP_MESSENGER_HANDLE messenger_handle);
	void amqp_messenger_do_work(AMQP_MESSENGER_HANDLE messenger_handle);
	void amqp_messenger_destroy(AMQP_MESSENGER_HANDLE messenger_handle);
	int amqp_messenger_set_option(AMQP_MESSENGER_HANDLE messenger_handle, const char* name, void* value);
	OPTIONHANDLER_HANDLE amqp_messenger_retrieve_options(AMQP_MESSENGER_HANDLE messenger_handle);
	void amqp_messenger_destroy_disposition_info(AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info);
```


## amqp_messenger_create

```c
	extern AMQP_MESSENGER_HANDLE amqp_messenger_create(const AMQP_MESSENGER_CONFIG* messenger_config);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_001: [**If `messenger_config` is NULL, amqp_messenger_create() shall return NULL**]** 
 
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_002: [**If `messenger_config`'s `device_id`, `iothub_host_fqdn`, `receive_link.source_suffix` or `send_link.target_suffix` are NULL, amqp_messenger_create() shall return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_003: [**amqp_messenger_create() shall allocate memory for the messenger instance structure (aka `instance`)**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_004: [**If malloc() fails, amqp_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_005: [**amqp_messenger_create() shall save a copy of `messenger_config` into `instance`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_006: [**If the copy fails, amqp_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_007: [**`instance->send_queue` shall be set using message_queue_create(), passing `on_process_message_callback`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_008: [**If message_queue_create() fails, amqp_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_009: [**If no failures occurr, amqp_messenger_create() shall return a handle to `instance`**]**  


### amqp_messenger_send_async
```c
	extern int amqp_messenger_send_async(AMQP_MESSENGER_HANDLE messenger_handle, MESSAGE_HANDLE message, AMQP_MESSENGER_SEND_COMPLETE_CALLBACK on_user_defined_send_complete_callback, void* user_context);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_010: [**If `messenger_handle`, `message` or `on_event_send_complete_callback` are NULL, amqp_messenger_send_async() shall fail and return a non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_011: [**`message` shall be cloned using message_clone()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_012: [**If message_clone() fails, amqp_messenger_send_async() shall fail and return a non-zero value**]**    

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_013: [**amqp_messenger_send_async() shall allocate memory for a MESSAGE_SEND_CONTEXT structure (aka `send_ctx`)**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_014: [**If malloc() fails, amqp_messenger_send_async() shall fail and return a non-zero value**]**    

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_015: [**The cloned `message`, callback and context shall be saved in ``send_ctx`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_016: [**`send_ctx` shall be added to `instance->send_queue` using message_queue_add(), passing `on_message_processing_completed_callback`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_017: [**If message_queue_add() fails, amqp_messenger_send_async() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_018: [**If any failure occurs, amqp_messenger_send_async() shall free any memory it has allocated**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_019: [**If no failures occur, amqp_messenger_send_async() shall return zero**]**  


#### on_message_processing_completed_callback
```c
static void on_message_processing_completed_callback(MQ_MESSAGE_HANDLE message, MESSAGE_QUEUE_RESULT result, USER_DEFINED_REASON reason, void* message_context)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_020: [**If `messenger_context` is NULL, `on_message_processing_completed_callback` shall return immediately**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_021: [**If `result` is MESSAGE_QUEUE_SUCCESS, `send_ctx->on_send_complete_callback` shall be invoked with AMQP_MESSENGER_SEND_RESULT_SUCCESS and AMQP_MESSENGER_REASON_NONE**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_022: [**If `result` is MESSAGE_QUEUE_TIMEOUT, `send_ctx->on_send_complete_callback` shall be invoked with AMQP_MESSENGER_SEND_RESULT_ERROR and AMQP_MESSENGER_REASON_TIMEOUT**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_023: [**If `result` is MESSAGE_QUEUE_CANCELLED and the messenger is STOPPED, `send_ctx->on_send_complete_callback` shall be invoked with AMQP_MESSENGER_SEND_RESULT_CANCELLED and AMQP_MESSENGER_REASON_MESSENGER_DESTROYED**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_024: [**Otherwise `send_ctx->on_send_complete_callback` shall be invoked with AMQP_MESSENGER_SEND_RESULT_ERROR and AMQP_MESSENGER_REASON_FAIL_SENDING**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_025: [**If `result` is MESSAGE_QUEUE_SUCCESS or MESSAGE_QUEUE_CANCELLED, `instance->send_error_count` shall be set to 0**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_026: [**Otherwise `instance->send_error_count` shall be incremented by 1**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_027: [**If `message` has not been destroyed, it shall be destroyed using message_destroy()**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_028: [**`send_ctx` shall be destroyed**]**



### amqp_messenger_get_send_status

```c
int amqp_messenger_get_send_status(AMQP_MESSENGER_HANDLE messenger_handle, AMQP_MESSENGER_SEND_STATUS* send_status);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_029: [**If `messenger_handle` or `send_status` are NULL, amqp_messenger_get_send_status() shall fail and return a non-zero value**]** 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_030: [**message_queue_is_empty() shall be invoked for `instance->send_queue`**]** 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_031: [**If message_queue_is_empty() fails, amqp_messenger_get_send_status() shall fail and return a non-zero value**]** 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_032: [**If message_queue_is_empty() returns `true`, `send_status` shall be set to AMQP_MESSENGER_SEND_STATUS_IDLE**]** 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_033: [**Otherwise, `send_status` shall be set to AMQP_MESSENGER_SEND_STATUS_BUSY**]** 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_034: [**If no failures occur, amqp_messenger_get_send_status() shall return 0**]** 


## amqp_messenger_subscribe_for_messages

```c
	extern int amqp_messenger_subscribe_for_messages(AMQP_MESSENGER_HANDLE messenger_handle, ON_AMQP_MESSENGER_MESSAGE_RECEIVED on_message_received_callback, void* context);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_035: [**If `messenger_handle` or `on_message_received_callback` are NULL, amqp_messenger_subscribe_for_messages() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_036: [**`on_message_received_callback` and `context` shall be saved on `instance->on_message_received_callback`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_037: [**amqp_messenger_subscribe_for_messages() shall set `instance->receive_messages` to true**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_038: [**If no failures occurr, amqp_messenger_subscribe_for_messages() shall return 0**]**  


## amqp_messenger_unsubscribe_for_messages

```c
	extern int amqp_messenger_unsubscribe_for_messages(AMQP_MESSENGER_HANDLE messenger_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_039: [**If `messenger_handle` is NULL, amqp_messenger_unsubscribe_for_messages() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_040: [**`instance->receive_messages` shall be saved to false**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_041: [**`instance->on_message_received_callback` and `instance->on_message_received_context` shall be set to NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_042: [**If no failures occurr, amqp_messenger_unsubscribe_for_messages() shall return 0**]**  


## amqp_messenger_send_message_disposition
```c
extern int amqp_messenger_send_message_disposition(AMQP_MESSENGER_HANDLE messenger_handle, AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT disposition_result);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_043: [**If `messenger_handle` or `disposition_info` are NULL, amqp_messenger_send_message_disposition() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_044: [**If `disposition_info->source` is NULL, amqp_messenger_send_message_disposition() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_045: [**If `messenger_handle->message_receiver` is NULL, amqp_messenger_send_message_disposition() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_046: [**An AMQP_VALUE disposition result shall be created corresponding to the `disposition_result` provided**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_047: [**`messagereceiver_send_message_disposition()` shall be invoked passing `disposition_info->source`, `disposition_info->message_id` and the AMQP_VALUE disposition result**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_048: [**If `messagereceiver_send_message_disposition()` fails, amqp_messenger_send_message_disposition() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_049: [**amqp_messenger_send_message_disposition() shall destroy the AMQP_VALUE disposition result**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_050: [**If no failures occurr, amqp_messenger_send_message_disposition() shall return 0**]**  



## amqp_messenger_start

```c
	extern int amqp_messenger_start(AMQP_MESSENGER_HANDLE messenger_handle, SESSION_HANDLE session_handle); 
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_051: [**If `messenger_handle` or `session_handle` are NULL, amqp_messenger_start() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_052: [**If `instance->state` is not AMQP_MESSENGER_STATE_STOPPED, amqp_messenger_start() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_053: [**`session_handle` shall be saved on `instance->session_handle`**]**   

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_054: [**If no failures occurr, `instance->state` shall be set to AMQP_MESSENGER_STATE_STARTING, and `instance->on_state_changed_callback` invoked if provided**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_055: [**If no failures occurr, amqp_messenger_start() shall return 0**]**  


## amqp_messenger_stop

```c
	extern int amqp_messenger_stop(AMQP_MESSENGER_HANDLE messenger_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_056: [**If `messenger_handle` is NULL, amqp_messenger_stop() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_057: [**If `instance->state` is AMQP_MESSENGER_STATE_STOPPED, amqp_messenger_stop() shall fail and return non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_058: [**`instance->state` shall be set to AMQP_MESSENGER_STATE_STOPPING, and `instance->on_state_changed_callback` invoked if provided**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_059: [**`instance->message_sender` and `instance->message_receiver` shall be destroyed along with all its links**]** 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_060: [**`instance->send_queue` items shall be rolled back using message_queue_move_all_back_to_pending()**]** 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_061: [**If message_queue_move_all_back_to_pending() fails, amqp_messenger_stop() shall change the messenger state to AMQP_MESSENGER_STATE_ERROR and return a non-zero value**]** 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_062: [**`instance->state` shall be set to AMQP_MESSENGER_STATE_STOPPED, and `instance->on_state_changed_callback` invoked if provided**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_063: [**If no failures occurr, amqp_messenger_stop() shall return 0**]**  


## amqp_messenger_do_work

```c
	extern void amqp_messenger_do_work(AMQP_MESSENGER_HANDLE messenger_handle);
```

Summary: creates/destroys the uAMQP messagesender, messagereceiver according to current subscription (amqp_messenger_subscribe_for_messages/amqp_messenger_unsubscribe_for_messages), sends pending D2C messages. 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_064: [**If `messenger_handle` is NULL, amqp_messenger_do_work() shall fail and return**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_065: [**amqp_messenger_do_work() shall update the current state according to the states of message sender and receiver**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_066: [**If the current state is AMQP_MESSENGER_STARTING, amqp_messenger_do_work() shall create and start `instance->message_sender`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_067: [**If the `instance->message_sender` fails to be created/started, amqp_messenger_do_work() shall set the state to AMQP_MESSENGER_STATE_ERROR**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_068: [**If the message sender state changes to MESSAGE_SENDER_STATE_OPEN, amqp_messenger_do_work() shall set the state to AMQP_MESSENGER_STATE_STARTED**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_069: [**If the `instance->receive_messages` is true, amqp_messenger_do_work() shall create and start `instance->message_receiver` if not done before**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_070: [**If the `instance->message_receiver` fails to be created/started, amqp_messenger_do_work() shall set the state to AMQP_MESSENGER_STATE_ERROR**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_071: [**If the `instance->receive_messages` is false, amqp_messenger_do_work() shall stop and destroy `instance->message_receiver` if not done before**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_072: [**If the `instance->message_receiver` fails to be stopped/destroyed, amqp_messenger_do_work() shall set the state to AMQP_MESSENGER_STATE_ERROR**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_073: [**amqp_messenger_do_work() shall invoke message_queue_do_work() on `instance->send_queue`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_074: [**If `instance->send_error_count` DEFAULT_MAX_SEND_ERROR_COUNT (10), amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR**]**  



### Create/Open the message sender

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_075: [**The AMQP link address shall be defined as "amqps://<`iothub_host_fqdn`>/devices/<`device_id`>/<`instance-config->send_link.source_suffix`>"**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_076: [**The AMQP link name shall be defined as "link-snd-<`device_id`>-<locally generated UUID>"**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_077: [**The AMQP link source shall be defined as "<link name>-source"**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_078: [**The AMQP link target shall be defined as <link address>**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_079: [**If the link fails to be created, amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_080: [**The AMQP link shall have its ATTACH properties set using `instance->config->send_link.attach_properties`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_081: [**If the AMQP link attach properties fail to be set, amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_082: [**The AMQP link maximum message size shall be set to UINT64_MAX using link_set_max_message_size()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_083: [**If link_set_max_message_size() fails, it shall be logged and ignored.**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_084: [**`instance->message_sender` shall be created using messagesender_create(), passing the `instance->sender_link` and `on_message_sender_state_changed_callback`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_085: [**If messagesender_create() fails, amqp_messenger_do_work() shall fail and return**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_086: [**`instance->message_sender` shall be opened using messagesender_open()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_087: [**If messagesender_open() fails, amqp_messenger_do_work() shall fail and return**]**  


#### on_message_sender_state_changed_callback

```c
	static void on_message_sender_state_changed_callback(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_088: [**`new_state`, `previous_state` shall be saved into `instance->message_sender_previous_state` and `instance->message_sender_current_state`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_089: [**`instance->last_message_sender_state_change_time` shall be set using get_time()**]**  


### Destroy the message sender
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_090: [**`instance->message_sender` shall be destroyed using messagesender_destroy()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_091: [**`instance->sender_link` shall be destroyed using link_destroy()**]**  


### Create a message receiver


**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_092: [**The AMQP link address shall be defined as "amqps://<`iothub_host_fqdn`>/devices/<`device_id`>/<`instance-config->receive_link.target_suffix`>"**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_093: [**The AMQP link name shall be defined as "link-rcv-<`device_id`>-<locally generated UUID>"**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_094: [**The AMQP link source shall be defined as <link address>**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_095: [**The AMQP link target shall be defined as "<link name>-target"**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_096: [**If the link fails to be created, amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_097: [**The AMQP link shall have its ATTACH properties set using `instance->config->receive_link.attach_properties`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_098: [**If the AMQP link attach properties fail to be set, amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_099: [**The AMQP link maximum message size shall be set to UINT64_MAX using link_set_max_message_size()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_100: [**If link_set_max_message_size() fails, it shall be logged and ignored.**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_101: [**`instance->message_receiver` shall be created using messagereceiver_create(), passing the `instance->receiver_link` and `on_message_receiver_state_changed_callback`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_102: [**If messagereceiver_create() fails, amqp_messenger_do_work() shall fail and return**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_103: [**`instance->message_receiver` shall be opened using messagereceiver_open() passing `on_message_received_internal_callback`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_104: [**If messagereceiver_open() fails, amqp_messenger_do_work() shall fail and return**]**  



#### on_message_receiver_state_changed_callback

```c
static void on_message_receiver_state_changed_callback(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_105: [**`new_state`, `previous_state` shall be saved into `instance->message_receiver_previous_state` and `instance->message_receiver_current_state`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_106: [**`instance->last_message_receiver_state_change_time` shall be set using get_time()**]**


#### on_message_received_internal_callback

```c
static AMQP_VALUE on_message_received_internal_callback(const void* context, MESSAGE_HANDLE message)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_107: [**A AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO instance shall be created containing the source link name and message delivery ID**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_108: [**If the AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO instance fails to be created, on_message_received_internal_callback shall return the result of messaging_delivery_released()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_109: [**`instance->on_message_received_callback` shall be invoked passing the `message` and AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO instance**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_110: [**If `instance->on_message_received_callback` returns AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED, on_message_received_internal_callback shall return the result of messaging_delivery_accepted()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_111: [**If `instance->on_message_received_callback` returns AMQP_MESSENGER_DISPOSITION_RESULT_RELEASED, on_message_received_internal_callback shall return the result of messaging_delivery_released()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_112: [**If `instance->on_message_received_callback` returns AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED, on_message_received_internal_callback shall return the result of messaging_delivery_rejected()**]**  


### Destroy the message receiver

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_113: [**`instance->message_receiver` shall be closed using messagereceiver_close()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_114: [**If messagereceiver_close() fails, it shall be logged and ignored**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_115: [**`instance->message_receiver` shall be destroyed using messagereceiver_destroy()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_116: [**`instance->receiver_link` shall be destroyed using link_destroy()**]**  


### on_process_message_callback
```c
static void on_process_message_callback(MESSAGE_QUEUE_HANDLE message_queue, MQ_MESSAGE_HANDLE message, PROCESS_MESSAGE_COMPLETED_CALLBACK on_process_message_completed_callback, void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_117: [**If any argument is NULL, `on_process_message_callback` shall return immediatelly**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_118: [**The MESSAGE_HANDLE shall be submitted for sending using messagesender_send(), passing `on_send_complete_callback`**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_119: [**If messagesender_send() fails, `on_process_message_completed_callback` shall be invoked with result MESSAGE_QUEUE_ERROR**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_120: [**The MESSAGE_HANDLE shall be destroyed using message_destroy() and marked as destroyed in the context provided**]**  


#### on_send_complete_callback

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_121: [**If no failure occurs, `context->on_process_message_completed_callback` shall be invoked with result MESSAGE_QUEUE_SUCCESS**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_122: [**If a failure occurred, `context->on_process_message_completed_callback` shall be invoked with result MESSAGE_QUEUE_ERROR**]**


## amqp_messenger_destroy

```c
	extern void amqp_messenger_destroy(AMQP_MESSENGER_HANDLE messenger_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_123: [**If `messenger_handle` is NULL, amqp_messenger_destroy() shall fail and return**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_124: [**If `instance->state` is not AMQP_MESSENGER_STATE_STOPPED, amqp_messenger_destroy() shall invoke amqp_messenger_stop()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_125: [**message_queue_destroy() shall be invoked for `instance->send_queue`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_126: [**amqp_messenger_destroy() shall release all the memory allocated for `instance`**]**  


## amqp_messenger_set_option

```c
	extern int amqp_messenger_set_option(AMQP_MESSENGER_HANDLE messenger_handle, const char* name, void* value);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_127: [**If `messenger_handle` or `name` or `value` is NULL, amqp_messenger_set_option shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_128: [**If name matches AMQP_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, `value` shall be set on `instance->send_queue` using message_queue_set_max_message_enqueued_time_secs()**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_129: [**If message_queue_set_max_message_enqueued_time_secs() fails, amqp_messenger_set_option() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_130: [**If `name` does not match any supported option, amqp_messenger_set_option() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_131: [**If no errors occur, amqp_messenger_set_option shall return 0**]**


## amqp_messenger_retrieve_options

```c
	extern OPTIONHANDLER_HANDLE amqp_messenger_retrieve_options(AMQP_MESSENGER_HANDLE messenger_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_132: [**If `messenger_handle` is NULL, amqp_messenger_retrieve_options shall fail and return NULL**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_133: [**An OPTIONHANDLER_HANDLE instance shall be created using OptionHandler_Create**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_134: [**If an OPTIONHANDLER_HANDLE instance fails to be created, amqp_messenger_retrieve_options shall fail and return NULL**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_135: [**`instance->send_queue` options shall be retrieved using message_queue_retrieve_options()**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_136: [**If message_queue_retrieve_options() fails, amqp_messenger_retrieve_options shall fail and return NULL**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_137: [**Each option of `instance` shall be added to the OPTIONHANDLER_HANDLE instance using OptionHandler_AddOption**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_138: [**If OptionHandler_AddOption fails, amqp_messenger_retrieve_options shall fail and return NULL**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_139: [**If amqp_messenger_retrieve_options fails, any allocated memory shall be freed**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_140: [**If no failures occur, amqp_messenger_retrieve_options shall return the OPTIONHANDLER_HANDLE instance**]**


## 	amqp_messenger_destroy_disposition_info
```c
void amqp_messenger_destroy_disposition_info(AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info);
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_132: [**If `disposition_info` is NULL, amqp_messenger_destroy_disposition_info shall return immediately**]**

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_132: [**All memory allocated for `disposition_info` shall be released**]**
