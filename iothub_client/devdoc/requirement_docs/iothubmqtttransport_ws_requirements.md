IoTHubMQTTTransport WebSockets Requirements
================

## Overview

IoTHubMQTTTransport_WS is the library that enables communications with the iothub system using the MQTT protocol over websockets. 

## Exposed API

```c
extern const TRANSPORT_PROVIDER* MQTT_Protocol(void);
```

  The following static functions are provided in the fields of the TRANSPORT_PROVIDER structure:
    - IoTHubTransportMqtt_WS_SendMessageDisposition,  
    - IoTHubTransportMqtt_WS_Subscribe_DeviceMethod,  
    - IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod,  
    - IoTHubTransportMqtt_WS_Subscribe_DeviceTwin,  
    - IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin,  
    - IoTHubTransportMqtt_WS_Unsubscribe_GetDeviceTwinAsync,  
    - IoTHubTransportMqtt_WS_ProcessItem,  
    - IoTHubTransportMqtt_WS_GetHostname,  
    - IoTHubTransportMqtt_WS_SetOption,  
    - IoTHubTransportMqtt_WS_Create,  
    - IoTHubTransportMqtt_WS_Destroy,  
    - IoTHubTransportMqtt_WS_Register,  
    - IoTHubTransportMqtt_WS_Unregister,  
    - IoTHubTransportMqtt_WS_Subscribe,  
    - IoTHubTransportMqtt_WS_Unsubscribe,  
    - IoTHubTransportMqtt_WS_DoWork,  
    - IoTHubTransportMqtt_WS_SetRetryPolicy,
    - IoTHubTransportMqtt_WS_GetSendStatus,
    - IotHubTransportMqtt_WS_Subscribe_InputQueue,
    - IotHubTransportMqtt_WS_Unsubscribe_InputQueue

## typedef XIO_HANDLE(*MQTT_GET_IO_TRANSPORT)(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options);

```c
static XIO_HANDLE getIoTransportProvider(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_001: [** `getIoTransportProvider` shall obtain the WebSocket IO interface handle by calling `wsio_get_interface_description`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_002: [** `getIoTransportProvider` shall call `xio_create` while passing the WebSocket IO interface description to it and the WebSocket configuration as a WSIO_CONFIG structure, filled as below **]**:

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_003: [** - `hostname` shall be set to `fully_qualified_name`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_004: [** - `port` shall be set to 443. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_005: [** - `protocol` shall be set to `MQTT`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_006: [** - `resource_name` shall be set to `/$iothub/websocket`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_007: [** - `underlying_io_interface` shall be set to the TLS IO interface description. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_008: [** - `underlying_io_parameters` shall be set to the TLS IO arguments. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_009: [** `getIoTransportProvider` shall obtain the TLS IO interface handle by calling `platform_get_default_tlsio`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_010: [** The TLS IO parameters shall be a `TLSIO_CONFIG` structure filled as below: **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_011: [** - `hostname` shall be set to `fully_qualified_name`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_012: [** - `port` shall be set to 443. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_013: [** - If `mqtt_transport_proxy_options` is NULL, `underlying_io_interface` shall be set to NULL. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_014: [** - If `mqtt_transport_proxy_options` is NULL `underlying_io_parameters` shall be set to NULL. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_015: [** - If `mqtt_transport_proxy_options` is not NULL, `underlying_io_interface` shall be set to the HTTP proxy IO interface description. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_016: [** - If `mqtt_transport_proxy_options` is not NULL `underlying_io_parameters` shall be set to the HTTP proxy IO arguments. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_022: [** `getIoTransportProvider` shall obtain the HTTP proxy IO interface handle by calling `http_proxy_io_get_interface_description`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_023: [** The HTTP proxy IO arguments shall be an `HTTP_PROXY_IO_CONFIG` structure, filled as below: **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_024: [** - `hostname` shall be set to `fully_qualified_name`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_025: [** - `port` shall be set to 443. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_026: [** - `proxy_hostname`, `proxy_port`, `username` and `password` shall be copied from the `mqtt_transport_proxy_options` argument. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_028: [** If `http_proxy_io_get_interface_description` returns NULL, NULL shall be set in the TLS IO parameters structure for the interface description and parameters. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_029: [** If `platform_get_default_tlsio` returns NULL, NULL shall be set in the WebSocket IO parameters structure for the interface description and parameters. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_012: [** `getIoTransportProvider` shall return the `XIO_HANDLE` returned by `xio_create`. **]**

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_013: [** If `wsio_get_interface_description` returns NULL `getIoTransportProvider` shall return NULL. **]**

## IoTHubTransportMqtt_WS_Create

```c
TRANSPORT_LL_HANDLE IoTHubTransportMqtt_WS_Create(const IOTHUBTRANSPORT_CONFIG* config)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_001: [** IoTHubTransportMqtt_WS_Create shall create a TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_MQTT_Common_Create function. **]**

## IoTHubTransportMqtt_WS_Destroy

```c
void IoTHubTransportMqtt_WS_Destroy(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_002: [** IoTHubTransportMqtt_WS_Destroy shall destroy the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_MQTT_Common_Destroy function. **]**

### IoTHubTransportMqtt_WS_Register

```c
extern IOTHUB_DEVICE_HANDLE IoTHubTransportMqtt_WS_Register(RANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend);
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_003: [** IoTHubTransportMqtt_WS_Register shall register the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_MQTT_Common_Register function. **]**

### IoTHubTransportMqtt_WS_Unregister

```c
extern void IoTHubTransportMqtt_WS_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle);
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_004: [** IoTHubTransportMqtt_WS_Unregister shall register the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_MQTT_Common_Unregister function. **]**

### IoTHubTransportMqtt_WS_Subscribe

```c
int IoTHubTransportMqtt_WS_Subscribe(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_005: [** IoTHubTransportMqtt_WS_Subscribe shall subscribe the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_MQTT_Common_Subscribe function. **]**

### IoTHubTransportMqtt_WS_Unsubscribe

```c
void IoTHubTransportMqtt_WS_Unsubscribe(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_006: [** IoTHubTransportMqtt_WS_Unsubscribe shall unsubscribe the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_MQTT_Common_Unsubscribe function. **]**

### IoTHubTransportMqtt_ProcessItem

```c
IOTHUB_PROCESS_ITEM_RESULT IoTHubTransportMqtt_WS_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_014: [** IoTHubTransportMqtt_WS_ProcessItem shall call into the IoTHubTransport_MQTT_Common_DoWork function **]**

### IoTHubTransportMqtt_WS_Subscribe_DeviceMethod

```c
static int IoTHubTransportMqtt_WS_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_015: [** IoTHubTransportMqtt_WS_Subscribe_DeviceMethod shall call into the IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod function **]**

### IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod

```c
static void IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_016: [** IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod shall call into the IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod **]**

### IoTHubTransportMqtt_WS_Subscribe_DeviceTwin

```c
static int IoTHubTransportMqtt_WS_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_017: [** IoTHubTransportMqtt_WS_Subscribe_DeviceTwin shall call into the IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin **]**

### IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin

```c
static void IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_018: [** IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin shall call into the IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin **]**

### IoTHubTransportMqtt_WS_GetDeviceTwinAsync

```c
static IOTHUB_CLIENT_RESULT IoTHubTransportMqtt_WS_GetDeviceTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_09_001: [** IoTHubTransportMqtt_WS_GetDeviceTwinAsync shall call into the IoTHubTransport_MQTT_Common_GetDeviceTwinAsync **]**

### IoTHubTransportMqtt_WS_DoWork

```c
void IoTHubTransportMqtt_WS_DoWork(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_007: [** IoTHubTransportMqtt_WS_DoWork shall call into the IoTHubTransport_MQTT_Common_DoWork function. **]**

### IoTHubTransportMqtt_WS_SetRetryPolicy

```c
int IoTHubTransportMqtt_WS_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitinSeconds)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_25_012: [** IoTHubTransportMqtt_WS_SetRetryPolicy shall call into the IoTHubMqttAbstract_SetRetryPolicy function. **]**


### IoTHubTransportMqtt_WS_GetSendStatus

```c
IOTHUB_CLIENT_RESULT IoTHubTransportMqtt_WS_GetSendStatus(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_008: [** IoTHubTransportMqtt_WS_GetSendStatus shall get the send status by calling into the IoTHubTransport_MQTT_Common_GetSendStatus function. **]**

### IoTHubTransportMqtt_WS_SetOption

```c
IOTHUB_CLIENT_RESULT IoTHubTransportMqtt_WS_SetOption(TRANSPORT_LL_HANDLE handle, const char* optionName, const void* value)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_009: [** IoTHubTransportMqtt_WS_SetOption shall set the options by calling into the IoTHubTransport_MQTT_Common_SetOption function. **]**

```c
STRING_HANDLE IoTHubTransportMqtt_WS_GetHostname(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_010: [** IoTHubTransportMqtt_WS_GetHostname shall get the hostname by calling into the IoTHubTransport_MQTT_Common_GetHostname function. **]**

### MQTT_WS_Protocol

```c
const TRANSPORT_PROVIDER* MQTT_WebSocket_Protocol(void)
```

**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_011: [** This function shall return a pointer to a structure of type TRANSPORT_PROVIDER having the following values for its fields:


## IoTHubTransportMqtt_WS_SendMessageDisposition
```c
IOTHUB_CLIENT_RESULT IoTHubTransportMqtt_WS_SendMessageDisposition(MESSAGE_CALLBACK_INFO* messageData, IOTHUBMESSAGE_DISPOSITION_RESULT disposition);
```

## IoTHubTransportMQTT_WS_Subscribe_InputQueue
```c
static int IoTHubTransportMqtt_WS_Subscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle);

```
Not implemented (yet).


## IoTHubTransportMqtt_WS_Unsubscribe_InputQueue
```c
static void IoTHubTransportMqtt_WS_Unsubscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle);

```
Not implemented (yet).


**SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_10_001: [**`IoTHubTransportMqtt_WS_SendMessageDisposition` shall send the message disposition by calling into the `IoTHubTransport_MQTT_Common_SendMessageDisposition`**]**


IoTHubTransport_SendMessageDisposition = IoTHubTransportMqtt_WS_SendMessageDisposition
IoTHubTransport_Subscribe_DeviceMethod = IoTHubTransportMqtt_WS_Subscribe_DeviceMethod  
IoTHubTransport_Unsubscribe_DeviceMethod = IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod  
IoTHubTransport_Subscribe_DeviceTwin = IoTHubTransportMqtt_WS_Subscribe_DeviceTwin  
IoTHubTransport_Unsubscribe_DeviceTwin = IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin  
IoTHubTransport_ProcessItem - IoTHubTransportMqtt_WS_ProcessItem  
IoTHubTransport_GetHostname = IoTHubTransportMqtt_WS_GetHostname  
IoTHubTransport_Create = IoTHubTransportMqtt_WS_Create  
IoTHubTransport_Destroy = IoTHubTransportMqtt_WS_Destroy  
IoTHubTransport_Subscribe = IoTHubTransportMqtt_WS_Subscribe  
IoTHubTransport_Unsubscribe = IoTHubTransportMqtt_WS_Unsubscribe  
IoTHubTransport_DoWork = IoTHubTransportMqtt_WS_DoWork  
IoTHubTransport_SetOption = IoTHubTransportMqtt_WS_SetOption
IoTHubTransport_Subscribe_InputQueue = IoTHubTransportMqtt_Subscribe_InputQueue
IoTHubTransport_Unsubscribe_InputQueue = IotHubTransportMqtt_Unsubscribe_InputQueue**]**
