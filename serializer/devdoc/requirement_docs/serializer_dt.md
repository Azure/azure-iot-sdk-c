# serializer_dt

serializer_dt.h is a header file that defines the macros an application can use to serialize and execute model commands in device twin context.
It inherits all the features of serializer.h. The glue code between serializer.h and iothub client is realized by the following functions: 

The new additions are:
```c

DECLARE_DEVICETWIN_MODEL(modelName, ... );

and several static functions:

static void serializer_ingest(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
static int deviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
static void* IoTHubDeviceTwinCreate_Impl(const char* name, size_t sizeOfName, SERIALIZER_DEVICETWIN_PROTOHANDLE* protoHandle)
static void IoTHubDeviceTwin_Destroy_Impl(void* model)
```

### serializer_ingest
```c
void serializer_ingest(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
```

`serializer_ingest` takes a payload consisting of `payLoad` and `size` (at the moment assumed to be containing a JSON value), removes clutter elements from it
and then updates the desired properties based on the payload.

**SRS_SERIALIZERDEVICETWIN_02_001: [** `serializer_ingest` shall clone the payload into a null terminated string. **]**

**SRS_SERIALIZERDEVICETWIN_02_002: [** `serializer_ingest` shall parse the null terminated string into parson data types. **]**

**SRS_SERIALIZERDEVICETWIN_02_008: [** If any of the above operations fail, then `serializer_ingest` shall return. **]**

### IoTHubDeviceTwinCreate_Impl
```c
static void* IoTHubDeviceTwinCreate_Impl(const char* name, size_t sizeOfName, SERIALIZER_DEVICETWIN_PROTOHANDLE* protoHandle)
```

`IoTHubDeviceTwinCreate_Impl` creates a device of type `name`, links it to IoTHubClient or IoTHubClient_LL and returns it.

**SRS_SERIALIZERDEVICETWIN_02_009: [** `IoTHubDeviceTwinCreate_Impl` shall locate the model and the metadata for `name` by calling Schema_GetSchemaForModel/Schema_GetMetadata/Schema_GetModelByName. **]**

**SRS_SERIALIZERDEVICETWIN_02_010: [** `IoTHubDeviceTwinCreate_Impl` shall call CodeFirst_CreateDevice. **]**

**SRS_SERIALIZERDEVICETWIN_02_011: [** `IoTHubDeviceTwinCreate_Impl` shall set the device twin callback. **]**

**SRS_SERIALIZERDEVICETWIN_02_027: [** `IoTHubDeviceTwinCreate_Impl` shall set the device method callback **]**

**SRS_SERIALIZERDEVICETWIN_02_012: [** `IoTHubDeviceTwinCreate_Impl` shall record the pair of (device, IoTHubClient(_LL)). **]**

**SRS_SERIALIZERDEVICETWIN_02_013: [** If all operations complete successfully then `IoTHubDeviceTwinCreate_Impl` shall succeeds and return a non-`NULL` value. **]**

**SRS_SERIALIZERDEVICETWIN_02_014: [** Otherwise, `IoTHubDeviceTwinCreate_Impl` shall fail and return `NULL`. **]**

### IoTHubDeviceTwin_Destroy_Impl
```c
static void IoTHubDeviceTwin_Destroy_Impl(void* model)
```

`IoTHubDeviceTwin_Destroy_Impl` frees all used resources created by `IoTHubDeviceTwinCreate_Impl`.

**SRS_SERIALIZERDEVICETWIN_02_020: [** If `model` is `NULL` then `IoTHubDeviceTwin_Destroy_Impl` shall return. **]**

**SRS_SERIALIZERDEVICETWIN_02_015: [** `IoTHubDeviceTwin_Destroy_Impl` shall locate the saved handle belonging to `model`. **]**

**SRS_SERIALIZERDEVICETWIN_02_016: [** `IoTHubDeviceTwin_Destroy_Impl` shall set the devicetwin callback to `NULL`. **]**

**SRS_SERIALIZERDEVICETWIN_02_028: [** `IoTHubDeviceTwin_Destroy_Impl` shall set the method callback to `NULL`. **]**

**SRS_SERIALIZERDEVICETWIN_02_017: [** `IoTHubDeviceTwin_Destroy_Impl` shall call `CodeFirst_DestroyDevice`. **]**

**SRS_SERIALIZERDEVICETWIN_02_018: [** `IoTHubDeviceTwin_Destroy_Impl` shall remove the IoTHubClient_Handle and the device handle from the recorded set. **]**

**SRS_SERIALIZERDEVICETWIN_02_019: [** If the recorded set of IoTHubClient handles is zero size, then the set shall be destroyed. **]**

### deviceMethodCallback
```c
static int deviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
```

`deviceMethodCallback` translates the method payload received from IoTHubClient(_LL) to the format required by `EXECUTE_METHOD`. 

`deviceMethodCallback` translates from the result provided by `EXECUTE_METHOD` back into the method response for IoTHubClient(_LL).

**SRS_SERIALIZERDEVICETWIN_02_021: [** `deviceMethodCallback` shall transform `payload` and `size` into a null terminated string. **]**

**SRS_SERIALIZERDEVICETWIN_02_022: [** `deviceMethodCallback` shall call `EXECUTE_METHOD` passing the `userContextCallback`, `method_name` and the null terminated string build before. **]**

**SRS_SERIALIZERDEVICETWIN_02_023: [** `deviceMethodCallback` shall get the `MethodReturn_Data` and shall copy the response JSON value into a new byte array. **]**

**SRS_SERIALIZERDEVICETWIN_02_024: [** `deviceMethodCallback` shall set `*response` to this new byte array, `*resp_size` to the size of the array. **]**

**SRS_SERIALIZERDEVICETWIN_02_025: [** `deviceMethodCallback` returns the statusCode from the user. **]**

**SRS_SERIALIZERDEVICETWIN_02_026: [** If any failure occurs in the above operations, then `deviceMethodCallback` shall fail, return 500, set `*response` to `NULL` and '*resp_size` to 0. **]** 

### IoTHubDeviceTwin_SendReportedState_Impl
```c
static IOTHUB_CLIENT_RESULT IoTHubDeviceTwin_SendReportedState_Impl(void* model, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK deviceTwinCallback, void* context)
```

`IoTHubDeviceTwin_SendReportedState_Impl` send the complete reported state for `model`. 

**SRS_SERIALIZERDEVICETWIN_02_029: [** `IoTHubDeviceTwin_SendReportedState_Impl` shall call `CodeFirst_SendAsyncReported`. **]** (which serializes the complete reported state to a byte buffer).

**SRS_SERIALIZERDEVICETWIN_02_030: [** `IoTHubDeviceTwin_SendReportedState_Impl` shall find `model` in the list of devices. **]**

**SRS_SERIALIZERDEVICETWIN_02_031: [** `IoTHubDeviceTwin_SendReportedState_Impl` shall use IoTHubClient_SendReportedState/IoTHubClient_LL_SendReportedState to send the serialized reported state. **]**

**SRS_SERIALIZERDEVICETWIN_02_032: [** `IoTHubDeviceTwin_SendReportedState_Impl` shall succeed and return `IOTHUB_CLIENT_OK` when all operations complete successfully. **]**

**SRS_SERIALIZERDEVICETWIN_02_033: [** Otherwise, `IoTHubDeviceTwin_SendReportedState_Impl` shall fail and return `IOTHUB_CLIENT_ERROR`. **]**




