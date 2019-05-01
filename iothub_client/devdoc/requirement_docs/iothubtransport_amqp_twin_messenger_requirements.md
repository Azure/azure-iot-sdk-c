# iothubtransport_amqp_twin_messenger Requirements


## Overview

This module provides functionality for IoTHubTransportAMQP to work with Device Twin operations.  


## Dependencies

azure_c_shared_utility
azure_uamqp_c

   
## Exposed API

```c
	typedef struct TWIN_MESSENGER_INSTANCE* TWIN_MESSENGER_HANDLE;

	typedef enum TWIN_MESSENGER_SEND_STATUS_TAG
	{
		TWIN_MESSENGER_SEND_STATUS_IDLE,
		TWIN_MESSENGER_SEND_STATUS_BUSY
	} TWIN_MESSENGER_SEND_STATUS;

	typedef enum TWIN_REPORT_STATE_RESULT_TAG
	{
		TWIN_REPORT_STATE_RESULT_OK,
		TWIN_REPORT_STATE_RESULT_ERROR_CANNOT_PARSE,
		TWIN_REPORT_STATE_RESULT_ERROR_FAIL_SENDING,
		TWIN_REPORT_STATE_RESULT_ERROR_TIMEOUT,
		TWIN_REPORT_STATE_RESULT_MESSENGER_DESTROYED
	} TWIN_REPORT_STATE_RESULT;

	typedef enum TWIN_MESSENGER_STATE_TAG
	{
		TWIN_MESSENGER_STATE_STARTING,
		TWIN_MESSENGER_STATE_STARTED,
		TWIN_MESSENGER_STATE_STOPPING,
		TWIN_MESSENGER_STATE_STOPPED,
		TWIN_MESSENGER_STATE_ERROR
	} TWIN_MESSENGER_STATE;

	typedef enum TWIN_UPDATE_TYPE_TAG 
	{
		TWIN_UPDATE_TYPE_PARTIAL,
		TWIN_UPDATE_TYPE_COMPLETE
 	} TWIN_UPDATE_TYPE;

	typedef void(*ON_TWIN_MESSENGER_STATE_CHANGED_CALLBACK)(void* context, TWIN_MESSENGER_STATE previous_state, TWIN_MESSENGER_STATE new_state);
	typedef void(*ON_TWIN_MESSENGER_REPORT_STATE_COMPLETE_CALLBACK)(TWIN_REPORT_STATE_RESULT result, int status_code, void* context);
	typedef void(*ON_TWIN_STATE_UPDATE_CALLBACK)(TWIN_UPDATE_TYPE update_type, const char* payload, size_t size, void* context);

	typedef struct MESSENGER_CONFIG_TAG
	{
		const char* device_id;
		char* iothub_host_fqdn;
		ON_TWIN_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
		void* on_state_changed_context;
	} MESSENGER_CONFIG;

	extern TWIN_MESSENGER_HANDLE twin_messenger_create(const TWIN_MESSENGER_CONFIG* messenger_config);
	extern int twin_messenger_report_state_async(TWIN_TWIN_MESSENGER_HANDLE twin_msgr_handle, CONSTBUFFER_HANDLE data, ON_TWIN_MESSENGER_REPORT_STATE_COMPLETE_CALLBACK on_report_state_complete_callback, const void* context);
	extern int twin_messenger_subscribe(TWIN_MESSENGER_HANDLE twin_msgr_handle, ON_TWIN_STATE_UPDATE_CALLBACK on_twin_state_update_callback, void* context);
	extern int twin_messenger_unsubscribe(TWIN_MESSENGER_HANDLE twin_msgr_handle);
	extern int twin_messenger_get_twin_async(TWIN_MESSENGER_HANDLE twin_msgr_handle, TWIN_STATE_UPDATE_CALLBACK on_get_twin_completed_callback, void* context);
	extern int twin_messenger_get_send_status(TWIN_MESSENGER_HANDLE twin_msgr_handle, TWIN_MESSENGER_SEND_STATUS* send_status);
	extern int twin_messenger_start(TWIN_MESSENGER_HANDLE twin_msgr_handle, SESSION_HANDLE session_handle); 
	extern int twin_messenger_stop(TWIN_MESSENGER_HANDLE twin_msgr_handle);
	extern void twin_messenger_do_work(TWIN_MESSENGER_HANDLE twin_msgr_handle);
	extern void twin_messenger_destroy(TWIN_MESSENGER_HANDLE twin_msgr_handle);
	extern int twin_messenger_set_option(TWIN_MESSENGER_HANDLE twin_msgr_handle, const char* name, void* value);
	extern OPTIONHANDLER_HANDLE twin_messenger_retrieve_options(TWIN_MESSENGER_HANDLE twin_msgr_handle);
```


### twin_messenger_create

```c
	TWIN_MESSENGER_HANDLE twin_messenger_create(const TWIN_MESSENGER_CONFIG* messenger_config);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_001: [**If parameter `messenger_config` is NULL, twin_messenger_create() shall return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_002: [**If `messenger_config`'s `device_id`, `iothub_host_fqdn` or `client_version` is NULL, twin_messenger_create() shall return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_003: [**twin_messenger_create() shall allocate memory for the messenger instance structure (aka `twin_msgr`)**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_004: [**If malloc() fails, twin_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_005: [**twin_messenger_create() shall save a copy of `messenger_config` info into `twin_msgr`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_006: [**If any `messenger_config` info fails to be copied, twin_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_007: [**`twin_msgr->pending_patches` shall be set using singlylinkedlist_create()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_008: [**If singlylinkedlist_create() fails, twin_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_009: [**`twin_msgr->operations` shall be set using singlylinkedlist_create()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_010: [**If singlylinkedlist_create() fails, twin_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_011: [**`twin_msgr->amqp_msgr` shall be set using amqp_messenger_create(), passing a AMQP_MESSENGER_CONFIG instance `amqp_msgr_config`**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_012: [**`amqp_msgr_config->client_version` shall be set with `twin_msgr->client_version`**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_013: [**`amqp_msgr_config->device_id` shall be set with `twin_msgr->device_id`**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_014: [**`amqp_msgr_config->iothub_host_fqdn` shall be set with `twin_msgr->iothub_host_fqdn`**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_015: [**`amqp_msgr_config` shall have "twin/" as send link target suffix and receive link source suffix**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_016: [**`amqp_msgr_config` shall have send and receive link attach properties set as "com.microsoft:client-version" = `twin_msgr->client_version`, "com.microsoft:channel-correlation-id" = `twin:<UUID>`, "com.microsoft:api-version" = "2016-11-14"**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_017: [**`amqp_msgr_config` shall be set with `on_amqp_messenger_state_changed_callback` and `on_amqp_messenger_subscription_changed_callback` callbacks**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_018: [**If amqp_messenger_create() fails, twin_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_019: [**`twin_msgr->amqp_msgr` shall subscribe for AMQP messages by calling amqp_messenger_subscribe_for_messages() passing `on_amqp_message_received`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_020: [**If amqp_messenger_subscribe_for_messages() fails, twin_messenger_create() shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_021: [**If no failures occurr, twin_messenger_create() shall return a handle to `twin_msgr`**]**  


### twin_messenger_report_state_async
```c
int twin_messenger_report_state_async(TWIN_TWIN_MESSENGER_HANDLE twin_msgr_handle, CONSTBUFFER_HANDLE data, ON_TWIN_MESSENGER_REPORT_STATE_COMPLETE_CALLBACK on_report_state_complete_callback, const void* context);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_022: [**If `twin_msgr_handle` or `data` are NULL, twin_messenger_report_state_async() shall fail and return a non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_023: [**twin_messenger_report_state_async() shall allocate memory for a TWIN_PATCH_OPERATION_CONTEXT structure (aka `twin_op_ctx`)**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_024: [**If malloc() fails, twin_messenger_report_state_async() shall fail and return a non-zero value**]**    

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_025: [**`twin_op_ctx` shall increment the reference count for `data` and store it.**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_027: [**`twin_op_ctx->time_enqueued` shall be set using get_time**]**    

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_028: [**If `twin_op_ctx->time_enqueued` fails to be set, twin_messenger_report_state_async() shall fail and return a non-zero value**]**    

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_029: [**`twin_op_ctx` shall be added to `twin_msgr->pending_patches` using singlylinkedlist_add()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_030: [**If singlylinkedlist_add() fails, twin_messenger_report_state_async() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_031: [**If any failure occurs, twin_messenger_report_state_async() shall free any memory it has allocated**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_032: [**If no failures occur, twin_messenger_report_state_async() shall return zero**]**  


### twin_messenger_get_send_status

```c
int twin_messenger_get_send_status(TWIN_MESSENGER_HANDLE twin_msgr_handle, TWIN_MESSENGER_SEND_STATUS* send_status);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_033: [**If `twin_msgr_handle` or `send_status` are NULL, twin_messenger_get_send_status() shall fail and return a non-zero value**]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_034: [**If `twin_msgr->pending_patches` or `twin_msgr->operations` have any TWIN patch requests, send_status shall be set to TWIN_MESSENGER_SEND_STATUS_BUSY**]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_035: [**Otherwise, send_status shall be set to TWIN_MESSENGER_SEND_STATUS_IDLE**]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_036: [**If no failures occur, twin_messenger_get_send_status() shall return 0**]** 


### twin_messenger_subscribe

```c
int twin_messenger_subscribe(TWIN_MESSENGER_HANDLE twin_msgr_handle, ON_TWIN_STATE_UPDATE_CALLBACK on_twin_state_update_callback, void* context);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_037: [**If `twin_msgr_handle` or `on_twin_state_update_callback` are NULL, twin_messenger_subscribe() shall fail and return a non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_038: [**If `twin_msgr` is already subscribed, twin_messenger_subscribe() shall return zero**]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_039: [**`on_twin_state_update_callback` and `context` shall be saved on `twin_msgr`**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_040: [**twin_messenger_subscribe() shall change `twin_msgr->subscription_state` to TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES**]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_041: [**If no failures occurr, twin_messenger_subscribe() shall return 0**]**  


### twin_messenger_unsubscribe

```c
int twin_messenger_unsubscribe(TWIN_MESSENGER_HANDLE twin_msgr_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_042: [**If `twin_msgr_handle` is NULL, twin_messenger_unsubscribe() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_043: [**twin_messenger_subscribe() shall change `twin_msgr->subscription_state` to TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBE**]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_044: [**If no failures occurr, twin_messenger_unsubscribe() shall return zero**]**  


### twin_messenger_get_twin_async

```c
int twin_messenger_get_twin_async(TWIN_MESSENGER_HANDLE twin_msgr_handle, TWIN_STATE_UPDATE_CALLBACK on_get_twin_completed_callback, void* context);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_109: [**If `twin_msgr_handle` or `on_twin_state_update_callback` are NULL, twin_messenger_get_twin_async() shall fail and return a non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_110: [** `on_get_twin_completed_callback` and `context` shall be saved **]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_111: [** An AMQP message shall be created to request a GET twin **]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_112: [** The AMQP message shall be sent to the twin send link **]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_113: [**If any failures occurr, twin_messenger_get_twin_async() shall return a non-zero value **]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_114: [**If no failures occurr, twin_messenger_get_twin_async() shall return 0 **]**  


### twin_messenger_start

```c
int twin_messenger_start(TWIN_MESSENGER_HANDLE twin_msgr_handle, SESSION_HANDLE session_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_045: [**If `twin_msgr_handle` or `session_handle` are NULL, twin_messenger_start() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_046: [**amqp_messenger_start() shall be invoked passing `twin_msgr->amqp_msgr` and `session_handle`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_047: [**If amqp_messenger_start() fails, twin_messenger_start() fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_048: [**If no failures occurr, `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_STARTING, and `twin_msgr->on_state_changed_callback` invoked if provided**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_049: [**If any failures occurr, `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_ERROR, and `twin_msgr->on_state_changed_callback` invoked if provided**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_050: [**If no failures occurr, twin_messenger_start() shall return 0**]**


### twin_messenger_stop

```c
int twin_messenger_stop(TWIN_MESSENGER_HANDLE twin_msgr_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_051: [**If `twin_msgr_handle` is NULL, twin_messenger_stop() shall fail and return a non-zero value**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_052: [**amqp_messenger_stop() shall be invoked passing `twin_msgr->amqp_msgr`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_053: [**If amqp_messenger_stop() fails, twin_messenger_stop() fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_054: [**`twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_STOPPING, and `twin_msgr->on_state_changed_callback` invoked if provided**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_055: [**If any failures occurr, `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_ERROR, and `twin_msgr->on_state_changed_callback` invoked if provided**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_115: [**If the client was subscribed for Twin updates, it must reset itself to continue receiving when twin_messenger_start is invoked **]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_056: [**If no failures occurr, twin_messenger_stop() shall return 0**]**


### twin_messenger_do_work
```c
void twin_messenger_do_work(TWIN_MESSENGER_HANDLE twin_msgr_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_057: [**If `twin_msgr_handle` is NULL, twin_messenger_do_work() shall return immediately**]**


#### Sending pending reported property PATCHES

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_058: [**If `twin_msgr->state` is TWIN_MESSENGER_STATE_STARTED, twin_messenger_do_work() shall send the PATCHES in `twin_msgr->pending_patches`, removing them from the list**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_059: [**If reported property PATCH shall be sent as an uAMQP MESSAGE_HANDLE instance using amqp_send_async() passing `on_amqp_send_complete_callback`**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_060: [**If amqp_send_async() fails, `on_report_state_complete_callback` shall be invoked with RESULT_ERROR and REASON_FAIL_SENDING**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_061: [**If any other failure occurs sending the PATCH request, `on_report_state_complete_callback` shall be invoked with RESULT_ERROR and REASON_INTERNAL_ERROR**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_062: [**If amqp_send_async() succeeds, the PATCH request shall be queued into `twin_msgr->operations`**]**


##### create_amqp_message_for_twin_operation
```c
MESSAGE_HANDLE create_amqp_message_for_twin_operation(TWIN_OPERATION_TYPE op_type, char* correlation_id, CONSTBUFFER_HANDLE data)
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_063: [**An `amqp_message` (MESSAGE_HANDLE) shall be created using message_create()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_064: [**If message_create() fails, message_create_for_twin_operation shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_065: [**`operation=<op_type>` (GET/PATCH/PUT/DELETE) must be added to the `amqp_message` annotations**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_066: [**If `op_type` is PATCH, `resource=/properties/reported` must be added to the `amqp_message` annotations**]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_067: [**If `op_type` is PUT or DELETE, `resource=/notifications/twin/properties/desired` must be added to the `amqp_message` annotations**]** 

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_068: [**The `correlation-id` property of `amqp_message` shall be set with an UUID string**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_069: [**If setting `correlation-id` fails, message_create_for_twin_operation shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_070: [**If `data` is not NULL, it shall be added to `amqp_message` using message_add_body_amqp_data()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_071: [**If message_add_body_amqp_data() fails, message_create_for_twin_operation shall fail and return NULL**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_072: [**If any errors occur, message_create_for_twin_operation shall release all memory it has allocated**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_073: [**If no errors occur, message_create_for_twin_operation shall return `amqp_message`**]**  


#### Handling report subscriptions

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_074: [**If subscribing, twin_messenger_do_work() shall request a complete desired properties report**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_075: [**If a complete desired report has been received, the TWIN messenger shall request partial updates to desired properties**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_076: [**If unsubscribing, twin_messenger_do_work() shall send a DELETE request to the service**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_078: [**If failures occur sending subscription requests to the service for more than 3 times, TWIN messenger shall set its state to TWIN_MESSENGER_STATE_ERROR and inform the user**]**


#### Time out verifications

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_080: [**twin_messenger_do_work() shall remove and destroy any timed out items from `twin_msgr->pending_patches` and `twin_msgr->operations`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_081: [**If a timed-out item is a reported property PATCH, `on_report_state_complete_callback` shall be invoked with RESULT_ERROR and REASON_TIMEOUT**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_082: [**If any failure occurs while verifying/removing timed-out items `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_ERROR and user informed**]**  


#### AMQP messenger do work

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_083: [**twin_messenger_do_work() shall invoke amqp_messenger_do_work() passing `twin_msgr->amqp_msgr`**]**  


#### on_amqp_message_received_callback
```c
static AMQP_MESSENGER_DISPOSITION_RESULT on_amqp_message_received_callback(MESSAGE_HANDLE message, AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_084: [**If `message` or `context` are NULL, on_amqp_message_received_callback shall return immediately**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_085: [**If `message` is a success response for a PATCH request, the `on_report_state_complete_callback` shall be invoked if provided passing RESULT_SUCCESS and the status_code received**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_086: [**If `message` is a failed response for a PATCH request, the `on_report_state_complete_callback` shall be invoked if provided passing RESULT_ERROR and the status_code zero**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_087: [**If `message` is a success response for a GET request, `on_message_received_callback` shall be invoked with TWIN_UPDATE_TYPE_COMPLETE and the message body received**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_088: [**If `message` is a success response for a GET request, the TWIN messenger shall trigger the subscription for partial updates**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_089: [**If `message` is a failed response for a GET request, the TWIN messenger shall attempt to send another GET request**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_090: [**If `message` is a failed response for a PUT request, the TWIN messenger shall attempt to send another PUT request**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_091: [**If `message` is a failed response for a DELETE request, the TWIN messenger shall attempt to send another DELETE request**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_092: [**The corresponding TWIN request shall be removed from `twin_msgr->operations` and destroyed**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_093: [**The corresponding TWIN request failed to be removed from `twin_msgr->operations`, `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_ERROR and informed to the user**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_094: [**If `message` is not a client request, `on_message_received_callback` shall be invoked with TWIN_UPDATE_TYPE_PARTIAL and the message body received**]**  


#### on_amqp_send_complete_callback

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_095: [**If operation is a reported state PATCH, if a failure occurs `on_report_state_complete_callback` shall be invoked with TWIN_REPORT_STATE_RESULT_ERROR, status code from the AMQP response and the saved context**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_096: [**If operation is a GET/PUT/DELETE, if a failure occurs the TWIN messenger shall attempt to subscribe/unsubscribe again**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_097: [**If a failure occurred, the current operation shall be removed from `twin_msgr->operations`**]**  


## twin_messenger_destroy

```c
void twin_messenger_destroy(TWIN_MESSENGER_HANDLE twin_msgr_handle);
```

Destroys all the components within the messenger data structure (twin_twin_msgr_handle) and releases any allocated memory.

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_098: [**If `twin_msgr_handle` is NULL, twin_messenger_destroy() shall return immediately**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_099: [**`twin_msgr->amqp_messenger` shall be destroyed using amqp_messenger_destroy()**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_100: [**All elements of `twin_msgr->pending_patches` shall be removed, invoking `on_report_state_complete_callback` for each with TWIN_REPORT_STATE_REASON_MESSENGER_DESTROYED**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_101: [**All elements of `twin_msgr->operations` shall be removed, invoking `on_report_state_complete_callback` for each PATCH with TWIN_REPORT_STATE_REASON_MESSENGER_DESTROYED**]**  

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_102: [**twin_messenger_destroy() shall release all memory allocated for and within `twin_msgr`**]**  


## twin_messenger_set_option

```c
int twin_messenger_set_option(TWIN_MESSENGER_HANDLE twin_msgr_handle, const char* name, void* value);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_103: [**If `twin_msgr_handle` or `name` or `value` are NULL, twin_messenger_set_option() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_104: [**amqp_messenger_set_option() shall be invoked passing `name` and `option`**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_105: [**If amqp_messenger_set_option() fails, twin_messenger_set_option() shall fail and return a non-zero value**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_106: [**If no errors occur, twin_messenger_set_option shall return zero**]**


## twin_messenger_retrieve_options

```c
OPTIONHANDLER_HANDLE twin_messenger_retrieve_options(TWIN_MESSENGER_HANDLE twin_msgr_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_107: [**If `twin_msgr_handle` is NULL, twin_messenger_retrieve_options shall fail and return NULL**]**

**SRS_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_108: [**twin_messenger_retrieve_options() shall return the result of amqp_messenger_retrieve_options()**]**
