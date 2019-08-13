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

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"


#define ENABLE_MOCKS
#include "iothub_device_client.h"
#include "internal/dt_client_core.h"
#include "internal/dt_lock_thread_binding_impl.h"
#include "iothub_client_options.h"
#undef ENABLE_MOCKS

#include "digitaltwin_device_client.h"

TEST_DEFINE_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);

#define DT_TEST_INTERFACE_HANDLE1 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1001)
#define DT_TEST_INTERFACE_HANDLE2 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1002)
#define DT_TEST_INTERFACE_HANDLE3 ((DIGITALTWIN_INTERFACE_CLIENT_HANDLE)0x1003)

static DIGITALTWIN_INTERFACE_CLIENT_HANDLE testDTInterfacesToRegister[] = {DT_TEST_INTERFACE_HANDLE1, DT_TEST_INTERFACE_HANDLE2, DT_TEST_INTERFACE_HANDLE3 };
static const int testDTInterfacesToRegisterLen = sizeof(testDTInterfacesToRegister) / sizeof(testDTInterfacesToRegister[0]);
static void* dtTestDeviceRegisterInterfaceCallbackContext = (void*)0x1236;
static const char* testDTDeviceCapabilityModel = "http://testDeviceCapabilityModel/1.0.0";

static const IOTHUB_DEVICE_CLIENT_HANDLE testIotHubDeviceHandle = (IOTHUB_DEVICE_CLIENT_HANDLE)0x1236;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES);

// Create a custom mock function to allocate a single byte (with a corresponding free mock function).
// This will make sure when Valgrind runs, it would catch handle leaks (which it wouldn't if this was the default mock)
static DT_CLIENT_CORE_HANDLE test_DT_ClientCoreCreate(DT_IOTHUB_BINDING* iotHubBinding, DT_LOCK_THREAD_BINDING* lockThreadBinding)
{
    (void)iotHubBinding;
    (void)lockThreadBinding;
    return (DT_CLIENT_CORE_HANDLE)malloc(1);
}

static void test_DT_ClientCoreDestroy(DT_CLIENT_CORE_HANDLE dtClientCoreHandle)
{
    (void)dtClientCoreHandle;
    free(dtClientCoreHandle);
}

BEGIN_TEST_SUITE(dt_device_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(DT_CLIENT_CORE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_CLIENT_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(DT_ClientCoreCreate, test_DT_ClientCoreCreate);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_ClientCoreCreate, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(DT_ClientCoreDestroy, test_DT_ClientCoreDestroy);
    REGISTER_GLOBAL_MOCK_RETURN(DT_ClientCoreRegisterInterfacesAsync, DIGITALTWIN_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_ClientCoreRegisterInterfacesAsync, DIGITALTWIN_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubDeviceClient_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubDeviceClient_SetOption, IOTHUB_CLIENT_ERROR);
}

TEST_FUNCTION_INITIALIZE(TestMethodInit)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
}

static void set_expected_calls_for_DT_DeviceClient_CreateFromDeviceHandle()
{
    STRICT_EXPECTED_CALL(IoTHubDeviceClient_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DT_ClientCoreCreate(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_DeviceClient_CreateFromDeviceHandle
///////////////////////////////////////////////////////////////////////////////
TEST_FUNCTION(DigitalTwin_DeviceClient_CreateFromDeviceHandle_ok)
{
    // arrange
    DIGITALTWIN_DEVICE_CLIENT_HANDLE h = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    set_expected_calls_for_DT_DeviceClient_CreateFromDeviceHandle();

    //act
    result = DigitalTwin_DeviceClient_CreateFromDeviceHandle(testIotHubDeviceHandle, &h);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        
    //cleanup
    DigitalTwin_DeviceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_DeviceClient_CreateFromDeviceHandle_NULL_iothub_handle_fails)
{
    // arrange
    DIGITALTWIN_DEVICE_CLIENT_HANDLE h = NULL;
    DIGITALTWIN_CLIENT_RESULT result;

    //act
    result = DigitalTwin_DeviceClient_CreateFromDeviceHandle(NULL, &h);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(DigitalTwin_DeviceClient_CreateFromDeviceHandle_NULL_dt_handle_fails)
{
    // arrange
    DIGITALTWIN_CLIENT_RESULT result;

    //act
    result = DigitalTwin_DeviceClient_CreateFromDeviceHandle(testIotHubDeviceHandle, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
}


TEST_FUNCTION(DigitalTwin_DeviceClient_CreateFromDeviceHandle_fail)
{
    // arrange
    DIGITALTWIN_CLIENT_RESULT result;
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    DIGITALTWIN_DEVICE_CLIENT_HANDLE h = NULL;

    set_expected_calls_for_DT_DeviceClient_CreateFromDeviceHandle();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        char message[256];
        sprintf(message, "DigitalTwin_DeviceClient_CreateFromDeviceHandle failure in test %lu", (unsigned long)i);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        result = DigitalTwin_DeviceClient_CreateFromDeviceHandle(testIotHubDeviceHandle, &h);
        
        //assert
        ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, message);
        ASSERT_IS_NULL(h, message);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}


static DIGITALTWIN_DEVICE_CLIENT_HANDLE allocate_DIGITALTWIN_DEVICE_CLIENT_HANDLE_for_test()
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_DEVICE_CLIENT_HANDLE h = NULL;

    result = DigitalTwin_DeviceClient_CreateFromDeviceHandle(testIotHubDeviceHandle, &h);

    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);

    umock_c_reset_all_calls();
    return h;
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_DeviceClient_Destroy
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_DeviceClient_Destroy()
{
    STRICT_EXPECTED_CALL(DT_ClientCoreDestroy(IGNORED_PTR_ARG));
}

TEST_FUNCTION(DigitalTwin_DeviceClient_Destroy_ok)
{
    //arrange
    DIGITALTWIN_DEVICE_CLIENT_HANDLE h = allocate_DIGITALTWIN_DEVICE_CLIENT_HANDLE_for_test();
    set_expected_calls_for_DT_DeviceClient_Destroy();

    //act
    DigitalTwin_DeviceClient_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_DeviceClient_RegisterInterfacesAsync
///////////////////////////////////////////////////////////////////////////////
static void dtTestDeviceRegisterInterfaceCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userContextCallback)
{
    (void)dtInterfaceStatus;
    (void)userContextCallback;
}

static void set_expected_calls_for_DT_DeviceClient_RegisterInterfacesAsync()
{
    STRICT_EXPECTED_CALL(DT_ClientCoreRegisterInterfacesAsync(IGNORED_PTR_ARG, testDTDeviceCapabilityModel, testDTInterfacesToRegister, testDTInterfacesToRegisterLen, dtTestDeviceRegisterInterfaceCallback, dtTestDeviceRegisterInterfaceCallbackContext));
}

TEST_FUNCTION(DigitalTwin_DeviceClient_RegisterInterfacesAsync_ok)
{
    // arrange
    DIGITALTWIN_DEVICE_CLIENT_HANDLE h = allocate_DIGITALTWIN_DEVICE_CLIENT_HANDLE_for_test();
    DIGITALTWIN_CLIENT_RESULT result;

    //act
    set_expected_calls_for_DT_DeviceClient_RegisterInterfacesAsync();
    result = DigitalTwin_DeviceClient_RegisterInterfacesAsync(h, testDTDeviceCapabilityModel, testDTInterfacesToRegister, testDTInterfacesToRegisterLen, dtTestDeviceRegisterInterfaceCallback, dtTestDeviceRegisterInterfaceCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        
    //cleanup
    DigitalTwin_DeviceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_DeviceClient_RegisterInterfacesAsync_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    DIGITALTWIN_DEVICE_CLIENT_HANDLE h = allocate_DIGITALTWIN_DEVICE_CLIENT_HANDLE_for_test();
    DIGITALTWIN_CLIENT_RESULT result;

    set_expected_calls_for_DT_DeviceClient_RegisterInterfacesAsync();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        //act
        result = DigitalTwin_DeviceClient_RegisterInterfacesAsync(h, testDTDeviceCapabilityModel, testDTInterfacesToRegister, testDTInterfacesToRegisterLen, dtTestDeviceRegisterInterfaceCallback, dtTestDeviceRegisterInterfaceCallbackContext);
        
        //assert
        ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK, "DigitalTwin_DeviceClient_RegisterInterfacesAsync failure in test %lu", (unsigned long)i);
    }

    //cleanup
    umock_c_negative_tests_deinit();
    DigitalTwin_DeviceClient_Destroy(h);
}

END_TEST_SUITE(dt_device_ut)

