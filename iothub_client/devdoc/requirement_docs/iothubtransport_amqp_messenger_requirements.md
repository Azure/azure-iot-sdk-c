# iothubtransport_amqp_messenger Requirements


## Overview

This module provides an abstraction for the IoTHubTransportAMQP to send events and receive messages.  


## Dependencies

uamqp_messaging
azure_c_shared_utility
azure_uamqp_c

   
## Exposed API

```c
	typedef enum MESSENGER_STATE_TAG
	{
		MESSENGER_STATE_STARTING,
		MESSENGER_STATE_STARTED,
		MESSENGER_STATE_STOPPED,
		MESSENGER_STATE_ERROR
	} MESSENGER_STATE;

	typedef void(*ON_MESSENGER_STATE_CHANGED_CALLBACK)(void* context, MESSENGER_STATE previous_state, MESSENGER_STATE new_state);

	typedef enum MESSENGER_DISPOSITION_RESULT_TAG
	{
		MESSENGER_DISPOSITION_RESULT_ACCEPTED,
		MESSENGER_DISPOSITION_RESULT_REJECTED,
		MESSENGER_DISPOSITION_RESULT_ABANDONED
	} MESSENGER_DISPOSITION_RESULT;

	typedef MESSENGER_DISPOSITION_RESULT(*ON_MESSAGE_RECEIVED)(IOTHUB_MESSAGE_HANDLE message, void* context);

	typedef struct MESSENGER_CONFIG_TAG
	{
		char* device_id;
		char* iothub_host_fqdn;
		PDLIST_ENTRY wait_to_send_list;
		ON_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
		void* on_state_changed_context;
	} MESSENGER_CONFIG;

	typedef struct MESSENGER_INSTANCE* MESSENGER_HANDLE;

	extern MESSENGER_HANDLE messenger_create(const MESSENGER_CONFIG* messenger_config);
	extern int messenger_subscribe_for_messages(MESSENGER_HANDLE messenger_handle, ON_MESSAGE_RECEIVED on_message_received_callback, void* context);
	extern int messenger_unsubscribe_for_messages(MESSENGER_HANDLE messenger_handle);
	extern int messenger_start(MESSENGER_HANDLE messenger_handle, SESSION_HANDLE session_handle); 
	extern int messenger_stop(MESSENGER_HANDLE messenger_handle);
	extern void messenger_do_work(MESSENGER_HANDLE messenger_handle);
	extern void messenger_destroy(MESSENGER_HANDLE messenger_handle);
```


## messenger_create

```c
	extern MESSENGER_HANDLE messenger_create(const MESSENGER_CONFIG* messenger_config);
```

Summary: creates struct instance to store data, checks and stores copies of parameters; no other components are created not started.  

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_001: [**If parameter `messenger_config` is NULL, messenger_create() shall return NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_002: [**If `messenger_config->device_id` is NULL, messenger_create() shall return NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_003: [**If `messenger_config->iothub_host_fqdn` is NULL, messenger_create() shall return NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_004: [**If `messenger_config->wait_to_send_list` is NULL, messenger_create() shall return NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_006: [**messenger_create() shall allocate memory for the messenger instance structure (aka `instance`)**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_007: [**If malloc() fails, messenger_create() shall fail and return NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_008: [**messenger_create() shall save a copy of `messenger_config->device_id` into `instance->device_id`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_009: [**If STRING_construct() fails, messenger_create() shall fail and return NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_010: [**messenger_create() shall save a copy of `messenger_config->iothub_host_fqdn` into `instance->iothub_host_fqdn`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_011: [**If STRING_construct() fails, messenger_create() shall fail and return NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_012: [**The pointer `messenger_config->wait_to_send_list` shall be saved into `instance->wait_to_send_list`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_013: [**`messenger_config->on_state_changed_callback` shall be saved into `instance->on_state_changed_callback`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_014: [**`messenger_config->on_state_changed_context` shall be saved into `instance->on_state_changed_context`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_015: [**If no failures occurr, messenger_create() shall return a handle to `instance`**]**  


## messenger_subscribe_for_messages

```c
	extern int messenger_subscribe_for_messages(MESSENGER_HANDLE messenger_handle, ON_MESSAGE_RECEIVED on_message_received_callback, void* context);
```

Summary: informs the messenger instance that a uAMQP messagereceiver shall be created/started (delayed, done on messenger_do_work()), stores the callback information.

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_016: [**If `messenger_handle` is NULL, messenger_subscribe_for_messages() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_017: [**If `instance->receive_messages` is already true, messenger_subscribe_for_messages() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_018: [**If `on_message_received_callback` is NULL, messenger_subscribe_for_messages() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_019: [**`on_message_received_callback` shall be saved on `instance->on_message_received_callback`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_020: [**`context` shall be saved on `instance->on_message_received_context`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_021: [**messenger_subscribe_for_messages() shall set `instance->receive_messages` to true**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_022: [**If no failures occurr, messenger_subscribe_for_messages() shall return 0**]**  


## messenger_unsubscribe_for_messages

```c
	extern int messenger_unsubscribe_for_messages(MESSENGER_HANDLE messenger_handle);
```

Summary: informs the messenger instance that an existing uAMQP messagereceiver shall be stopped/destroyed (delayed, done on messenger_do_work()).

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_023: [**If `messenger_handle` is NULL, messenger_unsubscribe_for_messages() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_024: [**If `instance->receive_messages` is already false, messenger_unsubscribe_for_messages() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_025: [**messenger_unsubscribe_for_messages() shall set `instance->receive_messages` to false**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_026: [**messenger_unsubscribe_for_messages() shall set `instance->on_message_received_callback` to NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_027: [**messenger_unsubscribe_for_messages() shall set `instance->on_message_received_context` to NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_028: [**If no failures occurr, messenger_unsubscribe_for_messages() shall return 0**]**  


## messenger_start

```c
	extern int messenger_start(MESSENGER_HANDLE messenger_handle, SESSION_HANDLE session_handle); 
```

Summary: creates and opens the uAMQP messagesender

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_029: [**If `messenger_handle` is NULL, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_030: [**If `session_handle` is NULL, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_031: [**If `instance->state` is not MESSENGER_STATE_STOPPED, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_032: [**`session_handle` shall be saved on `instance->session_handle`, and the `instance` marked as started**]**  


### Create/start the message sender

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_033: [**A variable, named `devices_path`, shall be created concatenating `instance->iothub_host_fqdn`, "/devices/" and `instance->device_id`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_034: [**If `devices_path` fails to be created, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_035: [**A variable, named `event_send_address`, shall be created concatenating "amqps://", `devices_path` and "/messages/events"**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_036: [**If `event_send_address` fails to be created, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_037: [**A `link_name` variable shall be created using an unique string label per AMQP session**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_038: [**If `link_name` fails to be created, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_039: [**A `source` variable shall be created with messaging_create_source() using an unique string label per AMQP session**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_040: [**If `source` fails to be created, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_041: [**A `target` variable shall be created with messaging_create_target() using `event_send_address`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_042: [**If `target` fails to be created, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_043: [**`instance->sender_link` shall be set using link_create(), passing `instance->session_handle`, `link_name`, "role_sender", `source` and `target` as parameters**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_044: [**If link_create() fails, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_047: [**`instance->sender_link` maximum message size shall be set to UINT64_MAX using link_set_max_message_size()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_048: [**If link_set_max_message_size() fails, it shall be logged and ignored.**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_049: [**`instance->sender_link` should have a property "com.microsoft:client-version" set as `CLIENT_DEVICE_TYPE_PREFIX/IOTHUB_SDK_VERSION`, using amqpvalue_set_map_value() and link_set_attach_properties()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_050: [**If amqpvalue_set_map_value() or link_set_attach_properties() fail, the failure shall be ignored**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_051: [**`instance->message_sender` shall be created using messagesender_create(), passing the `instance->sender_link` and `on_event_sender_state_changed_callback`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_052: [**If messagesender_create() fails, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_053: [**`instance->message_sender` shall be opened using messagesender_open()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_054: [**If messagesender_open() fails, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_055: [**Before returning, messenger_start() shall release all the temporary memory it has allocated**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_115: [**If no failures occurr, `instance->state` shall be set to MESSENGER_STATE_STARTING, and `instance->on_state_changed_callback` invoked if provided**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_056: [**If no failures occurr, messenger_start() shall return 0**]**  


#### on_event_sender_state_changed_callback

```c
static void on_event_sender_state_changed_callback(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_118: [**If the messagesender new state is MESSAGE_SENDER_STATE_OPEN, `instance->state` shall be set to MESSENGER_STATE_STARTED, and `instance->on_state_changed_callback` invoked if provided**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_119: [**If the messagesender new state is MESSAGE_SENDER_STATE_ERROR, `instance->state` shall be set to MESSENGER_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided**]**  


## messenger_stop

```c
	extern int messenger_stop(MESSENGER_HANDLE messenger_handle);
```

Summary: closes and destroys the uAMQP messagesender and messagereceiver (and their respective components).

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_057: [**If `messenger_handle` is NULL, messenger_stop() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_058: [**If `instance->state` is MESSENGER_STATE_STOPPED, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_060: [**`instance->message_sender` shall be destroyed using messagesender_destroy()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_061: [**`instance->message_receiver` shall be closed using messagereceiver_close()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_062: [**`instance->message_receiver` shall be destroyed using messagereceiver_destroy()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_063: [**`instance->sender_link` shall be destroyed using link_destroy()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_064: [**`instance->receiver_link` shall be destroyed using link_destroy()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_116: [**`instance->state` shall be set to MESSENGER_STATE_STOPPED, and `instance->on_state_changed_callback` invoked if provided**]**  



## messenger_do_work

```c
	extern void messenger_do_work(MESSENGER_HANDLE messenger_handle);
```

Summary: creates/destroys the uAMQP messagereceiver according to current subscription (messenger_subscribe_for_messages/messenger_unsubscribe_for_messages), sends pending D2C events. 

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_065: [**If `messenger_handle` is NULL, messenger_do_work() shall fail and return**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_066: [**If `instance->state` is not MESSENGER_STATE_STARTED, messenger_do_work() shall return**]**  


### Create a message receiver

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_067: [**If `instance->receive_messages` is true and `instance->message_receiver` is NULL, a message_receiver shall be created**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_068: [**A variable, named `devices_path`, shall be created concatenating `instance->iothub_host_fqdn`, "/devices/" and `instance->device_id`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_069: [**If `devices_path` fails to be created, messenger_do_work() shall fail and return**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_070: [**A variable, named `message_receive_address`, shall be created concatenating "amqps://", `devices_path` and "/messages/devicebound"**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_071: [**If `message_receive_address` fails to be created, messenger_do_work() shall fail and return**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_072: [**A `link_name` variable shall be created using an unique string label per AMQP session**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_073: [**If `link_name` fails to be created, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_074: [**A `target` variable shall be created with messaging_create_target() using an unique string label per AMQP session**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_075: [**If `target` fails to be created, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_076: [**A `source` variable shall be created with messaging_create_source() using `message_receive_address`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_077: [**If `source` fails to be created, messenger_start() shall fail and return __LINE__**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_078: [**`instance->receiver_link` shall be set using link_create(), passing `instance->session_handle`, `link_name`, "role_receiver", `source` and `target` as parameters**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_079: [**If link_create() fails, messenger_do_work() shall fail and return**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_080: [**`instance->receiver_link` settle mode shall be set to "receiver_settle_mode_first" using link_set_rcv_settle_mode(), **]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_081: [**If link_set_rcv_settle_mode() fails, messenger_do_work() shall fail and return**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_082: [**`instance->receiver_link` maximum message size shall be set to 65536 using link_set_max_message_size()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_083: [**If link_set_max_message_size() fails, it shall be logged and ignored.**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_084: [**`instance->receiver_link` should have a property "com.microsoft:client-version" set as `CLIENT_DEVICE_TYPE_PREFIX/IOTHUB_SDK_VERSION`, using amqpvalue_set_map_value() and link_set_attach_properties()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_085: [**If amqpvalue_set_map_value() or link_set_attach_properties() fail, the failure shall be ignored**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_086: [**`instance->message_receiver` shall be created using messagereceiver_create(), passing the `instance->receiver_link` and `on_messagereceiver_state_changed_callback`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_087: [**If messagereceiver_create() fails, messenger_do_work() shall fail and return**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_088: [**`instance->message_receiver` shall be opened using messagereceiver_open(), passing `on_message_received_internal_callback`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_089: [**If messagereceiver_open() fails, messenger_do_work() shall fail and return**]**  


#### on_messagereceiver_state_changed_callback

```c
static void on_message_receiver_state_changed(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_120: [**If the messagereceiver new state is MESSAGE_RECEIVER_STATE_ERROR, `instance->state` shall be set to MESSENGER_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided**]**  


#### on_message_received_internal_callback

```c
static AMQP_VALUE on_message_received_internal_callback(const void* context, MESSAGE_HANDLE message)
```

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_121: [**An IOTHUB_MESSAGE_HANDLE shall be obtained from MESSAGE_HANDLE using IoTHubMessage_CreateFromUamqpMessage()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_122: [**If IoTHubMessage_CreateFromUamqpMessage() fails, on_message_received_internal_callback shall return the result of messaging_delivery_rejected()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_123: [**`instance->on_message_received_callback` shall be invoked passing the IOTHUB_MESSAGE_HANDLE**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_124: [**The IOTHUB_MESSAGE_HANDLE instance shall be destroyed using IoTHubMessage_Destroy()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_125: [**If `instance->on_message_received_callback` returns MESSENGER_DISPOSITION_RESULT_ACCEPTED, on_message_received_internal_callback shall return the result of messaging_delivery_accepted()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_126: [**If `instance->on_message_received_callback` returns MESSENGER_DISPOSITION_RESULT_ABANDONED, on_message_received_internal_callback shall return the result of messaging_delivery_released()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_127: [**If `instance->on_message_received_callback` returns MESSENGER_DISPOSITION_RESULT_REJECTED, on_message_received_internal_callback shall return the result of messaging_delivery_rejected()**]**  


### Destroy the message receiver

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_092: [**If `instance->receive_messages` is false and `instance->message_receiver` is not NULL, it shall be destroyed**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_093: [**`instance->message_receiver` shall be closed using messagereceiver_close()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_094: [**If messagereceiver_close() fails, it shall be logged and ignored**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_095: [**`instance->message_receiver` shall be destroyed using messagereceiver_destroy()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_096: [**`instance->message_receiver` shall be set to NULL**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_097: [**`instance->receiver_link` shall be destroyed using link_destroy()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_098: [**`instance->receiver_link` shall be set to NULL**]**  


### Send pending events

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_100: [**messenger_do_work() shall move each event to be sent from `instance->wait_to_send_list` to `instance->in_progress_list`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_101: [**A MESSAGE_HANDLE shall be obtained out of the event's IOTHUB_MESSAGE_HANDLE instance by using message_create_from_iothub_message()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_102: [**If message_create_from_iothub_message() fails, `MESSAGE_HANDLE->callback` shall be invoked, if defined, with result IOTHUB_CLIENT_CONFIRMATION_ERROR**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_103: [**If message_create_from_iothub_message() fails, messenger_do_work() shall fail and return**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_104: [**The MESSAGE_HANDLE shall be submitted for sending using messagesender_send(), passing `on_message_send_complete_callback`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_105: [**If messagesender_send() fails, the event shall be rolled back from `instance->in_progress_list` to `instance->wait_to_send_list`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_106: [**The MESSAGE_HANDLE shall be destroyed using message_destroy().**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_131: [**If messenger_do_work() fail sending events for `instance->event_send_retry_limit` times, it shall invoke `instance->on_state_changed_callback`, if provided, with error code MESSENGER_STATE_ERROR**]**  


#### on_message_send_complete_callback

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_107: [**If no failure occurs, `MESSAGE_HANDLE->callback` shall be invoked with result IOTHUB_CLIENT_CONFIRMATION_OK**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_108: [**If a failure occurred, `MESSAGE_HANDLE->callback` shall be invoked with result IOTHUB_CLIENT_CONFIRMATION_ERROR**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_128: [**MESSAGE_HANDLE shall be removed from `instance->in_progress_list`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_129: [**`MESSAGE_HANDLE->messageHandle` shall be destroyed using IoTHubMessage_Destroy()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_130: [**MESSAGE_HANDLE shall be destroyed using free()**]**  


## messenger_destroy

```c
	extern void messenger_destroy(MESSENGER_HANDLE messenger_handle);
```

Summary: stops the messenger if needed, destroys all the components within the messenger data structure (messenger_handle) and releases any allocated memory.

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_109: [**If `messenger_handle` is NULL, messenger_destroy() shall fail and return**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_110: [**If the `instance->state` is not MESSENGER_STATE_STOPPED, messenger_destroy() shall invoke messenger_stop(), ignoring its result**]**  

Note: when events are moved from wait_to_send_list to in_progress_list, they are moved from beginning (oldest element) to end (newest).
Due to the logic used by messenger_do_work(), all elements of in_progress_list are always older than any current element on wait_to_send_list.
In that case, when they are rolled back, they need to go to the beginning of the wait_to_send_list.

**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_111: [**All elements of `instance->in_progress_list` shall be moved to the beginning of `instance->wait_to_send_list`**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_112: [**`instance->iothub_host_fqdn` shall be destroyed using STRING_delete()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_113: [**`instance->device_id` shall be destroyed using STRING_delete()**]**  
**SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_114: [**messenger_destroy() shall destroy `instance` with free()**]**  
