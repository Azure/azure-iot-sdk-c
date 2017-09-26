// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

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

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/platform.h"

#include "iothub_client_version.h"
#include "iothub_message.h"
#include "iothub_client_authorization.h"
#include "iothub_client_diagnostic.h"

#undef ENABLE_MOCKS

#include "iothub_transport_ll.h"
#include "iothub_client_ll.h"
#include "iothub_client_private.h"
#include "iothub_client_options.h"

#define ENABLE_MOCKS

#ifndef DONT_USE_UPLOADTOBLOB
#include "iothub_client_ll_uploadtoblob.h"
#endif

MOCKABLE_FUNCTION(, void, test_event_confirmation_callback, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, userContextCallback);
MOCKABLE_FUNCTION(, IOTHUBMESSAGE_DISPOSITION_RESULT, test_message_callback_async, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, iothub_reported_state_callback, int, status_code, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, iothub_device_twin_callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, deviceMethodCallback, const char*, method_name, const unsigned char*, payload, size_t, size, unsigned char**, response, size_t*, resp_size, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, iothub_client_inbound_device_method_callback, const char*, method_name, const unsigned char*, payload, size_t, size, METHOD_HANDLE, method_id, void*, userContextCallback);

MOCKABLE_FUNCTION(, STRING_HANDLE, FAKE_IoTHubTransport_GetHostname, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_SetOption, TRANSPORT_LL_HANDLE, handle, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, TRANSPORT_LL_HANDLE, FAKE_IoTHubTransport_Create, const IOTHUBTRANSPORT_CONFIG*, config);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Destroy, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_HANDLE, FAKE_IoTHubTransport_Register, TRANSPORT_LL_HANDLE, handle, const IOTHUB_DEVICE_CONFIG*, device, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, PDLIST_ENTRY, waitingToSend);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unregister, IOTHUB_DEVICE_HANDLE, deviceHandle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_DoWork, TRANSPORT_LL_HANDLE, handle, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_SetRetryPolicy, TRANSPORT_LL_HANDLE, handle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_GetSendStatus, IOTHUB_DEVICE_HANDLE, handle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe_DeviceTwin, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe_DeviceTwin, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_SendMessageDisposition, MESSAGE_CALLBACK_INFO*, messageData, IOTHUBMESSAGE_DISPOSITION_RESULT, disposition);
MOCKABLE_FUNCTION(, const char*, FAKE_IoTHubMessage_GetMessageId, IOTHUB_MESSAGE_HANDLE, message);
MOCKABLE_FUNCTION(, IOTHUB_PROCESS_ITEM_RESULT, FAKE_IoTHubTransport_ProcessItem, TRANSPORT_LL_HANDLE, handle, IOTHUB_IDENTITY_TYPE, item_type, IOTHUB_IDENTITY_INFO*, iothub_item);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe_DeviceMethod, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe_DeviceMethod, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, connectionStatusCallback, IOTHUB_CLIENT_CONNECTION_STATUS, result3, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, userContextCallback);
MOCKABLE_FUNCTION(, IOTHUBMESSAGE_DISPOSITION_RESULT, messageCallback, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback);
MOCKABLE_FUNCTION(, bool, messageCallbackEx, MESSAGE_CALLBACK_INFO*, messageData, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, eventConfirmationCallback, IOTHUB_CLIENT_CONFIRMATION_RESULT, result2, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_DeviceMethod_Response, IOTHUB_DEVICE_HANDLE, handle, METHOD_HANDLE, methodId, const unsigned char*, response, size_t, resp_size, int, status_response);

#undef ENABLE_MOCKS

TEST_DEFINE_ENUM_TYPE(IOTHUB_PROCESS_ITEM_RESULT, IOTHUB_PROCESS_ITEM_RESULT_VALUE);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_PROCESS_ITEM_RESULT, IOTHUB_PROCESS_ITEM_RESULT_VALUE);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

static TEST_MUTEX_HANDLE test_serialize_mutex;
static TEST_MUTEX_HANDLE g_dllByDll;

bool g_fail_string_construct_sprintf;
bool g_fail_platform_get_platform_info;
bool g_fail_string_concat_with_string;

static const char* TEST_STRING_VALUE = "Test string value";

#define TEST_DEVICE_ID "theidofTheDevice"
#define TEST_DEVICE_KEY "theKeyoftheDevice"
#define TEST_DEVICE_SAS "theSasOfTheDevice"
#define TEST_IOTHUBNAME "theNameoftheIotHub"
#define TEST_IOTHUBSUFFIX "theSuffixoftheIotHubHostname"
#define TEST_AUTHORIZATIONKEY "theAuthorizationKey"

#define TEST_HOSTNAME_TOKEN "HostName"
#define TEST_HOSTNAME_VALUE "theNameoftheIotHub.theSuffixoftheIotHubHostname"

#define TEST_DEVICEID_TOKEN "DeviceId"
#define TEST_DEVICEKEY_TOKEN "SharedAccessKey"
#define TEST_DEVICESAS_TOKEN "SharedAccessSignature"
#define TEST_PROTOCOL_GATEWAY_HOST_NAME_TOKEN "GatewayHostName"
#define TEST_X509 "x509"

#define TEST_DEVICEMESSAGE_HANDLE (IOTHUB_MESSAGE_HANDLE)0x52
#define TEST_DEVICEMESSAGE_HANDLE_2 (IOTHUB_MESSAGE_HANDLE)0x53
#define TEST_IOTHUB_CLIENT_LL_HANDLE    (IOTHUB_CLIENT_LL_HANDLE)0x4242

#define TEST_STRING_HANDLE (STRING_HANDLE)0x46
#define TEST_STRING_TOKENIZER_HANDLE (STRING_TOKENIZER_HANDLE)0x48

#define TEST_DEVICE_STATUS_CODE        200

#define TEST_TRANSPORT_LL_HANDLE            (TRANSPORT_LL_HANDLE)0x49
#define TEST_IOTHUB_DEVICE_HANDLE           (IOTHUB_DEVICE_HANDLE)0x50
#define TEST_MESSAGE_HANDLE                 (IOTHUB_MESSAGE_HANDLE)0x51
#define TEST_TIME_VALUE                     (time_t)123456

#define TEST_BUFFER_HANDLE                  (BUFFER_HANDLE)0x52
#define TEST_RETRY_POLICY                   IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define TEST_RETRY_TIMEOUT_SECS             60

#define TEST_METHOD_ID                      (METHOD_HANDLE)0x61

static const char* TEST_METHOD_NAME = "method_name";
static const char* TEST_CHAR = "TestChar";
static tickcounter_ms_t g_current_ms = 0;
static const char* TEST_DEVICE_METHOD_RESPONSE = "{device:method, response:true}";

const unsigned char TEST_REPORTED_STATE[] = { 0x01, 0x02, 0x03 };
const size_t TEST_REPORTED_SIZE = sizeof(TEST_REPORTED_STATE) / sizeof(TEST_REPORTED_STATE[0]);

static const TRANSPORT_PROVIDER* provideFAKE(void);

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_NULL_protocol =
{
    NULL,                   /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    TEST_DEVICE_KEY,        /* const char* deviceKey;                       */
    TEST_IOTHUBNAME,        /* const char* iotHubName;                      */
    TEST_IOTHUBSUFFIX,      /* const char* iotHubSuffix;                    */
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG =
{
    provideFAKE,            /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    TEST_DEVICE_KEY,        /* const char* deviceKey;                       */
    TEST_DEVICE_SAS,        /* const char* deviceSasToken;                  */
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

static const IOTHUB_CLIENT_DEVICE_CONFIG TEST_DEVICE_CONFIG_null_protocol =
{
    NULL,
    FAKE_TRANSPORT_HANDLE,
    TEST_DEVICE_ID,
    TEST_DEVICE_KEY
};

static const IOTHUB_CLIENT_DEVICE_CONFIG TEST_DEVICE_CONFIG_null_handle =
{
    provideFAKE,
    NULL,
    TEST_DEVICE_ID,
    TEST_DEVICE_KEY
};

static const IOTHUB_CLIENT_DEVICE_CONFIG TEST_DEVICE_CONFIG_NULL_device_key_NULL_sas_token =
{
    provideFAKE,
    FAKE_TRANSPORT_HANDLE,
    NULL,
    NULL
};

static IOTHUB_AUTHORIZATION_HANDLE my_IoTHubClient_Auth_Create(const char* device_key, const char* device_id, const char* sas_token)
{
    (void)device_key;
    (void)device_id;
    (void)sas_token;
    return (IOTHUB_AUTHORIZATION_HANDLE)my_gballoc_malloc(1);
}

static void my_IoTHubClient_Auth_Destroy(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    my_gballoc_free(handle);
}

static STRING_HANDLE my_STRING_new(void)
{
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

STRING_HANDLE STRING_construct_sprintf(const char* psz, ...)
{
    (void)psz;
    STRING_HANDLE result;
    if (g_fail_string_construct_sprintf)
    {
        result = (STRING_HANDLE)NULL;
    }
    else
    {
        result = (STRING_HANDLE)my_gballoc_malloc(1);   
    }
    return result;
}

static int m_STRING_concat_with_STRING(STRING_HANDLE handle, STRING_HANDLE arg1)
{
    (void)handle;
    (void)arg1;
    int result;
    if (g_fail_string_concat_with_string)
    {
        result = -1;
    }
    else
    {
        result = 0;
    }
    return result;
}

static STRING_HANDLE my_STRING_clone(STRING_HANDLE handle)
{
    (void)handle;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    (void)handle;

    if (handle != TEST_STRING_HANDLE)
    {
        my_gballoc_free(handle);
    }
}

static STRING_TOKENIZER_HANDLE my_STRING_TOKENIZER_create(STRING_HANDLE handle)
{
    (void)handle;
    return (STRING_TOKENIZER_HANDLE)my_gballoc_malloc(1);
}

int my_STRING_TOKENIZER_get_next_token(STRING_TOKENIZER_HANDLE t, STRING_HANDLE output, const char* delimiters)
{
    (void)delimiters;
    (void)t;
    (void)output;
    return 0;
}

static void my_STRING_TOKENIZER_destroy(STRING_TOKENIZER_HANDLE handle)
{
    my_gballoc_free(handle);
}

static TICK_COUNTER_HANDLE my_tickcounter_create(void)
{
    return (TICK_COUNTER_HANDLE)my_gballoc_malloc(1);
}

static int my_tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)
{
    (void)tick_counter;
    g_current_ms += 1000;
    *current_ms = g_current_ms;
    return 0;
}

static void my_tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)
{
    my_gballoc_free(tick_counter);
}

static CONSTBUFFER_HANDLE my_CONSTBUFFER_Create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (CONSTBUFFER_HANDLE)my_gballoc_malloc(1);
}

static void my_CONSTBUFFER_Destroy(CONSTBUFFER_HANDLE constbufferHandle)
{
    my_gballoc_free(constbufferHandle);
}

static IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE my_IoTHubClient_LL_UploadToBlob_Create(const IOTHUB_CLIENT_CONFIG* config)
{
    (void)config;
    return (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE)my_gballoc_malloc(1);
}

static void my_IoTHubClient_LL_UploadToBlob_Destroy(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle)
{
    my_gballoc_free(handle);
}

static IOTHUB_DEVICE_HANDLE my_FAKE_IoTHubTransport_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, PDLIST_ENTRY waitingToSend)
{
    (void)handle;
    (void)device;
    (void)iotHubClientHandle;
    (void)waitingToSend;
    return (IOTHUB_DEVICE_HANDLE)my_gballoc_malloc(1);
}

static void my_FAKE_IoTHubTransport_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    my_gballoc_free(deviceHandle);
}

static IOTHUB_CLIENT_RESULT my_FAKE_IoTHubTransport_GetSendStatus(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_STATUS* iotHubClientStatus)
{
    (void)handle;
    (void)iotHubClientStatus;
    return IOTHUB_CLIENT_OK;
}

static int my_FAKE_IoTHubTransport_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    (void)handle;
    (void)retryPolicy;
    (void)retryTimeoutLimitInSeconds;
    return 0;
}

static int my_FAKE_DeviceMethod_Response(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t resp_size, int status_response)
{
    (void)handle;
    (void)methodId;
    (void)response;
    (void)resp_size;
    (void)status_response;
    return 0;
}

STRING_HANDLE my_FAKE_IoTHubTransport_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    (void)handle;
    return TEST_STRING_HANDLE;
}

STRING_HANDLE my_plafrom_get_platform_info(void)
{
    STRING_HANDLE result;
    if (g_fail_platform_get_platform_info)
    {
        result = NULL;
    }
    else
    {
        result = (STRING_HANDLE)my_gballoc_malloc(1);
    }
    return result;
}

static TRANSPORT_PROVIDER FAKE_transport_provider =
{
    FAKE_IoTHubTransport_SendMessageDisposition,   /*pfIotHubTransport_SendMessageDisposition IoTHubTransport_SendMessageDisposition;*/
    FAKE_IoTHubTransport_Subscribe_DeviceMethod, /*pfIoTHubTransport_Subscribe_DeviceMethod IoTHubTransport_Subscribe_DeviceMethod;*/
    FAKE_IoTHubTransport_Unsubscribe_DeviceMethod, /*pfIoTHubTransport_Unsubscribe_DeviceMethod IoTHubTransport_Unsubscribe_DeviceMethod;*/
    FAKE_IoTHubTransport_DeviceMethod_Response, /*pfIoTHubTransport_DeviceMethod_Response IoTHubTransport_DeviceMethod_Response;*/
    FAKE_IoTHubTransport_Subscribe_DeviceTwin, /*pfIoTHubTransport_Subscribe_DeviceTwin IoTHubTransport_Subscribe_DeviceTwin; */
    FAKE_IoTHubTransport_Unsubscribe_DeviceTwin, /*pfIoTHubTransport_Unsubscribe_DeviceTwin IoTHubTransport_Unsubscribe_DeviceTwin; */
    FAKE_IoTHubTransport_ProcessItem,   /*pfIoTHubTransport_ProcessItem IoTHubTransport_ProcessItem     */
    FAKE_IoTHubTransport_GetHostname,   /*pfIoTHubTransport_GetHostname IoTHubTransport_GetHostname     */
    FAKE_IoTHubTransport_SetOption,     /*pfIoTHubTransport_SetOption IoTHubTransport_SetOption;        */
    FAKE_IoTHubTransport_Create,        /*pfIoTHubTransport_Create IoTHubTransport_Create;              */
    FAKE_IoTHubTransport_Destroy,       /*pfIoTHubTransport_Destroy IoTHubTransport_Destroy;            */
    FAKE_IoTHubTransport_Register,		/*pfIotHubTransport_Register IoTHubTransport_Register;          */
    FAKE_IoTHubTransport_Unregister,    /*pfIotHubTransport_Unregister IoTHubTransport_Unegister;       */
    FAKE_IoTHubTransport_Subscribe,     /*pfIoTHubTransport_Subscribe IoTHubTransport_Subscribe;        */
    FAKE_IoTHubTransport_Unsubscribe,   /*pfIoTHubTransport_Unsubscribe IoTHubTransport_Unsubscribe;    */
    FAKE_IoTHubTransport_DoWork,        /*pfIoTHubTransport_DoWork IoTHubTransport_DoWork;              */
    FAKE_IoTHubTransport_SetRetryPolicy,/*pfIoTHubTransport_SetRetryPolicy IoTHubTransport_SetRetryPolicy;*/
    FAKE_IoTHubTransport_GetSendStatus  /*pfIoTHubTransport_GetSendStatus IoTHubTransport_GetSendStatus;*/
};

static const TRANSPORT_PROVIDER* provideFAKE(void)
{
    return &FAKE_transport_provider;
}

#ifdef __cplusplus
extern "C"
{
#endif

    void real_DList_InitializeListHead(PDLIST_ENTRY listHead);
    int real_DList_IsListEmpty(const PDLIST_ENTRY listHead);
    void real_DList_InsertTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
    void real_DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
    void real_DList_AppendTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY ListToAppend);
    int real_DList_RemoveEntryList(PDLIST_ENTRY listEntry);
    PDLIST_ENTRY real_DList_RemoveHeadList(PDLIST_ENTRY listHead);

#ifdef __cplusplus
}
#endif

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static MESSAGE_CALLBACK_INFO* make_test_message_info(IOTHUB_MESSAGE_HANDLE message)
{
    MESSAGE_CALLBACK_INFO* result = (MESSAGE_CALLBACK_INFO*)malloc(sizeof(MESSAGE_CALLBACK_INFO));
    result->messageHandle = message;
    result->transportContext = NULL;

    return result;
}

static void destroy_test_message_info(MESSAGE_CALLBACK_INFO* oneMessageData)
{
    IoTHubMessage_Destroy(oneMessageData->messageHandle);
    free(oneMessageData->transportContext);
    free(oneMessageData);
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(iothubclient_ll_ut)

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

    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_TOKENIZER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_TRANSPORT_PROVIDER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_TWIN_STATE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_IDENTITY_TYPE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_PROCESS_ITEM_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_TWIN_UPDATE_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, int);

#ifndef DONT_USE_UPLOADTOBLOB
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE, void*);
#endif // DONT_USE_UPLOADTOBLOB

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_GetVersionString, "version 1.0");

    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_Subscribe_DeviceTwin, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Subscribe_DeviceTwin, __FAILURE__);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_ProcessItem, IOTHUB_PROCESS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_ProcessItem, IOTHUB_PROCESS_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_GetHostname, my_FAKE_IoTHubTransport_GetHostname);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_GetHostname, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_SetOption, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_Create, TEST_TRANSPORT_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_Register, my_FAKE_IoTHubTransport_Register);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Register, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_Unregister, my_FAKE_IoTHubTransport_Unregister);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_Subscribe, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Subscribe, __FAILURE__);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_SetRetryPolicy, my_FAKE_IoTHubTransport_SetRetryPolicy);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_SetRetryPolicy, __FAILURE__);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_GetSendStatus, my_FAKE_IoTHubTransport_GetSendStatus);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_GetSendStatus, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_Subscribe_DeviceMethod, 0);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Subscribe_DeviceMethod, __FAILURE__);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubMessage_GetMessageId, "1");
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_SendMessageDisposition, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_SendMessageDisposition, IOTHUB_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_DeviceMethod_Response, my_FAKE_DeviceMethod_Response);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_DeviceMethod_Response, __FAILURE__);

#ifndef DONT_USE_UPLOADTOBLOB
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_UploadToBlob_Create, my_IoTHubClient_LL_UploadToBlob_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_UploadToBlob_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_UploadToBlob_Destroy, my_IoTHubClient_LL_UploadToBlob_Destroy);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_UploadToBlob_SetOption, IOTHUB_CLIENT_OK);
#endif

    REGISTER_GLOBAL_MOCK_RETURN(deviceMethodCallback, 200);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_CreateFromString, (IOTHUB_MESSAGE_HANDLE)0x44);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_Clone, (IOTHUB_MESSAGE_HANDLE)0x44);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_Clone, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Diagnostic_AddIfNecessary, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Diagnostic_AddIfNecessary, 100);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, (time_t)TEST_TIME_VALUE);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat_with_STRING, m_STRING_concat_with_STRING);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, my_STRING_clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_clone, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_build, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_build, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Create, my_CONSTBUFFER_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(CONSTBUFFER_Create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Destroy, my_CONSTBUFFER_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_create, my_STRING_TOKENIZER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_get_next_token, my_STRING_TOKENIZER_get_next_token);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_get_next_token, __FAILURE__);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_destroy, my_STRING_TOKENIZER_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_create, my_tickcounter_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_destroy, my_tickcounter_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_get_current_ms, my_tickcounter_get_current_ms);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_get_current_ms, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(DList_InitializeListHead, real_DList_InitializeListHead);
    REGISTER_GLOBAL_MOCK_HOOK(DList_IsListEmpty, real_DList_IsListEmpty);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertTailList, real_DList_InsertTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertHeadList, real_DList_InsertHeadList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_AppendTailList, real_DList_AppendTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveEntryList, real_DList_RemoveEntryList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveHeadList, real_DList_RemoveHeadList);

    REGISTER_GLOBAL_MOCK_RETURN(test_message_callback_async, IOTHUBMESSAGE_ACCEPTED);
    REGISTER_GLOBAL_MOCK_RETURN(messageCallback, IOTHUBMESSAGE_ACCEPTED);
    REGISTER_GLOBAL_MOCK_RETURN(messageCallbackEx, true);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Create, my_IoTHubClient_Auth_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Destroy, my_IoTHubClient_Auth_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(platform_get_platform_info, my_plafrom_get_platform_info);
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
    g_fail_string_construct_sprintf = false;
    g_fail_platform_get_platform_info = false;
    g_fail_string_concat_with_string = false;
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
            result = __FAILURE__;
            break;
        }
    }
    return result;
}

static void setup_iothubclient_ll_create_mocks(bool use_device_config)
{
    STRICT_EXPECTED_CALL(platform_get_platform_info());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (use_device_config)
    {
        STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetHostname(IGNORED_PTR_ARG)); /*this is getting the hostname as STRING_HANDLE*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the hostname as const char* */
            .SetReturn(TEST_HOSTNAME_VALUE);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Create(IGNORED_PTR_ARG));
    }
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/
    STRICT_EXPECTED_CALL(tickcounter_create());
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Register(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, TEST_RETRY_POLICY, 0));
    if (use_device_config)
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
}

static void setup_iothubclient_ll_sendreportedstate_mocks()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(CONSTBUFFER_Create(TEST_REPORTED_STATE, TEST_REPORTED_SIZE));

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*_DoWork will ask "what's the time"*/
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceTwin(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
}

static void setup_iothubclient_ll_createfromconnectionstring_mocks(const char* device_token, const char* token_value)
{
#ifndef NO_LOGGING
    STRICT_EXPECTED_CALL(IoTHubClient_GetVersionString());
#endif

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_HOSTNAME_TOKEN);
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEID_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);  // 20
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(device_token);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(token_value);

    /* loop exit */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(1); // 27

    setup_iothubclient_ll_create_mocks(false);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // 38
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();
}

/*Tests_SRS_IOTHUBCLIENT_LL_05_001: [IoTHubClient_LL_CreateFromConnectionString shall obtain the version string by a call to IoTHubClient_GetVersionString.]*/
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_getsversionstring_fail)
{
    // arrange
#ifndef NO_LOGGING
    EXPECTED_CALL(IoTHubClient_GetVersionString());
#endif

    // act
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_CreateFromConnectionString(NULL, NULL);

    // assert
    ASSERT_IS_NULL(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_12_004: [IoTHubClient_LL_CreateFromConnectionString shall allocate IOTHUB_CLIENT_CONFIG structure] */
/*Tests_SRS_IOTHUBCLIENT_LL_12_006: [IoTHubClient_LL_CreateFromConnectionString shall verify the existence of the following Key/Value pairs in the connection string: HostName, DeviceId, SharedAccessKey, SharedAccessSignature or x509]  */
/*Tests_SRS_IOTHUBCLIENT_LL_03_010: [IoTHubClient_LL_CreateFromConnectionString shall return NULL if both a deviceKey & a deviceSasToken are specified.] */
/*Tests_SRS_IOTHUBCLIENT_LL_12_013: [If the parsing failed IoTHubClient_LL_CreateFromConnectionString returns NULL]  */
/*Tests_SRS_IOTHUBCLIENT_LL_12_014: [If either of key is missing or x509 is not set to "true" then IoTHubClient_LL_CreateFromConnectionString returns NULL ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_12_016: [IoTHubClient_LL_CreateFromConnectionString shall return NULL if IoTHubClient_LL_Create call fails] */
/*Tests_SRS_IOTHUBCLIENT_LL_02_093: [ If creating the IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE fails then IoTHubClient_LL_CreateFromConnectionString shall fail and return NULL. ]*/
/* Tests_SRS_IOTHUBCLIENT_LL_12_006: [IoTHubClient_LL_CreateFromConnectionString shall verify the existence of the following Key/Value pairs in the connection string: HostName, DeviceId, SharedAccessKey, SharedAccessSignature or x509.]  */
/*Tests_SRS_IOTHUBCLIENT_LL_12_014: [If either of key is missing or x509 is not set to "true" then IoTHubClient_LL_CreateFromConnectionString returns NULL ]*/
/* Tests_SRS_IOTHUBCLIENT_LL_12_009: [IoTHubClient_LL_CreateFromConnectionString shall split the value of HostName to Name and Suffix using the first "." as a separator] */
/* Tests_SRS_IOTHUBCLIENT_LL_12_015: [If the string split failed, IoTHubClient_LL_CreateFromConnectionString returns NULL ] */
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_with_DeviceKey_fail)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    //arrange
    setup_iothubclient_ll_createfromconnectionstring_mocks(TEST_DEVICEKEY_TOKEN, TEST_STRING_VALUE);

    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 0, 10, 16, 19, 24, 27, 29, 30, 36, 37, 38, 41, 42, 43, 44, 45, 46, 47, 48, 49 };
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClient_LL_CreateFromConnectionString failure in test %zu/%zu", index, count);

        //act
        IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

        ///assert
        ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
    }

    ///cleanup
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUBCLIENT_LL_02_092: [ IoTHubClient_LL_CreateFomConnectionString shall create a IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE from IOTHUB_CLIENT_CONFIG. ]*/
/* Tests_SRS_IOTHUBCLIENT_LL_12_011: [IoTHubClient_LL_CreateFromConnectionString shall call into the IoTHubClient_LL_Create API with the current structure and returns with the return value of it] */
/* Tests_SRS_IOTHUBCLIENT_LL_12_010: [IoTHubClient_LL_CreateFromConnectionString shall fill up the IOTHUB_CLIENT_CONFIG structure using the following mapping: iotHubName = Name, iotHubSuffix = Suffix, deviceId = DeviceId, deviceKey = SharedAccessKey or deviceSasToken = SharedAccessSignature] */
/* Tests_SRS_IOTHUBCLIENT_LL_04_002: [If it does not, it shall pass the protocolGatewayHostName NULL.] */
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_with_DeviceKey_succeeds)
{
    //arrange
    setup_iothubclient_ll_createfromconnectionstring_mocks(TEST_DEVICEKEY_TOKEN, TEST_STRING_VALUE);

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(result);
}

/* Tests_SRS_IOTHUBCLIENT_LL_12_011: [IoTHubClient_LL_CreateFromConnectionString shall call into the IoTHubClient_LL_Create API with the current structure and returns with the return value of it] */
/* Tests_SRS_IOTHUBCLIENT_LL_12_010: [IoTHubClient_LL_CreateFromConnectionString shall fill up the IOTHUB_CLIENT_CONFIG structure using the following mapping: iotHubName = Name, iotHubSuffix = Suffix, deviceId = DeviceId, deviceKey = SharedAccessKey or deviceSasToken = SharedAccessSignature] */
/* Tests_SRS_IOTHUBCLIENT_LL_02_092: [ IoTHubClient_LL_CreateFomConnectionString shall create a IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE from IOTHUB_CLIENT_CONFIG. ]*/
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_with_DeviceSasToken_succeeds)
{
    //arrange
    setup_iothubclient_ll_createfromconnectionstring_mocks(TEST_DEVICESAS_TOKEN, TEST_STRING_VALUE);

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    ///assert
    ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(result);
}

/* Tests_SRS_IOTHUBCLIENT_LL_04_001: [IoTHubClient_LL_CreateFromConnectionString shall verify the existence of key/value pair GatewayHostName. If it does exist it shall pass the value to IoTHubClient_LL_Create API.] */
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_withGatewayHostName_succeeds)
{
    //arrange
#ifndef NO_LOGGING
    STRICT_EXPECTED_CALL(IoTHubClient_GetVersionString());
#endif

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_HOSTNAME_TOKEN);
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEID_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);  // 20
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEKEY_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 4*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(0);
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_PROTOCOL_GATEWAY_HOST_NAME_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop exit */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(1); // 27

    setup_iothubclient_ll_create_mocks(false);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // 36
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(result);
}


/*Tests_SRS_IOTHUBCLIENT_LL_12_003: [IoTHubClient_LL_CreateFromConnectionString shall verify the input parameters and if any of them NULL then return NULL] */
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_if_input_parameter_connectionString_is_NULL_then_return_NULL)
{
    //arrange
#ifndef NO_LOGGING
    STRICT_EXPECTED_CALL(IoTHubClient_GetVersionString());
#endif

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateFromConnectionString(NULL, provideFAKE);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_12_003: [IoTHubClient_LL_CreateFromConnectionString shall return NULL if any of the input parameter is NULL] */
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_if_input_parameter_protocol_is_NULL_then_return_NULL)
{
    //arrange
#ifndef NO_LOGGING
    STRICT_EXPECTED_CALL(IoTHubClient_GetVersionString());
#endif

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateFromConnectionString(TEST_CHAR, NULL);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_12_014: [If either of key is missing or x509 is not set to "true" then IoTHubClient_LL_CreateFromConnectionString returns NULL ]*/
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_with_x509_true_succeeds)
{
    //arrange
#ifndef NO_LOGGING
    STRICT_EXPECTED_CALL(IoTHubClient_GetVersionString());
#endif

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_HOSTNAME_TOKEN);
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEID_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);  // 20
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_X509);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn("true");

    /* loop exit */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(1);

    setup_iothubclient_ll_create_mocks(false);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(result);

}

/*Tests_SRS_IOTHUBCLIENT_LL_12_014: [If either of key is missing or x509 is not set to "true" then IoTHubClient_LL_CreateFromConnectionString returns NULL ]*/
TEST_FUNCTION(IoTHubClient_LL_CreateFromConnectionString_with_x509_test_string_fails)
{
    //arrange
#ifndef NO_LOGGING
    STRICT_EXPECTED_CALL(IoTHubClient_GetVersionString());
#endif

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_HOSTNAME_TOKEN);
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEID_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);  // 20
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_X509);
    /* loop exit */
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_STRING_VALUE);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // 36
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(result);

}

/*Tests_SRS_IOTHUBCLIENT_LL_02_001: [IoTHubClient_LL_Create shall return NULL if config parameter is NULL or protocol field is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_Create_with_NULL_parameter_fails)
{
    // arrange

    // act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_Create(NULL);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_001: [IoTHubClient_LL_Create shall return NULL if config parameter is NULL or protocol field is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_Create_with_NULL_protocol_fails)
{
    // arrange

    // act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_Create(&TEST_CONFIG_NULL_protocol);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_094: [IoTHubClient_LL_Create shall create a IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE from IOTHUB_CLIENT_CONFIG. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_045: [Otherwise IoTHubClient_LL_Create shall create a new TICK_COUNTER_HANDLE ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_004: [Otherwise IoTHubClient_LL_Create shall initialize a new DLIST (further called "waitingToSend") containing records with fields of the following types: IOTHUB_MESSAGE_HANDLE, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_006: [IoTHubClient_LL_Create shall populate a structure of type IOTHUBTRANSPORT_CONFIG with the information from config parameter and the previous DLIST and shall pass that to the underlying layer _Create function.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_008: [Otherwise, IoTHubClient_LL_Create shall succeed and return a non-NULL handle.] */
/*Tests_SRS_IOTHUBCLIENT_LL_17_008: [IoTHubClient_LL_Create shall call the transport _Register function with the deviceId, DeviceKey and waitingToSend list.] */
/*Tests_SRS_IOTHUBCLIENT_LL_07_029: [ IoTHubClient_LL_Create shall create the Auth module with the device_key, device_id, and/or deviceSasToken values ] */
TEST_FUNCTION(IoTHubClient_LL_Create_suceeds)
{
    //arrange
    IOTHUB_DEVICE_CONFIG device;
    device.deviceId = TEST_CONFIG.deviceId;
    device.deviceKey = TEST_CONFIG.deviceKey;
    device.deviceSasToken = NULL;

    setup_iothubclient_ll_create_mocks(false);

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_Create(&TEST_CONFIG);

    ///assert
    ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_17_009: [If the _Register function fails, this function shall fail and return NULL.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_004: [Otherwise IoTHubClient_LL_Create shall initialize a new DLIST (further called "waitingToSend") containing records with fields of the following types: IOTHUB_MESSAGE_HANDLE, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_006: [IoTHubClient_LL_Create shall populate a structure of type IOTHUBTRANSPORT_CONFIG with the information from config parameter and the previous DLIST and shall pass that to the underlying layer _Create function.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_007: [If the underlaying layer _Create function fails them IoTHubClient_LL_Create shall fail and return NULL.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_046: [ If creating the TICK_COUNTER_HANDLE fails then IoTHubClient_LL_Create shall fail and return NULL. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_095: [ If creating the IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE fails then IoTHubClient_LL_Create shall fail and return NULL. ]*/
TEST_FUNCTION(IoTHubClient_LL_Create_fail)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_DEVICE_CONFIG device;
    device.deviceId = TEST_CONFIG.deviceId;
    device.deviceKey = TEST_CONFIG.deviceKey;
    device.deviceSasToken = NULL;
    umock_c_reset_all_calls();

    setup_iothubclient_ll_create_mocks(false);

    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 1, 2, 8, 9, 10 };
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
        sprintf(tmp_msg, "IoTHubClient_LL_Create failure in test %zu/%zu", index, count);

        IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_Create(&TEST_CONFIG);

        //assert
        ASSERT_ARE_EQUAL_WITH_MSG(void_ptr, NULL, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_IOTHUBCLIENT_LL_17_001: [IoTHubClient_LL_CreateWithTransport shall return NULL if config parameter is NULL, or protocol field is NULL or transportHandle is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_null_config_fails)
{
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(NULL);

    ASSERT_IS_NULL(result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_17_001: [IoTHubClient_LL_CreateWithTransport shall return NULL if config parameter is NULL, or protocol field is NULL or transportHandle is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_null_protocol_fails)
{
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG_null_protocol);

    ASSERT_IS_NULL(result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_17_001: [IoTHubClient_LL_CreateWithTransport shall return NULL if config parameter is NULL, or protocol field is NULL or transportHandle is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_null_transportHandle_fails)
{
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG_null_handle);

    ASSERT_IS_NULL(result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_098: [ IoTHubClient_LL_CreateWithTransport shall fail and return NULL if both config->deviceKey AND config->deviceSasToken are NULL. ]*/
TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_with_NULL_deviceKey_AND_NULL_sas_token_fails)
{
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG_NULL_device_key_NULL_sas_token);

    ASSERT_IS_NULL(result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_17_002: [IoTHubClient_LL_CreateWithTransport shall allocate data for the IOTHUB_CLIENT_LL_HANDLE.] */
/*Tests_SRS_IOTHUBCLIENT_LL_02_096: [ IoTHubClient_LL_CreateWithTransport shall create the data structures needed to instantiate a IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_047: [ IoTHubClient_LL_CreateWithTransport shall create a TICK_COUNTER_HANDLE. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_17_004: [IoTHubClient_LL_CreateWithTransport shall initialize a new DLIST (further called "waitingToSend") containing records with fields of the following types: IOTHUB_MESSAGE_HANDLE, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*.] */
/*Tests_SRS_IOTHUBCLIENT_LL_17_006: [IoTHubClient_LL_CreateWithTransport shall call the transport _Register function with the deviceId, DeviceKey and waitingToSend list.]*/
TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_Succeeds)
{
    //arrange
    IOTHUB_DEVICE_CONFIG device;
    device.deviceId = TEST_DEVICE_CONFIG.deviceId;
    device.deviceKey = TEST_DEVICE_CONFIG.deviceKey;
    device.deviceSasToken = NULL;

    setup_iothubclient_ll_create_mocks(true);

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_17_003: [If allocation fails, the function shall fail and return NULL.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_048: [ If creating the handle fails, then IoTHubClient_LL_CreateWithTransport shall fail and return NULL ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_17_007: [If the _Register function fails, this function shall fail and return NULL.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_097: [ If creating the data structures fails or instantiating the IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE fails then IoTHubClient_LL_CreateWithTransport shall fail and return NULL. ]*/

TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_fail)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    //arrange
    IOTHUB_DEVICE_CONFIG device;
    device.deviceId = TEST_DEVICE_CONFIG.deviceId;
    device.deviceKey = TEST_DEVICE_CONFIG.deviceKey;
    device.deviceSasToken = NULL;

    setup_iothubclient_ll_create_mocks(true);

    //act
    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 1, 2, 6, 9, 12, 13, 14, 17, 18 };

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
        sprintf(tmp_msg, "IoTHubClient_LL_CreateWithTransport failure in test %zu/%zu", index, count);

        IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

        //assert
        ASSERT_ARE_EQUAL_WITH_MSG(void_ptr, NULL, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUBCLIENT_LL_09_010: [ If any failure occurs `IoTHubClient_LL_Create` shall destroy the `transportHandle` only if it has created it ]
TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_create_tickcounter_fails_shared_transport_is_not_destroyed)
{
    //arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(platform_get_platform_info());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetHostname(IGNORED_PTR_ARG)); /*this is getting the hostname as STRING_HANDLE*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the hostname as const char* */
        .SetReturn(TEST_HOSTNAME_VALUE);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/

    STRICT_EXPECTED_CALL(tickcounter_create())
        .SetReturn(NULL);

    // Failure cleanup calls
    // Note: not destroying the shared transport!
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

    //assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SRS_IOTHUBCLIENT_LL_09_010: [ If any failure occurs `IoTHubClient_LL_Create` shall destroy the `transportHandle` only if it has created it ]
TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_register_fails_shared_transport_is_not_destroyed)
{
    //arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(platform_get_platform_info());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetHostname(IGNORED_PTR_ARG)); /*this is getting the hostname as STRING_HANDLE*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the hostname as const char* */
        .SetReturn(TEST_HOSTNAME_VALUE);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/

    STRICT_EXPECTED_CALL(tickcounter_create());
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Register(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(NULL);

    // Failure cleanup calls
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    // Note: not destroying the shared transport!
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SRS_IOTHUBCLIENT_LL_09_010: [ If any failure occurs `IoTHubClient_LL_Create` shall destroy the `transportHandle` only if it has created it ]
TEST_FUNCTION(IoTHubClient_LL_CreateWithTransport_set_retry_policy_fails_shared_transport_is_not_destroyed)
{
    //arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(platform_get_platform_info());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetHostname(IGNORED_PTR_ARG)); /*this is getting the hostname as STRING_HANDLE*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the hostname as const char* */
        .SetReturn(TEST_HOSTNAME_VALUE);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/

    STRICT_EXPECTED_CALL(tickcounter_create());
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Register(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, TEST_RETRY_POLICY, 0))
        .SetReturn(IOTHUB_CLIENT_ERROR);

    // Failure cleanup calls
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    // Note: not destroying the shared transport!
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_LL_HANDLE result = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_009: [IoTHubClient_LL_Destroy shall do nothing if parameter iotHubClientHandle is NULL.] */
TEST_FUNCTION(IoTHubClient_LL_Destroy_with_NULL_succeeds)
{
    //arrange

    //act
    IoTHubClient_LL_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_010: [If iotHubClientHandle was not created by IoTHubClient_LL_CreateWithTransport, IoTHubClient_LL_Destroy  shall call the underlaying layer's _Destroy function.] */
/*Tests_SRS_IOTHUBCLIENT_LL_17_010: [IoTHubClient_LL_Destroy  shall call the underlaying layer's _Unregister function] */
/*Tests_SRS_IOTHUBCLIENT_LL_17_011: [IoTHubClient_LL_Destroy  shall free the resources allocated by IoTHubClient (if any).] */
TEST_FUNCTION(IoTHubClient_LL_Destroys_the_underlying_transport_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(TEST_HOSTNAME_VALUE);
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClient_LL_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_17_005: [IoTHubClient_LL_CreateWithTransport shall save the transport handle and mark this transport as shared.] */
TEST_FUNCTION(IoTHubClient_LL_Destroys_unregisters_but_does_not_destroy_transport_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(TEST_HOSTNAME_VALUE);
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));


    //act
    IoTHubClient_LL_Destroy(handle);

    //assert -uMock does it
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_011: [IoTHubClient_LL_SendEventAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle or eventMessageHandle is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_SendEventAsync_with_NULL_iotHubClientHandle_fails)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendEventAsync(NULL, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, (void*)3);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_011: [IoTHubClient_LL_SendEventAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle or eventMessageHandle is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_SendEventAsync_with_NULL_messageHandle_fails)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendEventAsync(handle, NULL, test_event_confirmation_callback, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_012: [IoTHubClient_LL_SendEventAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter test_event_confirmation_callback is NULL and userContextCallback is not NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_SendEventAsync_with_NULL_test_event_confirmation_callback_and_non_NULL_context_fails)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendEventAsync(handle, TEST_MESSAGE_HANDLE, NULL, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);

}

/*Tests_SRS_IOTHUBCLIENT_LL_02_013: [IoTHubClient_SendEventAsync shall add the DLIST waitingToSend a new record cloning the information from eventMessageHandle, test_event_confirmation_callback, userContextCallback.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_015: [Otherwise IoTHubClient_LL_SendEventAsync shall succeed and return IOTHUB_CLIENT_OK.]*/
TEST_FUNCTION(IoTHubClient_LL_SendEventAsync_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubClient_Diagnostic_AddIfNecessary(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendEventAsync(handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_010: [IoTHubClient_LL_Destroy shall call the underlaying layer's _Destroy function and shall free the resources allocated by IoTHubClient (if any).] */
/*Tests_SRS_IOTHUBCLIENT_LL_02_033: [Otherwise, IoTHubClient_LL_Destroy shall complete all the event message callbacks that are in the waitingToSend list with the result IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY.] */
TEST_FUNCTION(IoTHubClient_LL_Destroy_after_sendEvent_succeeds)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);

    IoTHubClient_LL_SendEventAsync(handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); /*IOTHUBMESSAGE*/

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClient_LL_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_014: [If cloning and/or adding the information fails for any reason, IoTHubClient_LL_SendEventAsync shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SendEventAsync_fails)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t thisIsNotZero = 312984751;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &thisIsNotZero); /*this forces _SendEventAsync to query the currentTime. If that fails, _SendEvent should fail as well*/
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubClient_Diagnostic_AddIfNecessary(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 4 };
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
        sprintf(tmp_msg, "IoTHubClient_LL_Create failure in test %zu/%zu", index, count);

        IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendEventAsync(handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, (void*)1);

        //assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    }

    //cleanup
    IoTHubClient_LL_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_IOTHUBCLIENT_LL_25_111: [IoTHubClient_LL_SetConnectionStatusCallback shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter iotHubClientHandle]*/
TEST_FUNCTION(IoTHubClient_LL_SetConnectionStatusCallback_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetConnectionStatusCallback(NULL, connectionStatusCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_25_112: [IoTHubClient_LL_SetConnectionStatusCallback shall return IOTHUB_CLIENT_OK and save the callback and userContext as a member of the handle.]*/
TEST_FUNCTION(IoTHubClient_LL_SetConnectionStatusCallback_with_non_NULL_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetConnectionStatusCallback(handle, connectionStatusCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_016: [IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback(NULL, messageCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_019: [If parameter messageCallback is NULL then IoTHubClient_LL_SetMessageCallback shall call the underlying layer's _Unsubscribe function and return IOTHUB_CLIENT_OK.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_with_NULL_calls_Unsubscribe_and_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback(handle, test_message_callback_async, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_010: [If parameter messageCallback is NULL and the _SetMessageCallback had not been called to subscribe for messages, then IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_with_NULL_after_SetMessageCallbacEx_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_011: [If parameter messageCallback is non-NULL and the _SetMessageCallback_Ex had been used to susbscribe for messages, then IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_after_SetMessageCallbacEx_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback(handle, messageCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_021: [IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_Ex_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback_Ex(NULL, messageCallbackEx, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_018: [If parameter messageCallback is NULL and IoTHubClient_LL_SetMessageCallback_Ex had not been used to subscribe for messages, then IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_Ex_with_NULL_not_subscribed_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback_Ex(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_019: [If parameter messageCallback is NULL and IoTHubClient_LL_SetMessageCallback had been used to subscribe for messages, then IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_Ex_with_NULL_after_SetMessageCallback_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback(handle, messageCallback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback_Ex(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_023: [If parameter messageCallback is NULL then IoTHubClient_LL_SetMessageCallback_Ex shall call the underlying layer's _Unsubscribe function and return IOTHUB_CLIENT_OK.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_Ex_with_NULL_unsubscribe_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback_Ex(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_025: [If the underlying layer's _Subscribe function fails, then IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR. Otherwise IoTHubClient_LL_SetMessageCallback shall succeed and return IOTHUB_CLIENT_OK.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_Ex_Subscribe_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(__LINE__);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_020: [If parameter messageCallback is non-NULL, and IoTHubClient_LL_SetMessageCallback had been used to subscribe for messages, then IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_Ex_after_SetMessageCallback_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback(handle, messageCallback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_024: [If parameter messageCallback is non-NULL then IoTHubClient_LL_SetMessageCallback_Ex shall call the underlying layer's _Subscribe function.]*/
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_Ex_happy_path_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_026: [IoTHubClient_LL_SendMessageDisposition shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL.]*/
TEST_FUNCTION(IoTHubClient_LL_SendMessageDisposition_with_first_NULL_fails)
{
    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendMessageDisposition(NULL, NULL, IOTHUBMESSAGE_ACCEPTED);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_027: [IoTHubClient_LL_SendMessageDisposition shall return the result from calling the underlying layer's _Send_Message_Disposition.]*/
TEST_FUNCTION(IoTHubClient_LL_SendMessageDisposition_with_second_NULL_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendMessageDisposition(NULL, NULL, IOTHUBMESSAGE_ACCEPTED);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_027: [IoTHubClient_LL_SendMessageDisposition shall return the result from calling the underlying layer's Send_MessageDisposition.]*/
TEST_FUNCTION(IoTHubClient_LL_SendMessageDisposition_Succeeds)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(testMessage, IOTHUBMESSAGE_ACCEPTED));

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendMessageDisposition(handle, testMessage, IOTHUBMESSAGE_ACCEPTED);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_025: [If parameter result is IOTHUB_CLIENT_CONFIRMATION_OK then IoTHubClient_LL_SendComplete shall call all the non-NULL callbacks with the result parameter set to IOTHUB_CLIENT_CONFIRMATION_OK and the context set to the context passed originally in the SendEventAsync call.]*/
TEST_FUNCTION(IoTHubClient_LL_SendComplete_with_1_items_with_callback_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);
    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = eventConfirmationCallback;
    one->context = (void*)1;
    DList_InsertTailList(&temp, &(one->entry));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IoTHubClient_LL_SendComplete(handle, &temp, IOTHUB_CLIENT_CONFIRMATION_OK);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_025: [If parameter result is IOTHUB_CLIENT_CONFIRMATION_OK then IoTHubClient_LL_SendComplete shall call all the non-NULL callbacks with the result parameter set to IOTHUB_CLIENT_CONFIRMATION_OK and the context set to the context passed originally in the SendEventAsync call.]*/
TEST_FUNCTION(IoTHubClient_LL_SendComplete_with_3_items_with_callback_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);

    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = eventConfirmationCallback;
    one->context = (void*)1;
    DList_InsertTailList(&temp, &(one->entry));

    IOTHUB_MESSAGE_LIST* two = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    two->messageHandle = (IOTHUB_MESSAGE_HANDLE)2;
    two->callback = eventConfirmationCallback;
    two->context = (void*)2;
    DList_InsertTailList(&temp, &(two->entry));

    IOTHUB_MESSAGE_LIST* three = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    three->messageHandle = (IOTHUB_MESSAGE_HANDLE)3;
    three->callback = eventConfirmationCallback;
    three->context = (void*)3;
    DList_InsertTailList(&temp, &(three->entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)2));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)2));
    STRICT_EXPECTED_CALL(gballoc_free(two));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)3));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)3));
    STRICT_EXPECTED_CALL(gballoc_free(three));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IoTHubClient_LL_SendComplete(handle, &temp, IOTHUB_CLIENT_CONFIRMATION_OK);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_027: [If parameter result is IOTHUB_BACTCHSTATE_FAILED then IoTHubClient_LL_SendComplete shall call all the non-NULL callbacks with the result parameter set to IOTHUB_CLIENT_CONFIRMATION_ERROR and the context set to the context passed originally in the SendEventAsync call.]*/
TEST_FUNCTION(IoTHubClient_LL_SendComplete_with_3_items_with_callback__but_batch_failed_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);

    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = eventConfirmationCallback;
    one->context = (void*)1;
    DList_InsertTailList(&temp, &(one->entry));

    IOTHUB_MESSAGE_LIST* two = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    two->messageHandle = (IOTHUB_MESSAGE_HANDLE)2;
    two->callback = eventConfirmationCallback;
    two->context = (void*)2;
    DList_InsertTailList(&temp, &(two->entry));

    IOTHUB_MESSAGE_LIST* three = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    three->messageHandle = (IOTHUB_MESSAGE_HANDLE)3;
    three->callback = eventConfirmationCallback;
    three->context = (void*)3;
    DList_InsertTailList(&temp, &(three->entry));


    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_ERROR, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_ERROR, (void*)2));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)2));
    STRICT_EXPECTED_CALL(gballoc_free(two));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_ERROR, (void*)3));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)3));
    STRICT_EXPECTED_CALL(gballoc_free(three));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IoTHubClient_LL_SendComplete(handle, &temp, IOTHUB_CLIENT_CONFIRMATION_ERROR);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}


/* Tests_SRS_IOTHUBCLIENT_LL_25_116: [**IoTHubClient_LL_SetRetryPolicy shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL iotHubClientHandle]*/
TEST_FUNCTION(IoTHubClient_LL_SetRetryPolicy_with_NULL_iotHubClientHandle_fails)
{
    ///arrange
    umock_c_reset_all_calls();
    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetRetryPolicy(NULL, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

TEST_FUNCTION(IoTHubClient_LL_SetRetryPolicy_Retrytimeout_0_pass)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    for (int policy = 0; policy <= 6; policy++)
    {
        STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, (IOTHUB_CLIENT_RETRY_POLICY)policy, 0))
                .IgnoreArgument(1);

        IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetRetryPolicy(handle, (IOTHUB_CLIENT_RETRY_POLICY)policy, 0);
        ///assert

        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    }

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_LL_SetRetryPolicy_success)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_25_114: [**IoTHubClient_LL_ConnectionStatusCallBack shall call non-callback set by the user from IoTHubClient_LL_SetConnectionStatusCallback passing the status, reason and the passed userContextCallback.]*/
TEST_FUNCTION(IoTHubClient_LL_ConnectionStatusCallBack_calls_upper_layer_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetConnectionStatusCallback(handle, connectionStatusCallback, (void*)11);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(connectionStatusCallback((IOTHUB_CLIENT_CONNECTION_STATUS)IGNORED_NUM_ARG, (IOTHUB_CLIENT_CONNECTION_STATUS_REASON)IGNORED_NUM_ARG, (void*)11)).IgnoreAllArguments();

    ///act
    IoTHubClient_LL_ConnectionStatusCallBack(handle, (IOTHUB_CLIENT_CONNECTION_STATUS)IGNORED_NUM_ARG, (IOTHUB_CLIENT_CONNECTION_STATUS_REASON)IGNORED_NUM_ARG);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_25_113: [If parameter connectionStatus is NULL or parameter handle is NULL then IoTHubClient_LL_ConnectionStatusCallBack shall return.] */
TEST_FUNCTION(IoTHubClient_LL_ConnectionStatusCallBack_with_NULL_parameter_fails)
{
    ///arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetConnectionStatusCallback(handle, connectionStatusCallback, (void*)11);
    umock_c_reset_all_calls();

    ///act
    IoTHubClient_LL_ConnectionStatusCallBack(NULL, (IOTHUB_CLIENT_CONNECTION_STATUS)IGNORED_NUM_ARG, (IOTHUB_CLIENT_CONNECTION_STATUS_REASON)IGNORED_NUM_ARG);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_25_113: [If parameter connectionStatus is NULL or parameter handle is NULL then IoTHubClient_LL_ConnectionStatusCallBack shall return.] */
TEST_FUNCTION(IoTHubClient_LL_ConnectionStatusCallBack_with_NULL_callback_fails)
{
    ///arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetConnectionStatusCallback(handle, NULL, (void*)11);
    umock_c_reset_all_calls();

    ///act
    IoTHubClient_LL_ConnectionStatusCallBack(NULL, (IOTHUB_CLIENT_CONNECTION_STATUS)IGNORED_NUM_ARG, (IOTHUB_CLIENT_CONNECTION_STATUS_REASON)IGNORED_NUM_ARG);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_010: [If parameter messageCallback is NULL and the _SetMessageCallback had not been called to subscribe for messages, then IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_with_NULL_before_subscribe_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback(handle, NULL, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_017: [If parameter messageCallback is non-NULL then IoTHubClient_LL_SetMessageCallback shall call the underlying layer's _Subscribe function.]*/
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_with_non_NULL_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback(handle, test_message_callback_async, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_018: [If the underlying layer's _Subscribe function fails, then IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_ERROR. Otherwise IoTHubClient_LL_SetMessageCallback shall succeed and return IOTHUB_CLIENT_OK.]*/
TEST_FUNCTION(IoTHubClient_LL_SetMessageCallback_with_non_fails_when_underlying_transport_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback(handle, test_message_callback_async, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_020: [If parameter iotHubClientHandle is NULL then IoTHubClient_LL_DoWork shall not perform any action.] */
TEST_FUNCTION(IoTHubClient_LL_DoWork_with_NULL_does_nothing)
{
    //arrange

    //act
    IoTHubClient_LL_DoWork(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_021: [Otherwise, IoTHubClient_LL_DoWork shall invoke the underlaying layer's _DoWork function.] */
TEST_FUNCTION(IoTHubClient_LL_DoWork_calls_underlying_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*_DoWork will ask "what's the time"*/
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, handle))
        .IgnoreArgument(1);

    //act
    IoTHubClient_LL_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_022: [If parameter completed is NULL or parameter handle is NULL then IoTHubClient_LL_SendBatch shall return.]*/
TEST_FUNCTION(IoTHubClient_LL_SendComplete_with_NULL_handle_shall_return)
{
    //arrange
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);
    umock_c_reset_all_calls();

    //act
    IoTHubClient_LL_SendComplete(NULL, &temp, IOTHUB_CLIENT_CONFIRMATION_OK);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_022: [If parameter completed is NULL or parameter handle is NULL then IoTHubClient_LL_SendBatch shall return.]*/
TEST_FUNCTION(IoTHubClient_LL_SendComplete_with_NULL_completed_shall_return)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IoTHubClient_LL_SendComplete(handle, NULL, IOTHUB_CLIENT_CONFIRMATION_OK);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_022: [If parameter completed is NULL or parameter handle is NULL then IoTHubClient_LL_SendBatch shall return.]*/
TEST_FUNCTION(IoTHubClient_LL_SendComplete_with_empty_completed_shall_return)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(&temp));

    //act
    IoTHubClient_LL_SendComplete(handle, &temp, IOTHUB_CLIENT_CONFIRMATION_OK);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_025: [If parameter result is IOTHUB_CLIENT_CONFIRMATION_OK then IoTHubClient_LL_SendComplete shall call all the non-NULL callbacks with the result parameter set to IOTHUB_CLIENT_CONFIRMATION_OK and the context set to the context passed originally in the SendEventAsync call.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_026: [If any callback is NULL then there shall not be a callback call.]*/
TEST_FUNCTION(IoTHubClient_LL_SendComplete_with_3_items_one_without_callback_succeeds) /*for fun*/
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);

    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = test_event_confirmation_callback;
    one->context = (void*)1;
    DList_InsertTailList(&temp, &(one->entry));

    IOTHUB_MESSAGE_LIST* two = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    two->messageHandle = (IOTHUB_MESSAGE_HANDLE)2;
    two->callback = NULL;
    two->context = NULL;
    DList_InsertTailList(&temp, &(two->entry));

    IOTHUB_MESSAGE_LIST* three = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    three->messageHandle = (IOTHUB_MESSAGE_HANDLE)3;
    three->callback = test_event_confirmation_callback;
    three->context = (void*)3;
    DList_InsertTailList(&temp, &(three->entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)2));
    STRICT_EXPECTED_CALL(gballoc_free(two));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)3));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)3));
    STRICT_EXPECTED_CALL(gballoc_free(three));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubClient_LL_SendComplete(handle, &temp, IOTHUB_CLIENT_CONFIRMATION_OK);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_026: [If any callback is NULL then there shall not be a callback call.] */
TEST_FUNCTION(IoTHubClient_LL_SendComplete_with_3_items_one_with_callback_but_batch_failed_succeeds)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);

    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = NULL;
    one->context = NULL;
    DList_InsertTailList(&temp, &(one->entry));

    IOTHUB_MESSAGE_LIST* two = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    two->messageHandle = (IOTHUB_MESSAGE_HANDLE)2;
    two->callback = NULL;
    two->context = NULL;
    DList_InsertTailList(&temp, &(two->entry));

    IOTHUB_MESSAGE_LIST* three = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    three->messageHandle = (IOTHUB_MESSAGE_HANDLE)3;
    three->callback = test_event_confirmation_callback;
    three->context = (void*)3;
    DList_InsertTailList(&temp, &(three->entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)2));
    STRICT_EXPECTED_CALL(gballoc_free(two));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_ERROR, (void*)3));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)3));
    STRICT_EXPECTED_CALL(gballoc_free(three));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubClient_LL_SendComplete(handle, &temp, IOTHUB_CLIENT_CONFIRMATION_ERROR);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_029: [If parameter handle is NULL then IoTHubClient_LL_MessageCallback shall return IOTHUBMESSAGE_ABANDONED.] */
TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_handle_NULL_parameter_fails)
{
    //arrange

    //act
    bool result = IoTHubClient_LL_MessageCallback(NULL, (MESSAGE_CALLBACK_INFO*)1);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_029: [If either parameter `handle` or `messageData` is NULL then IoTHubClient_LL_MessageCallback shall return IOTHUBMESSAGE_ABANDONED.] */
TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_messageData_NULL_parameter_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, NULL);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_004: [If messageHandle field of the messageData parameter is NULL then IoTHubClient_LL_MessageCallback shall return false.] */
TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_messageData_messageHandle_is_NULL_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(NULL);
    umock_c_reset_all_calls();

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, testMessage);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_032: [If messageCallbackType is NONE then IoTHubClient_LL_MessageCallback shall report false.] */
TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_unsubscribed_returns_false)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, testMessage);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_030: [If messageCallbackType is LEGACY then IoTHubClient_LL_MessageCallback shall invoke the last callback function (the parameter messageCallback to IoTHubClient_LL_SetMessageCallback) passing the message and the passed userContextCallback.]*/
/*Tests_SRS_IOTHUBCLIENT_LL_10_007: [If messageCallbackType is LEGACY then IoTHubClient_LL_MessageCallback shall send the message disposition as returned by the client to the underlying layer and return true.]*/
TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_messageCallback_calls_client_layer_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback(handle, test_message_callback_async, (void*)11);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(test_message_callback_async(testMessage->messageHandle, (void*)11));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(testMessage, IOTHUBMESSAGE_ACCEPTED));

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, testMessage);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_messageCallback_calls_client_layer_succeeds_report_disposition_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback(handle, test_message_callback_async, (void*)11);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(test_message_callback_async(testMessage->messageHandle, (void*)11));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(testMessage, IOTHUBMESSAGE_ACCEPTED))
        .SetReturn(IOTHUB_CLIENT_ERROR);

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, testMessage);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_messageCallbackEx_calls_client_layer_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)11);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallbackEx(testMessage, (void*)11));

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, testMessage);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_009: [If messageCallbackType is ASYNC then  IoTHubClient_LL_MessageCallback shall return what messageCallbacEx returns.] */
TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_messageCallbackEx_calls_client_layer_reports_true)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)11);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallbackEx(testMessage, (void*)11));

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, testMessage);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_009: [If messageCallbackType is ASYNC then  IoTHubClient_LL_MessageCallback shall return what messageCallbacEx returns.] */
TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_messageCallbackEx_calls_client_layer_reports_false)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)11);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallbackEx(testMessage, (void*)11))
        .SetReturn(false);

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, testMessage);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_LL_MessageCallback_with_messageCallbackEx_calls_client_layer_fails_disposition_reporting_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)11);
    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallbackEx(testMessage, (void*)11))
        .SetReturn(false);

    //act
    bool result = IoTHubClient_LL_MessageCallback(handle, testMessage);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    destroy_test_message_info(testMessage);
    IoTHubClient_LL_Destroy(handle);
}

/*** IoTHubClient_LL_GetLastMessageReceiveTime ***/

/* Tests_SRS_IOTHUBCLIENT_LL_09_001: [IoTHubClient_LL_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_INVALID_ARG if any of the arguments is NULL] */
TEST_FUNCTION(IoTHubClient_LL_GetLastMessageReceiveTime_InvalidClientHandleArg_fails)
{
    // arrange
    time_t lastMessageReceiveTime;

    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetLastMessageReceiveTime(NULL, &lastMessageReceiveTime);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    IoTHubClient_LL_Destroy(iotHubClientHandle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_09_001: [IoTHubClient_LL_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_INVALID_ARG if any of the arguments is NULL] */
TEST_FUNCTION(IoTHubClient_LL_GetLastMessageReceiveTime_InvalidTimeRefArg_fails)
{
    // arrange
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetLastMessageReceiveTime(iotHubClientHandle, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());


    // Cleanup
    IoTHubClient_LL_Destroy(iotHubClientHandle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_09_002: [IoTHubClient_LL_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_INDEFINITE_TIME - and not set 'lastMessageReceiveTime' - if it is unable to provide the time for the last commands] */
TEST_FUNCTION(IoTHubClient_LL_GetLastMessageReceiveTime_NoMessagesReceived_fails)
{
    // arrange
    time_t lastMessageReceiveTime = (time_t)0;

    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetLastMessageReceiveTime(iotHubClientHandle, &lastMessageReceiveTime);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INDEFINITE_TIME, result);
    ASSERT_ARE_EQUAL(int, 0, (int)(lastMessageReceiveTime));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    IoTHubClient_LL_Destroy(iotHubClientHandle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_09_003: [IoTHubClient_LL_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_OK if it wrote in the lastMessageReceiveTime the time when the last command was received] */
/* Tests_SRS_IOTHUBCLIENT_LL_09_004: [IoTHubClient_LL_GetLastMessageReceiveTime shall return lastMessageReceiveTime in localtime] */
TEST_FUNCTION(IoTHubClient_LL_GetLastMessageReceiveTime_MessagesReceived_succeeds)
{
    // arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetMessageCallback(handle, test_message_callback_async, (void*)11);

    MESSAGE_CALLBACK_INFO* testMessage = make_test_message_info(TEST_MESSAGE_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(test_message_callback_async(testMessage->messageHandle, (void*)11));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(testMessage, IOTHUBMESSAGE_ACCEPTED));

    (void)IoTHubClient_LL_MessageCallback(handle, testMessage);

    // act
    time_t lastMessageReceiveTime = (time_t)0;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetLastMessageReceiveTime(handle, &lastMessageReceiveTime);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, (int)TEST_TIME_VALUE, (int)lastMessageReceiveTime);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
    destroy_test_message_info(testMessage);
}

/*** IoTHubClient_LL_GetSendStatus ***/

/* Tests_SRS_IOTHUBCLIENT_09_007: [IoTHubClient_LL_GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter] */
TEST_FUNCTION(IoTHubClient_LL_GetSendStatus_BadHandleArgument_fails)
{
    // arrange

    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetSendStatus(NULL, &status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
}

/* Tests_SRS_IOTHUBCLIENT_09_007: [IoTHubClient_LL_GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter] */
TEST_FUNCTION(IoTHubClient_LL_GetSendStatus_BadStatusArgument_fails)
{
    // arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetSendStatus(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    IoTHubClient_LL_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_09_008: [IoTHubClient_LL_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_IDLE if there is currently no items to be sent] */
TEST_FUNCTION(IoTHubClient_GetSendStatus_NoEventToSend_Succeeds)
{
    // arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS status;
    IOTHUB_CLIENT_STATUS desire_status = IOTHUB_CLIENT_SEND_STATUS_IDLE;

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSendStatus(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .CopyOutArgumentBuffer_iotHubClientStatus(&desire_status, sizeof(status))
        .SetReturn(IOTHUB_CLIENT_OK);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetSendStatus(handle, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_SEND_STATUS_IDLE, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_LL_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_09_009: [IoTHubClient_LL_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_BUSY if there are currently items to be sent] */
TEST_FUNCTION(IoTHubClient_GetSendStatus_HasEventToSend_Succeeds)
{
    // arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS status;
    IOTHUB_CLIENT_STATUS desire_status = IOTHUB_CLIENT_SEND_STATUS_BUSY;

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSendStatus(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .CopyOutArgumentBuffer_iotHubClientStatus(&desire_status, sizeof(status))
        .SetReturn(IOTHUB_CLIENT_OK);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetSendStatus(handle, &status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_SEND_STATUS_BUSY, status);

    // cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_034: [If iotHubClientHandle is NULL then IoTHubClient_LL_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_with_NULL_handle_fails)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(NULL, "a", "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_035: [If optionName is NULL then IoTHubClient_LL_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_LL_SetOption_with_NULL_optionName_fails)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(handle, NULL, "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);

}

/*Tests_SRS_IOTHUBCLIENT_LL_02_036: [If value is NULL then IoTHubClient_LL_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_LL_SetOption_with_NULL_value_fails)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(handle, "a", NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);

}

#ifndef DONT_USE_UPLOADTOBLOB
/*these tests are to be run when upload to blob functionality exists*/

/*Tests_SRS_IOTHUBCLIENT_LL_02_099 : [IoTHubClient_LL_SetOption shall return according to the table below]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_all_combinations)
{

    static struct SetOptionTriplet
    {
        IOTHUB_CLIENT_RESULT IoTHubClient_UploadToBlob_SetOption_return;
        IOTHUB_CLIENT_RESULT Transport_SetOption_return;
        IOTHUB_CLIENT_RESULT expected_return;
    } allCombinations[] =
    {
        { IOTHUB_CLIENT_OK, IOTHUB_CLIENT_OK , IOTHUB_CLIENT_OK },
        { IOTHUB_CLIENT_OK, IOTHUB_CLIENT_ERROR, IOTHUB_CLIENT_ERROR },
        { IOTHUB_CLIENT_OK, IOTHUB_CLIENT_INVALID_ARG, IOTHUB_CLIENT_OK },
        { IOTHUB_CLIENT_ERROR, IOTHUB_CLIENT_OK , IOTHUB_CLIENT_ERROR },
        { IOTHUB_CLIENT_ERROR, IOTHUB_CLIENT_ERROR, IOTHUB_CLIENT_ERROR },
        { IOTHUB_CLIENT_ERROR, IOTHUB_CLIENT_INVALID_ARG, IOTHUB_CLIENT_ERROR },
        { IOTHUB_CLIENT_INVALID_ARG, IOTHUB_CLIENT_OK , IOTHUB_CLIENT_OK },
        { IOTHUB_CLIENT_INVALID_ARG, IOTHUB_CLIENT_ERROR, IOTHUB_CLIENT_ERROR },
        { IOTHUB_CLIENT_INVALID_ARG, IOTHUB_CLIENT_INVALID_ARG, IOTHUB_CLIENT_INVALID_ARG }
    };

    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);


    for (size_t i = 0;i < sizeof(allCombinations) / sizeof(allCombinations[0]);i++)
    {
        umock_c_reset_all_calls();
        EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_SetOption(IGNORED_PTR_ARG, "a", "b"))
            .SetReturn(allCombinations[i].IoTHubClient_UploadToBlob_SetOption_return);

        EXPECTED_CALL(FAKE_IoTHubTransport_SetOption(IGNORED_PTR_ARG, "a", "b"))
            .SetReturn(allCombinations[i].Transport_SetOption_return);

        //act
        IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(handle, "a", "b");

        ///assert
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, allCombinations[i].expected_return, result);
    }

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
    umock_c_reset_all_calls();
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_038: [Otherwise, IoTHubClient_LL shall call the function _SetOption of the underlying transport and return what that function is returning.] */
TEST_FUNCTION(IoTHubClient_LL_SetOption_fails_when_underlying_transport_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_optionName()
        .IgnoreArgument_value()
        .SetReturn(IOTHUB_CLIENT_INVALID_ARG);

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_optionName()
        .IgnoreArgument_value()
        .SetReturn(IOTHUB_CLIENT_INDEFINITE_TIME);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(handle, "a", "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INDEFINITE_TIME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);

}
#else
/*these tests are to be run when uploadtoblob is not present*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_fails_when_underlying_transport_fails)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(FAKE_IoTHubTransport_SetOption(IGNORED_PTR_ARG, "a", "b"))
        .SetReturn(IOTHUB_CLIENT_INDEFINITE_TIME);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(handle, "a", "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INDEFINITE_TIME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);

}
#endif

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_messageTimeout_to_zero_after_Create_succeeds)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    tickcounter_ms_t zero = 0;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(handle, "messageTimeout", &zero);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_messageTimeout_to_one_after_Create_succeeds)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    tickcounter_ms_t one = 1;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);

}

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_messageTimeout_can_be_reverted_back_to_zero_succeeds)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);
    umock_c_reset_all_calls();

    //act
    tickcounter_ms_t zero = 0;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(handle, "messageTimeout", &zero);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_041: [ If more than value miliseconds have passed since the call to IoTHubClient_LL_SendEventAsync then the message callback shall be called with a status code of IOTHUB_CLIENT_CONFIRMATION_TIMEOUT. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_messageTimeout_calls_timeout_callback)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);

    /*send 1 messages that will expire*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);
    umock_c_reset_all_calls();

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    tickcounter_ms_t twelve = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &twelve, sizeof(twelve));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)TEST_DEVICEMESSAGE_HANDLE)); /*calling the callback*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    //act
    IoTHubClient_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_042: [ By default, messages shall not timeout. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_defaults_to_zero)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);

    /*send 1 messages that will expire*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);
    umock_c_reset_all_calls();

    /*we don't care what happens in the Transport, so let's ignore all those calls*/

    tickcounter_ms_t twelve = 12; /*12 > 10 (receive time) + 1 (timeout) => would result in timeout, except the fact that messageTimeout option has never been set, therefore no timeout shall be called*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &twelve, sizeof(twelve));
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    //act
    IoTHubClient_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_messageTimeout_only_removes_the_message_if_it_has_NULL_callback)
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);

    /*send 1 messages that will expire*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, NULL, NULL); /*this is a message with a NULL callback*/
    umock_c_reset_all_calls();

    tickcounter_ms_t twelve = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &twelve, sizeof(twelve));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    //act
    IoTHubClient_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_041: [ If more than value miliseconds have passed since the call to IoTHubClient_LL_SendEventAsync then the message callback shall be called with a status code of IOTHUB_CLIENT_CONFIRMATION_TIMEOUT. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_messageTimeout_when_exactly_on_the_edge_does_not_call_the_callback) /*because "more"*/
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);

    /*send 1 messages that will expire*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);
    umock_c_reset_all_calls();

    tickcounter_ms_t eleven = 11; /*11 = 10 (receive time) + 1 (timeout) => NO timeout*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &eleven, sizeof(eleven));

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    //act
    IoTHubClient_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_041: [ If more than value miliseconds have passed since the call to IoTHubClient_LL_SendEventAsync then the message callback shall be called with a status code of IOTHUB_CLIENT_CONFIRMATION_TIMEOUT. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_2_messages_with_timeouts_at_11_and_12_calls_1_timeout) /*test wants to see that message that did not timeout yet do not have their callbacks called*/
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);

    /*send 2 messages that will expire at 12 and 13, both of these messages are send at time=10*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);

    tickcounter_ms_t two = 2;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &two);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)(TEST_DEVICEMESSAGE_HANDLE_2));
    umock_c_reset_all_calls();

    tickcounter_ms_t twelve = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout!!!*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &twelve, sizeof(twelve));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)TEST_DEVICEMESSAGE_HANDLE)); /*calling the callback*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    /*because we're at time = 12 in this test, the second message is untouched*/

    //act
    IoTHubClient_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_041: [ If more than value miliseconds have passed since the call to IoTHubClient_LL_SendEventAsync then the message callback shall be called with a status code of IOTHUB_CLIENT_CONFIRMATION_TIMEOUT. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_044: [ Messages already delivered to IoTHubClient_LL shall not have their timeouts modified by a new call to IoTHubClient_LL_SetOption. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_2_messages_with_timeouts_at_11_and_12_calls_2_timeouts) /*test wants to see that message that did not timeout yet do not have their callbacks called*/
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);

    /*send 2 messages that will expire at 12 and 13, both of these messages are send at time=10*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);

    tickcounter_ms_t two = 2;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &two);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)(TEST_DEVICEMESSAGE_HANDLE_2));

    umock_c_reset_all_calls();

    /*we don't care what happens in the Transport, so let's ignore all those calls*/

    tickcounter_ms_t timeIsNow = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout!!!*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)TEST_DEVICEMESSAGE_HANDLE)); /*calling the callback*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);

    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    timeIsNow = 13; /*13 > 10 (receive time) + 2 (timeout) => timeout!!!*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)(TEST_DEVICEMESSAGE_HANDLE_2))); /*calling the callback*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);

    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();


    /*because we're at time = 13 in this test, the second message times out too*/

    //act
    IoTHubClient_LL_DoWork(handle);
    IoTHubClient_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a tickcounter_ms_t. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_041: [ If more than value miliseconds have passed since the call to IoTHubClient_LL_SendEventAsync then the message callback shall be called with a status code of IOTHUB_CLIENT_CONFIRMATION_TIMEOUT. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_043: [ Calling IoTHubClient_LL_SetOption with value set to "0" shall disable the timeout mechanism for all new messages. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_02_044: [ Messages already delivered to IoTHubClient_LL shall not have their timeouts modified by a new call to IoTHubClient_LL_SetOption. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_2_messages_one_with_timeout_one_without_call_1_callback) /*test wants to see that message that did not timeout yet do not have their callbacks called*/
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);

    /*send 2 messages that will expire at 12 and 13, both of these messages are send at time=10*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);

    tickcounter_ms_t zero = 0;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &zero); /*essentially no timeout*/
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)(TEST_DEVICEMESSAGE_HANDLE_2));

    umock_c_reset_all_calls();

    /*we don't care what happens in the Transport, so let's ignore all those calls*/

    {/*this scope happen in the first _DoWork call*/
        tickcounter_ms_t timeIsNow = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout!!!*/
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow));

        STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)TEST_DEVICEMESSAGE_HANDLE)); /*calling the callback*/
        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
            .IgnoreArgument(1);
    }

    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    {/*this scope happen in the second _DoWork call*/
        tickcounter_ms_t timeIsNow = 999999999UL; /*some very big number*/
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow));
    }
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    //act
    IoTHubClient_LL_DoWork(handle);
    IoTHubClient_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_041: [ If more than value miliseconds have passed since the call to IoTHubClient_LL_SendEventAsync then the message callback shall be called with a status code of IOTHUB_CLIENT_CONFIRMATION_TIMEOUT. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_messageTimeout_when_tickcounter_fails_in_do_work_no_timeout_callbacks_are_called) /*test wants to see that message that did not timeout yet do not have their callbacks called*/
{
    //arrange

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClient_LL_SetOption(handle, "messageTimeout", &one);

    /*send 2 messages that will expire at 12 and 13, both of these messages are send at time=10*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClient_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);

    umock_c_reset_all_calls();

    {/*this scope happen in the _DoWork call*/
        tickcounter_ms_t timeIsNow = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout!!! (well - normally - but here the code doesn't call any callbacks because time cannot be obtained*/
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow))
            .SetReturn(__FAILURE__);
    }

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    //act
    IoTHubClient_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

#ifndef DONT_USE_UPLOADTOBLOB
/*Tests_SRS_IOTHUBCLIENT_LL_02_061: [ If iotHubClientHandle is NULL then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_with_NULL_handle_fails)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob(NULL, "irrelevantFileName", (const unsigned char*)"a", 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
}
#endif

#ifndef DONT_USE_UPLOADTOBLOB
/*Tests_SRS_IOTHUBCLIENT_LL_02_062: [ If destinationFileName is NULL then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_with_NULL_fileName_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob(h, NULL, (const unsigned char*)"a", 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_02_063: [ If source is NULL and size is greater than 0 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
TEST_FUNCTION(IoTHubClient_LL_UploadToBlob_with_NULL_source_and_size_greater_than_0_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_UploadToBlob(h, "someFileName.txt", NULL, 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(h);
}
#endif 

/* Tests_SRS_IOTHUBCLIENT_LL_10_016: [ Otherwise IoTHubClient_LL_SendReportedState shall succeed and return IOTHUB_CLIENT_OK.] */
TEST_FUNCTION(IoTHubClient_LL_SendReportedState_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_iothubclient_ll_sendreportedstate_mocks();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(handle, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_012: [ IoTHubClient_LL_SendReportedState shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL. ] */
TEST_FUNCTION(IoTHubClient_LL_SendReportedState_NULL_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(NULL, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_013: [ IoTHubClient_LL_SendReportedState shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter reportedState is NULL] */
TEST_FUNCTION(IoTHubClient_LL_SendReportedState_reported_state_NULL_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(handle, NULL, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_LL_SendReportedState_reported_size_0_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();


    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(handle, TEST_REPORTED_STATE, 0, iothub_reported_state_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_015: [ If any error is encountered IoTHubClient_LL_SendReportedState shall return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SendReportedState_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_iothubclient_ll_sendreportedstate_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 4 };

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClient_LL_SendReportedState failure in test %zu/%zu", index, count);

        //act
        IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(handle, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    //cleanup
    IoTHubClient_LL_Destroy(handle);
    umock_c_negative_tests_deinit();
}


/*Tests_SRS_IOTHUBCLIENT_LL_07_008: [ IoTHubClient_LL_DoWork shall iterate the message queue and execute the underlying transports IoTHubTransport_ProcessItem function for each item. ] */
/*Tests_SRS_IOTHUBCLIENT_LL_07_011: [ If 'IoTHubTransport_ProcessItem' returns IOTHUB_PROCESS_OK IoTHubClient_LL_DoWork shall add the IOTHUB_QUEUE_DATA_ITEM to the ack queue. ]*/
TEST_FUNCTION(IoTHubClient_LL_DoWork_SendReportedState_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(h, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*_DoWork will ask "what's the time"*/
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_ProcessItem(IGNORED_PTR_ARG, IOTHUB_TYPE_DEVICE_TWIN, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument_item_type()
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, h))
        .IgnoreArgument(1);

    //act
    IoTHubClient_LL_DoWork(h);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_07_010: [ If 'IoTHubTransport_ProcessItem' returns IOTHUB_PROCESS_CONTINUE or IOTHUB_PROCESS_NOT_CONNECTED IoTHubClient_LL_DoWork shall continue on to call the underlaying layer's _DoWork function. ]*/
TEST_FUNCTION(IoTHubClient_LL_DoWork_SendReportedState_continue_processing_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(h, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*_DoWork will ask "what's the time"*/
        .IgnoreArgument_tick_counter()
        .IgnoreArgument_current_ms();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_ProcessItem(IGNORED_PTR_ARG, IOTHUB_TYPE_DEVICE_TWIN, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument_item_type()
        .IgnoreArgument(3)
        .SetReturn(IOTHUB_PROCESS_CONTINUE);

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG, h))
        .IgnoreArgument(1);

    //act
    IoTHubClient_LL_DoWork(h);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_12_017: [ `IoTHubClient_LL_SetDeviceMethodCallback` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(NULL, deviceMethodCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_12_018: [ If `deviceMethodCallback` is `NULL`, then `IoTHubClient_LL_SetDeviceMethodCallback` shall call the underlying layer's `IoTHubTransport_Unsubscribe_DeviceMethod` function and return `IOTHUB_CLIENT_OK`. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_with_NULL_callback_Unsubscribe_and_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetDeviceMethodCallback(handle, deviceMethodCallback, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_028: [If the user has subscribed using IoTHubClient_LL_SetDeviceMethodCallback_Ex, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_with_NULL_callback_Unsubscribe_After_SetDeviceMethodCallback_Ex_fail)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetDeviceMethodCallback_Ex(handle, iothub_client_inbound_device_method_callback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_029: [ If deviceMethodCallback is NULL and the client is not subscribed to receive method calls, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_unitialized_with_NULL_callback_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetDeviceMethodCallback(NULL, deviceMethodCallback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_12_019: [ If `deviceMethodCallback` is not `NULL`, then `IoTHubClient_LL_SetDeviceMethodCallback` shall call the underlying layer's `IoTHubTransport_Subscribe_DeviceMethod` function. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_with_non_NULL_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(handle, deviceMethodCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_028: [If the user has subscribed using IoTHubClient_LL_SetDeviceMethodCallback_Ex, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_After_SetDeviceMethodCallback_Ex_fail)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetDeviceMethodCallback_Ex(handle, iothub_client_inbound_device_method_callback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(handle, deviceMethodCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_12_020: [ If the underlying layer's `IoTHubTransport_Subscribe_DeviceMethod` function fails, then `IoTHubClient_LL_SetDeviceMethodCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_12_021: [ If adding the information fails for any reason, `IoTHubClient_LL_SetDeviceMethodCallback` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_fails_when_underlying_transport_fails)
{
    ///arrange
    IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(handle, deviceMethodCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClient_LL_Destroy(handle);
}

/*Tests_SRS_IOTHUBCLIENT_LL_07_003: [ IoTHubClient_LL_ReportedStateComplete shall enumerate through the IOTHUB_DEVICE_TWIN structures in queue_handle. ]*/
/*Tests_SRS_IOTHUBCLIENT_LL_07_009: [ IoTHubClient_LL_ReportedStateComplete shall remove the IOTHUB_QUEUE_DATA_ITEM item from the ack queue.]*/
TEST_FUNCTION(IoTHubClient_LL_ReportedStateComplete_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(h, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    IoTHubClient_LL_DoWork(h);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothub_reported_state_callback(IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubClient_LL_ReportedStateComplete(h, 2, TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_07_002: [ if handle is NULL then IoTHubClient_LL_ReportedStateComplete shall do nothing. ]*/
TEST_FUNCTION(IoTHubClient_LL_ReportedStateComplete_NULL_fail)
{
    //arrange

    //act
    IoTHubClient_LL_ReportedStateComplete(NULL, 2, TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_018: [ If deviceMethodCallback is not NULL IoTHubClient_LL_DeviceMethodComplete shall execute deviceMethodCallback and return the status. ] */
TEST_FUNCTION(IoTHubClient_LL_DeviceMethodComplete_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(h, deviceMethodCallback, (void*)1);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    size_t len = 1;
    unsigned char* resp = (unsigned char*)my_gballoc_malloc(len);
    resp[0] = 0xa;

    STRICT_EXPECTED_CALL(deviceMethodCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_method_name()
        .IgnoreArgument_payload()
        .IgnoreArgument_size()
        .IgnoreArgument_userContextCallback()
        .CopyOutArgumentBuffer_response(&resp, sizeof(unsigned char**))
        .CopyOutArgumentBuffer_resp_size(&len, sizeof(size_t));

    EXPECTED_CALL(FAKE_IoTHubTransport_DeviceMethod_Response(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    //act
    int status = IoTHubClient_LL_DeviceMethodComplete(h, TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID);

    //assert
    ASSERT_ARE_EQUAL(int, 0, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_018: [ If deviceMethodCallback is not NULL IoTHubClient_LL_DeviceMethodComplete shall execute deviceMethodCallback and return the status. ] */
TEST_FUNCTION(IoTHubClient_LL_DeviceMethodComplete_payload_NULL_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback(h, deviceMethodCallback, (void*)1);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    size_t len = 0;
    unsigned char* resp = NULL;

    STRICT_EXPECTED_CALL(deviceMethodCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_method_name()
        .IgnoreArgument_payload()
        .IgnoreArgument_size()
        .IgnoreArgument_userContextCallback()
        .CopyOutArgumentBuffer_response(&resp, sizeof(unsigned char**))
        .CopyOutArgumentBuffer_resp_size(&len, sizeof(size_t));

    //act
    int status = IoTHubClient_LL_DeviceMethodComplete(h, TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_DeviceMethodComplete_Async_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothub_client_inbound_device_method_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    int status = IoTHubClient_LL_DeviceMethodComplete(h, TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID);

    //assert
    ASSERT_ARE_EQUAL(int, 0, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_019: [ If deviceMethodCallback is NULL IoTHubClient_LL_DeviceMethodComplete shall return 404. ] */
TEST_FUNCTION(IoTHubClient_LL_DeviceMethodComplete_No_callback_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    int result = IoTHubClient_LL_DeviceMethodComplete(h, TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_017: [ If `handle` or response is NULL then `IoTHubClient_LL_DeviceMethodComplete` shall return 500. ] */
TEST_FUNCTION(IoTHubClient_LL_DeviceMethodComplete_handle_NULL_succeed)
{
    //arrange

    //act
    int result = IoTHubClient_LL_DeviceMethodComplete(NULL, TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_001: [ IoTHubClient_LL_SetDeviceTwinCallback shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL.] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceTwinCallback_iothubclienthandle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceTwinCallback(NULL, iothub_device_twin_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_016: [ If deviceTwinCallback is set and DEVICE_TWIN_UPDATE_COMPLETE has been encountered then IoTHubClient_LL_RetrievePropertyComplete shall call deviceTwinCallback.] */
TEST_FUNCTION(IoTHubClient_LL_RetrievePropertyComplete_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothub_device_twin_callback(DEVICE_TWIN_UPDATE_COMPLETE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_update_state()
        .IgnoreArgument_userContextCallback()
        .IgnoreArgument_payLoad()
        .IgnoreArgument_size();

    //act
    IoTHubClient_LL_RetrievePropertyComplete(h, DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_013: [ If handle is NULL then IoTHubClient_LL_RetrievePropertyComplete shall do nothing.] */
TEST_FUNCTION(IoTHubClient_LL_RetrievePropertyComplete_iothubclienthandle_NULL_fail)
{
    //arrange

    //act
    IoTHubClient_LL_RetrievePropertyComplete(NULL, DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_014: [ If deviceTwinCallback is NULL then IoTHubClient_LL_RetrievePropertyComplete shall do nothing.] */
TEST_FUNCTION(IoTHubClient_LL_RetrievePropertyComplete_deviceTwincallback_NULL_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);

    umock_c_reset_all_calls();

    //act
    IoTHubClient_LL_RetrievePropertyComplete(h, DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_015: [ If the the update_state parameter is DEVICE_TWIN_UPDATE_PARTIAL and a DEVICE_TWIN_UPDATE_COMPLETE has not been previously recieved then IoTHubClient_LL_RetrievePropertyComplete shall do nothing.] */
TEST_FUNCTION(IoTHubClient_LL_RetrievePropertyComplete_update_partial_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    //act
    IoTHubClient_LL_RetrievePropertyComplete(h, DEVICE_TWIN_UPDATE_PARTIAL, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_016: [ If deviceTwinCallback is set and DEVICE_TWIN_UPDATE_COMPLETE has been encountered then IoTHubClient_LL_RetrievePropertyComplete shall call deviceTwinCallback.] */
TEST_FUNCTION(IoTHubClient_LL_RetrievePropertyComplete_update_partial_before_complete_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothub_device_twin_callback(DEVICE_TWIN_UPDATE_PARTIAL, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_update_state()
        .IgnoreArgument_userContextCallback()
        .IgnoreArgument_payLoad()
        .IgnoreArgument_size();

    //act
    IoTHubClient_LL_RetrievePropertyComplete(h, DEVICE_TWIN_UPDATE_PARTIAL, NULL, 0);
    IoTHubClient_LL_RetrievePropertyComplete(h, DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_006: [ If deviceTwinCallback is NULL, then IoTHubClient_LL_SetDeviceTwinCallback shall call the underlying layer's _Unsubscribe function and return IOTHUB_CLIENT_OK.] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceTwinCallback_unsubscribe_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe_DeviceTwin(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceTwinCallback(h, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_002: [ If deviceTwinCallback is not NULL, then IoTHubClient_LL_SetDeviceTwinCallback shall call the underlying layer's _Subscribe function.] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceTwinCallback_subscribe_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceTwin(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_003: [ If the underlying layer's _Subscribe function fails, then IoTHubClient_LL_SetDeviceTwinCallback shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceTwinCallback_subscribe_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceTwin(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(__FAILURE__);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Test_SRS_IOTHUBCLIENT_LL_07_021: [ If handle is NULL then IoTHubClient_LL_SetDeviceMethodCallback_Ex shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_Ex_handle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(NULL, iothub_client_inbound_device_method_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_025: [ If any error is encountered then IoTHubClient_LL_SetDeviceMethodCallback_Ex shall return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_Ex_subscribe_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(__FAILURE__);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_022: [ If inboundDeviceMethodCallback is NULL then IoTHubClient_LL_SetDeviceMethodCallback_Ex shall call the underlying layer's IoTHubTransport_Unsubscribe_DeviceMethod function and return IOTHUB_CLIENT_OK.] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_Ex_inbound_device_method_cb_NULL_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(h, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_031: [If the user has subscribed using IoTHubClient_LL_SetDeviceMethodCallback, IoTHubClient_LL_SetDeviceMethodCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR. ] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_Ex_inbound_device_method_cb_NULL_After_SetDeviceMethodCallback_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetDeviceMethodCallback(h, deviceMethodCallback, (void*)1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(h, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_030: [ If deviceMethodCallback is NULL and the client is not subscribed to receive method calls, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_Ex_uninitialized_inbound_device_method_cb_NULL_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(h, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_10_031: [If the user has subscribed using IoTHubClient_LL_SetDeviceMethodCallback, IoTHubClient_LL_SetDeviceMethodCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR. ] */
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_Ex_After_SetDeviceMethodCallback_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    (void)IoTHubClient_LL_SetDeviceMethodCallback(h, deviceMethodCallback, (void*)1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_023: [ If inboundDeviceMethodCallback is non-NULL then IoTHubClient_LL_SetDeviceMethodCallback_Ex shall call the underlying layer's IoTHubTransport_Subscribe_DeviceMethod function.]*/
TEST_FUNCTION(IoTHubClient_LL_SetDeviceMethodCallback_Ex_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_026: [ If handle or methodId is NULL then IoTHubClient_LL_DeviceMethodResponse shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_LL_DeviceMethodResponse_handle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_DeviceMethodResponse(NULL, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_026: [ If handle or methodId is NULL then IoTHubClient_LL_DeviceMethodResponse shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_LL_DeviceMethodResponse_method_id_NULL_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_DeviceMethodResponse(h, NULL, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_026: [ If handle or methodId is NULL then IoTHubClient_LL_DeviceMethodResponse shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubClient_LL_DeviceMethodResponse_IoTHubTransport_DeviceMethod_Response_FAILS_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DeviceMethod_Response(IGNORED_PTR_ARG, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE)).SetReturn(__FAILURE__);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_DeviceMethodResponse(h, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_027: [ IoTHubClient_LL_DeviceMethodResponse shall call the IoTHubTransport_DeviceMethod_Response transport function.] */
TEST_FUNCTION(IoTHubClient_LL_DeviceMethodResponse_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DeviceMethod_Response(IGNORED_PTR_ARG, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE))
        .IgnoreArgument(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_DeviceMethodResponse(h, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}


/* Tests_SRS_IOTHUBCLIENT_LL_25_120: [If iotHubClientHandle, retryPolicy or retryTimeoutLimitinSeconds is NULL, IoTHubClient_LL_GetRetryPolicy shall return IOTHUB_CLIENT_INVALID_ARG ] */
TEST_FUNCTION(IoTHubClient_LL_GetRetryPolicy_NULL_HANDLEParam_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetRetryPolicy(NULL, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUBCLIENT_LL_25_120: [If iotHubClientHandle, retryPolicy or retryTimeoutLimitinSeconds is NULL, IoTHubClient_LL_GetRetryPolicy shall return `IOTHUB_CLIENT_INVALID_ARG ] */
TEST_FUNCTION(IoTHubClient_LL_GetRetryPolicy_NULL_RetryPolicyParam_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetRetryPolicy(h, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_25_120: [If iotHubClientHandle, retryPolicy or retryTimeoutLimitinSeconds is NULL, IoTHubClient_LL_GetRetryPolicy shall return `IOTHUB_CLIENT_INVALID_ARG ] */
TEST_FUNCTION(IoTHubClient_LL_GetRetryPolicy_NULL_SizeParam_fail)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RETRY_POLICY r;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetRetryPolicy(h, &r, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/* Tests_SRS_IOTHUBCLIENT_LL_25_121: [IoTHubClient_LL_GetRetryPolicy shall retrieve connection retry policy from retryPolicy in struct IOTHUB_CLIENT_LL_HANDLE_DATA] */
/* Tests_SRS_IOTHUBCLIENT_LL_25_122: [IoTHubClient_LL_GetRetryPolicy shall retrieve retryTimeoutLimit in seconds from retryTimeoutinSeconds in struct IOTHUB_CLIENT_LL_HANDLE_DATA] */
TEST_FUNCTION(IoTHubClient_LL_GetRetryPolicy_succeed)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RETRY_POLICY r;
    size_t s;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_GetRetryPolicy(h, &r, &s);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_SetOption_product_info_twice_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(platform_get_platform_info());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(platform_get_platform_info());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");
    if (result == IOTHUB_CLIENT_OK)
    {
        result = IoTHubClient_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");
    }

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_SetOption_product_info_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(platform_get_platform_info());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_SetOption_product_info_fails_case2)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(platform_get_platform_info());

    //act
    g_fail_platform_get_platform_info = true;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_LL_SetOption_product_info_fails_case1)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(platform_get_platform_info());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    g_fail_string_construct_sprintf = true;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_037: [Calling IoTHubClient_LL_SetOption with value between [0, 100] shall return `IOTHUB_CLIENT_OK`. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_diag_sampling_percentage_succeeds)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    uint32_t diagPercentage = 9;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(h, OPTION_DIAGNOSTIC_SAMPLING_PERCENTAGE, &diagPercentage);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}

/*Tests_SRS_IOTHUBCLIENT_LL_10_036: [Calling IoTHubClient_LL_SetOption with value > 100 shall return `IOTHUB_CLIENT_ERRROR`. ]*/
TEST_FUNCTION(IoTHubClient_LL_SetOption_diag_sampling_percentage_fails)
{
    //arrange
    IOTHUB_CLIENT_LL_HANDLE h = IoTHubClient_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    uint32_t diagPercentage = 101;
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetOption(h, OPTION_DIAGNOSTIC_SAMPLING_PERCENTAGE, &diagPercentage);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_LL_Destroy(h);
}


END_TEST_SUITE(iothubclient_ll_ut)
