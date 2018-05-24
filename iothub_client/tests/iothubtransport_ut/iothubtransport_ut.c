// Copyright (c) Microsoft. All rights reserved
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

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_stdint.h"
#include "umocktypes_bool.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"

#include "iothub_transport_ll.h"

#define IOTHUB_TRANSPORT_H
#define IOTHUB_TRANSPORT_LL_H

#define ENABLE_MOCKS
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "iothub_client_core_common.h"

#include "iothub_client_core.h"
#include "internal/iothub_transport_ll_private.h"
#undef ENABLE_MOCKS

#undef IOTHUB_TRANSPORT_H
#undef IOTHUB_TRANSPORT_LL_H

#include "internal/iothubtransport.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "iothub_client_core_ll.h"
#include "internal/iothub_client_private.h"

MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe_DeviceMethod, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe_DeviceMethod, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe_DeviceTwin, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe_DeviceTwin, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_SendMessageDisposition, MESSAGE_CALLBACK_INFO*, messageData, IOTHUBMESSAGE_DISPOSITION_RESULT, disposition);
MOCKABLE_FUNCTION(, IOTHUB_PROCESS_ITEM_RESULT, FAKE_IoTHubTransport_ProcessItem, TRANSPORT_LL_HANDLE, handle, IOTHUB_IDENTITY_TYPE, item_type, IOTHUB_IDENTITY_INFO*, iothub_item);
MOCKABLE_FUNCTION(, STRING_HANDLE, FAKE_IoTHubTransport_GetHostname, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_SetOption, TRANSPORT_LL_HANDLE, handle, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Destroy, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_HANDLE, FAKE_IoTHubTransport_Register, TRANSPORT_LL_HANDLE, handle, const IOTHUB_DEVICE_CONFIG*, device, IOTHUB_CLIENT_CORE_LL_HANDLE, iotHubClientHandle, PDLIST_ENTRY, waitingToSend);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unregister, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_DoWork, TRANSPORT_LL_HANDLE, handle, IOTHUB_CLIENT_CORE_LL_HANDLE, iotHubClientHandle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_SetRetryPolicy, TRANSPORT_LL_HANDLE, handle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitinSeconds);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_GetSendStatus, TRANSPORT_LL_HANDLE, handle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_DeviceMethod_Response, IOTHUB_DEVICE_HANDLE, handle, METHOD_HANDLE, methodId, const unsigned char*, response, size_t, resp_size, int, status_response);
MOCKABLE_FUNCTION(, TRANSPORT_LL_HANDLE, FAKE_IoTHubTransport_Create, const IOTHUBTRANSPORT_CONFIG*, config);

#undef ENABLE_MOCKS

#ifdef __cplusplus
extern "C" {
#endif

    extern VECTOR_HANDLE real_VECTOR_create(size_t elementSize);
    extern VECTOR_HANDLE real_VECTOR_move(VECTOR_HANDLE handle);
    extern void real_VECTOR_destroy(VECTOR_HANDLE handle);
    extern int real_VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements);
    extern void real_VECTOR_erase(VECTOR_HANDLE handle, void* elements, size_t numElements);
    extern void real_VECTOR_clear(VECTOR_HANDLE handle);
    extern void* real_VECTOR_element(VECTOR_HANDLE handle, size_t index);
    extern void* real_VECTOR_front(VECTOR_HANDLE handle);
    extern void* real_VECTOR_back(VECTOR_HANDLE handle);
    extern void* real_VECTOR_find_if(VECTOR_HANDLE handle, PREDICATE_FUNCTION pred, const void* value);
    extern size_t real_VECTOR_size(VECTOR_HANDLE handle);

    //extern int real_mallocAndStrcpy_s(char** destination, const char* source);
    //extern int real_size_tToString(char* destination, size_t destinationSize, size_t value);

#ifdef __cplusplus
}
#endif

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

#define TEST_DEVICE_ID "theidofTheDevice"
#define TEST_DEVICE_KEY "theKeyoftheDevice"
#define TEST_IOTHUBNAME "theNameoftheIotHub"
#define TEST_IOTHUBSUFFIX "theSuffixoftheIotHubHostname"
#define TEST_AUTHORIZATIONKEY "theAuthorizationKey"
static const IOTHUB_CLIENT_CORE_HANDLE TEST_IOTHUB_CLIENT_CORE_HANDLE1 = (IOTHUB_CLIENT_CORE_HANDLE)0xDEAD;
#define TEST_IOTHUB_CLIENT_CORE_HANDLE2 (IOTHUB_CLIENT_CORE_HANDLE)0xDEAF
#define TEST_CLIENTS_LOCK_HANDLE (LOCK_HANDLE)0x4445
#define TEST_THREAD_HANDLE (THREAD_HANDLE)0x4442

static const TRANSPORT_LL_HANDLE TEST_TRANSPORT_LL_HANDLE = (TRANSPORT_LL_HANDLE)0x112233;
static const LOCK_HANDLE TEST_LOCK_HANDLE = (LOCK_HANDLE)0x4443;
static const VECTOR_HANDLE TEST_VECTOR_HANDLE = (VECTOR_HANDLE)0x4444;

#define TEST_HOSTNAME_TOKEN "HostName"
#define TEST_HOSTNAME_VALUE "theNameoftheIotHub.theSuffixoftheIotHubHostname"

#define TEST_DEVICEID_TOKEN "DeviceId"
#define TEST_DEVICEKEY_TOKEN "SharedAccessKey"
#define TEST_PROTOCOL_GATEWAY_HOST_NAME_TOKEN "GatewayHostName"

#define TEST_DEVICEMESSAGE_HANDLE (IOTHUB_MESSAGE_HANDLE)0x52
#define TEST_DEVICEMESSAGE_HANDLE_2 (IOTHUB_MESSAGE_HANDLE)0x53
#define TEST_IOTHUB_CLIENT_CORE_LL_HANDLE    (IOTHUB_CLIENT_CORE_LL_HANDLE)0x4242

#define TEST_STRING_HANDLE (STRING_HANDLE)0x46
static const char* TEST_CHAR = "TestChar";
static THREAD_START_FUNC threadFunc = NULL;
static void* threadFuncArg = NULL;
static size_t g_num_of_calls = 0;
static size_t g_how_many_dowork_calls = 0;
static TRANSPORT_HANDLE g_transport_handle = NULL;

static const TRANSPORT_PROVIDER* provideFAKE(void);

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG =
{
    provideFAKE,            /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    TEST_DEVICE_KEY,        /* const char* deviceKey;                       */
    NULL,                   /* const char* deviceSasToken                   */
    TEST_IOTHUBNAME,        /* const char* iotHubName;                      */
    TEST_IOTHUBSUFFIX,      /* const char* iotHubSuffix;                    */
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_NULL_protocol =
{
    NULL,                   /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    TEST_DEVICE_KEY,        /* const char* deviceKey;                       */
    NULL,                   /* const char* deviceSasToken                   */
    TEST_IOTHUBNAME,        /* const char* iotHubName;                      */
    TEST_IOTHUBSUFFIX,      /* const char* iotHubSuffix;                    */
};

#define FAKE_TRANSPORT_HANDLE (TRANSPORT_LL_HANDLE)0xDEAD

static const IOTHUB_CLIENT_DEVICE_CONFIG TEST_DEVICE_CONFIG =
{
    provideFAKE,
    FAKE_TRANSPORT_HANDLE,
    TEST_DEVICE_ID,
    TEST_DEVICE_KEY
};

static TRANSPORT_PROVIDER FAKE_transport_provider =
{
    FAKE_IoTHubTransport_SendMessageDisposition,
    FAKE_IoTHubTransport_Subscribe_DeviceMethod,
    FAKE_IoTHubTransport_Unsubscribe_DeviceMethod,
    FAKE_IoTHubTransport_DeviceMethod_Response,
    FAKE_IoTHubTransport_Subscribe_DeviceTwin,
    FAKE_IoTHubTransport_Unsubscribe_DeviceTwin,
    FAKE_IoTHubTransport_ProcessItem,
    FAKE_IoTHubTransport_GetHostname,
    FAKE_IoTHubTransport_SetOption,
    FAKE_IoTHubTransport_Create,
    FAKE_IoTHubTransport_Destroy,
    FAKE_IoTHubTransport_Register,
    FAKE_IoTHubTransport_Unregister,
    FAKE_IoTHubTransport_Subscribe,
    FAKE_IoTHubTransport_Unsubscribe,
    FAKE_IoTHubTransport_DoWork,
    FAKE_IoTHubTransport_SetRetryPolicy,
    FAKE_IoTHubTransport_GetSendStatus
};

static const TRANSPORT_PROVIDER* provideFAKE(void)
{
    return &FAKE_transport_provider; /*by convention... */
}

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static size_t clientDoWork_calls = 0;
static void clientDoWork(void* clientHandle)
{
    (void)clientHandle;
    clientDoWork_calls++;
}

static LOCK_HANDLE my_Lock_Init(void)
{
    return (LOCK_HANDLE)my_gballoc_malloc(1);
}

static LOCK_RESULT my_Lock_Deinit(LOCK_HANDLE handle)
{
    my_gballoc_free(handle);
    return LOCK_OK;
}

static VECTOR_HANDLE my_VECTOR_create(size_t elementSize)
{
    (void)elementSize;
    return (VECTOR_HANDLE)my_gballoc_malloc(1);
}

static void my_VECTOR_destroy(VECTOR_HANDLE handle)
{
    my_gballoc_free(handle);
}

static THREADAPI_RESULT my_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    *threadHandle = TEST_THREAD_HANDLE;
    threadFunc = func;
    threadFuncArg = arg;
    return THREADAPI_OK;
}

static int my_VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements)
{
    (void)handle;
    (void)elements;
    (void)numElements;
    return 0;
}

static void my_ThreadAPI_Sleep(unsigned int milliseconds)
{
    (void)milliseconds;
}

static void my_FAKE_IoTHubTransport_DoWork(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle)
{
    (void)iotHubClientHandle;
    (void)handle;
    if ((g_transport_handle != NULL) && (g_num_of_calls >= g_how_many_dowork_calls))
    {
        (void)IoTHubTransport_SignalEndWorkerThread(g_transport_handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1);
    }
    g_num_of_calls++;
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(iothubtransport_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);
    (void)umocktypes_bool_register_types();
    (void)umocktypes_stdint_register_types();

    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PREDICATE_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_RESULT, int);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_Create, TEST_TRANSPORT_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_DoWork, my_FAKE_IoTHubTransport_DoWork);

    REGISTER_GLOBAL_MOCK_HOOK(Lock_Init, my_Lock_Init);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Init, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Lock_Deinit, my_Lock_Deinit);
    REGISTER_GLOBAL_MOCK_RETURN(Lock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, real_VECTOR_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_move, real_VECTOR_move);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_move, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, real_VECTOR_push_back);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_push_back, __FAILURE__);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, real_VECTOR_element);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_element, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_find_if, real_VECTOR_find_if);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_find_if, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_erase, real_VECTOR_erase);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_clear, real_VECTOR_clear);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, real_VECTOR_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, real_VECTOR_size);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Join, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Sleep, my_ThreadAPI_Sleep);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    umock_c_reset_all_calls();

    clientDoWork_calls = 0;
    threadFunc = NULL;
    threadFuncArg = NULL;
    g_num_of_calls = 0;
    g_how_many_dowork_calls = 0;
    g_transport_handle = NULL;

}

TEST_FUNCTION_CLEANUP(method_cleanup)
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

static void setup_IoTHubTransport_Create(void)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Init());
    STRICT_EXPECTED_CALL(Lock_Init());
    STRICT_EXPECTED_CALL(VECTOR_create(IGNORED_NUM_ARG));
}

TEST_FUNCTION(IoTHubTransport_Create_provider_NULL_fail)
{
    TRANSPORT_HANDLE handle = NULL;

    //arrange

    //act
    handle = IoTHubTransport_Create(NULL, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_Create_iothub_name_NULL_fail)
{
    TRANSPORT_HANDLE handle = NULL;

    //arrange

    //act
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, NULL, TEST_CONFIG.iotHubSuffix);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_Create_iothubsuffix_NULL_fail)
{
    TRANSPORT_HANDLE handle = NULL;

    //arrange

    //act
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, NULL);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_Create_success)
{
    TRANSPORT_HANDLE handle = NULL;

    //arrange
    setup_IoTHubTransport_Create();

    //act
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);

    //assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

//Tests_SRS_IOTHUBTRANSPORT_17_009: [ IoTHubTransport_Create shall clean up any resources it creates if the function does not succeed. ]
//Tests_SRS_IOTHUBTRANSPORT_17_039: [ If the Vector creation fails, IoTHubTransport_Create shall return NULL. ]
TEST_FUNCTION(IoTHubTransport_Create_fails)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_IoTHubTransport_Create();

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubTransport_Create failure in test %zu/%zu", index, count);

        TRANSPORT_HANDLE handle = NULL;

        //act
        handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);

        //assert
        ASSERT_IS_NULL_WITH_MSG(handle, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_Destroy_handle_NULL_fail)
{
    //arrange

    //act
    IoTHubTransport_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_Destroy_success)
{
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Destroy(TEST_TRANSPORT_LL_HANDLE));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubTransport_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_Destroy_worker_thread_success)
{
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Destroy(TEST_TRANSPORT_LL_HANDLE));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubTransport_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_Destroy_fails)
{
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).SetReturn(LOCK_ERROR);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Destroy(TEST_TRANSPORT_LL_HANDLE));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubTransport_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}
TEST_FUNCTION(IoTHubTransport_StartWorkerThread_handle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_StartWorkerThread(NULL, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_StartWorkerThread_core_handle_NULL_fail)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_StartWorkerThread(handle, NULL, clientDoWork);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_StartWorkerThread_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, handle));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_StartWorkerThread_client_call_twice_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, TEST_IOTHUB_CLIENT_CORE_HANDLE1));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_StartWorkerThread_client_call_new_client_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, TEST_IOTHUB_CLIENT_CORE_HANDLE2));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE2, clientDoWork);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_GetLLTransport_handle_NULL_fail)
{
    //act
    TRANSPORT_LL_HANDLE ll_transport = IoTHubTransport_GetLLTransport(NULL);

    //assert
    ASSERT_IS_NULL(ll_transport);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_GetLLTransport_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    umock_c_reset_all_calls();

    //act
    TRANSPORT_LL_HANDLE ll_transport = IoTHubTransport_GetLLTransport(handle);

    //assert
    ASSERT_IS_NOT_NULL(ll_transport);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_SignalEndWorkerThread_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    //act
    bool result = IoTHubTransport_SignalEndWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_SignalEndWorkerThread_2_clients_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE2, clientDoWork);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, TEST_IOTHUB_CLIENT_CORE_HANDLE1));
    STRICT_EXPECTED_CALL(VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    //act
    bool result = IoTHubTransport_SignalEndWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    (void)IoTHubTransport_SignalEndWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE2);
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_GetLock_handle_NULL_fail)
{
    //arrange

    //act
    LOCK_HANDLE lock = IoTHubTransport_GetLock(NULL);

    //assert
    ASSERT_IS_NULL(lock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_GetLock_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    umock_c_reset_all_calls();

    //act
    LOCK_HANDLE lock = IoTHubTransport_GetLock(handle);

    //assert
    ASSERT_IS_NOT_NULL(lock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_worker_thread_runs_every_1_ms)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);
    g_transport_handle = handle;
    umock_c_reset_all_calls();

    g_how_many_dowork_calls = 2;

    for (size_t index = 0; index < g_how_many_dowork_calls+1; index++)
    {
        STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (index == g_how_many_dowork_calls)
        {
            // For stopping the threading
            STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
            STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
        if (index != g_how_many_dowork_calls)
        {
            STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        }
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
    }
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(IGNORED_NUM_ARG));

    //act
    threadFunc(threadFuncArg);

    //assert
    //ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    (void)IoTHubTransport_SignalEndWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1);
    (void)IoTHubTransport_SignalEndWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE2);
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_JoinWorkerThread_handle_NULL_fail)
{
    //arrange

    //act
    IoTHubTransport_JoinWorkerThread(NULL, TEST_IOTHUB_CLIENT_CORE_HANDLE1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_JoinWorkerThread_client_NULL_fail)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    umock_c_reset_all_calls();

    //act
    IoTHubTransport_JoinWorkerThread(handle, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_JoinWorkerThread_no_workers_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    umock_c_reset_all_calls();

    //act
    IoTHubTransport_JoinWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_JoinWorkerThread_success)
{
    //arrange
    TRANSPORT_HANDLE handle = NULL;
    handle = IoTHubTransport_Create(TEST_CONFIG.protocol, TEST_CONFIG.iotHubName, TEST_CONFIG.iotHubSuffix);
    (void)IoTHubTransport_StartWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1, clientDoWork);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    IoTHubTransport_JoinWorkerThread(handle, TEST_IOTHUB_CLIENT_CORE_HANDLE1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_Destroy(handle);
}

END_TEST_SUITE(iothubtransport_ut)
