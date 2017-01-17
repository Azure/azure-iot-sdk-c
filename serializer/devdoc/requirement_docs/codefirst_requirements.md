# CodeFirst Requirements

Overview
CodeFirst is a module responsible with:
1.	Aggregation of "code first" type of data from data providers
In order to collect data, it shall use the definitions made available in the data providers by the usage of the iotagent.h macros.
 
## Exposed API
**SRS_CODEFIRST_99_001: [**  CodeFirst shall have the following API: **]**

```c
#define CODEFIRST_ENUM_VALUES                  \
CODEFIRST_OK,                                  \
CODEFIRST_INVALID_ARG,                         \
CODEFIRST_ALREADY_INIT,                        \
CODEFIRST_NOT_INIT,                            \
CODEFIRST_ERROR,                               \
CODEFIRST_NOT_ENOUGH_MEMORY,                   \
CODEFIRST_ACTION_NOT_FOUND,                    \
CODEFIRST_ACTION_EXECUTION_ERROR,              \
CODEFIRST_SCHEMA_ERROR,                        \
CODEFIRST_AGENT_DATA_TYPE_ERROR,               \
CODEFIRST_VALUES_FROM_DIFFERENT_DEVICES_ERROR, \
CODEFIRST_DEVICE_FAILED,                       \
CODEFIRST_DEVICE_PUBLISH_FAILED,               \
CODEFIRST_NOT_A_PROPERTY
 
DEFINE_ENUM(CODEFIRST_RESULT, CODEFIRST_ENUM_VALUES)
 
extern CODEFIRST_RESULT CodeFirst_Init(const char* overrideSchemaNamespace);
extern void CodeFirst_Deinit(void);
extern SCHEMA_HANDLE CodeFirst_RegisterSchema(const char* schemaNamespace, const REFLECTED_DATA_FROM_DATAPROVIDER* metadata);
 
extern IOTHUBMESSAGE_DISPOSITION_RESULT CodeFirst_InvokeAction(void* deviceHandle, void* callbackUserContext, const char* relativeActionPath, const char* actionName, size_t parameterCount, const AGENT_DATA_TYPE* parameterValues);
 extern EXECUTE_COMMAND_RESULT CodeFirst_ExecuteCommand(void* device, const char* command);

extern METHODRETURN_HANDLE CodeFirst_InvokeMethod(DEVICE_HANDLE deviceHandle, void* callbackUserContext, const char* relativeMethodPath, const char* methodName, size_t parameterCount, const AGENT_DATA_TYPE* parameterValues);

extern METHODRETURN_HANDLE CodeFirst_ExecuteMethod(void* device, const char* methodName, const char* methodPayload);

extern void* CodeFirst_CreateDevice(SCHEMA_MODEL_TYPE_HANDLE model, const REFLECTED_DATA_FROM_DATAPROVIDER* metadata, size_t dataSize, bool includePropertyPath);
 
extern CODEFIRST_RESULT CodeFirst_SendAsync(unsigned char** destination, size_t* destinationSize, size_t numProperties, ...);
 
extern CODEFIRST_RESULT CodeFirst_IngestDesiredProperties(void* device, const char* desiredProperties);

extern AGENT_DATA_TYPE_TYPE CodeFirst_GetPrimitiveType(const char* typeName);
```

### CodeFirst_Init
```c
CODEFIRST_RESULT CodeFirst_Init(const char* overrideSchemaNamespace);
```

`CodeFirst_Init` is a lazy-init style function called by all the other public APIs of this module. 
By default, the APIs of CodeFirst call this API with `NULL` for `overrideSchemaNamespace`.

**SRS_CODEFIRST_99_002: [**  CodeFirst_Init shall initialize the CodeFirst module. If initialization is successful, it shall return CODEFIRST_OK. **]**

**SRS_CODEFIRST_99_003: [**  If the module is already initialized, the initialization shall fail and the return value shall be CODEFIRST_ALREADY_INIT. **]**

**SRS_CODEFIRST_99_004: [**  If initialization fails for a reason not specifically indicated here, CODEFIRST_ERROR shall be returned. **]**


### CodeFirst_Deinit
```c
void CodeFirst_Deinit(void);
```

`CodeFirst_Deinit` deinitializes CodeFirst.

**SRS_CODEFIRST_99_005: [**  CodeFirst_Deinit shall deinitialize the module, freeing all the resources and placing the module in an uninitialized state. **]**

**SRS_CODEFIRST_99_006: [**  If the module is not previously initialed, CodeFirst_Deinit shall do nothing. **]**


### CodeFirst_RegisterSchema
```c
SCHEMA_HANDLE CodeFirst_RegisterSchema(const char* schemaNamespace, const REFLECTED_DATA_FROM_DATAPROVIDER* metadata);
```

**SRS_CODEFIRST_02_048: [** If `schemaNamespace` is `NULL` then `CodeFirst_RegisterSchema` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_02_049: [** If `metadata` is `NULL` then `CodeFirst_RegisterSchema` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_99_002: [**  CodeFirst_RegisterSchema shall create the schema information and give it to the Schema module for one schema, identified by the metadata argument. On success, it shall return a handle to the schema. **]**

**SRS_CODEFIRST_99_121: [** If the schema has already been registered, CodeFirst_RegisterSchema shall return its handle. **]**

**SRS_CODEFIRST_99_076: [** If any Schema APIs fail, CodeFirst_RegisterSchema shall return NULL. **]**


### CodeFirst_CreateDevice
```c 
extern void* CodeFirst_CreateDevice(SCHEMA_MODEL_TYPE_HANDLE model, const REFLECTED_DATA_FROM_DATAPROVIDER* metadata, size_t dataSize, bool includePropertyPath);
```

`CodeFirst_CreateDevice` creates a device.

**SRS_CODEFIRST_02_037: [** `CodeFirst_CreateDevice` shall call `CodeFirst_Init`, passing `NULL` for `overrideSchemaNamespace`. **]**

**SRS_CODEFIRST_99_079: [** CodeFirst_CreateDevice shall create a device and allocate a memory block that should hold the device data. **]**

**SRS_CODEFIRST_99_101: [** On success, CodeFirst_CreateDevice shall return a non NULL pointer to the device data. **]**

**SRS_CODEFIRST_99_080: [** If CodeFirst_CreateDevice is invoked with a NULL model, it shall return NULL. **]**

**SRS_CODEFIRST_99_081: [** CodeFirst_CreateDevice shall use Device_Create to create a device handle. **]**

**SRS_CODEFIRST_02_036: [** `CodeFirst_CreateDevice` shall initialize all the desired properties to their default values. **]**

**SRS_CODEFIRST_01_001: [** CodeFirst_CreateDevice shall pass the includePropertyPath argument to Device_Create. **]**

**SRS_CODEFIRST_99_082: [** CodeFirst_CreateDevice shall pass to Device_Create the function CodeFirst_InvokeAction, action callback argument and 
the CodeFirst_InvokeMethod **]**

**SRS_CODEFIRST_99_084: [** If Device_Create fails, CodeFirst_CreateDevice shall return NULL. **]**

**SRS_CODEFIRST_99_106: [** If CodeFirst_CreateDevice is called when the modules is not initialized is shall return NULL. **]**

**SRS_CODEFIRST_99_102: [** On any other errors, _CreateDevice shall return NULL. **]**

### CodeFirst_DestroyDevice
```c
extern void CodeFirst_DestroyDevice(void* device);
```
`CodeFirst_DestroyDevice` destroys a device, freeing all used resources.

**SRS_CODEFIRST_99_085: [** CodeFirst_DestroyDevice shall free all resources associated with a device. **]**

**SRS_CODEFIRST_99_086: [** If the argument is NULL, CodeFirst_DestroyDevice shall do nothing. **]**

**SRS_CODEFIRST_99_087: [** In order to release the device handle, CodeFirst_DestroyDevice shall call Device_Destroy. **]**

**SRS_CODEFIRST_02_039: [** If the current device count is zero then `CodeFirst_DestroyDevice` shall deallocate all other used resources. **]**

### CodeFirst_SendAsync
```c 
extern CODEFIRST_RESULT CodeFirst_SendAsync(unsigned char** destination, size_t* destinationSize, size_t numProperties, ...);
```

`CodeFirst_SendAsync` shall send to the Device module a set of properties, a `destination` and a `destinationSize`.

**SRS_CODEFIRST_02_040: [** `CodeFirst_SendAsync` shall call `CodeFirst_Init`, passing `NULL` for `overrideSchemaNamespace`. **]**

**SRS_CODEFIRST_99_117: [** On success, CodeFirst_SendAsync shall return CODEFIRST_OK. **]**

**SRS_CODEFIRST_99_105: [** The properties are passed as pointers to the memory locations where the data exists in the device block allocated by CodeFirst_CreateDevice. **]**

**SRS_CODEFIRST_99_089: [** The numProperties argument shall indicate how many properties are to be sent. **]**

**SRS_CODEFIRST_99_090: [** All the properties shall be sent together by using the transacted APIs of the device. **]**

**SRS_CODEFIRST_99_091: [** CodeFirst_SendAsync shall start a transaction by calling Device_StartTransaction. **]**

**SRS_CODEFIRST_99_092: [** CodeFirst shall publish each value by using Device_PublishTransacted. **]**

**SRS_CODEFIRST_99_093: [** After all values have been published, Device_EndTransaction shall be called. **]**

**SRS_CODEFIRST_99_094: [** If any Device API fail, CodeFirst_SendAsync shall return CODEFIRST_DEVICE_PUBLISH_FAILED. **]**

**SRS_CODEFIRST_99_103: [** If CodeFirst_SendAsync is called with numProperties being zero, CODEFIRST_INVALID_ARG shall be returned. **]**

**SRS_CODEFIRST_99_095: [** For each value passed to it, CodeFirst_SendAsync shall look up to which device the value belongs. **]**

**SRS_CODEFIRST_99_096: [** All values have to belong to the same device, otherwise CodeFirst_SendAsync shall return CODEFIRST_VALUES_FROM_DIFFERENT_DEVICES_ERROR. **]**

**SRS_CODEFIRST_99_104: [** If a property cannot be associated with a device, CodeFirst_SendAsync shall return CODEFIRST_INVALID_ARG. **]**

**SRS_CODEFIRST_99_097: [** For each value marshalling to AGENT_DATA_TYPE shall be performed. **]**

**SRS_CODEFIRST_99_098: [** The marshalling shall be done by calling the Create_AGENT_DATA_TYPE_from_Ptr function associated with the property. **]**

**SRS_CODEFIRST_99_099: [** If Create_AGENT_DATA_TYPE_from_Ptr fails, CodeFirst_SendAsync shall return CODEFIRST_AGENT_DATA_TYPE_ERROR. **]**

**SRS_CODEFIRST_99_122: [**  CodeFirst_SendAsync shall Invoke EndTransaction. **]**

**SRS_CODEFIRST_99_130: [** If a pointer to the beginning of a device block is passed to CodeFirst_SendAsync instead of a pointer to a property, CodeFirst_SendAsync shall send all the properties that belong to that device. **]**

**SRS_CODEFIRST_99_131: [** The properties shall be given to Device as one transaction, as if they were all passed as individual arguments to Code_First. **]**

**SRS_CODEFIRST_99_133: [** CodeFirst_SendAsync shall allow sending of properties that are part of a child model. **]**

Specifically it shall allow sending of ParentModel.ChildModel.InnerProperty for the below example:
ParentModel
	|
	|--   ChildModel
		|----------- int InnerProperty

**SRS_CODEFIRST_99_136: [** CodeFirst_SendAsync shall build the full path for each property and then pass it to Device_PublishTransacted. **]**

For the above example CodeFirst_SendAsync shall pass "ChildModel/InnerProperty" to Device_PublishTransacted.
**SRS_CODEFIRST_04_001: [** CodeFirst_SendAsync shall pass callback to IoTDevice without validating if it's NULL. **]**

**SRS_CODEFIRST_04_002: [** If CodeFirst_SendAsync receives destination or destinationSize NULL, CodeFirst_SendAsync shall return Invalid Argument. **]**


### CodeFirst_InvokeAction
```c 
IOTHUBMESSAGE_DISPOSITION_RESULT CodeFirst_InvokeAction(void* deviceHandle, const char* relativeActionPath, const char* actionName, size_t parameterCount, const AGENT_DATA_TYPE* parameterValues);
```

**SRS_CODEFIRST_99_062: [**  When CodeFirst_InvokeAction is called it shall look through the codefirst metadata associated with a specific device for a previously declared action (function) named actionName. **]**

**SRS_CODEFIRST_99_078: [** If such a function is not found then the function shall return IOTHUBMESSAGE_ABANDONED. **]**

**SRS_CODEFIRST_99_063: [**  If the function is found, then CodeFirst shall call the wrapper of the found function inside the data provider. The wrapper is linked in the reflected data to the function name. The wrapper shall be called with the same arguments as CodeFirst_InvokeAction has been called. **]**

**SRS_CODEFIRST_02_013: [** The wrapper's return value shall be returned. **]**

**SRS_CODEFIRST_99_066: [**  If actionName, relativeActionPath or deviceHandle is NULL then IOTHUBMESSAGE_ABANDONED shall be returned. **]**

**SRS_CODEFIRST_99_067: [**  If parameterCount is greater than zero and parameterValues is NULL then IOTHUBMESSAGE_ABANDONED shall be returned. **]**

**SRS_CODEFIRST_99_068: [**  If the function is called before CodeFirst is initialized then IOTHUBMESSAGE_ABANDONED shall be returned. **]**

**SRS_CODEFIRST_99_138: [** The relativeActionPath argument shall be used by CodeFirst_InvokeAction to find the child model where the action is declared. **]**

An example would be relativeActionPath being passed as "ChildModel" and actionName being passed as "reset":
```code 
ParentModel
	|
	|--   ChildModel
		|----------- void reset()
```
**SRS_CODEFIRST_99_139: [** If the relativeActionPath is empty then the action shall be looked up in the device model. **]**

**SRS_CODEFIRST_99_140: [** CodeFirst_InvokeAction shall pass to the action wrapper that it calls a pointer to the model where the action is defined. **]**

**SRS_CODEFIRST_99_141: [** If a child model specified in the relativeActionPath argument cannot be found by CodeFirst_InvokeAction, it shall return IOTHUBMESSAGE_ABANDONED. **]**

**SRS_CODEFIRST_99_142: [** The relativeActionPath argument shall be in the format "childModel1/childModel2/.../childModelN". **]**


### CodeFirst_ExecuteCommand
```c
extern EXECUTE_COMMAND_RESULT CodeFirst_ExecuteCommand(void* device, const char* command);
```

`CodeFirst_ExecuteCommand` executes a command.

**SRS_CODEFIRST_02_014: [** If parameter device or command is NULL then CodeFirst_ExecuteCommand shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_CODEFIRST_02_015: [** CodeFirst_ExecuteCommand shall find the device. **]**

**SRS_CODEFIRST_02_016: [** If finding the device fails, then CodeFirst_ExecuteCommand shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_CODEFIRST_02_017: [** Otherwise CodeFirst_ExecuteCommand shall call Device_ExecuteCommand and return what Device_ExecuteCommand is returning. **]**

### CodeFirst_SendAsyncReported
```c 
extern CODEFIRST_RESULT CodeFirst_SendAsyncReported(unsigned char** destination, size_t* destinationSize, size_t numProperties, ...);
```

`CodeFirst_SendAsyncReported` starts, publishes and finishes a device transaction.

**SRS_CODEFIRST_02_046: [** `CodeFirst_SendAsyncReported` shall call `CodeFirst_Init`, passing `NULL` for `overrideSchemaNamespace`. **]**

**SRS_CODEFIRST_02_018: [** If parameter `destination`, `destinationSize` or any of the values passed through va_args is `NULL` then `CodeFirst_SendAsyncReported` shall fail and return `CODEFIRST_INVALID_ARG`. **]**

**SRS_CODEFIRST_02_019: [** If values passed through va_args do not belong to the same device then `CodeFirst_SendAsyncReported` shall fail and return `CODEFIRST_VALUES_FROM_DIFFERENT_DEVICES_ERROR`. **]**

**SRS_CODEFIRST_02_020: [** If values passed through va_args are not all of type REFLECTED_REPORTED_PROPERTY then `CodeFirst_SendAsyncReported` shall fail and return `CODEFIRST_INVALID_ARG`. **]**

**SRS_CODEFIRST_02_021: [** If the value passed through va_args is a complete model instance, then `CodeFirst_SendAsyncReported` shall send all the reported properties of that device. **]**

**SRS_CODEFIRST_02_022: [** `CodeFirst_SendAsyncReported` shall start a transaction by calling `Device_CreateTransaction_ReportedProperties`. **]**

**SRS_CODEFIRST_02_023: [** `CodeFirst_SendAsyncReported` shall convert all `REPORTED_PROPERTY` model components to `AGENT_DATA_TYPE`. **]** 

**SRS_CODEFIRST_02_024: [** `CodeFirst_SendAsyncReported` shall call `Device_PublishTransacted_ReportedProperty` for every `AGENT_DATA_TYPE` converted from `REPORTED_PROPERTY`. **]**

**SRS_CODEFIRST_02_025: [** `CodeFirst_SendAsyncReported` shall compute for every `AGENT_DATA_TYPE` the valuePath. **]**

**SRS_CODEFIRST_02_026: [** `CodeFirst_SendAsyncReported` shall call `Device_CommitTransaction_ReportedProperties` to commit the transaction. **]**

**SRS_CODEFIRST_02_029: [** `CodeFirst_SendAsyncReported` shall call `Device_DestroyTransaction_ReportedProperties` to destroy the transaction. **]**

**SRS_CODEFIRST_02_027: [** If any error occurs, `CodeFirst_SendAsyncReported` shall fail and return `CODEFIRST_ERROR`. **]**

**SRS_CODEFIRST_02_028: [** `CodeFirst_SendAsyncReported` shall return `CODEFIRST_OK` when it succeeds. **]**

### CODEFIRST_RESULT CodeFirst_IngestDesiredProperties
```c
extern CODEFIRST_RESULT CodeFirst_IngestDesiredProperties(void* device, const char* desiredProperties);
```

`CodeFirst_IngestDesiredProperties` applies desired properties to a device.

**SRS_CODEFIRST_02_030: [** If argument `device` is `NULL` then `CodeFirst_IngestDesiredProperties` shall fail and return `CODEFIRST_INVALID_ARG`. **]**

**SRS_CODEFIRST_02_031: [** If argument `desiredProperties` is `NULL` then `CodeFirst_IngestDesiredProperties` shall fail and return `CODEFIRST_INVALID_ARG`. **]**

**SRS_CODEFIRST_02_032: [** `CodeFirst_IngestDesiredProperties` shall locate the device associated with `device`. **]**

**SRS_CODEFIRST_02_033: [** `CodeFirst_IngestDesiredProperties` shall call `Device_IngestDesiredProperties`. **]**

**SRS_CODEFIRST_02_034: [** If there is any failure, then `CodeFirst_IngestDesiredProperties` shall fail and return `CODEFIRST_ERROR`. **]**

**SRS_CODEFIRST_02_035: [** Otherwise, `CodeFirst_IngestDesiredProperties` shall return `CODEFIRST_OK`. **]**

### CodeFirst_InvokeMethod
```c
METHODRETURN_HANDLE CodeFirst_InvokeMethod(DEVICE_HANDLE deviceHandle, void* callbackUserContext, const char* relativeMethodPath, const char* methodName, size_t parameterCount, const AGENT_DATA_TYPE* parameterValues)
```

`CodeFirst_InvokeMethod` invokes a method call.

**SRS_CODEFIRST_02_050: [** If CodeFirst was not init before, `CodeFirst_InvokeMethod` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_02_051: [** If `deviceHandle` is `NULL` then `CodeFirst_InvokeMethod` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_02_052: [** If `relativeMethodPath` is `NULL` then `CodeFirst_InvokeMethod` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_02_053: [** If `methodName` is `NULL` then `CodeFirst_InvokeMethod` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_02_054: [** If `parameterCount` is greater than 0 and `parameterValues` is `NULL` then then `CodeFirst_InvokeMethod` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_02_055: [** `CodeFirst_InvokeMethod` shall get the model name of the device. **]**

**SRS_CODEFIRST_02_056: [** `CodeFirst_InvokeMethod` shall locate the model in the codefirst data. **]**

**SRS_CODEFIRST_02_057: [** `CodeFirst_InvokeMethod` shall locate the method in the model data. **]**

**SRS_CODEFIRST_02_058: [** `CodeFirst_InvokeMethod` shall call the methodCallback and return what the methodCallback returns. **]**

**SRS_CODEFIRST_02_059: [** If any of the above fails then `CodeFirst_InvokeMethod` shall fail and return `NULL`. **]**



### CodeFirst_ExecuteMethod
```c
METHODRETURN_HANDLE CodeFirst_ExecuteMethod(void* device, const char* methodName, const char* methodPayload)
```

`CodeFirst_ExecuteMethod` calls `Device_ExecuteMethod`.

**SRS_CODEFIRST_02_060: [** If `device` is `NULL` then `CodeFirst_ExecuteMethod` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_02_061: [** If `methodName` is `NULL` then `CodeFirst_ExecuteMethod` shall fail and return `NULL`. **]**

**SRS_CODEFIRST_02_062: [** `CodeFirst_ExecuteMethod` shall find the device data. **]**

**SRS_CODEFIRST_02_063: [** `CodeFirst_ExecuteMethod` shall call `Device_ExecuteMethod` and return what `Device_ExecuteMethod` returns. **]**

**SRS_CODEFIRST_02_064: [** If any of the above operation fails then `CodeFirst_ExecuteMethod` shall fail and return `NULL`. **]** 