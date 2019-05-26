# IoTHubClient Requirements

## References

[Azure Storage Services REST API Reference](https://msdn.microsoft.com/en-us/library/azure/dd179355.aspx)


## Overview

IoTHubClient is a module that extends the IoTHubCLient_LL module with 2 features:
-scheduling the work for the IoTHubCLient from a thread, so that the user does not need to create its own thread.
-Thread-safe APIs
Underlaying layer in the following requirements refers to IoTHubClient_LL.

## Exposed API

```c
typedef void* IOTHUB_CLIENT_HANDLE;
extern const char* IoTHubClient_GetVersionString(void);
extern IOTHUB_CLIENT_HANDLE IoTHubClient_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern IOTHUB_CLIENT_HANDLE IoTHubClient_Create(const IOTHUB_CLIENT_CONFIG* config);
extern IOTHUB_CLIENT_HANDLE IoTHubClient_CreateWithTransport(TRANSPORT_HANDLE transportHandle, const IOTHUB_CLIENT_CONFIG* config);
extern IOTHUB_CLIENT_HANDLE IoTHubClient_CreateFromDeviceAuth(const char* iothub_uri, const char* device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void IoTHubClient_Destroy(IOTHUB_CLIENT_HANDLE iotHubClientHandle);

extern IOTHUB_CLIENT_RESULT IoTHubClient_SendEventAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetMessageCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback);

extern IOTHUB_CLIENT_RESULT IoTHubClient_SetConnectionStatusCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitinSeconds);
extern IOTHUB_CLIENT_RESULT IoTHubClient_GetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitinSeconds);

extern IOTHUB_CLIENT_RESULT IoTHubClient_GetLastMessageReceiveTime(IOTHUB_CLIENT_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime);
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetOption(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* optionName, const void* value);
extern IOTHUB_CLIENT_RESULT IoTHubClient_UploadToBlobAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* destinationFileName, const unsigned char* source, size_t size, IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK iotHubClientFileUploadCallback, void* context);
extern IOTHUB_CLIENT_RESULT IoTHubClient_UploadMultipleBlocksToBlobAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context);

extern IOTHUB_CLIENT_RESULT IoTHubClient_SendEventToOutputAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetInputMessageCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback);

## Device Twin
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceTwinCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_SendReportedState(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, uint32_t reportedVersion, uint32_t lastSeenDesiredVersion, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClientCore_GetDeviceTwinAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);

## IoTHub Methods
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceMethodCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback);
unsigned char* payload, IOTHUB_CLIENT_IOTHUB_METHOD_EXECUTE_CALLBACK iotHubExecuteCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback);
```

## IoTHubClient_GetVersionString

```c
extern const char* IoTHubClient_GetVersionString(void);
```

**SRS_IOTHUBCLIENT_05_001: [** `IoTHubClient_GetVersionString` shall return a pointer to a constant string which indicates the version of `IoTHubClient` API. **]**


## IoTHubClient_CreateFromConnectionString

```c
extern IOTHUB_CLIENT_HANDLE IoTHubClient_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
``` 
 
**SRS_IOTHUBCLIENT_12_003: [** `IoTHubClient_CreateFromConnectionString` shall verify the input parameters and if any of them `NULL` then return `NULL`. **]**

**SRS_IOTHUBCLIENT_12_004: [** `IoTHubClient_CreateFromConnectionString` shall allocate a new `IoTHubClient` instance. **]**

**SRS_IOTHUBCLIENT_02_059: [** `IoTHubClient_CreateFromConnectionString` shall create a `LIST_HANDLE` containing informations saved by `IoTHubClient_UploadToBlobAsync`. **]** 

**SRS_IOTHUBCLIENT_02_070: [** If creating the `SINGLYLINKEDLIST_HANDLE` fails then `IoTHubClient_CreateFromConnectionString` shall fail and return NULL**]**

**SRS_IOTHUBCLIENT_12_011: [** If the allocation failed, `IoTHubClient_CreateFromConnectionString` returns `NULL`. **]**

**SRS_IOTHUBCLIENT_12_005: [** `IoTHubClient_CreateFromConnectionString` shall create a lock object to be used later for serializing `IoTHubClient` calls. **]**

**SRS_IOTHUBCLIENT_12_009: [** If lock creation failed, `IoTHubClient_CreateFromConnectionString` shall do clean up and return `NULL`. **]**

**SRS_IOTHUBCLIENT_12_006: [** `IoTHubClient_CreateFromConnectionString` shall instantiate a new `IoTHubClient_LL` instance by calling `IoTHubClient_LL_CreateFromConnectionString` and passing the connectionString. **]**

**SRS_IOTHUBCLIENT_12_010: [** If `IoTHubClient_LL_CreateFromConnectionString` fails then `IoTHubClient_CreateFromConnectionString` shall do clean-up and return `NULL`. **]**


## IoTHubClient_Create

```c 
extern IOTHUB_CLIENT_HANDLE IoTHubClient_Create(const IOTHUB_CLIENT_CONFIG* config);
```

**SRS_IOTHUBCLIENT_01_001: [** `IoTHubClient_Create` shall allocate a new IoTHubClient instance and return a non-`NULL` handle to it. **]**

**SRS_IOTHUBCLIENT_02_060: [** `IoTHubClient_Create` shall create a `SINGLYLINKEDLIST_HANDLE` that shall be used by `IoTHubClient_UploadToBlobAsync`. **]**  

**SRS_IOTHUBCLIENT_02_061: [** If creating the `LIST_HANDLE` fails then `IoTHubClient_Create` shall fail and return NULL. **]**

**SRS_IOTHUBCLIENT_01_002: [** `IoTHubClient_Create` shall instantiate a new `IoTHubClient_LL` instance by calling `IoTHubClient_LL_Create` and passing the config argument. **]**

**SRS_IOTHUBCLIENT_01_003: [** If `IoTHubClient_LL_Create` fails, then `IoTHubClient_Create` shall return `NULL`. **]**

**SRS_IOTHUBCLIENT_01_004: [** If allocating memory for the new `IoTHubClient` instance fails, then `IoTHubClient_Create` shall return `NULL`. **]**

**SRS_IOTHUBCLIENT_01_029: [** `IoTHubClient_Create` shall create a lock object to be used later for serializing IoTHubClient calls. **]**

**SRS_IOTHUBCLIENT_01_030: [** If creating the lock fails, then `IoTHubClient_Create` shall return `NULL`. **]**

**SRS_IOTHUBCLIENT_01_031: [** If `IoTHubClient_Create` fails, all resources allocated by it shall be freed. **]**

**SRS_IOTHUBCLIENT_41_002: [** `IoTHubClient_Create` shall set `do_work_freq_ms` to the default sleep value of 1 ms **]** 


## IoTHubClient_CreateWithTransport

```c
extern IOTHUB_CLIENT_HANDLE IoTHubClient_CreateWithTransport(TRANSPORT_HANDLE transportHandle, const IOTHUB_CLIENT_CONFIG* config);
```

Create an IoTHubClient using an existing connection.

**SRS_IOTHUBCLIENT_17_013: [** `IoTHubClient_CreateWithTransport` shall return `NULL` if `transportHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_17_014: [** `IoTHubClient_CreateWithTransport` shall return `NULL` if config is `NULL`. **]**

**SRS_IOTHUBCLIENT_17_001: [** `IoTHubClient_CreateWithTransport` shall allocate a new `IoTHubClient` instance and return a non-`NULL` handle to it. **]**

**SRS_IOTHUBCLIENT_02_073: [** `IoTHubClient_CreateWithTransport` shall create a `LIST_HANDLE` that shall be used by `IoTHubClient_UploadToBlobAsync`. **]**  

**SRS_IOTHUBCLIENT_02_074: [** If creating the `SINGLYLINKEDLIST_HANDLE` fails then `IoTHubClient_CreateWithTransport` shall fail and return NULL. **]**
 
**SRS_IOTHUBCLIENT_17_002: [** If allocating memory for the new `IoTHubClient` instance fails, then `IoTHubClient_CreateWithTransport` shall return `NULL`. **]**
 
**SRS_IOTHUBCLIENT_17_003: [** `IoTHubClient_CreateWithTransport` shall call `IoTHubTransport_GetLLTransport` on `transportHandle` to get lower layer transport. **]**

**SRS_IOTHUBCLIENT_17_004: [** If `IoTHubTransport_GetLLTransport` fails, then `IoTHubClient_CreateWithTransport` shall return `NULL`. **]**

**SRS_IOTHUBCLIENT_17_005: [** `IoTHubClient_CreateWithTransport` shall call `IoTHubTransport_GetLock` to get the transport lock to be used later for serializing `IoTHubClient` calls. **]**

**SRS_IOTHUBCLIENT_17_006: [** If `IoTHubTransport_GetLock` fails, then `IoTHubClient_CreateWithTransport` shall return `NULL`. **]**

**SRS_IOTHUBCLIENT_17_007: [** `IoTHubClient_CreateWithTransport` shall instantiate a new `IoTHubClient_LL` instance by calling `IoTHubClient_LL_CreateWithTransport` and passing the lower layer transport and config argument. **]**

**SRS_IOTHUBCLIENT_17_008: [** If `IoTHubClient_LL_CreateWithTransport` fails, then `IoTHubClient_Create` shall return `NULL`. **]**

**SRS_IOTHUBCLIENT_17_009: [** If `IoTHubClient_LL_CreateWithTransport` fails, all resources allocated by it shall be freed. **]**


### IoTHubClient_CreateFromDeviceAuth

```c
extern IOTHUB_CLIENT_HANDLE IoTHubClient_CreateFromDeviceAuth(const char* iothub_uri, const char* device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
```

**SRS_IOTHUBCLIENT_12_019: [** `IoTHubClient_CreateFromDeviceAuth` shall verify the input parameters and if any of them `NULL` then return `NULL`. **]**

**SRS_IOTHUBCLIENT_12_020: [** `IoTHubClient_CreateFromDeviceAuth` shall allocate a new `IoTHubClient` instance. **]**

**SRS_IOTHUBCLIENT_12_021: [** If allocating memory for the new `IoTHubClient` instance fails, then `IoTHubClient_CreateFromDeviceAuth` shall return `NULL`. **]**

**SRS_IOTHUBCLIENT_12_022: [** `IoTHubClient_CreateFromDeviceAuth` shall create a lock object to be used later for serializing IoTHubClient calls. **]**

**SRS_IOTHUBCLIENT_12_023: [** If creating the lock fails, then IoTHubClient_CreateFromDeviceAuth shall return NULL. **]**

**SRS_IOTHUBCLIENT_12_024: [** If IoTHubClient_CreateFromDeviceAuth fails, all resources allocated by it shall be freed. **]**

**SRS_IOTHUBCLIENT_12_025: [** `IoTHubClient_CreateFromDeviceAuth` shall instantiate a new `IoTHubClient_LL` instance by calling `IoTHubClient_LL_CreateFromDeviceAuth` and passing iothub_uri, device_id and protocol argument.  **]**


## IoTHubClient_Destroy

```c
extern void IoTHubClient_Destroy(IOTHUB_CLIENT_HANDLE iotHubClientHandle);
```

**SRS_IOTHUBCLIENT_01_005: [** `IoTHubClient_Destroy` shall free all resources associated with the `iotHubClientHandle` instance. **]**

**SRS_IOTHUBCLIENT_02_069: [** `IoTHubClient_Destroy` shall free all data created by `IoTHubClient_UploadToBlobAsync`. **]**

**SRS_IOTHUBCLIENT_01_006: [** That includes destroying the `IoTHubClient_LL` instance by calling `IoTHubClient_LL_Destroy`. **]**

**SRS_IOTHUBCLIENT_02_043: [** `IoTHubClient_Destroy` shall lock the serializing lock and signal the worker thread (if any) to end. **]**

**SRS_IOTHUBCLIENT_02_045: [** `IoTHubClient_Destroy` shall unlock the serializing lock. **]**

**SRS_IOTHUBCLIENT_01_007: [** The thread created as part of executing `IoTHubClient_SendEventAsync` or `IoTHubClient_SetNotificationMessageCallback` shall be joined. **]**

**SRS_IOTHUBCLIENT_01_032: [** If the lock was allocated in `IoTHubClient_Create`, it shall be also freed. **]**

**SRS_IOTHUBCLIENT_01_008: [** `IoTHubClient_Destroy` shall do nothing if parameter `iotHubClientHandle` is `NULL`. **]**


## IoTHubClient_SendEventAsync

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SendEventAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_01_009: [** `IoTHubClient_SendEventAsync` shall start the worker thread if it was not previously started. **]**

**SRS_IOTHUBCLIENT_17_012: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_01_010: [** If starting the thread fails, `IoTHubClient_SendEventAsync` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_01_011: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SendEventAsync` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_01_012: [** `IoTHubClient_SendEventAsync` shall call `IoTHubClient_LL_SendEventAsync`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameters `eventMessageHandle`, `iothub_ll_event_confirm_callback` and IOTHUB_QUEUE_CONTEXT variable. **]**

**SRS_IOTHUBCLIENT_01_013: [** When `IoTHubClient_LL_SendEventAsync` is called, `IoTHubClient_SendEventAsync` shall return the result of `IoTHubClient_LL_SendEventAsync`. **]**

**SRS_IOTHUBCLIENT_01_025: [** `IoTHubClient_SendEventAsync` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_01_026: [** If acquiring the lock fails, `IoTHubClient_SendEventAsync` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_07_001: [** `IoTHubClient_SendEventAsync` shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the `IoTHubClient_LL_SendEventAsync` function as a user context. **]**


## IoTHubClient_SetMessageCallback

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetMessageCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_01_014: [** `IoTHubClient_SetMessageCallback` shall start the worker thread if it was not previously started. **]**

**SRS_IOTHUBCLIENT_17_011: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_01_015: [** If starting the thread fails, `IoTHubClient_SetMessageCallback` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_01_016: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetMessageCallback` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_01_017: [** `IoTHubClient_SetMessageCallback` shall call `IoTHubClient_LL_SetMessageCallbackEx`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the local `iothub_ll_message_callback` wrapper of `messageCallback` and `userContextCallback`. **]**

**SRS_IOTHUBCLIENT_01_018: [** When `IoTHubClient_LL_SetMessageCallbackEx` is called, `IoTHubClient_SetMessageCallback` shall return the result of `IoTHubClient_LL_SetMessageCallbackEx`. **]**

**SRS_IOTHUBCLIENT_01_027: [** `IoTHubClient_SetMessageCallback` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_01_028: [** If acquiring the lock fails, `IoTHubClient_SetMessageCallback` shall return `IOTHUB_CLIENT_ERROR`. **]**


###IoTHubClient_SetConnectionStatusCallback

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetConnectionStatusCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_25_081: [** `IoTHubClient_SetConnectionStatusCallback` shall start the worker thread if it was not previously started. **]**

**SRS_IOTHUBCLIENT_25_082: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_25_083: [** If starting the thread fails, `IoTHubClient_SetConnectionStatusCallback` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_25_084: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetConnectionStatusCallback` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_25_085: [** `IoTHubClient_SetConnectionStatusCallback` shall call `IoTHubClient_LL_SetConnectionStatusCallback`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameters `connectionStatusCallback` and `userContextCallback`. **]**

**SRS_IOTHUBCLIENT_25_086: [** When `IoTHubClient_LL_SetConnectionStatusCallback` is called, `IoTHubClient_SetConnectionStatusCallback` shall return the result of `IoTHubClient_LL_SetConnectionStatusCallback`. **]**

**SRS_IOTHUBCLIENT_25_087: [** `IoTHubClient_SetConnectionStatusCallback` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_25_088: [** If acquiring the lock fails, `IoTHubClient_SetConnectionStatusCallback` shall return `IOTHUB_CLIENT_ERROR`. **]**


###IoTHubClient_SetRetryPolicy

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitinSeconds);
```

**SRS_IOTHUBCLIENT_25_073: [** `IoTHubClient_SetRetryPolicy` shall start the worker thread if it was not previously started. **]**

**SRS_IOTHUBCLIENT_25_074: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_25_075: [** If starting the thread fails, `IoTHubClient_SetRetryPolicy` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_25_076: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetRetryPolicy` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_25_077: [** `IoTHubClient_SetRetryPolicy` shall call `IoTHubClient_LL_SetRetryPolicy`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameters `retryPolicy` and `retryTimeoutLimitinSeconds`. **]**

**SRS_IOTHUBCLIENT_25_078: [** When `IoTHubClient_LL_SetRetryPolicy` is called, `IoTHubClient_SetRetryPolicy` shall return the result of `IoTHubClient_LL_SetRetryPolicy`. **]**

**SRS_IOTHUBCLIENT_25_079: [** `IoTHubClient_SetRetryPolicy` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_25_080: [** If acquiring the lock fails, `IoTHubClient_SetRetryPolicy` shall return `IOTHUB_CLIENT_ERROR`. **]**


###IoTHubClient_GetRetryPolicy

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_GetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitinSeconds);
```

**SRS_IOTHUBCLIENT_25_089: [** `IoTHubClient_GetRetryPolicy` shall start the worker thread if it was not previously started. **]**

**SRS_IOTHUBCLIENT_25_090: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_25_091: [** If starting the thread fails, `IoTHubClient_GetRetryPolicy` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_25_092: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_GetRetryPolicy` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_25_093: [** `IoTHubClient_GetRetryPolicy` shall call `IoTHubClient_LL_GetRetryPolicy`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameters `connectionStatusCallback` and `userContextCallback`. **]**

**SRS_IOTHUBCLIENT_25_094: [** When `IoTHubClient_LL_GetRetryPolicy` is called, `IoTHubClient_GetRetryPolicy` shall return the result of `IoTHubClient_LL_GetRetryPolicy`. **]**

**SRS_IOTHUBCLIENT_25_095: [** `IoTHubClient_GetRetryPolicy` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_25_096: [** If acquiring the lock fails, `IoTHubClient_GetRetryPolicy` shall return `IOTHUB_CLIENT_ERROR`. **]**


## IoTHubClient_GetLastMessageReceiveTime

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_GetLastMessageReceiveTime(IOTHUB_CLIENT_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime);
```

**SRS_IOTHUBCLIENT_01_019: [** `IoTHubClient_GetLastMessageReceiveTime` shall call `IoTHubClient_LL_GetLastMessageReceiveTime`, while passing the IoTHubClient_LL handle created by `IoTHubClient_Create` and the parameter `lastMessageReceiveTime`. **]**

**SRS_IOTHUBCLIENT_01_020: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_GetLastMessageReceiveTime` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_01_021: [** Otherwise, `IoTHubClient_GetLastMessageReceiveTime` shall return the result of `IoTHubClient_LL_GetLastMessageReceiveTime`. **]**

**SRS_IOTHUBCLIENT_01_035: [** `IoTHubClient_GetLastMessageReceiveTime` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_01_036: [** If acquiring the lock fails, `IoTHubClient_GetLastMessageReceiveTime` shall return `IOTHUB_CLIENT_ERROR`. **]**


## IoTHubClient_GetSendStatus

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_GetSendStatus(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus);
```

**SRS_IOTHUBCLIENT_01_022: [** `IoTHubClient_GetSendStatus` shall call `IoTHubClient_LL_GetSendStatus`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameter `iotHubClientStatus`. **]**

**SRS_IOTHUBCLIENT_01_023: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_GetSendStatus` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_01_024: [** Otherwise, `IoTHubClient_GetSendStatus` shall return the result of `IoTHubClient_LL_GetSendStatus`. **]**

**SRS_IOTHUBCLIENT_01_033: [** `IoTHubClient_GetSendStatus` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_01_034: [** If acquiring the lock fails, `IoTHubClient_GetSendStatus` shall return `IOTHUB_CLIENT_ERROR`. **]**

### Scheduling work

**SRS_IOTHUBCLIENT_01_037: [** The thread created by `IoTHubClient_SendEvent` or `IoTHubClient_SetMessageCallback` shall call `IoTHubClient_LL_DoWork` every 1 ms. **]**

**SRS_IOTHUBCLIENT_01_038: [** The thread shall exit when all IoTHubClients using the thread have had `IoTHubClient_Destroy` called. **]**

**SRS_IOTHUBCLIENT_01_039: [** All calls to `IoTHubClient_LL_DoWork` shall be protected by the lock created in `IotHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_01_040: [** If acquiring the lock fails, `IoTHubClient_LL_DoWork` shall not be called. **]**

**SRS_IOTHUBCLIENT_02_072: [** All threads marked as disposable (upon completion of a file upload) shall be joined and the data structures build for them shall be freed. **]**


## IoTHubClient_SetOption

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetOption(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* optionName, const void* value);
```

IoTHubClient_SetOption allows run-time changing of settings of the IoTHubClient. 

**SRS_IOTHUBCLIENT_02_034: [** If parameter `iotHubClientHandle` is `NULL` then `IoTHubClient_SetOption` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_02_035: [** If parameter `optionName` is `NULL` then IoTHubClient_SetOption shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_02_036: [** If parameter value is `NULL` then `IoTHubClient_SetOption` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_02_038: [** If `optionName` does not match one of the options handled by this module then `IoTHubClient_SetOption` shall call `IoTHubClient_LL_SetOption` passing the same parameters and return what `IoTHubClient_LL_SetOption` returns. **]**

**SRS_IOTHUBCLIENT_01_041: [** `IoTHubClient_SetOption` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. **]**

**SRS_IOTHUBCLIENT_01_042: [** If acquiring the lock fails, `IoTHubClient_SetOption` shall return `IOTHUB_CLIENT_ERROR`. **]**

Options handled by IoTHubClient_SetOption:

**SRS_IOTHUBCLIENT_41_001: [** If parameter `optionName` is `OPTION_DO_WORK_FREQUENCY_IN_MS` then `IoTHubClientCore_SetOption` shall set `do_work_freq_ms` parameter of `IoTHubClientInstance` **]** 

**SRS_IOTHUBCLIENT_41_003: [** The value of `OPTION_DO_WORK_FREQUENCY_IN_MS` shall be limited to 100 to follow SDK best practices by not reducing the DoWork frequency below 10 Hz **]** 

**SRS_IOTHUBCLIENT_41_004: [** If `currentMessageTimeout` is not greater than `do_work_freq_ms`, `IotHubClientCore_SetOption` shall return `IOTHUB_CLIENT_INVALID_ARG` **]**

**SRS_IOTHUBCLIENT_41_005: [** If parameter `optionName` is `OPTION_MESSAGE_TIMEOUT` then `IoTHubClientCore_SetOption` shall set `currentMessageTimeout` parameter of `IoTHubClientInstance` **]** 

**SRS_IOTHUBCLIENT_41_006: [** If parameter `optionName` is `OPTION_MESSAGE_TIMEOUT` then `IoTHubClientCore_SetOption` shall call `IoTHubClientCore_LL_SetOption` passing the same parameters and return what IoTHubClientCore_LL_SetOption returns. **]**

**SRS_IOTHUBCLIENT_41_007: [** If parameter `optionName` is `OPTION_DO_WORK_FREQUENCY_IN_MS` then `value` should be of type `tickcounter_ms_t *`. **]**


## IoTHubClient_SetDeviceTwinCallback

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceTwinCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);
```

`IoTHubClient_SetDeviceTwinCallback` sets up the callback for Device Twin events which are sent from the IoTHub.

**SRS_IOTHUBCLIENT_10_001: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_10_002: [** If acquiring the lock fails, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_10_003: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_10_004: [** If starting the thread fails, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_10_005: [** `IoTHubClient_SetDeviceTwinCallback` shall call `IoTHubClient_LL_SetDeviceTwinCallback`, while passing the `IoTHubClient_LL handle` created by `IoTHubClient_LL_Create` along with the parameters `iothub_ll_device_twin_callback` and IOTHUB_QUEUE_CONTEXT variable. **]**

**SRS_IOTHUBCLIENT_10_006: [** When `IoTHubClient_LL_SetDeviceTwinCallback` is called, `IoTHubClient_SetDeviceTwinCallback` shall return the result of `IoTHubClient_LL_SetDeviceTwinCallback`. **]**

**SRS_IOTHUBCLIENT_10_020: [** `IoTHubClient_SetDeviceTwinCallback` shall be made thread-safe by using the lock created in IoTHubClient_Create. **]**

**SRS_IOTHUBCLIENT_07_002: [** `IoTHubClient_SetDeviceTwinCallback` shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the `IoTHubClient_LL_SetDeviceTwinCallback` function as a user context. **]**


## IoTHubClient_SendReportedState

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SendReportedState(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_PATCH_REPORTED_CALLBACK reportedStateCallback, void* userContextCallback);
```

`IoTHubClient_SendReportedState` sends an asynchronous reported state to the IoTHub. The response is returned via call to the specified reportedStateCallback.

**SRS_IOTHUBCLIENT_10_013: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_10_014: [** If acquiring the lock fails, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_10_015: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_10_016: [** If starting the thread fails, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_10_017: [** `IoTHubClient_SendReportedState` shall call `IoTHubClient_LL_SendReportedState`, while passing the `IoTHubClient_LL handle` created by `IoTHubClient_LL_Create` along with the parameters `reportedState`, `size`, `iothub_ll_reported_state_callback`, and IOTHUB_QUEUE_CONTEXT variable. **]**

**SRS_IOTHUBCLIENT_10_018: [** When `IoTHubClient_LL_SendReportedState` is called, `IoTHubClient_SendReportedState` shall return the result of `IoTHubClient_LL_SendReportedState`. **]**

**SRS_IOTHUBCLIENT_10_021: [** `IoTHubClient_SendReportedState` shall be made thread-safe by using the lock created in IoTHubClient_Create. **]**

**SRS_IOTHUBCLIENT_07_003: [** `IoTHubClient_SendReportedState` shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the `IoTHubClient_LL_SendReportedState` function as a user context. **]**


## IoTHubClientCore_GetDeviceTwinAsync
```c
extern IOTHUB_CLIENT_RESULT IoTHubClientCore_GetDeviceTwinAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);
```

`IoTHubClientCore_GetDeviceTwinAsync` retrieves the current complete Device Twin properties from the IoTHub (including Desired and Reported Properties).

**SRS_IOTHUBCLIENT_09_009: [** If `iotHubClientHandle` or `deviceTwinCallback` are `NULL`, `IoTHubClientCore_GetDeviceTwinAsync` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_09_010: [** The thread that executes the client  I/O shall be started if not running already. **]**

**SRS_IOTHUBCLIENT_09_011: [** If starting the thread fails, `IoTHubClientCore_GetDeviceTwinAsync` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_09_012: [** `IoTHubClientCore_GetDeviceTwinAsync` shall be made thread-safe by using the lock created in IoTHubClient_Create. **]**

**SRS_IOTHUBCLIENT_09_013: [** If acquiring the lock fails, `IoTHubClientCore_GetDeviceTwinAsync` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_09_014: [** `IoTHubClientCore_GetDeviceTwinAsync` shall call `IoTHubClientCore_LL_GetDeviceTwinAsync`, passing the `IoTHubClient_LL handle`, `deviceTwinCallback` and `userContextCallback` as arguments **]**

**SRS_IOTHUBCLIENT_09_015: [** When `IoTHubClientCore_LL_GetDeviceTwinAsync` is called, `IoTHubClientCore_GetDeviceTwinAsync` shall return the result of `IoTHubClientCore_LL_GetDeviceTwinAsync`. **]**



## IoTHubClient_SetDeviceMethodCallback

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceMethodCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback);
```

`IoTHubClient_SetDeviceMethodCallback` sets up the callback for a method which will be called by IoTHub.

**SRS_IOTHUBCLIENT_12_012: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetDeviceMethodCallback` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_12_013: [** If acquiring the lock fails, `IoTHubClient_SetDeviceMethodCallback` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_12_014: [** If the transport handle is `NULL` and the worker thread is not initialized, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_12_015: [** If starting the thread fails, `IoTHubClient_SetDeviceMethodCallback` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_12_016: [** `IoTHubClient_SetDeviceMethodCallback` shall call `IoTHubClient_LL_SetDeviceMethodCallback`, while passing the `IoTHubClient_LL_handle` created by `IoTHubClient_LL_Create` along with the parameters `deviceMethodCallback` and `userContextCallback`. **]**

**SRS_IOTHUBCLIENT_12_017: [** When `IoTHubClient_LL_SetDeviceMethodCallback` is called, `IoTHubClient_SetDeviceMethodCallback` shall return the result of `IoTHubClient_LL_SetDeviceMethodCallback`. **]**

**SRS_IOTHUBCLIENT_12_018: [** `IoTHubClient_SetDeviceMethodCallback` shall be made thread-safe by using the lock created in IoTHubClient_Create. **]**


## IoTHubClient_SetDeviceMethodCallback_Ex

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback);
```

`IoTHubClient_SetDeviceMethodCallback_Ex` sets up the callback for a method which will be called by IoTHub.

**SRS_IOTHUBCLIENT_07_001: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetDeviceMethodCallback_Ex` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_07_002: [** If acquiring the lock fails, `IoTHubClient_SetDeviceMethodCallback_Ex` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_07_003: [** If the transport handle is `NULL` and the worker thread is not initialized, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. **]**

**SRS_IOTHUBCLIENT_07_004: [** If starting the thread fails, `IoTHubClient_SetDeviceMethodCallback_Ex` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_07_005: [** `IoTHubClient_SetDeviceMethodCallback_Ex` shall call `IoTHubClient_LL_SetDeviceMethodCallback_Ex`, while passing the `IoTHubClient_LL_handle` created by `IoTHubClient_LL_Create` along with the parameters `iothub_ll_inbound_device_method_callback` and `IOTHUB_QUEUE_CONTEXT`. **]**

**SRS_IOTHUBCLIENT_07_008: [** If inboundDeviceMethodCallback is NULL, `IoTHubClient_SetDeviceMethodCallback_Ex` shall call `IoTHubClient_LL_SetDeviceMethodCallback_Ex`, passing NULL for the `iothub_ll_inbound_device_method_callback`. **]**

**SRS_IOTHUBCLIENT_07_006: [** When `IoTHubClient_LL_SetDeviceMethodCallback_Ex` is called, `IoTHubClient_SetDeviceMethodCallback_Ex` shall return the result of `IoTHubClient_LL_SetDeviceMethodCallback_Ex`. **]**

**SRS_IOTHUBCLIENT_07_007: [** `IoTHubClient_SetDeviceMethodCallback_Ex` shall be made thread-safe by using the lock created in IoTHubClient_Create. **]**

## IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK

```c
int(*IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK)(const char* method_name, const unsigned char* payload, size_t size, METHOD_ID_HANDLE method_id, void* userContextCallback);
```

**SRS_IOTHUB_MQTT_TRANSPORT_07_001: [** if userContextCallback is NULL, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a nonNULL value. **]**

**SRS_IOTHUB_MQTT_TRANSPORT_07_002: [** IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall copy the method_name and payload. **]**

**SRS_IOTHUB_MQTT_TRANSPORT_07_003: [** If a failure is encountered IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a non-NULL value. **]**

**SRS_IOTHUB_MQTT_TRANSPORT_07_004: [** On success IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a 0 value. **]**


## IoTHubClient_UploadToBlobAsync

```c
IOTHUB_CLIENT_RESULT IoTHubClient_UploadToBlobAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* destinationFileName, const unsigned char* source, size_t size, IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK iotHubClientFileUploadCallback, void* context);
```

`IoTHubClient_UploadToBlobAsync` asynchronously uploads the data pointed to by `source` having the size `size` to a file 
called `destinationFileName` in Azure Blob Storage and calls `iotHubClientFileUploadCallback` once the operation has completed

**SRS_IOTHUBCLIENT_02_047: [** If `iotHubClientHandle` is `NULL` then `IoTHubClient_UploadToBlobAsync` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_02_048: [** If `destinationFileName` is `NULL` then `IoTHubClient_UploadToBlobAsync` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_02_049: [** If `source` is `NULL` and size is greated than 0 then `IoTHubClient_UploadToBlobAsync` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_02_051: [** `IoTHubClient_UploadToBlobAsync` shall copy the `source`, `size`, `iotHubClientFileUploadCallback`, `context` into a structure. **]**

**SRS_IOTHUBCLIENT_02_058: [** `IoTHubClient_UploadToBlobAsync` shall add the structure to the list of structures that need to be cleaned once file upload finishes. **]**

**SRS_IOTHUBCLIENT_02_052: [** `IoTHubClient_UploadToBlobAsync` shall spawn a thread passing the structure build in SRS IOTHUBCLIENT 02 051 as thread data.]**

**SRS_IOTHUBCLIENT_02_053: [** If copying to the structure or spawning the thread fails, then `IoTHubClient_UploadToBlobAsync` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_02_054: [** The thread shall call `IoTHubClient_LL_UploadToBlob` passing the information packed in the structure. **]**

**SRS_IOTHUBCLIENT_02_055: [** If `IoTHubClient_LL_UploadToBlob` fails then the thread shall call the callback passing as result `FILE_UPLOAD_ERROR` and as context the structure from SRS IOTHUBCLIENT 02 051. **]**

**SRS_IOTHUBCLIENT_02_056: [** Otherwise the thread `iotHubClientFileUploadCallbackInternal` passing as result `FILE_UPLOAD_OK` and the structure from SRS IOTHUBCLIENT 02 051. **]**

**SRS_IOTHUBCLIENT_02_071: [** The thread shall mark itself as disposable. **]**


## IoTHubClient_SendEventToOutputAsync
```c
IOTHUB_CLIENT_RESULT IoTHubClient_SendEventToOutputAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
```

`IoTHubClient_SendEventToOutputAsync` sends a message to specified `outputName`

**SRS_IOTHUBCLIENT_31_100: [** If `iotHubClientHandle`, `outputName`, or `eventConfirmationCallback` is `NULL`, `IoTHubClient_SendEventToOutputAsync` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_31_101: [** `IoTHubClient_SendEventToOutputAsync` shall set the outputName of the message to send. **]**

**SRS_IOTHUBCLIENT_31_102: [** `IoTHubClient_SendEventToOutputAsync` shall invoke `IoTHubClient_SendEventAsync` to send the message. **]**


## IoTHubClient_SetInputMessageCallback 

```c
IOTHUB_CLIENT_RESULT IoTHubClient_SetInputMessageCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback);
```

`IoTHubClient_SetInputMessageCallback` sets up the callback for a method which will be called by IoTHub when the message arrives on `inputName` queue.

**SRS_IOTHUBCLIENT_31_098: [** `IoTHubClient_SetMessageCallback` shall start the worker thread if it was not previously started. **]**

**SRS_IOTHUBCLIENT_31_099: [** `IoTHubClient_SetMessageCallback` shall call `IoTHubClient_LL_SetInputMessageCallback`, passing its input arguments. **]**



## IoTHubClient_UploadMultipleBlocksToBlobAsync

```c
IOTHUB_CLIENT_RESULT IoTHubClient_UploadMultipleBlocksToBlobAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context);
IOTHUB_CLIENT_RESULT IoTHubClient_UploadMultipleBlocksToBlobAsyncEx(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallbackEx, void* context);
```

`IoTHubClient_UploadMultipleBlocksToBlobAsync` uploads data retrieved from the callback provided in `getDataCallback`.  `IoTHubClient_UploadMultipleBlocksToBlobAsyncEx` is identical, except the `getDataCallbackEx` returns
a value indicating whether to continue or abort the request.

`IoTHubClient_UploadMultipleBlocksToBlobAsync` asynchronously uploads multiples blocks of data to a file called `destinationFileName` in Azure Blob Storage. The blocks are provided by calling repetitively `getDataCallback` until it returns an empty block.

**SRS_IOTHUBCLIENT_99_072: [** If `iotHubClientHandle` is `NULL` then `IoTHubClient_UploadMultipleBlocksToBlobAsync(Ex)` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_99_073: [** If `destinationFileName` is `NULL` then `IoTHubClient_UploadMultipleBlocksToBlobAsync(Ex)` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_99_074: [** If `getDataCallback` is `NULL` then `IoTHubClient_UploadMultipleBlocksToBlobAsync(Ex)` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_99_075: [** `IoTHubClient_UploadMultipleBlocksToBlobAsync(Ex)` shall copy the `destinationFileName`, `getDataCallback`, `context`  and `iotHubClientHandle` into a structure. **]**

**SRS_IOTHUBCLIENT_99_076: [** `IoTHubClient_UploadMultipleBlocksToBlobAsync(Ex)` shall spawn a thread passing the structure build in SRS IOTHUBCLIENT 99 075 as thread data.]**

**SRS_IOTHUBCLIENT_99_077: [** If copying to the structure or spawning the thread fails, then `IoTHubClient_UploadMultipleBlocksToBlobAsync(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_99_078: [** The thread shall call `IoTHubClient_LL_UploadMultipleBlocksToBlob` or `IoTHubClient_LL_UploadMultipleBlocksToBlobEx` passing the information packed in the structure. **]**

**SRS_IOTHUBCLIENT_99_077: [** If copying to the structure and spawning the thread succeeds, then `IoTHubClient_UploadMultipleBlocksToBlobAsync(Ex)` shall return `IOTHUB_CLIENT_OK`. **]**
