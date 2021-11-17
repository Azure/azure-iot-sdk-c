// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#if defined _MSC_VER
#pragma warning(disable: 4054) /* MSC incorrectly fires this */
#endif

static void* my_gballoc_malloc(size_t t)
{
    return malloc(t);
}

static void* my_gballoc_calloc(size_t m, size_t t)
{
    return calloc(m, t);
}

static void my_gballoc_free(void * t)
{
    free(t);
}

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"


#define ENABLE_MOCKS
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "multitree.h"
#include "schema.h"
#include "agenttypesystem.h"
MOCKABLE_FUNCTION(, void, onDesiredPropertySimpleProperty, void*, v);
MOCKABLE_FUNCTION(, void, onDesiredPropertyModelInModel, void*, v);
#undef ENABLE_MOCKS

#include "commanddecoder.h"

#define ENABLE_MOCKS
#include "codefirst.h"
#include "jsondecoder.h"

MOCKABLE_FUNCTION(, EXECUTE_COMMAND_RESULT, ActionCallbackMock, void*, actionCallbackContext, const char*, relativeActionPath, const char*, actionName, size_t, parameterCount, const AGENT_DATA_TYPE*, parameterValues);
MOCKABLE_FUNCTION(, METHODRETURN_HANDLE, methodCallbackMock, void*, methodCallbackContext, const char*, relativeMethodPath, const char*, mthodName, size_t, parameterCount, const AGENT_DATA_TYPE*, parameterValues);
MOCKABLE_FUNCTION(, int, int_pfDesiredPropertyFromAGENT_DATA_TYPE, const AGENT_DATA_TYPE*, source, void*, dest);
#undef ENABLE_MOCKS

#include "azure_c_shared_utility/lock.h"
#include "testrunnerswitcher.h"

static const SCHEMA_MODEL_TYPE_HANDLE TEST_CHILD_MODEL_HANDLE = (SCHEMA_MODEL_TYPE_HANDLE)0x4302;
static const MULTITREE_HANDLE TEST_NESTED_STRUCT_NODE = (MULTITREE_HANDLE)0x4283;
static const SCHEMA_PROPERTY_HANDLE memberNestedComplexTypeProperty = (SCHEMA_PROPERTY_HANDLE)0x4403;
static char lastMemberNames[100][100][100];
static AGENT_DATA_TYPE LatAgentDataType;
static AGENT_DATA_TYPE LongAgentDataType;
static const MULTITREE_HANDLE TEST_MEMBER1_NODE = (MULTITREE_HANDLE)0x4401;
static const MULTITREE_HANDLE TEST_MEMBER2_NODE = (MULTITREE_HANDLE)0x4402;
static const SCHEMA_PROPERTY_HANDLE memberProperty2 = (SCHEMA_PROPERTY_HANDLE)0x4402;
static const SCHEMA_PROPERTY_HANDLE memberProperty1 = (SCHEMA_PROPERTY_HANDLE)0x4401;
static const char OtherArgValue[] = "SomeString";
static const SCHEMA_STRUCT_TYPE_HANDLE TEST_STRUCT_1_HANDLE = (SCHEMA_STRUCT_TYPE_HANDLE)0x4301;
static const SCHEMA_STRUCT_TYPE_HANDLE TEST_STRUCT_2_HANDLE = (SCHEMA_STRUCT_TYPE_HANDLE)0x4302;
static const char LocationActionArgument_Name[] = "Location";
static const char LocationActionArgument_Type[] = "GeoLocation";
static const SCHEMA_ACTION_ARGUMENT_HANDLE LocationActionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE)0x5254;
static const SCHEMA_ACTION_HANDLE SetLocationActionHandle = (SCHEMA_ACTION_HANDLE)0x4242;
static const char* quotedSetLocationName = "\"SetLocation\"";
static const char* setLocationName = "SetLocation";
static AGENT_DATA_TYPE OtherArgAgentDataType;
static const MULTITREE_HANDLE TEST_ARG2_NODE = (MULTITREE_HANDLE)0x4282;
static const char OtherArgActionArgument_Name[] = "OtherArg";
static const char OtherArgActionArgument_Type[] = "ascii_char_ptr";
static const SCHEMA_ACTION_ARGUMENT_HANDLE OtherArgActionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE)0x5253;
static const MULTITREE_HANDLE TEST_ARG1_NODE = (MULTITREE_HANDLE)0x4281;
static const SCHEMA_ACTION_ARGUMENT_HANDLE StateActionArgument = (SCHEMA_ACTION_ARGUMENT_HANDLE)0x5252;
static const char StateActionArgument_Type[] = "bool";
static const char StateActionArgument_Name[] = "State";
static const SCHEMA_ACTION_HANDLE SetACStateActionHandle = (SCHEMA_ACTION_HANDLE)0x4242;
static const char* setACStateName = "SetACState";
static const MULTITREE_HANDLE TEST_COMMAND_ARGS_NODE = (MULTITREE_HANDLE)0x4202;
static const MULTITREE_HANDLE TEST_COMMANDS_ROOT_NODE = (MULTITREE_HANDLE)0x4201;
static const char* quotedSetACStateName = "\"SetACState\"";
static const MULTITREE_HANDLE TEST_COMMAND_NAME_NODE = (MULTITREE_HANDLE)0x4202;
static const MULTITREE_HANDLE TEST_COMMAND_ROOT_NODE = (MULTITREE_HANDLE)0x4201;
static const SCHEMA_HANDLE TEST_SCHEMA_HANDLE = (SCHEMA_HANDLE)0x4401;

#define TEST_COMMAND \
    "{ "  \
    "\"Name\" : \"hagauaga\"," \
    "\"Parameters\":" \
    "{ "  \
    "  \"param1\" : \"42\"," \
    "} "  \
    " }"

static char TestCommand[] = TEST_COMMAND;

TEST_DEFINE_ENUM_TYPE(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(JSON_DECODER_RESULT, JSON_DECODER_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(JSON_DECODER_RESULT, JSON_DECODER_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(MULTITREE_RESULT, MULTITREE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MULTITREE_RESULT, MULTITREE_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(SCHEMA_RESULT, SCHEMA_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SCHEMA_RESULT, SCHEMA_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(AGENT_DATA_TYPE_TYPE, AGENT_DATA_TYPE_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(AGENT_DATA_TYPE_TYPE, AGENT_DATA_TYPE_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(AGENT_DATA_TYPES_RESULT, AGENT_DATA_TYPES_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(AGENT_DATA_TYPES_RESULT, AGENT_DATA_TYPES_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(SCHEMA_ELEMENT_TYPE, SCHEMA_ELEMENT_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SCHEMA_ELEMENT_TYPE, SCHEMA_ELEMENT_TYPE_VALUES);

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(SCHEMA_ELEMENT_TYPE, SCHEMA_ELEMENT_TYPE_VALUES);

#define TEST_STRING_HANDLE_CHILD_NAME (STRING_HANDLE) 0x3
#define TEST_DESIRED_PROPERTY_HANDLE_INT_FIELD (SCHEMA_DESIRED_PROPERTY_HANDLE)0x4
#define TEST_SCHEMA (SCHEMA_HANDLE)0x5
#define SCHEMA_MODEL_TYPE_HANDLE_MODEL_IN_MODEL (SCHEMA_MODEL_TYPE_HANDLE)0x6

static const SCHEMA_MODEL_TYPE_HANDLE TEST_MODEL_HANDLE = (SCHEMA_MODEL_TYPE_HANDLE)0x4301;
static void* TEST_CALLBACK_CONTEXT_VALUE = (void*)0x4242;
static bool isIoTHubMessage_GetData_writing_to_outputs = true;
static size_t nCall = 0;
static AGENT_DATA_TYPE StateAgentDataType;

static SCHEMA_METHOD_HANDLE TEST_MODEL_METHOD_HANDLE = (SCHEMA_METHOD_HANDLE)0x56;
static SCHEMA_METHOD_ARGUMENT_HANDLE TEST_METHOD_ARGUMENT_HANDLE_0 = (SCHEMA_METHOD_ARGUMENT_HANDLE)0x57;
static SCHEMA_METHOD_ARGUMENT_HANDLE TEST_METHOD_ARGUMENT_HANDLE_1 = (SCHEMA_METHOD_ARGUMENT_HANDLE)0x58;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES);

static void SetupCommand(const char* quotedActionName, const char* actionName)
{
    STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
    STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
    STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
    STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*quotedActionName*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Parameters", IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(3, &TEST_COMMAND_ARGS_NODE, sizeof(TEST_COMMAND_ARGS_NODE));
    STRICT_EXPECTED_CALL(Schema_GetModelActionByName(TEST_MODEL_HANDLE, actionName))
        .SetReturn(SetACStateActionHandle);
}

void SetupArgumentCalls(SCHEMA_ACTION_HANDLE actionHandle, size_t index, SCHEMA_ACTION_ARGUMENT_HANDLE argHandle, const char* argName, const char* argType)
{

    STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(actionHandle, index))
        .SetReturn(argHandle);
    STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(argHandle))
        .SetReturn(argName);
    STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(argHandle))
        .SetReturn(argType);
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

static JSON_DECODER_RESULT my_JSONDecoder_JSON_To_MultiTree(char* json, MULTITREE_HANDLE* multiTreeHandle)
{
    (void)json;
    *multiTreeHandle = TEST_COMMANDS_ROOT_NODE;
    return JSON_DECODER_OK;
}

static void my_MultiTree_Destroy(MULTITREE_HANDLE treeHandle)
{
    (void)(treeHandle);
    //sigh
}

static AGENT_DATA_TYPES_RESULT my_Create_AGENT_DATA_TYPE_from_Members(AGENT_DATA_TYPE* agentData, const char* typeName, size_t nMembers, const char* const * memberNames, const AGENT_DATA_TYPE* memberValues)
{
    (void)memberValues;
    (void)typeName;
    (void)agentData;
    for (size_t i = 0; i < nMembers; i++)
    {
        strcpy(lastMemberNames[nCall][i], memberNames[i]);
    }
    nCall++;
    return AGENT_DATA_TYPES_OK;
}

static SCHEMA_MODEL_ELEMENT Schema_GetModelElementByName_notFound;
static SCHEMA_MODEL_ELEMENT Schema_GetModelElementByName_desiredProperty_int_field;

static SCHEMA_MODEL_ELEMENT Schema_GetModelElementByName_modelInModel;


char* umockvalue_stringify_SCHEMA_MODEL_ELEMENT(const SCHEMA_MODEL_ELEMENT* value)
{
    char* result;
    size_t needed = snprintf(NULL, 0, "{.elementType=%s, .elementHandle=%p}", MU_ENUM_TO_STRING(SCHEMA_ELEMENT_TYPE, value->elementType), value->elementHandle.modelHandle);
    result = (char*)malloc(needed + 1);
    if (result == NULL)
    {
        ASSERT_FAIL("unable to malloc");
    }
    else
    {
        (void) snprintf(result, needed+1, "{.elementType=%s, .elementHandle=%p}", MU_ENUM_TO_STRING(SCHEMA_ELEMENT_TYPE, value->elementType), value->elementHandle.modelHandle);
    }
    return result;
}

int umockvalue_are_equal_SCHEMA_MODEL_ELEMENT(const SCHEMA_MODEL_ELEMENT* left, const SCHEMA_MODEL_ELEMENT* right)
{
    int result;
    if (left == NULL)
    {
        if (right == NULL)
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        if (right == NULL)
        {
            result = 0;
        }
        else
        {
            result = (memcmp(left , right, sizeof(*left)) == 0);
        }
    }
    return result;
}

int umockvalue_copy_SCHEMA_MODEL_ELEMENT(SCHEMA_MODEL_ELEMENT* destination, const SCHEMA_MODEL_ELEMENT* source)
{
    (void)memcpy(destination, source, sizeof(*destination));
    return 0;
}

void umockvalue_free_SCHEMA_MODEL_ELEMENT(SCHEMA_MODEL_ELEMENT* value)
{
    (void)(value);
    //my_gballoc_free(value);
}

static METHODRETURN_HANDLE g_methodReturnValue = (METHODRETURN_HANDLE)0x3;

BEGIN_TEST_SUITE(CommandDecoder_ut)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        Schema_GetModelElementByName_notFound.elementType = SCHEMA_NOT_FOUND;

        Schema_GetModelElementByName_desiredProperty_int_field.elementType = SCHEMA_DESIRED_PROPERTY;
        Schema_GetModelElementByName_desiredProperty_int_field.elementHandle.desiredPropertyHandle = TEST_DESIRED_PROPERTY_HANDLE_INT_FIELD;
        Schema_GetModelElementByName_modelInModel.elementType = SCHEMA_MODEL_IN_MODEL;
        Schema_GetModelElementByName_modelInModel.elementHandle.modelHandle = SCHEMA_MODEL_TYPE_HANDLE_MODEL_IN_MODEL;

        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);

        (void)umocktypes_bool_register_types();
        (void)umocktypes_charptr_register_types();
        (void)umocktypes_stdint_register_types();

        REGISTER_GLOBAL_MOCK_RETURN(ActionCallbackMock, EXECUTE_COMMAND_SUCCESS);
        REGISTER_GLOBAL_MOCK_RETURN(methodCallbackMock, g_methodReturnValue);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(methodCallbackMock, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_calloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_MODEL_TYPE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MULTITREE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_ACTION_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_ACTION_ARGUMENT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_STRUCT_TYPE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_PROPERTY_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_DESIRED_PROPERTY_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(pfDesiredPropertyFromAGENT_DATA_TYPE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(pfOnDesiredProperty, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_METHOD_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_METHOD_ARGUMENT_HANDLE, void*);


        REGISTER_UMOCK_ALIAS_TYPE(JSON_DECODER_RESULT, int);
        REGISTER_UMOCK_ALIAS_TYPE(MULTITREE_RESULT, int);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_RESULT, int);
        REGISTER_UMOCK_ALIAS_TYPE(AGENT_DATA_TYPE_TYPE, int);
        REGISTER_UMOCK_ALIAS_TYPE(AGENT_DATA_TYPES_RESULT, int);
        REGISTER_UMOCK_ALIAS_TYPE(EXECUTE_COMMAND_RESULT, int);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_ELEMENT_TYPE, int);

        REGISTER_UMOCK_VALUE_TYPE(SCHEMA_MODEL_ELEMENT,
            umockvalue_stringify_SCHEMA_MODEL_ELEMENT,
            umockvalue_are_equal_SCHEMA_MODEL_ELEMENT,
            umockvalue_copy_SCHEMA_MODEL_ELEMENT,
            umockvalue_free_SCHEMA_MODEL_ELEMENT
        );

        REGISTER_GLOBAL_MOCK_HOOK(JSONDecoder_JSON_To_MultiTree, my_JSONDecoder_JSON_To_MultiTree);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(JSONDecoder_JSON_To_MultiTree, JSON_DECODER_ERROR);
        REGISTER_GLOBAL_MOCK_HOOK(MultiTree_Destroy, my_MultiTree_Destroy);

        REGISTER_GLOBAL_MOCK_HOOK(Create_AGENT_DATA_TYPE_from_Members, my_Create_AGENT_DATA_TYPE_from_Members);

        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetSchemaForModelType, TEST_SCHEMA_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetSchemaForModelType, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(MultiTree_GetChildByName, MULTITREE_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(MultiTree_GetChildByName, MULTITREE_ERROR);
        REGISTER_GLOBAL_MOCK_RETURN(MultiTree_GetValue, MULTITREE_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(MultiTree_GetValue, MULTITREE_ERROR);
        REGISTER_GLOBAL_MOCK_RETURN(MultiTree_DeleteChild, MULTITREE_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(MultiTree_DeleteChild, MULTITREE_ERROR);
        REGISTER_GLOBAL_MOCK_RETURN(MultiTree_GetChild, MULTITREE_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(MultiTree_GetChild, MULTITREE_ERROR);
        REGISTER_GLOBAL_MOCK_RETURN(MultiTree_GetName, MULTITREE_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(MultiTree_GetName, MULTITREE_ERROR);

        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetModelModelByName, TEST_CHILD_MODEL_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelModelByName, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetModelDesiredProperty_pfOnDesiredProperty, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetModelModelByName_OnDesiredProperty, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetModelMethodByName, TEST_MODEL_METHOD_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelMethodArgumentByIndex, NULL);

        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetMethodArgumentName, NULL);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetMethodArgumentType, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetModelActionArgumentCount, SCHEMA_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelActionArgumentCount, SCHEMA_ERROR);

        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelMethodByName, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetModelMethodArgumentCount, SCHEMA_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelMethodArgumentCount, SCHEMA_ERROR);

        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetStructTypePropertyCount, SCHEMA_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetStructTypePropertyCount, SCHEMA_ERROR);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);

        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

        REGISTER_GLOBAL_MOCK_FAIL_RETURN(MultiTree_GetChild, MULTITREE_ERROR);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(MultiTree_GetName, MULTITREE_ERROR);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelElementByName, Schema_GetModelElementByName_notFound);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelDesiredPropertyByName, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(CreateAgentDataType_From_String, AGENT_DATA_TYPES_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(CreateAgentDataType_From_String, AGENT_DATA_TYPES_ERROR);

        REGISTER_GLOBAL_MOCK_FAIL_RETURN(int_pfDesiredPropertyFromAGENT_DATA_TYPE, MU_FAILURE);


    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        umock_c_reset_all_calls();

        nCall = 0;
        memset(lastMemberNames, 0, sizeof(lastMemberNames));

        StateAgentDataType.type = EDM_BOOLEAN_TYPE;
        StateAgentDataType.value.edmBoolean.value = EDM_TRUE;

        StateAgentDataType.type = EDM_STRING_TYPE;
        StateAgentDataType.value.edmString.length = COUNT_OF(OtherArgValue);
        StateAgentDataType.value.edmString.chars = (char*)OtherArgValue;

        isIoTHubMessage_GetData_writing_to_outputs = true;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    /* CommandDecoder_Create */

    TEST_FUNCTION(CommandDecoder_Create_With_NULL_Model_Handle_Fails)
    {
        // arrange
        umock_c_reset_all_calls();

        // act
        COMMAND_DECODER_HANDLE result = CommandDecoder_Create(NULL, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_CommandDecoder_Create shall give its receive callback to the transport by calling IoTHubClient_SetMessageCallback when transportType is TRANSPORT_THREADED.*/
    TEST_FUNCTION(CommandDecoder_Create_With_Non_NULL_Callback_Argument_Succeeds)
    {
        // arrange
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        // act
        COMMAND_DECODER_HANDLE result = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);

        // assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(result);
    }

    /* Tests_CommandDecoder_Create shall give its receive callback to the transport by calling IoTHubClient_SetMessageCallback when transportType is TRANSPORT_THREADED.*/
    TEST_FUNCTION(CommandDecoder_Create_With_NULL_Callback_does_not_call_StartReceive)
    {
        // arrange
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        // act
        COMMAND_DECODER_HANDLE result = CommandDecoder_Create(TEST_MODEL_HANDLE, NULL, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);

        // assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(result);
    }

    TEST_FUNCTION(CommandDecoder_Create_TRANSPORT_TYPE_LL_succeeds)
    {
        // arrange
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        // act
        COMMAND_DECODER_HANDLE result = CommandDecoder_Create(TEST_MODEL_HANDLE, NULL, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);

        // assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(result);
    }

    TEST_FUNCTION(When_SetReceiveCallback_Fails_Then_gballoc_Fails)
    {
        // arrange
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetReturn(NULL);

        // act
        COMMAND_DECODER_HANDLE result = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* CommandDecoder_Destroy */

    TEST_FUNCTION(CommandDecoder_Destroy_With_NULL_Handle_Does_Nothing)
    {
        // arrange
        umock_c_reset_all_calls();

        // act
        CommandDecoder_Destroy(NULL);

        // assert
        // no explicit asserts
    }

    TEST_FUNCTION(CommandDecoder_Destroy_succeeds)
    {
        ///arrange

        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        // arrange

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        // act
        CommandDecoder_Destroy(commandDecoderHandle);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_with_NULL_messageHandle_fails)
    {
        /// arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /// act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(NULL, "some command");

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        /// cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_with_NULL_context_fails)
    {
        /// arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /// act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, NULL);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        /// cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_with_zeroSize_message_fails)
    {
        /// arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /// act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, "");

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        /// cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_copying_the_command_content_fails_returns_EXECUTE_COMMAND_ERROR)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)) /*this creates a copy of the command that is given to JSON decoder*/
            .SetReturn(NULL);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_Parsing_The_JSON_To_MultiTree_Fails_Then_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2)
            .SetReturn(JSON_DECODER_INVALID_ARG);

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_Getting_The_Schema_For_The_Model_Fails_Then_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE))
            .SetReturn((SCHEMA_HANDLE)NULL);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_Getting_The_ActionName_Node_Fails_Then_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(IGNORED_PTR_ARG, "Name", IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle()
            .IgnoreArgument_childHandle()
            .SetReturn(MULTITREE_INVALID_ARG);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_Getting_The_ActionName_Fails_Then_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedSetACStateName, sizeof(quotedSetACStateName))
            .SetReturn(MULTITREE_INVALID_ARG);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_Getting_The_Parameters_Node_Fails_Then_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedSetACStateName, sizeof(quotedSetACStateName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Parameters", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_ARGS_NODE, sizeof(TEST_COMMAND_ARGS_NODE))
            .SetReturn(MULTITREE_INVALID_ARG);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_Copying_The_relativepath_fails_EXECUTE_COMMAND_ERROR_is_returned)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedSetACStateName, sizeof(quotedSetACStateName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/
            .IgnoreArgument(1)
            .SetReturn(NULL);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_GetModelActionByName_Fails_Then_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedSetACStateName, sizeof(quotedSetACStateName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Parameters", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_ARGS_NODE, sizeof(TEST_COMMAND_ARGS_NODE));
        STRICT_EXPECTED_CALL(Schema_GetModelActionByName(TEST_MODEL_HANDLE, setACStateName))
            .SetReturn((SCHEMA_ACTION_HANDLE)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_Getting_The_ArgCount_Fails_Then_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        const char* quotedActionName = "\"SetACState\"";
        const char* actionName = "SetACState";

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Parameters", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_ARGS_NODE, sizeof(TEST_COMMAND_ARGS_NODE));
        STRICT_EXPECTED_CALL(Schema_GetModelActionByName(TEST_MODEL_HANDLE, actionName))
            .SetReturn(SetACStateActionHandle);
        size_t argCount = 0;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount))
            .SetReturn(SCHEMA_ERROR);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act

        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_The_actionname_Contains_Only_One_Quote_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        const char* quotedActionName = "\"";

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(When_The_actionname_Contains_Only_2_Quotes_No_Command_Is_Dispatched)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        const char* quotedActionName = "\"\"";

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_fails_when_array_of_args_fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        SetupCommand(quotedSetACStateName, setACStateName);

        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments()
            .SetReturn(NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_With_Valid_Command_With_1_Arg_Decodes_The_Argument_And_Calls_The_ActionCallback)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        SetupCommand(quotedSetACStateName, setACStateName);
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();
        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType));
        STRICT_EXPECTED_CALL(ActionCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "", "SetACState", 1, IGNORED_PTR_ARG))
            .IgnoreArgument(5);
        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_GetModelActionArgumentByIndex_Fails_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        SetupCommand(quotedSetACStateName, setACStateName);
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(SetACStateActionHandle, 0))
            .SetReturn((SCHEMA_ACTION_ARGUMENT_HANDLE)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_GetActionArgumentName_Fails_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        SetupCommand(quotedSetACStateName, setACStateName);
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(SetACStateActionHandle, 0))
            .SetReturn(StateActionArgument);
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(StateActionArgument))
            .SetReturn((const char*)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_GetActionArgumentType_Fails_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        SetupCommand(quotedSetACStateName, setACStateName);
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(SetACStateActionHandle, 0))
            .SetReturn(StateActionArgument);
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(StateActionArgument))
            .SetReturn(StateActionArgument_Name);
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(StateActionArgument))
            .SetReturn((const char*)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Argument_Node_Fails_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        SetupCommand(quotedSetACStateName, setACStateName);
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();
        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE))
            .SetReturn(MULTITREE_INVALID_ARG);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Argument_Node_Value_Fails_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        SetupCommand(quotedSetACStateName, setACStateName);
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();
        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue))
            .SetReturn(MULTITREE_INVALID_ARG);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Decoding_The_Argument_Value_Fails_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();
        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType))
            .SetReturn(AGENT_DATA_TYPES_INVALID_ARG);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_With_Valid_Command_With_2_Args_Decodes_The_Arguments_And_Calls_The_ActionCallback)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /* arg 1 */
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        SetupCommand(quotedSetACStateName, setACStateName);

        size_t argCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();
        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);

        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType));


        /* arg 2 */
        SetupArgumentCalls(SetACStateActionHandle, 1, OtherArgActionArgument, OtherArgActionArgument_Name, OtherArgActionArgument_Type);
        const char* otherArgValue = OtherArgValue;
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "OtherArg", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG2_NODE, sizeof(TEST_ARG2_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("ascii_char_ptr"))
            .SetReturn(EDM_STRING_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG2_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &otherArgValue, sizeof(otherArgValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(otherArgValue, EDM_STRING_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &OtherArgAgentDataType, sizeof(OtherArgAgentDataType));

        STRICT_EXPECTED_CALL(ActionCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "", "SetACState", 2, IGNORED_PTR_ARG))
            .IgnoreArgument(5);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_GetArgument_For_The_2nd_Argument_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /* arg 1 */
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);

        size_t argCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType));

        /* arg 2 */
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(SetACStateActionHandle, 1))
            .SetReturn((SCHEMA_ACTION_ARGUMENT_HANDLE)NULL);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_GetArgument_Name_For_The_2nd_Argument_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /* arg 1 */
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType));

        /* arg 2 */
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(SetACStateActionHandle, 1))
            .SetReturn(OtherArgActionArgument);
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(OtherArgActionArgument))
            .SetReturn((const char*)NULL);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_GetArgument_Type_For_The_2nd_Argument_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /* arg 1 */
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();


        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType));

        /* arg 2 */
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(SetACStateActionHandle, 1))
            .SetReturn(OtherArgActionArgument);
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(OtherArgActionArgument))
            .SetReturn(OtherArgActionArgument_Name);
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(OtherArgActionArgument))
            .SetReturn((const char*)NULL);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_GetChildName_For_The_2nd_Argument_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /* arg 1 */
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/


        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();


        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType));

        /* arg 2 */
        SetupArgumentCalls(SetACStateActionHandle, 1, OtherArgActionArgument, OtherArgActionArgument_Name, OtherArgActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "OtherArg", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG2_NODE, sizeof(TEST_ARG2_NODE))
            .SetReturn(MULTITREE_INVALID_ARG);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_GetValue_For_The_2nd_Argument_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /* arg 1 */
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/


        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType));

        /* arg 2 */
        SetupArgumentCalls(SetACStateActionHandle, 1, OtherArgActionArgument, OtherArgActionArgument_Name, OtherArgActionArgument_Type);
        const char* otherArgValue = OtherArgValue;
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "OtherArg", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG2_NODE, sizeof(TEST_ARG2_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("ascii_char_ptr"))
            .SetReturn(EDM_STRING_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG2_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &otherArgValue, sizeof(otherArgValue))
            .SetReturn(MULTITREE_INVALID_ARG);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Creating_The_Agent_Data_Type_For_The_2nd_Argument_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        /* arg 1 */
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/


        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, StateActionArgument, StateActionArgument_Name, StateActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "State", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        const char* stateValue = "true";
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("bool"))
            .SetReturn(EDM_BOOLEAN_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &stateValue, sizeof(stateValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(stateValue, EDM_BOOLEAN_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &StateAgentDataType, sizeof(StateAgentDataType));

        /* arg 2 */
        SetupArgumentCalls(SetACStateActionHandle, 1, OtherArgActionArgument, OtherArgActionArgument_Name, OtherArgActionArgument_Type);
        const char* otherArgValue = OtherArgValue;
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "OtherArg", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG2_NODE, sizeof(TEST_ARG2_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("ascii_char_ptr"))
            .SetReturn(EDM_STRING_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_ARG2_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &otherArgValue, sizeof(otherArgValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(otherArgValue, EDM_STRING_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &OtherArgAgentDataType, sizeof(OtherArgAgentDataType))
            .SetReturn(AGENT_DATA_TYPES_INVALID_ARG);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_The_Argument_Is_Complex_The_Nodes_Are_Scanned_To_Get_The_Members)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetLocationName, setLocationName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();


        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);

        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn("Lat");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty1))
            .SetReturn("double");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_ARG1_NODE, "Lat", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER1_NODE, sizeof(TEST_MEMBER1_NODE));
        const char* latValue = "42.42";
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("double"))
            .SetReturn(EDM_DOUBLE_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_MEMBER1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &latValue, sizeof(latValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(latValue, EDM_DOUBLE_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &LatAgentDataType, sizeof(LatAgentDataType));

        /* member 2 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 1))
            .SetReturn(memberProperty2);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty2))
            .SetReturn("Long");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty2))
            .SetReturn("double");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_ARG1_NODE, "Long", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER2_NODE, sizeof(TEST_MEMBER2_NODE));
        const char * longValue = "1.2";
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("double"))
            .SetReturn(EDM_DOUBLE_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_MEMBER2_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &longValue, sizeof(longValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(longValue, EDM_DOUBLE_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &LongAgentDataType, sizeof(LongAgentDataType));

        EXPECTED_CALL(Create_AGENT_DATA_TYPE_from_Members(IGNORED_PTR_ARG, "GeoLocation", 2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .ValidateArgument(2).ValidateArgument(3);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ActionCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "", "SetLocation", 1, IGNORED_PTR_ARG))
            .IgnoreArgument(5);
        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);
        ASSERT_ARE_EQUAL(char_ptr, "Lat", lastMemberNames[0][0]);
        ASSERT_ARE_EQUAL(char_ptr, "Long", lastMemberNames[0][1]);

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_The_Argument_Is_Complex_The_Nodes_Are_Scanned_To_Get_The_Members_and_fail_when_gbaloc_fails_1)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetLocationName, setLocationName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();


        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1)
            .SetReturn(NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_The_Argument_Is_Complex_The_Nodes_Are_Scanned_To_Get_The_Members_and_fail_when_gbaloc_fails_2)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetLocationName, setLocationName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();


        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));

        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments()
            .SetReturn(NULL);

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Structure_Type_For_A_Complex_Type_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn((SCHEMA_STRUCT_TYPE_HANDLE)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);
        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Structure_PropertyCount_For_A_Complex_Type_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount))
            .SetReturn(SCHEMA_ERROR);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_The_Structure_PropertyCount_For_A_Complex_Type_Is_Zero_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 0;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Structure_Property_For_A_Complex_Type_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();


        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);

        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn((SCHEMA_PROPERTY_HANDLE)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Struct_Member_Name_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);


        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn((const char*)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Struct_Member_Type_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();


        size_t argCount = 1;
        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);

        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn("Lat");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty1))
            .SetReturn((const char*)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Child_Node_For_A_Member_Property_For_A_Complex_Type_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);


        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn("Lat");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty1))
            .SetReturn("double");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_ARG1_NODE, "Lat", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER1_NODE, sizeof(TEST_MEMBER1_NODE))
            .SetReturn(MULTITREE_INVALID_ARG);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Getting_The_Child_Value_For_A_Member_Property_For_A_Complex_Type_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);

        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn("Lat");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty1))
            .SetReturn("double");

        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_ARG1_NODE, "Lat", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER1_NODE, sizeof(TEST_MEMBER1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("double")) /*22*/
            .SetReturn(EDM_DOUBLE_TYPE);
        const char* latValue = "42.42";
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_MEMBER1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &latValue, sizeof(latValue))
            .SetReturn(MULTITREE_INVALID_ARG);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Creating_The_Agent_Data_Type_Value_For_A_Member_Property_For_A_Complex_Type_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();


        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);


        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn("Lat");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty1))
            .SetReturn("double");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_ARG1_NODE, "Lat", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER1_NODE, sizeof(TEST_MEMBER1_NODE));
        const char* latValue = "42.42";
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("double"))
            .SetReturn(EDM_DOUBLE_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_MEMBER1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &latValue, sizeof(latValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(latValue, EDM_DOUBLE_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &LatAgentDataType, sizeof(LatAgentDataType))
            .SetReturn(AGENT_DATA_TYPES_INVALID_ARG);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Creating_The_Complex_Type_Agent_Data_Type_Value_For_A_Complex_Type_Fails_Then_ExecuteCommand_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);

        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn("Lat");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty1))
            .SetReturn("double");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_ARG1_NODE, "Lat", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER1_NODE, sizeof(TEST_MEMBER1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("double"))
            .SetReturn(EDM_DOUBLE_TYPE);
        const char* latValue = "42.42";
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_MEMBER1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &latValue, sizeof(latValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(latValue, EDM_DOUBLE_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &LatAgentDataType, sizeof(LatAgentDataType));

        EXPECTED_CALL(Create_AGENT_DATA_TYPE_from_Members(IGNORED_PTR_ARG, "GeoLocation", 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .ValidateArgument(2).ValidateArgument(3)
            .SetReturn(AGENT_DATA_TYPES_INVALID_ARG);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_When_Decoding_The_2nd_Member_Of_A_Complex_Property_Fails_Execute_Command_Frees_The_Previously_Allocated_Member)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetACStateName, setACStateName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);

        /* member 1 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn("Lat");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty1))
            .SetReturn("double");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_ARG1_NODE, "Lat", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER1_NODE, sizeof(TEST_MEMBER1_NODE));
        const char* latValue = "42.42";
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("double"))
            .SetReturn(EDM_DOUBLE_TYPE);
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_MEMBER1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &latValue, sizeof(latValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(latValue, EDM_DOUBLE_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &LatAgentDataType, sizeof(LatAgentDataType));

        /* member 2 */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 1))
            .SetReturn((SCHEMA_PROPERTY_HANDLE)NULL);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_Decoding_An_Action_With_Nested_Complex_Types_Succeeds)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        SetupCommand(quotedSetLocationName, setLocationName);
        size_t argCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetLocationActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating memory for the argument array*/
            .IgnoreAllArguments();

        SetupArgumentCalls(SetACStateActionHandle, 0, LocationActionArgument, LocationActionArgument_Name, LocationActionArgument_Type);
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ARGS_NODE, "Location", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_ARG1_NODE, sizeof(TEST_ARG1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("GeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "GeoLocation"))
            .SetReturn(TEST_STRUCT_1_HANDLE);
        size_t memberCount = 1;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_1_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);

        /* member 1, nested complex type */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_1_HANDLE, 0))
            .SetReturn(memberNestedComplexTypeProperty);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberNestedComplexTypeProperty))
            .SetReturn("NestedLocation");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberNestedComplexTypeProperty))
            .SetReturn("NestedGeoLocation");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_ARG1_NODE, "NestedLocation", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_NESTED_STRUCT_NODE, sizeof(TEST_NESTED_STRUCT_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("NestedGeoLocation"))
            .SetReturn(EDM_NO_TYPE);
        STRICT_EXPECTED_CALL(Schema_GetStructTypeByName(TEST_SCHEMA_HANDLE, "NestedGeoLocation"))
            .SetReturn(TEST_STRUCT_2_HANDLE);
        memberCount = 2;
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyCount(TEST_STRUCT_2_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &memberCount, sizeof(memberCount));
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is allocating the member values of the struct*/
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is allocating the member names of the struct*/
            .IgnoreArgument(1);

        /* member 1, nested */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_2_HANDLE, 0))
            .SetReturn(memberProperty1);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty1))
            .SetReturn("Lat");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty1))
            .SetReturn("double");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_NESTED_STRUCT_NODE, "Lat", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER1_NODE, sizeof(TEST_MEMBER1_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("double"))
            .SetReturn(EDM_DOUBLE_TYPE);
        const char* latValue = "42.42";
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_MEMBER1_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &latValue, sizeof(latValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(latValue, EDM_DOUBLE_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &LatAgentDataType, sizeof(LatAgentDataType));

        /* member 2, nested */
        STRICT_EXPECTED_CALL(Schema_GetStructTypePropertyByIndex(TEST_STRUCT_2_HANDLE, 1))
            .SetReturn(memberProperty2);
        STRICT_EXPECTED_CALL(Schema_GetPropertyName(memberProperty2))
            .SetReturn("Long");
        STRICT_EXPECTED_CALL(Schema_GetPropertyType(memberProperty2))
            .SetReturn("double");
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_NESTED_STRUCT_NODE, "Long", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_MEMBER2_NODE, sizeof(TEST_MEMBER2_NODE));
        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("double"))
            .SetReturn(EDM_DOUBLE_TYPE);
        const char* longValue = "1.2";
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_MEMBER2_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &longValue, sizeof(longValue));
        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(longValue, EDM_DOUBLE_TYPE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &LongAgentDataType, sizeof(LongAgentDataType));

        EXPECTED_CALL(Create_AGENT_DATA_TYPE_from_Members(IGNORED_PTR_ARG, "NestedGeoLocation", 2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .ValidateArgument(2).ValidateArgument(3);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        EXPECTED_CALL(Create_AGENT_DATA_TYPE_from_Members(IGNORED_PTR_ARG, "GeoLocation", 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .ValidateArgument(2).ValidateArgument(3);
        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ActionCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "", "SetLocation", 1, IGNORED_PTR_ARG))
            .IgnoreArgument(5);

        EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(char_ptr, "Lat", lastMemberNames[0][0]);
        ASSERT_ARE_EQUAL(char_ptr, "Long", lastMemberNames[0][1]);
        ASSERT_ARE_EQUAL(char_ptr, "NestedLocation", lastMemberNames[1][0]);

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_With_A_Command_In_A_Child_Model_Calls_The_ActionCallback)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        const char* quotedActionName = "\"ChildModel/SetACState\"";

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/ /*well - first part of it ="ChildModel"*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Schema_GetModelModelByName(TEST_MODEL_HANDLE, "ChildModel"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/ /*well - first part of it ="ChildModel"*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Parameters", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_ARGS_NODE, sizeof(TEST_COMMAND_ARGS_NODE));

        STRICT_EXPECTED_CALL(Schema_GetModelActionByName(TEST_CHILD_MODEL_HANDLE, "SetACState"))
            .SetReturn(SetACStateActionHandle);
        size_t argCount = 0;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(ActionCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "ChildModel", "SetACState", 0, NULL));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);


        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_With_A_Command_In_A_Child_Model_Calls_The_ActionCallback_and_returns_REJECTED)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        const char* quotedActionName = "\"ChildModel/SetACState\"";

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/ /*well - first part of it ="ChildModel"*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Schema_GetModelModelByName(TEST_MODEL_HANDLE, "ChildModel"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/ /*well - last part of it ="SetACState"*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Parameters", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_ARGS_NODE, sizeof(TEST_COMMAND_ARGS_NODE));

        STRICT_EXPECTED_CALL(Schema_GetModelActionByName(TEST_CHILD_MODEL_HANDLE, "SetACState"))
            .SetReturn(SetACStateActionHandle);
        size_t argCount = 0;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(ActionCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "ChildModel", "SetACState", 0, NULL))
            .SetReturn(EXECUTE_COMMAND_FAILED);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_FAILED, result);


        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_With_A_Command_In_A_Child_Model_Calls_The_ActionCallback_and_returns_ABANDONED)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        const char* quotedActionName = "\"ChildModel/SetACState\"";

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/ /*well - first part of it ="ChildModel"*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(Schema_GetModelModelByName(TEST_MODEL_HANDLE, "ChildModel"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/ /*well - last part of it ="SetACState"*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Parameters", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_ARGS_NODE, sizeof(TEST_COMMAND_ARGS_NODE));

        STRICT_EXPECTED_CALL(Schema_GetModelActionByName(TEST_CHILD_MODEL_HANDLE, "SetACState"))
            .SetReturn(SetACStateActionHandle);
        size_t argCount = 0;
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(SetACStateActionHandle, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &argCount, sizeof(argCount));
        STRICT_EXPECTED_CALL(ActionCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "ChildModel", "SetACState", 0, NULL))
            .SetReturn(EXECUTE_COMMAND_ERROR);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_With_A_Command_In_A_Child_Model_when_gballoc_fails_it_fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        const char* quotedActionName = "\"ChildModel/SetACState\"";

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/ /*well - first part of it ="ChildModel"*/
            .IgnoreArgument(1)
            .SetReturn(NULL);


        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        ASSERT_IS_NOT_NULL(CommandDecoder_ExecuteCommand);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteCommand_With_A_Command_In_A_Child_Model_And_The_Child_Model_Does_Not_Exist_Fails)
    {
        // arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        const char* quotedActionName = "\"ChildModel/SetLocation\"";

        STRICT_EXPECTED_CALL(gballoc_malloc(strlen(TEST_COMMAND) + 1)); /*this creates a copy of the command that is given to JSON decoder*/
        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(TestCommand, IGNORED_PTR_ARG)).IgnoreArgument(2);
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(TEST_COMMAND_ROOT_NODE, "Name", IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_COMMAND_NAME_NODE, sizeof(TEST_COMMAND_NAME_NODE));
        STRICT_EXPECTED_CALL(MultiTree_GetValue(TEST_COMMAND_NAME_NODE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &quotedActionName, sizeof(quotedActionName));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is relativeActionPath*/ /*well - first part of it ="ChildModel"*/
            .IgnoreArgument(1);

        /*this is relativeActionPath*/ /*well - notice there's no last part of it not found*/

        STRICT_EXPECTED_CALL(Schema_GetModelModelByName(TEST_MODEL_HANDLE, "ChildModel"))
            .SetReturn((SCHEMA_MODEL_TYPE_HANDLE)NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*free-ing the copy of the buffer*/
            .IgnoreArgument(1);

        // act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_ExecuteCommand(commandDecoderHandle, TEST_COMMAND);

        // assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_NULL_startAddress_fails)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        ///act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(NULL, commandDecoderHandle, "some properties", false);

        ///assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);

        ///clean
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_NULL_handle_fails)
    {
        ///arrange
        char deviceMemoryArea[100];

        ///act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(deviceMemoryArea, NULL, "some properties", false);

        ///assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);

        ///clean
    }

    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_NULL_desiredProperties_fails)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();
        char deviceMemoryArea[100];

        ///act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(deviceMemoryArea, commandDecoderHandle, NULL, false);

        ///assert
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_ERROR, result);

        ///clean
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    void CommandDecoder_IngestDesiredProperties_with_1_simple_desired_property_succeeds_inert_path(unsigned char* deviceMemoryArea, const char* desiredPropertiesJSON, const char* three, size_t one, MULTITREE_HANDLE childHandle, bool desiredPropertyHasCallback)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, desiredPropertiesJSON))
            .IgnoreArgument_destination();

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_json()
            .IgnoreArgument_multiTreeHandle();

        STRICT_EXPECTED_CALL(MultiTree_DeleteChild(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();

        STRICT_EXPECTED_CALL(MultiTree_GetChildCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail()
            .CopyOutArgumentBuffer_count(&one, sizeof(one));

        STRICT_EXPECTED_CALL(MultiTree_GetChild(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle()
            .CopyOutArgumentBuffer_childHandle(&childHandle, sizeof(childHandle));

        STRICT_EXPECTED_CALL(STRING_new())
            .SetReturn(TEST_STRING_HANDLE_CHILD_NAME);

        STRICT_EXPECTED_CALL(MultiTree_GetName(childHandle, TEST_STRING_HANDLE_CHILD_NAME)); /*this fills in TEST_STRING_HANDLE_CHILD_NAME with "int_field"*/

        STRICT_EXPECTED_CALL(STRING_c_str(TEST_STRING_HANDLE_CHILD_NAME)) /*6*/
            .SetReturn("int_field")
            .CallCannotFail();

        STRICT_EXPECTED_CALL(Schema_GetModelElementByName(TEST_MODEL_HANDLE, "int_field"))
            .SetReturn(Schema_GetModelElementByName_desiredProperty_int_field);

        STRICT_EXPECTED_CALL(Schema_GetModelDesiredPropertyType(TEST_DESIRED_PROPERTY_HANDLE_INT_FIELD))
            .CallCannotFail()
            .SetReturn("int");

        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE))
            .CallCannotFail()
            .SetReturn(TEST_SCHEMA);

        /*this is DecodeValueFromNode expected calls*/

        STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("int"))
            .CallCannotFail()
            .SetReturn(EDM_INT32_TYPE);

        STRICT_EXPECTED_CALL(MultiTree_GetValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle()
            .CopyOutArgumentBuffer_destination(&three, sizeof(three));

        STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(IGNORED_PTR_ARG, EDM_INT32_TYPE, IGNORED_PTR_ARG))
            .IgnoreArgument_source()
            .IgnoreArgument_agentData()
            .SetReturn(AGENT_DATA_TYPES_OK);

        STRICT_EXPECTED_CALL(Schema_GetModelDesiredProperty_pfDesiredPropertyFromAGENT_DATA_TYPE(TEST_DESIRED_PROPERTY_HANDLE_INT_FIELD))
            .CallCannotFail()
            .SetReturn(int_pfDesiredPropertyFromAGENT_DATA_TYPE);

        STRICT_EXPECTED_CALL(Schema_GetModelDesiredProperty_offset(TEST_DESIRED_PROPERTY_HANDLE_INT_FIELD))
            .CallCannotFail()
            .SetReturn(2);

        STRICT_EXPECTED_CALL(int_pfDesiredPropertyFromAGENT_DATA_TYPE(IGNORED_PTR_ARG, (unsigned char*)deviceMemoryArea + 2))
            .IgnoreArgument_source();

        STRICT_EXPECTED_CALL(Schema_GetModelDesiredProperty_pfOnDesiredProperty(IGNORED_PTR_ARG))
            .CallCannotFail()
            .SetReturn(desiredPropertyHasCallback ? onDesiredPropertySimpleProperty : NULL);

        if (desiredPropertyHasCallback)
        {
            STRICT_EXPECTED_CALL(onDesiredPropertySimpleProperty(IGNORED_PTR_ARG));
        }

        STRICT_EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(STRING_delete(TEST_STRING_HANDLE_CHILD_NAME));

        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    /*case1: a simple property (non-recursive) is ingested*/
    /*the property is called "int_field" and shall have the value 3*/
    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_1_simple_desired_property_happy_path)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();
        unsigned char deviceMemoryArea[100];
        const char* desiredPropertiesJSON = "{\"int_field\":3}";
        const char* three = "3";
        size_t one = 1;
        MULTITREE_HANDLE childHandle = (MULTITREE_HANDLE)0x11;

        CommandDecoder_IngestDesiredProperties_with_1_simple_desired_property_succeeds_inert_path(deviceMemoryArea, desiredPropertiesJSON, three, one, childHandle, false);

        ///act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(deviceMemoryArea, commandDecoderHandle, desiredPropertiesJSON, false);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);

        ///clean
        CommandDecoder_Destroy(commandDecoderHandle);

    }

    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_1_simple_desired_property_succeeds_unhappy_paths)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        unsigned char deviceMemoryArea[100];
        const char* desiredPropertiesJSON = "{\"int_field\":3}";
        const char* three = "3";
        size_t one = 1;
        MULTITREE_HANDLE childHandle = (MULTITREE_HANDLE)0x11;
        (void)umock_c_negative_tests_init();
        umock_c_reset_all_calls();

        CommandDecoder_IngestDesiredProperties_with_1_simple_desired_property_succeeds_inert_path(deviceMemoryArea, desiredPropertiesJSON, three, one, childHandle, false);

        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t i = 0; i < count; i++)
        {
            umock_c_negative_tests_reset();

            if (umock_c_negative_tests_can_call_fail(i))
            {
                umock_c_negative_tests_fail_call(i);

                //act
                EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(deviceMemoryArea, commandDecoderHandle, desiredPropertiesJSON, false);

                //assert
                ASSERT_ARE_NOT_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result, "CommandDecoder_IngestDesiredProperties failure in test %zu/%zu", i, count);
            }
        }

        umock_c_negative_tests_deinit();

        ///clean
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    void CommandDecoder_IngestDesiredProperties_with_1_simple_model_in_model_desired_property_inert_path(unsigned char* deviceMemoryArea, const char* desiredPropertiesJSON, const char* three, size_t one, MULTITREE_HANDLE childHandle, bool desiredPropertiesHaveCallbacks)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, desiredPropertiesJSON))
            .IgnoreArgument_destination();

        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_json()
            .IgnoreArgument_multiTreeHandle();

        STRICT_EXPECTED_CALL(MultiTree_DeleteChild(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle()
            .IgnoreArgument_childName();

        STRICT_EXPECTED_CALL(MultiTree_GetChildCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*2*/
            .IgnoreArgument_treeHandle()
            .CopyOutArgumentBuffer_count(&one, sizeof(one));

        STRICT_EXPECTED_CALL(MultiTree_GetChild(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle()
            .CopyOutArgumentBuffer_childHandle(&childHandle, sizeof(childHandle));

        STRICT_EXPECTED_CALL(STRING_new())
            .SetReturn(TEST_STRING_HANDLE_CHILD_NAME);

        STRICT_EXPECTED_CALL(MultiTree_GetName(childHandle, TEST_STRING_HANDLE_CHILD_NAME)); /*this fills in TEST_STRING_HANDLE_CHILD_NAME with "modelInModel"*/

        STRICT_EXPECTED_CALL(STRING_c_str(TEST_STRING_HANDLE_CHILD_NAME)) /*6*/
            .SetReturn("modelInModel");

        STRICT_EXPECTED_CALL(Schema_GetModelElementByName(TEST_MODEL_HANDLE, "modelInModel"))
            .SetReturn(Schema_GetModelElementByName_modelInModel);

        STRICT_EXPECTED_CALL(Schema_GetModelModelByName_Offset(TEST_MODEL_HANDLE, "modelInModel")) /*9*/
            .SetReturn(10);

        /*here recursion happens*/

        {
            STRICT_EXPECTED_CALL(MultiTree_GetChildCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*10*/
                .IgnoreArgument_treeHandle()
                .CopyOutArgumentBuffer_count(&one, sizeof(one));

            STRICT_EXPECTED_CALL(MultiTree_GetChild(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
                .IgnoreArgument_treeHandle()
                .CopyOutArgumentBuffer_childHandle(&childHandle, sizeof(childHandle));

            STRICT_EXPECTED_CALL(STRING_new())
                .SetReturn(TEST_STRING_HANDLE_CHILD_NAME);

            STRICT_EXPECTED_CALL(MultiTree_GetName(childHandle, TEST_STRING_HANDLE_CHILD_NAME)); /*13*/ /*this fills in TEST_STRING_HANDLE_CHILD_NAME with "int_field"*/

            STRICT_EXPECTED_CALL(STRING_c_str(TEST_STRING_HANDLE_CHILD_NAME)) /*14*/
                .SetReturn("int_field");

            STRICT_EXPECTED_CALL(Schema_GetModelElementByName(SCHEMA_MODEL_TYPE_HANDLE_MODEL_IN_MODEL, "int_field"))
                .SetReturn(Schema_GetModelElementByName_desiredProperty_int_field);

            STRICT_EXPECTED_CALL(Schema_GetModelDesiredPropertyType(TEST_DESIRED_PROPERTY_HANDLE_INT_FIELD)) /*17*/
                .SetReturn("int");

            STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(SCHEMA_MODEL_TYPE_HANDLE_MODEL_IN_MODEL)) /*18*/
                .SetReturn(TEST_SCHEMA);

            /*this is DecodeValueFromNodea expected calls*/

            STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("int")) /*19*/
                .SetReturn(EDM_INT32_TYPE);

            STRICT_EXPECTED_CALL(MultiTree_GetValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument_treeHandle()
                .CopyOutArgumentBuffer_destination(&three, sizeof(three));

            STRICT_EXPECTED_CALL(CreateAgentDataType_From_String(IGNORED_PTR_ARG, EDM_INT32_TYPE, IGNORED_PTR_ARG))
                .IgnoreArgument_source()
                .IgnoreArgument_agentData()
                .SetReturn(AGENT_DATA_TYPES_OK);

            STRICT_EXPECTED_CALL(Schema_GetModelDesiredProperty_pfDesiredPropertyFromAGENT_DATA_TYPE(TEST_DESIRED_PROPERTY_HANDLE_INT_FIELD)) /*22*/
                .SetReturn(int_pfDesiredPropertyFromAGENT_DATA_TYPE);

            STRICT_EXPECTED_CALL(Schema_GetModelDesiredProperty_offset(TEST_DESIRED_PROPERTY_HANDLE_INT_FIELD)) /*23*/
                .SetReturn(2);

            STRICT_EXPECTED_CALL(int_pfDesiredPropertyFromAGENT_DATA_TYPE(IGNORED_PTR_ARG, (unsigned char*)deviceMemoryArea + 12))  /*notice here the new offset (2+10)*/
                .IgnoreArgument_source();

            STRICT_EXPECTED_CALL(Schema_GetModelDesiredProperty_pfOnDesiredProperty(IGNORED_PTR_ARG)) /*24*/
                .IgnoreArgument_desiredPropertyHandle()
                .SetReturn(desiredPropertiesHaveCallbacks ? onDesiredPropertySimpleProperty : NULL);

            if (desiredPropertiesHaveCallbacks)
            {
                STRICT_EXPECTED_CALL(onDesiredPropertySimpleProperty(IGNORED_PTR_ARG))
                    .IgnoreArgument_v();
            }


            STRICT_EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG))
                .IgnoreArgument_agentData();

            STRICT_EXPECTED_CALL(STRING_delete(TEST_STRING_HANDLE_CHILD_NAME));
        }

        STRICT_EXPECTED_CALL(Schema_GetModelModelByName_OnDesiredProperty(IGNORED_PTR_ARG, "modelInModel")) /*27*/
            .IgnoreArgument_modelTypeHandle()
            .SetReturn(desiredPropertiesHaveCallbacks ? onDesiredPropertyModelInModel : NULL);

        if (desiredPropertiesHaveCallbacks)
        {
            STRICT_EXPECTED_CALL(onDesiredPropertyModelInModel(IGNORED_PTR_ARG))
                .IgnoreArgument_v();
        }

        STRICT_EXPECTED_CALL(STRING_delete(TEST_STRING_HANDLE_CHILD_NAME));

        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument_ptr();
    }

    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_1_simple_model_in_model_desired_property_happy_path)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();
        unsigned char deviceMemoryArea[100];
        const char* desiredPropertiesJSON = "{\"modelInModel\":{\"int_field\":3}}";
        const char* three = "3";
        size_t one = 1;
        MULTITREE_HANDLE childHandle = (MULTITREE_HANDLE)0x11;

        CommandDecoder_IngestDesiredProperties_with_1_simple_model_in_model_desired_property_inert_path(deviceMemoryArea, desiredPropertiesJSON, three, one, childHandle, false);

        ///act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(deviceMemoryArea, commandDecoderHandle, desiredPropertiesJSON, false);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);

        ///clean
        CommandDecoder_Destroy(commandDecoderHandle);

    }

    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_1_simple_model_in_model_desired_property_unhappy_paths)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        unsigned char deviceMemoryArea[100];
        const char* desiredPropertiesJSON = "{\"modelInModel\":{\"int_field\":3}}";
        const char* three = "3";
        size_t one = 1;
        MULTITREE_HANDLE childHandle = (MULTITREE_HANDLE)0x11;
        (void)umock_c_negative_tests_init();
        umock_c_reset_all_calls();

        CommandDecoder_IngestDesiredProperties_with_1_simple_model_in_model_desired_property_inert_path(deviceMemoryArea, desiredPropertiesJSON, three, one, childHandle, false);

        umock_c_negative_tests_snapshot();

        size_t calls_that_cannot_fail[] =
        {
            2, /*MultiTree_DeleteChild*/
            3, /*MultiTree_GetChildCount*/
            7, /*STRING_c_str*/
            9, /*Schema_GetModelModelByName_Offset*/
            10, /*MultiTree_GetChildCount*/
            13, /*MultiTree_GetName*/
            14, /*STRING_c_str*/
            16, /*Schema_GetModelDesiredPropertyType*/
            17, /*Schema_GetSchemaForModelType*/
            18, /*CodeFirst_GetPrimitiveType*/
            21, /*Schema_GetModelDesiredProperty_pfDesiredPropertyFromAGENT_DATA_TYPE*/
            22, /*Schema_GetModelDesiredProperty_offset*/
            24, /*Destroy_AGENT_DATA_TYPE*/
            25, /*Schema_GetModelDesiredProperty_pfOnDesiredProperty*/
            26, /*STRING_delete*/
            27, /*STRING_delete*/
            28, /*Schema_GetModelModelByName_OnDesiredProperty*/
            29, /*MultiTree_Destroy*/
            30, /*gballoc_free*/
        };

        size_t count = umock_c_negative_tests_call_count();
        for (size_t i = 0; i < count; i++)
        {
            size_t j;
            umock_c_negative_tests_reset();

            for (j = 0;j<sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]);j++) /*not running the tests that cannot fail*/
            {
                if (calls_that_cannot_fail[j] == i)
                    break;
            }

            if (j == sizeof(calls_that_cannot_fail) / sizeof(calls_that_cannot_fail[0]))
            {

                umock_c_negative_tests_fail_call(i);
                char temp_str[128];
                sprintf(temp_str, "On failed call %lu", (unsigned long)i);

                ///act
                EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(deviceMemoryArea, commandDecoderHandle, desiredPropertiesJSON, false);

                ///assert
                ASSERT_ARE_NOT_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result, "CommandDecoder_IngestDesiredProperties failure in test %zu/%zu", i, count);
            }

        }

        umock_c_negative_tests_deinit();

        ///clean
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_1_simple_desired_property_calls_onDesiredProperty_happy_path)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();
        unsigned char deviceMemoryArea[100];
        const char* desiredPropertiesJSON = "{\"int_field\":3}";
        const char* three = "3";
        size_t one = 1;
        MULTITREE_HANDLE childHandle = (MULTITREE_HANDLE)0x11;

        CommandDecoder_IngestDesiredProperties_with_1_simple_desired_property_succeeds_inert_path(deviceMemoryArea, desiredPropertiesJSON, three, one, childHandle, true);

        ///act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(deviceMemoryArea, commandDecoderHandle, desiredPropertiesJSON, false);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);

        ///clean
        CommandDecoder_Destroy(commandDecoderHandle);

    }

    TEST_FUNCTION(CommandDecoder_IngestDesiredProperties_with_1_model_in_model_desired_property_calls_onDesiredProperty_happy_path)
    {

        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();
        unsigned char deviceMemoryArea[100];
        const char* desiredPropertiesJSON = "{\"modelInModel\":{\"int_field\":3}}";
        const char* three = "3";
        size_t one = 1;
        MULTITREE_HANDLE childHandle = (MULTITREE_HANDLE)0x11;

        CommandDecoder_IngestDesiredProperties_with_1_simple_model_in_model_desired_property_inert_path(deviceMemoryArea, desiredPropertiesJSON, three, one, childHandle, true);

        ///act
        EXECUTE_COMMAND_RESULT result = CommandDecoder_IngestDesiredProperties(deviceMemoryArea, commandDecoderHandle, desiredPropertiesJSON, false);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(EXECUTE_COMMAND_RESULT, EXECUTE_COMMAND_SUCCESS, result);

        ///clean
        CommandDecoder_Destroy(commandDecoderHandle);

    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_NULL_handle_fails)
    {
        ///arrange

        ///act
        METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(NULL, "methodA", "{\"a\":1}");

        ///assert
        ASSERT_IS_NULL(methodReturn);

        ///clean
    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_NULL_fulMethodName_fails)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);

        ///act
        METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, NULL, "{\"a\":1}");

        ///assert
        ASSERT_IS_NULL(methodReturn);

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_NULL_methodCallback_fails)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, NULL, TEST_CALLBACK_CONTEXT_VALUE);

        ///act
        METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "methodA", "{\"a\":1}");

        ///assert
        ASSERT_IS_NULL(methodReturn);

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_NULL_fullMethodName_fails)
    {
        ///arrange
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);

        ///act
        METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, NULL, "{\"a\":1}");

        ///assert
        ASSERT_IS_NULL(methodReturn);

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    static void CommandDecoder_ExecuteMethod_with_NULL_payload_inert_path(size_t* zero)
    {
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(gballoc_malloc(1)); /*this is the string "" for relative relativeMethodPath*/
        STRICT_EXPECTED_CALL(Schema_GetModelMethodByName(TEST_MODEL_HANDLE, "methodA"));
        STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_methodHandle()
            .IgnoreArgument_argumentCount()
            .CopyOutArgumentBuffer_argumentCount(zero, sizeof(*zero));

        STRICT_EXPECTED_CALL(methodCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "", "methodA", 0, NULL));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the relativeMethodPath*/
            .IgnoreArgument_ptr();
    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_NULL_payload_hapy_path)
    {
        /*this TEST_FUNCTION assumes that there is a method in the root model called "methodA" that takes no arguments*/

        ///arrange
        size_t zero = 0;
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        CommandDecoder_ExecuteMethod_with_NULL_payload_inert_path(&zero);

        ///act
        METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "methodA", NULL);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(void_ptr, g_methodReturnValue, methodReturn);

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_NULL_payload_unhapy_paths)
    {
        /*this TEST_FUNCTION assumes that there is a method in the root model called "methodA" that takes no arguments*/

        ///arrange
        size_t zero = 0;
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        umock_c_negative_tests_init();
        CommandDecoder_ExecuteMethod_with_NULL_payload_inert_path(&zero);
        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t i = 0; i < count; i++)
        {
            if (umock_c_negative_tests_can_call_fail(i))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(i);

                ///act
                METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "methodA", NULL);

                ///assert
                ASSERT_IS_NULL(methodReturn, "CommandDecoder_ExecuteMethod failure in test %zu/%zu", i, count);
            }
        }

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
        umock_c_negative_tests_deinit();
    }

    static void CommandDecoder_ExecuteMethod_with_1_arg_payload_inert_path(size_t* one, const char* methodPayload, const char** aValue)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, methodPayload))
            .IgnoreArgument_destination();
        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_json()
            .IgnoreArgument_multiTreeHandle();
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(gballoc_malloc(1)); /*this is the string "" for relative relativeMethodPath*/
        STRICT_EXPECTED_CALL(Schema_GetModelMethodByName(TEST_MODEL_HANDLE, "methodA"));
        STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_methodHandle()
            .IgnoreArgument_argumentCount()
            .CopyOutArgumentBuffer_argumentCount(one, sizeof(*one));

        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)) /*this is the array holding 1 x AGENT_DATA_TYPE */
            .IgnoreAllArguments().IgnoreArgument_size();

        STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentByIndex(TEST_MODEL_METHOD_HANDLE, 0))
            .SetReturn(TEST_METHOD_ARGUMENT_HANDLE_0);
        STRICT_EXPECTED_CALL(Schema_GetMethodArgumentName(TEST_METHOD_ARGUMENT_HANDLE_0))
            .SetReturn("a");
        STRICT_EXPECTED_CALL(Schema_GetMethodArgumentType(TEST_METHOD_ARGUMENT_HANDLE_0))
            .SetReturn("int");

        STRICT_EXPECTED_CALL(MultiTree_GetChildByName(IGNORED_PTR_ARG, "a", IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle()
            .IgnoreArgument_childHandle();

        { /*scope for DecodeValueFromNode*/
            STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("int"))
                .SetReturn(EDM_INT32_TYPE);
            STRICT_EXPECTED_CALL(MultiTree_GetValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument_treeHandle()
                .IgnoreArgument_destination()
                .CopyOutArgumentBuffer_destination(aValue, sizeof(aValue));
            STRICT_EXPECTED_CALL(CreateAgentDataType_From_String("2", EDM_INT32_TYPE, IGNORED_PTR_ARG))
                .IgnoreArgument_agentData();
        }

        STRICT_EXPECTED_CALL(methodCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "", "methodA", 1, IGNORED_PTR_ARG))
            .IgnoreArgument_parameterValues();

        STRICT_EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG))
            .IgnoreArgument_agentData();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument_ptr();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*this is freeing the relativeMethodPath*/
            .IgnoreArgument_ptr();

        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument_treeHandle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument_ptr();

    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_1_arg_payload_hapy_path)
    {
        /*this TEST_FUNCTION assumes that there is a method in the root model called "methodA" that takes 1x arguments*/

        ///arrange
        size_t one = 1;
        const char* aValue = "2";
        const char* methodPayload = "{\"a\":2}";
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        CommandDecoder_ExecuteMethod_with_1_arg_payload_inert_path(&one, methodPayload, &aValue);

        ///act
        METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "methodA", methodPayload);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(void_ptr, g_methodReturnValue, methodReturn);

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }


    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_1_arg_payload_unhapy_paths)
    {
        /*this TEST_FUNCTION assumes that there is a method in the root model called "methodA" that takes 1x arguments*/

        ///arrange
        size_t one = 1;
        const char* aValue = "2";
        const char* methodPayload = "{\"a\":2}";
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        umock_c_negative_tests_init();
        CommandDecoder_ExecuteMethod_with_1_arg_payload_inert_path(&one, methodPayload, &aValue);
        umock_c_negative_tests_snapshot();

        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            if (
                (i != 11)&& /*gballoc_free*/
                (i != 15) && /*Destroy_AGENT_DATA_TYPE*/
                (i != 16) && /*gballoc_free*/
                (i != 17) && /*gballoc_free*/
                (i != 18) && /*MultiTree_Destroy*/
                (i != 19)  /*gballoc_free*/
                )
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(i);

                ///act
                METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "methodA", methodPayload);

                ///assert
                ASSERT_IS_NULL(methodReturn);
            }
        }

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
        umock_c_negative_tests_deinit();
    }


    static void CommandDecoder_ExecuteMethod_with_2_arg_payload_inert_path(size_t* two, const char* methodPayload, const char** aValue, const char** bValue)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, methodPayload))
            .IgnoreArgument_destination();
        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_json()
            .IgnoreArgument_multiTreeHandle();
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(gballoc_malloc(1)); /*this is the string "" for relative relativeMethodPath*/
        STRICT_EXPECTED_CALL(Schema_GetModelMethodByName(TEST_MODEL_HANDLE, "methodA"));
        STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_argumentCount(two, sizeof(*two));

        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).IgnoreAllArguments();

        { /*scope for processing every individual argument*/
            STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentByIndex(TEST_MODEL_METHOD_HANDLE, 0))
                .SetReturn(TEST_METHOD_ARGUMENT_HANDLE_0);
            STRICT_EXPECTED_CALL(Schema_GetMethodArgumentName(TEST_METHOD_ARGUMENT_HANDLE_0))
                .SetReturn("a");
            STRICT_EXPECTED_CALL(Schema_GetMethodArgumentType(TEST_METHOD_ARGUMENT_HANDLE_0))
                .SetReturn("int");

            STRICT_EXPECTED_CALL(MultiTree_GetChildByName(IGNORED_PTR_ARG, "a", IGNORED_PTR_ARG))
                .IgnoreArgument_treeHandle()
                .IgnoreArgument_childHandle();

            { /*scope for DecodeValueFromNode*/
                STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("int"))
                    .CallCannotFail()
                    .SetReturn(EDM_INT32_TYPE);
                STRICT_EXPECTED_CALL(MultiTree_GetValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                    .CopyOutArgumentBuffer_destination(aValue, sizeof(aValue));
                STRICT_EXPECTED_CALL(CreateAgentDataType_From_String("2", EDM_INT32_TYPE, IGNORED_PTR_ARG))
                    .IgnoreArgument_agentData();
            }

            STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentByIndex(TEST_MODEL_METHOD_HANDLE, 1))
                .SetReturn(TEST_METHOD_ARGUMENT_HANDLE_1);
            STRICT_EXPECTED_CALL(Schema_GetMethodArgumentName(TEST_METHOD_ARGUMENT_HANDLE_1))
                .SetReturn("b");
            STRICT_EXPECTED_CALL(Schema_GetMethodArgumentType(TEST_METHOD_ARGUMENT_HANDLE_1))
                .SetReturn("int");

            STRICT_EXPECTED_CALL(MultiTree_GetChildByName(IGNORED_PTR_ARG, "b", IGNORED_PTR_ARG))
                .IgnoreArgument_treeHandle()
                .IgnoreArgument_childHandle();

            { /*scope for DecodeValueFromNode*/
                STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("int"))
                    .CallCannotFail()
                    .SetReturn(EDM_INT32_TYPE);
                STRICT_EXPECTED_CALL(MultiTree_GetValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                    .CopyOutArgumentBuffer_destination(bValue, sizeof(bValue));
                STRICT_EXPECTED_CALL(CreateAgentDataType_From_String("3", EDM_INT32_TYPE, IGNORED_PTR_ARG)) /*20*/
                    .IgnoreArgument_agentData();
            }
        }

        STRICT_EXPECTED_CALL(methodCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "", "methodA", 2, IGNORED_PTR_ARG))
            .IgnoreArgument_parameterValues();

        STRICT_EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_2_arg_payload_hapy_path)
    {
        /*this TEST_FUNCTION assumes that there is a method in the root model called "methodA" that takes 1x arguments*/

        ///arrange
        size_t two = 2;
        const char* aValue = "2";
        const char* bValue = "3";
        const char* methodPayload = "{\"a\":2, \"b\":3}";
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        CommandDecoder_ExecuteMethod_with_2_arg_payload_inert_path(&two, methodPayload, &aValue, &bValue);

        ///act
        METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "methodA", methodPayload);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(void_ptr, g_methodReturnValue, methodReturn);

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }


    TEST_FUNCTION(CommandDecoder_ExecuteMethod_with_2_arg_payload_unhapy_paths)
    {
        /*this TEST_FUNCTION assumes that there is a method in the root model called "methodA" that takes 1x arguments*/

        ///arrange
        size_t two = 2;
        const char* aValue = "2";
        const char* bValue = "3";
        const char* methodPayload = "{\"a\":2, \"b\":3}";
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        umock_c_negative_tests_init();
        CommandDecoder_ExecuteMethod_with_2_arg_payload_inert_path(&two, methodPayload, &aValue, &bValue);
        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t i = 0; i < count; i++)
        {
            if (umock_c_negative_tests_can_call_fail(i))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(i);

                //act
                METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "methodA", methodPayload);

                //assert
                ASSERT_IS_NULL(methodReturn, "CommandDecoder_ExecuteMethod failure in test %zu/%zu", i, count);
            }
        }

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
        umock_c_negative_tests_deinit();
    }

    static void CommandDecoder_ExecuteMethod_model_in_model_with_2_arg_payload_inert_path(size_t* two, const char* methodPayload, const char** aValue, const char** bValue)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, methodPayload))
            .IgnoreArgument_destination();
        STRICT_EXPECTED_CALL(JSONDecoder_JSON_To_MultiTree(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Schema_GetSchemaForModelType(TEST_MODEL_HANDLE));
        STRICT_EXPECTED_CALL(gballoc_malloc(11)); /*this is the string "innermodel" for relative relativeMethodPath*/
        STRICT_EXPECTED_CALL(Schema_GetModelModelByName(TEST_MODEL_HANDLE, "innermodel"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); /*this is the string "innermodel" for relative relativeMethodPath*/

        STRICT_EXPECTED_CALL(gballoc_malloc(11)); /*this is the string "innermodel" for relative relativeMethodPath*/

        STRICT_EXPECTED_CALL(Schema_GetModelMethodByName(TEST_CHILD_MODEL_HANDLE, "methodA"));
        STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_argumentCount(two, sizeof(*two));

        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).IgnoreAllArguments();

        { /*scope for processing every individual argument*/
            STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentByIndex(TEST_MODEL_METHOD_HANDLE, 0))
                .SetReturn(TEST_METHOD_ARGUMENT_HANDLE_0);
            STRICT_EXPECTED_CALL(Schema_GetMethodArgumentName(TEST_METHOD_ARGUMENT_HANDLE_0))
                .SetReturn("a");
            STRICT_EXPECTED_CALL(Schema_GetMethodArgumentType(TEST_METHOD_ARGUMENT_HANDLE_0))
                .SetReturn("int");

            STRICT_EXPECTED_CALL(MultiTree_GetChildByName(IGNORED_PTR_ARG, "a", IGNORED_PTR_ARG))
                .IgnoreArgument_treeHandle()
                .IgnoreArgument_childHandle();

            { /*scope for DecodeValueFromNode*/
                STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("int"))
                    .SetReturn(EDM_INT32_TYPE);
                STRICT_EXPECTED_CALL(MultiTree_GetValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                    .CopyOutArgumentBuffer_destination(aValue, sizeof(aValue));
                STRICT_EXPECTED_CALL(CreateAgentDataType_From_String("2", EDM_INT32_TYPE, IGNORED_PTR_ARG))
                    .IgnoreArgument_agentData();
            }

            STRICT_EXPECTED_CALL(Schema_GetModelMethodArgumentByIndex(TEST_MODEL_METHOD_HANDLE, 1))
                .SetReturn(TEST_METHOD_ARGUMENT_HANDLE_1);
            STRICT_EXPECTED_CALL(Schema_GetMethodArgumentName(TEST_METHOD_ARGUMENT_HANDLE_1))
                .SetReturn("b");
            STRICT_EXPECTED_CALL(Schema_GetMethodArgumentType(TEST_METHOD_ARGUMENT_HANDLE_1))
                .SetReturn("int");

            STRICT_EXPECTED_CALL(MultiTree_GetChildByName(IGNORED_PTR_ARG, "b", IGNORED_PTR_ARG))
                .IgnoreArgument_treeHandle()
                .IgnoreArgument_childHandle();

            { /*scope for DecodeValueFromNode*/
                STRICT_EXPECTED_CALL(CodeFirst_GetPrimitiveType("int"))
                    .SetReturn(EDM_INT32_TYPE).CallCannotFail();
                STRICT_EXPECTED_CALL(MultiTree_GetValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                    .IgnoreArgument_treeHandle()
                    .IgnoreArgument_destination()
                    .CopyOutArgumentBuffer_destination(bValue, sizeof(bValue));
                STRICT_EXPECTED_CALL(CreateAgentDataType_From_String("3", EDM_INT32_TYPE, IGNORED_PTR_ARG)) /*23*/
                    .IgnoreArgument_agentData();
            }
        }

        STRICT_EXPECTED_CALL(methodCallbackMock(TEST_CALLBACK_CONTEXT_VALUE, "innermodel", "methodA", 2, IGNORED_PTR_ARG))
            .IgnoreArgument_parameterValues();

        STRICT_EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Destroy_AGENT_DATA_TYPE(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(MultiTree_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    }

    TEST_FUNCTION(CommandDecoder_ExecuteMethod_model_in_model_with_2_arg_payload_hapy_path)
    {
        /*this TEST_FUNCTION assumes that there is a method in the root model called "methodA" that takes 1x arguments*/

        ///arrange
        size_t two = 2;
        const char* aValue = "2";
        const char* bValue = "3";
        const char* methodPayload = "{\"a\":2, \"b\":3}";
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        CommandDecoder_ExecuteMethod_model_in_model_with_2_arg_payload_inert_path(&two, methodPayload, &aValue, &bValue);

        ///act
        METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "innermodel/methodA", methodPayload);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(void_ptr, g_methodReturnValue, methodReturn);

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
    }


    TEST_FUNCTION(CommandDecoder_ExecuteMethod_model_in_model_with_2_arg_payload_unhapy_paths)
    {
        /*this TEST_FUNCTION assumes that there is a method in the root model called "methodA" that takes 1x arguments*/

        ///arrange
        size_t two = 2;
        const char* aValue = "2";
        const char* bValue = "3";
        const char* methodPayload = "{\"a\":2, \"b\":3}";
        COMMAND_DECODER_HANDLE commandDecoderHandle = CommandDecoder_Create(TEST_MODEL_HANDLE, ActionCallbackMock, TEST_CALLBACK_CONTEXT_VALUE, methodCallbackMock, TEST_CALLBACK_CONTEXT_VALUE);
        umock_c_reset_all_calls();

        umock_c_negative_tests_init();
        CommandDecoder_ExecuteMethod_model_in_model_with_2_arg_payload_inert_path(&two, methodPayload, &aValue, &bValue);
        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t i = 0; i < count; i++)
        {
            if (umock_c_negative_tests_can_call_fail(i))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(i);

                //act
                METHODRETURN_HANDLE methodReturn = CommandDecoder_ExecuteMethod(commandDecoderHandle, "innermodel/methodA", methodPayload);

                //assert
                ASSERT_IS_NULL(methodReturn, "CommandDecoder_ExecuteMethod failure in test %zu/%zu", i, count);
            }
        }

        ///cleanup
        CommandDecoder_Destroy(commandDecoderHandle);
        umock_c_negative_tests_deinit();
    }


END_TEST_SUITE(CommandDecoder_ut)
