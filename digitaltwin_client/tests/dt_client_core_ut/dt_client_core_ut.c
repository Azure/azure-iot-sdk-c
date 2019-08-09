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

#if defined _MSC_VER
// MSC incorrectly fires this, not correctly allowing UT to test against function pointer being non-NULL
#pragma warning(disable: 4054) 
#endif


#include "testrunnerswitcher.h"

#include "../../../serializer/tests/serializer_dt_ut/real_crt_abstractions.h" // TODO: Move this file to common if this approach is good.
#include "real_strings.h"

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
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/gballoc.h"
#include "iothub_message.h"
#include "dt_interface_list.h"
#include "digitaltwin_interface_client.h"
#include "dt_interface_private.h"
#undef ENABLE_MOCKS

#include "dt_client_core.h"

static int dtTestClientExecuteCallbackResponseCode;
static const char dtTestClientExecuteCallbackResponseData[] = "Test Response Data from Mocked caller";
static const size_t dtTestClientExecuteCallbackResponseDataLen = sizeof(dtTestClientExecuteCallbackResponseData) - 1;

#define ENABLE_MOCKS
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

MOCKABLE_FUNCTION(, void, testInterfaceRegisteredCallback, DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, testSendDTTelemetryConfirmationCallback, DIGITALTWIN_CLIENT_RESULT, dtTelemetryStatus, void*, userContextCallback);

// Use a mockable layer for binding interface, so we can track these bindings called back at appropriate times with correct patterns.
MOCKABLE_FUNCTION(, int, testBindingDTClientSendEventAsync, void*, iothubClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, testBindingDTDeviceSetTwinCallback, void*, iothubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, deviceTwinCallback, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, testBindingDTDeviceSendReportedState, void*, iothubClientHandle, const unsigned char*, reportedState, size_t, size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, reportedStateCallback, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, testBindingDTDeviceSetMethodCallback, void*, iothubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, deviceMethodCallback, void*, userContextCallback);

MOCKABLE_FUNCTION(, void, testBindingDTDeviceClientDestory, void*, iothubClientHandle);
MOCKABLE_FUNCTION(, void, testBindingDTDeviceDoWork, void*, iothubClientHandle);


MOCKABLE_FUNCTION(, LOCK_HANDLE, testBindingLockInit);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingLock, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingUnlock, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingLockDeinit, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, void, testBindingThreadSleep, unsigned int, milliseconds);

MOCKABLE_FUNCTION(, void, testDTClientExecuteCallback, const DIGITALTWIN_CLIENT_COMMAND_REQUEST*, dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE*, dtClientCommandResponseContext, void*, userContextCallback);

#undef ENABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);


static void* testBindingIotHubBindingHandle = (void*)0x1221;
static void* testBindingIotHubBindingLockHandle = (void*)0x1221;

static unsigned char* testDTRegisterInterfacesAsyncContext = (unsigned char*)0x1235;
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE testDTInterfaceClientHandle = (DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1236;
static const char* testDTDeviceCapabilityModel = "http://testDeviceCapabilityModel/1.0.0";


#define DT_TEST_INTERFACE_HANDLE1 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1001)
#define DT_TEST_INTERFACE_HANDLE2 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1002)
#define DT_TEST_INTERFACE_HANDLE3 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1003)

#define DT_TEST_MESSAGE_DATA_VALUE "MessageData"

static const char* dtTestTelemetry = "TestTelemetryName";
static const unsigned char* dtTestMessageData = (const unsigned char*)DT_TEST_MESSAGE_DATA_VALUE;
static const size_t dtTestMessageDataLen = sizeof(DT_TEST_MESSAGE_DATA_VALUE) - 1;
static unsigned char* dtTestMessageContext = (unsigned char*)0x1237;
static const IOTHUB_MESSAGE_HANDLE dtTestMessageHandle = (IOTHUB_MESSAGE_HANDLE)0x1238;

static DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtTestInterfaceArray[] = { DT_TEST_INTERFACE_HANDLE1, DT_TEST_INTERFACE_HANDLE2, DT_TEST_INTERFACE_HANDLE3 };

static const char* testDTTwinPayload = "testPayload";
static const int testDTTwinPayloadLen = sizeof(testDTTwinPayload);

#define DT_TEST_COMMAND_NAME_VALUE1 "TestCommandName1"
#define DT_TEST_COMMAND_NAME_VALUE2 "TestCommandName2"
#define DT_TEST_COMMAND_NAME_VALUE3 "TestCommandName3"

static const char* dtTestCommandName1 = DT_TEST_COMMAND_NAME_VALUE1;
static const char* dtTestCommandName2 = DT_TEST_COMMAND_NAME_VALUE2;
static const char* dtTestCommandName3 = DT_TEST_COMMAND_NAME_VALUE3;

static const char* dtTestSdkInformation = "TestOnly SDKInformation Value";

static void* dtTestClientExecuteCallbackContext = (void*)0x1239;

static const char* dtTestReportedPropertyData = "Testing DigitalTwin reported property";
static const size_t dtTestReportedPropertyDataLen = sizeof(dtTestReportedPropertyData) - 1;
static void* dtTestSendReportedPropertyCallbackContext = (void*)0x1240;


static IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK dtTestTelemetryConfirmationCallback;
static void* dtTestTelemetryConfirmationCallbackContext;

static IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC dtTestDeviceMethodCallback;
static void* dtTestDeviceMethodCallbackContext;

static IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK dtTestDeviceTwinCallback;
static void* dtTestDeviceTwinContext;

static IOTHUB_CLIENT_REPORTED_STATE_CALLBACK dtTestDeviceReportedStateCallback;
static void* dtTestDeviceReportedStateContext;

static const char dtTestInputPayloadForCommand[] = "This simulates payload from DeviceMethod callback being sent to DigitalTwin Command callback";
static const size_t dtTestInputPayloadForCommandLen = sizeof(dtTestInputPayloadForCommand) - 1;

static const char dtTestCommandResponse[] = "This simulates the data a command callback returns to caller";
static const size_t dtTestCommandResponseLen= sizeof(dtTestCommandResponse) - 1;

static const char dtTestMethodNotPresentResponse[] = "\"Method not present\"";
static const char dtTestMethodInternalError[] =  "\"Internal error\"";

// Implements UT's binding interface for testing
int impl_testBindingDTClientSendEventAsync(void* iothubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    (void)iothubClientHandle;
    (void)eventMessageHandle;
    dtTestTelemetryConfirmationCallback = eventConfirmationCallback;
    dtTestTelemetryConfirmationCallbackContext = userContextCallback;
    return 0;
}

int impl_testBindingDTDeviceSetTwinCallback(void* iothubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    (void)iothubClientHandle;
    dtTestDeviceTwinCallback = deviceTwinCallback;
    dtTestDeviceTwinContext = userContextCallback;
    return 0;
}

int impl_testBindingDTDeviceSendReportedState(void* iothubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    (void)iothubClientHandle;
    (void)reportedState;
    (void)size;
    dtTestDeviceReportedStateCallback = reportedStateCallback;
    dtTestDeviceReportedStateContext = userContextCallback;
    return 0;
}

int impl_testBindingDTDeviceSetMethodCallback(void* iothubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    (void)iothubClientHandle;
    dtTestDeviceMethodCallback = deviceMethodCallback;
    dtTestDeviceMethodCallbackContext = userContextCallback;
    return 0;
}


// Implement a mock of this & Destroy so that we simulate allocating memory (and hence Valgrind would catch leaks at UT layer)
DIGITALTWIN_INTERFACE_LIST_HANDLE impl_testDigitalTwin_InterfaceList_Create(void)
{
    return (DIGITALTWIN_INTERFACE_LIST_HANDLE)my_gballoc_malloc(1);
}

void impl_testDigitalTwin_InterfaceList_Destroy(DIGITALTWIN_INTERFACE_LIST_HANDLE dtInterfaceListHandle)
{
    my_gballoc_free((void*)dtInterfaceListHandle);
}

// Need to stub out this function so it returns an actual STRING_HANDLE for product code to manipulate
DIGITALTWIN_CLIENT_RESULT impl_testDT_InterfaceClient_GetSdkInformation(STRING_HANDLE* sdkInfo)
{
    *sdkInfo = real_STRING_construct(dtTestSdkInformation);
    ASSERT_IS_NOT_NULL(*sdkInfo);
    return DIGITALTWIN_CLIENT_OK;
}


// impl_DTClientExecuteCallback is invoked when a mocked DigitalTwin command is invoked.  Add extra verification here that we get expected data and return data for caller to verify, also.
static void impl_DTClientExecuteCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponse, void* userContextCallback)
{
    ASSERT_IS_NOT_NULL(dtClientCommandRequest);
    ASSERT_ARE_EQUAL(int, DIGITALTWIN_CLIENT_COMMAND_REQUEST_VERSION_1, dtClientCommandRequest->version);
    ASSERT_ARE_EQUAL(char_ptr, dtClientCommandRequest->requestData, dtTestInputPayloadForCommand);
    ASSERT_ARE_EQUAL(int, dtClientCommandRequest->requestDataLen, dtTestInputPayloadForCommandLen);

    ASSERT_IS_NOT_NULL(dtClientCommandResponse);
    ASSERT_ARE_EQUAL(int, DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1, dtClientCommandResponse->version);

    ASSERT_IS_TRUE(dtTestClientExecuteCallbackContext == userContextCallback);

    // Return values so that caller can check they're set correctly up to the mocked IoThub*DeviceCallback 
    dtClientCommandResponse->status = dtTestClientExecuteCallbackResponseCode;

    ASSERT_ARE_EQUAL(int, 0, real_mallocAndStrcpy_s((char**)&dtClientCommandResponse->responseData, dtTestClientExecuteCallbackResponseData));
    dtClientCommandResponse->responseDataLen = strlen((const char*)dtClientCommandResponse->responseData);
}

LOCK_HANDLE impl_testBindingLockInit(void)
{
    return (LOCK_HANDLE)my_gballoc_malloc(1);
}

LOCK_RESULT impl_testBindingLockDeinit(LOCK_HANDLE bindingLock)
{
    my_gballoc_free((void*)bindingLock);
    return LOCK_OK;
}

static void impl_testInterfaceRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userContextCallback)
{
    (void)dtInterfaceStatus;
    (void)userContextCallback;
}

static void impl_testSendDTTelemetryConfirmationCallback(DIGITALTWIN_CLIENT_RESULT dtTelemetryStatus, void* userContextCallback)
{
    (void)dtTelemetryStatus;
    (void)userContextCallback;
}

TEST_DEFINE_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);

#ifdef __cplusplus
extern "C"
{
#endif

SINGLYLINKEDLIST_HANDLE real_singlylinkedlist_create(void);
void real_singlylinkedlist_destroy(SINGLYLINKEDLIST_HANDLE list);
LIST_ITEM_HANDLE real_singlylinkedlist_add(SINGLYLINKEDLIST_HANDLE list, const void* item);
int real_singlylinkedlist_remove(SINGLYLINKEDLIST_HANDLE list, LIST_ITEM_HANDLE item_handle);
LIST_ITEM_HANDLE real_singlylinkedlist_get_head_item(SINGLYLINKEDLIST_HANDLE list);
LIST_ITEM_HANDLE real_singlylinkedlist_get_next_item(LIST_ITEM_HANDLE item_handle);
LIST_ITEM_HANDLE real_singlylinkedlist_find(SINGLYLINKEDLIST_HANDLE list, LIST_MATCH_FUNCTION match_function, const void* match_context);
const void* real_singlylinkedlist_item_get_value(LIST_ITEM_HANDLE item_handle);
int real_singlylinkedlist_foreach(SINGLYLINKEDLIST_HANDLE list, LIST_ACTION_FUNCTION action_function, const void* match_context);
int real_singlylinkedlist_remove_if(SINGLYLINKEDLIST_HANDLE list, LIST_CONDITION_FUNCTION condition_function, const void* match_context);

static DT_IOTHUB_BINDING dtIothubTestBinding;
static DT_LOCK_THREAD_BINDING dtLockThreadBinding;


#ifdef __cplusplus
}
#endif


static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

BEGIN_TEST_SUITE(dt_client_core_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ACTION_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_MATCH_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DT_CLIENT_CORE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_INTERFACE_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_COMMAND_EXECUTE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_INTERFACE_LIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_CONDITION_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(DT_COMMAND_PROCESSOR_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_create, real_singlylinkedlist_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_destroy, real_singlylinkedlist_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_add, real_singlylinkedlist_add);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_head_item, real_singlylinkedlist_get_head_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_get_head_item, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_remove, real_singlylinkedlist_remove);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_remove, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_item_get_value, real_singlylinkedlist_item_get_value);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_item_get_value, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_next_item, real_singlylinkedlist_get_next_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_get_next_item, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_find, real_singlylinkedlist_find);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_find, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_foreach, real_singlylinkedlist_foreach);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_foreach, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_remove_if, real_singlylinkedlist_remove_if);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_remove_if, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_calloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(testInterfaceRegisteredCallback, impl_testInterfaceRegisteredCallback);
    REGISTER_GLOBAL_MOCK_HOOK(testSendDTTelemetryConfirmationCallback, impl_testSendDTTelemetryConfirmationCallback);
    REGISTER_GLOBAL_MOCK_HOOK(testDTClientExecuteCallback, impl_DTClientExecuteCallback);

    REGISTER_GLOBAL_MOCK_HOOK(testBindingDTClientSendEventAsync, impl_testBindingDTClientSendEventAsync);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingDTClientSendEventAsync, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(testBindingDTDeviceSetTwinCallback, impl_testBindingDTDeviceSetTwinCallback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingDTDeviceSetTwinCallback, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(testBindingDTDeviceSendReportedState, impl_testBindingDTDeviceSendReportedState);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingDTDeviceSendReportedState, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(testBindingDTDeviceSetMethodCallback, impl_testBindingDTDeviceSetMethodCallback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingDTDeviceSetMethodCallback, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_RETURN(testBindingLock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingLock, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(testBindingUnlock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingUnlock, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(testBindingLockInit, impl_testBindingLockInit);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingLockInit, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(testBindingLockDeinit, impl_testBindingLockDeinit);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingLockDeinit, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(DigitalTwin_InterfaceList_Create, impl_testDigitalTwin_InterfaceList_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DigitalTwin_InterfaceList_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(DT_InterfaceList_Destroy, impl_testDigitalTwin_InterfaceList_Destroy);    

    REGISTER_GLOBAL_MOCK_HOOK(DT_InterfaceClient_GetSdkInformation, impl_testDT_InterfaceClient_GetSdkInformation);

    REGISTER_GLOBAL_MOCK_RETURN(DT_InterfaceClient_CheckNameValid, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_InterfaceClient_CheckNameValid, DIGITALTWIN_CLIENT_ERROR);

    REGISTER_STRING_GLOBAL_MOCK_HOOK;
}


TEST_FUNCTION_INITIALIZE(TestMethodInit)
{
    umock_c_reset_all_calls();

    // Sets up the binding table with a mocked out version for this UT
    dtIothubTestBinding.iothubClientHandle = testBindingIotHubBindingHandle;
    dtIothubTestBinding.dtClientSendEventAsync = testBindingDTClientSendEventAsync;
    dtIothubTestBinding.dtClientSetTwinCallback = testBindingDTDeviceSetTwinCallback;
    dtIothubTestBinding.dtClientSendReportedState = testBindingDTDeviceSendReportedState;
    dtIothubTestBinding.dtClientSetMethodCallback = testBindingDTDeviceSetMethodCallback;
    dtIothubTestBinding.dtClientDestroy = testBindingDTDeviceClientDestory;
    dtIothubTestBinding.dtClientDoWork = testBindingDTDeviceDoWork;
    
    dtLockThreadBinding.dtBindingLockHandle = testBindingIotHubBindingLockHandle;
    dtLockThreadBinding.dtBindingLockInit = testBindingLockInit;
    dtLockThreadBinding.dtBindingLock = testBindingLock;
    dtLockThreadBinding.dtBindingUnlock = testBindingUnlock;
    dtLockThreadBinding.dtBindingLockDeinit = testBindingLockDeinit;
    dtLockThreadBinding.dtBindingThreadSleep = testBindingThreadSleep;

    dtTestDeviceReportedStateCallback = NULL;
    dtTestDeviceReportedStateContext = NULL;
    dtTestDeviceTwinCallback = NULL;
    dtTestDeviceMethodCallback = NULL;
    dtTestDeviceMethodCallbackContext = NULL;
    dtTestTelemetryConfirmationCallback = NULL;
    dtTestTelemetryConfirmationCallbackContext = NULL;
    dtTestDeviceTwinCallback = NULL;
    dtTestDeviceTwinContext = NULL;
}


TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
}


///////////////////////////////////////////////////////////////////////////////
// DT_ClientCoreCreate
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_ClientCore_Create()
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(testBindingLockInit());
    STRICT_EXPECTED_CALL(DigitalTwin_InterfaceList_Create());
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
}

static DT_CLIENT_CORE_HANDLE allocateDT_client_core_for_test()
{
    DT_CLIENT_CORE_HANDLE h = DT_ClientCoreCreate(&dtIothubTestBinding, &dtLockThreadBinding);
    ASSERT_IS_NOT_NULL(h);
    umock_c_reset_all_calls();
    return h;
}

TEST_FUNCTION(DT_ClientCoreCreate_ok)
{
    //arrange
    set_expected_calls_for_DT_ClientCore_Create();

    //act
    DT_CLIENT_CORE_HANDLE h = DT_ClientCoreCreate(&dtIothubTestBinding, &dtLockThreadBinding);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_ClientCoreDestroy(h);
}   

TEST_FUNCTION(DT_ClientCoreCreate_NULL_iothub_binding_handle_fail)
{
    //act
    DT_CLIENT_CORE_HANDLE h = DT_ClientCoreCreate(NULL, &dtLockThreadBinding);

    //assert
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(DT_ClientCoreCreate_NULL_lock_binding_handle_fail)
{
    //act
    DT_CLIENT_CORE_HANDLE h = DT_ClientCoreCreate(&dtIothubTestBinding, NULL);

    //assert
    ASSERT_IS_NULL(h);
}


TEST_FUNCTION(DT_ClientCoreCreate_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_ClientCore_Create();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DT_CLIENT_CORE_HANDLE h = DT_ClientCoreCreate(&dtIothubTestBinding, &dtLockThreadBinding);

            //assert
            ASSERT_IS_NULL(h, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
}


///////////////////////////////////////////////////////////////////////////////
// DT_ClientCoreDestroy
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_BlockForActiveCallers()
{
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
}

// This is logic that actually deallocates clientcore memory itself, after prerequites for deletion met.
static void set_expected_calls_for_FreeClientCore()
{
    STRICT_EXPECTED_CALL(testBindingDTDeviceClientDestory(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));   

    STRICT_EXPECTED_CALL(DT_InterfaceList_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingLockDeinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_DT_ClientCore_Destroy()
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DT_InterfaceList_UnbindInterfaces(IGNORED_PTR_ARG));
    
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG));
    set_expected_calls_for_FreeClientCore();
}

TEST_FUNCTION(DT_ClientCoreDestroy_ok)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = allocateDT_client_core_for_test();
    set_expected_calls_for_DT_ClientCore_Destroy();


    //act
    DT_ClientCoreDestroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_ClientCoreDestroy_NULL_handle_ok)
{
    //act
    DT_ClientCoreDestroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

///////////////////////////////////////////////////////////////////////////////
// DT_ClientCoreDoWork
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_ClientCore_DoWork()
{
    STRICT_EXPECTED_CALL(testBindingDTDeviceDoWork(IGNORED_PTR_ARG));
}

TEST_FUNCTION(DT_ClientCoreDoWork_ok)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = allocateDT_client_core_for_test();
    set_expected_calls_for_DT_ClientCore_DoWork();

    //act
    DT_ClientCoreDoWork(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreDoWork_NULL_handle)
{
    //act
    DT_ClientCoreDoWork(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

///////////////////////////////////////////////////////////////////////////////
// DT_ClientCoreRegisterInterfacesAsync
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_ClientCore_RegisterInterfacesAsync()
{
    STRICT_EXPECTED_CALL(DT_InterfaceClient_CheckNameValid(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DT_InterfaceList_BindInterfaces(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(DT_InterfaceList_CreateRegistrationMessage(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(testBindingDTClientSendEventAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG));
}

// test_register_interfaces_init tests the first stage of interface registration, and in particular does NOT 
// invoke any of the callbacks into test framework we've provided (leaving for subsequent test helpers to process)
static DT_CLIENT_CORE_HANDLE test_register_interfaces_init(DT_CLIENT_CORE_HANDLE h, int numInterfaces)
{
    // Perform the initial registration of async interfaces
    set_expected_calls_for_DT_ClientCore_RegisterInterfacesAsync();
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreRegisterInterfacesAsync(h, testDTDeviceCapabilityModel, (numInterfaces == 0) ? NULL : dtTestInterfaceArray, numInterfaces, testInterfaceRegisteredCallback, testDTRegisterInterfacesAsyncContext);
    
    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    // registering an interface results in the product code sending a telemetry message, which for the UT case is mocked
    // to the function impl_testBindingDTClientSendEventAsync which will record the callbacks for later.
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallback);
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallbackContext);

    umock_c_reset_all_calls();
    return h;
}

// test_DTClientCore_DestroyHandleIfRegistrationPending is used by test functions where they initiate an interface registration
// but close the handle before the registration is completed.  This "just works" in product code but needs some extra
// logic to avoid leaking in UT based on which layers we mock out.
static void test_DTClientCore_DestroyHandleIfRegistrationPending(DT_CLIENT_CORE_HANDLE h)
{
    // Invoking destroy will (in product code) result in the pending telemetry message callback being invoked with a destroy.
    // Because we've mocked out that layer in UT, we perform same operation here.
    dtTestTelemetryConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, dtTestTelemetryConfirmationCallbackContext);

    DT_ClientCoreDestroy(h);
}

// The _close_before_twin_callback_ tests simulate registering an interface, but then closing the handle before
// any callbacks that the registration process initiated have completed
TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_zero_interfaces_close_before_twin_callback_ok)
{
    //act
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_init(allocateDT_client_core_for_test(), 0);
    //cleanup
    test_DTClientCore_DestroyHandleIfRegistrationPending(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_one_interface_close_before_twin_callback_ok)
{
    //act
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_init(allocateDT_client_core_for_test(), 1);
    //cleanup
    test_DTClientCore_DestroyHandleIfRegistrationPending(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_two_interfaces_close_before_twin_callback_ok)
{
    //act
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_init(allocateDT_client_core_for_test(), 2);
    //cleanup
    test_DTClientCore_DestroyHandleIfRegistrationPending(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_three_interfaces_close_before_twin_callback_ok)
{
    //act
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_init(allocateDT_client_core_for_test(), 3);

    //cleanup
    test_DTClientCore_DestroyHandleIfRegistrationPending(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_NULL_handle)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreRegisterInterfacesAsync(NULL, testDTDeviceCapabilityModel, dtTestInterfaceArray, 1, testInterfaceRegisteredCallback, testDTRegisterInterfacesAsyncContext);
    
    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);

    umock_c_reset_all_calls();
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_NULL_capabilityModel)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = allocateDT_client_core_for_test();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreRegisterInterfacesAsync(h, NULL, dtTestInterfaceArray, 1, testInterfaceRegisteredCallback, testDTRegisterInterfacesAsyncContext);
    
    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);

    //cleanup
    DT_ClientCoreDestroy(h);
}


// Tests scenario where the client core receives a registration complete message, but the user context pointer is NULL.
// This should never happen - would point to a serious bug in underlying IoTHub device sdk.  But checking guard just in case.
TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_NULL_context_ptr_in_callback)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_init(allocateDT_client_core_for_test(), 3);

    //act
    dtTestTelemetryConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_DTClientCore_DestroyHandleIfRegistrationPending(h);
}

static void set_expected_calls_for_BeginClientCoreCallbackProcessing()
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_EndClientCoreCallbackProcessing()
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG));
}


// DTClientCore processes device twin callback.  Set expected functions for this callback's invocation.
static void set_expected_calls_for_DT_ClientCore_process_twin()
{
    // We need to allocate in this function because the caller will ultimately free() this.
    char* testTwinPayloadToReturn = (char*)my_gballoc_malloc(testDTTwinPayloadLen);
    ASSERT_IS_NOT_NULL(testTwinPayloadToReturn, "Allocating test twin payload failed");

    set_expected_calls_for_BeginClientCoreCallbackProcessing();

    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingDTDeviceSendReportedState(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DT_InterfaceList_ProcessTwinCallbackForProperties(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    set_expected_calls_for_EndClientCoreCallbackProcessing();
}

// Sets expected behavior when sending the SDKInformation twin update.  Mark non-void
// functions as CallCannotFail() because this is best effort only; SDK continues on if failures occur.
static void set_expected_calls_for_SendSdkInformation()
{
    STRICT_EXPECTED_CALL(DT_InterfaceClient_GetSdkInformation(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingDTDeviceSendReportedState(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

// Sets expected behavior for reported properties.
static void set_expected_calls_for_properties_reported_callback()
{
    set_expected_calls_for_BeginClientCoreCallbackProcessing();

    STRICT_EXPECTED_CALL(testBindingDTDeviceSetMethodCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingDTDeviceSetTwinCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    set_expected_calls_for_SendSdkInformation();

    STRICT_EXPECTED_CALL(DT_InterfaceList_RegistrationCompleteCallback(IGNORED_PTR_ARG, (DIGITALTWIN_CLIENT_RESULT)IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(testInterfaceRegisteredCallback(DIGITALTWIN_CLIENT_OK, testDTRegisterInterfacesAsyncContext));

    set_expected_calls_for_EndClientCoreCallbackProcessing();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

// Tests full registration process; initial register and a simulation of service ack'ng the registration message
static DT_CLIENT_CORE_HANDLE test_register_interfaces_complete(DT_CLIENT_CORE_HANDLE h, int numInterfaces)
{
    //arrange 
    test_register_interfaces_init(h, numInterfaces);
    set_expected_calls_for_properties_reported_callback();

    //act
    // The mocked called of this UT, impl_testBindingDTClientSendEventAsync, stores out the callback that the product
    // code provides for the service to signal that the telemetry message has been accepted.  Since there is no service
    // in a UT, we perform the acknowledegment call here.
    dtTestTelemetryConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, dtTestTelemetryConfirmationCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // After the DigitalTwin Client Core product code is signaled IOTHUB_CLIENT_CONFIRMATION_OK, it should start listening for
    // device methods and twin callbacks.  Verify that this is the case.
    ASSERT_IS_NOT_NULL(dtTestDeviceMethodCallback, "Test device method callback not set");
    ASSERT_IS_NOT_NULL(dtTestDeviceMethodCallbackContext, "Test device method callback context not set");
    ASSERT_IS_NOT_NULL(dtTestDeviceTwinCallback, "Test twin callback not set");
    ASSERT_IS_NOT_NULL(dtTestDeviceTwinContext, "Test twin callback context not set");

    //cleanup
    umock_c_reset_all_calls();
    return h;
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_zero_interfaces_with_callback_complete_ok)
{
    //act
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 0);
    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_one_interface_with_callback_complete_ok)
{
    //act
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 1);
    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_two_interfaces_with_callback_complete_ok)
{
    //act
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_three_interfaces_with_callback_complete_ok)
{
    //act
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 3);
    //cleanup
    DT_ClientCoreDestroy(h);
}

// TODO: Need to figure out best way to error test this.  Potentially multiple _fail() calls per variation?

// DT_ClientCoreRegisterInterfacesAsync is invoked multiple times, which isn't supported.
TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_multiple_calls_fail)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 3);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreRegisterInterfacesAsync(h, testDTDeviceCapabilityModel, dtTestInterfaceArray, 1, testInterfaceRegisteredCallback, testDTRegisterInterfacesAsyncContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INTERFACE_ALREADY_REGISTERED, result);
    
    //cleanup
    DT_ClientCoreDestroy(h);    
}

///////////////////////////////////////////////////////////////////////////////
// DT_ClientCoreRegisterInterfacesAsync (still) - testing device method callbacks
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_DeviceMethod_callback(DT_COMMAND_PROCESSOR_RESULT commandProcessorResult, int httpStatusResultToReturn, const char* methodDataToReturn)
{
    unsigned char* testInvokeCommandResponse;
    size_t testInvokeCommandResponseLen;

    // DT_InterfaceList_InvokeCommand returns actual data, which DTClient core frees.  Simulate this here.
    if (methodDataToReturn != NULL)
    {
        ASSERT_ARE_EQUAL(int, 0, real_mallocAndStrcpy_s((char**)&testInvokeCommandResponse, methodDataToReturn));
        testInvokeCommandResponseLen = strlen(methodDataToReturn);
    }
    else
    {
        testInvokeCommandResponse = NULL;
        testInvokeCommandResponseLen = 0;
    }

    set_expected_calls_for_BeginClientCoreCallbackProcessing();
    STRICT_EXPECTED_CALL(DT_InterfaceList_InvokeCommand(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)). 
        CopyOutArgumentBuffer(5, &testInvokeCommandResponse, sizeof(testInvokeCommandResponse)).
        CopyOutArgumentBuffer(6, &testInvokeCommandResponseLen, sizeof(testInvokeCommandResponseLen)).
        CopyOutArgumentBuffer(7, &httpStatusResultToReturn, sizeof(httpStatusResultToReturn)).
        SetReturn(commandProcessorResult);

    if (commandProcessorResult != DT_COMMAND_PROCESSOR_PROCESSED)
    {
        // When interface layer doesn't process the command, the client core will allocate.
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    set_expected_calls_for_EndClientCoreCallbackProcessing();
}

static void testDT_command(DT_COMMAND_PROCESSOR_RESULT commandProcessorResult, int httpStatusResultToReturn, int httpStatusResultExpected, const char* methodDataToReturn, const char* expectedMethodCallbackData)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 3);
    set_expected_calls_for_DT_DeviceMethod_callback(commandProcessorResult, httpStatusResultToReturn, methodDataToReturn);

    //act
    //Invoke the callback into DTClientCore
    unsigned char* response = NULL;
    size_t responseSize;
    int callbackResult = dtTestDeviceMethodCallback(dtTestCommandName1, (const unsigned char*)dtTestInputPayloadForCommand, dtTestInputPayloadForCommandLen, &response, &responseSize, dtTestDeviceMethodCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(int, httpStatusResultExpected, callbackResult, "Status codes don't match");
    ASSERT_ARE_EQUAL(char_ptr, expectedMethodCallbackData, response,  "Method callback data doesn't match");
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    free(response);
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_device_method_callback_ok)
{
    testDT_command(DT_COMMAND_PROCESSOR_PROCESSED, 200, 200, dtTestCommandResponse, dtTestCommandResponse);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_device_method_error_fails)
{
    testDT_command(DT_COMMAND_PROCESSOR_ERROR, 123, 500, NULL, dtTestMethodInternalError);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_device_method_not_processed_fails)
{
    testDT_command(DT_COMMAND_PROCESSOR_NOT_APPLICABLE, 500, 501, NULL, dtTestMethodNotPresentResponse);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_already_registering_close_before_register_done_fails)
{
    //arrange
    // Starts registration process, but we're not done yet.  Twin hasn't arrived.
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_init(allocateDT_client_core_for_test(), 0);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreRegisterInterfacesAsync(h, testDTDeviceCapabilityModel, dtTestInterfaceArray, 2, testInterfaceRegisteredCallback, testDTRegisterInterfacesAsyncContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_REGISTRATION_PENDING);

    //cleanup
    test_DTClientCore_DestroyHandleIfRegistrationPending(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_already_registering_close_after_register_done_fails)
{
    //arrange
    // Analogous to DT_ClientCoreRegisterInterfacesAsync_already_registering_pre_twin_callback_fails, but we're a bit further along
    // in registration process (but not yet complete)
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_init(allocateDT_client_core_for_test(), 1);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreRegisterInterfacesAsync(h, testDTDeviceCapabilityModel, dtTestInterfaceArray, 2, testInterfaceRegisteredCallback, testDTRegisterInterfacesAsyncContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_REGISTRATION_PENDING);

    // Now make sure that when the interface registration completes (from first call), everything is OK despite the failed second attempt
    umock_c_reset_all_calls();
    set_expected_calls_for_properties_reported_callback();
    dtTestTelemetryConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, dtTestTelemetryConfirmationCallbackContext);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(dtTestDeviceMethodCallback, "Test device method callback not set");
    ASSERT_IS_NOT_NULL(dtTestDeviceMethodCallbackContext, "Test device method callback context not set");
    ASSERT_IS_NOT_NULL(dtTestDeviceTwinCallback, "Test twin callback not set");
    ASSERT_IS_NOT_NULL(dtTestDeviceTwinContext, "Test twin callback context not set");   

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreRegisterInterfacesAsync_NULL_core_handle_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreRegisterInterfacesAsync(NULL, testDTDeviceCapabilityModel, dtTestInterfaceArray, 1, testInterfaceRegisteredCallback, testDTRegisterInterfacesAsyncContext);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
}

///////////////////////////////////////////////////////////////////////////////
//DT_ClientCoreSendTelemetryAsync
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_ClientCore_SendTelemetryAsync()
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(testBindingDTClientSendEventAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG)).CallCannotFail();
}

// Simulates IoTHub* layer calling back into DigitalTwin's device callback function
static void test_call_sendtelemetryasync_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT iothubResult, DIGITALTWIN_CLIENT_RESULT dtExpectedResult, void* userContextCallback)
{
    //arrange
    umock_c_reset_all_calls();
    set_expected_calls_for_BeginClientCoreCallbackProcessing();
    STRICT_EXPECTED_CALL(DT_InterfaceList_ProcessTelemetryCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, dtExpectedResult, IGNORED_PTR_ARG));
    set_expected_calls_for_EndClientCoreCallbackProcessing();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    dtTestTelemetryConfirmationCallback(iothubResult, userContextCallback);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    umock_c_reset_all_calls();
}

// Simulates a message being sent successfully
TEST_FUNCTION(DT_ClientCoreSendTelemetryAsync_close_before_callback_complete_OK)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    set_expected_calls_for_DT_ClientCore_SendTelemetryAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreSendTelemetryAsync(h, DT_TEST_INTERFACE_HANDLE1, dtTestMessageHandle, dtTestMessageContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallback, "Send telemetry callback is NULL");
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallbackContext, "Send telemetry callback context is NULL");

    // The callback arrives indicating success.
    test_call_sendtelemetryasync_callback(IOTHUB_CLIENT_CONFIRMATION_OK, DIGITALTWIN_CLIENT_OK, dtTestTelemetryConfirmationCallbackContext);

    //cleanup
    DT_ClientCoreDestroy(h);
}


// Simulates a message being sent, but the underlying handle being destroyed before the callback returns.
TEST_FUNCTION(DT_ClientCoreSendTelemetryAsync_close_before_callback_complete_fails)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    set_expected_calls_for_DT_ClientCore_SendTelemetryAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreSendTelemetryAsync(h, DT_TEST_INTERFACE_HANDLE1, dtTestMessageHandle, dtTestMessageContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallback, "Send telemetry callback is NULL");
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallbackContext, "Send telemetry callback context is NULL");

    // Need to simulate the SendTelemetry callback for handle destruction coming in, both since this is what product
    // will do and to give everything a chance to cleanup so test doesn't have false positives in Valgrind.
    test_call_sendtelemetryasync_callback(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, DIGITALTWIN_CLIENT_ERROR_HANDLE_DESTROYED, dtTestTelemetryConfirmationCallbackContext);

    //cleanup
    DT_ClientCoreDestroy(h);
}

// Simulates a message being sent, but the underlying handle being destroyed before the callback returns.
TEST_FUNCTION(DT_ClientCoreSendTelemetryAsync_timeout_fails)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    set_expected_calls_for_DT_ClientCore_SendTelemetryAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreSendTelemetryAsync(h, DT_TEST_INTERFACE_HANDLE1, dtTestMessageHandle, dtTestMessageContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallback, "Send telemetry callback is NULL");
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallbackContext, "Send telemetry callback context is NULL");

    // 
    test_call_sendtelemetryasync_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, DIGITALTWIN_CLIENT_ERROR_TIMEOUT, dtTestTelemetryConfirmationCallbackContext);

    //cleanup
    DT_ClientCoreDestroy(h);
}


TEST_FUNCTION(DT_ClientCoreSendTelemetryAsync_error_fails)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    set_expected_calls_for_DT_ClientCore_SendTelemetryAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreSendTelemetryAsync(h, DT_TEST_INTERFACE_HANDLE1, dtTestMessageHandle, dtTestMessageContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallback, "Send telemetry callback is NULL");
    ASSERT_IS_NOT_NULL(dtTestTelemetryConfirmationCallbackContext, "Send telemetry callback context is NULL");

    // 
    test_call_sendtelemetryasync_callback(IOTHUB_CLIENT_CONFIRMATION_ERROR, DIGITALTWIN_CLIENT_ERROR, dtTestTelemetryConfirmationCallbackContext);

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreSendTelemetryAsync_clientCore_NULL_handle_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreSendTelemetryAsync(NULL, DT_TEST_INTERFACE_HANDLE1, dtTestMessageHandle, dtTestMessageContext);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_ClientCoreSendTelemetryAsync_interface_NULL_handle_fails)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreSendTelemetryAsync(h, NULL, dtTestMessageHandle, dtTestMessageContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreSendTelemetryAsync_telemetry_NULL_handle_fails)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreSendTelemetryAsync(h, DT_TEST_INTERFACE_HANDLE1, NULL, dtTestMessageContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreSendTelemetryAsync_fail)
{
    // arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_ClientCore_SendTelemetryAsync();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreSendTelemetryAsync(h, DT_TEST_INTERFACE_HANDLE1, dtTestMessageHandle, dtTestMessageContext);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    DT_ClientCoreDestroy(h);
    umock_c_negative_tests_deinit();
}



///////////////////////////////////////////////////////////////////////////////
// DT_ClientCoreReportPropertyStatusAsync
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_ClientCore_ReportPropertyStatusAsync()
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingDTDeviceSendReportedState(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG)).CallCannotFail();
}

// Simulates IoTHub* layer calling back into DigitalTwin's reported status complete callback
static void test_call_sendreportedproperties_callback(int status_code, DIGITALTWIN_CLIENT_RESULT expectedReportedStatus, void* userContextCallback)
{
    //arrange
    umock_c_reset_all_calls();
    
    set_expected_calls_for_BeginClientCoreCallbackProcessing();
    
    STRICT_EXPECTED_CALL(DT_InterfaceList_ProcessReportedPropertiesUpdateCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, expectedReportedStatus, userContextCallback));
    set_expected_calls_for_EndClientCoreCallbackProcessing();
    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    dtTestDeviceReportedStateCallback(status_code, dtTestDeviceReportedStateContext);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    umock_c_reset_all_calls();
}


TEST_FUNCTION(DT_ClientCoreReportPropertyStatusAsync_ok)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    set_expected_calls_for_DT_ClientCore_ReportPropertyStatusAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreReportPropertyStatusAsync(h, DT_TEST_INTERFACE_HANDLE1, (const unsigned char*)dtTestReportedPropertyData, dtTestReportedPropertyDataLen, dtTestSendReportedPropertyCallbackContext);
    
    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    // Now, simulate callback
    test_call_sendreportedproperties_callback(200, DIGITALTWIN_CLIENT_OK, dtTestSendReportedPropertyCallbackContext);

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreReportPropertyStatusAsync_error_status_code_fail)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);
    set_expected_calls_for_DT_ClientCore_ReportPropertyStatusAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreReportPropertyStatusAsync(h, DT_TEST_INTERFACE_HANDLE1, (const unsigned char*)dtTestReportedPropertyData, dtTestReportedPropertyDataLen, dtTestSendReportedPropertyCallbackContext);
    
    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    // Now, simulate callback
    test_call_sendreportedproperties_callback(500, DIGITALTWIN_CLIENT_ERROR, dtTestSendReportedPropertyCallbackContext);

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreReportPropertyStatusAsync_clientCore_NULL_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreReportPropertyStatusAsync(NULL, DT_TEST_INTERFACE_HANDLE1, (const unsigned char*)dtTestReportedPropertyData, dtTestReportedPropertyDataLen, dtTestSendReportedPropertyCallbackContext);
    
    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);
}

TEST_FUNCTION(DT_ClientCoreReportPropertyStatusAsync_interfaceHandle_NULL_fails)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreReportPropertyStatusAsync(h, NULL, (const unsigned char*)dtTestReportedPropertyData, dtTestReportedPropertyDataLen, dtTestSendReportedPropertyCallbackContext);
    
    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreReportPropertyStatusAsync_dataToSend_NULL_fails)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreReportPropertyStatusAsync(h, DT_TEST_INTERFACE_HANDLE1, NULL, dtTestReportedPropertyDataLen, dtTestSendReportedPropertyCallbackContext);
    
    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreReportPropertyStatusAsync_dataToSendLen_0_fails)
{
    //arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreReportPropertyStatusAsync(h, DT_TEST_INTERFACE_HANDLE1, (const unsigned char*)dtTestReportedPropertyData, 0, dtTestSendReportedPropertyCallbackContext);
    
    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);

    //cleanup
    DT_ClientCoreDestroy(h);
}

TEST_FUNCTION(DT_ClientCoreReportPropertyStatusAsync_fail)
{
    // arrange
    DT_CLIENT_CORE_HANDLE h = test_register_interfaces_complete(allocateDT_client_core_for_test(), 2);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_ClientCore_ReportPropertyStatusAsync();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_ClientCoreReportPropertyStatusAsync(h, DT_TEST_INTERFACE_HANDLE1, (const unsigned char*)dtTestReportedPropertyData, dtTestReportedPropertyDataLen, dtTestSendReportedPropertyCallbackContext);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    DT_ClientCoreDestroy(h);
    umock_c_negative_tests_deinit();
}


// TODO: Add shutting down tests & other of the state checking.

END_TEST_SUITE(dt_client_core_ut)

