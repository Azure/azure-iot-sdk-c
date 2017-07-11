# CommandDecoder 


## Overview
The CommandDecoder module decodes one command extracting the name and arguments and passes the call to the supplied actionCallback.

## Public API
**SRS_COMMAND_DECODER_99_001: [**  The CommandDecoder module shall expose the following API: **]**

```c
#define COMMANDDECODER_RESULT_VALUES \
    COMMANDDECODER_OK, \
    COMMANDDECODER_ERROR, \
    COMMANDDECODER_INVALID_ARG, \

DEFINE_ENUM(COMMANDDECODER_RESULT, COMMANDDECODER_RESULT_VALUES)
 
#ifdef __cplusplus
extern "C" {
#endif
 
typedef void* COMMAND_DECODER_HANDLE;
typedef int(*ACTION_CALLBACK_FUNC)(void* actionCallbackContext, const char* relativeActionPath, const char* actionName, size_t argCount, const AGENT_DATA_TYPE* args);
typedef METHODRETURN_HANDLE(*METHOD_CALLBACK_FUNC)(void* methodCallbackContext, const char* relativeMethodPath, const char* methodName, size_t argCount, const AGENT_DATA_TYPE* args);

extern COMMAND_DECODER_HANDLE CommandDecoder_Create(SCHEMA_MODEL_TYPE_HANDLE modelHandle, ACTION_CALLBACK_FUNC actionCallback, void* actionCallbackContext, METHOD_CALLBACK_FUNC, methodCallback, void*, methodCallbackContext);


extern EXECUTE_COMMAND_RESULT CommandDecoder_ExecuteCommand(COMMAND_DECODER_HANDLE handle, const char* command);
extern METHODRETURN_HANDLE CommandDecoder_ExecuteMethod(COMMAND_DECODER_HANDLE handle, const char* fullMethodName, const char* methodPayload);

extern void CommandDecoder_Destroy(COMMAND_DECODER_HANDLE commandDecoderHandle);
 
extern EXECUTE_COMMAND_RESULT CommandDecoder_IngestDesiredProperties( void* startAddress, COMMAND_DECODER_HANDLE handle, const char* desiredProperties);

#ifdef __cplusplus
}
#endif
```

### CommandDecoder_Create
```c
extern COMMAND_DECODER_HANDLE CommandDecoder_Create(SCHEMA_MODEL_TYPE_HANDLE modelHandle, ACTION_CALLBACK_FUNC actionCallback, void* actionCallbackContext);
```

**SRS_COMMAND_DECODER_01_001: [** CommandDecoder_Create shall create a new instance of a CommandDecoder. **]**

**SRS_COMMAND_DECODER_01_003: [** If `modelHandle` is NULL, CommandDecoder_Create shall return NULL. **]**

**SRS_COMMAND_DECODER_01_004: [** If any error is encountered during CommandDecoder_Create CommandDecoder_Create shall return NULL. **]**


### CommandDecoder_Destroy
```c
extern void CommandDecoder_Destroy(COMMAND_DECODER_HANDLE commandDecoderHandle);
```

**SRS_COMMAND_DECODER_01_005: [** CommandDecoder_Destroy shall free all resources associated with the commandDecoderHandle instance. **]**

**SRS_COMMAND_DECODER_01_007: [** If CommandDecoder_Destroy is called with a NULL handle, CommandDecoder_Destroy shall do nothing. **]**


### CommandDecoder_ExecuteCommand
```c
extern EXECUTE_COMMAND_RESULT CommandDecoder_ExecuteCommand(COMMAND_DECODER_HANDLE handle, const char* command);
```

**SRS_COMMAND_DECODER_01_009: [** Whenever CommandDecoder_ExecuteCommand is the command shall be decoded and further dispatched to the actionCallback passed in CommandDecoder_Create. **]**

**SRS_COMMAND_DECODER_01_010: [** If either parameter is NULL, the processing shall stop and the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_01_011: [** If the size of the command is 0 then the processing shall stop and the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_01_012: [** CommandDecoder shall decode the command JSON contained in buffer to a multi-tree by using JSONDecoder_JSON_To_MultiTree. **]**

**SRS_COMMAND_DECODER_01_013: [** If parsing the JSON to a multi tree fails, the processing shall stop and the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_01_014: [** CommandDecoder shall use the MultiTree APIs to extract a specific element from the command JSON. **]**

**SRS_COMMAND_DECODER_01_015: [** If any MultiTree API call fails then the processing shall stop and the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_01_016: [** CommandDecoder shall ensure that the multi-tree resulting from JSONDecoder_JSON_To_MultiTree is freed after the commands are executed. **]**

**SRS_COMMAND_DECODER_99_005: [**  If an action is decoded successfully then the callback actionCallback shall be called, passing to it the callback action context, decoded name and arguments. **]**

**SRS_COMMAND_DECODER_99_006: [**  The action name shall be decoded from the element "Name" of the command JSON. **]**

**SRS_COMMAND_DECODER_99_009: [**  CommandDecoder shall call Schema_GetModelActionByName to obtain the information about a specific action. **]**

**SRS_COMMAND_DECODER_99_022: [**  CommandDecoder shall use the Schema APIs to obtain the information about the entity set name and namespace **]**

**SRS_COMMAND_DECODER_99_010: [**  If any Schema API fails then the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_99_011: [**  CommandDecoder shall attempt to extract the command arguments from the command JSON by looking them up under the node "Parameters". **]**

**SRS_COMMAND_DECODER_01_008: [** Each argument shall be looked up as a field, member of the "Parameters" node. **]**


The below example shows the argument State having the value true. In this case State has the type Edm.Boolean.
```json
{
  "Name" : "SetACState",
  "Parameters" : 
  {
      "State" : "true"  
  }
}
```

**SRS_COMMAND_DECODER_99_012: [**  If any argument is missing in the command text then the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_99_021: [**  If the parsing of the command fails for any other reason the command shall not be dispatched. **]**

**SRS_COMMAND_DECODER_99_027: [**  The value for an argument of primitive type shall be decoded by using the CreateAgentDataType_From_String API. **]**

**SRS_COMMAND_DECODER_99_028: [**  If decoding the argument fails then the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_99_029: [**  If the argument type is complex then a complex type value shall be built from the child nodes. **]**

**SRS_COMMAND_DECODER_99_030: [**  For each child node a value shall be built by using AgentTypeSystem APIs. **]**

**SRS_COMMAND_DECODER_99_031: [**  The complex type value that aggregates the children shall be built by using the Create_AGENT_DATA_TYPE_from_Members. **]**

**SRS_COMMAND_DECODER_99_032: [**  Nesting shall be supported for complex type. **]**

**SRS_COMMAND_DECODER_99_033: [**  In order to determine which are the members of a complex types, Schema APIs for structure types shall be used. **]**

**SRS_COMMAND_DECODER_99_034: [**  If Schema APIs indicate that a complex type has 0 members then the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_99_035: [**  CommandDecoder_ExecuteCommand shall support paths to actions that are in child models (i.e. ChildModel/SomeAction. **]**

**SRS_COMMAND_DECODER_99_036: [**  If a child model cannot be found by using Schema APIs then the command shall not be dispatched and it shall return EXECUTE_COMMAND_ERROR. **]**

**SRS_COMMAND_DECODER_99_037: [**  The relative path passed to the actionCallback shall be in the format "childModel1/childModel2/.../childModelN". **]**

Miscellaneous
**SRS_COMMAND_DECODER_99_019: [**  For all exposed APIs argument validity checks shall precede other checks. **]**

If any parameter is NULL then CommandDecoder_ExecuteCommand shall return EXECUTE_COMMAND_ERROR.

### CommandDecoder_IngestDesiredProperties
```c
extern EXECUTE_COMMAND_RESULT CommandDecoder_IngestDesiredProperties( void* startAddress, COMMAND_DECODER_HANDLE handle, const char* jsonPayload, bool removedDesiredNode);
```

`CommandDecoder_IngestDesiredProperties` applies `jsonPayload` to the device at `startAddress` in memory. It is not transactional (so far).

**SRS_COMMAND_DECODER_02_001: [** If `startAddress` is NULL then `CommandDecoder_IngestDesiredProperties` shall fail and return `EXECUTE_COMMAND_ERROR`. **]**

**SRS_COMMAND_DECODER_02_002: [** If `handle` is NULL then `CommandDecoder_IngestDesiredProperties` shall fail and return `EXECUTE_COMMAND_ERROR`. **]**

**SRS_COMMAND_DECODER_02_003: [** If `jsonPayload` is NULL then `CommandDecoder_IngestDesiredProperties` shall fail and return `EXECUTE_COMMAND_ERROR`. **]**

**SRS_COMMAND_DECODER_02_004: [** `CommandDecoder_IngestDesiredProperties` shall clone `jsonPayload`. **]**

**SRS_COMMAND_DECODER_02_005: [** `CommandDecoder_IngestDesiredProperties` shall create a MULTITREE_HANDLE ouf of the clone of `jsonPayload`. **]**

**SRS_COMMAND_DECODER_02_014: [** If removedDesiredNode is TRUE, parse only the `desired` part of JSON tree **]**

**SRS_COMMAND_DECODER_02_015: [** Remove '$version' string from node, if it is present.  It not being present is not an error **]**

**SRS_COMMAND_DECODER_02_006: [** `CommandDecoder_IngestDesiredProperties` shall parse the MULTITREEE recursively. **]**

**SRS_COMMAND_DECODER_02_007: [** If the child name corresponds to a desired property then an AGENT_DATA_TYPE shall be constructed from the MULTITREE node. **]**

**SRS_COMMAND_DECODER_02_008: [** The desired property shall be constructed in memory by calling pfDesiredPropertyFromAGENT_DATA_TYPE. **]**

**SRS_COMMAND_DECODER_02_013: [** If the desired property has a non-`NULL` `pfOnDesiredProperty` then it shall be called. **]**

**SRS_COMMAND_DECODER_02_009: [** If the child name corresponds to a model in model then the function shall call itself recursively. **]**

**SRS_COMMAND_DECODER_02_012: [** If the child model in model has a non-`NULL` `pfOnDesiredProperty` then `pfOnDesiredProperty` shall be called. **]** 

**SRS_COMMAND_DECODER_02_010: [** If the complete MULTITREE has been parsed then `CommandDecoder_IngestDesiredProperties` shall succeed and return `EXECUTE_COMMAND_SUCCESS`. **]**

**SRS_COMMAND_DECODER_02_011: [** Otherwise `CommandDecoder_IngestDesiredProperties` shall fail and return `EXECUTE_COMMAND_FAILED`. **]**

### CommandDecoder_ExecuteMethod
```c 
METHODRETURN_HANDLE CommandDecoder_ExecuteMethod(COMMAND_DECODER_HANDLE handle, const char* fullMethodName, const char* methodPayload)
```

`CommandDecoder_ExecuteMethod` calls the method callback passing the decoded arguments from `fullMethodPayload`. A `fullMethodName` includes
several segments as in `model1/model2/model3/methodName`. A segment is a model_in_model name.

**SRS_COMMAND_DECODER_02_014: [** If `handle` is `NULL` then `CommandDecoder_ExecuteMethod` shall fail and return `NULL`. **]**

**SRS_COMMAND_DECODER_02_015: [** If `fullMethodName` is `NULL` then `CommandDecoder_ExecuteMethod` shall fail and return `NULL`. **]**

**SRS_COMMAND_DECODER_02_025: [** If `methodCallback` is `NULL` then `CommandDecoder_ExecuteMethod` shall fail and return `NULL`. **]** 

**SRS_COMMAND_DECODER_02_016: [** If `methodPayload` is not `NULL` then `CommandDecoder_ExecuteMethod` shall build a `MULTITREE_HANDLE` out of `methodPayload`. **]**

**SRS_COMMAND_DECODER_02_017: [** `CommandDecoder_ExecuteMethod` shall get the `SCHEMA_HANDLE` associated with the modelHandle passed at `CommandDecoder_Create`. **]**

**SRS_COMMAND_DECODER_02_018: [** `CommandDecoder_ExecuteMethod` shall validate that consecutive segments of the `fullMethodName` exist in the model. **]**

**SRS_COMMAND_DECODER_02_019: [** `CommandDecoder_ExecuteMethod` shall locate the final model to which the `methodName` applies. **]**

**SRS_COMMAND_DECODER_02_020: [** `CommandDecoder_ExecuteMethod` shall verify that the model has a method called `methodName`. **]**

**SRS_COMMAND_DECODER_02_021: [** For every argument of `methodName`, `CommandDecoder_ExecuteMethod` shall build an `AGENT_DATA_TYPE` from the node with the same name from the `MULTITREE_HANDLE`. **]**  

**SRS_COMMAND_DECODER_02_022: [** `CommandDecoder_ExecuteMethod` shall call `methodCallback` passing the context, the `methodName`, number of arguments and the `AGENT_DATA_TYPE`. **]**

**SRS_COMMAND_DECODER_02_023: [** If any of the previous operations fail, then `CommandDecoder_ExecuteMethod` shall return `NULL`. **]**

**SRS_COMMAND_DECODER_02_024: [** Otherwise, `CommandDecoder_ExecuteMethod` shall return what `methodCallback` returns. **]**