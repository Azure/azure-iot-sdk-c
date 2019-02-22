
# IoTHubTransportAMQP Requirements
================

## Overview

IoTHubTransportAMQP is the library that enables communications with the iothub system using the AMQP protocol over plain TLS connection.


## Dependencies

iothubtransport_amqp_common
azure_c_shared_utility


## Exposed API

```c
static const TRANSPORT_PROVIDER* AMQP_Protocol(void);
```
  The following static functions are provided in the fields of the TRANSPORT_PROVIDER structure:

    - IoTHubTransportAMQP_SendMessageDisposition,
    - IoTHubTransportAMQP_Subscribe_DeviceMethod,
    - IoTHubTransportAMQP_Unsubscribe_DeviceMethod,
    - IoTHubTransportAMQP_Subscribe_DeviceTwin,
    - IoTHubTransportAMQP_Unsubscribe_DeviceTwin,
    - IoTHubTransportAMQP_GetDeviceTwinAsync,
    - IoTHubTransportAMQP_ProcessItem,
    - IoTHubTransportAMQP_GetHostname,
    - IoTHubTransportAMQP_SetOption,
    - IoTHubTransportAMQP_Create,
    - IoTHubTransportAMQP_Destroy,
    - IoTHubTransportAMQP_Register,
    - IoTHubTransportAMQP_Unregister,
    - IoTHubTransportAMQP_Subscribe,
    - IoTHubTransportAMQP_Unsubscribe,
    - IoTHubTransportAMQP_DoWork,
    - IoTHubTransportAMQP_SetRetryPolicy,
    - IoTHubTransportAMQP_GetSendStatus,
    - IotHubTransportAMQP_Subscribe_InputQueue,
    - IotHubTransportAMQP_Unsubscribe_InputQueue


## IoTHubTransportAMQP_Create

```c
static TRANSPORT_LL_HANDLE IoTHubTransportAMQP_Create(const IOTHUBTRANSPORT_CONFIG* config)
```

**SRS_IOTHUBTRANSPORTAMQP_09_001: [**IoTHubTransportAMQP_Create shall create a TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_AMQP_Common_Create function, passing `config` and getTLSIOTransport.**]**


### getTLSIOTransport

```c
static XIO_HANDLE getTLSIOTransport(const char* fqdn, const AMQP_TRANSPORT_PROXY_OPTIONS* amqp_transport_proxy_options)
```

**SRS_IOTHUBTRANSPORTAMQP_01_009: [** `getIoTransportProvider` shall obtain the TLS IO interface handle by calling `platform_get_default_tlsio`. **]**

**SRS_IOTHUBTRANSPORTAMQP_01_010: [** The TLS IO parameters shall be a `TLSIO_CONFIG` structure filled as below: **]**

**SRS_IOTHUBTRANSPORTAMQP_01_011: [** - `hostname` shall be set to `fqdn`. **]**

**SRS_IOTHUBTRANSPORTAMQP_01_012: [** - `port` shall be set to 5671. **]**

**SRS_IOTHUBTRANSPORTAMQP_01_013: [** `underlying_io_interface` shall be set to NULL. **]**

**SRS_IOTHUBTRANSPORTAMQP_01_014: [** `underlying_io_parameters` shall be set to NULL. **]**

**SRS_IOTHUBTRANSPORTAMQP_09_003: [**If `platform_get_default_tlsio` returns NULL `getTLSIOTransport` shall return NULL.**]**
**SRS_IOTHUBTRANSPORTAMQP_09_004: [**`getTLSIOTransport` shall return the `XIO_HANDLE` created using `xio_create`.**]**


## IoTHubTransportAMQP_Destroy

```c
static void IoTHubTransportAMQP_Destroy(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_005: [**IoTHubTransportAMQP_Destroy shall destroy the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_AMQP_Common_Destroy().**]**


## IoTHubTransportAMQP_Register

```c
static IOTHUB_DEVICE_HANDLE IoTHubTransportAMQP_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, PDLIST_ENTRY waitingToSend)
```

**SRS_IOTHUBTRANSPORTAMQP_09_006: [**IoTHubTransportAMQP_Register shall register the device by calling into the IoTHubTransport_AMQP_Common_Register().**]**


## IoTHubTransportAMQP_Unregister

```c
static void IoTHubTransportAMQP_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_007: [**IoTHubTransportAMQP_Unregister shall unregister the device by calling into the IoTHubTransport_AMQP_Common_Unregister().**]**


## IoTHubTransportAMQP_Subscribe_DeviceTwin

```c
int IoTHubTransportAMQP_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_008: [**IoTHubTransportAMQP_Subscribe_DeviceTwin shall invoke IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin() and return its result.**]**


## IoTHubTransportAMQP_Unsubscribe_DeviceTwin

```c
void IoTHubTransportAMQP_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_009: [**IoTHubTransportAMQP_Unsubscribe_DeviceTwin shall invoke IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin()**]**


## IoTHubTransportAMQP_GetDeviceTwinAsync

```c
IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_GetDeviceTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
```

**SRS_IOTHUBTRANSPORTAMQP_09_021: [**IoTHubTransportAMQP_GetDeviceTwinAsync shall invoke IoTHubTransport_AMQP_Common_GetDeviceTwinAsync()**]**


## IoTHubTransportAMQP_Subscribe_DeviceMethod

```c
int IoTHubTransportAMQP_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_010: [**IoTHubTransportAMQP_Subscribe_DeviceMethod shall invoke IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod() and return its result.**]**


## IoTHubTransportAMQP_Unsubscribe_DeviceMethod

```c
void IoTHubTransportAMQP_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_011: [**IoTHubTransportAMQP_Unsubscribe_DeviceMethod shall invoke IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod()**]**


## IoTHubTransportAMQP_Subscribe

```c
static int IoTHubTransportAMQP_Subscribe(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_012: [**IoTHubTransportAMQP_Subscribe shall subscribe for D2C messages by calling into the IoTHubTransport_AMQP_Common_Subscribe().**]**


## IoTHubTransportAMQP_Unsubscribe

```c
static void IoTHubTransportAMQP_Unsubscribe(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_013: [**IoTHubTransportAMQP_Unsubscribe shall subscribe for D2C messages by calling into the IoTHubTransport_AMQP_Common_Unsubscribe().**]**


## IoTHubTransportAMQP_ProcessItem

```c
static IOTHUB_PROCESS_ITEM_RESULT IoTHubTransportAMQP_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
```

**SRS_IOTHUBTRANSPORTAMQP_09_014: [**IoTHubTransportAMQP_ProcessItem shall invoke IoTHubTransport_AMQP_Common_ProcessItem() and return its result.**]**


## IoTHubTransportAMQP_DoWork

```c
static void IoTHubTransportAMQP_DoWork(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_015: [**IoTHubTransportAMQP_DoWork shall call into the IoTHubTransport_AMQP_Common_DoWork()**]**


## IoTHubTransportAMQP_GetSendStatus

```c
IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_GetSendStatus(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
```

**SRS_IOTHUBTRANSPORTAMQP_09_016: [**IoTHubTransportAMQP_GetSendStatus shall get the send status by calling into the IoTHubTransport_AMQP_Common_GetSendStatus()**]**


## IoTHubTransportAMQP_SetOption

```c
IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_SetOption(TRANSPORT_LL_HANDLE handle, const char* optionName, const void* value)
```

**SRS_IOTHUBTRANSPORTAMQP_09_017: [**IoTHubTransportAMQP_SetOption shall set the options by calling into the IoTHubTransport_AMQP_Common_SetOption()**]**


## IoTHubTransportAMQP_GetHostname

```c
static STRING_HANDLE IoTHubTransportAMQP_GetHostname(TRANSPORT_LL_HANDLE handle)
```

**SRS_IOTHUBTRANSPORTAMQP_09_018: [**IoTHubTransportAMQP_GetHostname shall get the hostname by calling into the IoTHubTransport_AMQP_Common_GetHostname()**]**


## IoTHubTransportAMQP_SetRetryPolicy

```c
static int IoTHubTransportAMQP_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
```

**SRS_IOTHUBTRANSPORTAMQP_09_020: [**IoTHubTransportAMQP_SetRetryPolicy shall call into the IoTHubTransport_AMQP_Common_SetRetryPolicy()**]**


## IoTHubTransportAMQP_SendMessageDisposition
```c
IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_SendMessageDisposition(MESSAGE_CALLBACK_INFO* messageData, IOTHUBMESSAGE_DISPOSITION_RESULT disposition);
```

**SRS_IOTHUBTRANSPORTAMQP_10_001: [**IoTHubTransportAMQP_SendMessageDisposition shall send the message disposition by calling into the IoTHubTransport_AMQP_Common_SendMessageDisposition()**]**


## IoTHubTransportAMQP_Subscribe_InputQueue
```c
static int IotHubTransportAMQP_Subscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle);

```

**SRS_IOTHUBTRANSPORTAMQP_31_021: [**IoTHubTransportAMQP_Subscribe_InputQueue shall return a failure as input queues are not implemented for AMQP**]**



## IotHubTransportAMQP_Unsubscribe_InputQueue
```c
static void IotHubTransportAMQP_Unsubscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle);

```

**SRS_IOTHUBTRANSPORTAMQP_31_022: [**IotHubTransportAMQP_Unsubscribe_InputQueue shall do nothing as input queues are not implemented for AMQP**]**



### AMQP_Protocol

```c
const TRANSPORT_PROVIDER* AMQP_Protocol(void)
```

**SRS_IOTHUBTRANSPORTAMQP_09_019: [**This function shall return a pointer to a structure of type TRANSPORT_PROVIDER having the following values for it's fields:
IoTHubTransport_SendMessageDisposition = IoTHubTransportAMQP_SendMessageDisposition
IoTHubTransport_Subscribe_DeviceMethod = IoTHubTransportAMQP_Subscribe_DeviceMethod
IoTHubTransport_Unsubscribe_DeviceMethod = IoTHubTransportAMQP_Unsubscribe_DeviceMethod
IoTHubTransport_Subscribe_DeviceTwin = IoTHubTransportAMQP_Subscribe_DeviceTwin
IoTHubTransport_Unsubscribe_DeviceTwin = IoTHubTransportAMQP_Unsubscribe_DeviceTwin
IoTHubTransport_ProcessItem - IoTHubTransportAMQP_ProcessItem
IoTHubTransport_GetHostname = IoTHubTransportAMQP_GetHostname
IoTHubTransport_Create = IoTHubTransportAMQP_Create
IoTHubTransport_Destroy = IoTHubTransportAMQP_Destroy
IoTHubTransport_Subscribe = IoTHubTransportAMQP_Subscribe
IoTHubTransport_Unsubscribe = IoTHubTransportAMQP_Unsubscribe
IoTHubTransport_DoWork = IoTHubTransportAMQP_DoWork
IoTHubTransport_SetRetryPolicy = IoTHubTransportAMQP_SetRetryPolicy
IoTHubTransport_SetOption = IoTHubTransportAMQP_SetOption
IoTHubTransport_Subscribe_InputQueue = IoTHubTransportAMQP_Subscribe_InputQueue
IoTHubTransport_Unsubscribe_InputQueue = IotHubTransportAMQP_Unsubscribe_InputQueue**]**
