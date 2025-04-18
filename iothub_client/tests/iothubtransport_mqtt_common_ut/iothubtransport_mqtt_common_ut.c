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

#if defined _MSC_VER
#pragma warning(disable: 4054) /* MSC incorrectly fires this */
#endif

static size_t TEST_METHOD_ID_VALUE = 0x1010;

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    if (ptr != (void*)TEST_METHOD_ID_VALUE &&
        ptr != (void*)&TEST_METHOD_ID_VALUE)
    {
        free(ptr);
    }
}

void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/constbuffer.h"

#include "azure_umqtt_c/mqtt_client.h"

#include "internal/iothub_message_private.h"
#include "iothub_client_options.h"
#include "internal/iothub_client_retry_control.h"

#include "azure_c_shared_utility/xio.h"

#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/urlencode.h"

#include "internal/iothub_transport_ll_private.h"

#define RESEND_TIMEOUT_VALUE_MS            1*60*1000 // should match value in iothubtransport_mqtt_common.c * 1000 so it's in ms
#define MSG_TIMEOUT_VALUE_MS               2*60*1000 // should match value in iothubtransport_mqtt_common.c * 1000 so it's in ms

MOCKABLE_FUNCTION(, bool, Transport_MessageCallbackFromInput, IOTHUB_MESSAGE_HANDLE, message, void*, ctx);
MOCKABLE_FUNCTION(, bool, Transport_MessageCallback, IOTHUB_MESSAGE_HANDLE, message, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_ConnectionStatusCallBack, IOTHUB_CLIENT_CONNECTION_STATUS, status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_SendComplete_Callback, PDLIST_ENTRY, completed, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, ctx);
MOCKABLE_FUNCTION(, const char*, Transport_GetOption_Product_Info_Callback, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_ReportedStateComplete_Callback, uint32_t, item_id, int, status_code, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_RetrievePropertyComplete_Callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, ctx);
MOCKABLE_FUNCTION(, int, Transport_DeviceMethod_Complete_Callback, const char*, method_name, const unsigned char*, payLoad, size_t, size, METHOD_HANDLE, response_id, void*, ctx);
MOCKABLE_FUNCTION(, const char*, Transport_GetOption_Model_Id_Callback, void*, ctx);

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

static const char* my_Transport_GetOption_Product_Info_Callback(void* ctx)
{
    (void)ctx;
    return "product_info";
}

static const char* my_Transport_GetOption_Model_Id_Callback(void* ctx)
{
    (void)ctx;
    return "dtmi:testDeviceCapabilityModel;1";
}

static const char* TEST_STRING_VALUE = "Test string value";
static const char* TEST_DEVICE_ID = "thisIsDeviceID";
static const char* TEST_MODULE_ID = "thisIsModuleID";
static const char* TEST_DEVICE_KEY = "thisIsDeviceKey";
static const char* TEST_DEVICE_SAS = "thisIsDeviceSasToken";
static const char* TEST_IOTHUB_NAME = "thisIsIotHubName";
static const char* TEST_IOTHUB_SUFFIX = "thisIsIotHubSuffix";
static const char* TEST_PROTOCOL_GATEWAY_HOSTNAME = NULL;
static const char* TEST_PROTOCOL_GATEWAY_HOSTNAME_NON_NULL = "ssl://thisIsAGatewayHostName.net";
static const char* TEST_VERY_LONG_DEVICE_ID = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
static const char* TEST_MQTT_MESSAGE_TOPIC = "devices/myDeviceId/messages/devicebound/#";
static const char* TEST_MQTT_MSG_TOPIC = "devices/myDeviceId/messages/devicebound/iothub-ack=Full&%24.to=%2Fdevices%2FmyDeviceId%2Fmessages%2FdeviceBound&%24.cid=123&%24.uid=456";
static const char* TEST_MQTT_MSG_TOPIC_W_1_PROP = "devices/myDeviceId/messages/devicebound/iothub-ack=Full&propName=PropValue&DeviceInfo=smokeTest&%24.to=%2Fdevices%2FmyDeviceId%2Fmessages%2FdeviceBound&%24.cid&%24.uid";
static const char* TEST_MQTT_MSG_TOPIC_GET_TWIN = "$iothub/twin/res/200/?$rid=2";
static const char* TEST_MQTT_INPUT_QUEUE_SUBSCRIBE_NAME_1 = "devices/thisIsDeviceID/modules/thisIsModuleID/#";
static const char* TEST_MQTT_INPUT_1 = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs/input1/%24.cdid=connected_device&%24.cmid=connected_module/";
static const char* TEST_MQTT_INPUT_NO_PROPERTIES = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs/input1/";
static const char* TEST_MQTT_INPUT_MISSING_INPUT_QUEUE_NAME = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs";
static const char* TEST_INPUT_QUEUE_1 = "input1";
static const char* TEST_MQTT_DEV_TWIN_MSG_TOPIC = "$iothub/twin/$res/200/?$rid=2";
static const char* TEST_MQTT_DEV_METHOD_MSG = "$iothub/methods/POST/method_name/?$rid=b";
static const char* TEST_MQTT_DEV_TWIN_MSG_TOPIC_MISSING_REQUEST_ID = "$iothub/twin/$res/200";
static const char* TEST_MQTT_DEV_TWIN_MSG_TOPIC_INVALID_REQUEST_ID = "$iothub/twin/$res/200/?$NotSetRequestId=2";


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
static const char* TEST_MESSAGE_CREATION_TIME_UTC = "2010-01-01T01:00:00.000Z";
static const char* TEST_OUTPUT_NAME = "TestOutputName";
static const char* TEST_COMPONENT_NAME = "TestComponentName";

static const char* PROPERTY_SEPARATOR = "&";
static const char* DIAGNOSTIC_CONTEXT_CREATION_TIME_UTC_PROPERTY = "creationtimeutc";

static IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA TEST_DIAG_DATA;

static MQTT_TRANSPORT_PROXY_OPTIONS* expected_MQTT_TRANSPORT_PROXY_OPTIONS;

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
static const MAP_HANDLE TEST_MESSAGE_PROP_MAP = (MAP_HANDLE)0x1212;

static char appMessageString[] = "App Message String";
static uint8_t appMessage[] = { 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x4d, 0x73, 0x67 };
static const size_t appMsgSize = sizeof(appMessage) / sizeof(appMessage[0]);
static CONSTBUFFER g_cbuff;

static IOTHUB_CLIENT_CONFIG g_iothubClientConfig = { 0 };
static DLIST_ENTRY g_waitingToSend;

static tickcounter_ms_t g_current_ms;
static size_t g_tokenizerIndex;

static CONSTBUFFER_HANDLE TEST_CONST_BUFFER_HANDLE = (CONSTBUFFER_HANDLE)0x2331;

// Use #define and not const because switch statement that consumes these assumes they're not const and won't compile.
#define PARSE_SLASHES_FOR_INPUT_QUEUE_INDEX_5 (205)


#define PARSE_SLASHES_FOR_INPUT_QUEUE_NO_TOKENS               (500)
#define NUM_DOWORK_VALUE                1

static const unsigned char* TEST_DEVICE_METHOD_RESPONSE = (const unsigned char*)0x62;
static size_t TEST_DEVICE_RESP_LENGTH = 1;
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

#define STATUS_CODE_TIMEOUT_VALUE           408


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

#define TEST_RETRY_POLICY IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define TEST_RETRY_TIMEOUT_SECS 60
#define TEST_MAX_DISCONNECT_VALUE 50

//Callbacks for Testing
static ON_MQTT_MESSAGE_RECV_CALLBACK g_fnMqttMsgRecv;
static ON_MQTT_OPERATION_CALLBACK g_fnMqttOperationCallback;
static ON_MQTT_ERROR_CALLBACK g_fnMqttErrorCallback;
static void* g_callbackCtx;
static void* g_errorcallbackCtx;
static bool g_nullMapVariable;
static bool g_skip_disconnect_callback;
static ON_MQTT_DISCONNECTED_CALLBACK g_disconnect_callback;
static void* g_disconnect_callback_ctx;
static TRANSPORT_CALLBACKS_INFO transport_cb_info;
static void* transport_cb_ctx = (void*)0x499922;

typedef struct TEST_MESSAGE_DISPOSITION_CONTEXT_TAG
{
    uint16_t packet_id;
    QOS_VALUE qos_value;
} TEST_MESSAGE_DISPOSITION_CONTEXT;

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

static char* my_IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, uint64_t expiry_time_relative_seconds, const char* keyname)
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

static MESSAGE_DISPOSITION_CONTEXT_HANDLE g_messageDispositionContext;
static MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION g_messageDispositionDestroyFunction;
static void my_IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    (void)iotHubMessageHandle;

    if (g_messageDispositionContext != NULL)
    {
        g_messageDispositionDestroyFunction(g_messageDispositionContext);
        g_messageDispositionContext = NULL;
        g_messageDispositionDestroyFunction = NULL;
    }
}

static IOTHUB_MESSAGE_RESULT my_IoTHubMessage_SetDispositionContext(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, MESSAGE_DISPOSITION_CONTEXT_HANDLE dispositionContext, MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION dispositionContextDestroyFunction)
{
    (void)iotHubMessageHandle;
    g_messageDispositionContext = dispositionContext;
    g_messageDispositionDestroyFunction = dispositionContextDestroyFunction;
    return IOTHUB_MESSAGE_OK;
}


static int my_Transport_DeviceMethod_Complete_Callback(const char* method_name, const unsigned char* payLoad, size_t size, METHOD_HANDLE response_id, void* ctx)
{
    (void)ctx;
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

static MQTT_CLIENT_HANDLE my_mqtt_client_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* callbackCtx, ON_MQTT_ERROR_CALLBACK errorCallback, void* errorcallbackCtx)
{
    g_fnMqttMsgRecv = msgRecv;
    g_fnMqttOperationCallback = opCallback;
    g_callbackCtx = callbackCtx;
    g_fnMqttErrorCallback = errorCallback;
    g_errorcallbackCtx = errorcallbackCtx;
    return (MQTT_CLIENT_HANDLE)my_gballoc_malloc(sizeof(MQTT_CLIENT_HANDLE));
}

static int my_mqtt_client_disconnect(MQTT_CLIENT_HANDLE handle, ON_MQTT_DISCONNECTED_CALLBACK callback, void* ctx)
{
    (void)handle;
    g_disconnect_callback = callback;
    g_disconnect_callback_ctx = ctx;
    return 0;
}

static void my_mqtt_client_deinit(MQTT_CLIENT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static void my_mqtt_client_dowork(MQTT_CLIENT_HANDLE handle)
{
    (void)handle;
}

static STRING_TOKENIZER_HANDLE my_STRING_TOKENIZER_create_from_char(const char* input)
{
    (void)input;
    return (STRING_TOKENIZER_HANDLE)my_gballoc_malloc(sizeof(STRING_TOKENIZER_HANDLE));
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
            g_tokenizerIndex = 6;
            break;
        case 4:
            text = "%24.to=%2Fdevices%2FmyDeviceId%2Fmessages%2FdeviceBound&%24.cid";
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
        result = MU_FAILURE;
    }
    return result;
}

static void my_STRING_TOKENIZER_destroy(STRING_TOKENIZER_HANDLE handle)
{
    my_gballoc_free(handle);
}

static STRING_HANDLE my_SASToken_Create(STRING_HANDLE key, STRING_HANDLE scope, STRING_HANDLE keyName, uint64_t expiry)
{
    (void)key;
    (void)scope;
    (void)keyName;
    (void)expiry;
    return (STRING_HANDLE)my_gballoc_malloc(sizeof(STRING_HANDLE));
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
    return (XIO_HANDLE)my_gballoc_malloc(sizeof(XIO_HANDLE));
}

static void my_xio_destroy(XIO_HANDLE ioHandle)
{
    my_gballoc_free(ioHandle);
}

static TICK_COUNTER_HANDLE my_tickcounter_create(void)
{
    return (TICK_COUNTER_HANDLE)my_gballoc_malloc(sizeof(TICK_COUNTER_HANDLE));
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

static double my_get_difftime(time_t stopTime, time_t startTime)
{
    return (double)(stopTime - startTime);
}

static void my_ThreadAPI_Sleep(unsigned int milliseconds)
{
    (void)milliseconds;
    // this is for disconnect callback
    if (g_disconnect_callback != NULL && !g_skip_disconnect_callback)
    {
        g_disconnect_callback(g_disconnect_callback_ctx);
    }
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
    return (XIO_HANDLE)my_gballoc_malloc(sizeof(XIO_HANDLE));
}

static DEVICE_TWIN_UPDATE_STATE get_twin_update_state;
static const unsigned char* get_twin_payLoad;
static size_t get_twin_size;
static void* get_twin_userContextCallback;
static void on_get_device_twin_completed_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    get_twin_update_state = update_state;
    get_twin_payLoad = payLoad;
    get_twin_size = size;
    get_twin_userContextCallback = userContextCallback;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

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

    transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
    transport_cb_info.twin_retrieve_prop_complete_cb = Transport_Twin_RetrievePropertyComplete_Callback;
    transport_cb_info.twin_rpt_state_complete_cb = Transport_Twin_ReportedStateComplete_Callback;
    transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
    transport_cb_info.connection_status_cb = Transport_ConnectionStatusCallBack;
    transport_cb_info.prod_info_cb = Transport_GetOption_Product_Info_Callback;
    transport_cb_info.msg_input_cb = Transport_MessageCallbackFromInput;
    transport_cb_info.msg_cb = Transport_MessageCallback;
    transport_cb_info.method_complete_cb = Transport_DeviceMethod_Complete_Callback;
    transport_cb_info.get_model_id_cb = Transport_GetOption_Model_Id_Callback;

    g_cbuff.buffer = appMessage;
    g_cbuff.size = appMsgSize;

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
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_CONTENT_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_TWIN_UPDATE_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_DISPOSITION_CONTEXT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_RESULT, int);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_realloc, my_gballoc_realloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_realloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Sleep, my_ThreadAPI_Sleep);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat_with_STRING, my_STRING_concat_with_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, -1);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, my_STRING_c_str);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(URL_Encode, my_URL_Encode);
    REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(URL_DecodeString, my_URL_DecodeString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_DecodeString, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Transport_GetOption_Product_Info_Callback, my_Transport_GetOption_Product_Info_Callback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_GetOption_Product_Info_Callback, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Transport_GetOption_Model_Id_Callback, my_Transport_GetOption_Model_Id_Callback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_GetOption_Model_Id_Callback, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHub_Transport_ValidateCallbacks, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHub_Transport_ValidateCallbacks, __LINE__);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceKey, TEST_DEVICE_KEY);

    REGISTER_GLOBAL_MOCK_RETURN(Transport_MessageCallback, true);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_MessageCallback, false);

    REGISTER_GLOBAL_MOCK_HOOK(Transport_DeviceMethod_Complete_Callback, my_Transport_DeviceMethod_Complete_Callback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_DeviceMethod_Complete_Callback, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_GetContentType, my_IoTHubMessage_GetContentType);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetContentType, IOTHUBMESSAGE_UNKNOWN);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetString, appMessageString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetString, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_CreateFromByteArray, TEST_IOTHUB_MSG_BYTEARRAY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_CreateFromByteArray, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_GetByteArray, my_IoTHubMessage_GetByteArray);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetOutputName, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_Destroy, my_IoTHubMessage_Destroy);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_SetDispositionContext, my_IoTHubMessage_SetDispositionContext);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetDispositionContext, IOTHUB_MESSAGE_ERROR);

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
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_connect, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_deinit, my_mqtt_client_deinit);

    REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_disconnect, my_mqtt_client_disconnect);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_disconnect, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_subscribe, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_subscribe, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_unsubscribe, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_unsubscribe, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_publish, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_publish, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURNS(mqttmessage_setIsDuplicateMsg, 0, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_create, TEST_MQTT_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_create_in_place, TEST_MQTT_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_create_in_place, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_getApplicationMsg, &TEST_APP_PAYLOAD);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_getApplicationMsg, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_getTopicName, TEST_MQTT_MSG_TOPIC);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_getTopicName, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_create_from_char, my_STRING_TOKENIZER_create_from_char);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_create_from_char, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_get_next_token, my_STRING_TOKENIZER_get_next_token);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_get_next_token, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_destroy, my_STRING_TOKENIZER_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(SASToken_Create, my_SASToken_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(SASToken_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, TEST_TIME_T);

    REGISTER_GLOBAL_MOCK_HOOK(get_difftime, my_get_difftime);

    REGISTER_GLOBAL_MOCK_HOOK(xio_create, my_xio_create);

    REGISTER_GLOBAL_MOCK_RETURN(xio_close, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_close, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(xio_setoption, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_setoption, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(xio_destroy, my_xio_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_create, my_tickcounter_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_destroy, my_tickcounter_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_get_current_ms, my_tickcounter_get_current_ms);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_get_current_ms, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(CONSTBUFFER_Create, TEST_CONST_BUFFER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(CONSTBUFFER_Create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(CONSTBUFFER_GetContent, &g_cbuff);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_SasToken, my_IoTHubClient_Auth_Get_SasToken);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_SasToken, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_SasToken_Expiry, 3600);

    REGISTER_GLOBAL_MOCK_HOOK(DList_InitializeListHead, real_DList_InitializeListHead);
    REGISTER_GLOBAL_MOCK_HOOK(DList_IsListEmpty, real_DList_IsListEmpty);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertTailList, real_DList_InsertTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertHeadList, real_DList_InsertHeadList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_AppendTailList, real_DList_AppendTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveEntryList, real_DList_RemoveEntryList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveHeadList, real_DList_RemoveHeadList);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetInputName, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetInputName, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_IsSecurityMessage, false);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetConnectionDeviceId, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetConnectionDeviceId, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetConnectionModuleId, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetConnectionModuleId, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_create, TEST_RETRY_CONTROL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_should_retry, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_should_retry, 1);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_set_option, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_set_option, 1);

    REGISTER_UMOCK_ALIAS_TYPE(RETRY_CONTROL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(RETRY_ACTION, int);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

static void reset_test_data()
{
    g_fnMqttMsgRecv = NULL;
    g_fnMqttOperationCallback = NULL;
    g_callbackCtx = NULL;
    g_fnMqttErrorCallback = NULL;
    g_errorcallbackCtx = NULL;
    g_method_handle_value = NULL;

    // We set the counter somewhat far off into the future.  Previously
    // there were bugs that this UT let slip through because timers were initialized
    // to 0 in UT itself AND the product code was wrongly leaving timers as 0.
    // Now if a product timer was uninitialized at 0, it would trigger unexpected timeouts.
    g_current_ms = 1000*60*30;
    g_tokenizerIndex = 0;
    g_nullMapVariable = true;

    expected_MQTT_TRANSPORT_PROXY_OPTIONS = NULL;
    g_disconnect_callback = NULL;
    g_disconnect_callback_ctx = NULL;
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    TEST_MUTEX_ACQUIRE(test_serialize_mutex);

    reset_test_data();
    real_DList_InitializeListHead(&g_waitingToSend);

    umock_c_reset_all_calls();

    g_skip_disconnect_callback = false;

    get_twin_update_state = DEVICE_TWIN_UPDATE_COMPLETE;
    get_twin_payLoad = NULL;
    get_twin_size = 0;
    get_twin_userContextCallback = NULL;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

static void SetupIothubTransportConfig(IOTHUBTRANSPORT_CONFIG* config, const char* deviceId, const char* deviceKey, const char* iotHubName,
    const char* iotHubSuffix, const char* protocolGatewayHostName, const char* moduleId)
{
    g_iothubClientConfig.protocol = TEST_PROTOCOL;
    g_iothubClientConfig.deviceId = deviceId;
    g_iothubClientConfig.deviceKey = deviceKey;
    g_iothubClientConfig.deviceSasToken = NULL;
    g_iothubClientConfig.iotHubName = iotHubName;
    g_iothubClientConfig.iotHubSuffix = iotHubSuffix;
    g_iothubClientConfig.protocolGatewayHostName = protocolGatewayHostName;
    config->moduleId = moduleId;
    config->waitingToSend = &g_waitingToSend;
    config->upperConfig = &g_iothubClientConfig;
    config->auth_module_handle = TEST_IOTHUB_AUTHORIZATION_HANDLE;
}

static void SetupIothubTransportConfigWithKeyAndSasToken(IOTHUBTRANSPORT_CONFIG* config, const char* deviceId, const char* deviceKey, const char* deviceSasToken,
    const char* iotHubName, const char* iotHubSuffix, const char* protocolGatewayHostName, const char* moduleId)
{
    g_iothubClientConfig.protocol = TEST_PROTOCOL;
    g_iothubClientConfig.deviceId = deviceId;
    g_iothubClientConfig.deviceKey = deviceKey;
    g_iothubClientConfig.deviceSasToken = deviceSasToken;
    g_iothubClientConfig.iotHubName = iotHubName;
    g_iothubClientConfig.iotHubSuffix = iotHubSuffix;
    g_iothubClientConfig.protocolGatewayHostName = protocolGatewayHostName;
    config->moduleId = moduleId;
    config->waitingToSend = &g_waitingToSend;
    config->upperConfig = &g_iothubClientConfig;
    config->auth_module_handle = TEST_IOTHUB_AUTHORIZATION_HANDLE;
}

static void setup_IoTHubTransport_MQTT_Common_Create_mocks(bool use_gateway, const char* moduleId)
{
    STRICT_EXPECTED_CALL(IoTHub_Transport_ValidateCallbacks(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tickcounter_create());
    STRICT_EXPECTED_CALL(retry_control_create(DEFAULT_RETRY_POLICY, DEFAULT_RETRY_TIMEOUT_IN_SECONDS));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));

    if (moduleId != NULL)
    {
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    }

    EXPECTED_CALL(mqtt_client_init(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    if (use_gateway)
    {
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    }

    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG)); // pending_get_twin_queue
    STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG))
        .IgnoreArgument(1).SetReturn(TEST_SMALL_TIME_T).CallCannotFail();
}

// Initial calls when receiving a message
static void setup_message_receive_initial_calls(const char* topicName, bool isSubscribedToInputQueue)
{
    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(topicName);
    if (isSubscribedToInputQueue)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(NULL);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(TEST_MQTT_INPUT_QUEUE_SUBSCRIBE_NAME_1);
    }
    else
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(TEST_MQTT_MESSAGE_TOPIC);
    }
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(TEST_MQTT_MESSAGE_HANDLE)).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(appMessage, appMsgSize));
}

// Calls invoked when adding an application custom property to a C2D or IoT Hub module to module message
static void set_expected_calls_for_custom_message_property(bool auto_decode)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    if (auto_decode)
    {
        STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    }

    STRICT_EXPECTED_CALL(Map_AddOrUpdate(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (auto_decode)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void setup_set_message_disposition_context()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mqttmessage_getPacketId(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mqttmessage_getQosType(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void setup_message_recv_with_properties_mocks(
    bool has_content_type, 
    bool has_content_encoding, 
    bool auto_decode, 
    bool msgCbResult)
{
    setup_message_receive_initial_calls(TEST_MQTT_MSG_TOPIC_W_1_PROP, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(TEST_MQTT_MESSAGE_TOPIC);
    EXPECTED_CALL(STRING_TOKENIZER_create_from_char(TEST_MQTT_MSG_TOPIC_W_1_PROP));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));


    if (has_content_type)
    {
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .SetReturn("%24.ct=application/json")
            .CallCannotFail();
        if (auto_decode)
        {
            STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        }
        STRICT_EXPECTED_CALL(IoTHubMessage_SetContentTypeSystemProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (auto_decode)
        {
            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        }
    }

    if (has_content_encoding)
    {
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .SetReturn("%24.ce=utf8")
            .CallCannotFail();
        if (auto_decode)
        {
            STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        }
        STRICT_EXPECTED_CALL(IoTHubMessage_SetContentEncodingSystemProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (auto_decode)
        {
            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        }
    }

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
            .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("propName=propValue")
        .CallCannotFail();

    set_expected_calls_for_custom_message_property(auto_decode);

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
            .CallCannotFail();
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
    setup_set_message_disposition_context();
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(msgCbResult);
    
    if (!msgCbResult)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
}

static void setup_connection_success_mocks()
{
    STRICT_EXPECTED_CALL(retry_control_reset(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, IGNORED_PTR_ARG));
}

static void setup_initialize_reconnection_mocks()
{
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    STRICT_EXPECTED_CALL(retry_control_should_retry(TEST_RETRY_CONTROL_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID).CallCannotFail();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL).CallCannotFail();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
}

static void setup_devicemethod_response_mocks()
{
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    EXPECTED_CALL(mqttmessage_create_in_place(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_MOST_ONCE, appMessage, appMsgSize))
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


static void setup_initialize_connection_mocks(bool useModelId)
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_GetOption_Model_Id_Callback(IGNORED_PTR_ARG)).SetReturn(useModelId ? TEST_STRING_VALUE : NULL);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();

    if (useModelId)
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID).CallCannotFail();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE).CallCannotFail();

    // from GetTransportProviderIfNecessary()
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_HOST_NAME).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();
}

static void setup_subscribe_devicetwin_dowork_mocks()
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(mqtt_client_subscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    // process_queued_ack_messages
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void setup_IoTHubTransport_MQTT_Common_DoWork_mocks()
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_MQTT_MSG_TOPIC).CallCannotFail();
    EXPECTED_CALL(mqtt_client_subscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void setup_invoke_message_callback_mocks(IOTHUB_CLIENT_CONFIRMATION_RESULT expected, bool removeEntryList, bool removeHeadList)
{
    if (removeEntryList)
    {
        EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    }

    if (removeHeadList)
    {
        EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    }

    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, expected, transport_cb_ctx));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(
    const char* const** ppKeys,
    const char* const** ppValues,
    size_t propCount,
    IOTHUB_MESSAGE_HANDLE msg_handle,
    bool resend,
    bool msg_expiry,
    bool disconnect,
    const char* msg_id,
    const char* core_id,
    const char* content_type,
    const char* content_encoding,
    const char* diag_id,
    const char* diag_creation_time_utc,
    const char* message_creation_time_utc,
    bool auto_urlencode,
    const char* output_name,
    const char* component_name,
    bool security_msg)
{
    TEST_DIAG_DATA.diagnosticId = (char*)diag_id;
    TEST_DIAG_DATA.diagnosticCreationTimeUtc = (char*)diag_creation_time_utc;
    if (!msg_expiry || resend)
    {
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    }
    if (resend) {
#ifdef RUN_SFC_TESTS
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(msg_handle));
        if (propCount == 0)
        {
            EXPECTED_CALL(Map_GetInternals(TEST_MESSAGE_PROP_MAP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
        else
        {
            STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MESSAGE_PROP_MAP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .CopyOutArgumentBuffer(2, &ppKeys, sizeof(ppKeys))
                .CopyOutArgumentBuffer(3, &ppValues, sizeof(ppValues))
                .CopyOutArgumentBuffer(4, &propCount, sizeof(propCount));

            for (size_t i=0; i < propCount; i++)
            {
                if (auto_urlencode)
                {
                    STRICT_EXPECTED_CALL(URL_EncodeString((const char*)ppKeys[i]));
                    STRICT_EXPECTED_CALL(URL_EncodeString((const char*)ppValues[i]));
                    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
                    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
                    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
                    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
                }
            }
        }
#endif //RUN_SFC_TESTS
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));  //ericwol
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));  //ericwol
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));  //ericwol
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));  //ericwol
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));  //ericwol
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));  //ericwol
    }
    if (!msg_expiry || resend)
    {
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(msg_handle));
    if (msg_handle == TEST_IOTHUB_MSG_STRING)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(TEST_IOTHUB_MSG_STRING));
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MSG_BYTEARRAY, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    if (!resend)
    {
        EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    //Add Properties
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(msg_handle));
    if (propCount == 0)
    {
        EXPECTED_CALL(Map_GetInternals(TEST_MESSAGE_PROP_MAP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MESSAGE_PROP_MAP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &ppKeys, sizeof(ppKeys))
            .CopyOutArgumentBuffer(3, &ppValues, sizeof(ppValues))
            .CopyOutArgumentBuffer(4, &propCount, sizeof(propCount));

        for (size_t i=0; i < propCount; i++)
        {
            if (auto_urlencode)
            {
                STRICT_EXPECTED_CALL(URL_EncodeString((const char*)ppKeys[i]));
                STRICT_EXPECTED_CALL(URL_EncodeString((const char*)ppValues[i]));
                STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
                STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
                STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
                STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
            }
        }
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_IsSecurityMessage(IGNORED_PTR_ARG)).SetReturn(security_msg);
    STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG)).SetReturn(core_id);
    if (auto_urlencode && (core_id != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG)).SetReturn(msg_id);
    if (auto_urlencode && (msg_id != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(IGNORED_PTR_ARG)).SetReturn(content_type);
    if (auto_urlencode && (content_type != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(IGNORED_PTR_ARG)).SetReturn(content_encoding);
    if (security_msg || (auto_urlencode && (content_encoding != NULL)))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageCreationTimeUtcSystemProperty(IGNORED_PTR_ARG)).SetReturn(message_creation_time_utc);
    if (auto_urlencode && (message_creation_time_utc != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    if (security_msg)
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(IoTHubMessage_GetOutputName(IGNORED_PTR_ARG)).SetReturn(output_name);
    if (auto_urlencode && output_name != NULL)
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(IoTHubMessage_GetComponentName(IGNORED_PTR_ARG)).SetReturn(component_name);
    if (auto_urlencode && (component_name != NULL))
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(IoTHubMessage_GetDiagnosticPropertyData(IGNORED_PTR_ARG)).SetReturn(&TEST_DIAG_DATA);

    bool validMessage = true;
    if (diag_id != NULL && diag_creation_time_utc != NULL)
    {
        STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else if (diag_id != NULL || diag_creation_time_utc != NULL)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        validMessage = false;
    }

    //Publish
    if (validMessage)
    {
        EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        EXPECTED_CALL(mqttmessage_create_in_place(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_LEAST_ONCE, IGNORED_PTR_ARG, appMsgSize));
        STRICT_EXPECTED_CALL(mqttmessage_setIsDuplicateMsg(IGNORED_PTR_ARG, resend));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqtt_client_publish(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqttmessage_destroy(TEST_MQTT_MESSAGE_HANDLE));
        EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        if (!resend)
        {
            EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
    }
    else
    {
        setup_invoke_message_callback_mocks(IOTHUB_CLIENT_CONFIRMATION_ERROR, true, false);
    }
    if (!resend || disconnect) 
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    if (!msg_expiry || !resend)
    {
        // removeExpiredTwinRequests
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void setup_calls_for_next_token_with_slash(int expectedTokens)
{
    for (int i = 0; i < expectedTokens; i++)
    {
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "/")).SetReturn(0);
    }
}

static void setup_message_recv_device_method_mocks()
{
    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(TEST_MQTT_DEV_METHOD_MSG);
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create_from_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_new());
    setup_calls_for_next_token_with_slash(4);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "/")).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
        .SetReturn(7);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("?$rid=b")
        .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(TEST_MQTT_MESSAGE_HANDLE)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(Transport_DeviceMethod_Complete_Callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setup_processItem_mocks(bool fail_test)
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&g_cbuff);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(mqttmessage_create_in_place(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_MOST_ONCE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    if (!fail_test)
    {
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(mqtt_client_publish(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqttmessage_destroy(TEST_MQTT_MESSAGE_HANDLE));
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
        .IgnoreArgument_t()
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_output()
        .IgnoreArgument_delimiters()
        .IgnoreArgument_t()
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_output()
        .IgnoreArgument_delimiters()
        .IgnoreArgument_t()
        .SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(token_type)
        .CallCannotFail();

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("200")
        .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("?$rid=4")
        .IgnoreArgument_handle()
        .CallCannotFail();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_t();
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .CallCannotFail();

    EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_Twin_ReportedStateComplete_Callback(1, 200, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();
    EXPECTED_CALL(gballoc_free(NULL));
}

static void setup_message_recv_msg_callback_mocks(bool msgCbResult)
{
    setup_message_receive_initial_calls(TEST_MQTT_MSG_TOPIC, false);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(TEST_MQTT_MESSAGE_TOPIC);
    EXPECTED_CALL(STRING_TOKENIZER_create_from_char(TEST_MQTT_MSG_TOPIC));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(1);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    setup_set_message_disposition_context();
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(msgCbResult);

    if (!msgCbResult)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
}

static TRANSPORT_LL_HANDLE setup_iothub_mqtt_connection(IOTHUBTRANSPORT_CONFIG* config)
{
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    CONNECT_ACK connack;
    connack.isSessionPresent = true;
    connack.returnCode = CONNECTION_ACCEPTED;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    return handle;
}

static XIO_HANDLE get_IO_transport_fail(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options)
{
    (void)fully_qualified_name;
    (void)mqtt_transport_proxy_options;
    return NULL;
}

static void setup_subscribe_inputqueue_dowork_mocks()
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(mqtt_client_subscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    // process_queued_ack_messages
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void set_expected_calls_around_unsubscribe()
{
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(mqtt_client_unsubscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, 1));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_proxy_default_copy()
{
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"));
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_parameter_Succeed)
{
    //arrange

    //act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(NULL, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_IS_NULL(result);

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_config_parameter_fails)
{
    //arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    config.waitingToSend = &g_waitingToSend;

    //act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_IS_NULL(result);

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_waitingToSend_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    config.waitingToSend = NULL;

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_protocol_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    g_iothubClientConfig.protocol = NULL;

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_device_id_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, NULL, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_deviceSasToken_blank_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, TEST_EMPTY_STRING, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_deviceKey_blank_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, "", TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_iothub_name_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, NULL, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_very_long_device_id_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_VERY_LONG_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_empty_device_id_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_EMPTY_STRING, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_empty_device_key_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_EMPTY_STRING, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_both_deviceKey_and_deviceSasToken_defined_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };

    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_empty_iothub_name_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_EMPTY_STRING, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_validConfig_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, NULL, NULL);

    setup_IoTHubTransport_MQTT_Common_Create_mocks(false, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean up
    IoTHubTransport_MQTT_Common_Destroy(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_callbacks_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, NULL, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, NULL, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean up
    IoTHubTransport_MQTT_Common_Destroy(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_NULL_protocol_gateway_hostname_Succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, NULL, NULL);

    setup_IoTHubTransport_MQTT_Common_Create_mocks(false, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME_NON_NULL, NULL);

    setup_IoTHubTransport_MQTT_Common_Create_mocks(true, NULL);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean up
    IoTHubTransport_MQTT_Common_Destroy(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_with_Module_Succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME_NON_NULL, TEST_MODULE_ID);

    setup_IoTHubTransport_MQTT_Common_Create_mocks(true, TEST_MODULE_ID);

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean up
    IoTHubTransport_MQTT_Common_Destroy(result);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Create_validConfig_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, NULL, TEST_MODULE_ID);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_Create_mocks(false, TEST_MODULE_ID);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_Create failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
            TRANSPORT_LL_HANDLE result = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

            // assert
            ASSERT_IS_NULL(result, tmp_msg);
        }     
    }

    // clean up
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_parameter_NULL_succeed)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_Destroy(NULL);

    // assert
}

static void IoTHubTransport_MQTT_Common_Destroy_Unsubscribe_impl(bool useModelId)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe(handle);
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };

    setup_initialize_connection_mocks(useModelId);
    IoTHubTransport_MQTT_Common_DoWork(handle);
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
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert

}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_Unsubscribe_succeeds)
{
    IoTHubTransport_MQTT_Common_Destroy_Unsubscribe_impl(false);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_Unsubscribe_with_modelID_succeeds)
{
    IoTHubTransport_MQTT_Common_Destroy_Unsubscribe_impl(true);
}


static void set_expected_calls_for_free_transport_handle_data()
{
    STRICT_EXPECTED_CALL(mqtt_client_deinit(TEST_MQTT_CLIENT_HANDLE)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));
    EXPECTED_CALL(STRING_delete(NULL));

    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));

    EXPECTED_CALL(gballoc_free(NULL));
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_One_Message_Ack_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST) );
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));

    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    setup_invoke_message_callback_mocks(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, false, true);

    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG)); // pending_get_twin_queue
    set_expected_calls_for_free_transport_handle_data();

    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_disconnect_timeout_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST) );
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    g_skip_disconnect_callback = true;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));

    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    for(int i = 0; i < TEST_MAX_DISCONNECT_VALUE; ++i)
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_NUM_ARG));
    }
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    setup_invoke_message_callback_mocks(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, false, true);
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG)); // pending_get_twin_queue
    set_expected_calls_for_free_transport_handle_data();

    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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


static void set_expected_calls_for_IoTHubTransport_MQTT_Common_Destroy()
{
    STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    set_expected_calls_for_free_transport_handle_data();
}

// Tests that if MQTT has entered a disconnected state and stays there over time, Destroy performs a proper cleanup
// https://github.com/Azure/azure-iot-sdk-c/issues/924 was hit in field where SDK previously wasn't performing this correctly.
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_from_disconnected_state)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe(handle);
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };

    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);


    // Signal an error to put us into the pending disconnect state.
    g_fnMqttErrorCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_NO_PING_RESPONSE, g_callbackCtx);
    // The initial call to DoWork() is required to process the disconnection and move us into the disconnected state.
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // A future Dowork call is required to re-allocate the xio.  As part of this, make the mqtt_client_connect fail.  
    // This has the effect of allocationg the xio but leaving us in a disconnected state.

    // Using the REGISTER_GLOBAL_MOCK_RETURN to force an error in a LONG list of calls there's not otherwise need
    // to STRICT_EXPECTED_CALL mocks on.
    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_connect, MU_FAILURE);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_connect, 0);

    umock_c_reset_all_calls();

    set_expected_calls_for_IoTHubTransport_MQTT_Common_Destroy();
   
    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_parameter_NULL_fail)
{
    // arrange

    // act
    int res = IoTHubTransport_MQTT_Common_Subscribe(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_Succeed)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    setup_IoTHubTransport_MQTT_Common_DoWork_mocks();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// This code path (subscribing with a module via IoTHubTransport_MQTT_Common_Subscribe) should never
// be invoked by the product in practice, as the IoTHub client layer when using modules doesn't call into it.
TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_With_Module_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe(handle);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);

}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();
    (void)IoTHubTransport_MQTT_Common_Subscribe(handle);

    setup_IoTHubTransport_MQTT_Common_DoWork_mocks();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            IoTHubTransport_MQTT_Common_DoWork(handle);
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_set_subscribe_type_Succeed)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    setup_IoTHubTransport_MQTT_Common_DoWork_mocks();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetRetryPolicy_parameter_NULL_fail)
{
    // arrange

    // act
    int res = IoTHubTransport_MQTT_Common_SetRetryPolicy(NULL, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetRetryPolicy_failure)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetRetryPolicy_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetRetryPolicy_change_policy_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_Succeed)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    set_expected_calls_around_unsubscribe();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_handle_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin_No_subscribe_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin_handle_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetTwinAsync_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetTwinAsync(handle, on_get_device_twin_completed_callback, (void*)0x4445);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetTwinAsync_NULL_handle_failure)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetTwinAsync(NULL, on_get_device_twin_completed_callback, (void*)0x4445);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetTwinAsync_NULL_completionCallback_failure)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetTwinAsync(handle, NULL, (void*)0x4445);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetTwinAsync_fails)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (!umock_c_negative_tests_can_call_fail(index))
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "Failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetTwinAsync(handle, on_get_device_twin_completed_callback, (void*)0x4445);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_return_pending_get_twin_requests_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    (void)IoTHubTransport_MQTT_Common_GetTwinAsync(handle, on_get_device_twin_completed_callback, (void*)0x4445);

    get_twin_size = 1234;
    get_twin_payLoad = (const unsigned char*)0x4567;

    umock_c_reset_all_calls();

    EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    EXPECTED_CALL(xio_destroy(NULL));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG));

    set_expected_calls_for_free_transport_handle_data();

    // act
    IoTHubTransport_MQTT_Common_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void*)0x4445, get_twin_userContextCallback);
    ASSERT_IS_NULL(get_twin_payLoad);
    ASSERT_ARE_EQUAL(int, 0, get_twin_size);

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_invokes_xio_setoption_when_option_not_consumed_by_mqtt_transport)
{
    // arrange
    const char* SOME_OPTION = "AnOption";
    const void* SOME_VALUE = (void*)42;

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_option_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_value_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_LOG_TRACE, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_x509Certificate_no_509_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_keepAlive_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_keepAlive_previous_connection_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_DoWork(handle);

    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    for (size_t index = 0; index < NUM_DOWORK_VALUE; index++)
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_NUM_ARG));
    }
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));

    int keepAlive = 10;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_KEEP_ALIVE, &keepAlive);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_xio_create_fail)
{
    // arrange
    const char* SOME_OPTION = "AnOption";
    const void* SOME_VALUE = (void*)42;

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport_fail, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_fails_when_xio_setoption_fails)
{
    // arrange
    const char* SOME_OPTION = "AnOption";
    const void* SOME_VALUE = (void*)42;

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, SOME_OPTION, SOME_VALUE))
        .IgnoreArgument(1)
        .SetReturn(MU_FAILURE);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, SOME_OPTION, SOME_VALUE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}


TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_keepAlive_same_value_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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

TEST_FUNCTION(SetOption_with_proxy_data_copies_the_options_for_later_use)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(SetOption_proxy_data_with_NULL_host_address_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(SetOption_proxy_data_with_NULL_option_value_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", NULL);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_and_password_saves_only_the_hostname)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_but_non_NULL_password_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(SetOption_proxy_data_with_NULL_password_but_non_NULL_username_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(SetOption_proxy_data_frees_previously_Saved_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    set_expected_calls_for_proxy_default_copy();

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

TEST_FUNCTION(when_allocating_proxy_name_fails_SetOption_proxy_data_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(when_allocating_username_fails_SetOption_proxy_data_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(when_allocating_password_fails_SetOption_proxy_data_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(when_allocating_proxy_name_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    set_expected_calls_for_proxy_default_copy();
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

TEST_FUNCTION(when_allocating_username_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    set_expected_calls_for_proxy_default_copy();
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

TEST_FUNCTION(when_allocating_password_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    set_expected_calls_for_proxy_default_copy();
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

TEST_FUNCTION(SetOption_proxy_data_with_NULL_hostname_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    set_expected_calls_for_proxy_default_copy();
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

TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_and_non_NULL_password_does_not_free_previous_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    set_expected_calls_for_proxy_default_copy();
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

TEST_FUNCTION(SetOption_xio_option_get_underlying_TLS_when_proxy_data_was_not_set_passes_down_NULL)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(SetOption_xio_option_get_underlying_TLS_when_proxy_data_was_set_passes_down_the_proxy_options)
{
    // arrange
    MQTT_TRANSPORT_PROXY_OPTIONS mqtt_transport_proxy_options;
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    set_expected_calls_for_proxy_default_copy();
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

TEST_FUNCTION(SetOption_proxy_data_when_underlying_IO_is_already_created_fails)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Destroy_frees_the_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    set_expected_calls_for_proxy_default_copy();

    (void)IoTHubTransport_MQTT_Common_SetOption(handle, "proxy_data", &http_proxy_options);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mqtt_client_deinit(IGNORED_PTR_ARG))
        .IgnoreAllCalls();
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG)).IgnoreAllCalls();
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_retry_interval_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(retry_control_set_option(IGNORED_PTR_ARG, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, IGNORED_PTR_ARG));

    // act
    int retry_interval = 10;
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_RETRY_INTERVAL_SEC, &retry_interval);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_retry_max_delay_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(retry_control_set_option(IGNORED_PTR_ARG, RETRY_CONTROL_OPTION_MAX_DELAY_IN_SECS, IGNORED_PTR_ARG));

    // act
    int retry_interval = 10;
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_RETRY_MAX_DELAY_SECS, &retry_interval);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SetOption_retry_interval_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(retry_control_set_option(IGNORED_PTR_ARG, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, IGNORED_PTR_ARG)).SetReturn(__LINE__);

    // act
    int retry_interval = 10;
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_RETRY_INTERVAL_SEC, &retry_interval);

    // assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}


TEST_FUNCTION(IoTHubTransport_MQTT_Common_mqtt_operation_complete_msgInfo_NULL_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_parameter_handle_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_DoWork(NULL);
    // assert

}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_all_parameters_NULL_fail)
{
    // arrange

    // act
    IoTHubTransport_MQTT_Common_DoWork(NULL);
    // assert

}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_mqtt_client_connect_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks(false);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_First_connect_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks(false);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

static void set_expected_calls_for_first_gettwin_dowork()
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    // STRING_construct_sprintf
    STRICT_EXPECTED_CALL(mqttmessage_create(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_MOST_ONCE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_publish(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqttmessage_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_send_get_twin_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 0;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // Request the Twin content
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    (void)IoTHubTransport_MQTT_Common_GetTwinAsync(handle, on_get_device_twin_completed_callback, (void*)0x4445);

    // Send the GET request
    umock_c_reset_all_calls();
    set_expected_calls_for_first_gettwin_dowork();
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // Receive the response with Twin document.
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE))
        .SetReturn(TEST_MQTT_MSG_TOPIC_GET_TWIN);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create_from_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("res");
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("200");
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("?$rid=2");
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_TWIN_UPDATE_COMPLETE, get_twin_update_state);
    ASSERT_ARE_EQUAL(void_ptr, appMessage, get_twin_payLoad, "Incorrect payload"); // Not a json, but good enough for testing.
    ASSERT_ARE_EQUAL(int, appMsgSize, get_twin_size, "Incorrect message size");
    ASSERT_ARE_EQUAL(void_ptr, (void*)0x4445, get_twin_userContextCallback, "Incorrect user context");

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// Initial, common calls for a run through DoWork that is going to destined to have timeouts
static void set_expected_calls_for_DoWork_for_twin_timeouts()
{
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_send_get_twin_timesout)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // Request the Twin content
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    (void)IoTHubTransport_MQTT_Common_GetTwinAsync(handle, on_get_device_twin_completed_callback, (void*)0x4445);

    // Send the GET request
    umock_c_reset_all_calls();
    set_expected_calls_for_first_gettwin_dowork();
    IoTHubTransport_MQTT_Common_DoWork(handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork_for_twin_timeouts();

    // Set up the returned time such that it's far enough into the future to trigger time out related logic.
    g_current_ms += 6*60*1000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));

    // The initial message removed is the implicit GetTwin() created on a listen for twin subscription.
    // This is not reported back to the application, by convention.
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_ARE_EQUAL(int, DEVICE_TWIN_UPDATE_COMPLETE, get_twin_update_state);
    ASSERT_IS_NULL(get_twin_payLoad); // Not a json, but good enough for testing.
    ASSERT_ARE_EQUAL(int, 0, get_twin_size, "Incorrect message size");
    ASSERT_ARE_EQUAL(void_ptr, (void*)0x4445, get_twin_userContextCallback, "Incorrect user context");

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}


TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_First_connect_succeed_calls_retry_control_reset)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };

    umock_c_reset_all_calls();
    setup_initialize_connection_mocks(false);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    setup_connection_success_mocks();

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);
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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    CONNECT_ACK connack;
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_SERVER_UNAVAIL;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_Connection_Break_Wait_2_Times_Success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = {0};
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    umock_c_reset_all_calls();
    /*First Do_Work*/
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_LATER;
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    /*Second Do_Work*/
    EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    /* Attempt to connect again*/
    setup_initialize_reconnection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    /* Break Connection */
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);
    /* Retry connecting */
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_Retry_Policy_Connection_Break_Reconnection_Times_Out)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    /* Break Connection */
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);

    umock_c_reset_all_calls();

    /*Do_Work Festival*/
    size_t counter;
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_LATER;

    // setup_initialize_connection_mocks(false);
    for (counter = 0; counter < 100; counter++)
    {
        if (counter == 30) // This is a random number....
        {
            retry_action = RETRY_ACTION_STOP_RETRYING;
        }

        EXPECTED_CALL(retry_control_should_retry(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));

        if (counter == 30)
        {
            STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED, IGNORED_PTR_ARG));
        }

        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        // removeExpiredTwinRequests
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        IoTHubTransport_MQTT_Common_DoWork(handle);
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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);
    IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);
    CONNECT_ACK connack;
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_UNKNOWN;
    /* Break Connection */
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);
    umock_c_reset_all_calls();

    /*First Do_Work*/
    setup_initialize_reconnection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    // process_queued_ack_messages
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    /*Second Do_Work*/
    /* Attempt to connect again*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    // process_queued_ack_messages
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    ///* Fail connection */
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    // process_queued_ack_messages
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    ///* Second Retry */
    //// setup retry-setup mocks
    ///* Attempt to connect again*/
    setup_initialize_reconnection_mocks();
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    /* Retry connecting */
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    /* Connect fail */
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    /* Disconnecting */
    IoTHubTransport_MQTT_Common_DoWork(handle);
    /*Second Retry connecting */
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_no_messages_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks(false);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

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

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks(false);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            IoTHubTransport_MQTT_Common_DoWork(handle);

            //assert
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_SAS_token_from_user_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks(false);

    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_x509_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, NULL, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks(false);
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_BYTEARRAY, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_get_item_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(true);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetString(IGNORED_PTR_ARG)).SetReturn(NULL);

    EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_ERROR, transport_cb_ctx));
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_x509_no_expire_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    // Reset the calls to make sure we're mocket the following calls
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_X509);
    // DoWork to process the CONNACK call
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_X509);

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetString(IGNORED_PTR_ARG)).SetReturn(NULL);

    EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_ERROR, transport_cb_ctx));
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_emtpy_item_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_security_msg_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, false, false, NULL, NULL, NULL, "json/application", NULL, NULL, NULL, false, NULL, NULL, true);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_BYTEARRAY, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            IoTHubTransport_MQTT_Common_DoWork(handle);

            //assert
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_with_properties_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

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
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks((const char* const**)&keys, (const char* const**)&values, propCount, TEST_IOTHUB_MSG_BYTEARRAY, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_with_2_properties_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

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
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks((const char* const**)&keys, (const char* const**)&values, propCount, TEST_IOTHUB_MSG_BYTEARRAY, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_with_properties_succeeds_autoencode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

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
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks((const char* const**)&keys, (const char* const**)&values, propCount, TEST_IOTHUB_MSG_BYTEARRAY, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, true, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_with_2_properties_succeeds_autoencode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

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
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks((const char* const**)&keys, (const char* const**)&values, propCount, TEST_IOTHUB_MSG_BYTEARRAY, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, true, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_no_resend_message_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_resend_message_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    g_current_ms += 5*60*1000;
    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/*  This test covers the scenario where there are multiple disconnects and
    reconnects in a row and the packet is resent each time without expiring by count.
*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_resend_multiple_disconnects)
{
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    SUBSCRIBE_ACK suback;
    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    IOTHUB_MESSAGE_LIST message1;
    TRANSPORT_LL_HANDLE handle;

    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // create packet & add to sending queue
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message1.entry));

    // Send packet 1st time
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // Disconnect
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);

    // Reconnect
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    // Dowork where packet is resent 1st time
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // Disconnect
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);
    // setup is now complete
    umock_c_reset_all_calls();

    // Reconnect
    setup_connection_success_mocks();

    // DoWork where packet is resent 2nd time (wouldn't happen without unlimited resend on reconnect)
    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, true, false, true, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_message_timeout_succeeds)
{
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    SUBSCRIBE_ACK suback;
    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    IOTHUB_MESSAGE_LIST message2;
    TRANSPORT_LL_HANDLE handle;

    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    IoTHubTransport_MQTT_Common_DoWork(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle);

    for (int i = 1; i < (MSG_TIMEOUT_VALUE_MS) / (RESEND_TIMEOUT_VALUE_MS); i++)
    {
        g_current_ms += RESEND_TIMEOUT_VALUE_MS;
        IoTHubTransport_MQTT_Common_DoWork(handle);
    }
    umock_c_reset_all_calls();

    g_current_ms += RESEND_TIMEOUT_VALUE_MS;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, transport_cb_ctx));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    for (size_t index = 0; index < NUM_DOWORK_VALUE; index++)
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_NUM_ARG));
    }
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, transport_cb_ctx));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message1.entry));

    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    IoTHubTransport_MQTT_Common_DoWork(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle);

    for (int i = 1; i < (MSG_TIMEOUT_VALUE_MS) / (RESEND_TIMEOUT_VALUE_MS); i++)
    {
        g_current_ms += RESEND_TIMEOUT_VALUE_MS;
        IoTHubTransport_MQTT_Common_DoWork(handle);
    }

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));

    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

    g_current_ms += RESEND_TIMEOUT_VALUE_MS;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));

    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, transport_cb_ctx));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    for (size_t index = 0; index < NUM_DOWORK_VALUE; index++)
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_NUM_ARG));
    }
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, transport_cb_ctx));

    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, transport_cb_ctx));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, transport_cb_ctx));

    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_device_twin_resend_message_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // Disconnect due to sas reconnect
    g_current_ms += 3600*1000;
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    umock_c_reset_all_calls();

    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    STRICT_EXPECTED_CALL(retry_control_should_retry(TEST_RETRY_CONTROL_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(retry_action));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_1_event_item_STRING_type_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, false, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

// As of now, this test covers: message id, correlation id, content type, content encoding, diagId, diagCreationTimeUtc.
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_message_system_properties_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, false, false,
        "msg_id", "core_id", TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING, TEST_DIAG_ID, TEST_DIAG_CREATION_TIME_UTC, TEST_MESSAGE_CREATION_TIME_UTC, false, TEST_OUTPUT_NAME, TEST_COMPONENT_NAME, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_message_system_properties_succeeds_autoencode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, false, false,
        "msg_id", "core_id", TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING, TEST_DIAG_ID, TEST_DIAG_CREATION_TIME_UTC, TEST_MESSAGE_CREATION_TIME_UTC, true, NULL, TEST_COMPONENT_NAME, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

/*  This test covers the scenario where upon reconnect, there are both packets
    in the waiting to send queue and the waiting for ack queue. It ensures
    packets are sent in order after a disconnect.
*/
TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_reconnect_send_order)
{
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    SUBSCRIBE_ACK suback;
    PUBLISH_ACK puback;
    puback.packetId = 2;
    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    IOTHUB_MESSAGE_LIST message1;
    IOTHUB_MESSAGE_LIST message2;
    IOTHUB_MESSAGE_LIST message3;
    TRANSPORT_LL_HANDLE handle;

    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // create first 2 packets
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message1.entry));

    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message2.entry));


    // Send first 2 packets
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // receive first puback
    setup_invoke_message_callback_mocks(IOTHUB_CLIENT_CONFIRMATION_OK, true, false);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_PUBLISH_ACK, &puback, g_callbackCtx);

    // add 1 more packets to waiting to send queue (so we have one packet in waiting to send and one in waiting to ack)
    memset(&message3, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message3.messageHandle = TEST_IOTHUB_MSG_STRING;
    DList_InsertTailList(config.waitingToSend, &(message3.entry));

    // Disconnect
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_DISCONNECT, NULL, g_callbackCtx);
    // setup is now complete
    umock_c_reset_all_calls();

    // Reconnect
    setup_connection_success_mocks();

    // DoWork where packet 2 is resent
    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, true, true, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // DoWork where packet 3 is sent for the first time
    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, true, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, false);

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_with_message_incomplete_diagnostic_system_properties_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    setup_initialize_connection_mocks(true);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_IoTHubTransport_MQTT_Common_DoWork_events_mocks(NULL, NULL, 0, TEST_IOTHUB_MSG_STRING, false, false, false,
        NULL, NULL, NULL, NULL, TEST_DIAG_ID, NULL, NULL, false, NULL, NULL, false);

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_resend_max_recount_reached_message_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message2;
    memset(&message2, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message2.messageHandle = TEST_IOTHUB_MSG_STRING;

    DList_InsertTailList(config.waitingToSend, &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
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
        IoTHubTransport_MQTT_Common_DoWork(handle);
        g_current_ms = (tickcounter_ms_t)(index * 15 * 1000);
    }
    umock_c_reset_all_calls();

    set_expected_calls_for_DoWork_for_twin_timeouts();
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setup_initialize_connection_mocks(false);

    for (size_t index = 0; index < iterationCount-1; index++)
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        // removeExpiredTwinRequests
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    for (size_t index = 0; index < iterationCount; index++)
    {
        IoTHubTransport_MQTT_Common_DoWork(handle);
    }

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_mqtt_client_connect_times_out)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };

    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    g_current_ms += (5 * 600 * 2000); // 4+ minutes have passed.
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    for (size_t index = 0; index < NUM_DOWORK_VALUE; index++)
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_NUM_ARG));
    }
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DoWork_mqtt_client_connecting_times_out)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };

    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();
    setup_initialize_connection_mocks(false);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    g_current_ms += (5 * 600 * 2000); // 4+ minutes have passed.
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //
    // client is not connected now, so mqtt_client_dowork shouldn't be called
    //

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetSendStatus_InvalidHandleArgument_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetSendStatus_InvalidStatusArgument_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_GetSendStatus(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_INVALID_ARG);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetSendStatus_empty_waitingToSend_and_empty_waitingforAck_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetSendStatus_waitingToSend_not_empty_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    g_fnMqttErrorCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_NO_PING_RESPONSE, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_delivered_MQTT_CLIENT_MQTT_CLIENT_MEMORY_ERROR_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(xio_retrieveoptions(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_clear_xio(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    // removeExpiredTwinRequests
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    g_fnMqttErrorCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_MEMORY_ERROR, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_delivered_MQTT_CLIENT_NO_NETWORK_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_NETWORK, IGNORED_PTR_ARG));

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, IGNORED_PTR_ARG));

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    CONNECT_ACK connack;
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_BAD_USERNAME_PASSWORD;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL, IGNORED_PTR_ARG));

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    CONNECT_ACK connack;
    connack.isSessionPresent = false;
    connack.returnCode = CONN_REFUSED_NOT_AUTHORIZED;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL, IGNORED_PTR_ARG));

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
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

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
    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_invoke_message_callback_mocks(IOTHUB_CLIENT_CONFIRMATION_OK, true, false);

    // act
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_PUBLISH_ACK, &puback, g_callbackCtx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_message_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(NULL, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_context_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_Message_context_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_recv_msg_callback_mocks(true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_twin_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 2;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    CONSTBUFFER cbuff;
    cbuff.buffer = appMessage;
    cbuff.size = appMsgSize;
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&cbuff);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = TEST_CONST_BUFFER_HANDLE;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    (void)IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);

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

static void test_invalid_mqtt_twin_topic_setup(const char* mqtt_topic, const char* status_code, const char* response_id)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 2;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    CONSTBUFFER cbuff;
    cbuff.buffer = appMessage;
    cbuff.size = appMsgSize;
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&cbuff);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = TEST_CONST_BUFFER_HANDLE;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    (void)IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);

    umock_c_reset_all_calls();

    g_tokenizerIndex = 1;

    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(mqtt_topic);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create_from_char(IGNORED_PTR_ARG)).IgnoreArgument_input();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn("res");

    if (status_code != NULL)
    {
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(status_code);

        if (response_id != NULL)
        {
            STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(0);
            STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(response_id);
        }
        else
        {
            STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(1);
        }
    }
    else
    {
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(1);
    }

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG))
        .IgnoreArgument_t();

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_twin_missing_status_code_fails)
{
    test_invalid_mqtt_twin_topic_setup(TEST_MQTT_DEV_TWIN_MSG_TOPIC_INVALID_REQUEST_ID, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_twin_invalid_request_id_fails)
{
    test_invalid_mqtt_twin_topic_setup(TEST_MQTT_DEV_TWIN_MSG_TOPIC_INVALID_REQUEST_ID, "200", "?$NotSetRequestId=2");
}

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_twin_missing_request_id_fails)
{
    test_invalid_mqtt_twin_topic_setup(TEST_MQTT_DEV_TWIN_MSG_TOPIC_MISSING_REQUEST_ID, "200", NULL);
}

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_twin_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    CONSTBUFFER cbuff;
    cbuff.buffer = appMessage;
    cbuff.size = appMsgSize;
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&cbuff);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = TEST_CONST_BUFFER_HANDLE;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    (void)IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);
    umock_c_reset_all_calls();

    g_tokenizerIndex = 8;

    setup_message_recv_callback_device_twin_mocks("res");

    umock_c_negative_tests_snapshot();

    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "MessageRecv_device_twin failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);
            // assert
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

// Subscribe for a twin notification, which implicitly initiates a GetTwin, and have the request timeout.
TEST_FUNCTION(IoTHubTransportMqtt_implicit_gettwin_timeout)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 2;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    IoTHubTransport_MQTT_Common_DoWork(handle);    

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork_for_twin_timeouts();

    // Set up the returned time such that it's far enough into the future to trigger time out related logic.
    g_current_ms += 6*60*1000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));

    // The initial message removed is the implicit GetTwin() created on a listen for twin subscription.
    // This is not reported back to the application, by convention.
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}


// Test that if a Reported Property is timed out, then appropriate application callbacks are notified
TEST_FUNCTION(IoTHubTransportMqtt_reported_property_timeout)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 2;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    CONSTBUFFER cbuff;
    cbuff.buffer = appMessage;
    cbuff.size = appMsgSize;
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&cbuff);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = TEST_CONST_BUFFER_HANDLE;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    (void)IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork_for_twin_timeouts();

    // Set up the returned time such that it's far enough into the future to trigger time out related logic.
    g_current_ms += 6*60*1000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &g_current_ms, sizeof(g_current_ms));

    // The initial message removed is the implicit GetTwin() created on a listen for twin subscription.
    // This is not reported back to the application, by convention.
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // The next message was queued there by the UT's request to send a reported property.
    STRICT_EXPECTED_CALL(Transport_Twin_ReportedStateComplete_Callback(1, STATUS_CODE_TIMEOUT_VALUE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}


TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_sys_Properties_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_receive_initial_calls(TEST_MQTT_MSG_TOPIC_W_1_PROP, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(TEST_MQTT_MESSAGE_TOPIC);

    EXPECTED_CALL(STRING_TOKENIZER_create_from_char(TEST_MQTT_MSG_TOPIC_W_1_PROP));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("iothub-ack=Full");

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(1);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    setup_set_message_disposition_context();
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    g_messageDispositionDestroyFunction(g_messageDispositionContext);
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_sys_Properties_succeed_autodecode)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_receive_initial_calls(TEST_MQTT_MSG_TOPIC_W_1_PROP, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(TEST_MQTT_MESSAGE_TOPIC);

    EXPECTED_CALL(STRING_TOKENIZER_create_from_char(TEST_MQTT_MSG_TOPIC_W_1_PROP));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));

    // "%24.cid&%24.uid";
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("iothub-ack=Full");
    // iothub-ack-full is a "system" property but not mapped to IOTHUB_MESSAGE_HANDLE so it is silently ignored

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("propName=PropValue");

    set_expected_calls_for_custom_message_property(true);

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("DeviceInfo=smokeTest");

    set_expected_calls_for_custom_message_property(true);

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("%24.to=%2Fdevices%2FmyDeviceId%2Fmessages%2FdeviceBound&");
    // %24.to is also silently ignored

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("%24.cid=123");

    STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubMessage_SetCorrelationId(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("%24.uid=456");

    STRICT_EXPECTED_CALL(URL_DecodeString(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubMessage_SetMessageUserIdSystemProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&")).SetReturn(1);
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    setup_set_message_disposition_context();
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    g_messageDispositionDestroyFunction(g_messageDispositionContext);
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_Properties_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    g_tokenizerIndex = 4;
    setup_message_recv_with_properties_mocks(true, true, false, true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    g_messageDispositionDestroyFunction(g_messageDispositionContext);
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_no_Properties_no_ct_no_ce_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_recv_with_properties_mocks(false, false, false, true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    g_messageDispositionDestroyFunction(g_messageDispositionContext);
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_no_Properties_no_ct_no_ce_cb_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_recv_with_properties_mocks(false, false, false, false);

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    g_tokenizerIndex = 2;
    setup_message_recv_with_properties_mocks(true, true, true, true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    g_messageDispositionDestroyFunction(g_messageDispositionContext);
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_Properties_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_recv_with_properties_mocks(false, false, false, true);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            g_messageDispositionContext = NULL;
            g_messageDispositionDestroyFunction = NULL;
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            g_tokenizerIndex = 6;
            g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

            if (g_messageDispositionContext != NULL)
            {
                g_messageDispositionDestroyFunction(g_messageDispositionContext);
            }
        }
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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    bool urlencode = true;
    IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlencode);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_recv_with_properties_mocks(false, false, true, true);

    umock_c_negative_tests_snapshot();

    // act
     size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index)) 
        {
            g_messageDispositionContext = NULL;
            g_messageDispositionDestroyFunction = NULL;
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "g_fnMqttMsgRecv failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            g_tokenizerIndex = 6;
            g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

            if (g_messageDispositionContext != NULL)
            {
                g_messageDispositionDestroyFunction(g_messageDispositionContext);
            }
        }
    }

    // assert

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_messagecallback_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    IOTHUB_MESSAGE_LIST message1;
    memset(&message1, 0, sizeof(IOTHUB_MESSAGE_LIST));
    message1.messageHandle = TEST_IOTHUB_MSG_BYTEARRAY;

    DList_InsertTailList(config.waitingToSend, &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_recv_msg_callback_mocks(false);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_method_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_tokenizerIndex = 8;
    IoTHubTransport_MQTT_Common_DoWork(handle);
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

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_device_method_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    pfTransport_DeviceMethod_Complete_Callback old_method_complete_cb = transport_cb_info.method_complete_cb;
    transport_cb_info.method_complete_cb = my_Transport_DeviceMethod_Complete_Callback;
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    g_tokenizerIndex = 6;
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_recv_device_method_mocks();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index)) 
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

            if (g_method_handle_value != NULL)
            {
                IoTHubTransport_MQTT_Common_DeviceMethod_Response(handle, g_method_handle_value, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE);
                g_method_handle_value = NULL;
            }
        }
    }

    // assert

    //cleanup
    transport_cb_info.method_complete_cb = old_method_complete_cb;
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceKey_null_and_deviceSasToken_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = NULL;
    deviceConfig.deviceSasToken = NULL;
    deviceConfig.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceKey_provided_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = NULL;
    deviceConfig.deviceSasToken = TEST_DEVICE_SAS;
    deviceConfig.moduleId = NULL;
    //deviceConfig.authorization_module.cred_type = IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

static void set_expected_call_for_get_cred_and_devicekey()
{
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceKey(IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_succeeds_returns_transport)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    set_expected_call_for_get_cred_and_devicekey();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_ARE_EQUAL(void_ptr, handle, devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_twice_fails_second_time)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    set_expected_call_for_get_cred_and_devicekey();
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_KEY);

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_IS_NULL(devHandle2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceKey_mismatch_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = "not the right device key";
    deviceConfig.deviceSasToken = NULL;
    deviceConfig.moduleId = NULL;


    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    set_expected_call_for_get_cred_and_devicekey();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceid_mismatch_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = "not a good id after all";
    deviceConfig.deviceKey = TEST_DEVICE_KEY;
    deviceConfig.deviceSasToken = NULL;
    deviceConfig.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_wts_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, NULL);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_device_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, NULL, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceId_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = NULL;
    deviceConfig.deviceKey = TEST_DEVICE_KEY;
    deviceConfig.deviceSasToken = NULL;
    deviceConfig.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_deviceKey_and_deviceSasToken_both_provided_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = TEST_DEVICE_KEY;
    deviceConfig.deviceSasToken = TEST_DEVICE_SAS;
    deviceConfig.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend);

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
    SetupIothubTransportConfigWithKeyAndSasToken(&config, TEST_DEVICE_ID, NULL, TEST_DEVICE_SAS, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = NULL;
    deviceConfig.deviceSasToken = TEST_DEVICE_SAS;
    deviceConfig.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICE_ID);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);
    EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_transport_null_returns_null)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(NULL, &TEST_DEVICE_1, config.waitingToSend);

    // assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unregister_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    set_expected_call_for_get_cred_and_devicekey();

    // act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);

    IoTHubTransport_MQTT_Common_Unregister(devHandle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Register_Unregister_Register_returns_handle)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);
    IoTHubTransport_MQTT_Common_Unregister(devHandle);

    umock_c_reset_all_calls();

    set_expected_call_for_get_cred_and_devicekey();

    // act
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(devHandle2);
    ASSERT_ARE_EQUAL(void_ptr, handle, devHandle2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_GetHostname_with_non_NULL_handle_succeeds)
{
    //arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_Succeed)
{
    // arrange
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).IgnoreArgument_psz();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_DoWork_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    setup_subscribe_devicetwin_dowork_mocks();

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_DoWork_fails)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle);

    umock_c_reset_all_calls();

    setup_subscribe_devicetwin_dowork_mocks();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
            g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            IoTHubTransport_MQTT_Common_DoWork(handle);

            // assert
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_DoWork_2nd_fails)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle);

    umock_c_reset_all_calls();

     STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
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
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
            SUBSCRIBE_ACK suback;
            suback.packetId = 1234;
            suback.qosCount = 1;
            suback.qosReturn = QosValue;

            g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            IoTHubTransport_MQTT_Common_DoWork(handle);

            // assert
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_notify_Succeed)
{
    // arrange
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin_notify_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);
        // assert
        ASSERT_ARE_NOT_EQUAL(int, result, 0, tmp_msg);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod_Succeed)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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
        sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        int result = IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle);
        // assert
        ASSERT_ARE_NOT_EQUAL(int, result, 0);
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod_handle_OK)
{
    // arrange
    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    set_expected_calls_around_unsubscribe();

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
    set_expected_calls_around_unsubscribe();

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
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 2;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = TEST_CONST_BUFFER_HANDLE;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    umock_c_reset_all_calls();

    setup_processItem_mocks(false);

    // act
    IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_PROCESS_OK, result_item);

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_iothub_item_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    // act
    IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_PROCESS_ERROR, result_item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_not_connected_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    CONSTBUFFER cbuff;
    cbuff.buffer = appMessage;
    cbuff.size = appMsgSize;
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&cbuff);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = TEST_CONST_BUFFER_HANDLE;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    umock_c_reset_all_calls();

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_PROCESS_NOT_CONNECTED, result_item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_continue_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    QOS_VALUE QosValue[] ={ DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    TRANSPORT_LL_HANDLE handle = setup_iothub_mqtt_connection(&config);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    CONSTBUFFER data_buff;
    data_buff.buffer = (const unsigned char*)0x46;
    data_buff.size = 22;

    CONSTBUFFER cbuff;
    cbuff.buffer = appMessage;
    cbuff.size = appMsgSize;
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&cbuff);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = TEST_CONST_BUFFER_HANDLE;
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
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_ProcessItem_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
    SUBSCRIBE_ACK suback;
    suback.packetId = 1234;
    suback.qosCount = 1;
    suback.qosReturn = QosValue;

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_callbackCtx);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    CONSTBUFFER cbuff;
    cbuff.buffer = appMessage;
    cbuff.size = appMsgSize;
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&cbuff);
    IOTHUB_DEVICE_TWIN device_twin;
    device_twin.report_data_handle = TEST_CONST_BUFFER_HANDLE;
    device_twin.item_id = 1;
    IOTHUB_IDENTITY_INFO identity_info;
    identity_info.device_twin = &device_twin;
    umock_c_reset_all_calls();

    setup_processItem_mocks(true);
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(NULL)).IgnoreArgument_listEntry();

    STRICT_EXPECTED_CALL(gballoc_free(NULL)).IgnoreArgument_ptr();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_ProcessItem failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            IOTHUB_PROCESS_ITEM_RESULT result_item = IoTHubTransport_MQTT_Common_ProcessItem(handle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);
            // assert
            ASSERT_ARE_NOT_EQUAL(int, IOTHUB_PROCESS_OK, result_item, tmp_msg);
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DeviceMethod_Response_handle_NULL_fail)
{
    // arrange
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    int result = IoTHubTransport_MQTT_Common_DeviceMethod_Response(NULL, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DeviceMethod_Response_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DeviceMethod_Response_NULL_RESPONSE_succeeds)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_DeviceMethod_Response_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    setup_devicemethod_response_mocks();

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_reset_all_calls();
            g_tokenizerIndex = 8;
            setup_message_recv_device_method_mocks();
            g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            (void)sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DeviceMethod_Response failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            // act
            int result = IoTHubTransport_MQTT_Common_DeviceMethod_Response(handle, g_method_handle_value, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SendMessageDisposition_NULL_fails)
{
    //arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();
    set_expected_call_for_get_cred_and_devicekey();
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SendMessageDisposition(devHandle, NULL, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SendMessageDisposition_Message_NULL_fails)
{
    //arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();
    set_expected_call_for_get_cred_and_devicekey();
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);

    TEST_MESSAGE_DISPOSITION_CONTEXT* msgDispCtx = (TEST_MESSAGE_DISPOSITION_CONTEXT*) malloc(sizeof(TEST_MESSAGE_DISPOSITION_CONTEXT));;
    msgDispCtx->packet_id = 10;
    msgDispCtx->qos_value = DELIVER_AT_LEAST_ONCE;

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SendMessageDisposition(devHandle, NULL, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    free(msgDispCtx);
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_SendMessageDisposition_succeeds)
{
    //arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    IOTHUB_DEVICE_CONFIG TEST_DEVICE_1;
    TEST_DEVICE_1.deviceId = TEST_DEVICE_ID;
    TEST_DEVICE_1.deviceKey = TEST_DEVICE_KEY;
    TEST_DEVICE_1.deviceSasToken = NULL;
    TEST_DEVICE_1.moduleId = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();
    set_expected_call_for_get_cred_and_devicekey();
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransport_MQTT_Common_Register(handle, &TEST_DEVICE_1, config.waitingToSend);

    TEST_MESSAGE_DISPOSITION_CONTEXT msgDispCtx;
    msgDispCtx.packet_id = 10;
    msgDispCtx.qos_value = DELIVER_AT_LEAST_ONCE;

    MESSAGE_DISPOSITION_CONTEXT_HANDLE msgDispCtxHandle = (MESSAGE_DISPOSITION_CONTEXT_HANDLE)&msgDispCtx;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_dispositionContext(&msgDispCtxHandle, sizeof(&msgDispCtxHandle));
    STRICT_EXPECTED_CALL(mqtt_client_send_message_response(IGNORED_PTR_ARG, IGNORED_NUM_ARG, DELIVER_AT_LEAST_ONCE));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_MQTT_Common_SendMessageDisposition(devHandle, (IOTHUB_MESSAGE_HANDLE)0x42, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_InputQueue_Succeed)
{
    // arrange
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_STRING_VALUE);

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_InputQueue(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_InputQueue_Message_NULL_fails)
{
    // arrange
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_InputQueue(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_InputQueue_ModuleId_Null_fails)
{
    // arrange
    CONNECT_ACK connack ={ true, CONNECTION_ACCEPTED };
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);
    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_Common_Subscribe_InputQueue(handle);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_Subscribe_InputQueue(handle);
    umock_c_reset_all_calls();

    set_expected_calls_around_unsubscribe();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue_Handle_Null_fails)
{
    // arrange
    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue_No_InputQueue_fails)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_InputQueue_DoWork_Succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_Subscribe_InputQueue(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle);

    CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
    g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

    umock_c_reset_all_calls();

    setup_subscribe_inputqueue_dowork_mocks();

    // act
    IoTHubTransport_MQTT_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_Subscribe_InputQueue_DoWork_fails)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    (void)IoTHubTransport_MQTT_Common_Subscribe_InputQueue(handle);

    IoTHubTransport_MQTT_Common_DoWork(handle);

    umock_c_reset_all_calls();

    setup_subscribe_inputqueue_dowork_mocks();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
            g_fnMqttOperationCallback(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_callbackCtx);

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubTransport_MQTT_Common_DoWork failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            IoTHubTransport_MQTT_Common_DoWork(handle);

            // assert
        }
    }

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}


static void setup_message_recv_extractMqttProperties(const char* topicName, const char* inputQueueSubscribeName, const char* inputQueueName, bool connectedSystemProps)
{
    (void)inputQueueName;
    // findMessagePropertyStart
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument(1).SetReturn(inputQueueSubscribeName).CallCannotFail();

    // addInputNamePropertyToMsg
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetInputName(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(NULL)).IgnoreArgument_ptr();

    EXPECTED_CALL(STRING_TOKENIZER_create_from_char(topicName));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MSG_BYTEARRAY));

    if (connectedSystemProps)
    {
        // Parse out connected_device
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
            .IgnoreArgument(3)
            .CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn("%24.cdid=connected_device").
            CallCannotFail();

        STRICT_EXPECTED_CALL(IoTHubMessage_SetConnectionDeviceId(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        // Parse out connected_module
        STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
            .IgnoreArgument(3)
            .SetReturn(0);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn("%24.cmid=connected_module/")
            .CallCannotFail();

        STRICT_EXPECTED_CALL(IoTHubMessage_SetConnectionModuleId(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, "&"))
        .IgnoreArgument(3)
        .SetReturn(1);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
}

static void setup_message_recv_with_input_queue_mocks(
    const char* topicName, 
    const char* inputQueueSubscribeName, 
    const char* inputQueueName, 
    bool connectedSystemProps, 
    bool msgCbResult)
{
    STRICT_EXPECTED_CALL(mqttmessage_getTopicName(TEST_MQTT_MESSAGE_HANDLE)).SetReturn(topicName);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn(NULL)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn(inputQueueSubscribeName)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(TEST_MQTT_MESSAGE_HANDLE)).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(appMessage, appMsgSize));

    // Retrieve the input queue name
    setup_message_recv_extractMqttProperties(topicName, inputQueueSubscribeName, inputQueueName, connectedSystemProps);

    setup_set_message_disposition_context();
    STRICT_EXPECTED_CALL(Transport_MessageCallbackFromInput(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(msgCbResult);

    if (!msgCbResult)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_InputQueue_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    g_tokenizerIndex = 4;
    setup_message_recv_with_input_queue_mocks(TEST_MQTT_INPUT_1, TEST_MQTT_INPUT_QUEUE_SUBSCRIBE_NAME_1, TEST_INPUT_QUEUE_1, true, true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_InputQueue_no_system_properties_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_receive_initial_calls(TEST_MQTT_INPUT_NO_PROPERTIES, true);
    // We only have an input queue but no properties.  In this case far fewer calls will be invoked than typical case with properties.
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetInputName(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(IOTHUB_MESSAGE_OK);
    STRICT_EXPECTED_CALL(gballoc_free(NULL)).IgnoreArgument_ptr();

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create_from_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mqttmessage_getPacketId(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mqttmessage_getQosType(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_MessageCallbackFromInput(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(true);

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    g_messageDispositionDestroyFunction(g_messageDispositionContext);
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_InputQueue_missing_queue_name_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_receive_initial_calls(TEST_MQTT_INPUT_MISSING_INPUT_QUEUE_NAME, true);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail().SetReturn(TEST_MQTT_INPUT_MISSING_INPUT_QUEUE_NAME);
    // Because the MQTT topic isn't formatted correctly and we detect this early, don't parse through it.

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_MessageRecv_with_InputQueue_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    setup_message_recv_with_input_queue_mocks(TEST_MQTT_INPUT_1, TEST_MQTT_INPUT_QUEUE_SUBSCRIBE_NAME_1, TEST_INPUT_QUEUE_1, true, false);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            g_messageDispositionContext = NULL;
            g_messageDispositionDestroyFunction = NULL;
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

            if (g_messageDispositionContext != NULL)
            {
                g_messageDispositionDestroyFunction(g_messageDispositionContext);
            }
        }
    }

    // assert

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransport_MQTT_SetCallbackContext_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_SetCallbackContext(handle, transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_SetCallbackContext_fail)
{
    // arrange

    // act
    int result = IoTHubTransport_MQTT_SetCallbackContext(NULL, transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_GetSupportedPlatformInfo_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    PLATFORM_INFO_OPTION info;

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_GetSupportedPlatformInfo(handle, &info);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, info, PLATFORM_INFO_OPTION_RETRIEVE_SQM);

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransport_MQTT_GetSupportedPlatformInfo_NULL_handle)
{
    // arrange
    PLATFORM_INFO_OPTION info;

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_GetSupportedPlatformInfo(NULL, &info);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_GetSupportedPlatformInfo_NULL_info)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_MQTT_GetSupportedPlatformInfo(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

END_TEST_SUITE(iothubtransport_mqtt_common_ut)
