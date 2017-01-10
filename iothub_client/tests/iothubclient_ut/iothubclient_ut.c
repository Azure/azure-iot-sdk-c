// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "iothubtransport.h"
#undef ENABLE_MOCKS

#include "iothub_client.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/threadapi.h"

#include "iothub_client_ll.h"

MOCKABLE_FUNCTION(, void, test_event_confirmation_callback, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, userContextCallback);
MOCKABLE_FUNCTION(, IOTHUBMESSAGE_DISPOSITION_RESULT, test_message_confirmation_callback, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_device_twin_callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_connection_status_callback, IOTHUB_CLIENT_CONNECTION_STATUS, result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_report_state_callback, int, status_code, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, test_incoming_method_callback, const char*, method_name, const unsigned char*, payload, size_t, size, METHOD_HANDLE, method_id, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, test_method_callback, const char*, method_name, const unsigned char*, payload, size_t, size, unsigned char**, response, size_t*, resp_size, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_file_upload_callback, IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result, void*, userContextCallback);

#undef ENABLE_MOCKS

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

// Overloading operators for Micromock
static TEST_MUTEX_HANDLE test_serialize_mutex;
static TEST_MUTEX_HANDLE g_dllByDll;

#ifdef __cplusplus
extern "C" const size_t IoTHubClient_ThreadTerminationOffset;
#else
extern const size_t IoTHubClient_ThreadTerminationOffset;
#endif

typedef struct LOCK_TEST_INFO_TAG
{
    void* lock_taken;
} LOCK_TEST_INFO;

static LOCK_TEST_INFO g_transport_lock;

static THREAD_START_FUNC g_thread_func;
static void* g_thread_func_arg;
static IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK g_eventConfirmationCallback;
static IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK g_deviceTwinCallback;
static IOTHUB_CLIENT_REPORTED_STATE_CALLBACK g_reportedStateCallback;
static IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK g_connectionStatusCallback;
static IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK g_inboundDeviceCallback;

static void* g_userContextCallback;
static size_t g_how_thread_loops = 0;
static size_t g_thread_loop_count = 0;
static size_t g_queue_element_size = 0;
static void* g_queue_element;
static size_t g_queue_number_items = 0;

static const IOTHUB_CLIENT_TRANSPORT_PROVIDER TEST_TRANSPORT_PROVIDER = (IOTHUB_CLIENT_TRANSPORT_PROVIDER)0x1110;
static IOTHUB_CLIENT_LL_HANDLE TEST_IOTHUB_CLIENT_HANDLE = (IOTHUB_CLIENT_LL_HANDLE)0x1111;
static VECTOR_HANDLE TEST_VECTOR_HANDLE = (VECTOR_HANDLE)0x1113;
static SINGLYLINKEDLIST_HANDLE TEST_SLL_HANDLE = (SINGLYLINKEDLIST_HANDLE)0x1114;
static const IOTHUB_CLIENT_CONFIG* TEST_CLIENT_CONFIG = (IOTHUB_CLIENT_CONFIG*)0x1115;
static IOTHUB_MESSAGE_HANDLE TEST_MESSAGE_HANDLE = (IOTHUB_MESSAGE_HANDLE)0x1116;
static THREAD_HANDLE TEST_THREAD_HANDLE = (THREAD_HANDLE)0x1117;
static LIST_ITEM_HANDLE TEST_LIST_HANDLE = (LIST_ITEM_HANDLE)0x1118;
static TRANSPORT_HANDLE TEST_TRANSPORT_HANDLE = (TRANSPORT_HANDLE)0x1119;
static IOTHUB_CLIENT_DEVICE_CONFIG* TEST_CLIENT_DEVICE_CONFIG = (IOTHUB_CLIENT_DEVICE_CONFIG*)0x111A;
static METHOD_HANDLE TEST_METHOD_ID = (METHOD_HANDLE)0x111B;
static STRING_HANDLE TEST_STRING_HANDLE = (STRING_HANDLE)0x111C;
static BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x111D;

static const char* TEST_CONNECTION_STRING = "Test_connection_string";
static const char* TEST_DEVICE_ID = "theidofTheDevice";
static const char* TEST_DEVICE_KEY = "theKeyoftheDevice";
static const char* TEST_IOTHUBNAME = "theNameoftheIotHub";
static const char* TEST_DEVICE_SAS = "theSasOfTheDevice";
static const char* TEST_IOTHUBSUFFIX = "theSuffixoftheIotHubHostname";
static const char* TEST_METHOD_NAME = "method_name";
static const unsigned char* TEST_DEVICE_METHOD_RESPONSE = (const unsigned char*)0x62;
static size_t TEST_DEVICE_RESP_LENGTH = 1;
static void* CALLBACK_CONTEXT = (void*)0x1210;

#define REPORTED_STATE_STATUS_CODE      200

static LOCK_HANDLE my_Lock_Init(void)
{
    LOCK_TEST_INFO* lock_info = (LOCK_TEST_INFO*)my_gballoc_malloc(sizeof(LOCK_TEST_INFO) );
    lock_info->lock_taken = NULL;
    return (LOCK_HANDLE)lock_info;
}

static LOCK_RESULT my_Lock_Deinit(LOCK_HANDLE handle)
{
    LOCK_TEST_INFO* lock_info = (LOCK_TEST_INFO*)handle;
    my_gballoc_free(lock_info);
    return LOCK_OK;
}

static LOCK_RESULT my_Lock(LOCK_HANDLE handle)
{
    LOCK_TEST_INFO* lock_info = (LOCK_TEST_INFO*)handle;
    lock_info->lock_taken = my_gballoc_malloc(1);
    return LOCK_OK;
}

static LOCK_RESULT my_Unlock(LOCK_HANDLE handle)
{
    LOCK_TEST_INFO* lock_info = (LOCK_TEST_INFO*)handle;
    my_gballoc_free(lock_info->lock_taken);
    return LOCK_OK;
}

static LOCK_HANDLE my_IoTHubTransport_GetLock(TRANSPORT_HANDLE transportHandle)
{
    (void)transportHandle;
    return (LOCK_HANDLE)&g_transport_lock;
}

static THREADAPI_RESULT my_ThreadAPI_Join(THREAD_HANDLE threadHandle, int *res)
{
    (void*)threadHandle;
    res = 0;
    return THREADAPI_OK;
}

static THREADAPI_RESULT my_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    *threadHandle = TEST_THREAD_HANDLE;
    g_thread_func = func;
    g_thread_func_arg = arg;
    return THREADAPI_OK;
}

static void my_ThreadAPI_Sleep(unsigned int milliseconds)
{
    (void)milliseconds;
    g_thread_loop_count++;
    if ( (g_how_thread_loops > 0) && (g_how_thread_loops == g_thread_loop_count))
    {
        *(sig_atomic_t*)(((char*)g_thread_func_arg) + IoTHubClient_ThreadTerminationOffset) = 1; /*tell the thread to stop*/
    }
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_GetSendStatus(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    (void)iotHubClientHandle;
    *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_IDLE;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SendEventAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    (void)eventMessageHandle;
    g_eventConfirmationCallback = eventConfirmationCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SetDeviceTwinCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    g_deviceTwinCallback = deviceTwinCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SendReportedState(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    (void)reportedState;
    (void)size;
    g_reportedStateCallback = reportedStateCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SetConnectionStatusCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    g_connectionStatusCallback = connectionStatusCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SetDeviceMethodCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    (void)deviceMethodCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    g_inboundDeviceCallback = inboundDeviceMethodCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_GetLastMessageReceiveTime(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime)
{
    (void)iotHubClientHandle;
    *lastMessageReceiveTime = time(NULL);
    return IOTHUB_CLIENT_OK;
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    size_t l = strlen(source);
    *destination = (char*)my_gballoc_malloc(l + 1);
    strcpy(*destination, source);
    return 0;
}

static VECTOR_HANDLE my_VECTOR_create(size_t elementSize)
{
    g_queue_element_size = elementSize;
    return TEST_VECTOR_HANDLE;
}

static int my_VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements)
{
    (void)handle;
    (void)numElements;
    g_queue_element = my_gballoc_malloc(g_queue_element_size);
    memcpy(g_queue_element, elements, g_queue_element_size);
    g_queue_number_items++;
    return 0;
}

static void* my_VECTOR_element(const VECTOR_HANDLE handle, size_t index)
{
    (void)handle;
    (void)index;
    return g_queue_element;
}

static void my_VECTOR_clear(VECTOR_HANDLE handle)
{
    (void)handle;
    if (g_queue_element != NULL)
    {
        my_gballoc_free(g_queue_element);
        g_queue_element = NULL;
        g_queue_number_items = 0;
    }
}

static void my_VECTOR_destroy(VECTOR_HANDLE handle)
{
    (void)handle;
    if (g_queue_element != NULL)
    {
        my_gballoc_free(g_queue_element);
        g_queue_element = NULL;
        g_queue_number_items = 0;
    }
}

static size_t my_VECTOR_size(VECTOR_HANDLE handle)
{
    (void)handle;
    return g_queue_number_items;
}

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(iothubclient_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    umock_c_init(on_umock_c_error);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_TRANSPORT_PROVIDER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_TWIN_STATE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_PROCESS_ITEM_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_TWIN_UPDATE_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, int);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_RESULT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_CreateFromConnectionString, TEST_IOTHUB_CLIENT_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_CreateFromConnectionString, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_Create, TEST_IOTHUB_CLIENT_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_Create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_CreateWithTransport, TEST_IOTHUB_CLIENT_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_CreateWithTransport, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SendEventAsync, my_IoTHubClient_LL_SendEventAsync);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_SendEventAsync, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_GetSendStatus, my_IoTHubClient_LL_GetSendStatus);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_GetSendStatus, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_GetLastMessageReceiveTime, my_IoTHubClient_LL_GetLastMessageReceiveTime);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_GetLastMessageReceiveTime, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_SetOption, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_SetMessageCallback, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_SetMessageCallback, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SetConnectionStatusCallback, my_IoTHubClient_LL_SetConnectionStatusCallback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_SetConnectionStatusCallback, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SetDeviceTwinCallback, my_IoTHubClient_LL_SetDeviceTwinCallback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_SetDeviceTwinCallback, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SendReportedState, my_IoTHubClient_LL_SendReportedState);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_SendReportedState, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SetDeviceMethodCallback, my_IoTHubClient_LL_SetDeviceMethodCallback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_SetDeviceMethodCallback, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SetDeviceMethodCallback_Ex, my_IoTHubClient_LL_SetDeviceMethodCallback_Ex);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_SetDeviceMethodCallback_Ex, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_DeviceMethodResponse, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_DeviceMethodResponse, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_UploadToBlob, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_UploadToBlob, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_GetRetryPolicy, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_GetRetryPolicy, IOTHUB_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubTransport_GetLock, my_IoTHubTransport_GetLock);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubTransport_GetLock, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_GetLLTransport, TEST_TRANSPORT_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubTransport_GetLLTransport, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_create, TEST_BUFFER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(Lock_Init, my_Lock_Init);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Init, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Lock_Deinit, my_Lock_Deinit);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Deinit, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(Lock, my_Lock);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(Unlock, my_Unlock);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Unlock, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Sleep, my_ThreadAPI_Sleep);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Join, my_ThreadAPI_Join);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Join, THREADAPI_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, my_VECTOR_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, my_VECTOR_push_back);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_push_back, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, my_VECTOR_element);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_element, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_clear, my_VECTOR_clear);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, my_VECTOR_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, my_VECTOR_size);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_create, TEST_SLL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_get_head_item, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_add, TEST_LIST_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_SignalEndWorkerThread, true);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    TEST_MUTEX_ACQUIRE(test_serialize_mutex);
    umock_c_reset_all_calls();

    g_thread_func = NULL;
    g_thread_func_arg = NULL;
    g_userContextCallback = NULL;
    g_how_thread_loops = 0;
    g_thread_loop_count = 0;
    g_queue_element = NULL;
    g_queue_element_size = 0;
    g_queue_number_items = 0;
    
    g_eventConfirmationCallback = NULL;
    g_deviceTwinCallback = NULL;
    g_reportedStateCallback = NULL;
    g_connectionStatusCallback = NULL;
    g_inboundDeviceCallback = NULL;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
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

static void setup_create_iothub_instance(bool use_ll_create)
{
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG) );
    STRICT_EXPECTED_CALL(VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    STRICT_EXPECTED_CALL(Lock_Init());
    if (use_ll_create)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_LL_Create(TEST_CLIENT_CONFIG));
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubClient_LL_CreateFromConnectionString(TEST_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER));
    }
}

static void setup_iothubclient_createwithtransport()
{
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG) );
    STRICT_EXPECTED_CALL(VECTOR_create(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(singlylinkedlist_create());

    STRICT_EXPECTED_CALL(IoTHubTransport_GetLock(TEST_TRANSPORT_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubTransport_GetLLTransport(TEST_TRANSPORT_HANDLE));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_CreateWithTransport(IGNORED_PTR_ARG))
        .IgnoreArgument_config();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
}

static void setup_iothubclient_sendeventasync(bool use_threads)
{
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    if (use_threads)
    {
        EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SendEventAsync(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(3)
        .IgnoreArgument(4);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
}

static void setup_iothubclient_uploadtoblobasync()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a UPLOADTOBLOB_SAVED_DATA*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "someFileName.txt")) /*this is making a copy of the filename*/
        .IgnoreArgument_destination()
        .IgnoreArgument_source();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) /*this is creating a UPLOADTOBLOB_SAVED_DATA*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*this is adding UPLOADTOBLOB_SAVED_DATA to the list of UPLOADTOBLOB_SAVED_DATAs to be cleaned*/
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(Lock_Init());
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1)) /*this is the thread calling into _LL layer*/
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(test_file_upload_callback(FILE_UPLOAD_OK, (void*)1))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
}

/* Tests_SRS_IOTHUBCLIENT_12_003: [IoTHubClient_CreateFromConnectionString shall verify the input parameters and if any of them NULL then return NULL] */
TEST_FUNCTION(IoTHubClient_CreateFromConnectionString_connection_string_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_HANDLE result = IoTHubClient_CreateFromConnectionString(NULL, TEST_TRANSPORT_PROVIDER);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_12_003: [IoTHubClient_CreateFromConnectionString shall verify the input parameters and if any of them NULL then return NULL] */
TEST_FUNCTION(IoTHubClient_CreateFromConnectionString_transport_provider_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_HANDLE result = IoTHubClient_CreateFromConnectionString(TEST_CONNECTION_STRING, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_12_004: [IoTHubClient_CreateFromConnectionString shall allocate a new IoTHubClient instance.] */
/* Tests_SRS_IOTHUBCLIENT_12_005: [IoTHubClient_CreateFromConnectionString shall create a lock object to be used later for serializing IoTHubClient calls] */
/* Tests_SRS_IOTHUBCLIENT_12_006: [IoTHubClient_CreateFromConnectionString shall instantiate a new IoTHubClient_LL instance by calling IoTHubClient_LL_CreateFromConnectionString and passing the connectionString and protocol] */
/* Tests_SRS_IOTHUBCLIENT_02_059: [ IoTHubClient_CreateFromConnectionString shall create a SINGLYLINKEDLIST_HANDLE containing informations saved by IoTHubClient_UploadToBlobAsync. ]*/
TEST_FUNCTION(IoTHubClient_CreateFromConnectionString_succeeds)
{
    // arrange
    setup_create_iothub_instance(false);

    // act
    IOTHUB_CLIENT_HANDLE result = IoTHubClient_CreateFromConnectionString(TEST_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(result);
}

/* Tests_SRS_IOTHUBCLIENT_12_010: [If IoTHubClient_LL_CreateFromConnectionString fails then IoTHubClient_CreateFromConnectionString shall do clean - up and return NULL] */
/* Tests_SRS_IOTHUBCLIENT_12_011: [If the allocation failed, IoTHubClient_CreateFromConnectionString returns NULL] */
/* Tests_SRS_IOTHUBCLIENT_12_009: [If lock creation failed, IoTHubClient_CreateFromConnectionString shall do clean up and return NULL] */
TEST_FUNCTION(IoTHubClient_CreateFromConnectionString_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_create_iothub_instance(false);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_CreateFromConnectionString failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_HANDLE result = IoTHubClient_CreateFromConnectionString(TEST_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER);

        // assert
        ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_Create_client_config_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_HANDLE result = IoTHubClient_Create(NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_01_001: [IoTHubClient_Create shall allocate a new IoTHubClient instance and return a non-NULL handle to it.] */
/* Tests_SRS_IOTHUBCLIENT_01_002: [IoTHubClient_Create shall instantiate a new IoTHubClient_LL instance by calling IoTHubClient_LL_Create and passing the config argument.] */
/* Tests_SRS_IOTHUBCLIENT_01_029: [IoTHubClient_Create shall create a lock object to be used later for serializing IoTHubClient calls.] */
/* Tests_SRS_IOTHUBCLIENT_02_060: [ IoTHubClient_Create shall create a SINGLYLINKEDLIST_HANDLE that shall be used beIoTHubClient_UploadToBlobAsync. ]*/
TEST_FUNCTION(IoTHubClient_Create_client_succeed)
{
    // arrange
    setup_create_iothub_instance(true);

    // act
    IOTHUB_CLIENT_HANDLE result = IoTHubClient_Create(TEST_CLIENT_CONFIG);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(result);
}

/* Tests_SRS_IOTHUBCLIENT_01_003: [If IoTHubClient_LL_Create fails, then IoTHubClient_Create shall return NULL.] */
/* Tests_SRS_IOTHUBCLIENT_01_031: [If IoTHubClient_Create fails, all resources allocated by it shall be freed.] */
/* Tests_SRS_IOTHUBCLIENT_02_061: [ If creating the SINGLYLINKEDLIST_HANDLE fails then IoTHubClient_Create shall fail and return NULL. ]*/
/* Tests_SRS_IOTHUBCLIENT_01_030: [If creating the lock fails, then IoTHubClient_Create shall return NULL.] */
TEST_FUNCTION(IoTHubClient_Create_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_create_iothub_instance(true);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_Create failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_HANDLE result = IoTHubClient_Create(TEST_CLIENT_CONFIG);

        // assert
        ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_CreateWithTransport_transport_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_CONFIG client_config;
    client_config.deviceId = TEST_DEVICE_ID;
    client_config.deviceKey = TEST_DEVICE_KEY;
    client_config.deviceSasToken = TEST_DEVICE_SAS;
    client_config.protocol = TEST_TRANSPORT_PROVIDER;

    // act
    IOTHUB_CLIENT_HANDLE result = IoTHubClient_CreateWithTransport(NULL, &client_config);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClient_CreateWithTransport_client_config_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_HANDLE result = IoTHubClient_CreateWithTransport(TEST_TRANSPORT_HANDLE, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/*Tests_SRS_IOTHUBCLIENT_17_012: [ If the transport connection is shared, the thread shall be started by calling IoTHubTransport_StartWorkerThread. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_011: [ If the transport connection is shared, the thread shall be started by calling IoTHubTransport_StartWorkerThread. ]*/
TEST_FUNCTION(IoTHubClient_CreateWithTransport_succeed)
{
    // arrange
    setup_iothubclient_createwithtransport();

    IOTHUB_CLIENT_CONFIG client_config;
    client_config.deviceId = TEST_DEVICE_ID;
    client_config.deviceKey = TEST_DEVICE_KEY;
    client_config.deviceSasToken = TEST_DEVICE_SAS;
    client_config.protocol = TEST_TRANSPORT_PROVIDER;

    // act
    IOTHUB_CLIENT_HANDLE result = IoTHubClient_CreateWithTransport(TEST_TRANSPORT_HANDLE, &client_config);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(result);
}

/*Tests_SRS_IOTHUBCLIENT_17_001: [ IoTHubClient_CreateWithTransport shall allocate a new IoTHubClient instance and return a non-NULL handle to it. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_002: [ If allocating memory for the new IoTHubClient instance fails, then IoTHubClient_CreateWithTransport shall return NULL. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_003: [ IoTHubClient_CreateWithTransport shall call IoTHubTransport_HL_GetLLTransport on transportHandle to get lower layer transport. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_004: [ If IoTHubTransport_GetLLTransport fails, then IoTHubClient_CreateWithTransport shall return NULL. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_005: [ IoTHubClient_CreateWithTransport shall call IoTHubTransport_GetLock to get the transport lock to be used later for serializing IoTHubClient calls. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_006: [ If IoTHubTransport_GetLock fails, then IoTHubClient_CreateWithTransport shall return NULL. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_007: [ IoTHubClient_CreateWithTransport shall instantiate a new IoTHubClient_LL instance by calling IoTHubClient_LL_CreateWithTransport and passing the lower layer transport and config argument. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_008: [ If IoTHubClient_LL_CreateWithTransport fails, then IoTHubClient_Create shall return NULL. ]*/
/*Tests_SRS_IOTHUBCLIENT_17_009: [ If IoTHubClient_LL_CreateWithTransport fails, all resources allocated by it shall be freed. ]*/
/*Tests_SRS_IOTHUBCLIENT_02_073: [ IoTHubClient_CreateWithTransport shall create a SINGLYLINKEDLIST_HANDLE that shall be used by IoTHubClient_UploadToBlobAsync. ]*/
TEST_FUNCTION(IoTHubClient_CreateWithTransport_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_iothubclient_createwithtransport();

    umock_c_negative_tests_snapshot();

    IOTHUB_CLIENT_CONFIG client_config;
    client_config.deviceId = TEST_DEVICE_ID;
    client_config.deviceKey = TEST_DEVICE_KEY;
    client_config.deviceSasToken = TEST_DEVICE_SAS;
    client_config.protocol = TEST_TRANSPORT_PROVIDER;

    size_t calls_cannot_fail[] = { 7 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_CreateWithTransport failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_HANDLE result = IoTHubClient_CreateWithTransport(TEST_TRANSPORT_HANDLE, &client_config);

        // assert
        ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUBCLIENT_01_008: [IoTHubClient_Destroy shall do nothing if parameter iotHubClientHandle is NULL.] */
TEST_FUNCTION(IoTHubClient_Destroy_iothub_client_handle_NULL_fail)
{
    // arrange

    // act
    IoTHubClient_Destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_01_005: [IoTHubClient_Destroy shall free all resources associated with the iotHubClientHandle instance.] */
/* Tests_SRS_IOTHUBCLIENT_01_006: [That includes destroying the IoTHubClient_LL instance by calling IoTHubClient_LL_Destroy.] */
/* Tests_SRS_IOTHUBCLIENT_01_007: [The thread created as part of executing IoTHubClient_SendEventAsync or IoTHubClient_SetMessageCallback shall be joined.] */
/* Tests_SRS_IOTHUBCLIENT_01_032: [The lock allocated in IoTHubClient_Create shall be also freed.] */
/* Tests_SRS_IOTHUBCLIENT_02_069: [ IoTHubClient_Destroy shall free all data created by IoTHubClient_UploadToBlobAsync ]*/
/* Tests_SRS_IOTHUBCLIENT_02_072: [ All threads marked as disposable (upon completion of a file upload) shall be joined and the data structures build for them shall be freed. ]*/
/* Tests_SRS_IOTHUBCLIENT_02_043: [IoTHubClient_Destroy shall lock the serializing lock.]*/
/* Tests_SRS_IOTHUBCLIENT_02_045: [ IoTHubClient_Destroy shall unlock the serializing lock. ]*/
TEST_FUNCTION(IoTHubClient_Destroy_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubClient_LL_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubClientHandle();
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(VECTOR_size(TEST_VECTOR_HANDLE));
    STRICT_EXPECTED_CALL(VECTOR_destroy(TEST_VECTOR_HANDLE));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG) );

    // act
    IoTHubClient_Destroy(iothub_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClient_SendEventAsync_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SendEventAsync(NULL, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_01_009: [IoTHubClient_SendEventAsync shall start the worker thread if it was not previously started.] */
/* Tests_SRS_IOTHUBCLIENT_01_012: [IoTHubClient_SendEventAsync shall call IoTHubClient_LL_SendEventAsync, while passing the IoTHubClient_LL handle created by IoTHubClient_Create and the parameters eventMessageHandle, eventConfirmationCallback and userContextCallback.] */
/* Tests_SRS_IOTHUBCLIENT_01_025: [IoTHubClient_SendEventAsync shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
TEST_FUNCTION(IoTHubClient_SendEventAsync_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    setup_iothubclient_sendeventasync(true);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    my_gballoc_free(g_userContextCallback);
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_01_010: [If starting the thread fails, IoTHubClient_SendEventAsync shall return IOTHUB_CLIENT_ERROR.] */
/* Tests_SRS_IOTHUBCLIENT_01_011: [If iotHubClientHandle is NULL, IoTHubClient_SendEventAsync shall return IOTHUB_CLIENT_INVALID_ARG.] */
/* Tests_SRS_IOTHUBCLIENT_01_013: [When IoTHubClient_LL_SendEventAsync is called, IoTHubClient_SendEventAsync shall return the result of IoTHubClient_LL_SendEventAsync.] */
/* Tests_SRS_IOTHUBCLIENT_01_026: [If acquiring the lock fails, IoTHubClient_SendEventAsync shall return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_SendEventAsync_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_iothubclient_sendeventasync(false);

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_SendEventAsync failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_Destroy(iothub_handle);
    my_gballoc_free(g_userContextCallback);
}

/* Tests_SRS_IOTHUBCLIENT_07_001: [ IoTHubClient_SendEventAsync shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SendEventAsync function as a user context. ] */
TEST_FUNCTION(IoTHubClient_SendEventAsync_event_confirm_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClient_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_push_back(TEST_VECTOR_HANDLE, IGNORED_PTR_ARG, 1))
        .IgnoreArgument_elements();
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG) );

    // act
    g_eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    my_VECTOR_clear(NULL);
    IoTHubClient_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_GetSendStatus_iothub_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_STATUS iothub_status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_GetSendStatus(NULL, &iothub_status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_01_023: [If iotHubClientHandle is NULL, IoTHubClient_ GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG.] */
/* Tests_SRS_IOTHUBCLIENT_01_024: [Otherwise, IoTHubClient_GetSendStatus shall return the result of IoTHubClient_LL_GetSendStatus.] */
/* Tests_SRS_IOTHUBCLIENT_01_034: [If acquiring the lock fails, IoTHubClient_GetSendStatus shall return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_GetSendStatus_lock_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS iothub_status;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle().SetReturn(LOCK_ERROR);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_GetSendStatus(iothub_handle, &iothub_status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_01_022: [IoTHubClient_GetSendStatus shall call IoTHubClient_LL_GetSendStatus, while passing the IoTHubClient_LL handle created by IoTHubClient_Create and the parameter iotHubClientStatus.] */
/* Tests_SRS_IOTHUBCLIENT_01_033: [IoTHubClient_GetSendStatus shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
TEST_FUNCTION(IoTHubClient_GetSendStatus_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS iothub_status;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_GetSendStatus(TEST_IOTHUB_CLIENT_HANDLE, &iothub_status));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_GetSendStatus(iothub_handle, &iothub_status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_SetMessageCallback_client_handle_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetMessageCallback(NULL, test_message_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_01_014: [IoTHubClient_SetMessageCallback shall start the worker thread if it was not previously started.] */
/* Tests_SRS_IOTHUBCLIENT_01_017: [IoTHubClient_SetMessageCallback shall call IoTHubClient_LL_SetMessageCallback, while passing the IoTHubClient_LL handle created by IoTHubClient_Create and the parameters messageCallback and userContextCallback.] */
/* Tests_SRS_IOTHUBCLIENT_01_018: [When IoTHubClient_LL_SetMessageCallback is called, IoTHubClient_SetMessageCallback shall return the result of IoTHubClient_LL_SetMessageCallback.] */
/* Tests_SRS_IOTHUBCLIENT_01_027: [IoTHubClient_SetMessageCallback shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
/* Tests_SRS_IOTHUBCLIENT_25_087: [ `IoTHubClient_SetConnectionStatusCallback` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. ] */
/* Tests_SRS_IOTHUBCLIENT_25_086: [ When `IoTHubClient_LL_SetConnectionStatusCallback` is called, `IoTHubClient_SetConnectionStatusCallback` shall return the result of `IoTHubClient_LL_SetConnectionStatusCallback`.] */
TEST_FUNCTION(IoTHubClient_SetMessageCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetMessageCallback(TEST_IOTHUB_CLIENT_HANDLE, test_message_confirmation_callback, NULL));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetMessageCallback(iothub_handle, test_message_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_25_084: [ If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetConnectionStatusCallback` shall return `IOTHUB_CLIENT_INVALID_ARG`.]*/
/* Tests_SRS_IOTHUBCLIENT_01_015: [If starting the thread fails, IoTHubClient_SetMessageCallback shall return IOTHUB_CLIENT_ERROR.] */
/* Tests_SRS_IOTHUBCLIENT_01_016: [If iotHubClientHandle is NULL, IoTHubClient_SetMessageCallback shall return IOTHUB_CLIENT_INVALID_ARG.] */
/* Tests_SRS_IOTHUBCLIENT_01_028: [If acquiring the lock fails, IoTHubClient_SetMessageCallback shall return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_SetMessageCallback_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetMessageCallback(TEST_IOTHUB_CLIENT_HANDLE, test_message_confirmation_callback, NULL));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 2 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_SetMessageCallback failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_SetMessageCallback(iothub_handle, test_message_confirmation_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClient_SetConnectionStatusCallback_client_handle_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetConnectionStatusCallback(NULL, test_connection_status_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClient_SetConnectionStatusCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetConnectionStatusCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_connectionStatusCallback()
        .IgnoreArgument_userContextCallback();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetConnectionStatusCallback(iothub_handle, test_connection_status_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_SetConnectionStatusCallback_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_SetConnectionStatusCallback failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_SetConnectionStatusCallback(iothub_handle, test_connection_status_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUBCLIENT_25_076: [ If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetRetryPolicy` shall return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
TEST_FUNCTION(IoTHubClient_SetRetryPolicy_client_handle_fail)
{
    // arrange
    IOTHUB_CLIENT_RETRY_POLICY retry_policy = IOTHUB_CLIENT_RETRY_RANDOM;
    size_t retry_in_seconds = 10;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetRetryPolicy(NULL, retry_policy, retry_in_seconds);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_25_078: [ When `IoTHubClient_LL_SetRetryPolicy` is called, `IoTHubClient_SetRetryPolicy` shall return the result of `IoTHubClient_LL_SetRetryPolicy`. ]*/
TEST_FUNCTION(IoTHubClient_SetRetryPolicy_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_RETRY_POLICY retry_policy = IOTHUB_CLIENT_RETRY_RANDOM;
    size_t retry_in_seconds = 10;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetRetryPolicy(TEST_IOTHUB_CLIENT_HANDLE, retry_policy, retry_in_seconds))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetRetryPolicy(iothub_handle, retry_policy, retry_in_seconds);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_25_092: [ If `iotHubClientHandle` is `NULL`, `IoTHubClient_GetRetryPolicy` shall return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
TEST_FUNCTION(IoTHubClient_GetRetryPolicy_client_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_RETRY_POLICY retry_policy;
    size_t retry_in_seconds;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_GetRetryPolicy(NULL, &retry_policy, &retry_in_seconds);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_25_094: [ When `IoTHubClient_LL_GetRetryPolicy` is called, `IoTHubClient_GetRetryPolicy` shall return the result of `IoTHubClient_LL_GetRetryPolicy`.]*/
TEST_FUNCTION(IoTHubClient_GetRetryPolicy_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_RETRY_POLICY retry_policy;
    size_t retry_in_seconds;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_LL_GetRetryPolicy(TEST_IOTHUB_CLIENT_HANDLE, &retry_policy, &retry_in_seconds));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_GetRetryPolicy(iothub_handle, &retry_policy, &retry_in_seconds);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_GetRetryPolicy_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_RETRY_POLICY retry_policy;
    size_t retry_in_seconds;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_GetRetryPolicy(TEST_IOTHUB_CLIENT_HANDLE, &retry_policy, &retry_in_seconds));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_CreateFromConnectionString failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_GetRetryPolicy(iothub_handle, &retry_policy, &retry_in_seconds);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_01_020: [If iotHubClientHandle is NULL, IoTHubClient_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_INVALID_ARG.] */
/* Tests_SRS_IOTHUBCLIENT_01_021: [Otherwise, IoTHubClient_GetLastMessageReceiveTime shall return the result of IoTHubClient_LL_GetLastMessageReceiveTime.] */
/* Tests_SRS_IOTHUBCLIENT_01_036: [If acquiring the lock fails, IoTHubClient_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_GetLastMessageReceiveTime_client_handle_NULL_fail)
{
    // arrange
    time_t recv_time;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_GetLastMessageReceiveTime(NULL, &recv_time);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_01_019: [IoTHubClient_GetLastMessageReceiveTime shall call IoTHubClient_LL_GetLastMessageReceiveTime, while passing the IoTHubClient_LL handle created by IoTHubClient_Create and the parameter lastMessageReceiveTime.] */
/* Tests_SRS_IOTHUBCLIENT_01_035: [IoTHubClient_GetLastMessageReceiveTime shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
TEST_FUNCTION(IoTHubClient_GetLastMessageReceiveTime_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    time_t recv_time;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_GetLastMessageReceiveTime(TEST_IOTHUB_CLIENT_HANDLE, &recv_time));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_GetLastMessageReceiveTime(iothub_handle, &recv_time);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_GetLastMessageReceiveTime_failed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    time_t recv_time;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_GetLastMessageReceiveTime(TEST_IOTHUB_CLIENT_HANDLE, &recv_time));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 2 };

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_GetLastMessageReceiveTime failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_GetLastMessageReceiveTime(iothub_handle, &recv_time);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_Destroy(iothub_handle);
}

/*Tests_SRS_IOTHUBCLIENT_02_034: [If parameter iotHubClientHandle is NULL then IoTHubClient_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_SetOption_client_handle_NULL_fail)
{
    // arrange
    const char* option_name = "option_name";
    const void* option_value = (void*)0x0123;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetOption(NULL, option_name, option_value);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/*Tests_SRS_IOTHUBCLIENT_02_035: [If parameter optionName is NULL then IoTHubClient_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_SetOption_option_name_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const void* option_value = (void*)0x0123;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetOption(iothub_handle, NULL, option_value);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/*Tests_SRS_IOTHUBCLIENT_02_036: [If parameter value is NULL then IoTHubClient_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_SetOption_option_value_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const char* option_name = "option_name";

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetOption(iothub_handle, option_name, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_02_038: [If optionName doesn't match one of the options handled by this module then IoTHubClient_SetOption shall call IoTHubClient_LL_SetOption passing the same parameters and return what IoTHubClient_LL_SetOption returns.] */
/* Tests_SRS_IOTHUBCLIENT_01_041: [ IoTHubClient_SetOption shall be made thread-safe by using the lock created in IoTHubClient_Create. ] */
TEST_FUNCTION(IoTHubClient_SetOption_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const char* option_name = "option_name";
    const void* option_value = (void*)0x0123;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetOption(TEST_IOTHUB_CLIENT_HANDLE, option_name, option_value));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetOption(iothub_handle, option_name, option_value);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_02_038: [If optionName doesn't match one of the options handled by this module then IoTHubClient_SetOption shall call IoTHubClient_LL_SetOption passing the same parameters and return what IoTHubClient_LL_SetOption returns.]*/
/* Tests_SRS_IOTHUBCLIENT_01_042: [ If acquiring the lock fails, IoTHubClient_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_ERROR. ]*/
/* Tests_SRS_IOTHUBCLIENT_LL_10_007: [** `IoTHubClient_SetDeviceTwinCallback` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. ]*/
/* Tests_SRS_IOTHUBCLIENT_10_002: [** If acquiring the lock fails, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_ERROR`. ]*/
/* Tests_SRS_IOTHUBCLIENT_10_004: [** If starting the thread fails, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_ERROR`. ]*/
TEST_FUNCTION(IoTHubClient_SetOption_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    const char* option_name = "option_name";
    const void* option_value = (void*)0x0123;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetOption(TEST_IOTHUB_CLIENT_HANDLE, option_name, option_value));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 2 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_SetOption failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_SetOption(iothub_handle, option_name, option_value);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_007: [** `IoTHubClient_SetDeviceTwinCallback` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. ]*/
TEST_FUNCTION(IoTHubClient_SetDeviceTwinCallback_client_handle_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceTwinCallback(NULL, test_device_twin_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_10_005: [ IoTHubClient_SetDeviceTwinCallback shall call IoTHubClient_LL_SetDeviceTwinCallback, while passing the IoTHubClient_LL handle created by IoTHubClient_LL_Create along with the parameters iothub_ll_device_twin_callback and IOTHUB_QUEUE_CONTEXT variable. ] */
/* Tests_SRS_IOTHUBCLIENT_10_006: [ When IoTHubClient_LL_SetDeviceTwinCallback is called, IoTHubClient_SetDeviceTwinCallback shall return the result of IoTHubClient_LL_SetDeviceTwinCallback. ] */
/* Tests_SRS_IOTHUBCLIENT_10_020: [ IoTHubClient_SetDeviceTwinCallback shall be made thread-safe by using the lock created in IoTHubClient_Create. ] */
TEST_FUNCTION(IoTHubClient_SetDeviceTwinCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_deviceTwinCallback()
        .IgnoreArgument_userContextCallback();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceTwinCallback(iothub_handle, test_device_twin_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_10_002: [** If acquiring the lock fails, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_ERROR`. ]*/
/* Tests_SRS_IOTHUBCLIENT_10_004: [** If starting the thread fails, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_ERROR`. ]*/
TEST_FUNCTION(IoTHubClient_SetDeviceTwinCallback_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_deviceTwinCallback()
        .IgnoreArgument_userContextCallback();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 2, 3 };

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_SetDeviceTwinCallback failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceTwinCallback(iothub_handle, test_device_twin_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_07_002: [ IoTHubClient_SetDeviceTwinCallback shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SetDeviceTwinCallback function as a user context. ] */
TEST_FUNCTION(IoTHubClient_SetDeviceTwinCallback_device_twin_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClient_SetDeviceTwinCallback(iothub_handle, test_device_twin_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_push_back(TEST_VECTOR_HANDLE, IGNORED_PTR_ARG, 1))
        .IgnoreArgument_elements();

    // act
    g_deviceTwinCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    my_VECTOR_clear(NULL);
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_10_013: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
TEST_FUNCTION(IoTHubClient_SendReportedState_client_handle_NULL_fail)
{
    // arrange
    const unsigned char* reported_state = (const unsigned char*)0x1234;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SendReportedState(NULL, reported_state, 1, test_report_state_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_10_017: [** `IoTHubClient_SendReportedState` shall call `IoTHubClient_LL_SendReportedState`, while passing the `IoTHubClient_LL handle` created by `IoTHubClient_LL_Create` along with the parameters `reportedState`, `size`, `reportedVersion`, `lastSeenDesiredVersion`, `reportedStateCallback`, and `userContextCallback`. ]*/
TEST_FUNCTION(IoTHubClient_SendReportedState_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const unsigned char* reported_state = (const unsigned char*)0x1234;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SendReportedState(TEST_IOTHUB_CLIENT_HANDLE, reported_state, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_reportedStateCallback()
        .IgnoreArgument_userContextCallback();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
    my_gballoc_free(g_userContextCallback);
}

/* Tests_SRS_IOTHUBCLIENT_10_014: [** If acquiring the lock fails, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_ERROR`. ]*/
/* Tests_SRS_IOTHUBCLIENT_10_016: [** If starting the thread fails, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_ERROR`. ]*/
/* Tests_SRS_IOTHUBCLIENT_10_017: [** `IoTHubClient_SendReportedState` shall call `IoTHubClient_LL_SendReportedState`, while passing the `IoTHubClient_LL handle` created by `IoTHubClient_LL_Create` along with the parameters `reportedState`, `size`, `reportedVersion`, `lastSeenDesiredVersion`, `reportedStateCallback`, and `userContextCallback`. ]*/
TEST_FUNCTION(IoTHubClient_SendReportedState_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    const unsigned char* reported_state = (const unsigned char*)0x1234;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SendReportedState(TEST_IOTHUB_CLIENT_HANDLE, reported_state, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_reportedStateCallback()
        .IgnoreArgument_userContextCallback();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_SendReportedState failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUBCLIENT_07_003: [ IoTHubClient_SendReportedState shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SendReportedState function as a user context. ] */
TEST_FUNCTION(IoTHubClient_SendReportedState_report_state_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    const unsigned char* reported_state = (const unsigned char*)0x1234;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_push_back(TEST_VECTOR_HANDLE, IGNORED_PTR_ARG, 1))
        .IgnoreArgument_elements();
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG) );

    // act
    g_reportedStateCallback(REPORTED_STATE_STATUS_CODE, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    my_VECTOR_clear(NULL);
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_12_012: [ If iotHubClientHandle is NULL, IoTHubClient_SetDeviceMethodCallback shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_SetDeviceMethodCallback_iothub_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceMethodCallback(NULL, test_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_12_016: [ IoTHubClient_SetDeviceMethodCallback shall call IoTHubClient_LL_SetDeviceMethodCallback, while passing the IoTHubClient_LL_handle created by IoTHubClient_LL_Create along with the parameters deviceMethodCallback and userContextCallback. ]*/
/* Tests_SRS_IOTHUBCLIENT_12_018: [ IoTHubClient_SetDeviceMethodCallback shall be made thread - safe by using the lock created in IoTHubClient_Create. ]*/
/* Tests_SRS_IOTHUBCLIENT_12_016: [ IoTHubClient_SetDeviceMethodCallback shall call IoTHubClient_LL_SetDeviceMethodCallback, while passing the IoTHubClient_LL_handle created by IoTHubClient_LL_Create along with the parameters deviceMethodCallback and userContextCallback. ]*/
/* Tests_SRS_IOTHUBCLIENT_12_018: [ IoTHubClient_SetDeviceMethodCallback shall be made thread - safe by using the lock created in IoTHubClient_Create. ]*/
/* Tests_SRS_IOTHUBCLIENT_12_017: [ When IoTHubClient_LL_SetDeviceMethodCallback is called, IoTHubClient_SetDeviceMethodCallback shall return the result of IoTHubClient_LL_SetDeviceMethodCallback. ]*/
/* Tests_SRS_IOTHUBCLIENT_12_018: [ IoTHubClient_SetDeviceMethodCallback shall be made thread - safe by using the lock created in IoTHubClient_Create. ]*/
TEST_FUNCTION(IoTHubClient_SetDeviceMethodCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_HANDLE, test_method_callback, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceMethodCallback(iothub_handle, test_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_*SRS_IOTHUBCLIENT_12_013: [ If acquiring the lock fails, IoTHubClient_SetDeviceMethodCallback shall return IOTHUB_CLIENT_ERROR. ]*/
/* Tests_SRS_IOTHUBCLIENT_12_014: [ If the transport handle is null and the worker thread is not initialized, the thread shall be started by calling IoTHubTransport_StartWorkerThread. ]*/
/* Tests_SRS_IOTHUBCLIENT_12_015: [ If starting the thread fails, IoTHubClient_SetDeviceMethodCallback shall return IOTHUB_CLIENT_ERROR. ]*/
/* Tests_SRS_IOTHUBCLIENT_12_018: [ IoTHubClient_SetDeviceMethodCallback shall be made thread - safe by using the lock created in IoTHubClient_Create. ]*/
TEST_FUNCTION(IoTHubClient_SetDeviceMethodCallback_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_HANDLE, test_method_callback, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 2 };

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_SetDeviceMethodCallback failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceMethodCallback(iothub_handle, test_method_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_Destroy(iothub_handle);
}

/*Tests_SRS_IOTHUBCLIENT_07_001: [ If iotHubClientHandle is NULL, IoTHubClient_SetDeviceMethodCallback_Ex shall return IOTHUB_CLIENT_INVALID_ARG. ]*/ 
TEST_FUNCTION(IoTHubClient_SetDeviceMethodCallback_Ex_handle_NULL)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceMethodCallback_Ex(NULL, test_incoming_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/*Tests_SRS_IOTHUBCLIENT_07_007: [ IoTHubClient_SetDeviceMethodCallback_Ex shall be made thread-safe by using the lock created in IoTHubClient_Create. ]*/
/*Tests_SRS_IOTHUBCLIENT_07_003: [ If the transport handle is NULL and the worker thread is not initialized, the thread shall be started by calling IoTHubTransport_StartWorkerThread. ]*/
/*Tests_SRS_IOTHUBCLIENT_07_005: [ IoTHubClient_SetDeviceMethodCallback_Ex shall call IoTHubClient_LL_SetDeviceMethodCallback_Ex, while passing the IoTHubClient_LL_handle created by IoTHubClient_LL_Create along with the parameters iothub_ll_inbound_device_method_callback and IOTHUB_QUEUE_CONTEXT. ]*/
/*Tests_SRS_IOTHUBCLIENT_07_006: [ When IoTHubClient_LL_SetDeviceMethodCallback_Ex is called, IoTHubClient_SetDeviceMethodCallback_Ex shall return the result of IoTHubClient_LL_SetDeviceMethodCallback_Ex. ] */
TEST_FUNCTION(IoTHubClient_SetDeviceMethodCallback_Ex_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_inboundDeviceMethodCallback()
        .IgnoreArgument_userContextCallback();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    my_gballoc_free(g_userContextCallback);
    IoTHubClient_Destroy(iothub_handle);
}

/*Tests_SRS_IOTHUBCLIENT_07_007: [ IoTHubClient_SetDeviceMethodCallback_Ex shall be made thread-safe by using the lock created in IoTHubClient_Create. ]*/
/*Tests_SRS_IOTHUBCLIENT_07_003: [ If the transport handle is NULL and the worker thread is not initialized, the thread shall be started by calling IoTHubTransport_StartWorkerThread. ]*/
/*Tests_SRS_IOTHUBCLIENT_07_005: [ IoTHubClient_SetDeviceMethodCallback_Ex shall call IoTHubClient_LL_SetDeviceMethodCallback_Ex, while passing the IoTHubClient_LL_handle created by IoTHubClient_LL_Create along with the parameters iothub_ll_inbound_device_method_callback and IOTHUB_QUEUE_CONTEXT. ]*/
/*Tests_SRS_IOTHUBCLIENT_07_006: [ When IoTHubClient_LL_SetDeviceMethodCallback_Ex is called, IoTHubClient_SetDeviceMethodCallback_Ex shall return the result of IoTHubClient_LL_SetDeviceMethodCallback_Ex. ] */
TEST_FUNCTION(IoTHubClient_SetDeviceMethodCallback_Ex_NonNULL_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_inboundDeviceMethodCallback()
        .IgnoreArgument_userContextCallback();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    my_gballoc_free(g_userContextCallback);
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [ IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall copy the method_name and payload. ] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_004: [ On success IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a 0 value. ] */
TEST_FUNCTION(IoTHubClient_call_inbound_device_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClient_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
        .IgnoreArgument_psz();
    EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    // act
    ASSERT_IS_NOT_NULL(g_inboundDeviceCallback);
    int result = g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_001: [ if userContextCallback is NULL, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a nonNULL value. ] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [ If a failure is encountered IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a non-NULL value. ]*/
TEST_FUNCTION(IoTHubClient_call_inbound_device_callback_usercontext_NULL)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClient_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, NULL);
    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_inboundDeviceCallback);
    int result = g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    free(g_userContextCallback);
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_07_008: [ If inboundDeviceMethodCallback is NULL, IoTHubClient_SetDeviceMethodCallback_Ex shall call IoTHubClient_LL_SetDeviceMethodCallback_Ex, passing NULL for the iothub_ll_inbound_device_method_callback. ] */
TEST_FUNCTION(IoTHubClient_SetDeviceMethodCallback_Ex_remove_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_SetDeviceMethodCallback_Ex(iothub_handle, NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_DeviceMethodResponse_handle_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_DeviceMethodResponse(NULL, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClient_DeviceMethodResponse_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_HANDLE, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_DeviceMethodResponse(iothub_handle, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_DeviceMethodResponse_fail)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_HANDLE, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClient_DeviceMethodResponse failure in test %zu/%zu", index, count);
        IOTHUB_CLIENT_RESULT result = IoTHubClient_DeviceMethodResponse(iothub_handle, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClient_Destroy(iothub_handle);
}

#ifndef DONT_USE_UPLOADTOBLOB
/* Tests_SRS_IOTHUBCLIENT_02_047: [ If iotHubClientHandle is NULL then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_UploadToBlobAsync_with_NULL_iotHubClientHandle_fails)
{
    //arrange
    IOTHUB_CLIENT_RESULT result;

    //act
    result = IoTHubClient_UploadToBlobAsync(NULL, "a", (const unsigned char*)"b", 1, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBCLIENT_02_048: [ If destinationFileName is NULL then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_UploadToBlobAsync_with_NULL_destinationFileName_fails)
{
    //arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_UploadToBlobAsync(iothub_handle, NULL, (const unsigned char*)"b", 1, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_02_049: [ If source is NULL and size is greated than 0 then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_UploadToBlobAsync_with_NULL_source_and_size_1_fails)
{
    //arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_UploadToBlobAsync(iothub_handle, "someFileName.txt", NULL, 1, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/*Tests_SRS_IOTHUBCLIENT_02_051: [ IoTHubClient_UploadToBlobAsync shall copy the souce, size, iotHubClientFileUploadCallback, context and a non-initialized(1) THREAD_HANDLE parameters into a structure. ]*/
/*Tests_SRS_IOTHUBCLIENT_02_058: [ IoTHubClient_UploadToBlobAsync shall add the structure to the list of structures that need to be cleaned once file upload finishes. ]*/
/*Tests_SRS_IOTHUBCLIENT_02_052: [ IoTHubClient_UploadToBlobAsync shall spawn a thread passing the structure build in SRS IOTHUBCLIENT 02 051 as thread data.]*/
/*Tests_SRS_IOTHUBCLIENT_02_054: [ The thread shall call IoTHubClient_LL_UploadToBlob passing the information packed in the structure. ]*/
/*Tests_SRS_IOTHUBCLIENT_02_056: [ Otherwise the thread iotHubClientFileUploadCallbackInternal passing as result FILE_UPLOAD_OK and the structure from SRS IOTHUBCLIENT 02 051. ]*/
/*Tests_SRS_IOTHUBCLIENT_02_071: [ The thread shall mark itself as disposable. ]*/
TEST_FUNCTION(IoTHubClient_UploadToBlobAsync_succeeds)
{
    //arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    setup_iothubclient_uploadtoblobasync();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_UploadToBlobAsync(iothub_handle, "someFileName.txt", (const unsigned char*)"a", 1, test_file_upload_callback, (void*)1);

    g_thread_func(g_thread_func_arg); /*this is the thread uploading function*/

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .SetReturn((LIST_ITEM_HANDLE)0x1111);
    EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .SetReturn((LIST_ITEM_HANDLE)0x1112);
    EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
        .SetReturn((void*)g_thread_func_arg);
    IoTHubClient_Destroy(iothub_handle);
}
#endif

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_method_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClient_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_DoWork(TEST_IOTHUB_CLIENT_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(VECTOR_size(TEST_VECTOR_HANDLE)).SetReturn(1);

    STRICT_EXPECTED_CALL(VECTOR_element(TEST_VECTOR_HANDLE, 0));

    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(test_incoming_method_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, TEST_METHOD_ID, CALLBACK_CONTEXT))
        .IgnoreArgument_method_name()
        .IgnoreArgument_payload()
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(VECTOR_clear(TEST_VECTOR_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Test_SRS_IOTHUBCLIENT_07_002: [ IoTHubClient_SetDeviceTwinCallback shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SetDeviceTwinCallback function as a user context. ] */
TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_device_twin_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClient_SetDeviceTwinCallback(iothub_handle, test_device_twin_callback, NULL);
    g_deviceTwinCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_DoWork(TEST_IOTHUB_CLIENT_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(VECTOR_size(TEST_VECTOR_HANDLE)).SetReturn(1);

    STRICT_EXPECTED_CALL(VECTOR_element(TEST_VECTOR_HANDLE, 0));

    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(test_device_twin_callback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, NULL));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(VECTOR_clear(TEST_VECTOR_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_07_001: [ IoTHubClient_SendEventAsync shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SendEventAsync function as a user context. ]*/
/* Tests_SRS_IOTHUBCLIENT_01_037: [The thread created by IoTHubClient_Create shall call IoTHubClient_LL_DoWork every 1 ms.] */
/* Tests_SRS_IOTHUBCLIENT_01_038: [The thread shall exit when IoTHubClient_Destroy is called.] */
/* Tests_SRS_IOTHUBCLIENT_01_039: [All calls to IoTHubClient_LL_DoWork shall be protected by the lock created in IotHubClient_Create.] */
/* Tests_SRS_IOTHUBCLIENT_02_072: [ All threads marked as disposable (upon completion of a file upload) shall be joined and the data structures build for them shall be freed. ]*/
TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_event_confirm_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClient_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);
    g_eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_DoWork(TEST_IOTHUB_CLIENT_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(VECTOR_size(TEST_VECTOR_HANDLE)).SetReturn(1);

    STRICT_EXPECTED_CALL(VECTOR_element(TEST_VECTOR_HANDLE, 0));

    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_OK, NULL));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(VECTOR_clear(TEST_VECTOR_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

/* Tests_SRS_IOTHUBCLIENT_07_003: [ IoTHubClient_SendReportedState shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SendReportedState function as a user context. ] */
TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_reported_state_succeed)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iothub_handle = IoTHubClient_Create(TEST_CLIENT_CONFIG);
    const unsigned char* reported_state = (const unsigned char*)0x1234;
    (void)IoTHubClient_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);
    g_reportedStateCallback(REPORTED_STATE_STATUS_CODE, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClient_LL_DoWork(TEST_IOTHUB_CLIENT_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(VECTOR_size(TEST_VECTOR_HANDLE)).SetReturn(1);

    STRICT_EXPECTED_CALL(VECTOR_element(TEST_VECTOR_HANDLE, 0));

    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(test_report_state_callback(REPORTED_STATE_STATUS_CODE, NULL));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(VECTOR_clear(TEST_VECTOR_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Destroy(iothub_handle);
}

END_TEST_SUITE(iothubclient_ut)
