// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#endif


#include "testrunnerswitcher.h"

#include "real_parson.h"
#include "../../../serializer/tests/serializer_dt_ut/real_crt_abstractions.h"


#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void* my_gballoc_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
#include "internal/dt_lock_thread_binding.h"
#include "internal/dt_client_core.h"
#include "internal/dt_raw_interface.h"
#include "internal/dt_interface_private.h"
#include "azure_c_shared_utility/gballoc.h"
#include "parson.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, double, json_object_get_number, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, const char*, json_object_dotget_string, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Status, json_object_dotset_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, JSON_Array*, json_array_get_array, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Object*, json_array_get_object, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value*, value);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_array_clear, JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_object_clear, JSON_Object*, object);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, char *, json_serialize_to_string_pretty, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_dotset_value, JSON_Object *, object, const char *, name, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object *, json_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_value, JSON_Object *, object, const char *, name, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object *, json_object_dotget_object, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, size_t, json_object_get_count, const JSON_Object *, object);
MOCKABLE_FUNCTION(, JSON_Value *, json_object_get_value_at, const JSON_Object *, object, size_t, index);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_null, JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_array);
MOCKABLE_FUNCTION(, JSON_Status, json_array_append_string, JSON_Array*, array, const char*, string)
MOCKABLE_FUNCTION(, LOCK_HANDLE, testBindingLockInit);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingLock, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingUnlock, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingLockDeinit, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, void, testBindingThreadSleep, unsigned int, milliseconds);

#ifdef __cplusplus
}
#endif

#undef ENABLE_MOCKS

#include "dt_interface_list.h"
#include "../../../serializer/tests/serializer_dt_ut/real_crt_abstractions.h"
#include "real_strings.h"


#define DT_MODEL_DISCOVERY_INTERFACE_ID "urn:azureiot:ModelDiscovery:ModelInformation:1"
#define DT_MODEL_DISCOVERY_INTERFACE_NAME "urn_azureiot_ModelDiscovery_ModelInformation"

#define DT_SDK_INFORMATION_INTERFACE_ID "urn:azureiot:Client:SDKInformation:1"


#define DT_TEST_INTERFACE_ID_1 "urn:testOnly:testInterface1:1"
#define DT_TEST_INTERFACE_ID_2 "urn:testOnly:testInterface2:5"
#define DT_TEST_INTERFACE_ID_3 "urn:testOnly:testInterface3:1"

#define DT_TEST_INTERFACE_NAME_1 "TestOnly_TestInterface1"
#define DT_TEST_INTERFACE_NAME_2 "TestOnly_TestInterface2"
#define DT_TEST_INTERFACE_NAME_3 "TestOnly_TestInterface3"

#define DT_TEST_CAPABILITY_MODEL_ID "urn:testOnly:testDeviceCapabilityModel:1"
static const char* dtTestInterfaceIds[] =  {DT_TEST_INTERFACE_ID_1, DT_TEST_INTERFACE_ID_2, DT_TEST_INTERFACE_ID_3};
static const char* dtTestInterfaceInstanceNames[] =  {DT_TEST_INTERFACE_NAME_1, DT_TEST_INTERFACE_NAME_2, DT_TEST_INTERFACE_NAME_3 };

TEST_DEFINE_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_RESULT_VALUES);

#define DT_TEST_MESSAGE_DATA_VALUE "MessageData"
static const char* dtTestTelemetryName = "TestDTTelemetryName";
static const unsigned char* dtTestMessageData = (const unsigned char*)DT_TEST_MESSAGE_DATA_VALUE;
static const size_t dtTestMessageDataLen = sizeof(DT_TEST_MESSAGE_DATA_VALUE) - 1;
static void* dtTestClientTelemetryConfirmationContext = (void*)0x1235;

static void* testDTClientExecuteCallbackContext = (void*)0x1236;
static const char dtTestCommandName[] = "TestCommandName";
static const char dtTestCommandPayload[] = "Test payload for command invocation in list";
static const int dtTestCommandPayloadLen = sizeof(dtTestCommandPayload) - 1;

static const char dtTestCommandResponse[] = "Test simulated response data from method invocation through list";
static const int dtTestCommandResponseLen = sizeof(dtTestCommandResponse) - 1;

static const int dtTestInvalidMethod = 1000;

#define DT_TEST_INTERFACE_HANDLE1 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1001)
#define DT_TEST_INTERFACE_HANDLE2 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1002)
#define DT_TEST_INTERFACE_HANDLE3 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1003)
#define DT_TEST_INTERFACE_NOT_REGISTERED ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1004)

static DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtTestInterfaceArray[] = { DT_TEST_INTERFACE_HANDLE1, DT_TEST_INTERFACE_HANDLE2, DT_TEST_INTERFACE_HANDLE3 };

// DT_TEST_BUILD_JSON_TUPLE is used to build up the "name":"id" JSON string of the JSON in a registration message
#define DT_TEST_BUILD_JSON_TUPLE(NAME, ID) "\"" NAME "\":\"" ID "\""

#define DT_TEST_INTERFACE_MODEL_DISCOVERY_TUPLE DT_TEST_BUILD_JSON_TUPLE(DT_MODEL_DISCOVERY_INTERFACE_NAME, DT_MODEL_DISCOVERY_INTERFACE_ID)
#define DT_TEST_SDK_INFORMATION_TUPLE DT_TEST_BUILD_JSON_TUPLE(DT_SDK_INFORMATION_INTERFACE_NAME, DT_SDK_INFORMATION_INTERFACE_ID)

#define DT_TEST_INTERFACE_JSON_TUPLE_1 DT_TEST_BUILD_JSON_TUPLE(DT_TEST_INTERFACE_NAME_1, DT_TEST_INTERFACE_ID_1)
#define DT_TEST_INTERFACE_JSON_TUPLE_2 DT_TEST_BUILD_JSON_TUPLE(DT_TEST_INTERFACE_NAME_2, DT_TEST_INTERFACE_ID_2)
#define DT_TEST_INTERFACE_JSON_TUPLE_3 DT_TEST_BUILD_JSON_TUPLE(DT_TEST_INTERFACE_NAME_3, DT_TEST_INTERFACE_ID_3)

#define DT_TEST_INTERFACE_REGISTER_START  "{\"modelInformation\":{\"capabilityModelId\":\"" DT_TEST_CAPABILITY_MODEL_ID "\",\"interfaces\":{" DT_TEST_INTERFACE_MODEL_DISCOVERY_TUPLE "," DT_TEST_SDK_INFORMATION_TUPLE ","

#define DT_TEST_JSON_COMMA ","
#define DT_TEST_INTERFACE_REGISTER_END "}}}"

static const char* testDTInterfaceListRegistartionBody1Interface = 
DT_TEST_INTERFACE_REGISTER_START DT_TEST_INTERFACE_JSON_TUPLE_1 DT_TEST_INTERFACE_REGISTER_END;

static const char* testDTInterfaceListRegistartionBody2Interfaces = 
DT_TEST_INTERFACE_REGISTER_START DT_TEST_INTERFACE_JSON_TUPLE_1 DT_TEST_JSON_COMMA DT_TEST_INTERFACE_JSON_TUPLE_2 DT_TEST_INTERFACE_REGISTER_END;

static const char* testDTInterfaceListRegistartionBody3Interfaces = 
DT_TEST_INTERFACE_REGISTER_START DT_TEST_INTERFACE_JSON_TUPLE_1 DT_TEST_JSON_COMMA DT_TEST_INTERFACE_JSON_TUPLE_2 DT_TEST_JSON_COMMA DT_TEST_INTERFACE_JSON_TUPLE_3 DT_TEST_INTERFACE_REGISTER_END;

static const IOTHUB_MESSAGE_HANDLE dtTestMessageHande = (IOTHUB_MESSAGE_HANDLE)0x1400;

static const char dtTestProcessTwinJson[] = "Test JSon";
static const size_t dtTestProcessTwinJsonLen = sizeof(dtTestProcessTwinJson);

static void* dtTestInvokeCommandUserContext = (void*)0x1250;
static void* dtTestProcessReportedPropertiesUserContext = (void*)0x1251;

static void testDTClientTelemetryConfirmationCallback(DIGITALTWIN_CLIENT_RESULT dtTelemetryStatus, void* userContextCallback)
{
    (void)dtTelemetryStatus;
    (void)userContextCallback;
}

static void testDTClientExecuteCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userContextCallback)
{
    (void)dtClientCommandContext;
    (void)dtClientCommandResponseContext;
    (void)userContextCallback;
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

const char* impl_testDT_Get_RawInterfaceId(const char* dtInterface)
{
    (void)dtInterface;
    return (const char*)(my_gballoc_malloc(1));
}

const char* test_dtInterfaceList_ExpectedMessageBody;

DIGITALTWIN_CLIENT_RESULT impl_test_DT_InterfaceClient_CreateTelemetryMessage(const char* interfaceId, const char* interfaceInstanceName, const char* telemetryName, const char* messageData, IOTHUB_MESSAGE_HANDLE* telemetryMessageHandle)
{
    // Verify that the JSON generated during message creation (and hence being passed to this function) is as expected
    ASSERT_ARE_EQUAL(char_ptr, test_dtInterfaceList_ExpectedMessageBody, messageData);

    (void)interfaceId;
    (void)interfaceInstanceName;
    (void)telemetryName;
    (void)telemetryMessageHandle;
    return DIGITALTWIN_CLIENT_OK;
}


MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

BEGIN_TEST_SUITE(dt_interface_list_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_INTERFACE_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DT_COMMAND_PROCESSOR_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(DT_CLIENT_CORE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(json_value_init_object, real_json_value_init_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object, real_json_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, real_json_parse_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotset_value, real_json_object_dotset_value);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotset_value, JSONFailure);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, real_json_value_free);
    REGISTER_GLOBAL_MOCK_HOOK(json_serialize_to_string, real_json_serialize_to_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_free_serialized_string, real_json_free_serialized_string);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_get_object, real_json_value_get_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);   
    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotset_string, real_json_object_dotset_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotset_string, JSONFailure);   
    REGISTER_GLOBAL_MOCK_HOOK(json_object_set_value, real_json_object_set_value);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_value, JSONFailure);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotget_object, real_json_object_dotget_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_object, NULL); 
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_count, real_json_object_get_count);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_value_at, real_json_object_get_value_at);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_value_at, NULL); 
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_string, real_json_object_get_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL); 
    REGISTER_GLOBAL_MOCK_HOOK(json_value_init_array, real_json_value_init_array);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_array, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_array_append_string, real_json_array_append_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_array_append_string, JSONFailure);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_get_array, real_json_value_get_array);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_array, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(json_object_set_string, real_json_object_set_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_string, JSONFailure);

    REGISTER_GLOBAL_MOCK_HOOK(json_object_set_null, real_json_object_set_null);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_null, JSONFailure);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);    

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_calloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(DT_InterfaceClient_BindToClientHandle, DIGITALTWIN_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_BindToClientHandle, DIGITALTWIN_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(DT_InterfaceClient_InvokeCommandIfSupported, DT_COMMAND_PROCESSOR_PROCESSED);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_InvokeCommandIfSupported, DT_COMMAND_PROCESSOR_NOT_APPLICABLE);

    REGISTER_GLOBAL_MOCK_RETURN(DT_InterfaceClient_ProcessTwinCallback, DIGITALTWIN_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_ProcessTwinCallback, DIGITALTWIN_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback, DIGITALTWIN_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback, DIGITALTWIN_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(DT_InterfaceClient_ProcessTelemetryCallback, DIGITALTWIN_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_ProcessTelemetryCallback, DIGITALTWIN_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(DT_InterfaceClient_CreateTelemetryMessage, impl_test_DT_InterfaceClient_CreateTelemetryMessage);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_CreateTelemetryMessage, DIGITALTWIN_CLIENT_ERROR);
    
    REGISTER_GLOBAL_MOCK_HOOK(DT_Get_RawInterfaceId, impl_testDT_Get_RawInterfaceId);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_Get_RawInterfaceId, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_GetInterfaceId, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_GetInterfaceInstanceName, NULL);

    REGISTER_STRING_GLOBAL_MOCK_HOOK;
}

TEST_FUNCTION_INITIALIZE(TestMethodInit)
{
    umock_c_reset_all_calls();
    test_dtInterfaceList_ExpectedMessageBody = NULL;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_InterfaceList_Create
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT__InterfaceList_Create()
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
}

TEST_FUNCTION(DigitalTwin_InterfaceList_Create_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h;
    set_expected_calls_for_DT__InterfaceList_Create();

    //act
    h = DigitalTwin_InterfaceList_Create();

    //assert    
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        
    //cleanup
    DT_InterfaceList_Destroy(h);
}

static DIGITALTWIN_INTERFACE_LIST_HANDLE create_DT_interface_list_for_test()
{
    DIGITALTWIN_INTERFACE_LIST_HANDLE h;
    h = DigitalTwin_InterfaceList_Create();
    ASSERT_IS_NOT_NULL(h);
    umock_c_reset_all_calls();
    return h;
}

TEST_FUNCTION(DigitalTwin_InterfaceList_Create_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT__InterfaceList_Create();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_INTERFACE_LIST_HANDLE h = DigitalTwin_InterfaceList_Create();

            //assert
            ASSERT_IS_NULL(h, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_BindInterfaces
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceList_RegisterInterfaces(int numInterfacesToRegister)
{
    if (numInterfacesToRegister > 0)
    {
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    }

    for (int i = 0; i < numInterfacesToRegister; i++)
    {
        STRICT_EXPECTED_CALL(DT_InterfaceClient_BindToClientHandle(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void test_register_interface_list(DIGITALTWIN_INTERFACE_CLIENT_HANDLE* dtInterfaces, int numInterfacesToRegister)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_for_test();
    set_expected_calls_for_DT_InterfaceList_RegisterInterfaces(numInterfacesToRegister);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_BindInterfaces(h, dtInterfaces, numInterfacesToRegister, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_RegisterInterfaces_zero_interfaces_ok)
{
    test_register_interface_list(dtTestInterfaceArray, 0);
}

TEST_FUNCTION(DT_InterfaceList_RegisterInterfaces_one_interface_ok)
{
    test_register_interface_list(dtTestInterfaceArray, 1);
}

TEST_FUNCTION(DT_InterfaceList_RegisterInterfaces_two_interfaces_ok)
{
    test_register_interface_list(dtTestInterfaceArray, 2);
}

TEST_FUNCTION(DT_InterfaceList_RegisterInterfaces_three_interfaces_ok)
{
    test_register_interface_list(dtTestInterfaceArray, 3);
}

// Make sure that if we're registering multiple interfaces and final one fails (but initial,
// ones don't), that we return an error and also that we unregister the interfaces.
TEST_FUNCTION(DT_InterfaceList_RegisterInterfaces_last_interface_register_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_for_test();

    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(DT_InterfaceClient_BindToClientHandle(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DT_InterfaceClient_BindToClientHandle(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DT_InterfaceClient_BindToClientHandle(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DIGITALTWIN_CLIENT_ERROR); // 3rd interface fails.

    // After failure, expect 1st two interfaces to be unregistered
    STRICT_EXPECTED_CALL(DT_InterfaceClient_UnbindFromClientHandle(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DT_InterfaceClient_UnbindFromClientHandle(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_BindInterfaces(h, dtTestInterfaceArray, 3, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_RegisterInterfaces_NULL_list_handle_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_BindInterfaces(NULL, dtTestInterfaceArray, 1, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());    
}

TEST_FUNCTION(DT_InterfaceList_RegisterInterfaces_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_for_test();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceList_RegisterInterfaces(2);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_BindInterfaces(h, dtTestInterfaceArray, 2, NULL, NULL);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    DT_InterfaceList_Destroy(h);
    umock_c_negative_tests_deinit();
}

static DIGITALTWIN_INTERFACE_LIST_HANDLE create_DT_interface_list_and_bind(int numInterfacesToRegister)
{
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_for_test();

    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_BindInterfaces(h, dtTestInterfaceArray, numInterfacesToRegister, NULL, NULL);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    umock_c_reset_all_calls();
    return h;
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_Destroy
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceList_Destroy()
{
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void testDT_Interface_Destroy(int numInterfaces)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(numInterfaces);
    set_expected_calls_for_DT_InterfaceList_Destroy();

    //act
    DT_InterfaceList_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceList_Destroy_empty_list_ok)
{
    testDT_Interface_Destroy(0);
}

TEST_FUNCTION(DT_InterfaceList_Destroy_one_element)
{
    testDT_Interface_Destroy(1);
}

TEST_FUNCTION(DT_InterfaceList_Destroy_two_elements)
{
    testDT_Interface_Destroy(2);
}

TEST_FUNCTION(DT_InterfaceList_Destroy_three_elements)
{
    testDT_Interface_Destroy(3);
}

TEST_FUNCTION(DT_InterfaceList_Destroy_NULL_handle_fails)
{
    //act
    DT_InterfaceList_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_UnbindInterfaces
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_UnregisterExistingInterfaceHandles(int numInterfacesRegistered)
{
    for (int i = 0; i < numInterfacesRegistered; i++)
    {
        STRICT_EXPECTED_CALL(DT_InterfaceClient_UnbindFromClientHandle(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void test_DT_InterfaceList_UnregisterHandles(int numInterfacesRegistered)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(numInterfacesRegistered);
    set_expected_calls_for_UnregisterExistingInterfaceHandles(numInterfacesRegistered);

    //act
    DT_InterfaceList_UnbindInterfaces(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_UnregisterHandles_empty_interfaces)
{
    test_DT_InterfaceList_UnregisterHandles(0);
}

TEST_FUNCTION(DT_InterfaceList_UnregisterHandles_one_interface)
{
    test_DT_InterfaceList_UnregisterHandles(1);
}

TEST_FUNCTION(DT_InterfaceList_UnregisterHandles_two_interfaces)
{
    test_DT_InterfaceList_UnregisterHandles(2);
}

TEST_FUNCTION(DT_InterfaceList_UnregisterHandles_three_interfaces)
{
    test_DT_InterfaceList_UnregisterHandles(3);
}

TEST_FUNCTION(DT_InterfaceList_UnregisterHandles_NULL_handle_fails)
{
    //act
    DT_InterfaceList_UnbindInterfaces(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_RegistrationCompleteCallback
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceList_RegistrationCompleteCallback(int numInterfaces, DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus)
{
    for (int i = 0; i < numInterfaces; i++)
    {
        STRICT_EXPECTED_CALL(DT_InterfaceClient_RegistrationCompleteCallback(IGNORED_PTR_ARG, dtInterfaceStatus));
    }
}

static void test_DT_InterfaceList_RegistrationCompleteCallback(int numInterfaces, DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus)
{
    // arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(numInterfaces);
    set_expected_calls_for_DT_InterfaceList_RegistrationCompleteCallback(numInterfaces, dtInterfaceStatus);

    // act
    DT_InterfaceList_RegistrationCompleteCallback(h, dtInterfaceStatus);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_RegistrationCompleteCallback_one_interface_ok)
{
    test_DT_InterfaceList_RegistrationCompleteCallback(1, DIGITALTWIN_CLIENT_OK);
}

TEST_FUNCTION(DT_InterfaceList_RegistrationCompleteCallback_two_interfaces_ok)
{
    test_DT_InterfaceList_RegistrationCompleteCallback(2, DIGITALTWIN_CLIENT_OK);
}

TEST_FUNCTION(DT_InterfaceList_RegistrationCompleteCallback_three_interfaces_ok)
{
    test_DT_InterfaceList_RegistrationCompleteCallback(3, DIGITALTWIN_CLIENT_OK);
}

TEST_FUNCTION(DT_InterfaceList_RegistrationCompleteCallback_one_interface_out_of_memory_fail)
{
    test_DT_InterfaceList_RegistrationCompleteCallback(1, DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY);
}

TEST_FUNCTION(DT_InterfaceList_RegistrationCompleteCallback_two_interfaces_timeout_fail)
{
    test_DT_InterfaceList_RegistrationCompleteCallback(2, DIGITALTWIN_CLIENT_ERROR_TIMEOUT);
}

TEST_FUNCTION(DT_InterfaceList_RegistrationCompleteCallback_three_interfaces_timeout_fail)
{
    test_DT_InterfaceList_RegistrationCompleteCallback(3, DIGITALTWIN_CLIENT_ERROR);
}


TEST_FUNCTION(DT_InterfaceList_RegistrationCompleteCallback_NULL_handle_fails)
{
    // act
    DT_InterfaceList_RegistrationCompleteCallback(NULL, DIGITALTWIN_CLIENT_OK);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_InvokeCommand
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceList_InvokeCommand(int numInterfaces, int matchedInterfaceIndex, const unsigned char* testResponseData, size_t testResponseDataLen, int testCallbackResult)
{
    for (int i = 0; i < numInterfaces; i++)
    {
        if (i != matchedInterfaceIndex)
        {
            STRICT_EXPECTED_CALL(DT_InterfaceClient_InvokeCommandIfSupported(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).
                SetReturn(DT_COMMAND_PROCESSOR_NOT_APPLICABLE);
        }
        else
        {
            // When product code finds a match, we break out.
            STRICT_EXPECTED_CALL(DT_InterfaceClient_InvokeCommandIfSupported(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).
                CopyOutArgumentBuffer(5, &testResponseData, sizeof(testResponseData)).
                CopyOutArgumentBuffer(6, &testResponseDataLen, sizeof(testResponseDataLen)).
                CopyOutArgumentBuffer(7, &testCallbackResult, sizeof(testCallbackResult)).
                SetReturn(DT_COMMAND_PROCESSOR_PROCESSED);
            break;
        }
    }
}

static void test_DT_InterfaceList_InvokeCommand(int numInterfacesRegistered, int matchedInterfaceIndex, const unsigned char* testResponseData, size_t testResponseLen, int testCallbackResult)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(numInterfacesRegistered);
    set_expected_calls_for_DT_InterfaceList_InvokeCommand(numInterfacesRegistered, matchedInterfaceIndex, testResponseData, testResponseLen, testCallbackResult);

    unsigned char* receivedResponseData = NULL;
    size_t receivedResponseLen = 0x1234;
    int receivedResult = 0x5678;

    //act
    DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceList_InvokeCommand(h, dtTestCommandName, (const unsigned char*)dtTestCommandPayload, dtTestCommandPayloadLen, &receivedResponseData, &receivedResponseLen, &receivedResult);

    //assert
    if (matchedInterfaceIndex <= numInterfacesRegistered)
    {
        // We should have found a match.  Make sure that we're getting correct data plumbed up
        ASSERT_ARE_EQUAL(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_PROCESSED, result);
        ASSERT_ARE_EQUAL(size_t, testResponseLen, receivedResponseLen);
        ASSERT_ARE_EQUAL(int, testCallbackResult, receivedResult);
        ASSERT_ARE_EQUAL(char_ptr, testResponseData, receivedResponseData);
    }
    else
    {
        // No match was found.  Make sure initial data was not modified.
        ASSERT_ARE_EQUAL(int, (int)DT_COMMAND_PROCESSOR_NOT_APPLICABLE, (int)result);
        ASSERT_ARE_EQUAL(size_t, 0x1234, receivedResponseLen);
        ASSERT_ARE_EQUAL(int, 0x5678, receivedResult);
        ASSERT_ARE_EQUAL(char_ptr, NULL, receivedResponseData);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    // do not free() receivedResponseData, since for test we don't duplicate it as DT_InterfaceList itself isn't the free/malloc layer.
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_one_element_index_one_match_OK)
{
    test_DT_InterfaceList_InvokeCommand(1, 0, (const unsigned char*)dtTestCommandResponse, dtTestCommandResponseLen, 200);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_two_elements_index_one_match_OK)
{
    test_DT_InterfaceList_InvokeCommand(2, 0, (const unsigned char*)dtTestCommandResponse, dtTestCommandResponseLen, 204);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_three_elements_index_one_match_OK)
{
    test_DT_InterfaceList_InvokeCommand(3, 0, (const unsigned char*)dtTestCommandResponse, dtTestCommandResponseLen, 201);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_three_elements_index_two_match_OK)
{
    test_DT_InterfaceList_InvokeCommand(3, 1, (const unsigned char*)dtTestCommandResponse, dtTestCommandResponseLen, 210);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_three_elements_index_three_match_OK)
{
    test_DT_InterfaceList_InvokeCommand(3, 2, (const unsigned char*)dtTestCommandResponse, dtTestCommandResponseLen, 203);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_zero_elements_in_list_fail_to_match)
{
    test_DT_InterfaceList_InvokeCommand(0, dtTestInvalidMethod, NULL, 0, 404);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_one_elements_in_list_fail_to_match)
{
    test_DT_InterfaceList_InvokeCommand(1, dtTestInvalidMethod, NULL, 0, 404);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_two_elements_in_list_fail_to_match)
{
    test_DT_InterfaceList_InvokeCommand(2, dtTestInvalidMethod, NULL, 0, 404);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_three_elements_in_list_fail_to_match)
{
    test_DT_InterfaceList_InvokeCommand(3, dtTestInvalidMethod, NULL, 0, 404);
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_NULL_handle_fails)
{
    //arrange
    unsigned char* receivedResponseData = NULL;
    size_t receivedResponseLen = 0x1234;
    int receivedResult = 0x5678;

    //act
    DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceList_InvokeCommand(NULL, dtTestCommandName, (const unsigned char*)dtTestCommandPayload, dtTestCommandPayloadLen, &receivedResponseData, &receivedResponseLen, &receivedResult);

    //assert
    ASSERT_ARE_EQUAL(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_ERROR, result);
    ASSERT_ARE_EQUAL(size_t, 0x1234, receivedResponseLen);
    ASSERT_ARE_EQUAL(int, 0x5678, receivedResult);
    ASSERT_ARE_EQUAL(char_ptr, NULL, receivedResponseData);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceList_InvokeCommand_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(3);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceList_InvokeCommand(3, 3, (const unsigned char*)dtTestCommandResponse, dtTestCommandResponseLen, 203);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            unsigned char* receivedResponseData = NULL;
            size_t receivedResponseLen = 0x1234;
            int receivedResult = 0x5678;
        
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceList_InvokeCommand(h, dtTestCommandName, (const unsigned char*)dtTestCommandPayload, dtTestCommandPayloadLen, &receivedResponseData, &receivedResponseLen, &receivedResult);

            //assert
            ASSERT_ARE_NOT_EQUAL(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_PROCESSED, result, "Failure in test %lu", (unsigned long)i);
            ASSERT_ARE_EQUAL(size_t, 0x1234, receivedResponseLen);
            ASSERT_ARE_EQUAL(int, 0x5678, receivedResult);
            ASSERT_ARE_EQUAL(char_ptr, NULL, receivedResponseData);
        }
    }

    //cleanup
    DT_InterfaceList_Destroy(h);
    umock_c_negative_tests_deinit();
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_CreateRegistrationMessage
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceList_CreateRegistrationMessage(int numInterfacesToRegister)
{
    int i;

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_dotset_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    for (i = 0; i < numInterfacesToRegister; i++)
    {
        STRICT_EXPECTED_CALL(DT_InterfaceClient_GetInterfaceId(IGNORED_PTR_ARG)).SetReturn(dtTestInterfaceIds[i]);
        STRICT_EXPECTED_CALL(DT_InterfaceClient_GetInterfaceInstanceName(IGNORED_PTR_ARG)).SetReturn(dtTestInterfaceInstanceNames[i]);
        STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(json_object_dotset_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DT_InterfaceClient_CreateTelemetryMessage(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG)).
        CopyOutArgumentBuffer(5, &dtTestMessageHande, sizeof(dtTestMessageHande));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
}

static void test_DT_InterfaceList_CreateRegistrationMessage(int numInterfacesToRegister, const char* expectedMessageBody)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(numInterfacesToRegister);
    set_expected_calls_for_DT_InterfaceList_CreateRegistrationMessage(numInterfacesToRegister);
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    test_dtInterfaceList_ExpectedMessageBody = expectedMessageBody;
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_CreateRegistrationMessage(h, DT_TEST_CAPABILITY_MODEL_ID, &messageHandle);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(messageHandle);

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForRegistration_one_interface_succeeds)
{
    test_DT_InterfaceList_CreateRegistrationMessage(1, testDTInterfaceListRegistartionBody1Interface);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForRegistration_two_interfaces_succeeds)
{
    test_DT_InterfaceList_CreateRegistrationMessage(2, testDTInterfaceListRegistartionBody2Interfaces);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForRegistration_three_interfaces_succeeds)
{
    test_DT_InterfaceList_CreateRegistrationMessage(3, testDTInterfaceListRegistartionBody3Interfaces);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForRegistration_NULL_list_handle_fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_CreateRegistrationMessage(NULL, DT_TEST_CAPABILITY_MODEL_ID, &messageHandle);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(messageHandle);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForRegistration_NULL_device_capability_model_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(3);
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_CreateRegistrationMessage(h, NULL, &messageHandle);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(messageHandle);

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForRegistration_NULL_message_handle_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(3);
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_CreateRegistrationMessage(h, DT_TEST_CAPABILITY_MODEL_ID, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_InterfaceList_Destroy(h);
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_ProcessTwinCallbackForProperties
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceList_ProcessTwinCallbackForProperties(int numInterfaces)
{
    for (int i = 0; i < numInterfaces; i++)
    {
        STRICT_EXPECTED_CALL(DT_InterfaceClient_ProcessTwinCallback(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).CallCannotFail();
    }
}

static void test_DT_InterfaceList_ProcessTwinCallbackForProperties(int numInterfaces)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(numInterfaces);
    set_expected_calls_for_DT_InterfaceList_ProcessTwinCallbackForProperties(numInterfaces);
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessTwinCallbackForProperties(h, true, (const unsigned char*)dtTestProcessTwinJson, dtTestProcessTwinJsonLen);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForProperties_zero_interfaces)
{
    test_DT_InterfaceList_ProcessTwinCallbackForProperties(0);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForProperties_one_interface)
{
    test_DT_InterfaceList_ProcessTwinCallbackForProperties(1);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForProperties_two_interfaces)
{
    test_DT_InterfaceList_ProcessTwinCallbackForProperties(2);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallbackForProperties_three_interfaces)
{
    test_DT_InterfaceList_ProcessTwinCallbackForProperties(3);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallback_NULL_handle_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessTwinCallbackForProperties(NULL, true, (const unsigned char*)dtTestProcessTwinJson, dtTestProcessTwinJsonLen);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceList_ProcessTwinCallback_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(3);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceList_ProcessTwinCallbackForProperties(3);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessTwinCallbackForProperties(h, true, (const unsigned char*)dtTestProcessTwinJson, dtTestProcessTwinJsonLen);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanupdtTestProcessTwinJson
    DT_InterfaceList_Destroy(h);
    umock_c_negative_tests_deinit();
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_ProcessTelemetryCallback
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceList_ProcessTelemetryCallback(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle, DIGITALTWIN_CLIENT_RESULT dtSendTelemetryStatus, void* userContext)
{
    STRICT_EXPECTED_CALL(DT_InterfaceClient_ProcessTelemetryCallback(interfaceHandle, dtSendTelemetryStatus, userContext)).CallCannotFail();
}

static void test_DT_InterfaceList_ProcessTelemetryCallback(int numInterfaces, DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle, DIGITALTWIN_CLIENT_RESULT expectedResult, DIGITALTWIN_CLIENT_RESULT dtSendTelemetryStatus, void* userContext, bool expectMatch)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(numInterfaces);
    if (expectMatch)
    {
        set_expected_calls_for_DT_InterfaceList_ProcessTelemetryCallback(interfaceHandle, dtSendTelemetryStatus, userContext);
    }
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessTelemetryCallback(h, interfaceHandle, dtSendTelemetryStatus, userContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, expectedResult, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTelemetryCallback_one_interface_with_OK_succeeds)
{
    test_DT_InterfaceList_ProcessTelemetryCallback(1, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTelemetryCallback_one_interface_with_ERROR_succeeds)
{
    test_DT_InterfaceList_ProcessTelemetryCallback(1, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_ERROR, dtTestInvokeCommandUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTelemetryCallback_three_interfaces_first_handle_with_OK_succeeds)
{
    test_DT_InterfaceList_ProcessTelemetryCallback(3, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTelemetryCallback_three_interfaces_second_handle_with_OK_succeeds)
{
    test_DT_InterfaceList_ProcessTelemetryCallback(3, DT_TEST_INTERFACE_HANDLE2, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTelemetryCallback_three_interfaces_third_handle_with_OK_succeeds)
{
    test_DT_InterfaceList_ProcessTelemetryCallback(3, DT_TEST_INTERFACE_HANDLE3, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTelemetryCallback_no_interfaces_registered_fails)
{
    test_DT_InterfaceList_ProcessTelemetryCallback(0, DT_TEST_INTERFACE_NOT_REGISTERED, DIGITALTWIN_CLIENT_ERROR, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext, false);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTelemetryCallback_three_interfaces_registered_no_match_fails)
{
    test_DT_InterfaceList_ProcessTelemetryCallback(3, DT_TEST_INTERFACE_NOT_REGISTERED, DIGITALTWIN_CLIENT_ERROR, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext, false);
}

TEST_FUNCTION(DT_InterfaceList_ProcessTelemetryCallback_NULL_list_handle)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessTelemetryCallback(NULL, DT_TEST_INTERFACE_HANDLE2, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceList_ProcessTelemetryCallback_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(3);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceList_ProcessTelemetryCallback(DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessTelemetryCallback(h, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    DT_InterfaceList_Destroy(h);
    umock_c_negative_tests_deinit();
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceList_ProcessReportedPropertiesUpdateCallback
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle, DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContext)
{
    STRICT_EXPECTED_CALL(DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(interfaceHandle, dtReportedStatus, userContext)).CallCannotFail();
}

static void test_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(int numInterfaces, DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle, DIGITALTWIN_CLIENT_RESULT expectedResult, DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContext, bool expectMatch)
{
    //arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(numInterfaces);
    if (expectMatch)
    {
        set_expected_calls_for_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(interfaceHandle, dtReportedStatus, userContext);
    }
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(h, interfaceHandle, dtReportedStatus, userContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, expectedResult, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_InterfaceList_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_one_interface_with_OK_succeeds)
{
    test_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(1, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_OK, dtTestProcessReportedPropertiesUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_one_interface_with_ERROR_succeeds)
{
    test_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(1, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_ERROR, dtTestProcessReportedPropertiesUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_three_interfaces_first_handle_with_OK_succeeds)
{
    test_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(3, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_OK, dtTestProcessReportedPropertiesUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_three_interfaces_second_handle_with_OK_succeeds)
{
    test_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(3, DT_TEST_INTERFACE_HANDLE2, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_OK, dtTestProcessReportedPropertiesUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_three_interfaces_third_handle_with_OK_succeeds)
{
    test_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(3, DT_TEST_INTERFACE_HANDLE3, DIGITALTWIN_CLIENT_OK, DIGITALTWIN_CLIENT_OK, dtTestProcessReportedPropertiesUserContext, true);
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_no_interfaces_registered_fails)
{
    test_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(0, DT_TEST_INTERFACE_NOT_REGISTERED, DIGITALTWIN_CLIENT_ERROR, DIGITALTWIN_CLIENT_OK, dtTestProcessReportedPropertiesUserContext, false);
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_three_interfaces_registered_no_match_fails)
{
    test_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(3, DT_TEST_INTERFACE_NOT_REGISTERED, DIGITALTWIN_CLIENT_ERROR, DIGITALTWIN_CLIENT_OK, dtTestProcessReportedPropertiesUserContext, false);
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_NULL_handle_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(NULL, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, dtTestProcessReportedPropertiesUserContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_LIST_HANDLE h = create_DT_interface_list_and_bind(3);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, dtTestProcessReportedPropertiesUserContext);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(h, DT_TEST_INTERFACE_HANDLE1, DIGITALTWIN_CLIENT_OK, dtTestInvokeCommandUserContext);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    DT_InterfaceList_Destroy(h);
    umock_c_negative_tests_deinit();
}


END_TEST_SUITE(dt_interface_list_ut)

