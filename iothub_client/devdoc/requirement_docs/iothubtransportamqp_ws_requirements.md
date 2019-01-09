
# IoTHubTransportAMQP_WS Requirements
================

## Overview

IoTHubTransportAMQP_WS is the library that enables communications with the iothub system using the AMQP protocol over Websockets with TLS connection.


## Dependencies

iothubtransport_amqp_common
azure_c_shared_utility


## Exposed API

```c
static const TRANSPORT_PROVIDER* AMQP_Protocol_over_WebSocketsTls(void);
```
  The following static functions are provided in the fields of the TRANSPORT_PROVIDER structure:

    - IoTHubTransportAMQP_WS_Subscribe_DeviceMethod,
    - IoTHubTransportAMQP_WS_SendMessageDisposition,
    - IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod,
    - IoTHubTransportAMQP_WS_Subscribe_DeviceTwin,
    - IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin,
	- IoTHubTransportAMQP_WS_GetDeviceTwinAsync,
    - IoTHubTransportAMQP_WS_ProcessItem,
    - IoTHubTransportAMQP_WS_GetHostname,
    - IoTHubTransportAMQP_WS_SetOption,
    - IoTHubTransportAMQP_WS_Create,
    - IoTHubTransportAMQP_WS_Destroy,
    - IoTHubTransportAMQP_WS_SetRetryLogic,
    - IoTHubTransportAMQP_WS_Register,
    - IoTHubTransportAMQP_WS_Unregister,
    - IoTHubTransportAMQP_WS_Subscribe,
    - IoTHubTransportAMQP_WS_Unsubscribe,
    - IoTHubTransportAMQP_WS_DoWork,
    - IoTHubTransportAMQP_WS_GetSendStatus,
    - IotHubTransportAMQP_WS_Subscribe_InputQueue,
    - IotHubTransportAMQP_WS_Unsubscribe_InputQueue



## IoTHubTransportAMQP_WS_Create

```c
static TRANSPORT_LL_HANDLE IoTHubTransportAMQP_WS_Create(const IOTHUBTRANSPORT_CONFIG* config)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_001: [**IoTHubTransportAMQP_WS_Create shall create a TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_AMQP_Common_Create function, passing `config` and getWebSocketsIOTransport.**]**


### getWebSocketsIOTransport

```c
static XIO_HANDLE getWebSocketsIOTransport(const char* fqdn, const AMQP_TRANSPORT_PROXY_OPTIONS* amqp_transport_proxy_options)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_01_001: [** `getIoTransportProvider` shall obtain the WebSocket IO interface handle by calling `wsio_get_interface_description`. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_002: [** `getIoTransportProvider` shall call `xio_create` while passing the WebSocket IO interface description to it and the WebSocket configuration as a WSIO_CONFIG structure, filled as below: **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_003: [** - `hostname` shall be set to `fqdn`. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_004: [** - `port` shall be set to 443. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_005: [** - `protocol` shall be set to `AMQPWSB10`. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_006: [** - `resource_name` shall be set to `/$iothub/websocket`. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_007: [** - `underlying_io_interface` shall be set to the TLS IO interface description. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_008: [** - `underlying_io_parameters` shall be set to the TLS IO arguments. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_009: [** `getIoTransportProvider` shall obtain the TLS IO interface handle by calling `platform_get_default_tlsio`. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_010: [** The TLS IO parameters shall be a `TLSIO_CONFIG` structure filled as below: **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_011: [** - `hostname` shall be set to `fqdn`. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_012: [** - `port` shall be set to 443. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_013: [** - If `amqp_transport_proxy_options` is NULL, `underlying_io_interface` shall be set to NULL. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_014: [** - If `amqp_transport_proxy_options` is NULL `underlying_io_parameters` shall be set to NULL. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_015: [** - If `amqp_transport_proxy_options` is not NULL, `underlying_io_interface` shall be set to the HTTP proxy IO interface description. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_016: [** - If `amqp_transport_proxy_options` is not NULL `underlying_io_parameters` shall be set to the HTTP proxy IO arguments. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_022: [** `getIoTransportProvider` shall obtain the HTTP proxy IO interface handle by calling `http_proxy_io_get_interface_description`. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_023: [** The HTTP proxy IO arguments shall be an `HTTP_PROXY_IO_CONFIG` structure, filled as below: **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_024: [** - `hostname` shall be set to `fully_qualified_name`. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_025: [** - `port` shall be set to 443. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_026: [** - `proxy_hostname`, `proxy_port`, `username` and `password` shall be copied from the `mqtt_transport_proxy_options` argument. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_028: [** If `http_proxy_io_get_interface_description` returns NULL, NULL shall be set in the TLS IO parameters structure for the interface description and parameters. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_01_029: [** If `platform_get_default_tlsio` returns NULL, NULL shall be set in the WebSocket IO parameters structure for the interface description and parameters. **]**

**SRS_IOTHUBTRANSPORTAMQP_WS_09_003: [**If `io_interface_description` is NULL getWebSocketsIOTransport shall return NULL.**]**
**SRS_IOTHUBTRANSPORTAMQP_WS_09_004: [**getWebSocketsIOTransport shall return the XIO_HANDLE created using xio_create().**]**


## IoTHubTransportAMQP_WS_Destroy

```c
static void IoTHubTransportAMQP_WS_Destroy(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_005: [**IoTHubTransportAMQP_WS_Destroy shall destroy the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_AMQP_Common_Destroy().**]**


## IoTHubTransportAMQP_WS_Register

```c
static IOTHUB_DEVICE_HANDLE IoTHubTransportAMQP_WS_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, PDLIST_ENTRY waitingToSend)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_006: [**IoTHubTransportAMQP_WS_Register shall register the device by calling into the IoTHubTransport_AMQP_Common_Register().**]**


## IoTHubTransportAMQP_WS_Unregister

```c
static void IoTHubTransportAMQP_WS_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_007: [**IoTHubTransportAMQP_WS_Unregister shall unregister the device by calling into the IoTHubTransport_AMQP_Common_Unregister().**]**


## IoTHubTransportAMQP_WS_Subscribe_DeviceTwin

```c
int IoTHubTransportAMQP_WS_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_008: [**IoTHubTransportAMQP_WS_Subscribe_DeviceTwin shall invoke IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin() and return its result.**]**


## IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin

```c
void IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_009: [**IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin shall invoke IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin()**]**


## IoTHubTransportAMQP_WS_GetDeviceTwinAsync

```c
void IoTHubTransportAMQP_WS_GetDeviceTwinAsync(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_020: [**IoTHubTransportAMQP_WS_GetDeviceTwinAsync shall invoke IoTHubTransport_AMQP_Common_GetDeviceTwinAsync()**]**


## IoTHubTransportAMQP_WS_Subscribe_DeviceMethod

```c
int IoTHubTransportAMQP_WS_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_010: [**IoTHubTransportAMQP_WS_Subscribe_DeviceMethod shall invoke IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod() and return its result.**]**


## IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod

```c
void IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_011: [**IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod shall invoke IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod()**]**


## IoTHubTransportAMQP_WS_Subscribe

```c
static int IoTHubTransportAMQP_WS_Subscribe(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_012: [**IoTHubTransportAMQP_WS_Subscribe shall subscribe for D2C messages by calling into the IoTHubTransport_AMQP_Common_Subscribe().**]**


## IoTHubTransportAMQP_WS_Unsubscribe

```c
static void IoTHubTransportAMQP_WS_Unsubscribe(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_013: [**IoTHubTransportAMQP_WS_Unsubscribe shall subscribe for D2C messages by calling into the IoTHubTransport_AMQP_Common_Unsubscribe().**]**


## IoTHubTransportAMQP_WS_ProcessItem

```c
static IOTHUB_PROCESS_ITEM_RESULT IoTHubTransportAMQP_WS_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_014: [**IoTHubTransportAMQP_WS_ProcessItem shall invoke IoTHubTransport_AMQP_Common_ProcessItem() and return its result.**]**


## IoTHubTransportAMQP_WS_DoWork

```c
static void IoTHubTransportAMQP_WS_DoWork(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_015: [**IoTHubTransportAMQP_WS_DoWork shall call into the IoTHubTransport_AMQP_Common_DoWork()**]**


## IoTHubTransportAMQP_WS_GetSendStatus

```c
IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_WS_GetSendStatus(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_016: [**IoTHubTransportAMQP_WS_GetSendStatus shall get the send status by calling into the IoTHubTransport_AMQP_Common_GetSendStatus()**]**


## IoTHubTransportAMQP_WS_SetOption

```c
IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_WS_SetOption(TRANSPORT_LL_HANDLE handle, const char* optionName, const void* value)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_017: [**IoTHubTransportAMQP_WS_SetOption shall set the options by calling into the IoTHubTransport_AMQP_Common_SetOption()**]**


## IoTHubTransportAMQP_WS_GetHostname

```c
static STRING_HANDLE IoTHubTransportAMQP_WS_GetHostname(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_018: [**IoTHubTransportAMQP_WS_GetHostname shall get the hostname by calling into the IoTHubTransport_AMQP_Common_GetHostname()**]**


## IoTHubTransportAMQP_WS_SendMessageDisposition

```c
static STRING_HANDLE IoTHubTransportAMQP_WS_SendMessageDisposition(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_10_001 [**IoTHubTransportAMQP_WS_SendMessageDisposition shall sned the message disposition by calling into the IoTHubTransport_AMQP_Common_SendMessageDisposition()**]**


## IotHubTransportAMQP_WS_Subscribe_InputQueue
```c
static int IotHubTransportAMQP_WS_Subscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
```
Not implemented (yet).

## IotHubTransportAMQP_WS_Unsubscribe_InputQueue
```c
static void IotHubTransportAMQP_WS_Unsubscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
```
Not implemented (yet).


### AMQP_Protocol_over_WebSocketsTls

```c
const TRANSPORT_PROVIDER* AMQP_Protocol_over_WebSocketsTls(void)
```

**SRS_IOTHUBTRANSPORTAMQP_WS_09_019: [**This function shall return a pointer to a structure of type TRANSPORT_PROVIDER having the following values for it's fields:
IoTHubTransport_SendMessageDisposition = IoTHubTransportAMQP_WS_SendMessageDisposition
IoTHubTransport_Subscribe_DeviceMethod = IoTHubTransportAMQP_WS_Subscribe_DeviceMethod
IoTHubTransport_Unsubscribe_DeviceMethod = IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod
IoTHubTransport_Subscribe_DeviceTwin = IoTHubTransportAMQP_WS_Subscribe_DeviceTwin
IoTHubTransport_Unsubscribe_DeviceTwin = IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin
IoTHubTransport_ProcessItem - IoTHubTransportAMQP_WS_ProcessItem
IoTHubTransport_GetHostname = IoTHubTransportAMQP_WS_GetHostname
IoTHubTransport_Create = IoTHubTransportAMQP_WS_Create
IoTHubTransport_Destroy = IoTHubTransportAMQP_WS_Destroy
IoTHubTransport_Subscribe = IoTHubTransportAMQP_WS_Subscribe
IoTHubTransport_Unsubscribe = IoTHubTransportAMQP_WS_Unsubscribe
IoTHubTransport_DoWork = IoTHubTransportAMQP_WS_DoWork
IoTHubTransport_SetRetryLogic = IoTHubTransportAMQP_WS_SetRetryLogic
IoTHubTransport_SetOption = IoTHubTransportAMQP_WS_SetOption
IoTHubTransport_Subscribe_InputQueue = IoTHubTransportAMQP_WS_Subscribe_InputQueue
IoTHubTransport_Unsubscribe_InputQueue = IotHubTransportAMQP_WS_Unsubscribe_InputQueue**]**
