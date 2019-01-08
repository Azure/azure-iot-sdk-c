# IoTHubClient_LL Requirements

## Overview

IoTHubClient_LL is a module allowing a user (usually a device) to communicate with an Azure IoTHub. It can send event messages and receive messages. At any given moment in time there can only be at most 1 message sink function.
Undelaying layer in the following requirements refers either to AMQP or HTTP.
Exposed API

```c
#define IOTHUB_CLIENT_RESULT_VALUES          \
    IOTHUB_CLIENT_OK,                        \
    IOTHUB_CLIENT_INVALID_ARG,               \
    IOTHUB_CLIENT_ERROR,                     \

DEFINE_ENUM(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

#define IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES     \
    IOTHUB_CLIENT_CONFIRMATION_OK,                   \
    IOTHUB_CLIENT_CONFIRMATION_TIMEOUT,              \
    IOTHUB_CLIENT_CONFIRMATION_ERROR                 \

DEFINE_ENUM(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

#define IOTHUB_CLIENT_STATUS_VALUES       \
    IOTHUB_CLIENT_SEND_STATUS_IDLE,       \
    IOTHUB_CLIENT_SEND_STATUS_BUSY        \

DEFINE_ENUM(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

#define IOTHUB_CLIENT_CONNECTION_STATUS_VALUES                                     \
    IOTHUB_CLIENT_CONNECTION_AUTHENTICATED,                \
    IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED               \

    DEFINE_ENUM(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);

#define IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES                               \
    IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN,            \
    IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED,              \
    IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL,               \
    IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED,                \
    IOTHUB_CLIENT_CONNECTION_NO_NETWORK,                   \
    IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR,          \
    IOTHUB_CLIENT_CONNECTION_OK                            \

    DEFINE_ENUM(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);

#define IOTHUB_CLIENT_RETRY_POLICY_VALUES     \
    IOTHUB_CLIENT_RETRY_NONE,                   \
    IOTHUB_CLIENT_RETRY_IMMEDIATE,                  \
    IOTHUB_CLIENT_RETRY_INTERVAL,      \
    IOTHUB_CLIENT_RETRY_LINEAR_BACKOFF,      \
    IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF,                 \
    IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER,                 \
    IOTHUB_CLIENT_RETRY_RANDOM

DEFINE_ENUM(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

#define IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES \
    IOTHUBMESSAGE_ACCEPTED, \
    IOTHUBMESSAGE_REJECTED, \
    IOTHUBMESSAGE_ABANDONED

DEFINE_ENUM(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);

typedef void* IOTHUB_CLIENT_HANDLE;
typedef void(*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK)(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback);
typedef void(*IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK)(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback);
typedef int(*IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC)(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE message, void* userContextCallback);
typedef void*(*IOTHUB_CLIENT_TRANSPORT_PROVIDER)(void);

typedef void(*IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback);
typedef void(*IOTHUB_CLIENT_REPORTED_STATE_CALLBACK)(int status_code, void* userContextCallback);
typedef int(*IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC)(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback);
typedef int(*IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK)(const char* method_name, const unsigned char* payload, size_t size, METHOD_ID_HANDLE method_id, void* userContextCallback);
 
typedef struct IOTHUB_CLIENT_CONFIG_TAG
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    const char* deviceId;
    const char* deviceKey;
    const char* deviceSasToken;
    const char* iotHubName;
    const char* iotHubSuffix;
    const char* protocolGatewayHostName;
} IOTHUB_CLIENT_CONFIG;

typedef struct IOTHUB_CLIENT_DEVICE_CONFIG_TAG
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    void * transportHandle;
    const char* deviceId;
    const char* deviceKey;
    const char* deviceSasToken;
} IOTHUB_CLIENT_DEVICE_CONFIG;


extern IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_Create(const IOTHUB_CLIENT_CONFIG* config);
extern  IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateWithTransport(IOTHUB_CLIENT_DEVICE_CONFIG * config);

extern void IoTHubClient_LL_Destroy(IOTHUB_CLIENT_HANDLE iotHubClientHandle);

extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendEventAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
extern void IoTHubClient_LL_DoWork(IOTHUB_CLIENT_HANDLE iotHubClientHandle);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetMessageCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetConnectionStatusCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimit);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimit);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetSendStatus(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetLastMessageReceiveTime(IOTHUB_CLIENT_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetOption(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* destinationFileName, const unsigned char* source, size_t size);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendEventToOutputAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetInputMessageCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadMultipleBlocksToBlob(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context);

## DeviceTwin
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceTwinCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendReportedState(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, uint32_t reportedVersion, uint32_t lastSeenDesiredVersion, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_GetDeviceTwinAsync(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);

## DeviceMethod
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceMethodCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_DeviceMethodResponse(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, uint32_t methodId, unsigned char* response, size_t responeSize);
```

## IoTHubClient_LL_CreateFromConnectionString

```c
extern IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
```

**SRS_IOTHUBCLIENT_LL_12_003: [** `IoTHubClient_LL_CreateFromConnectionString` shall verify the input parameters and if any of them `NULL` then return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_12_004: [** `IoTHubClient_LL_CreateFromConnectionString` shall allocate `IOTHUB_CLIENT_CONFIG` structure. **]**

**SRS_IOTHUBCLIENT_LL_12_012: [** If the allocation failed `IoTHubClient_LL_CreateFromConnectionString`  returns `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_12_005: [** `IoTHubClient_LL_CreateFromConnectionString` shall try to parse the `connectionString` input parameter for the following structure: "Key1=value1;key2=value2;key3=value3... ." ]**

**SRS_IOTHUBCLIENT_LL_12_013: [** If the parsing failed `IoTHubClient_LL_CreateFromConnectionString` returns `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_12_006: [** `IoTHubClient_LL_CreateFromConnectionString` shall verify the existence of the following Key/Value pairs in the connection string: `HostName`, `DeviceId`, `SharedAccessKey`, `SharedAccessSignature` or `x509`. **]**

**SRS_IOTHUBCLIENT_LL_12_014: [** If either of key is missing or x509 is not set to `"true"` then `IoTHubClient_LL_CreateFromConnectionString` returns `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_092: [** `IoTHubClient_LL_CreateFromConnectionString` shall create a `IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE` from `IOTHUB_CLIENT_CONFIG`. **]**

**SRS_IOTHUBCLIENT_LL_02_093: [** If creating the `IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE` fails then `IoTHubClient_LL_CreateFromConnectionString` shall fail and return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_12_009: [** `IoTHubClient_LL_CreateFromConnectionString` shall split the value of HostName to Name and Suffix using the first "." as a separator. **]**

**SRS_IOTHUBCLIENT_LL_12_015: [** If the string split failed, `IoTHubClient_LL_CreateFromConnectionString` returns `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_12_010: [** `IoTHubClient_LL_CreateFromConnectionString` shall fill up the `IOTHUB_CLIENT_CONFIG` structure using the following mapping: iotHubName = Name, iotHubSuffix = Suffix, deviceId = DeviceId, deviceKey = SharedAccessKey or deviceSasToken = SharedAccessSignature. **]**

**SRS_IOTHUBCLIENT_LL_03_010: [** `IoTHubClient_LL_CreateFromConnectionString` shall return `NULL` if both a `deviceKey` & a `deviceSasToken` are specified. **]**

**SRS_IOTHUBCLIENT_LL_12_011: [** `IoTHubClient_LL_CreateFromConnectionString` shall call into the `IoTHubClient_LL_Create` API with the current structure and returns with the return value of it. **]**

**SRS_IOTHUBCLIENT_LL_12_016: [** `IoTHubClient_LL_CreateFromConnectionString` shall return `NULL` if `IoTHubClient_LL_Create` call fails. **]**

**SRS_IOTHUBCLIENT_LL_04_001: [** `IoTHubClient_LL_CreateFromConnectionString` shall verify the existence of key/value pair GatewayHostName. If it does exist it shall pass the value to IoTHubClient_LL_Create API. **]**

**SRS_IOTHUBCLIENT_LL_04_002: [** If it does not, it shall pass the `protocolGatewayHostName` `NULL`. **]** 

**SRS_IOTHUBCLIENT_LL_31_126: [** `IoTHubClient_LL_CreateFromConnectionString` shall optionally parse `ModuleId`, if present.** ]**


## IoTHubClient_LL_Create

```c
extern IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_Create(const IOTHUB_CLIENT_CONFIG* config);
```

**SRS_IOTHUBCLIENT_LL_02_001: [** `IoTHubClient_LL_Create` shall return `NULL` if config parameter is `NULL` or protocol field is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_094: [** `IoTHubClient_LL_Create` shall create a `IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE` from `IOTHUB_CLIENT_CONFIG`. **]**

**SRS_IOTHUBCLIENT_LL_02_095: [** If creating the `IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE` fails then `IoTHubClient_LL_Create` shall fail and return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_045: [** Otherwise `IoTHubClient_LL_Create` shall create a new `TICK_COUNTER_HANDLE`. **]**

**SRS_IOTHUBCLIENT_LL_02_046: [** If creating the `TICK_COUNTER_HANDLE` fails then `IoTHubClient_LL_Create` shall fail and return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_004: [** Otherwise `IoTHubClient_LL_Create` shall initialize a new DLIST (further called "waitingToSend") containing records with fields of the following types: IOTHUB_MESSAGE_HANDLE, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*. **]**

**SRS_IOTHUBCLIENT_LL_02_006: [** `IoTHubClient_LL_Create` shall populate a structure of type `IOTHUBTRANSPORT_CONFIG` with the information from config parameter and the previous DLIST and shall pass that to the underlying layer `_Create` function. **]**

**SRS_IOTHUBCLIENT_LL_02_007: [** If the underlaying layer `_Create` function fails them `IoTHubClient_LL_Create` shall fail and return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_17_008: [** `IoTHubClient_LL_Create` shall call the transport _Register function with a populated structure of type `IOTHUB_DEVICE_CONFIG` and waitingToSend list. **]**

**SRS_IOTHUBCLIENT_LL_17_009: [** If the `_Register` function fails, this function shall fail and return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_008: [** Otherwise, `IoTHubClient_LL_Create` shall succeed and return a `non-NULL` handle. **]** 

**SRS_IOTHUBCLIENT_LL_25_124: [** `IoTHubClient_LL_Create` shall set the default retry policy as Exponential backoff with jitter and if succeed and return a `non-NULL` handle. **]**

**SRS_IOTHUBCLIENT_LL_09_010: [** If any failure occurs `IoTHubClient_LL_Create` shall destroy the `transportHandle` only if it has created it **]**

**SRS_IOTHUBCLIENT_LL_07_029: [** `IoTHubClient_LL_Create` shall create the Auth module with the device_key, device_id, deviceSasToken, and/or module_id values **]**

## IoTHubClient_LL_CreateWithTransport

```c
extern  IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateWithTransport(IOTHUB_CLIENT_DEVICE_CONFIG * config);
```

**SRS_IOTHUBCLIENT_LL_17_001: [** `IoTHubClient_LL_CreateWithTransport` shall return `NULL` if `config` parameter is `NULL`, or `protocol` field is `NULL` or `transportHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_098: [** `IoTHubClient_LL_CreateWithTransport` shall fail and return `NULL` if both `config->deviceKey` AND `config->deviceSasToken` are `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_17_002: [** `IoTHubClient_LL_CreateWithTransport` shall allocate data for the `IOTHUB_CLIENT_LL_HANDLE`. **]**

**SRS_IOTHUBCLIENT_LL_17_003: [** If allocation fails, the function shall fail and return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_096: [** `IoTHubClient_LL_CreateWithTransport` shall create the data structures needed to instantiate a `IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE`. **]**

**SRS_IOTHUBCLIENT_LL_02_097: [** If creating the data structures fails or instantiating the `IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE` fails then `IoTHubClient_LL_CreateWithTransport` shall fail and return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_047: [** `IoTHubClient_LL_CreateWithTransport` shall create a `TICK_COUNTER_HANDLE`. **]**

**SRS_IOTHUBCLIENT_LL_02_048: [** If creating the handle fails, then `IoTHubClient_LL_CreateWithTransport` shall fail and return `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_17_004: [** `IoTHubClient_LL_CreateWithTransport` shall initialize a new DLIST (further called "waitingToSend") containing records with fields of the following types: IOTHUB_MESSAGE_HANDLE, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void\*. **]**

**SRS_IOTHUBCLIENT_LL_17_005: [** `IoTHubClient_LL_CreateWithTransport` shall save the transport handle and mark this transport as shared. **]**

**SRS_IOTHUBCLIENT_LL_17_006: [** `IoTHubClient_LL_CreateWithTransport` shall call the transport _Register function with the `IOTHUB_DEVICE_CONFIG` populated structure and waitingToSend list. **]**

**SRS_IOTHUBCLIENT_LL_17_007: [** If the _Register function fails, this function shall fail and return `NULL`. **]** 

**SRS_IOTHUBCLIENT_LL_25_125: [** `IoTHubClient_LL_CreateWithTransport` shall set the default retry policy as Exponential backoff with jitter and if succeed and return a `non-NULL` handle. **]**

## IoTHubClient_LL_Destroy

```c
extern void IoTHubClient_LL_Destroy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle);
```

**SRS_IOTHUBCLIENT_LL_02_009: [** `IoTHubClient_LL_Destroy` shall do nothing if parameter `iotHubClientHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_033: [** Otherwise, `IoTHubClient_LL_Destroy` shall complete all the event message callbacks that are in the waitingToSend list with the result IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY. **]**

**SRS_IOTHUBCLIENT_LL_17_010: [** `IoTHubClient_LL_Destroy`  shall call the underlaying layer's _Unregister function. **]**

**SRS_IOTHUBCLIENT_LL_02_010: [** If `iotHubClientHandle` was not created by `IoTHubClient_LL_CreateWithTransport`, `IoTHubClient_LL_Destroy`  shall call the underlaying layer's _Destroy function. and shall free the resources allocated by `IoTHubClient` (if any). **]**

**SRS_IOTHUBCLIENT_LL_17_011: [** `IoTHubClient_LL_Destroy` shall free the resources allocated by `IoTHubClient` (if any). **]** 

**SRS_IOTHUBCLIENT_LL_07_007: [** `IoTHubClient_LL_Destroy` shall iterate the device twin queues and destroy any remaining items. **]**

**SRS_IOTHUBCLIENT_LL_31_141: [** `IoTHubClient_LL_Destroy` shall iterate registered callbacks for input queues and destroy any remaining items. **]**


## IoTHubClient_LL_SendEventAsync

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendEventAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_02_011: [** `IoTHubClient_LL_SendEventAsync` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` or `eventMessageHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_012: [** `IoTHubClient_LL_SendEventAsync` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `eventConfirmationCallback` is `NULL` and userContextCallback is not `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_013: [** `IoTHubClient_LL_SendEventAsync` shall add the DLIST waitingToSend a new record cloning the information from `eventMessageHandle`, `eventConfirmationCallback`, `userContextCallback`. **]**

**SRS_IOTHUBCLIENT_LL_02_014: [** If cloning and/or adding the information fails for any reason, `IoTHubClient_LL_SendEventAsync` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_015: [** Otherwise `IoTHubClient_LL_SendEventAsync` shall succeed and return `IOTHUB_CLIENT_OK`. **]**

## IoTHubClient_LL_SetMessageCallback

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetMessageCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_02_016: [** `IoTHubClient_LL_SetMessageCallback` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_02_017: [** If parameter `messageCallback` is `non-NULL` then `IoTHubClient_LL_SetMessageCallback` shall call the underlying layer's _Subscribe function. **]**

**SRS_IOTHUBCLIENT_LL_02_018: [** If the underlying layer's `_Subscribe` function fails, then `IoTHubClient_LL_SetMessageCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. Otherwise `IoTHubClient_LL_SetMessageCallback` shall succeed and return `IOTHUB_CLIENT_OK`. **]**

**SRS_IOTHUBCLIENT_LL_02_019: [** If parameter `messageCallback` is `NULL` and the `_SetMessageCallback` had been used to susbscribe for messages, then `IoTHubClient_LL_SetMessageCallback` shall call the underlying layer's _Unsubscribe function and return `IOTHUB_CLIENT_OK`. **]** 

**SRS_IOTHUBCLIENT_LL_10_010: [** If parameter `messageCallback` is `NULL` and the _SetMessageCallback had not been called to subscribe for messages, then `IoTHubClient_LL_SetMessageCallback` shall fail and return `IOTHUB_CLIENT_ERROR`.**] **

**SRS_IOTHUBCLIENT_LL_10_011: [** If parameter `messageCallback` is `non-NULL` and the `_SetMessageCallback_Ex` had been used to susbscribe for messages, then `IoTHubClient_LL_SetMessageCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. **]** 

## IoTHubClient_LL_DoWork

```c
extern void IoTHubClient_LL_DoWork(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle);
```

**SRS_IOTHUBCLIENT_LL_02_020: [** If parameter `iotHubClientHandle` is `NULL` then `IoTHubClient_LL_DoWork` shall not perform any action. **]**

**SRS_IOTHUBCLIENT_LL_02_021: [** Otherwise, `IoTHubClient_LL_DoWork` shall invoke the underlaying layer's _DoWork function. **]** 

**SRS_IOTHUBCLIENT_LL_07_008: [** `IoTHubClient_LL_DoWork` shall iterate the message queue and execute the underlying transports `IoTHubTransport_ProcessItem` function for each item. **]** 

**SRS_IOTHUBCLIENT_LL_07_010: [** If 'IoTHubTransport_ProcessItem' returns IOTHUB_PROCESS_CONTINUE or IOTHUB_PROCESS_NOT_CONNECTED `IoTHubClient_LL_DoWork` shall continue on to call the underlaying layer's _DoWork function. **]**  

**SRS_IOTHUBCLIENT_LL_07_011: [** If 'IoTHubTransport_ProcessItem' returns IOTHUB_PROCESS_OK `IoTHubClient_LL_DoWork` shall add the `IOTHUB_QUEUE_DATA_ITEM` to the ack queue. **]**

**SRS_IOTHUBCLIENT_LL_07_012: [** If 'IoTHubTransport_ProcessItem' returns any other value `IoTHubClient_LL_DoWork` shall destroy the `IOTHUB_QUEUE_DATA_ITEM` item. **]**

## IoTHubClient_LL_SendComplete

```c
void IoTHubClient_LL_SendComplete(IOTHUB_CLIENT_LL_HANDLE handle, PDLIST_ENTRY completed, IOTHUB_BATCHSTATE result)
```

**SRS_IOTHUBCLIENT_LL_02_022: [** If parameter `completed` is `NULL` or parameter `handle` is `NULL` then `IoTHubClient_LL_SendComplete` shall return. **]**

IoTHubClient_LL_SendComplete is a function that is only called by the lower layers. Completed is a PDLIST containing records of the same type as those created in the IoTHubClient _Create function. Result is a parameter that indicates that the result of the sending the batch.

**SRS_IOTHUBCLIENT_LL_02_025: [** If parameter `result` is `IOTHUB_BATCHSTATE_SUCCESS` then `IoTHubClient_LL_SendComplete` shall call all the `non-NULL` callbacks with the result parameter set to `IOTHUB_CLIENT_CONFIRMATION_OK` and the context set to the context passed originally in the `SendEventAsync` call. **]**

**SRS_IOTHUBCLIENT_LL_02_026: [** If any callback is `NULL` then there shall not be a callback call. **]**

**SRS_IOTHUBCLIENT_LL_02_027: [** If parameter result is `IOTHUB_BACTCHSTATE_FAILED` then `IoTHubClient_LL_SendComplete` shall call all the `non-NULL` callbacks with the result parameter set to `IOTHUB_CLIENT_CONFIRMATION_ERROR` and the context set to the context passed originally in the `SendEventAsync` call. **]**

## IoTHubClient_LL_MessageCallback

```c
extern IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubClient_LL_MessageCallback(IOTHUB_CLIENT_LL_HANDLE handle, MESSAGE_CALLBACK_INFO* messageData);
```

This function is only called by the lower layers upon receiving a message from IoTHub.
**SRS_IOTHUBCLIENT_LL_02_029: [** If either parameter `handle` or `messageData` is `NULL` then `IoTHubClient_LL_MessageCallback` shall return `false`. **]**

**SRS_IOTHUBCLIENT_LL_10_004: [** If the client is not subscribed to receive messages then `IoTHubClient_LL_MessageCallback` shall return `false`. **]**

**SRS_IOTHUBCLIENT_LL_02_030: [** If `messageCallbackType` is `LEGACY` then `IoTHubClient_LL_MessageCallback` shall invoke the last callback function (the parameter `messageCallback` to `IoTHubClient_LL_SetMessageCallback`) passing the message and the userContextCallback. **]**

**SRS_IOTHUBCLIENT_LL_10_007: [** If `messageCallbackType` is `LEGACY` then `IoTHubClient_LL_MessageCallback` shall send the message disposition as returned by the client to the underlying layer and return `true`. **]**

**SRS_IOTHUBCLIENT_LL_02_032: [** If `messageCallbackType` is `NONE` then `IoTHubClient_LL_MessageCallback` shall return `false`. **]**

**SRS_IOTHUBCLIENT_LL_10_009: [** If `messageCallbackType` is `ASYNC` then `IoTHubClient_LL_MessageCallback` shall return what `messageCallbac_Ex` returns. **]**


## IoTHubClient_LL_MessageCallbackFromInput
```c
extern bool IoTHubClient_LL_MessageCallbackFromInput(IOTHUB_CLIENT_LL_HANDLE handle, MESSAGE_CALLBACK_INFO* messageData);
```

This function is only called by the lower layers upon receiving a message from IoTHub directed to a specific input queue.

**SRS_IOTHUBCLIENT_LL_31_137: [** If either parameter `handle` or `messageData` is `NULL` then `IoTHubClient_LL_MessageCallbackFromInput` shall return `false`.** ]**

**SRS_IOTHUBCLIENT_LL_31_138: [** If there is no registered handler for the inputName from `IoTHubMessage_GetInputName`, then `IoTHubClient_LL_MessageCallbackFromInput` shall attempt invoke the default handler handler.** ]**

**SRS_IOTHUBCLIENT_LL_31_139: [** `IoTHubClient_LL_MessageCallbackFromInput` shall the callback from the given inputName queue if it has been registered.** ]**

**SRS_IOTHUBCLIENT_LL_31_140: [** `IoTHubClient_LL_MessageCallbackFromInput` shall send the message disposition as returned by the client to the underlying layer and return `true` if an input queue match is found.** ]**

## IoTHubClient_LL_SetMessageCallback_Ex

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetMessageCallback_Ex(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX messageCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_10_021: [** `IoTHubClient_LL_SetMessageCallback_Ex` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_10_018: [** If parameter `messageCallback` is `NULL` and `IoTHubClient_LL_SetMessageCallback_Ex` had not been used to subscribe for messages, then `IoTHubClient_LL_SetMessageCallback_Ex` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_10_019: [** If parameter `messageCallback` is `NULL` and `IoTHubClient_LL_SetMessageCallback` had been used to subscribe for messages, then `IoTHubClient_LL_SetMessageCallback_Ex` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_10_023: [** If parameter `messageCallback` is `NULL` then `IoTHubClient_LL_SetMessageCallback_Ex` shall call the underlying layer's `_Unsubscribe` function and return `IOTHUB_CLIENT_OK`. **]** 

**SRS_IOTHUBCLIENT_LL_10_020: [** If parameter `messageCallback` is `non-NULL`, and `IoTHubClient_LL_SetMessageCallback` had been used to subscribe for messages, then `IoTHubClient_LL_SetMessageCallback_Ex` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_10_024: [** If parameter `messageCallback` is `non-NULL` then `IoTHubClient_LL_SetMessageCallback_Ex` shall call the underlying layer's _Subscribe function. **]**

**SRS_IOTHUBCLIENT_LL_10_025: [** If the underlying layer's _Subscribe function fails, then `IoTHubClient_LL_SetMessageCallback_Ex` shall fail and return `IOTHUB_CLIENT_ERROR`. Otherwise `IoTHubClient_LL_SetMessageCallback_Ex` shall succeed and return `IOTHUB_CLIENT_OK`. **]**

## IoTHubClient_LL_SendMessageDisposition

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendMessageDisposition(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, MESSAGE_CALLBACK_INFO* messageData, IOTHUBMESSAGE_DISPOSITION_RESULT disposition);
```

**SRS_IOTHUBCLIENT_LL_10_026: [** `IoTHubClient_LL_SendMessageDisposition` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_10_027: [** `IoTHubClient_LL_SendMessageDisposition` shall return the result from calling the underlying layer's `_SendMessageDisposition`. **]**

## IoTHubClient_LL_GetSendStatus

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetSendStatus(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus);
```

**SRS_IOTHUBCLIENT_LL_09_007: [** `IoTHubClient_LL_GetSendStatus` shall return `IOTHUB_CLIENT_INVALID_ARG` if called with `NULL` parameter. **]**

**SRS_IOTHUBCLIENT_LL_09_008: [** `IoTHubClient_LL_GetSendStatus` shall return `IOTHUB_CLIENT_OK` and status `IOTHUB_CLIENT_SEND_STATUS_IDLE` if there is currently no items to be sent. **]**

**SRS_IOTHUBCLIENT_LL_09_009: [** `IoTHubClient_LL_GetSendStatus` shall return `IOTHUB_CLIENT_OK` and status `IOTHUB_CLIENT_SEND_STATUS_BUSY` if there are currently items to be sent. **]**

### IoTHubClient_LL_SetConnectionStatusCallback

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetConnectionStatusCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_25_111: [** IoTHubClient_LL_SetConnectionStatusCallback shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter iotHubClientHandle **]**

**SRS_IOTHUBCLIENT_LL_25_112: [** IoTHubClient_LL_SetConnectionStatusCallback shall return IOTHUB_CLIENT_OK and save the callback and userContext as a member of the handle. **]**

### IoTHubClient_LL_ConnectionStatusCallBack

```c
extern void IoTHubClient_LL_ConnectionStatusCallBack(IOTHUB_CLIENT_LL_HANDLE handle, IOTHUB_CLIENT_CONNECTION_STATUS connectionStatus, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
```

**SRS_IOTHUBCLIENT_LL_25_113: [** If parameter connectionStatus is NULL or parameter handle is NULL then IoTHubClient_LL_ConnectionStatusCallBack shall return. **]**

IoTHubClient_LL_ConnectionStatusCallBack is a function that is only called by the lower layers. connectionStatus represents the authentication state of the client to the IOTHUB with reason for change.

**SRS_IOTHUBCLIENT_LL_25_114: [** IoTHubClient_LL_ConnectionStatusCallBack shall call non-callback set by the user from IoTHubClient_LL_SetConnectionStatusCallback passing the status, reason and the passed userContextCallback. **]**

### IoTHubClient_LL_SetRetryPolicy

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitinSeconds);
```

**SRS_IOTHUBCLIENT_LL_25_116: [** IoTHubClient_LL_SetRetryPolicy shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL iotHubClientHandle **]**

**SRS_IOTHUBCLIENT_LL_25_118: [** IoTHubClient_LL_SetRetryPolicy shall save connection retry policies specified by the user to retryPolicy in struct IOTHUB_CLIENT_LL_HANDLE_DATA **]**

**SRS_IOTHUBCLIENT_LL_25_119: [** IoTHubClient_LL_SetRetryPolicy shall save retryTimeoutLimitinSeconds in seconds to retryTimeout in struct IOTHUB_CLIENT_LL_HANDLE_DATA **]**

### IoTHubClient_LL_GetRetryPolicy

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitinSeconds);
```

**SRS_IOTHUBCLIENT_LL_25_120: [** If `iotHubClientHandle`, `retryPolicy` or `retryTimeoutLimitinSeconds` is `NULL`, `IoTHubClient_LL_GetRetryPolicy` shall return `IOTHUB_CLIENT_INVALID_ARG` **]**

**SRS_IOTHUBCLIENT_LL_25_121: [** `IoTHubClient_LL_GetRetryPolicy` shall retrieve connection retry policy from `retryPolicy` in struct `IOTHUB_CLIENT_LL_HANDLE_DATA`**]**

**SRS_IOTHUBCLIENT_LL_25_122: [** `IoTHubClient_LL_GetRetryPolicy` shall retrieve `retryTimeoutLimit` in seconds from `retryTimeoutinSeconds` in struct `IOTHUB_CLIENT_LL_HANDLE_DATA`**]**

**SRS_IOTHUBCLIENT_LL_25_123: [** If user did not set the `policy` and `timeout` values by calling `IoTHubClient_LL_SetRetryPolicy` then `IoTHubClient_LL_GetRetryPolicy` shall return default values**]**

## IoTHubClient_LL_GetLastMessageReceiveTime

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetLastMessageReceiveTime(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime);
```

**SRS_IOTHUBCLIENT_LL_09_001: [** `IoTHubClient_LL_GetLastMessageReceiveTime` shall return `IOTHUB_CLIENT_INVALID_ARG` if any of the arguments is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_09_002: [** `IoTHubClient_LL_GetLastMessageReceiveTime` shall return `IOTHUB_CLIENT_INDEFINITE_TIME` - and not set `lastMessageReceiveTime` - if it is unable to provide the time for the last commands. **]**

**SRS_IOTHUBCLIENT_LL_09_003: [** `IoTHubClient_LL_GetLastMessageReceiveTime` shall return `IOTHUB_CLIENT_OK` if it wrote in the `lastMessageReceiveTime` the time when the last command was received. **]**

**SRS_IOTHUBCLIENT_LL_09_004: [** `IoTHubClient_LL_GetLastMessageReceiveTime` shall return `lastMessageReceiveTime` in localtime. **]** 

## IoTHubClient_LL_SetOption

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetOption(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value);
```

IoTHubClient_LL_SetOption sets the runtime option "optionName" to the value pointed to by value.

**SRS_IOTHUBCLIENT_LL_02_034: [** If `iotHubClientHandle` is `NULL` then `IoTHubClient_LL_SetOption` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_02_035: [** If `optionName` is `NULL` then `IoTHubClient_LL_SetOption` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_02_036: [** If `value` is `NULL` then `IoTHubClient_LL_SetOption` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**  

### Options that shall be handled by IoTHubClient_LL:

**SRS_IOTHUBCLIENT_LL_02_039: [** `messageTimeout` - once `IoTHubClient_LL_SendEventAsync` is called the message shall timeout after `*value` miliseconds. value is a pointer to a tickcounter_ms_t. **]**

**SRS_IOTHUBCLIENT_LL_02_041: [** If more than \*value miliseconds have passed since the call to `IoTHubClient_LL_SendEventAsync` then the message callback shall be called with a status code of `IOTHUB_CLIENT_CONFIRMATION_TIMEOUT`. **]**

**SRS_IOTHUBCLIENT_LL_02_042: [** By default, messages shall not timeout. **]**

**SRS_IOTHUBCLIENT_LL_02_043: [** Calling `IoTHubClient_LL_SetOption` with \*value set to "0" shall disable the timeout mechanism for all new messages. **]**

**SRS_IOTHUBCLIENT_LL_02_044: [** Messages already delivered to `IoTHubClient_LL` shall not have their timeouts modified by a new call to `IoTHubClient_LL_SetOption`. **]**

**SRS_IOTHUBCLIENT_LL_10_032: [** `product_info` - takes a char string as an argument to specify the product information(e.g. `ProductName/ProductVersion`). **]**

**SRS_IOTHUBCLIENT_LL_10_033: [** repeat calls with `product_info` will erase the previously set product information if applicatble. **]**

**SRS_IOTHUBCLIENT_LL_10_034: [** `product_info` - shall store the given string concatenated with the sdk information and the platform information in the form (ProductInfo DeviceSDKName/DeviceSDKVersion (OSName OSVersion; Architecture). **]**

**SRS_IOTHUBCLIENT_LL_10_035: [** If string concatenation fails, `IoTHubClient_LL_SetOption` shall return `IOTHUB_CLIENT_ERRROR`. Otherwise, `IOTHUB_CLIENT_OK` shall be returned. **]**

**SRS_IOTHUBCLIENT_LL_12_023: [** `c2d_keep_alive_freq_secs` - shall set the cloud to device keep alive frequency (in seconds) for the connection. Zero means keep alive will not be sent. **]**

**SRS_IOTHUBCLIENT_LL_30_010: [** `blob_upload_timeout_secs` - `IoTHubClient_LL_SetOption` shall pass this option to `IoTHubClient_UploadToBlob_SetOption` and return its result. **]**

**SRS_IOTHUBCLIENT_LL_30_011: [** `IoTHubClient_LL_SetOption` shall always pass unhandled options to `Transport_SetOption
`. **]**

**SRS_IOTHUBCLIENT_LL_30_012: [** If `Transport_SetOption` fails, `IoTHubClient_LL_SetOption` shall return that failure code. **]**

**SRS_IOTHUBCLIENT_LL_30_013: [** If the `DONT_USE_UPLOADTOBLOB` compiler switch is undefined,  `IoTHubClient_LL_SetOption` shall pass unhandled options to `IoTHubClient_UploadToBlob_SetOption` and ignore the result. **]**


## IoTHubClient_LL_UploadToBlob

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* destinationFileName, const unsigned char* source, size_t size);
```

`IoTHubClient_LL_UploadToBlob` calls `IoTHubClient_LL_UploadToBlob_Impl` to synchronously upload the data pointed to by `source` having the size `size` to a blob called `destinationFileName` in Azure Blob Storage.

Design considerations: IoTHubClient_LL_UploadToBlob_Impl uses IoTHubClient_LL_UploadMultipleBlocksToBlob to upload the blob. An internal callback FileUpload_GetData_Callback and the corresponding context are used to chunk the source and feed it to IoTHubClient_LL_UploadMultipleBlocksToBlob.

**SRS_IOTHUBCLIENT_LL_02_061: [** If `iotHubClientHandle` is `NULL` then `IoTHubClient_LL_UploadToBlob` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_02_062: [** If `destinationFileName` is `NULL` then `IoTHubClient_LL_UploadToBlob` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_02_063: [** If `source` is `NULL` and size is greater than 0 then `IoTHubClient_LL_UploadToBlob` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_99_001: [** `IoTHubClient_LL_UploadToBlob` shall create a struct containing the `source`, the `size`, and the remaining size to upload. **]**

**SRS_IOTHUBCLIENT_LL_99_002: [** `IoTHubClient_LL_UploadToBlob` shall call `IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl` with `FileUpload_GetData_Callback` as `getDataCallback` and pass the struct created at step SRS_IOTHUBCLIENT_LL_99_001 as `context`**]**

## IoTHubClient_LL_UploadMultipleBlocksToBlob

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadMultipleBlocksToBlob(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context);
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadMultipleBlocksToBlobEx(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx, void* context);
```

`IoTHubClient_LL_UploadMultipleBlocksToBlobEx` is identical to `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)`, except the callback pointer it takes can return an abort code.

`IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` calls `IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl` to synchronously upload the blocks provided by `getDataCallback` to a blob called `destinationFileName` in Azure Blob Storage.

**SRS_IOTHUBCLIENT_LL_99_005: [** If `iotHubClientHandle` is `NULL` then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_99_006: [** If `destinationFileName` is `NULL` then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_99_007: [** If `getDataCallback` is `NULL` then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_30_020: [** If the `blob_upload_timeout_secs` option has been set to non-zero, `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall set the timeout on the underlying transport accordingly. **]**

These are the 3 steps that are required to upload a file to Azure Blob Storage using IoTHub: 
step 1: get the SasUri components from IoTHub service
step 2: upload using the SasUri.
step 3: inform IoTHub that the upload has finished. 

### step 1: get the SasUri components from IoTHub service.

**SRS_IOTHUBCLIENT_LL_02_064: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall create an `HTTPAPIEX_HANDLE` to the IoTHub hostname. **]**

**SRS_IOTHUBCLIENT_LL_02_065: [** If creating the `HTTPAPIEX_HANDLE` fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_066: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall create an HTTP relative path formed from "/devices/" + deviceId + "/files/" + destinationFileName + "?api-version=API_VERSION". **]**

**SRS_IOTHUBCLIENT_LL_02_067: [** If creating the relativePath fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_32_001: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall create a JSON string formed from "{ \"blobName\": \" + destinationFileName + "\" }". **]**

**SRS_IOTHUBCLIENT_LL_32_002: [** if creating the JSON string fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR` **]**

**SRS_IOTHUBCLIENT_LL_02_068: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall create an HTTP responseContent BUFFER_HANDLE. **]**

**SRS_IOTHUBCLIENT_LL_02_069: [** If creating the HTTP response buffer handle fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_070: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall create request HTTP headers. **]**

**SRS_IOTHUBCLIENT_LL_02_071: [** If creating the HTTP headers fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_072: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall add the following name:value to request HTTP headers:  ]**
"Content-Type": "application/json"
"Accept": "application/json"
"User-Agent": "iothubclient/" IOTHUB_SDK_VERSION

**SRS_IOTHUBCLIENT_LL_02_073: [** If the credentials used to create `iotHubClientHandle` have "sasToken" then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall add 
the following HTTP request headers:  ]**

### "Authorization": original SAS token

**SRS_IOTHUBCLIENT_LL_02_074: [** If adding "Authorization" fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`.  **]**

**SRS_IOTHUBCLIENT_LL_32_003: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall execute `HTTPAPIEX_ExecuteRequest` passing the following information for arguments:  **]**

- HTTPAPIEX_HANDLE handle - the handle created at the beginning of `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)`
- HTTPAPI_REQUEST_TYPE requestType - HTTPAPI_REQUEST_POST
- const char* relativePath - the HTTP relative path
- HTTP_HEADERS_HANDLE requestHttpHeadersHandle - request HTTP headers
- BUFFER_HANDLE requestContent - the JSON string containing the file name
- unsigned int* statusCode - the address of an unsigned int that will contain the HTTP status code
- HTTP_HEADERS_HANDLE responseHttpHeadersHandle - NULL
- BUFFER_HANDLE responseContent - the HTTP response BUFFER_HANDLE

**SRS_IOTHUBCLIENT_LL_02_076: [** If `HTTPAPIEX_ExecuteRequest` call fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_077: [** If HTTP statusCode is greater than or equal to 300 then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_078: [** If the credentials used to create `iotHubClientHandle` have "deviceKey" then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall create an `HTTPAPIEX_SAS_HANDLE` passing as arguments:  ]**

- STRING_HANDLE key - deviceKey
- STRING_HANDLE uriResource - "/devices/" + deviceId
- STRING_HANDLE keyName - "" (empty string)

**SRS_IOTHUBCLIENT_LL_02_089: [** If creating the `HTTPAPIEX_SAS_HANDLE` fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_32_004: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall call `HTTPAPIEX_SAS_ExecuteRequest` passing as arguments:  ]**

- HTTPAPIEX_SAS_HANDLE sasHandle - the created HTTPAPIEX_SAS_HANDLE
- HTTPAPIEX_HANDLE handle - the created HTTPAPIEX_HANDLE
- HTTPAPI_REQUEST_TYPE requestType - HTTPAPI_REQUEST_POST
- const char* relativePath - the HTTP relative path
- HTTP_HEADERS_HANDLE requestHttpHeadersHandle - request HTTP headers
- BUFFER_HANDLE requestContent - the JSON string containing the file name
- unsigned int* statusCode - the address of an unsigned int that will contain the HTTP status code
- HTTP_HEADERS_HANDLE responseHeadersHandle - NULL
- BUFFER_HANDLE responseContent - the HTTP response BUFFER_HANDLE

**SRS_IOTHUBCLIENT_LL_02_079: [** If `HTTPAPIEX_SAS_ExecuteRequest` fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_080: [** If status code is greater than or equal to 300 then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

If the credentials used to create `iotHubClientHandle` do not have "deviceKey" or "deviceSasToken" then
**SRS_IOTHUBCLIENT_LL_02_106: [** - `x509certificate` and `x509privatekey` saved options shall be passed on the HTTPAPIEX_SetOption. **]**

**SRS_IOTHUBCLIENT_LL_02_111: [** If `certificates` is non-`NULL` then `certificates` shall be passed to HTTPAPIEX_SetOption with optionName `TrustedCerts`. **]**

**SRS_IOTHUBCLIENT_LL_02_107: [** - "Authorization" header shall not be build. **]**

**SRS_IOTHUBCLIENT_LL_32_005: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall execute HTTPAPIEX_ExecuteRequest passing the following information for arguments:  ]**

- HTTPAPIEX_HANDLE handle - the handle created at the beginning of `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)`
- HTTPAPI_REQUEST_TYPE requestType - HTTPAPI_REQUEST_POST
- const char* relativePath - the HTTP relative path
- HTTP_HEADERS_HANDLE requestHttpHeadersHandle - request HTTP headers
- BUFFER_HANDLE requestContent - the JSON string containing the file name
- unsigned int* statusCode - the address of an unsigned int that will contain the HTTP status code
- HTTP_HEADERS_HANDLE responseHttpHeadersHandle - NULL
- BUFFER_HANDLE responseContent - the HTTP response BUFFER_HANDLE

**SRS_IOTHUBCLIENT_LL_02_081: [** Otherwise, `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall use parson to extract and save the following information from the response buffer: correlationID and SasUri. **]**

**SRS_IOTHUBCLIENT_LL_02_082: [** If extracting and saving the correlationId or SasUri fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_32_008: [** The returned file name shall be URL encoded before passing back to the cloud. **]**

**SRS_IOTHUBCLIENT_LL_32_009: [** If `URL_EncodeString` fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

### step 2: upload using the SasUri

**SRS_IOTHUBCLIENT_LL_02_083: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall call `Blob_UploadMultipleBlocksFromSasUri` and capture the HTTP return code and HTTP body. **]**

**SRS_IOTHUBCLIENT_LL_02_084: [** If `Blob_UploadMultipleBlocksFromSasUri` fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

### step 3: inform IoTHub that the upload has finished

**SRS_IOTHUBCLIENT_LL_02_085: [** `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall use the same authorization as step 1. to prepare and perform a HTTP request with the following parameters:  **]**

- HTTPAPI_REQUEST_TYPE requestType - HTTPAPI_REQUEST_POST
- const char* relativePath - "/devices" + deviceId + "/files/notifications/" + correlationId + "?api-version" API_VERSION
- HTTP message body: a JSON encoding of an object having the following structure:

```json
{
    "statusCode": what_Blob_UploadFromSasUri_returned_as_HTTP_code,
    "statusDescription": "what_Blob_UploadFromSasUri_returned_as_HTTP_message_body"
}
```

**SRS_IOTHUBCLIENT_LL_02_091: [** If step 2 fails without establishing an HTTP dialogue, then the HTTP message body shall look like:  **]**

```json
{
    "statusCode": -1
    "statusDescription": "client not able to connect with the server"
}
```
**SRS_IOTHUBCLIENT_LL_99_008: [** If step 2 is aborted by the client, then the HTTP message body shall look like:  **]**

```json
{
    "statusCode": -1
    "statusDescription": "file upload aborted"
}
```

**SRS_IOTHUBCLIENT_LL_99_009: [** If step 2 is aborted by the client and if step 3 succeeds, then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall return `IOTHUB_CLIENT_OK`. **]**

**SRS_IOTHUBCLIENT_LL_02_086: [** If performing the HTTP request fails then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_087: [** If the statusCode of the HTTP request is greater than or equal to 300 then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_088: [** Otherwise, `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall succeed and return `IOTHUB_CLIENT_OK`. **]**

**SRS_IOTHUBCLIENT_LL_99_003: [** If `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` return `IOTHUB_CLIENT_OK`, it shall call `getDataCallback` with `result` set to `FILE_UPLOAD_OK`, and `data` and `size` set to NULL. **]**

**SRS_IOTHUBCLIENT_LL_99_004: [** If `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` does not return `IOTHUB_CLIENT_OK`, it shall call `getDataCallback` with `result` set to `FILE_UPLOAD_ERROR`, and `data` and `size` set to NULL. **]**

## IoTHubClient_LL_UploadToBlob_SetOption

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_SetOption(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* optionName, const void* value)
```

IoTHubClient_LL_UploadToBlob_SetOption sets an option for UploadToBlob.

**SRS_IOTHUBCLIENT_LL_02_110: [** If parameter `handle` is `NULL` then `IoTHubClient_LL_UploadToBlob_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

Handled options are

**SRS_IOTHUBCLIENT_LL_02_100: [** `x509certificate` - then `value` then is a null terminated string that contains the x509 certificate. **]**

**SRS_IOTHUBCLIENT_LL_02_101: [** `x509privatekey` - then `value` is a null terminated string that contains the x509 privatekey. **]**

**SRS_IOTHUBCLIENT_LL_30_000: [** `blob_upload_timeout_secs` - shall set the timeout in seconds for blob transfer operations. **]**

**SRS_IOTHUBCLIENT_LL_30_001: [** A `blob_upload_timeout_secs` value of 0 shall not set any timeout on the transport (default behavior). **]**

**SRS_IOTHUBCLIENT_LL_02_102: [** If an unknown option is presented then `IoTHubClient_LL_UploadToBlob_SetOption` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_02_109: [** If the authentication scheme is NOT x509 then `IoTHubClient_LL_UploadToBlob_SetOption` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_02_103: [** The options shall be saved. **]**

**SRS_IOTHUBCLIENT_LL_02_104: [** If saving fails, then `IoTHubClient_LL_UploadToBlob_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_02_105: [** Otherwise `IoTHubClient_LL_UploadToBlob_SetOption` shall succeed and return `IOTHUB_CLIENT_OK`. **]**

**SRS_IOTHUBCLIENT_LL_32_008: [** OPTION_HTTP_PROXY - then the value will be a pointer to HTTP_PROXY_OPTIONS structure. **]**

**SRS_IOTHUBCLIENT_LL_32_006: [** If `host_address` is NULL, `IoTHubClient_LL_UploadToBlob_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`**]**

**SRS_IOTHUBCLIENT_LL_32_007: [** If only one of `username` and `password` is NULL, `IoTHubClient_LL_UploadToBlob_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. **]**

## IoTHubClient_LL_SetDeviceTwinCallback

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceTwinCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_10_001: [** `IoTHubClient_LL_SetDeviceTwinCallback` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_10_002: [** If `deviceTwinCallback` is not `NULL`, then `IoTHubClient_LL_SetDeviceTwinCallback` shall call the underlying layer's `_Subscribe` function. **]**

**SRS_IOTHUBCLIENT_LL_10_003: [** If the underlying layer's `_Subscribe` function fails, then `IoTHubClient_LL_SetDeviceTwinCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_10_005: [** Otherwise `IoTHubClient_LL_SetDeviceTwinCallback` shall succeed and return `IOTHUB_CLIENT_OK`. **]**

**SRS_IOTHUBCLIENT_LL_10_006: [** If `deviceTwinCallback` is `NULL`, then `IoTHubClient_LL_SetDeviceTwinCallback` shall call the underlying layer's `_Unsubscribe` function and return `IOTHUB_CLIENT_OK`. **]**

## IoTHubClient_LL_SendReportedState

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendReportedState(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_10_012: [** `IoTHubClient_LL_SendReportedState` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_10_013: [** `IoTHubClient_LL_SendReportedState` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `reportedState` is `NULL`]**

**SRS_IOTHUBCLIENT_LL_07_005: [** `IoTHubClient_LL_SendReportedState` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `size` is equal to 0. **]**

**SRS_IOTHUBCLIENT_LL_10_014: [** `IoTHubClient_LL_SendReportedState` shall construct a Device_Twin structure containing reportedState data. **]**

**SRS_IOTHUBCLIENT_LL_07_001: [** `IoTHubClient_LL_SendReportedState` shall queue the constructed reportedState data to be consumed by the targeted transport. **]**

**SRS_IOTHUBCLIENT_LL_10_015: [** If any error is encountered `IoTHubClient_LL_SendReportedState` shall return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_10_016: [** Otherwise `IoTHubClient_LL_SendReportedState` shall succeed and return `IOTHUB_CLIENT_OK`. **]**

**SRS_IOTHUBCLIENT_LL_10_017: [** If parameter `reportedStateCallback` is `NULL`, `IoTHubClient_LL_SendReportedState` shall send the reported state without any notification upon the message reaching the iothub. **]**

## IoTHubClient_LL_ReportedStateComplete

```c
void IoTHubClient_LL_ReportedStateComplete(IOTHUB_CLIENT_LL_HANDLE handle, uint32_t item_id, int status_code)
```

IoTHubClient_LL_ReportedStateComplete is a function only called by the transport that signals the completed transmission of a device_twin message to iothub, or an error.

**SRS_IOTHUBCLIENT_LL_07_002: [** If handle is `NULL` then `IoTHubClient_LL_ReportedStateComplete` shall do nothing. **]**

**SRS_IOTHUBCLIENT_LL_07_003: [** `IoTHubClient_LL_ReportedStateComplete` shall enumerate through the `IOTHUB_QUEUE_DATA_ITEM` structures in queue_handle. **]**

**SRS_IOTHUBCLIENT_LL_07_004: [** If the `IOTHUB_QUEUE_DATA_ITEM`'s `reported_state_callback` variable is non-`NULL` then `IoTHubClient_LL_ReportedStateComplete` shall call the function. **]**

**SRS_IOTHUBCLIENT_LL_07_009: [** `IoTHubClient_LL_ReportedStateComplete` shall remove the `IOTHUB_QUEUE_DATA_ITEM` item from the ack queue.]**

## IoTHubClient_LL_RetrievePropertyComplete

```c
void IoTHubClient_LL_RetrievePropertyComplete(IOTHUB_CLIENT_LL_HANDLE handle, DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size)
```

**SRS_IOTHUBCLIENT_LL_07_013: [** If `handle` is `NULL` then `IoTHubClient_LL_RetrievePropertyComplete` shall return. **]**

**SRS_IOTHUBCLIENT_LL_07_014: [** If `deviceTwinCallback` is `NULL` then `IoTHubClient_LL_RetrievePropertyComplete` shall return. **]**

**SRS_IOTHUBCLIENT_LL_07_015: [** If the the `update_state` parameter is `DEVICE_TWIN_UPDATE_PARTIAL` and a `DEVICE_TWIN_UPDATE_COMPLETE` message has not been previously received then `IoTHubClient_LL_RetrievePropertyComplete` shall return. **]**

**SRS_IOTHUBCLIENT_LL_07_016: [** If `deviceTwinCallback` is set and `DEVICE_TWIN_UPDATE_COMPLETE` has been encountered then `IoTHubClient_LL_RetrievePropertyComplete` shall call `deviceTwinCallback`. **]**


## IoTHubClientCore_LL_GetDeviceTwinAsync

```c
extern IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_GetDeviceTwinAsync(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_09_011: [** If `iotHubClientHandle` or `deviceTwinCallback` are `NULL`, `IoTHubClientCore_LL_GetDeviceTwinAsync` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`.** ]**

**SRS_IOTHUBCLIENT_LL_09_012: [** IoTHubClientCore_LL_GetDeviceTwinAsync shall invoke IoTHubTransport_GetDeviceTwinAsync, passing `on_device_twin_report_received` and the user data as context ** ]**

**SRS_IOTHUBCLIENT_LL_09_013: [** If IoTHubTransport_GetDeviceTwinAsync fails, `IoTHubClientCore_LL_GetDeviceTwinAsync` shall fail and return `IOTHUB_CLIENT_ERROR`.** ]**

**SRS_IOTHUBCLIENT_LL_09_014: [** If no errors occur IoTHubClientCore_LL_GetDeviceTwinAsync shall return `IOTHUB_CLIENT_OK`.** ]**


### on_device_twin_report_received

```
static void on_device_twin_report_received(IOTHUB_TRANSPORT_RESULT result, CONSTBUFFER_HANDLE data, void* callbackContext);
```

Note: `userContextCallback` bellow refers to the callback provided in IoTHubClient_LL_GetDeviceTwin.

**SRS_IOTHUBCLIENT_LL_09_015: [** If data is NULL or result is not IOTHUB_TRANSPORT_OK, `deviceTwinCallback` (user provided) shall be invoked passing DEVICE_TWIN_UPDATE_COMPLETE, NULL for `payload`, 0 for `size` and `userContextCallback`** ]**

**SRS_IOTHUBCLIENT_LL_09_016: [** Otherwise `deviceTwinCallback` (user provided) shall be invoked passing DEVICE_TWIN_UPDATE_COMPLETE, `data->buffer`, `data->size` and `userContextCallback`** ]**

**SRS_IOTHUBCLIENT_LL_09_017: [** If `data` is not NULL it shall be destroyed** ]**




## IoTHubClient_LL_SetDeviceMethodCallback

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceMethodCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
```

**SRS_IOTHUBCLIENT_LL_12_017: [** `IoTHubClient_LL_SetDeviceMethodCallback` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. **]**

**SRS_IOTHUBCLIENT_LL_12_018: [** If `deviceMethodCallback` is `NULL`, then `IoTHubClient_LL_SetDeviceMethodCallback` shall call the underlying layer's `IoTHubTransport_Unsubscribe_DeviceMethod` function and return `IOTHUB_CLIENT_OK`. **]**

**SRS_IOTHUBCLIENT_LL_10_029: [** If `deviceMethodCallback` is `NULL` and the client is not subscribed to receive method calls, `IoTHubClient_LL_SetDeviceMethodCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_12_019: [** If `deviceMethodCallback` is not `NULL`, then `IoTHubClient_LL_SetDeviceMethodCallback` shall call the underlying layer's `IoTHubTransport_Subscribe_DeviceMethod` function. **]**

**SRS_IOTHUBCLIENT_LL_12_020: [** If the underlying layer's `IoTHubTransport_Subscribe_DeviceMethod` function fails, then `IoTHubClient_LL_SetDeviceMethodCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_10_028: [** If the user has subscribed using `IoTHubClient_LL_SetDeviceMethodCallback_Ex`, `IoTHubClient_LL_SetDeviceMethodCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_12_021: [** If adding the information fails for any reason, `IoTHubClient_LL_SetDeviceMethodCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_12_022: [** Otherwise `IoTHubClient_LL_SetDeviceMethodCallback` shall succeed and return `IOTHUB_CLIENT_OK`. **]**

## IoTHubClient_LL_DeviceMethodComplete

```c
int IoTHubClient_LL_DeviceMethodComplete(IOTHUB_CLIENT_LL_HANDLE handle, const unsigned char* payLoad, size_t size, BUFFER_HANDLE response)
```

**SRS_IOTHUBCLIENT_LL_07_017: [** If `handle` or response is NULL then `IoTHubClient_LL_DeviceMethodComplete` shall return 500. **]**

**SRS_IOTHUBCLIENT_LL_07_018: [** If `deviceMethodCallback` is not NULL `IoTHubClient_LL_DeviceMethodComplete` shall execute `deviceMethodCallback` and return the status. **]**

**SRS_IOTHUBCLIENT_LL_07_019: [** If `deviceMethodCallback` is NULL `IoTHubClient_LL_DeviceMethodComplete` shall return 404. **]**

**SRS_IOTHUBCLIENT_LL_07_020: [** `deviceMethodCallback` shall buil the BUFFER_HANDLE with the response payload from the `IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC` callback. **]**

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_LL_HANDLE handle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_07_021: [** If `handle` is `NULL` then `IoTHubClient_LL_SetDeviceMethodCallback_Ex` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_07_022: [** If `inboundDeviceMethodCallback` is NULL then `IoTHubClient_LL_SetDeviceMethodCallback_Ex` shall call the underlying layer's `IoTHubTransport_Unsubscribe_DeviceMethod` function and return IOTHUB_CLIENT_OK. **]**

**SRS_IOTHUBCLIENT_LL_10_030: [** If `deviceMethodCallback` is `NULL` and the client is not subscribed to receive method calls, `IoTHubClient_LL_SetDeviceMethodCallback_Ex` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_07_023: [** If `inboundDeviceMethodCallback` is non-NULL then `IoTHubClient_LL_SetDeviceMethodCallback_Ex` shall call the underlying layer's `IoTHubTransport_Subscribe_DeviceMethod` function. **]**

**SRS_IOTHUBCLIENT_LL_10_031: [** If the user has subscribed using `IoTHubClient_LL_SetDeviceMethodCallback`, `IoTHubClient_LL_SetDeviceMethodCallback_Ex` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**

**SRS_IOTHUBCLIENT_LL_07_024: [** If `inboundDeviceMethodCallback` is non-NULL then `IoTHubClient_LL_SetDeviceMethodCallback_Ex` shall set the inboundDeviceMethodCallback to NULL. **]**

**SRS_IOTHUBCLIENT_LL_07_025: [** If any error is encountered then `IoTHubClient_LL_SetDeviceMethodCallback_Ex` shall return `IOTHUB_CLIENT_ERROR`. **]**

```c
extern IOTHUB_CLIENT_RESULT IoTHubClient_LL_DeviceMethodResponse(IOTHUB_CLIENT_LL_HANDLE handle, METHOD_ID_HANDLE methodId, unsigned char* response, size_t resp_size, int status_response);
```

**SRS_IOTHUBCLIENT_LL_07_026: [** If `handle` or methodId is `NULL` then `IoTHubClient_LL_DeviceMethodResponse` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_07_027: [** `IoTHubClient_LL_DeviceMethodResponse` shall call the `IoTHubTransport_DeviceMethod_Response` transport function. **]**

**SRS_IOTHUBCLIENT_LL_07_028: [** If the transport `IoTHubTransport_DeviceMethod_Response` succeed then, `IoTHubClient_LL_DeviceMethodResponse` shall return `IOTHUB_CLIENT_OK` Otherwise it shall return `IOTHUB_CLIENT_ERROR`. **]** 

## IoTHubClient_LL_CreateFromDeviceAuth

```c
IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateFromDeviceAuth(const char* iothub_uri, const char* device_id, XDA_HANDLE device_auth_handle, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
```

**SRS_IOTHUBCLIENT_LL_07_029: [** if `iothub_uri`, `protocol`, `device_id`, or `device_auth_handle` are NULL, `IoTHubClient_LL_CreateFromDeviceAuth` shall return NULL. **]**

**SRS_IOTHUBCLIENT_LL_07_030: [** `IoTHubClient_LL_CreateFromDeviceAuth` shall allocate data for the IOTHUB_CLIENT_LL_HANDLE. **]**

**SRS_IOTHUBCLIENT_LL_07_031: [** `IoTHubClient_LL_CreateFromDeviceAuth` shall initialize the IOTHUB_CLIENT_LL_HANDLE_DATA by calling initialize_iothub_client. **]**

**SRS_IOTHUBCLIENT_LL_07_032: [** If any error is encountered `IoTHubClient_LL_CreateFromDeviceAuth` shall return NULL. **]**

## create_iothub_client_data

```c
static IOTHUB_CLIENT_LL_HANDLE_DATA* create_iothub_client_data(IOTHUB_CLIENT_CONFIG* client_config, TRANSPORT_LL_HANDLE transport_handle, TRANSPORT_PROVIDER* protocol)
```

**SRS_IOTHUBCLIENT_LL_07_033: [** `create_iothub_client_data` shall initialize IOTHUB_CLIENT_LL_HANDLE_DATA variables. **]**

**SRS_IOTHUBCLIENT_LL_07_034: [** If any error is encountered `create_iothub_client_data` shall return NULL. **]**

**SRS_IOTHUBCLIENT_LL_07_035: [** `create_iothub_client_data` shall create a IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE from IOTHUB_CLIENT_CONFIG. **]**

**SRS_IOTHUBCLIENT_LL_07_036: [** `create_iothub_client_data` shall initialize a new DLIST (further called "waitingToSend") containing records with fields of the following types: IOTHUB_MESSAGE_HANDLE, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*. **]**

**SRS_IOTHUBCLIENT_LL_07_037: [** `create_iothub_client_data` shall populate a structure of type IOTHUBTRANSPORT_CONFIG with the information from config parameter and the previous DLIST and shall pass that to the underlying layer _Create function. **]**

**SRS_IOTHUBCLIENT_LL_07_038: [** If the underlaying layer _Create function fails them `create_iothub_client_data` shall fail and return NULL. **]**

**SRS_IOTHUBCLIENT_LL_07_039: [** `create_iothub_client_data` shall call the transport _Register function with a populated structure of type IOTHUB_DEVICE_CONFIG and waitingToSend list. **]**

**SRS_IOTHUBCLIENT_LL_07_040: [** `create_iothub_client_data` shall set the default retry policy as Exponential backoff with jitter and if succeed and return a `non-NULL` handle. **]**

## IoTHubClient_LL_SendEventToOutputAsync

```c
IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendEventToOutputAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_31_127: [** If `iotHubClientHandle`, `outputName`, or `eventConfirmationCallback` is `NULL`, `IoTHubClient_LL_SendEventToOutputAsync` shall return `IOTHUB_CLIENT_INVALID_ARG`. **]**

**SRS_IOTHUBCLIENT_LL_31_128: [** `IoTHubClient_LL_SendEventToOutputAsync` shall set the outputName of the message to send. **]**

**SRS_IOTHUBCLIENT_LL_31_129: [** `IoTHubClient_LL_SendEventToOutputAsync` shall invoke `IoTHubClient_LL_SendEventAsync` to send the message. **]**


## IoTHubClient_LL_SetInputMessageCallback

```c
IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetInputMessageCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback);
```

**SRS_IOTHUBCLIENT_LL_31_130: [** If `iotHubClientHandle` or `inputName` is NULL, `IoTHubClient_LL_SetInputMessageCallback` shall return IOTHUB_CLIENT_INVALID_ARG. **]**

**SRS_IOTHUBCLIENT_LL_31_131: [** If `eventHandlerCallback` is NULL, `IoTHubClient_LL_SetInputMessageCallback` shall remove the `inputName` from its callback list if present. **]**

**SRS_IOTHUBCLIENT_LL_31_132: [** If `eventHandlerCallback` is NULL, `IoTHubClient_LL_SetInputMessageCallback` shall return `IOTHUB_CLIENT_ERROR` if the `inputName` is not present. **]**

**SRS_IOTHUBCLIENT_LL_31_133: [** If `eventHandlerCallback` is NULL, `IoTHubClient_LL_SetInputMessageCallback` shall invoke `IoTHubTransport_Unsubscribe_InputQueue` if this was the last input callback. **]**

**SRS_IOTHUBCLIENT_LL_31_134: [** `IoTHubClient_LL_SetInputMessageCallback` shall allocate a callback handle to associate callbacks from the transport => client if `inputName` isn't already present in the callback list. **]**

**SRS_IOTHUBCLIENT_LL_31_135: [** `IoTHubClient_LL_SetInputMessageCallback` shall reuse the existing callback handle if `inputName` is already present in the callback list. **]**

**SRS_IOTHUBCLIENT_LL_31_136: [** `IoTHubClient_LL_SetInputMessageCallback` shall invoke `IoTHubTransport_Subscribe_InputQueue` if this is the first callback being registered. **]**


## IoTHubClient_LL_SetInputMessageCallbackEx

```c
IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetInputMessageCallbackEx(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX eventHandlerCallbackEx, void *userContextCallbackEx, size_t userContextCallbackExLength)
```

This function uses the same logic as `IoTHubClient_LL_SetInputMessageCallback` but uses a context and length.  It is for internal use only.

**SRS_IOTHUBCLIENT_LL_31_141: [** `IoTHubClient_LL_SetInputMessageCallbackEx` shall copy the data passed in extended context. **]**

