// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

static void* my_gballoc_malloc(size_t size)
{
    return (void*)malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/agenttime.h"
#include "parson.h"
#ifdef __cplusplus
#include <csignal>
#else
#include <signal.h>
#endif
#include "azure_prov_client/prov_device_ll_client.h"
#undef ENABLE_MOCKS

#include "azure_prov_client/prov_device_client.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
#undef ENABLE_MOCKS

static TEST_MUTEX_HANDLE g_testByTest;
static THREAD_START_FUNC g_thread_func;
static void* g_thread_func_arg;

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#undef ENABLE_MOCKS

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)error_code;
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", "");
    ASSERT_FAIL(temp_str);
}

static const char* TEST_PROV_URI = "www.prov_uri.com";
static const char* TEST_SCOPE_ID = "scope_id";

static PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION = (PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION)0x1414;

static LOCK_HANDLE TEST_LOCK_HANDLE = (LOCK_HANDLE)0x1212;
static LOCK_HANDLE my_Lock_Init(void)
{
    return TEST_LOCK_HANDLE;
}

static LOCK_RESULT my_Lock_Deinit(LOCK_HANDLE handle)
{
    (void)handle;
    return LOCK_OK;
}

static LOCK_RESULT my_Lock(LOCK_HANDLE handle)
{
    (void)handle;
    return LOCK_OK;
}

static LOCK_RESULT my_Unlock(LOCK_HANDLE handle)
{
    (void)handle;
    return LOCK_OK;
}

static THREAD_HANDLE TEST_THREAD_HANDLE = (THREAD_HANDLE)0x3535;
static THREAD_HANDLE TEST_THREAD_HANDLE_FAIL = (THREAD_HANDLE)0x1111;

static THREADAPI_RESULT my_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    *threadHandle = TEST_THREAD_HANDLE;
    g_thread_func = func;
    g_thread_func_arg = arg;
    return THREADAPI_OK;
}

static THREADAPI_RESULT my_ThreadAPI_Join(THREAD_HANDLE threadHandle, int *res)
{
    if (threadHandle == TEST_THREAD_HANDLE_FAIL)
    {
        return THREADAPI_ERROR;
    }
    else
    {
        res = 0;
        return THREADAPI_OK;
    }
}

static HTTP_PROXY_OPTIONS TEST_HTTP_PROXY_OPTIONS;

PROV_DEVICE_HANDLE TEST_PROV_DEVICE_HANDLE = (PROV_DEVICE_HANDLE)0x3434;

static PROV_DEVICE_LL_HANDLE TEST_PROV_DEVICE_LL_HANDLE = (PROV_DEVICE_LL_HANDLE)0x4343;

static PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK TEST_PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK = (PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK)0x1478;

static PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK TEST_PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK = (PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK)0x1236;

static const char* TEST_CONST_CHAR_PTR = "test_string";

static void* TEST_USER_CONTEXT = (void*)0x1598;

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);

typedef struct TEST_PROV_DEVICE_INSTANCE_TAG
{
    PROV_DEVICE_LL_HANDLE ProvDeviceLLHandle;
    THREAD_HANDLE ThreadHandle;
    LOCK_HANDLE LockHandle;
    sig_atomic_t StopThread;
} TEST_PROV_DEVICE_INSTANCE;

BEGIN_TEST_SUITE(prov_device_client_ut);

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);

    REGISTER_GLOBAL_MOCK_HOOK(Lock_Init, my_Lock_Init);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Init, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(Lock_Deinit, my_Lock_Deinit);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Deinit, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(Lock, my_Lock);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(Unlock, my_Unlock);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Unlock, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Join, my_ThreadAPI_Join);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Join, THREADAPI_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(Prov_Device_LL_Create, (PROV_DEVICE_LL_HANDLE)0x1234);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Prov_Device_LL_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(Prov_Device_LL_Register_Device, PROV_DEVICE_RESULT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Prov_Device_LL_Register_Device, PROV_DEVICE_RESULT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(Prov_Device_LL_SetOption, PROV_DEVICE_RESULT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Prov_Device_LL_SetOption, PROV_DEVICE_RESULT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(Prov_Device_LL_GetVersionString, TEST_CONST_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Prov_Device_LL_GetVersionString, NULL);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    (void)Lock_Init();
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
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
{
    int result = 0;
    for (size_t index = 0; index < length; index++)
    {
        if (current_index == skip_array[index])
        {
            result = __LINE__;
            break;
        }
    }
    return result;
}

TEST_FUNCTION(Prov_Device_Create_uri_NULL_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    PROV_DEVICE_HANDLE result = Prov_Device_Create(NULL, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_Create_scope_id_NULL_fail)
{
    //arrange

    //act
    PROV_DEVICE_HANDLE result = Prov_Device_Create(TEST_PROV_URI, NULL, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_Create_protocol_NULL_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    PROV_DEVICE_HANDLE result = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_Create_succeeds)
{
    //arrange
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Lock_Init());
    STRICT_EXPECTED_CALL(Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION));

    //act
    PROV_DEVICE_HANDLE result = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    Prov_Device_Destroy(result);
}

TEST_FUNCTION(Prov_Device_Create_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Lock_Init());
    STRICT_EXPECTED_CALL(Prov_Device_LL_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION));

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Prov_Device_Create failure in test %zu/%zu", index, count);

        PROV_DEVICE_HANDLE result = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);

        // assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Prov_Device_Destroy_handle_NULL)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    Prov_Device_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_Destroy_success)
{
    //arrange
    PROV_DEVICE_HANDLE prov_device_handle = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);
    TEST_PROV_DEVICE_INSTANCE* prov_device_handle_instance = (TEST_PROV_DEVICE_INSTANCE*)prov_device_handle;
    prov_device_handle_instance->LockHandle = TEST_LOCK_HANDLE;
    prov_device_handle_instance->ProvDeviceLLHandle = TEST_PROV_DEVICE_LL_HANDLE;
    prov_device_handle_instance->ThreadHandle = TEST_THREAD_HANDLE;
    prov_device_handle_instance->StopThread = 0;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Prov_Device_LL_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    Prov_Device_Destroy(prov_device_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_Destroy_Lock_fail)
{
    PROV_DEVICE_HANDLE prov_device_handle = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);
    TEST_PROV_DEVICE_INSTANCE* prov_device_handle_instance = (TEST_PROV_DEVICE_INSTANCE*)prov_device_handle;
    prov_device_handle_instance->LockHandle = TEST_LOCK_HANDLE;
    prov_device_handle_instance->ProvDeviceLLHandle = TEST_PROV_DEVICE_LL_HANDLE;
    prov_device_handle_instance->ThreadHandle = TEST_THREAD_HANDLE;
    prov_device_handle_instance->StopThread = 0;

    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Prov_Device_LL_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1 };

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Prov_Device_Create failure in test %zu/%zu", index, count);

        Prov_Device_Destroy(prov_device_handle);

        break;
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Prov_Device_Destroy_Thread_Join_fail)
{
    //arrange
    PROV_DEVICE_HANDLE prov_device_handle = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);
    TEST_PROV_DEVICE_INSTANCE* prov_device_handle_instance = (TEST_PROV_DEVICE_INSTANCE*)prov_device_handle;
    prov_device_handle_instance->LockHandle = TEST_LOCK_HANDLE;
    prov_device_handle_instance->ProvDeviceLLHandle = TEST_PROV_DEVICE_LL_HANDLE;
    prov_device_handle_instance->ThreadHandle = TEST_THREAD_HANDLE_FAIL;
    prov_device_handle_instance->StopThread = 0;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Prov_Device_LL_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    Prov_Device_Destroy(prov_device_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_Register_Device_handle_NULL_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    PROV_DEVICE_RESULT prov_device_result = Prov_Device_Register_Device(NULL, TEST_PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, TEST_USER_CONTEXT, TEST_PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, TEST_USER_CONTEXT);

    //assert
    ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_device_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_Register_Device_callback_NULL_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    PROV_DEVICE_RESULT prov_device_result = Prov_Device_Register_Device(TEST_PROV_DEVICE_HANDLE, NULL, TEST_USER_CONTEXT, TEST_PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, TEST_USER_CONTEXT);

    //assert
    ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_device_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_Register_Device_success)
{
    //arrange
    PROV_DEVICE_HANDLE prov_device_handle = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);
    TEST_PROV_DEVICE_INSTANCE* prov_device_handle_instance = (TEST_PROV_DEVICE_INSTANCE*)prov_device_handle;
    prov_device_handle_instance->LockHandle = TEST_LOCK_HANDLE;
    prov_device_handle_instance->ProvDeviceLLHandle = TEST_PROV_DEVICE_LL_HANDLE;
    prov_device_handle_instance->ThreadHandle = TEST_THREAD_HANDLE;
    prov_device_handle_instance->StopThread = 0;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Prov_Device_LL_Register_Device(TEST_PROV_DEVICE_LL_HANDLE, TEST_PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, TEST_USER_CONTEXT, TEST_PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, TEST_USER_CONTEXT));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    //act
    PROV_DEVICE_RESULT prov_device_result = Prov_Device_Register_Device(prov_device_handle, TEST_PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, TEST_USER_CONTEXT, TEST_PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, TEST_USER_CONTEXT);

    //assert
    ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_device_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    Prov_Device_Destroy(prov_device_handle);
}

TEST_FUNCTION(Prov_Device_Register_Device_fail)
{
    //arrange
    PROV_DEVICE_HANDLE prov_device_handle = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);
    TEST_PROV_DEVICE_INSTANCE* prov_device_handle_instance = (TEST_PROV_DEVICE_INSTANCE*)prov_device_handle;
    prov_device_handle_instance->LockHandle = TEST_LOCK_HANDLE;
    prov_device_handle_instance->ProvDeviceLLHandle = TEST_PROV_DEVICE_LL_HANDLE;
    prov_device_handle_instance->ThreadHandle = NULL;
    prov_device_handle_instance->StopThread = 0;

    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Prov_Device_LL_Register_Device(TEST_PROV_DEVICE_LL_HANDLE, TEST_PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, TEST_USER_CONTEXT, TEST_PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, TEST_USER_CONTEXT));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 2 };

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Prov_Device_Register_Device failure in test %zu/%zu", index, count);

        PROV_DEVICE_RESULT result = Prov_Device_Register_Device(prov_device_handle, TEST_PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, TEST_USER_CONTEXT, TEST_PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, TEST_USER_CONTEXT);

        // assert
        ASSERT_ARE_NOT_EQUAL(PROV_DEVICE_RESULT, result, PROV_DEVICE_RESULT_OK, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();

    Prov_Device_Destroy(prov_device_handle);
}

TEST_FUNCTION(Prov_Device_SetOption_handle_NULL_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    PROV_DEVICE_RESULT prov_device_result = Prov_Device_SetOption(NULL, OPTION_HTTP_PROXY, &TEST_HTTP_PROXY_OPTIONS);

    //assert
    ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_device_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_SetOption_option_name_NULL_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    PROV_DEVICE_RESULT prov_result = Prov_Device_SetOption(TEST_PROV_DEVICE_HANDLE, NULL, &TEST_HTTP_PROXY_OPTIONS);

    //assert
    ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_SetOption_value_NULL_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    PROV_DEVICE_RESULT prov_result = Prov_Device_SetOption(TEST_PROV_DEVICE_HANDLE, OPTION_HTTP_PROXY, NULL);

    //assert
    ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_INVALID_ARG, prov_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(Prov_Device_SetOption_success)
{
    //arrange
    PROV_DEVICE_HANDLE prov_device_handle = Prov_Device_Create(TEST_PROV_URI, TEST_SCOPE_ID, TEST_PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION);
    TEST_PROV_DEVICE_INSTANCE* prov_device_handle_instance = (TEST_PROV_DEVICE_INSTANCE*)prov_device_handle;
    prov_device_handle_instance->LockHandle = TEST_LOCK_HANDLE;
    prov_device_handle_instance->ProvDeviceLLHandle = TEST_PROV_DEVICE_LL_HANDLE;
    prov_device_handle_instance->ThreadHandle = TEST_THREAD_HANDLE;
    prov_device_handle_instance->StopThread = 0;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Prov_Device_LL_SetOption(TEST_PROV_DEVICE_LL_HANDLE, OPTION_HTTP_PROXY, &TEST_HTTP_PROXY_OPTIONS));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    //act
    PROV_DEVICE_RESULT prov_result = Prov_Device_SetOption(prov_device_handle, OPTION_HTTP_PROXY, &TEST_HTTP_PROXY_OPTIONS);

    //assert
    ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    Prov_Device_Destroy(prov_device_handle);
}

TEST_FUNCTION(Prov_Device_GetVersionString_success)
{
    //arrange
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Prov_Device_LL_GetVersionString());

    //act
    const char* version_string = Prov_Device_GetVersionString();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONST_CHAR_PTR, version_string);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

END_TEST_SUITE(prov_device_client_ut)
