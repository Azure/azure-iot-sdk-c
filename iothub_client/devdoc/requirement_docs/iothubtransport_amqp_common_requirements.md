# IoTHubTransport_AMQP_Common Requirements
================

## Overview

This module provides an implementation of the transport layer of the IoT Hub client based on the AMQP API, which implements the AMQP protocol.  
It is the base for the implentation of the actual respective AMQP transports, which will on their side provide the underlying I/O transport for this module.


## Dependencies

This module will depend on the following modules:

azure-c-shared-utility
azure-uamqp-c
iothubtransport_amqp_connection
iothubtransport_amqp_device


## Exposed API

```c
typedef XIO_HANDLE(*AMQP_GET_IO_TRANSPORT)(const char* target_fqdn, const AMQP_TRANSPORT_PROXY_OPTIONS* amqp_transport_proxy_options);

extern TRANSPORT_LL_HANDLE IoTHubTransport_AMQP_Common_Create(const IOTHUBTRANSPORT_CONFIG* config, AMQP_GET_IO_TRANSPORT get_io_transport);
extern void IoTHubTransport_AMQP_Common_Destroy(TRANSPORT_LL_HANDLE handle);
extern int IoTHubTransport_AMQP_Common_Subscribe(IOTHUB_DEVICE_HANDLE handle);
extern void IoTHubTransport_AMQP_Common_Unsubscribe(IOTHUB_DEVICE_HANDLE handle);
extern IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_SendMessageDisposition(MESSAGE_CALLBACK_INFO* messageData, IOTHUBMESSAGE_DISPOSITION_RESULT disposition);
extern int IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle);
extern void IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle);
extern IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_GetDeviceTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext);
extern int IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle);
extern void IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle);
extern IOTHUB_PROCESS_ITEM_RESULT IoTHubTransport_AMQP_Common_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item);
extern void IoTHubTransport_AMQP_Common_DoWork(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle);
extern IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_GetSendStatus(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS* iotHubClientStatus);
extern IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value);
extern int IoTHubTransport_AMQP_Common_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds);
extern IOTHUB_DEVICE_HANDLE IoTHubTransport_AMQP_Common_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, PDLIST_ENTRY waitingToSend);
extern void IoTHubTransport_AMQP_Common_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle);
extern STRING_HANDLE IoTHubTransport_AMQP_Common_GetHostname(TRANSPORT_LL_HANDLE handle);

```


Note: `instance` refers to the structure that holds the current state and control parameters of the transport. 
In each function (other than IoTHubTransport_AMQP_Common_Create) it shall derive from the TRANSPORT_LL_HANDLE handle passed as argument.  


### IoTHubTransport_AMQP_Common_GetHostname
```c
 STRING_HANDLE IoTHubTransport_AMQP_Common_GetHostname(TRANSPORT_LL_HANDLE handle)
```

IoTHubTransport_AMQP_Common_GetHostname provides a STRING_HANDLE containing the hostname with which the transport has been created.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_02_001: [**If `handle` is NULL, `IoTHubTransport_AMQP_Common_GetHostname` shall return NULL.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_02_002: [**IoTHubTransport_AMQP_Common_GetHostname shall return a copy of `instance->iothub_target_fqdn`.**]**


### IoTHubTransport_AMQP_Common_Create

```c
TRANSPORT_LL_HANDLE IoTHubTransport_AMQP_Common_Create(const IOTHUBTRANSPORT_CONFIG* config, AMQP_GET_IO_TRANSPORT get_io_transport)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_001: [**If `config` or `config->upperConfig` or `get_io_transport` are NULL then IoTHubTransport_AMQP_Common_Create shall fail and return NULL.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_002: [**IoTHubTransport_AMQP_Common_Create shall fail and return NULL if `config->upperConfig->protocol` is NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_003: [**Memory shall be allocated for the transport's internal state structure (`instance`)**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_004: [**If malloc() fails, IoTHubTransport_AMQP_Common_Create shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_124: [**`instance->connection_retry_control` shall be set using retry_control_create(), passing defaults EXPONENTIAL_BACKOFF_WITH_JITTER and 0**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_125: [**If retry_control_create() fails, IoTHubTransport_AMQP_Common_Create shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_005: [**If `config->upperConfig->protocolGatewayHostName` is NULL, `instance->iothub_target_fqdn` shall be set as `config->upperConfig->iotHubName` + "." + `config->upperConfig->iotHubSuffix`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_006: [**If `config->upperConfig->protocolGatewayHostName` is not NULL, `instance->iothub_target_fqdn` shall be set with a copy of it**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_007: [**If `instance->iothub_target_fqdn` fails to be set, IoTHubTransport_AMQP_Common_Create shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_008: [**`instance->registered_devices` shall be set using singlylinkedlist_create()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_009: [**If singlylinkedlist_create() fails, IoTHubTransport_AMQP_Common_Create shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_010: [**`get_io_transport` shall be saved on `instance->underlying_io_transport_provider`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_011: [**If IoTHubTransport_AMQP_Common_Create fails it shall free any memory it allocated**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_012: [**If IoTHubTransport_AMQP_Common_Create succeeds it shall return a pointer to `instance`.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_12_002: [**The connection idle timeout parameter default value shall be set to 240000 milliseconds using connection_set_idle_timeout()**]**



 
### IoTHubTransport_AMQP_Common_Destroy

```c
void IoTHubTransport_AMQP_Common_Destroy(TRANSPORT_LL_HANDLE handle)
```

This function will close connection established through AMQP API, as well as destroy all the components allocated internally for its proper functionality.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_013: [**If `handle` is NULL, IoTHubTransport_AMQP_Common_Destroy shall return immediatelly**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_014: [**IoTHubTransport_AMQP_Common_Destroy shall invoke IoTHubTransport_AMQP_Common_Unregister on each of its registered devices.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_015: [**All members of `instance` (including tls_io) shall be destroyed and its memory released**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_043: [** `IoTHubTransport_AMQP_Common_Destroy` shall free the stored proxy options. **]**

### IoTHubTransport_AMQP_Common_DoWork

```c
void IoTHubTransport_AMQP_Common_DoWork(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
```  

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_016: [**If `handle` is NULL, IoTHubTransport_AMQP_Common_DoWork shall return without doing any work**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_017: [**If `instance->state` is `RECONNECTION_REQUIRED`, IoTHubTransport_AMQP_Common_DoWork shall attempt to trigger the connection-retry logic and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_126: [**The connection retry shall be attempted only if retry_control_should_retry() returns RETRY_ACTION_NOW, or if it fails**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_018: [**If there are no devices registered on the transport, IoTHubTransport_AMQP_Common_DoWork shall skip do_work for devices**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_019: [**If `instance->amqp_connection` is NULL, it shall be established**]**
Note: see section "Connection Establishment" below.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_020: [**If the amqp_connection is OPENED, the transport shall iterate through each registered device and perform a device-specific do_work on each**]**
Note: see section "Per-Device DoWork Requirements" below.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_021: [**If DoWork fails for the registered device for more than MAX_NUMBER_OF_DEVICE_FAILURES, connection retry shall be triggered**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_022: [**If `instance->amqp_connection` is not NULL, amqp_connection_do_work shall be invoked**]**


#### Connection Establishment

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_023: [**If `instance->tls_io` is NULL, it shall be set invoking instance->underlying_io_transport_provider()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_024: [**If instance->underlying_io_transport_provider() fails, IoTHubTransport_AMQP_Common_DoWork shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_025: [**When `instance->tls_io` is created, it shall be set with `instance->saved_tls_options` using OptionHandler_FeedOptions()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_111: [**If OptionHandler_FeedOptions() fails, it shall be ignored**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_026: [**If `transport->connection` is NULL, it shall be created using amqp_connection_create()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_027: [**If `transport->preferred_authentication_method` is CBS, AMQP_CONNECTION_CONFIG shall be set with `create_sasl_io` = true and `create_cbs_connection` = true**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_028: [**If `transport->preferred_credential_method` is X509, AMQP_CONNECTION_CONFIG shall be set with `create_sasl_io` = false and `create_cbs_connection` = false**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_029: [**`instance->is_trace_on` shall be set into `AMQP_CONNECTION_CONFIG->is_trace_on`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_030: [**If amqp_connection_create() fails, IoTHubTransport_AMQP_Common_DoWork shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_110: [**If amqp_connection_create() succeeds, IoTHubTransport_AMQP_Common_DoWork shall proceed to invoke amqp_connection_do_work**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_12_003: [** AMQP connection will be configured using the `c2d_keep_alive_freq_secs` value from SetOption **]**

#### Connection-Retry Logic

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_031: [**amqp_device_stop() shall be invoked on all `instance->registered_devices` that are not already stopped**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_032: [** Each `instance->registered_devices` shall unsubscribe from receiving C2D method requests by calling `iothubtransportamqp_methods_unsubscribe`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_033: [**`instance->connection` shall be destroyed using amqp_connection_destroy()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_034: [**`instance->tls_io` options shall be saved on `instance->saved_tls_options` using xio_retrieveoptions()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_035: [**`instance->tls_io` shall be destroyed using xio_destroy()**]**

Note: all the components above will be re-created and re-started on the next call to IoTHubTransport_AMQP_Common_DoWork.


#### Per-Device DoWork Requirements

##### Starting the DEVICE_HANDLE

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_036: [**If the device state is DEVICE_STATE_STOPPED, it shall be started**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_037: [**If transport is using CBS authentication, amqp_connection_get_cbs_handle() shall be invoked on `instance->connection`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_038: [**If amqp_connection_get_cbs_handle() fails, IoTHubTransport_AMQP_Common_DoWork shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_039: [**amqp_connection_get_session_handle() shall be invoked on `instance->connection`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_040: [**If amqp_connection_get_session_handle() fails, IoTHubTransport_AMQP_Common_DoWork shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_041: [**The device handle shall be started using amqp_device_start_async()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_042: [**If amqp_device_start_async() fails, IoTHubTransport_AMQP_Common_DoWork shall fail and skip to the next registered device**]**


##### DEVICE_HANDLE Error Control

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_043: [**If the device handle is in state DEVICE_STATE_STARTING or DEVICE_STATE_STOPPING, it shall be checked for state change timeout**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_044: [**If the device times out in state DEVICE_STATE_STARTING or DEVICE_STATE_STOPPING, the registered device shall be marked with failure**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_045: [**If the registered device has a failure, it shall be stopped using amqp_device_stop()**]**
Note: this will cause the device to be restarted on the next call to DoWork.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_046: [**If the device has failed for MAX_NUMBER_OF_DEVICE_FAILURES in a row, it shall trigger a connection retry on the transport**]**


##### Device Methods
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_031: [** Once the device is authenticated, `iothubtransportamqp_methods_subscribe` shall be invoked (subsequent DoWork calls shall not call it if already subscribed). **]**


##### Send pending events

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_047: [**If the registered device is started, each event on `registered_device->wait_to_send_list` shall be removed from the list and sent using amqp_device_send_event_async()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_048: [**amqp_device_send_event_async() shall be invoked passing `on_event_send_complete`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_049: [**If amqp_device_send_event_async() fails, `on_event_send_complete` shall be invoked passing EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING and return**]**


###### on_event_send_complete
```c
static void on_event_send_complete(IOTHUB_MESSAGE_LIST* message, D2C_EVENT_SEND_RESULT result, void* context)
``` 

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_050: [**If result is D2C_EVENT_SEND_COMPLETE_RESULT_OK, `iothub_send_result` shall be set using IOTHUB_CLIENT_CONFIRMATION_OK**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_051: [**If result is D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE, `iothub_send_result` shall be set using IOTHUB_CLIENT_CONFIRMATION_ERROR**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_052: [**If result is D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING, `iothub_send_result` shall be set using IOTHUB_CLIENT_CONFIRMATION_ERROR**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_053: [**If result is D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT, `iothub_send_result` shall be set using IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_054: [**If result is D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED, `iothub_send_result` shall be set using IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_055: [**If result is D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_UNKNOWN, `iothub_send_result` shall be set using IOTHUB_CLIENT_CONFIRMATION_ERROR**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_056: [**If `message->callback` is not NULL, it shall invoked with the `iothub_send_result`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_057: [**`message->messageHandle` shall be destroyed using IoTHubMessage_Destroy**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_058: [**`message` shall be destroyed using free**]**


#### on_amqp_connection_state_changed

```c
static void on_amqp_connection_state_changed(const void* context, AMQP_CONNECTION_STATE old_state, AMQP_CONNECTION_STATE new_state)
```
This handler is provided when amqp_connection_create() is invoked.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_059: [**`new_state` shall be saved in to the transport instance**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_060: [**If `new_state` is AMQP_CONNECTION_STATE_ERROR, the connection shall be flagged as faulty (so the connection retry logic can be triggered)**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_115: [**If the AMQP connection is closed by the service side, the connection retry logic shall be triggered**]**


#### on_device_state_changed_callback

```c
static void on_device_state_changed_callback(void* context, DEVICE_STATE previous_state, DEVICE_STATE new_state)
```

This handler is provided to each registered device when messenger_create() is invoked.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_061: [**If `new_state` is the same as `previous_state`, on_device_state_changed_callback shall return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_062: [**If `new_state` shall be saved into the `registered_device` instance**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_063: [**If `registered_device->time_of_last_state_change` shall be set using get_time()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_127: [**If `new_state` is DEVICE_STATE_STARTED, retry_control_reset() shall be invoked passing `instance->connection_retry_control`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_120: [**If `new_state` is DEVICE_STATE_STARTED, IoTHubClient_LL_ConnectionStatusCallBack shall be invoked with IOTHUB_CLIENT_CONNECTION_AUTHENTICATED and IOTHUB_CLIENT_CONNECTION_OK**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_121: [**If `new_state` is DEVICE_STATE_STOPPED, IoTHubClient_LL_ConnectionStatusCallBack shall be invoked with IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED and IOTHUB_CLIENT_CONNECTION_OK**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_122: [**If `new_state` is DEVICE_STATE_ERROR_AUTH, IoTHubClient_LL_ConnectionStatusCallBack shall be invoked with IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED and IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_123: [**If `new_state` is DEVICE_STATE_ERROR_AUTH_TIMEOUT or DEVICE_STATE_ERROR_MSG, IoTHubClient_LL_ConnectionStatusCallBack shall be invoked with IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED and IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR**]**


### IoTHubTransport_AMQP_Common_Register

```c
IOTHUB_DEVICE_HANDLE IoTHubTransport_AMQP_Common_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, PDLIST_ENTRY waitingToSend)
```

This function registers a device with the transport.  The AMQP transport only supports a single device established on create, so this function will prevent multiple devices from being registered.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_17_005: [**If `handle`, `device`, `iotHubClientHandle` or `waitingToSend` is NULL, IoTHubTransport_AMQP_Common_Register shall return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_03_002: [**IoTHubTransport_AMQP_Common_Register shall return NULL if `device->deviceId` is NULL.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_064: [**If the device is already registered, IoTHubTransport_AMQP_Common_Register shall fail and return NULL.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_065: [**IoTHubTransport_AMQP_Common_Register shall fail and return NULL if the device is not using an authentication mode compatible with the currently used by the transport.**]**

Note: There should be no devices using different authentication modes registered on the transport at the same time (i.e., either all registered devices use CBS authentication, or all use x509 certificate authentication). 

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_066: [**IoTHubTransport_AMQP_Common_Register shall allocate an instance of AMQP_TRANSPORT_DEVICE_STATE to store the state of the new registered device.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_067: [**If malloc fails, IoTHubTransport_AMQP_Common_Register shall fail and return NULL.**]**

Note: the instance of AMQP_TRANSPORT_DEVICE_STATE will be referred below as `amqp_device_instance`.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_068: [**IoTHubTransport_AMQP_Common_Register shall save the handle references to the IoTHubClient, transport, waitingToSend list on `amqp_device_instance`.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_069: [**A copy of `config->deviceId` shall be saved into `device_state->device_id`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_070: [**If STRING_construct() fails, IoTHubTransport_AMQP_Common_Register shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_071: [**`amqp_device_instance->device_handle` shall be set using amqp_device_create()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_072: [**The configuration for amqp_device_create shall be set according to the authentication preferred by IOTHUB_DEVICE_CONFIG**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_073: [**If amqp_device_create() fails, IoTHubTransport_AMQP_Common_Register shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_010: [** `IoTHubTransport_AMQP_Common_Register` shall create a new iothubtransportamqp_methods instance by calling `iothubtransportamqp_methods_create` while passing to it the the fully qualified domain name, the device Id, and optional module Id.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_011: [** If `iothubtransportamqp_methods_create` fails, `IoTHubTransport_AMQP_Common_Register` shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_074: [**IoTHubTransport_AMQP_Common_Register shall add the `amqp_device_instance` to `instance->registered_devices`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_075: [**If it fails to add `amqp_device_instance`, IoTHubTransport_AMQP_Common_Register shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_076: [**If the device is the first being registered on the transport, IoTHubTransport_AMQP_Common_Register shall save its authentication mode as the transport preferred authentication mode**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_077: [**If IoTHubTransport_AMQP_Common_Register fails, it shall free all memory it allocated**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_078: [**IoTHubTransport_AMQP_Common_Register shall return a handle to `amqp_device_instance` as a IOTHUB_DEVICE_HANDLE**]**


### IoTHubTransport_AMQP_Common_Unregister

```c
void IoTHubTransport_AMQP_Common_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
```

This function is intended to remove a device if it is registered with the transport.  

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_079: [**if `deviceHandle` provided is NULL, IoTHubTransport_AMQP_Common_Unregister shall return.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_080: [**if `deviceHandle` has a NULL reference to its transport instance, IoTHubTransport_AMQP_Common_Unregister shall return.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_081: [**If the device is not registered with this transport, IoTHubTransport_AMQP_Common_Unregister shall return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_082: [**`device_instance` shall be removed from `instance->registered_devices`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_012: [**IoTHubTransport_AMQP_Common_Unregister shall destroy the C2D methods handler by calling iothubtransportamqp_methods_destroy**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_083: [**IoTHubTransport_AMQP_Common_Unregister shall free all the memory allocated for the `device_instance`**]**


### IoTHubTransport_AMQP_Common_Subscribe

```c
int IoTHubTransport_AMQP_Common_Subscribe(IOTHUB_DEVICE_HANDLE handle)
```

This function enables the transport to notify the upper client layer of new messages received from the cloud to the device.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_084: [**If `handle` is NULL, IoTHubTransport_AMQP_Common_Subscribe shall return a non-zero result**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_085: [**If `amqp_device_instance` is not registered, IoTHubTransport_AMQP_Common_Subscribe shall return a non-zero result**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_086: [**amqp_device_subscribe_message() shall be invoked passing `on_message_received_callback`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_087: [**If amqp_device_subscribe_message() fails, IoTHubTransport_AMQP_Common_Subscribe shall return a non-zero result**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_088: [**If no failures occur, IoTHubTransport_AMQP_Common_Subscribe shall return 0**]**


#### on_message_received

```c
static DEVICE_MESSAGE_DISPOSITION_RESULT on_message_received(IOTHUB_MESSAGE_HANDLE iothub_message, void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_089: [**IoTHubClient_LL_MessageCallback() shall be invoked passing the client and the incoming message handles as parameters**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_090: [**If IoTHubClient_LL_MessageCallback() fails, on_message_received_callback shall return DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_091: [**If IoTHubClient_LL_MessageCallback() succeeds, on_message_received_callback shall return DEVICE_MESSAGE_DISPOSITION_RESULT_NONE**]**


### IoTHubTransport_AMQP_Common_Unsubscribe

```c
void IoTHubTransport_AMQP_Common_Unsubscribe(IOTHUB_DEVICE_HANDLE handle)
```

This function disables the notifications to the upper client layer of new messages received from the cloud to the device.

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_093: [**If `handle` is NULL, IoTHubTransport_AMQP_Common_Subscribe shall return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_094: [**If `amqp_device_instance` is not registered, IoTHubTransport_AMQP_Common_Subscribe shall return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_095: [**amqp_device_unsubscribe_message() shall be invoked passing `amqp_device_instance->device_handle`**]**


### IoTHubTransport_AMQP_Common_SendMessageDisposition
```c
IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_SendMessageDisposition(MESSAGE_CALLBACK_INFO* messageData, IOTHUBMESSAGE_DISPOSITION_RESULT disposition);
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_10_001: [** If `messageData` is `NULL`, `IoTHubTransport_AMQP_Common_SendMessageDisposition` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_10_002: [** If any of the `messageData` fields are `NULL`, `IoTHubTransport_AMQP_Common_SendMessageDisposition` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_10_003: [** `IoTHubTransport_AMQP_Common_SendMessageDisposition` shall fail and return `IOTHUB_CLIENT_ERROR` if the POST message fails, otherwise return `IOTHUB_CLIENT_OK`. **]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_112: [**A DEVICE_MESSAGE_DISPOSITION_INFO instance shall be created with a copy of the `link_name` and `message_id` contained in `message_data`**]**  

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_113: [**If the DEVICE_MESSAGE_DISPOSITION_INFO fails to be created, `IoTHubTransport_AMQP_Common_SendMessageDisposition()` shall fail and return IOTHUB_CLIENT_ERROR**]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_10_004: [**IoTHubTransport_AMQP_Common_SendMessageDisposition shall convert the given IOTHUBMESSAGE_DISPOSITION_RESULT to the equivalent AMQP_VALUE and will return the result of calling messagereceiver_send_message_disposition. **]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_114: [**`IoTHubTransport_AMQP_Common_SendMessageDisposition()` shall destroy the DEVICE_MESSAGE_DISPOSITION_INFO instance**]**  

  
### IoTHubTransport_AMQP_Common_GetSendStatus

```c
IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_GetSendStatus(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_096: [**If `handle` or `iotHubClientStatus` are NULL, IoTHubTransport_AMQP_Common_GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_097: [**IoTHubTransport_AMQP_Common_GetSendStatus shall invoke amqp_device_get_send_status()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_098: [**If amqp_device_get_send_status() fails, IoTHubTransport_AMQP_Common_GetSendStatus shall return IOTHUB_CLIENT_ERROR**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_099: [**If amqp_device_get_send_status() returns DEVICE_SEND_STATUS_BUSY, IoTHubTransport_AMQP_Common_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_BUSY**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_100: [**If amqp_device_get_send_status() returns DEVICE_SEND_STATUS_IDLE, IoTHubTransport_AMQP_Common_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_IDLE**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_109: [**If no failures occur, IoTHubTransport_AMQP_Common_GetSendStatus shall return IOTHUB_CLIENT_OK**]**

  
### IoTHubTransport_AMQP_Common_SetOption

```c
IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
```

Summary of options:

|Parameter              |Possible Values               |Details                                          |
|-----------------------|------------------------------|-------------------------------------------------|
|TrustedCerts           |                              |Sets the certificate to be used by the transport.|
|sas_token_lifetime     | 0 to TIME_MAX (seconds)      |Default: 3600 seconds (1 hour)	How long a SAS token created by the transport is valid, in seconds.|
|sas_token_refresh_time | 0 to TIME_MAX (seconds)      |Default: sas_token_lifetime/2	Maximum period of time for the transport to wait before refreshing the SAS token it created previously.|
|cbs_request_timeout    | 1 to TIME_MAX (seconds)      |Default: 30 seconds	Maximum time the transport waits for AMQP cbs_put_token() to complete before marking it a failure.|
|event_send_timeout_in_secs| 0 to TIME_MAX (seconds)   |Default: 600 seconds|
|x509certificate        | const char*                  |Default: NONE. An x509 certificate in PEM format |
|x509privatekey         | const char*                  |Default: NONE. An x509 RSA private key in PEM format|
|logtrace               | true or false                |Default: false|
|proxy_data             | *                            |Default: N/A|


**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_101: [**If `handle`, `option` or `value` are NULL then IoTHubTransport_AMQP_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.**]**


**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_102: [**If `option` is a device-specific option, it shall be saved and applied to each registered device using amqp_device_set_option()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_103: [**If amqp_device_set_option() fails, IoTHubTransport_AMQP_Common_SetOption shall return IOTHUB_CLIENT_ERROR**]**

Note: device-specific options: sas_token_lifetime, sas_token_refresh_time, cbs_request_timeout, event_send_timeout_in_secs

The following requirements only apply to x509 authentication:
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_02_007: [** If `option` is `x509certificate` and the transport preferred authentication method is not x509 then IoTHubTransport_AMQP_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_02_008: [** If `option` is `x509privatekey` and the transport preferred authentication method is not x509 then IoTHubTransport_AMQP_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. **]**

The remaining requirements apply independent of the authentication mode:
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_104: [**If `option` is `logtrace`, `value` shall be saved and applied to `instance->connection` using amqp_connection_set_logging()**]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_105: [**If `option` does not match one of the options handled by this module, it shall be passed to `instance->tls_io` using xio_setoption()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_106: [**If `instance->tls_io` is NULL, it shall be set invoking instance->underlying_io_transport_provider()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_107: [**If instance->underlying_io_transport_provider() fails, IoTHubTransport_AMQP_Common_SetOption shall fail and return IOTHUB_CLIENT_ERROR**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_108: [**When `instance->tls_io` is created, IoTHubTransport_AMQP_Common_SetOption shall apply `instance->saved_tls_options` with OptionHandler_FeedOptions()**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_03_001: [**If xio_setoption fails, IoTHubTransport_AMQP_Common_SetOption shall return IOTHUB_CLIENT_ERROR.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_03_001: [**If no failures occur, IoTHubTransport_AMQP_Common_SetOption shall return IOTHUB_CLIENT_OK.**]**

The following requirements apply to `proxy_data`:
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_032: [** If `option` is `proxy_data`, `value` shall be used as an `HTTP_PROXY_OPTIONS*`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_033: [** The fields `host_address`, `port`, `username` and `password` shall be saved for later used (needed when creating the underlying IO to be used by the transport). **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_034: [** If `host_address` is NULL, `IoTHubTransport_AMQP_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_035: [** If copying `host_address`, `username` or `password` fails, `IoTHubTransport_AMQP_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_036: [** `username` and `password` shall be allowed to be NULL. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_037: [** If only one of `username` and `password` is NULL, `IoTHubTransport_AMQP_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_038: [** If the underlying IO has already been created, then `IoTHubTransport_AMQP_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_039: [** If setting the `proxy_data` option succeeds, `IoTHubTransport_AMQP_Common_SetOption` shall return `IOTHUB_CLIENT_OK` **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_040: [** When setting the proxy options succeeds any previously saved proxy options shall be freed. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_041: [** If the `proxy_data` option has been set, the proxy options shall be filled in the argument `amqp_transport_proxy_options` when calling the function `underlying_io_transport_provider()` to obtain the underlying IO handle. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_042: [** If no `proxy_data` option has been set, NULL shall be passed as the argument `amqp_transport_proxy_options` when calling the function `underlying_io_transport_provider()`. **]**

### IoTHubTransport_AMQP_Common_SetRetryPolicy
```c
int IoTHubTransport_AMQP_Common_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds);
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_128: [**If `handle` is NULL, `IoTHubTransport_AMQP_Common_SetRetryPolicy` shall fail and return non-zero.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_129: [**`transport_instance->connection_retry_control` shall be set using retry_control_create(), passing `retryPolicy` and `retryTimeoutLimitInSeconds`.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_130: [**If retry_control_create() fails, `IoTHubTransport_AMQP_Common_SetRetryPolicy` shall fail and return non-zero.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_128: [**If no errors occur, `IoTHubTransport_AMQP_Common_SetRetryPolicy` shall return zero.**]**



### IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin
```c
int IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_131: [**If `handle` is NULL, `IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin` shall fail and return non-zero.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_134: [**amqp_device_subscribe_for_twin_updates() shall be invoked for the registered device, passing `on_device_twin_update_received_callback`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_135: [**If amqp_device_subscribe_for_twin_updates() fails, `IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin` shall fail and return non-zero.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_136: [**If no errors occur, `IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin` shall return zero.**]**

#### on_device_twin_update_received_callback
```
static void on_device_twin_update_received_callback(DEVICE_TWIN_UPDATE_TYPE update_type, const unsigned char* message, size_t length, void* context)
``` 

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_137: [**If `context` is NULL, the callback shall return.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_138: [**If `update_type` is DEVICE_TWIN_UPDATE_TYPE_PARTIAL IoTHubClient_LL_RetrievePropertyComplete shall be invoked passing `context` as handle, `DEVICE_TWIN_UPDATE_PARTIAL`, `payload` and `size`.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_139: [**If `update_type` is DEVICE_TWIN_UPDATE_TYPE_COMPLETE IoTHubClient_LL_RetrievePropertyComplete shall be invoked passing `context` as handle, `DEVICE_TWIN_UPDATE_COMPLETE`, `payload` and `size`.**]**


### IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin
```c
void IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_140: [**If `handle` is NULL, `IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin` shall return.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_142: [**amqp_device_unsubscribe_for_twin_updates() shall be invoked for the registered device**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_143: [**If `amqp_device_unsubscribe_for_twin_updates` fails, the error shall be ignored**]**


### IoTHubTransport_AMQP_Common_GetDeviceTwinAsync
```c
IOTHUB_CLIENT_RESULT IoTHubTransport_AMQP_Common_GetDeviceTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext);
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_154: [** If `handle` or `completionCallback` are NULL, `IoTHubTransport_AMQP_Common_GetDeviceTwinAsync` shall fail and return IOTHUB_CLIENT_INVALID_ARG **]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_155: [** amqp_device_get_twin_async() shall be invoked for the registered device, passing `on_device_get_twin_completed_callback`**]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_156: [** If amqp_device_get_twin_async() fails, `IoTHubTransport_AMQP_Common_GetDeviceTwinAsync` shall fail and return IOTHUB_CLIENT_ERROR **]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_157: [** If no errors occur, `IoTHubTransport_AMQP_Common_GetDeviceTwinAsync` shall return IOTHUB_CLIENT_OK **]**


#### on_device_get_twin_completed_callback
```c
static void on_device_get_twin_completed_callback(DEVICE_TWIN_UPDATE_TYPE update_type, const unsigned char* message, size_t length, void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_158: [** If `message` or `context` are NULL, the callback shall do nothing and return. **]**

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_159: [** `message` and `length` shall be passed to the callback `completionCallback` provided in IoTHubTransport_AMQP_Common_GetDeviceTwinAsync. **]**


### IoTHubTransport_AMQP_Common_ProcessItem
```c
IOTHUB_PROCESS_ITEM_RESULT IoTHubTransport_AMQP_Common_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
```

This function was introduced to be generic point for processing user requests, however it was been used only for device Twin. 

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_144: [**If `handle` or `iothub_item` are NULL, `IoTHubTransport_AMQP_Common_ProcessItem` shall fail and return IOTHUB_PROCESS_ERROR.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_145: [**If `item_type` is not IOTHUB_TYPE_DEVICE_TWIN, `IoTHubTransport_AMQP_Common_ProcessItem` shall fail and return IOTHUB_PROCESS_ERROR.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_146: [**amqp_device_send_twin_update_async() shall be invoked passing `iothub_item->device_twin->report_data_handle` and `on_device_send_twin_update_complete_callback`**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_147: [**If amqp_device_send_twin_update_async() fails, `IoTHubTransport_AMQP_Common_ProcessItem` shall fail and return IOTHUB_PROCESS_ERROR.**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_150: [**If no errors occur, `IoTHubTransport_AMQP_Common_ProcessItem` shall return IOTHUB_PROCESS_OK.**]**


#### on_device_send_twin_update_complete_callback
```c
static void on_device_send_twin_update_complete_callback(DEVICE_TWIN_UPDATE_RESULT result, int status_code, void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_151: [**If `context` is NULL, the callback shall return**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_152: [**`IoTHubClient_LL_ReportedStateComplete` shall be invoked passing `status_code` and `context` details**]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_09_153: [**The memory allocated for `context` shall be released**]**


### IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod
```c
int IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_026: [** `IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod` shall remember that a subscribe is to be performed in the next call to DoWork and on success it shall return 0. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_004: [** If `handle` is NULL, `IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod` shall fail and return a non-zero value. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_005: [** If the transport is already subscribed to receive C2D method requests, `IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod` shall perform no additional action and return 0. **]**


### IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod
```c
void IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_006: [** If `handle` is NULL, `IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod` shall do nothing. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_007: [** `IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod` shall unsubscribe from receiving C2D method requests by calling `iothubtransportamqp_methods_unsubscribe`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_008: [** If the transport is not subscribed to receive C2D method requests then `IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod` shall do nothing. **]**


### on_methods_request_received

```c
void on_methods_request_received(void* context, const char* method_name, const unsigned char* request, size_t request_size, IOTHUBTRANSPORT_AMQP_METHOD_HANDLE method_handle);
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_016: [** `on_methods_request_received` shall create a BUFFER_HANDLE by calling `BUFFER_new`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_028: [** On success, `on_methods_request_received` shall return 0. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_025: [** If creating the buffer fails, on_methods_request_received shall fail and return a non-zero value. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_017: [** `on_methods_request_received` shall call the `IoTHubClient_LL_DeviceMethodComplete` passing the method name, request buffer and size and the newly created BUFFER handle. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_019: [** `on_methods_request_received` shall call `iothubtransportamqp_methods_respond` passing to it the `method_handle` argument, the response bytes, response size and the status code. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_020: [** The response bytes shall be obtained by calling `BUFFER_u_char`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_021: [** The response size shall be obtained by calling `BUFFER_length`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_022: [** The status code shall be the return value of the call to `IoTHubClient_LL_DeviceMethodComplete`. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_023: [** After calling `iothubtransportamqp_methods_respond`, the allocated buffer shall be freed by using BUFFER_delete. **]**
**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_029: [** If `iothubtransportamqp_methods_respond` fails, `on_methods_request_received` shall return a non-zero value. **]**


### on_methods_error

```c
void on_methods_error(void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_COMMON_01_030: [** `on_methods_error` shall do nothing. **]**

```c
void on_methods_unsubscribed(void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_METHODS_12_001: [** `on_methods_unsubscribed` calls iothubtransportamqp_methods_unsubscribe. **]**
