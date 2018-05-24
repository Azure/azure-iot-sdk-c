// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
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
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"

#include "real_constbuffer.h"
#define ENABLE_MOCKS

#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"

#include "azure_umqtt_c/mqtt_client.h"

#include "internal/iothub_client_private.h"
#include "iothub_client_options.h"
#include "internal/iothub_client_retry_control.h"

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/tlsio.h"

#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/urlencode.h"
#undef ENABLE_MOCKS

#include "internal/iothubtransport_mqtt_common.h"
#include "azure_c_shared_utility/strings.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...);
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

#ifdef __cplusplus
}
#endif

static STRING_HANDLE my_STRING_new(void)
{
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static int my_STRING_concat_with_STRING(STRING_HANDLE handle, STRING_HANDLE data)
{
    (void)handle;
    (void)data;
    return 0;
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    my_gballoc_free(handle);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t l = strlen(source);
    *destination = (char*)my_gballoc_malloc(l + 1);
    strcpy(*destination, source);
    return 0;
}

static STRING_HANDLE my_URL_Encode(STRING_HANDLE string)
{
    (void)string;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_URL_EncodeString(const char* textEncode)
{
    (void)textEncode;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_URL_DecodeString(const char* textDecode)
{
    (void)textDecode;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_GetOption(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* optionName, void** value)
{
    (void)iotHubClientHandle;
    (void)optionName;

    *value = (STRING_HANDLE)0x87;
    return IOTHUB_CLIENT_OK;
}

static const char* TEST_STRING_VALUE = "Test string value";
static const char* TEST_DEVICE_ID = "thisIsDeviceID";
static const char* TEST_DEVICE_KEY = "thisIsDeviceKey";
static const char* TEST_DEVICE_SAS = "thisIsDeviceSasToken";
static const char* TEST_IOTHUB_NAME = "thisIsIotHubName";
static const char* TEST_IOTHUB_SUFFIX = "thisIsIotHubSuffix";
static const char* TEST_PROTOCOL_GATEWAY_HOSTNAME = NULL;
static const char* TEST_PROTOCOL_GATEWAY_HOSTNAME_NON_NULL = "ssl://thisIsAGatewayHostName.net";
static const char* TEST_VERY_LONG_DEVICE_ID = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
static const char* TEST_MQTT_MESSAGE_TOPIC = "devices/thisIsDeviceID/messages/devicebound/#";
static const char* TEST_MQTT_MSG_TOPIC = "devices/jebrandoDevice/messages/devicebound/iothub-ack=Full&%24.to=%2Fdevices%2FjebrandoDevice%2Fmessages%2FdeviceBound&%24.cid&%24.uid";
static const char* TEST_MQTT_MSG_TOPIC_W_1_PROP = "devices/thisIsDeviceID/messages/devicebound/iothub-ack=Full&propName=PropValue&DeviceInfo=smokeTest&%24.to=%2Fdevices%2FjebrandoDevice%2Fmessages%2FdeviceBound&%24.cid&%24.uid";
static const char* TEST_MQTT_DEV_TWIN_MSG_TOPIC = "$iothub/twin/$res/200/?$rid=2";
static const char* TEST_MQTT_DEV_METHOD_MSG = "$iothub/methods/POST/method_name/?$rid=b";

static const char* TEST_MQTT_EVENT_TOPIC = "devices/thisIsDeviceID/messages/events/";
static const char* TEST_MQTT_SAS_TOKEN = "thisIsIotHubName.thisIsIotHubSuffix/devices/thisIsDeviceID";
static const char* TEST_HOST_NAME = "thisIsIotHubName.thisIsIotHubSuffix";
static const char* TEST_EMPTY_STRING = "";
static const char* TEST_SAS_TOKEN = "Test_SAS_Token_value";
static const char* X509_CERT_CERTIFICATE = "-----BEGIN CERTIFICATE-----MIID-----END CERTIFICATE-----";
static const char* X509_PRIVATE_KEY_OPTION = "x509privatekey";
static const char* X509_PRIVATE_KEY = "-----BEGIN RSA PRIVATE KEY-----MIIE-----END RSA PRIVATE KEY-----";

static const char* TEST_CONTENT_TYPE = "application/json";
static const char* TEST_CONTENT_ENCODING = "utf8";
static const char* TEST_DIAG_ID = "1234abcd";
static const char* TEST_DIAG_CREATION_TIME_UTC = "1506054516.100";

static const char* PROPERTY_SEPARATOR = "&";
static const char* DIAGNOSTIC_CONTEXT_CREATION_TIME_UTC_PROPERTY = "creationtimeutc";

static IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA TEST_DIAG_DATA;

static MQTT_TRANSPORT_PROXY_OPTIONS* expected_MQTT_TRANSPORT_PROXY_OPTIONS;

static const IOTHUB_CLIENT_CORE_LL_HANDLE TEST_IOTHUB_CLIENT_CORE_LL_HANDLE = (IOTHUB_CLIENT_CORE_LL_HANDLE)0x4343;
static const TRANSPORT_LL_HANDLE TEST_TRANSPORT_HANDLE = (TRANSPORT_LL_HANDLE)0x4444;
static const MQTT_CLIENT_HANDLE TEST_MQTT_CLIENT_HANDLE = (MQTT_CLIENT_HANDLE)0x1122;
static const PDLIST_ENTRY TEST_PDLIST_ENTRY = (PDLIST_ENTRY)0x1123;
static const MQTT_MESSAGE_HANDLE TEST_MQTT_MESSAGE_HANDLE = (MQTT_MESSAGE_HANDLE)0x1124;

static const IOTHUB_CLIENT_TRANSPORT_PROVIDER TEST_PROTOCOL = (IOTHUB_CLIENT_TRANSPORT_PROVIDER)0x1127;

static XIO_HANDLE TEST_XIO_HANDLE = (XIO_HANDLE)0x1126;

static const STRING_TOKENIZER_HANDLE TEST_STRING_TOKENIZER_HANDLE = (STRING_TOKENIZER_HANDLE)0x1127;

static const IOTHUB_AUTHORIZATION_HANDLE TEST_IOTHUB_AUTHORIZATION_HANDLE = (IOTHUB_AUTHORIZATION_HANDLE)0x1128;

/*this is the default message and has type BYTEARRAY*/
static const IOTHUB_MESSAGE_HANDLE TEST_IOTHUB_MSG_BYTEARRAY = (const IOTHUB_MESSAGE_HANDLE)0x01d1;

/*this is a STRING type message*/
static IOTHUB_MESSAGE_HANDLE TEST_IOTHUB_MSG_STRING = (IOTHUB_MESSAGE_HANDLE)0x01d2;

static const TICK_COUNTER_HANDLE TEST_COUNTER_HANDLE = (TICK_COUNTER_HANDLE)0x12;
static const MAP_HANDLE TEST_MESSAGE_PROP_MAP = (MAP_HANDLE)0x1212;

static char appMessageString[] = "App Message String";
static uint8_t appMessage[] = { 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x4d, 0x73, 0x67 };
static const size_t appMsgSize = sizeof(appMessage) / sizeof(appMessage[0]);

static IOTHUB_CLIENT_CONFIG g_iothubClientConfig = { 0 };
static DLIST_ENTRY g_waitingToSend;

static tickcounter_ms_t g_current_ms = 0;
static size_t g_tokenizerIndex;

static const unsigned char* TEST_DEVICE_METHOD_RESPONSE = (const unsigned char*)0x62;
static size_t TEST_DEVICE_RESP_LENGTH = 1;
static size_t TEST_METHOD_ID_VALUE = 12;
static METHOD_HANDLE TEST_METHOD_ID = &TEST_METHOD_ID_VALUE;
static METHOD_HANDLE g_method_handle_value = NULL;

#define TEST_TIME_T ((time_t)-1)
#define TEST_DIFF_TIME TEST_DIFF_TIME_POSITIVE
#define TEST_DIFF_TIME_POSITIVE 12
#define TEST_DIFF_TIME_NEGATIVE -12
#define TEST_DIFF_WITHIN_ERROR  5
#define TEST_DIFF_GREATER_THAN_WAIT  6
#define TEST_DIFF_LESS_THAN_WAIT  1
#define TEST_DIFF_GREATER_THAN_ERROR 10
#define TEST_BIG_TIME_T (TEST_RETRY_TIMEOUT_SECS - TEST_DIFF_WITHIN_ERROR)
#define TEST_SMALL_TIME_T ((time_t)(TEST_DIFF_WITHIN_ERROR - 1))
#define TEST_DEVICE_STATUS_CODE     200
#define TEST_HOSTNAME_STRING_HANDLE    (STRING_HANDLE)0x5555
#define TEST_RETRY_CONTROL_HANDLE      (RETRY_CONTROL_HANDLE)0x6666

#define DEFAULT_RETRY_POLICY                IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define DEFAULT_RETRY_TIMEOUT_IN_SECONDS    0

static APP_PAYLOAD TEST_APP_PAYLOAD;

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(SAS_TOKEN_STATUS, SAS_TOKEN_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SAS_TOKEN_STATUS, SAS_TOKEN_STATUS_VALUES);

static TEST_MUTEX_HANDLE test_serialize_mutex;
static TEST_MUTEX_HANDLE g_dllByDll;

static IOTHUBMESSAGE_DISPOSITION_RESULT g_msg_disposition;

#define TEST_RETRY_POLICY IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define TEST_RETRY_TIMEOUT_SECS 60

//Callbacks for Testing
static ON_MQTT_MESSAGE_RECV_CALLBACK g_fnMqttMsgRecv;
static ON_MQTT_OPERATION_CALLBACK g_fnMqttOperationCallback;
static ON_MQTT_ERROR_CALLBACK g_fnMqttErrorCallback;
static void* g_callbackCtx;
static void* g_errorcallbackCtx;
static bool g_nullMapVariable;

#ifdef __cplusplus
extern "C"
{
#endif
    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
    {
        (void)handle;
        (void)format;
        return 0;
    }

    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        return (STRING_HANDLE)my_gballoc_malloc(1);
    }

    const char* my_STRING_c_str(STRING_HANDLE handle)
    {
        (void)handle;
        return TEST_STRING_VALUE;
    }

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

static char* my_IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, size_t expiry_time_relative_seconds, const char* keyname)
{
    (void)handle;
    (void)scope;
    (void)expiry_time_relative_seconds;
    (void)keyname;

    char* result;
    size_t len = strlen(TEST_SAS_TOKEN);
    result = (char*)my_gballoc_malloc(len+1);
    strcpy(result, TEST_SAS_TOKEN);
    return result;
}

static IOTHUBMESSAGE_CONTENT_TYPE my_IoTHubMessage_GetContentType(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    IOTHUBMESSAGE_CONTENT_TYPE result2;
    if (iotHubMessageHandle == TEST_IOTHUB_MSG_BYTEARRAY)
    {
        result2 = IOTHUBMESSAGE_BYTEARRAY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MSG_STRING)
    {
        result2 = IOTHUBMESSAGE_STRING;
    }
    else
    {
        result2 = IOTHUBMESSAGE_UNKNOWN;
    }
    return result2;
}

static IOTHUB_MESSAGE_RESULT my_IoTHubMessage_GetByteArray(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const unsigned char** buffer, size_t* size)
{
    if (iotHubMessageHandle == TEST_IOTHUB_MSG_BYTEARRAY)
    {
        *buffer = appMessage;
        *size = appMsgSize;
    }
    else
    {
        /*not expected really*/
        *buffer = (const unsigned char*)"333";
        *size = 3;
    }
    return IOTHUB_MESSAGE_OK;
}

static void my_IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    (void)iotHubMessageHandle;
}

static int my_IoTHubClientCore_LL_DeviceMethodComplete(IOTHUB_CLIENT_CORE_LL_HANDLE handle, const char* method_name, const unsigned char* payLoad, size_t size, METHOD_HANDLE response_id)
{
    (void)handle;
    (void)method_name;
    (void)payLoad;
    (void)size;
    g_method_handle_value = response_id;
    return 0;
}

static void my_IoTHubClientCore_LL_SendComplete(IOTHUB_CLIENT_CORE_LL_HANDLE handle, PDLIST_ENTRY completed, IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
    (void)handle;
    (void)completed;
    (void)result;
}

static void my_IoTHubClientCore_LL_ConnectionStatusCallBack(IOTHUB_CLIENT_CORE_LL_HANDLE handle, IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    (void)handle;
    (void)status;
    (void)reason;
}

static bool my_IoTHubClientCore_LL_MessageCallback(IOTHUB_CLIENT_CORE_LL_HANDLE handle, MESSAGE_CALLBACK_INFO* message_data)
{
    (void)handle;

    bool result;
    if (IOTHUBMESSAGE_ABANDONED == g_msg_disposition)
    {
        result = false;
    }
    else
    {
        IoTHubMessage_Destroy(message_data->messageHandle);
        free(message_data);
        result = true;
    }

    return result;
}

static MQTT_CLIENT_HANDLE my_mqtt_client_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* callbackCtx, ON_MQTT_ERROR_CALLBACK errorCallback, void* errorcallbackCtx)
{
    g_fnMqttMsgRecv = msgRecv;
    g_fnMqttOperationCallback = opCallback;
    g_callbackCtx = callbackCtx;
    g_fnMqttErrorCallback = errorCallback;
    g_errorcallbackCtx = errorcallbackCtx;
    return (MQTT_CLIENT_HANDLE)my_gballoc_malloc(12);
}

static void my_mqtt_client_deinit(MQTT_CLIENT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static void my_mqtt_client_dowork(MQTT_CLIENT_HANDLE handle)
{
    (void)handle;
}

static STRING_TOKENIZER_HANDLE my_STRING_TOKENIZER_create(STRING_HANDLE handle)
{
    (void)handle;
    return (STRING_TOKENIZER_HANDLE)my_gballoc_malloc(1);
}

static STRING_TOKENIZER_HANDLE my_STRING_TOKENIZER_create_from_char(const char* input)
{
    (void)input;
    return (STRING_TOKENIZER_HANDLE)my_gballoc_malloc(1);
}

int my_STRING_TOKENIZER_get_next_token(STRING_TOKENIZER_HANDLE t, STRING_HANDLE output, const char* delimiters)
{
    int result;
    (void)delimiters;
    (void)t;
    const char* text = NULL;
    switch (g_tokenizerIndex++)
    {
        case 0:
            break;
        case 1:
            text = "devices/thisIsDeviceID/messages/devicebound/iothub-ack=Full";
            break;
        case 2:
            text = "propname1=value2";
            break;
        case 3:
            text = "propname2=value2";
            break;
        case 4:
            text = "%24.to=%2Fdevices%2FjebrandoDevice%2Fmessages%2FdeviceBound&%24.cid";
            break;
        case 5:
            text = "%24.cid";
            break;
        case 6:
            text = ""; // Done
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            text = "200";
            break;
        case 12:
            text = "2";
            break;
        case 7:
        default:
            break;
    }
    if (text != NULL)
    {
        result = 0;
        output = NULL;
    }
    else
    {
        result = __FAILURE__;
    }
    return result;
}

static void my_STRING_TOKENIZER_destroy(STRING_TOKENIZER_HANDLE handle)
{
    my_gballoc_free(handle);
}

static STRING_HANDLE my_SASToken_Create(STRING_HANDLE key, STRING_HANDLE scope, STRING_HANDLE keyName, size_t expiry)
{
    (void)key;
    (void)scope;
    (void)keyName;
    (void)expiry;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static MAP_RESULT my_Map_GetInternals(MAP_HANDLE handle, const char*const** keys, const char*const** values, size_t* count)
{
    (void)handle;
    *keys = NULL;
    *values = NULL;
    *count = 0;
    return MAP_OK;
}

static XIO_HANDLE my_xio_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, const void* xio_create_parameters)
{
    (void)io_interface_description;
    (void)xio_create_parameters;
    return (XIO_HANDLE)my_gballoc_malloc(1);
}

static void my_xio_destroy(XIO_HANDLE ioHandle)
{
    my_gballoc_free(ioHandle);
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

static MAP_HANDLE my_Map_Create(MAP_FILTER_CALLBACK mapFilterFunc)
{
    (void)mapFilterFunc;
    return (MAP_HANDLE)malloc(1);
}

static MAP_HANDLE my_Map_Clone(MAP_HANDLE handle)
{
    (void)handle;
    return (MAP_HANDLE)malloc(1);
}

static void my_Map_Destroy(MAP_HANDLE handle)
{
    free(handle);
}

static BUFFER_HANDLE my_BUFFER_new(void)
{
    return (BUFFER_HANDLE)malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    free(handle);
}

double my_get_difftime(time_t stopTime, time_t startTime)
{
    return (double)(stopTime - startTime);
}

static int error_proxy_options;
static XIO_HANDLE get_IO_transport(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options)
{
    (void)fully_qualified_name;
    (void)mqtt_transport_proxy_options;
    error_proxy_options = 0;
    if (((expected_MQTT_TRANSPORT_PROXY_OPTIONS == NULL) && (mqtt_transport_proxy_options != NULL)) ||
        ((expected_MQTT_TRANSPORT_PROXY_OPTIONS != NULL) && (mqtt_transport_proxy_options == NULL)))
    {
        error_proxy_options = 1;
    }
    else
    {
        if (expected_MQTT_TRANSPORT_PROXY_OPTIONS != NULL)
        {
            if ((strcmp(expected_MQTT_TRANSPORT_PROXY_OPTIONS->host_address, mqtt_transport_proxy_options->host_address) != 0) ||
                ((expected_MQTT_TRANSPORT_PROXY_OPTIONS->username != NULL) && (strcmp(expected_MQTT_TRANSPORT_PROXY_OPTIONS->username, mqtt_transport_proxy_options->username) != 0)) ||
                ((expected_MQTT_TRANSPORT_PROXY_OPTIONS->password != NULL) && (strcmp(expected_MQTT_TRANSPORT_PROXY_OPTIONS->password, mqtt_transport_proxy_options->password) != 0)))
            {
                error_proxy_options = 1;
            }
        }
    }
    return (XIO_HANDLE)my_gballoc_malloc(1);
}

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

BEGIN_TEST_SUITE(iothubtransport_mqtt_common_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_APP_PAYLOAD.message = appMessage;
    TEST_APP_PAYLOAD.length = appMsgSize;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    umock_c_init(on_umock_c_error);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MQTT_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_OPERATION_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_ERROR_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_CLOSE_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(QOS_VALUE, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(MQTT_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_MESSAGE_RECV_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_TOKENIZER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, int);
    REGISTER_UMOCK_ALIAS_TYPE(time_t, uint64_t);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CREDENTIAL_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(SAS_TOKEN_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_DISCONNECTED_CALLBACK, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_realloc, my_gballoc_realloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_realloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat_with_STRING, my_STRING_concat_with_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, -1);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, my_STRING_c_str);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(URL_Encode, my_URL_Encode);
    REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(URL_DecodeString, my_URL_DecodeString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_DecodeString, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_GetOption, my_IoTHubClientCore_LL_GetOption);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceKey, TEST_DEVICE_KEY);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_MessageCallback, my_IoTHubClientCore_LL_MessageCallback);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_ConnectionStatusCallBack, my_IoTHubClientCore_LL_ConnectionStatusCallBack);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_SendComplete, my_IoTHubClientCore_LL_SendComplete);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_DeviceMethodComplete, my_IoTHubClientCore_LL_DeviceMethodComplete);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_DeviceMethodComplete, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_GetContentType, my_IoTHubMessage_GetContentType);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetContentType, IOTHUBMESSAGE_UNKNOWN);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetString, appMessageString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetString, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_CreateFromByteArray, TEST_IOTHUB_MSG_BYTEARRAY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_CreateFromByteArray, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_GetByteArray, my_IoTHubMessage_GetByteArray);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_Destroy, my_IoTHubMessage_Destroy);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_Properties, TEST_MESSAGE_PROP_MAP);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_Properties, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(Map_GetInternals, my_Map_GetInternals);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetInternals, MAP_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(Map_AddOrUpdate, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetInternals, MAP_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(Map_Create, my_Map_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Map_Clone, my_Map_Clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Clone, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Map_Destroy, my_Map_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_init, my_mqtt_client_init);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_init, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_connect, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_connect, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_deinit, my_mqtt_client_deinit);

    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_disconnect, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_disconnect, __FAILURE__);

    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_subscribe, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_subscribe, __FAILURE__);

    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_unsubscribe, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_unsubscribe, __FAILURE__);

    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_publish, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_publish, __FAILURE__);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_create, TEST_MQTT_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_getApplicationMsg, &TEST_APP_PAYLOAD);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_getApplicationMsg, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_getTopicName, TEST_MQTT_MSG_TOPIC);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_getTopicName, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_create, my_STRING_TOKENIZER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_create_from_char, my_STRING_TOKENIZER_create_from_char);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_create_from_char, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_get_next_token, my_STRING_TOKENIZER_get_next_token);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_get_next_token, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_destroy, my_STRING_TOKENIZER_destroy);
    
    REGISTER_GLOBAL_MOCK_HOOK(SASToken_Create, my_SASToken_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(SASToken_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, TEST_TIME_T);

    REGISTER_GLOBAL_MOCK_HOOK(get_difftime, my_get_difftime);

    REGISTER_GLOBAL_MOCK_HOOK(xio_create, my_xio_create);

    REGISTER_GLOBAL_MOCK_RETURN(xio_close, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_close, __FAILURE__);

    REGISTER_GLOBAL_MOCK_RETURN(xio_setoption, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_setoption, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(xio_destroy, my_xio_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_create, my_tickcounter_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_destroy, my_tickcounter_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_get_current_ms, my_tickcounter_get_current_ms);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_get_current_ms, __FAILURE__);

    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Create, real_CONSTBUFFER_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(CONSTBUFFER_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_SasToken, my_IoTHubClient_Auth_Get_SasToken);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_SasToken, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Is_SasToken_Valid, SAS_TOKEN_STATUS_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Is_SasToken_Valid, SAS_TOKEN_STATUS_FAILED);

    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_GetContent, (const CONSTBUFFER* (*) (CONSTBUFFER_HANDLE constbufferHandle)) real_CONSTBUFFER_GetContent);
    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Destroy, real_CONSTBUFFER_Destroy);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InitializeListHead, real_DList_InitializeListHead);
    REGISTER_GLOBAL_MOCK_HOOK(DList_IsListEmpty, real_DList_IsListEmpty);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertTailList, real_DList_InsertTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertHeadList, real_DList_InsertHeadList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_AppendTailList, real_DList_AppendTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveEntryList, real_DList_RemoveEntryList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveHeadList, real_DList_RemoveHeadList);


    REGISTER_GLOBAL_MOCK_RETURN(retry_control_create, TEST_RETRY_CONTROL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_should_retry, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_should_retry, 1);

    REGISTER_UMOCK_ALIAS_TYPE(RETRY_CONTROL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(RETRY_ACTION, int);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

static void reset_test_data()
{
    g_fnMqttMsgRecv = NULL;
    g_fnMqttOperationCallback = NULL;
    g_callbackCtx = NULL;
    g_fnMqttErrorCallback = NULL;
    g_errorcallbackCtx = NULL;
    g_method_handle_value = NULL;

    g_current_ms = 0;
    g_tokenizerIndex = 0;
    g_nullMapVariable = true;

    g_msg_disposition = IOTHUBMESSAGE_ACCEPTED;
    expected_MQTT_TRANSPORT_PROXY_OPTIONS = NULL;
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    TEST_MUTEX_ACQUIRE(test_serialize_mutex);

    reset_test_data();
    real_DList_InitializeListHead(&g_waitingToSend);

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
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

static void SetupIothubTransportConfig(IOTHUBTRANSPORT_CONFIG* config, const char* deviceId, const char* deviceKey, const char* iotHubName,
    const char* iotHubSuffix, const char* protocolGatewayHostName)
{
    g_iothubClientConfig.protocol = TEST_PROTOCOL;
    g_iothubClientConfig.deviceId = deviceId;
    g_iothubClientConfig.deviceKey = deviceKey;
    g_iothubClientConfig.deviceSasToken = NULL;
    g_iothubClientConfig.iotHubName = iotHubName;
    g_iothubClientConfig.iotHubSuffix = iotHubSuffix;
    g_iothubClientConfig.protocolGatewayHostName = protocolGatewayHostName;
    config->waitingToSend = &g_waitingToSend;
    config->upperConfig = &g_iothubClientConfig;
    config->auth_module_handle = TEST_IOTHUB_AUTHORIZATION_HANDLE;
}

static void SetupIothubTransportConfigWithKeyAndSasToken(IOTHUBTRANSPORT_CONFIG* config, const char* deviceId, const char* deviceKey, const char* deviceSasToken,
    const char* iotHubName, const char* iotHubSuffix, const char* protocolGatewayHostName)
{
    g_iothubClientConfig.protocol = TEST_PROTOCOL;
    g_iothubClientConfig.deviceId = deviceId;
    g_iothubClientConfig.deviceKey = deviceKey;
    g_iothubClientConfig.deviceSasToken = deviceSasToken;
    g_iothubClientConfig.iotHubName = iotHubName;
    g_iothubClientConfig.iotHubSuffix = iotHubSuffix;
    g_iothubClientConfig.protocolGatewayHostName = protocolGatewayHostName;
    config->waitingToSend = &g_waitingToSend;
    config->upperConfig = &g_iothubClientConfig;
    config->auth_module_handle = TEST_IOTHUB_AUTHORIZATION_HANDLE;
}

static void setup_IoTHubTransport_MQTT_Common_Create_mocks(bool use_gateway)
{
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tickcounter_create());
    STRICT_EXPECTED_CALL(retry_control_create(DEFAULT_RETRY_POLICY, DEFAULT_RETRY_TIMEOUT_IN_SECONDS));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));

    EXPECTED_CALL(mqtt_client_init(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    if (use_gateway)
    {
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    }

    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG))
        .IgnoreArgument(1).SetReturn(TEST_SMALL_TIME_T);
}

static void setup_message_recv_with_properties_mocks(bool has_content_type, bool has_content_encoding, bool auto_decode)
{
    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(TEST_MQTT_MSG_TOPIC_W_1_PROP);
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(TEST_MQTT_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(appMessage, appMsgSize));
    STRICT_EXPECTED_CALL(STRING_construct(TEST_MQTT_MSG_TOPIC_W_1_PROP));
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));
    STRICT_EXPECTED_CALL(STRING_new());

    if (has_content_type)
    {
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn("%24.ct=application/json");
        STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
        if (auto_decode)
        {
            STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(IoTHubMessage_SetContentTypeSystemProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (auto_decode)
        {
            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_NUM_ARG));
    }

    if (has_content_encoding)
    {
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn("%24.ce=utf8");
        STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
        if (auto_decode)
        {
            STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(IoTHubMessage_SetContentEncodingSystemProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (auto_decode)
        {
            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_NUM_ARG));
    }

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn("propName=propValue");

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();
    if (auto_decode)
    {
        STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(Map_AddOrUpdate(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_key()
        .IgnoreArgument_value();
    if (auto_decode)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(gballoc_free(NULL)).IgnoreArgument_ptr();
    STRICT_EXPECTED_CALL(gballoc_free(NULL)).IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_MessageCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument_message_data();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubMessageHandle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
}

static void setup_connection_success_mocks()
{
    STRICT_EXPECTED_CALL(retry_control_reset(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK))
        .IgnoreArgument(1);
}

static void setup_initialize_reconnection_mocks()
{
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    STRICT_EXPECTED_CALL(retry_control_should_retry(TEST_RETRY_CONTROL_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void setup_devicemethod_response_mocks()
{
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqttmessage_create(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_MOST_ONCE, appMessage, appMsgSize))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mqtt_client_publish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mqttmessage_destroy(TEST_MQTT_MESSAGE_HANDLE))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void setup_initialize_connection_mocks()
{
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    STRICT_EXPECTED_CALL(retry_control_should_retry(TEST_RETRY_CONTROL_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetOption(IGNORED_PTR_ARG, OPTION_PRODUCT_INFO, IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubClientHandle()
        .IgnoreArgument_value();
    STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG))
        .IgnoreArgument_input();
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_s1()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    
    // from GetTransportProviderIfNecessary()
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_HOST_NAME);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void setup_subscribe_devicetwin_dowork_mocks()
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(mqtt_client_subscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_packetId()
        .IgnoreArgument_subscribeList()
        .IgnoreArgument_count();
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
}

static void setup_IoTHubTransport_MQTT_Common_DoWork_mocks()
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_MQTT_MSG_TOPIC);
    EXPECTED_CALL(mqtt_client_subscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);
}

static void setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(
    const char* const** ppKeys, 
    const char* const** ppValues, 
    size_t propCount, 
    IOTHUB_MESSAGE_HANDLE msg_handle, 
    bool resend, 
    const char* msg_id, 
    const char* core_id,
    const char* content_type,
    const char* content_encoding,
    const char* diag_id,
    const char* creation_time_utc,
    bool auto_urlencode)
{
    TEST_DIAG_DATA.diagnosticId = (char*)diag_id;
    TEST_DIAG_DATA.diagnosticCreationTimeUtc = (char*)creation_time_utc;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(msg_handle));
    if (msg_handle == TEST_IOTHUB_MSG_STRING)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(TEST_IOTHUB_MSG_STRING))
            .IgnoreArgument(1);
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MSG_BYTEARRAY, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    if (!resend)
    {
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    }
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(TEST_MQTT_EVENT_TOPIC)).IgnoreArgument(1);
    //Add Properties
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(msg_handle));
    if (propCount == 0)
    {
        EXPECTED_CALL(Map_GetInternals(TEST_MESSAGE_PROP_MAP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MESSAGE_PROP_MAP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &ppKeys, sizeof(ppKeys))
            .CopyOutArgumentBuffer(3, &ppValues, sizeof(ppValues))
            .CopyOutArgumentBuffer(4, &propCount, sizeof(propCount));
        
        for (size_t i=0; i < propCount; i++)
        {
            if (auto_urlencode)
            {
                STRICT_EXPECTED_CALL(URL_EncodeString((const char*)ppKeys[i]));
                STRICT_EXPECTED_CALL(URL_EncodeString((const char*)ppValues[i]));
                STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
                STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
                STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
                STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
            }
        }
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG)).SetReturn(core_id);
    if (auto_urlencode && (core_id != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG)).SetReturn(msg_id);
    if (auto_urlencode && (msg_id != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(IGNORED_PTR_ARG)).SetReturn(content_type);
    if (auto_urlencode && (content_type != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(IGNORED_PTR_ARG)).SetReturn(content_encoding);
    if (auto_urlencode && (content_encoding != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetDiagnosticPropertyData(IGNORED_PTR_ARG)).SetReturn(&TEST_DIAG_DATA);
    bool validMessage = true;
    if (diag_id != NULL && creation_time_utc != NULL)
    {
        STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else if (diag_id != NULL || creation_time_utc != NULL)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        validMessage = false;
    }

    //Publish
    if (validMessage)
    {
        EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        EXPECTED_CALL(mqttmessage_create(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_LEAST_ONCE, appMessage, appMsgSize));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mqtt_client_publish(TEST_MQTT_CLIENT_HANDLE, IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mqttmessage_destroy(TEST_MQTT_MESSAGE_HANDLE))
            .IgnoreArgument(1);
        EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        if (!resend)
        {
            EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
    }
    else
    {
        EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
        EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
        EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(IoTHubClientCore_LL_SendComplete(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_ERROR));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));
    }
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
}

static void setup_message_recv_device_method_mocks()
{
    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(TEST_MQTT_DEV_METHOD_MSG);
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create_from_char(IGNORED_PTR_ARG))
        .IgnoreArgument_input();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "/"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "/"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "/"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "/"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_s1()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "/"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(7);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn("?$rid=b");
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_t();
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(TEST_MQTT_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodComplete(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_method_name()
        .IgnoreArgument_payLoad()
        .IgnoreArgument_size()
        .IgnoreArgument_response_id();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
}

static void setup_processItem_mocks(bool fail_test)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).IgnoreArgument_constbufferHandle();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(mqttmessage_create(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_MOST_ONCE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument_packetId()
        .IgnoreArgument_topicName()
        .IgnoreArgument_appMsg()
        .IgnoreArgument_appMsgLength();
    if (!fail_test)
    {
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }
    STRICT_EXPECTED_CALL(mqtt_client_publish(TEST_MQTT_CLIENT_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .IgnoreArgument_msgHandle();
    STRICT_EXPECTED_CALL(mqttmessage_destroy(TEST_MQTT_MESSAGE_HANDLE))
        .IgnoreArgument_handle();
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setup_message_recv_callback_device_twin_mocks(const char* token_type)
{
    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(TEST_MQTT_DEV_TWIN_MSG_TOPIC);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create_from_char(IGNORED_PTR_ARG)).IgnoreArgument_input();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_output()
        .IgnoreArgument_delimiters()
        .IgnoreArgument_t();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_output()
        .IgnoreArgument_delimiters()
        .IgnoreArgument_t();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_output()
        .IgnoreArgument_delimiters()
        .IgnoreArgument_t();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(token_type);

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_output()
        .IgnoreArgument_delimiters()
        .IgnoreArgument_t();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("200")
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_output()
        .IgnoreArgument_delimiters()
        .IgnoreArgument_t();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("2")
        .IgnoreArgument_handle();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_t();
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ReportedStateComplete(IGNORED_PTR_ARG, 2, 200))
        .IgnoreArgument_handle()
        .IgnoreArgument_item_id()
        .IgnoreArgument_status_code();
    EXPECTED_CALL(gballoc_free(NULL));
}

static void setup_message_recv_msg_callback_mocks()
{
    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(TEST_MQTT_MSG_TOPIC);
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(TEST_MQTT_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(appMessage, appMsgSize));

    STRICT_EXPECTED_CALL(STRING_construct(TEST_MQTT_MSG_TOPIC));
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_MessageCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument_message_data();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubMessageHandle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();
}

static XIO_HANDLE get_IO_transport_fail(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options)
{
    (void)fully_qualified_name;
    (void)mqtt_transport_proxy_options;
    return NULL;
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_001: [If parameter config is NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_parameter_Succeed)
{
    //arrange

    //act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(NULL, get_IO_transport);

    //assert
    ASSERT_IS_NULL(result);

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [If the parameter config's variables upperConfig or waitingToSend are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_config_parameter_fails)
{
    //arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    config.waitingToSend = &g_waitingToSend;

    //act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    //assert
    ASSERT_IS_NULL(result);

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [If the parameter config's variables upperConfig or waitingToSend are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_waitingToSend_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    config.waitingToSend = NULL;

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_009: [If any error is encountered then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_protocol_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    g_iothubClientConfig.protocol = NULL;

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, deviceKey, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_device_id_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, NULL, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_005: [If the upperConfig's variables deviceKey, iotHubName, or iotHubSuffix are empty strings then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_deviceSasToken_blank_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, TEST_EMPTY_STRING, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_deviceKey_blank_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, "", TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, deviceKey, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_iothub_name_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, NULL, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_006: [If the upperConfig's variables deviceId is an empty strings or length is greater then 128 then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_very_long_device_id_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_VERY_LONG_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, deviceKey, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_empty_device_id_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_EMPTY_STRING, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, deviceKey, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_empty_device_key_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_EMPTY_STRING, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_03_003: [If both deviceKey & deviceSasToken fields are NOT NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_both_deviceKey_and_deviceSasToken_defined_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };

    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, deviceKey, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_empty_iothub_name_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_EMPTY_STRING, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_010: [IoTHubTransport_MQTT_Common_Create shall allocate memory to save its internal state where all topics, hostname, device_id, device_key, sasTokenSr and client handle shall be saved.] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_011: [On Success IoTHubTransport_MQTT_Common_Create shall return a non-NULL value.] */
// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_005: [ MQTT transport shall use EXPONENTIAL_WITH_BACK_OFF as default retry policy ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_validConfig_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, NULL);

    setup_IoTHubTransport_MQTT_Common_Create_mocks(false);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean up
    IoTHubTransport_MQTT_Common_Destroy(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_007: [If the upperConfig's variables protocolGatewayHostName is non-Null and the length is an empty string then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_protocol_gateway_hostname_Succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, NULL);

    setup_IoTHubTransport_MQTT_Common_Create_mocks(false);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean up
    IoTHubTransport_MQTT_Common_Destroy(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_protocol_gateway_hostname_Succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME_NON_NULL);

    setup_IoTHubTransport_MQTT_Common_Create_mocks(true);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean up
    IoTHubTransport_MQTT_Common_Destroy(result);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, deviceKey, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_009: [If any error is encountered then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_011: [On Success IoTHubTransport_MQTT_Common_Create shall return a non-NULL value.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_validConfig_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, NULL);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_Create_mocks(false);

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 4, 5, 6, 7 };

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_Create failure in test %zu/%zu", index, count);
        TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

        // assert
        ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
    }

    // clean up
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_012: [IoTHubTransport_MQTT_Common_Destroy shall do nothing if parameter handle is NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_parameter_NULL_succeed)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_Destroy(NULL);

    // assert
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_013: [If the parameter subscribe is true then IoTHubTransport_MQTT_Common_Destroy shall call IoTHubTransport_MQTT_Common_Unsubscribe.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_Unsubscribe_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    (void)IoTHubTransport_MQTT_Common_Subscribe(handle);
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };

    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(NULL)).SetReturn(TEST_STRING_VALUE);

    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));

    STRICT_EXPECTED_CALL(mqtt_client_disconnect(TEST_MQTT_CLIENT_HANDLE, NULL, NULL));
    STRICT_EXPECTED_CALL(mqtt_client_deinit(TEST_MQTT_CLIENT_HANDLE));
    STRICT_EXPECTED_CALL(mqtt_client_unsubscribe(TEST_MQTT_CLIENT_HANDLE, IGNORED_NUM_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    EXPECTED_CALL(gballoc_free(NULL));
    STRICT_EXPECTED_CALL(xio_destroy(TEST_XIO_HANDLE));
    STRICT_EXPECTED_CALL(tickcounter_destroy(TEST_COUNTER_HANDLE));

    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert
}

static void set_expected_calls_for_free_transport_handle_data()
{
    STRICT_EXPECTED_CALL(mqtt_client_deinit(TEST_MQTT_CLIENT_HANDLE)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(tickcounter_destroy(TEST_COUNTER_HANDLE)).IgnoreArgument(1);

    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));

    EXPECTED_CALL(gballoc_free(NULL));
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_014: [IoTHubTransport_MQTT_Common_Destroy shall free all the resources currently in use.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_One_Message_Ack_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST) );
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mqtt_client_disconnect(TEST_MQTT_CLIENT_HANDLE, NULL, NULL))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(xio_destroy(TEST_XIO_HANDLE))
        .IgnoreArgument(1);
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendComplete(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(gballoc_free(NULL));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));

    set_expected_calls_for_free_transport_handle_data();

    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_014: [IoTHubTransport_MQTT_Common_Destroy shall free all the resources currently in use.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));

    STRICT_EXPECTED_CALL(mqtt_client_deinit(TEST_MQTT_CLIENT_HANDLE));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(TEST_MQTT_CLIENT_HANDLE, NULL, NULL));
    EXPECTED_CALL(xio_destroy(NULL));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_015: [If parameter handle is NULL than IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_parameter_NULL_fail)
{
    // arrange

    // act
    int res = IoTHubTransport_MQTT_Common_Subscribe(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_016: [IoTHubTransport_MQTT_Common_Subscribe shall set a flag to enable mqtt_client_subscribe to be called to subscribe to the Message Topic.] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_018: [On success IoTHubTransport_MQTT_Common_Subscribe shall return 0.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_Succeed)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    setup_IoTHubTransport_MQTT_Common_DoWork_mocks();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_017: [Upon failure IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();
    (void)IoTHubTransport_MQTT_Common_Subscribe(handle);

    setup_IoTHubTransport_MQTT_Common_DoWork_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 0 };

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %zu/%zu", index, count);
        
        IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_017: [Upon failure IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_set_subscribe_type_Succeed)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    
    umock_c_reset_all_calls();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    setup_IoTHubTransport_MQTT_Common_DoWork_mocks();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_set_subscribe_after_publish_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(STRING_c_str(NULL)).SetReturn(TEST_MQTT_MESSAGE_TOPIC);
    STRICT_EXPECTED_CALL(mqtt_client_subscribe(TEST_MQTT_CLIENT_HANDLE, IGNORED_NUM_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    int result = IoTHubTransport_MQTT_Common_Subscribe(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_25_041: [**If any handle is NULL then IoTHubTransport_MQTT_Common_SetRetryPolicy shall return resultant line.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetRetryPolicy_parameter_NULL_fail)
{
    // arrange

    // act
    int res = IoTHubTransport_MQTT_Common_SetRetryPolicy(NULL, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
}

// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_009: [ If retry_control_create() fails then IoTHubTransport_MQTT_Common_SetRetryPolicy shall revert to previous retry policy and return non-zero value ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetRetryPolicy_failure)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(retry_control_create(TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS))
        .SetReturn(NULL);

    // act
    int res = IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_006: [ IoTHubTransport_MQTT_Common_SetRetryPolicy shall set the retry logic by calling retry_control_create() with `retryPolicy` and `retryTimeoutLimitinSeconds` as parameters]
/*Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_25_042: [**If the retry logic is not already created then IoTHubTransport_MQTT_Common_SetRetryPolicy shall create retry logic by calling CreateRetryLogic with retry policy and retryTimeout as parameters]*/
/*Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_25_045: [**If retry logic for specified parameters of retry policy and retryTimeoutLimitInSeconds is created successfully then IoTHubTransport_MQTT_Common_SetRetryPolicy shall return 0]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetRetryPolicy_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(retry_control_create(TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS));
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));

    // act
    int res = IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/*Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_25_043: [**If the retry logic is already created then IoTHubTransport_MQTT_Common_SetRetryPolicy shall destroy existing retry logic and create retry logic by calling CreateRetryLogic with retry policy and retryTimeout as parameters]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetRetryPolicy_change_policy_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(retry_control_create(TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS));
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(retry_control_create(TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS));
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));

    // act
    int res = IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    res = IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_020: [ IoTHubTransport_MQTT_Common_Unsubscribe shall call mqtt_client_unsubscribe to unsubscribe the mqtt message topic.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_Succeed)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_unsubscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, 1));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_019: [ If parameter handle is NULL then IoTHubTransport_MQTT_Common_Unsubscribe shall do nothing.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_handle_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_049: [ If subscribe_state is set to IOTHUB_DEVICE_TWIN_DESIRED_STATE then IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall send the get state topic to the mqtt client. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_049: [ If subscribe_state is set to IOTHUB_DEVICE_TWIN_DESIRED_STATE then IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall send the get state topic to the mqtt client. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin_No_subscribe_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_048: [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall do nothing.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin_handle_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_032: [IoTHubTransport_MQTT_Common_SetOption shall pass down the option to xio_setoption if the option parameter is not a known option string for the MQTT transport.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_invokes_xio_setoption_when_option_not_consumed_by_mqtt_transport)
{
    // arrange
    const char* SOME_OPTION = "AnOption";
    const void* SOME_VALUE = (void*)42;

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(NULL)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_setoption(NULL, SOME_OPTION, SOME_VALUE))
        .IgnoreArgument(1);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, SOME_OPTION, SOME_VALUE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_021: [If any parameter is NULL then IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_option_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    bool traceOn = true;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, NULL, &traceOn);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_021: [If any parameter is NULL then IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_value_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_LOG_TRACE, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_021: [If any parameter is NULL then IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_SAS_TOKEN_LIFETIME_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_SAS_TOKEN_LIFETIME, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_052: [ If the option parameter is set to "sas_token_lifetime" then the value shall be a size_t_ptr and the value will determine the mqtt sas token lifetime.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_SAS_TOKEN_LIFETIME_succees)
{
    // arrange
    size_t token_lifetime = 600;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_SAS_TOKEN_LIFETIME, &token_lifetime);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_039: [If the option parameter is set to "x509certificate" then the value shall be a const char of the certificate to be used for x509.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_x509Certificate_no_509_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_X509_CERT, X509_CERT_CERTIFICATE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_x509Private_key_no_509_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, X509_PRIVATE_KEY_OPTION, X509_PRIVATE_KEY);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_x509Certificate_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Set_x509_Type(IGNORED_PTR_ARG, true));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_X509_ECC);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Set_xio_Certificate(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, OPTION_X509_CERT, IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_X509_CERT, X509_CERT_CERTIFICATE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_x509Private_key_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Set_x509_Type(IGNORED_PTR_ARG, true));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_X509_ECC);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Set_xio_Certificate(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, X509_PRIVATE_KEY_OPTION, X509_PRIVATE_KEY));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, X509_PRIVATE_KEY_OPTION, X509_PRIVATE_KEY);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_031: [If the option parameter is set to "logtrace" then the value shall be a bool_ptr and the value will determine if the mqtt client log is on or off.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_set_trace(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    bool traceOn = true;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_LOG_TRACE, &traceOn);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_036: [If the option parameter is set to "keepalive" then the value shall be a int_ptr and the value will determine the mqtt keepalive time that is set for pings.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_keepAlive_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    int keepAlive = 10;
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_KEEP_ALIVE, &keepAlive);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_038: [If the client is connected when the keepalive is set then IoTHubTransport_MQTT_Common_SetOption shall disconnect and reconnect with the specified keepalive value.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_keepAlive_previous_connection_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(xio_destroy(TEST_XIO_HANDLE)).IgnoreArgument(1);

    int keepAlive = 10;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_KEEP_ALIVE, &keepAlive);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_032: [IoTHubTransport_MQTT_Common_SetOption shall pass down the option to xio_setoption if the option parameter is not a known option string for the MQTT transport.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_xio_create_fail)
{
    // arrange
    const char* SOME_OPTION = "AnOption";
    const void* SOME_VALUE = (void*)42;

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport_fail);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(NULL)).SetReturn(TEST_STRING_VALUE);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, SOME_OPTION, SOME_VALUE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_132: [IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG xio_setoption fails] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_fails_when_xio_setoption_fails)
{
    // arrange
    const char* SOME_OPTION = "AnOption";
    const void* SOME_VALUE = (void*)42;

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, SOME_OPTION, SOME_VALUE))
        .IgnoreArgument(1)
        .SetReturn(__FAILURE__);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, SOME_OPTION, SOME_VALUE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}


/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_037 : [If the option parameter is set to supplied int_ptr keepalive is the same value as the existing keepalive then IoTHubTransport_MQTT_Common_SetOption shall do nothing.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_keepAlive_same_value_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    int keepAlive = 30;
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_KEEP_ALIVE, &keepAlive);

    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_KEEP_ALIVE, &keepAlive);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_001: [ If `option` is `proxy_data`, `value` shall be used as an `HTTP_PROXY_OPTIONS*`. ]*/
/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_002: [ The fields `host_address`, `port`, `username` and `password` shall be saved for later used (needed when creating the underlying IO to be used by the transport). ]*/
/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_008: [ If setting the `proxy_data` option succeeds, `IoTHubTransport_MQTT_Common_SetOption` shall return `IOTHUB_CLIENT_OK` ]*/
TEST_FUNCTION(SetOption_with_proxy_data_copies_the_options_for_later_use)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "me";
    http_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "me"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "shhhh"));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_003: [ If `host_address` is NULL, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. *]*/
TEST_FUNCTION(SetOption_proxy_data_with_NULL_host_address_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = NULL;
    http_proxy_options.port = 2222;
    http_proxy_options.username = "me";
    http_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_021: [ If any parameter is NULL then IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.]*/
TEST_FUNCTION(SetOption_proxy_data_with_NULL_option_value_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", NULL);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_005: [ `username` and `password` shall be allowed to be NULL. ]*/
/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_008: [ If setting the `proxy_data` option succeeds, `IoTHubTransport_MQTT_Common_SetOption` shall return `IOTHUB_CLIENT_OK` ]*/
TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_and_password_saves_only_the_hostname)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = NULL;
    http_proxy_options.password = NULL;

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_006: [ If only one of `username` and `password` is NULL, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_but_non_NULL_password_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "the_other_me";
    http_proxy_options.password = NULL;

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_006: [ If only one of `username` and `password` is NULL, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
TEST_FUNCTION(SetOption_proxy_data_with_NULL_password_but_non_NULL_username_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = NULL;
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_009: [ When setting the proxy options succeeds any previously saved proxy options shall be freed. ]*/
TEST_FUNCTION(SetOption_proxy_data_frees_previously_Saved_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "me"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "shhhh"));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "me";
    http_proxy_options.password = "shhhh";

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
TEST_FUNCTION(when_allocating_proxy_name_fails_SetOption_proxy_data_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .SetReturn(1);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
TEST_FUNCTION(when_allocating_username_fails_SetOption_proxy_data_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
TEST_FUNCTION(when_allocating_password_fails_SetOption_proxy_data_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_009: [ When setting the proxy options succeeds any previously saved proxy options shall be freed. ]*/
TEST_FUNCTION(when_allocating_proxy_name_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .SetReturn(1);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_009: [ When setting the proxy options succeeds any previously saved proxy options shall be freed. ]*/
TEST_FUNCTION(when_allocating_username_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_009: [ When setting the proxy options succeeds any previously saved proxy options shall be freed. ]*/
TEST_FUNCTION(when_allocating_password_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_009: [ When setting the proxy options succeeds any previously saved proxy options shall be freed. ]*/
TEST_FUNCTION(SetOption_proxy_data_with_NULL_hostname_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = NULL;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_009: [ When setting the proxy options succeeds any previously saved proxy options shall be freed. ]*/
TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_and_non_NULL_password_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "baba";
    http_proxy_options.username = NULL;
    http_proxy_options.password = "cloanta";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_011: [ If no `proxy_data` option has been set, NULL shall be passed as the argument `mqtt_transport_proxy_options` when calling the function `get_io_transport` passed in `IoTHubTransport_MQTT_Common__Create`. ]*/
TEST_FUNCTION(SetOption_xio_option_get_underlying_TLS_when_proxy_data_was_not_set_passes_down_NULL)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    bool value = true;

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, "Some XIO option name", &value));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "Some XIO option name", &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, 0, error_proxy_options);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_010: [ If the `proxy_data` option has been set, the proxy options shall be filled in the argument `mqtt_transport_proxy_options` when calling the function `get_io_transport` passed in `IoTHubTransport_MQTT_Common__Create` to obtain the underlying IO handle. ]*/
TEST_FUNCTION(SetOption_xio_option_get_underlying_TLS_when_proxy_data_was_set_passes_down_the_proxy_options)
{
    // arrange
    MQTT_TRANSPORT_PROXY_OPTIONS mqtt_transport_proxy_options;
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    mqtt_transport_proxy_options.host_address = "test_proxy";
    mqtt_transport_proxy_options.port = 2222;
    mqtt_transport_proxy_options.username = "haha";
    mqtt_transport_proxy_options.password = "bleah";

    bool value = true;
    expected_MQTT_TRANSPORT_PROXY_OPTIONS = &mqtt_transport_proxy_options;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, "Some XIO option name", &value));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "Some XIO option name", &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, 0, error_proxy_options);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_007: [ If the underlying IO has already been created, then `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
TEST_FUNCTION(SetOption_proxy_data_when_underlying_IO_is_already_created_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    bool value = true;

    // act
    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "Some XIO option name", &value);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_012: [ `IoTHubTransport_MQTT_Common_Destroy` shall free the stored proxy options. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_frees_the_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL))
        .IgnoreAllCalls();
    STRICT_EXPECTED_CALL(mqtt_client_deinit(IGNORED_PTR_ARG))
        .IgnoreAllCalls();
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG))
        .IgnoreAllCalls();
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
        .IgnoreAllCalls();
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreAllCalls();
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_mqtt_operation_complete_msgInfo_NULL_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    g_fnMqttOperationCallback(NULL, MQTT_CLIENT_ON_CONNACK, NULL, g_callbackCtx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, NULL, NULL);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, NULL, g_callbackCtx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_PUBLISH_ACK, NULL, g_callbackCtx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_PUBLISH_COMP, NULL, g_callbackCtx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, NULL, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_026: [IoTHubTransport_MQTT_Common_DoWork shall do nothing if parameter handle and/or iotHubClientHandle is NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_parameter_handle_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_DoWork(NULL, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    // assert

}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_026: [IoTHubTransport_MQTT_Common_DoWork shall do nothing if parameter handle and/or iotHubClientHandle is NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_parameter_iothubClient_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_DoWork(TEST_TRANSPORT_HANDLE, NULL);
    // assert

}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_026: [IoTHubTransport_MQTT_Common_DoWork shall do nothing if parameter handle and/or iotHubClientHandle is NULL.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_all_parameters_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_DoWork(NULL, NULL);
    // assert

}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_027: [IoTHubTransport_MQTT_Common_DoWork shall inspect the "waitingToSend" DLIST passed in config structure.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_mqtt_client_connect_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetOption(IGNORED_PTR_ARG, OPTION_PRODUCT_INFO, IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubClientHandle()
        .IgnoreArgument_value();
    STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG))
        .IgnoreArgument_input();
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_s1()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_HOST_NAME);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_First_connect_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_008: [ Upon successful connection the retry control shall be reset using retry_control_reset() ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_First_connect_succeed_calls_retry_control_reset)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };

    umock_c_reset_all_calls();
    setup_initialize_connection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);
    setup_connection_success_mocks();

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_First_Connect_Failed_Retry_Success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    CONNECT_ACK connack;
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_SERVER_UNAVAIL;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_007: [ IoTHubTransport_MQTT_Common_DoWork shall try to reconnect according to the current retry policy set ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_Connection_Break_Wait_2_Times_Success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    CONNECT_ACK connack;
    connack.isSessionPresent = true;
    connack.returnCode = CONNECTION_ACCEPTED;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();
    /*First Do_Work*/
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_LATER;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);
    /*Second Do_Work*/
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);
    /* Attempt to connect again*/
    setup_initialize_reconnection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);

    // act
    /* Break Connection */
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);
    /* Retry connecting */
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_Connection_Break_Reconnection_Times_Out)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    CONNECT_ACK connack;
    connack.isSessionPresent = true;
    connack.returnCode = CONNECTION_ACCEPTED;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_SERVER_UNAVAIL;
    /* Break Connection */
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);

    umock_c_reset_all_calls();

    /*Do_Work Festival*/
    size_t counter;
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_LATER;

    for (counter = 0; counter < 100; counter++)
    {
        if (counter == 30) // This is a random number.... 
        {
            retry_action = RETRY_ACTION_STOP_RETRYING;
        }

        EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
        STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
            .IgnoreArgument(1);

        IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    }

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_Connection_Break_2_Reconnection_Attempts)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    CONNECT_ACK connack;
    connack.isSessionPresent = true;
    connack.returnCode = CONNECTION_ACCEPTED;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_UNKNOWN;
    /* Break Connection */
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);
    umock_c_reset_all_calls();

    /*First Do_Work*/
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_LATER;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    /*Second Do_Work*/
    /* Attempt to connect again*/
    setup_initialize_reconnection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    /* Fail connection */
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    /* Second Retry */
    // setup retry-setup mocks
    /* Attempt to connect again*/
    setup_initialize_reconnection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    /* Retry connecting */
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    /* Connect fail */
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    /* Disconnecting */
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    /*Second Retry connecting */
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_027: [IoTHubTransport_MQTT_Common_DoWork shall inspect the "waitingToSend" DLIST passed in config structure.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_no_messages_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_no_messages_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %zu/%zu", index, count);

        IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

        //assert
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_SAS_token_from_user_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Is_SasToken_Valid(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetOption(IGNORED_PTR_ARG, OPTION_PRODUCT_INFO, IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubClientHandle()
        .IgnoreArgument_value();
    STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG))
        .IgnoreArgument_input();
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_s1()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_SAS_token_from_user_invalid_callback_connection_status)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Is_SasToken_Valid(IGNORED_PTR_ARG)).SetReturn(SAS_TOKEN_STATUS_INVALID);
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN))
        .IgnoreArgument(1);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_SAS_token_from_user_failed_callback_connection_status)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Is_SasToken_Valid(IGNORED_PTR_ARG)).SetReturn(SAS_TOKEN_STATUS_FAILED);
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL))
        .IgnoreArgument(1);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_041: [If both deviceKey and deviceSasToken fields are NULL then IoTHubTransport_MQTT_Common_Create shall assume a x509 authentication.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_x509_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_X509);
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetOption(IGNORED_PTR_ARG, OPTION_PRODUCT_INFO, IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubClientHandle()
        .IgnoreArgument_value();
    STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG))
        .IgnoreArgument_input();
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_s1()
        .IgnoreArgument_s2();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_027: [IoTHubTransport_MQTT_Common_DoWork shall inspect the "waitingToSend" DLIST passed in config structure.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_BYTEARRAY, false, NULL, NULL, NULL, NULL, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_BYTEARRAY, false, NULL, NULL, NULL, NULL, NULL, NULL, false);

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 4 };

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %zu/%zu", index, count);

        IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

        //assert
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_with_properties_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_nullMapVariable = false;

    const size_t propCount = 1;
    const char* keys[1] = { "propKey1" };
    const char* values[1] = { "propValue1" };

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks((const char* const**)&keys, (const char* const**)&values, propCount, TEST_IOTHUB_MSG_BYTEARRAY, false, NULL, NULL, NULL, NULL, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_with_2_properties_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_nullMapVariable = false;

    const size_t propCount = 2;
    const char* keys[2] = { "propKey1", "propKey2" };
    const char* values[2] = { "propValue1", "propValue2" };

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks((const char* const**)&keys, (const char* const**)&values, propCount, TEST_IOTHUB_MSG_BYTEARRAY, false, NULL, NULL, NULL, NULL, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_with_properties_succeeds_autoencode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_nullMapVariable = false;

    const size_t propCount = 1;
    const char* keys[1] = { "propKey1" };
    const char* values[1] = { "propValue1" };

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks((const char* const**)&keys, (const char* const**)&values, propCount, TEST_IOTHUB_MSG_BYTEARRAY, false, NULL, NULL, NULL, NULL, NULL, NULL, true);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_with_2_properties_succeeds_autoencode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_nullMapVariable = false;

    const size_t propCount = 2;
    const char* keys[2] = { "propKey1", "propKey2" };
    const char* values[2] = { "propValue1", "propValue2" };

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks((const char* const**)&keys, (const char* const**)&values, propCount, TEST_IOTHUB_MSG_BYTEARRAY, false, NULL, NULL, NULL, NULL, NULL, NULL, true);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Test_SRS_IOTHUB_MQTT_TRANSPORT_07_033: [IoTHubTransport_MQTT_Common_DoWork shall iterate through the Waiting Acknowledge messages looking for any message that has been waiting longer than 2 min.]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_no_resend_message_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Test_SRS_IOTHUB_MQTT_TRANSPORT_07_033: [IoTHubTransport_MQTT_Common_DoWork shall iterate through the Waiting Acknowledge messages looking for any message that has been waiting longer than 2 min.]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_resend_message_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));
    g_current_ms += 5*60*1000;
    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, true, NULL, NULL, NULL, NULL, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_034: [ If IoTHubTransport_MQTT_Common_DoWork has previously resent the message two times then it shall fail the message and reconnect to IoTHub ... ]*/
/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_057: [ ... then go through all the rest of the waiting messages and reset the retryCount on the message. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_message_timeout_succeeds)
{
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    SUBSCRIBE_ACK suback;
    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    IOTHUB_MESSAGE_LIST message2;
    TRANSPORT_LL_HANDLE handle;

    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    g_current_ms += 5 * 60 * 1000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    g_current_ms += 5 * 60 * 1000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendComplete(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_034: [ If IoTHubTransport_MQTT_Common_DoWork has previously resent the message two times then it shall fail the message and reconnect to IoTHub ... ]*/
/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_057: [ ... then go through all the rest of the waiting messages and reset the retryCount on the message. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_2_message_timeout_succeeds)
{
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    SUBSCRIBE_ACK suback;
    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    IOTHUB_MESSAGE_LIST message2;
    IOTHUB_MESSAGE_LIST message1;
    TRANSPORT_LL_HANDLE handle;

    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message1.entry));

    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    g_current_ms += 5 * 60 * 1000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    g_current_ms += 5 * 60 * 1000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendComplete(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetString(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); 
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetDiagnosticPropertyData(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqttmessage_create(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_LEAST_ONCE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_publish(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqttmessage_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}
/* Test_SRS_IOTHUB_MQTT_TRANSPORT_07_055: [ IoTHubTransport_MQTT_Common_DoWork shall send a device twin get property message upon successfully retrieving a SUBACK on device twin topics. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_device_twin_resend_message_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    
    // Disconnect due to sas reconnect
    g_current_ms += 3600*1000;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument_current_ms();
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqttmessage_create(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_MOST_ONCE, appMessage, appMsgSize))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mqtt_client_publish(TEST_MQTT_CLIENT_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mqttmessage_destroy(TEST_MQTT_MESSAGE_HANDLE))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_027: [IoTHubTransport_MQTT_Common_DoWork shall inspect the "waitingToSend" DLIST passed in config structure.] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_029: [IoTHubTransport_MQTT_Common_DoWork shall create a MQTT_MESSAGE_HANDLE and pass this to a call to mqtt_client_publish.] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_030: [IoTHubTransport_MQTT_Common_DoWork shall call mqtt_client_dowork everytime it is called if it is connected.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_STRING_type_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, NULL, NULL, NULL, NULL, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// As of now, this test covers: message id, correlation id, content type, content encoding, diagId, diagCreationTimeUtc.
// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_010: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the ContentType property and if found add the `value` as a system property in the format of `$.ct=<value>` ]
// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_011: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the ContentEncoding property and if found add the `value` as a system property in the format of `$.ce=<value>` ]
// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_014: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the diagnostic properties including diagid and diagCreationTimeUtc and if found both add them as system property in the format of `$.diagid` and `$.diagctx` respectively]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_message_system_properties_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, 
        "msg_id", "core_id", TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING, TEST_DIAG_ID, TEST_DIAG_CREATION_TIME_UTC, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_message_system_properties_succeeds_autoencode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false,
        "msg_id", "core_id", TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING, TEST_DIAG_ID, TEST_DIAG_CREATION_TIME_UTC, true);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_015: [ `IoTHubTransport_MQTT_Common_DoWork` shall check whether diagid and diagCreationTimeUtc be present simultaneously, treat as error if not]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_message_incomplete_diagnostic_system_properties_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false,
        NULL, NULL, NULL, NULL, TEST_DIAG_ID, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_034: [If IoTHubTransport_MQTT_Common_DoWork has resent the message two times then it shall fail the message and reconnect to IoTHub ... ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_resend_max_recount_reached_message_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));

    for (size_t index = 0; index < 3; index++)
    {
        IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
        g_current_ms = (tickcounter_ms_t)(index * 15 * 1000);
    }
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_connectFailCount_exceed_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    const size_t iterationCount = 7;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks();

    for (size_t index = 0; index < iterationCount-1; index++)
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_COUNTER_HANDLE, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }
    STRICT_EXPECTED_CALL(mqtt_client_dowork(TEST_MQTT_CLIENT_HANDLE))
        .IgnoreArgument(1);

    // act
    for (size_t index = 0; index < iterationCount; index++)
    {
        IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    }

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_001: [ IoTHubTransport_MQTT_Common_DoWork shall trigger reconnection if the mqtt_client_connect does not complete within `keepalive` seconds]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_mqtt_client_connect_times_out)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };

    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    g_current_ms += (5 * 600 * 2000); // 4+ minutes have passed.
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(TEST_MQTT_CLIENT_HANDLE, NULL, NULL))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(xio_destroy(TEST_XIO_HANDLE))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_030: [ IoTHubTransport_MQTT_Common_DoWork shall call mqtt_client_dowork everytime it is called if it is connected. ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_mqtt_client_connecting_times_out)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };

    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();
    setup_initialize_connection_mocks();
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    g_current_ms += (5 * 600 * 2000); // 4+ minutes have passed.
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(TEST_MQTT_CLIENT_HANDLE, NULL, NULL))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(xio_destroy(TEST_XIO_HANDLE))
        .IgnoreArgument(1);

    //
    // client is not connected now, so mqtt_client_dowork shouldn't be called
    //

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Test_SRS_IOTHUB_MQTT_TRANSPORT_07_023: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetSendStatus_InvalidHandleArgument_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetSendStatus(NULL, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_INVALID_ARG);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Test_SRS_IOTHUB_MQTT_TRANSPORT_07_023: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetSendStatus_InvalidStatusArgument_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetSendStatus(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_INVALID_ARG);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_024: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_IDLE if there are currently no event items to be sent or being sent.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetSendStatus_empty_waitingToSend_and_empty_waitingforAck_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(config.waitingToSend));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG)).IgnoreArgument(1);

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetSendStatus(handle, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_STATUS, status, IOTHUB_CLIENT_SEND_STATUS_IDLE);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_025: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_BUSY if there are currently event items to be sent or being sent.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetSendStatus_waitingToSend_not_empty_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    IOTHUB_MESSAGE_HANDLE eventMessageHandle = IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG);
    IOTHUB_MESSAGE_LIST newEntry;
    newEntry.messageHandle = eventMessageHandle;
    DList_InsertTailList(config.waitingToSend, &(newEntry.entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(config.waitingToSend));

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetSendStatus(handle, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_STATUS, status, IOTHUB_CLIENT_SEND_STATUS_BUSY);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    IoTHubMessage_Destroy(eventMessageHandle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_delivered_NULL_context_do_Nothing)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    // act
    g_fnMqttErrorCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_UNKNOWN_ERROR, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_delivered_MQTT_CLIENT_NO_PING_RESPONSE_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));

    // act
    g_fnMqttErrorCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_NO_PING_RESPONSE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_delivered_MQTT_CLIENT_NO_NETWORK_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_NETWORK));

    // act
    g_fnMqttErrorCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_CONNECTION_ERROR, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_delivered_MQTT_CLIENT_COMMUNICATION_ERROR_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR));

    // act
    g_fnMqttErrorCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_COMMUNICATION_ERROR, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MqttOpCompleteCallback_DISCONNECT_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MqttOpCompleteCallback_CONN_ACK_NOT_CONNECTED_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    CONNECT_ACK connack;
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_BAD_USERNAME_PASSWORD;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL));

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_MqttOpCompleteCallback_CONN_ACK_CONN_REFUSED_NOT_CONNECTED_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    CONNECT_ACK connack;
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_NOT_AUTHORIZED;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ConnectionStatusCallBack(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED));

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MqttOpCompleteCallback_PUBLISH_ACK_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    PUBLISH_ACK puback;
    puback.packetId = 2;

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendComplete(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_free(NULL))
        .IgnoreArgument(1);

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_PUBLISH_ACK, &puback, g_callbackCtx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_051: [ If msgHandle or callbackCtx is NULL, mqtt_notification_callback shall do nothing. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_message_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(NULL, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_051: [ If msgHandle or callbackCtx is NULL, mqtt_notification_callback shall do nothing. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_context_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_051: [ If msgHandle or callbackCtx is NULL, mqtt_notification_callback shall do nothing. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_Message_context_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_056: [ If type is IOTHUB_TYPE_TELEMETRY, then on success mqtt_notification_callback shall call IoTHubClientCore_LL_MessageCallback. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    g_msg_disposition = IOTHUBMESSAGE_ACCEPTED;
    setup_message_recv_msg_callback_mocks();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_055: [ if device_twin_msg_type is not RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClientCore_LL_ReportedStateComplete ] */
TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_twin_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    CONSTBUFFER_HANDLE cbh = CONSTBUFFER_Create(appMessage, appMsgSize);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = cbh;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    (void)IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);
    CONSTBUFFER_Destroy(cbh);
    umock_c_reset_all_calls();

    g_tokenizerIndex = 1;

    setup_message_recv_callback_device_twin_mocks("res");

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_twin_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    CONSTBUFFER_HANDLE cbh = CONSTBUFFER_Create(appMessage, appMsgSize);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = cbh;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    (void)IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);
    CONSTBUFFER_Destroy(cbh);
    umock_c_reset_all_calls();

    g_tokenizerIndex = 8;

    setup_message_recv_callback_device_twin_mocks("res");

    umock_c_negative_tests_snapshot();

    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);

    // act
    size_t calls_cannot_fail[] = { 6, 7, 8, 9, 10, 11, 12 };
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
        sprintf(tmp_msg, "MessageRecv_device_twin failure in test %zu/%zu", index, count);

        g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);
        // assert
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ If type is IOTHUB_TYPE_DEVICE_TWIN, then on success if msg_type is RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClientCore_LL_RetrievePropertyComplete... ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_sys_Properties_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(TEST_MQTT_MSG_TOPIC_W_1_PROP);
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(TEST_MQTT_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(appMessage, appMsgSize));
    STRICT_EXPECTED_CALL(STRING_construct(TEST_MQTT_MSG_TOPIC_W_1_PROP));
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));
    STRICT_EXPECTED_CALL(STRING_new());

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn("iothub-ack=Full");

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_MessageCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument_message_data();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubMessageHandle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_sys_Properties_succeed_autodecode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(TEST_MQTT_MSG_TOPIC_W_1_PROP);
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(TEST_MQTT_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(appMessage, appMsgSize));
    STRICT_EXPECTED_CALL(STRING_construct(TEST_MQTT_MSG_TOPIC_W_1_PROP));
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));
    STRICT_EXPECTED_CALL(STRING_new());

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn("iothub-ack=Full");

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument_size();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_MessageCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument_message_data();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_iotHubMessageHandle();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument_ptr();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ If type is IOTHUB_TYPE_DEVICE_TWIN, then on success if msg_type is RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClientCore_LL_RetrievePropertyComplete... ]*/
// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_012: [ If type is IOTHUB_TYPE_TELEMETRY and the system property `$.ct` is defined, its value shall be set on the IOTHUB_MESSAGE_HANDLE's ContentType property ]
// Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_013: [ If type is IOTHUB_TYPE_TELEMETRY and the system property `$.ce` is defined, its value shall be set on the IOTHUB_MESSAGE_HANDLE's ContentEncoding property ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_Properties_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_tokenizerIndex = 4;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_message_recv_with_properties_mocks(true, true, false);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_Properties_succeed_autodecode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_tokenizerIndex = 4;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_message_recv_with_properties_mocks(true, true, true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ If type is IOTHUB_TYPE_DEVICE_TWIN, then on success if msg_type is RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClientCore_LL_RetrievePropertyComplete... ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_Properties_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_message_recv_with_properties_mocks(false, false, false);

    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 0, 1, 8, 12, 13, 15, 16, 17, 19 };
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
        sprintf(tmp_msg, "g_fnMqttMsgRecv failure in test %zu/%zu", index, count);
        
        g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);
    }

    // assert

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_Properties_fail_autodecode)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_message_recv_with_properties_mocks(false, false, true);

    umock_c_negative_tests_snapshot();

    // act
    //size_t calls_cannot_fail[] = { 0, 1, 8, 12, 13, 15, 16, 17, 19 };
    size_t calls_cannot_fail[] = { 0, 1, 8, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25 };
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
        sprintf(tmp_msg, "g_fnMqttMsgRecv failure in test %zu/%zu", index, count);

        g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);
    }

    // assert

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ If type is IOTHUB_TYPE_DEVICE_TWIN, then on success if msg_type is RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClientCore_LL_RetrievePropertyComplete... ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_messagecallback_ABANDONED_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    g_msg_disposition = IOTHUBMESSAGE_ABANDONED;
    setup_message_recv_msg_callback_mocks();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_053: [ If type is IOTHUB_TYPE_DEVICE_METHODS, then on success mqtt_notification_callback shall call IoTHubClientCore_LL_DeviceMethodComplete. ] */
TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_method_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_tokenizerIndex = 8;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_message_recv_device_method_mocks();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_DeviceMethod_Response(handle, g_method_handle_value, NULL, 0, 0);
    IoTHubTransport_MQTT_Common_Destroy(handle);

}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_053: [ If type is IOTHUB_TYPE_DEVICE_METHODS, then on success mqtt_notification_callback shall call IoTHubClientCore_LL_DeviceMethodComplete. ] */
TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_method_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    setup_message_recv_device_method_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 6, 7, 8, 9, 11, 12, 13, 15, 16, 18, 20 };

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %zu/%zu", index, count);

        g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);
    }

    // assert

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_03_001: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceId, or both deviceKey and deviceSasToken are NULL.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceKey_null_and_deviceSasToken_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = NULL;
    deviceConfig.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_004: [ IoTHubTransport_MQTT_Common_Register shall return the TRANSPORT_LL_HANDLE as the IOTHUB_DEVICE_HANDLE. ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_succeeds_returns_transport)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(IGNORED_PTR_ARG));

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_ARE_EQUAL(void_ptr, handle, devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_004: [ IoTHubTransport_MQTT_Common_Register shall return the TRANSPORT_LL_HANDLE as the IOTHUB_DEVICE_HANDLE. ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_twice_fails_second_time)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_KEY);

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_IS_NULL(devHandle2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_003: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceId or deviceKey do not match the deviceId and deviceKey passed in during IoTHubTransport_MQTT_Common_Create.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceKey_mismatch_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = "not the right device key";
    deviceConfig.deviceSasToken = NULL;


    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(IGNORED_PTR_ARG));

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_003: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceId or deviceKey do not match the deviceId and deviceKey passed in during IoTHubTransport_MQTT_Common_Create.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceid_mismatch_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = "not a good id after all";
    deviceConfig.deviceKey = TEST_DEVICE_KEY;
    deviceConfig.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_002: [ IoTHubTransport_MQTT_Common_Register shall return NULL if device or waitingToSend are NULL.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_wts_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, NULL);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_002: [ IoTHubTransport_MQTT_Common_Register shall return NULL if device or waitingToSend are NULL.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_device_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, NULL, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_03_001: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceId, or both deviceKey and deviceSasToken are NULL.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceId_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = NULL;
    deviceConfig.deviceKey = TEST_DEVICE_KEY;
    deviceConfig.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_03_002: [ IoTHubTransport_MQTT_Common_Register shall return NULL if both deviceKey and deviceSasToken are provided.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceKey_and_deviceSasToken_both_provided_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = TEST_DEVICE_KEY;
    deviceConfig.deviceSasToken = TEST_DEVICE_SAS;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// SRS_IOTHUB_MQTT_TRANSPORT_17_004: [IoTHubTransport_MQTT_Common_Register shall return the TRANSPORT_LL_HANDLE as the IOTHUB_DEVICE_HANDLE.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceKey_null_and_deviceSas_valid_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = NULL;
    deviceConfig.deviceSasToken = TEST_DEVICE_SAS;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_001: [ IoTHubTransport_MQTT_Common_Register shall return NULL if the TRANSPORT_LL_HANDLE is NULL.]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_transport_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(NULL, &TEST_DEVICE_1, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_005: [ IoTHubTransport_MQTT_Common_Unregister shall return. ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unregister_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(IGNORED_PTR_ARG));

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    IoTHubTransport_MQTT_Common_Unregister(devHandle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_TRANSPORT_17_004: [ IoTHubTransport_MQTT_Common_Register shall return the TRANSPORT_LL_HANDLE as the IOTHUB_DEVICE_HANDLE. ]
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_Unregister_Register_returns_handle)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);
    IoTHubTransport_MQTT_Common_Unregister(devHandle);

    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(IGNORED_PTR_ARG));
    //EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_KEY);

    // act
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(devHandle2);
    ASSERT_ARE_EQUAL(void_ptr, handle, devHandle2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_02_001: [ If handle is NULL then IoTHubTransport_MQTT_Common_GetHostname shall fail and return NULL. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetHostname_with_NULL_handle_fails)
{
    //arrange

    //act
    STRING_HANDLE hostname = IoTHubTransport_MQTT_Common_GetHostname(NULL);

    //assert
    ASSERT_IS_NULL(hostname);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_02_002: [ Otherwise IoTHubTransport_MQTT_Common_GetHostname shall return a non-NULL STRING_HANDLE containg the hostname. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetHostname_with_non_NULL_handle_succeeds)
{
    //arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(TEST_HOSTNAME_STRING_HANDLE);

    //act
    STRING_HANDLE hostname = IoTHubTransport_MQTT_Common_GetHostname(handle);

    //assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_HOSTNAME_STRING_HANDLE, hostname);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    // No need to destroy STRING_HANDLE since it's a fake mem address.
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_045: [If 'subscribe_state' is set to IOTHUB_DEVICE_TWIN_NOTIFICATION_STATE then IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall construct the string $iothub/twin/PATCH/properties/desired] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_047: [On success IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return 0.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_Succeed)
{
    // arrange
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();
    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_044: [`IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin` shall construct the get state topic string and the notify state topic string.] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_047: [On success IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return 0.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_DoWork_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    setup_subscribe_devicetwin_dowork_mocks();

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ IoTHubTransport_MQTT_Common_DoWork shall subscribe to the Notification and get_state Topics if they are defined. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_DoWork_fails)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    umock_c_reset_all_calls();

    setup_subscribe_devicetwin_dowork_mocks();

    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 1, 2, 4 };
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %zu/%zu", index, count);

        IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

        // assert
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ IoTHubTransport_MQTT_Common_DoWork shall subscribe to the Notification and get_state Topics if they are defined. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_DoWork_2nd_fails)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    umock_c_reset_all_calls();

     STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqttmessage_create(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_MOST_ONCE, appMessage, appMsgSize))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mqtt_client_publish(TEST_MQTT_CLIENT_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(mqttmessage_destroy(TEST_MQTT_MESSAGE_HANDLE))
        .IgnoreArgument(1);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    // act
    size_t calls_cannot_fail[] = { 2, 5, 6, 7, 8 };
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %zu/%zu", index, count);

        IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

        // assert
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_045: [If 'subscribe_state' is set to IOTHUB_DEVICE_TWIN_NOTIFICATION_STATE then IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall construct the string $iothub/twin/PATCH/properties/desired] */
/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_047: [On success IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return 0.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_notify_Succeed)
{
    // arrange
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();
    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_handle_NULL_fail)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_043: [If the subscribe_state has been previously subscribed IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return a non-zero value.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();
    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin failure in test %zu/%zu", index, count);

        int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, tmp_msg);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_043: [If the subscribe_state has been previously subscribed IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return a non-zero value.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_notify_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();
    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin failure in test %zu/%zu", index, count);

        int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);
        // assert
        ASSERT_ARE_NOT_EQUAL(int, result, 0);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_003: [ IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall set the signaling flag for DEVICE_METHOD topic for the receiver's topic list. ]*/
/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_004: [ IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall construct the DEVICE_METHOD topic string for subscribe. ]*/
/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_005: [ IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall schedule the send of the subscription. ]*/
/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_007: [ On success IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return 0. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod_Succeed)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_001: [ If the parameter 'handle' is NULL than IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return a non-zero value. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod_handle_NULL_fail)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_006: [ Upon failure IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return a non-zero value. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod failure in test %zu/%zu", index, count);

        int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle);
        // assert
        ASSERT_ARE_NOT_EQUAL(int, result, 0);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_008: [ If the parameter 'handle' is NULL than IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall do nothing and return. ]*/
/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_009: [ If the MQTT transport has not been subscribed to DEVICE_METHOD topic IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall do nothing and return. ]*/
/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_010: [ IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall construct the DEVICE_METHOD topic string for unsubscribe. ]*/
/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_011: [ IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall send the unsubscribe. ]*/
/*Tests_SRS_IOTHUB_MQTT_TRANSPORT_12_012: [ IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall removes the signaling flag for DEVICE_METHOD topic from the receiver's topic list. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod_handle_OK)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mqtt_client_unsubscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod_handle_null)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod_negative_cases)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mqtt_client_unsubscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, 1))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(1);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    
    CONSTBUFFER_HANDLE cbh = CONSTBUFFER_Create(appMessage, appMsgSize);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = cbh;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    umock_c_reset_all_calls();

    setup_processItem_mocks(false);

    // act
    IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_PROCESS_OK, result_item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    CONSTBUFFER_Destroy(cbh);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_001: [ If handle or iothub_item are NULL then IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR.]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_iothub_item_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    umock_c_reset_all_calls();

    // act
    IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_PROCESS_ERROR, result_item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_001: [ If handle or iothub_item are NULL then IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR.]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_NULL_handle_fail)
{
    // arrange

    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = (CONSTBUFFER_HANDLE)0x48;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    umock_c_reset_all_calls();

    // act
    IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(NULL, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_PROCESS_ERROR, result_item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_002: [ If the mqtt is not ready to publish messages IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_NOT_CONNECTED.]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_not_connected_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    CONSTBUFFER_HANDLE cbh = CONSTBUFFER_Create(appMessage, appMsgSize);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = cbh;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    umock_c_reset_all_calls();

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);
    umock_c_reset_all_calls();

    // act
    IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_PROCESS_NOT_CONNECTED, result_item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    CONSTBUFFER_Destroy(cbh);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_006: [ If the item_type is not a supported type IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_CONTINUE.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_continue_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    CONSTBUFFER data_buff;
    data_buff.buffer = (const unsigned char*)0x46;
    data_buff.size = 22;

    CONSTBUFFER_HANDLE cbh = CONSTBUFFER_Create(appMessage, appMsgSize);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = cbh;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    umock_c_reset_all_calls();

    // act
    IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_TELEMETRY, &identity_info);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_PROCESS_CONTINUE, result_item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    CONSTBUFFER_Destroy(cbh);
}

/* Tests_SRS_IOTHUBCLIENT_LL_07_004: [ If any errors are encountered IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR. ]*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);

    CONSTBUFFER_HANDLE cbh = CONSTBUFFER_Create(appMessage, appMsgSize);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = cbh;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    umock_c_reset_all_calls();

    setup_processItem_mocks(true);
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(NULL)).IgnoreArgument_listEntry();

    STRICT_EXPECTED_CALL(gballoc_free(NULL)).IgnoreArgument_ptr();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 3, 6, 7, 8, 9 };

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_ProcessItem failure in test %zu/%zu", index, count);

        IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);
        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, IOTHUB_PROCESS_OK, result_item, tmp_msg);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    CONSTBUFFER_Destroy(cbh);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_041: [ If the parameter handle is NULL than IoTHubTransport_MQTT_Common_DeviceMethod_Response shall return a non-zero value. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DeviceMethod_Response_handle_NULL_fail)
{
    // arrange

    // act
    int result = IoTHubTransport_MQTT_Common_DeviceMethod_Response(NULL, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_042: [ IoTHubTransport_MQTT_Common_DeviceMethod_Response shall publish an mqtt message for the device method response. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DeviceMethod_Response_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    g_tokenizerIndex = 8;
    setup_message_recv_device_method_mocks();
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    umock_c_reset_all_calls();

    setup_devicemethod_response_mocks();

    // act
    int result = IoTHubTransport_MQTT_Common_DeviceMethod_Response(handle, g_method_handle_value, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_042: [ IoTHubTransport_MQTT_Common_DeviceMethod_Response shall publish an mqtt message for the device method response. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DeviceMethod_Response_NULL_RESPONSE_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();
    g_tokenizerIndex = 8;
    setup_message_recv_device_method_mocks();
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    umock_c_reset_all_calls();

    setup_devicemethod_response_mocks();

    // act
    int result = IoTHubTransport_MQTT_Common_DeviceMethod_Response(handle, g_method_handle_value, NULL, 0, TEST_DEVICE_STATUS_CODE);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/* Tests_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_051: [ If any error is encountered, IoTHubTransport_MQTT_Common_DeviceMethod_Response shall return a non-zero value. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DeviceMethod_Response_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport);

    umock_c_reset_all_calls();

    setup_devicemethod_response_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 0, 1, 4, 5, 6, 7 };

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_reset_all_calls();
        g_tokenizerIndex = 8;
        setup_message_recv_device_method_mocks();
        g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        (void)sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DeviceMethod_Response failure in test %zu/%zu", index, count);

        // act
        int result = IoTHubTransport_MQTT_Common_DeviceMethod_Response(handle, g_method_handle_value, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE);

        // assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, tmp_msg);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_10_001: [If messageData is NULL, IoTHubTransport_MQTT_Common_SendMessageDisposition shall fail and return IOTHUB_CLIENT_ERROR.] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SendMessageDisposition_NULL_fails)
{
    //arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SendMessageDisposition(NULL, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_10_002: [If any of the messageData fields are NULL, IoTHubTransport_MQTT_Common_SendMessageDisposition shall fail and return IOTHUB_CLIENT_ERROR. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SendMessageDisposition_Message_NULL_fails)
{
    //arrange
    MESSAGE_CALLBACK_INFO* md = (MESSAGE_CALLBACK_INFO*)malloc(sizeof(MESSAGE_CALLBACK_INFO));
    md->messageHandle = NULL;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_free(NULL)).IgnoreArgument_ptr();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SendMessageDisposition(md, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_MQTT_TRANSPORT_10_00#: [IoTHubTransport_MQTT_Common_SendMessageDisposition shall release the given data and return IOTHUB_CLIENT_OK. ] */
TEST_FUNCTION(IoTHubTransport_MQTT_Common_SendMessageDisposition_succeeds)
{
    //arrange
    MESSAGE_CALLBACK_INFO* md = (MESSAGE_CALLBACK_INFO*)malloc(sizeof(MESSAGE_CALLBACK_INFO));
    md->messageHandle = (IOTHUB_MESSAGE_HANDLE)0x42;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)).IgnoreArgument_iotHubMessageHandle();
    STRICT_EXPECTED_CALL(gballoc_free(NULL)).IgnoreArgument_ptr();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SendMessageDisposition(md, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(iothubtransport_mqtt_common_ut)
