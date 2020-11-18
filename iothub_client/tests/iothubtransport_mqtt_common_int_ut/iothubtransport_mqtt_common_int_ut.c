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
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"


#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/constbuffer.h"

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/tlsio.h"

#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "iothub_message.h"
#include "iothub_client_options.h"

#define ENABLE_MOCKS
#include "azure_umqtt_c/mqtt_client.h"
#include "internal/iothub_client_private.h"
#include "internal/iothub_client_retry_control.h"
#include "internal/iothub_transport_ll_private.h"

MOCKABLE_FUNCTION(, bool, Transport_MessageCallbackFromInput, MESSAGE_CALLBACK_INFO*, messageData, void*, ctx);
MOCKABLE_FUNCTION(, bool, Transport_MessageCallback, MESSAGE_CALLBACK_INFO*, messageData, void*, ctx);
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

static const char* TEST_DEVICE_ID = "jebrandoDevice";
static const char* TEST_MODULE_ID = "thisIsModuleID";
static const char* TEST_DEVICE_KEY = "thisIsDeviceKey";
static const char* TEST_IOTHUB_NAME = "thisIsIotHubName";
static const char* TEST_IOTHUB_SUFFIX = "thisIsIotHubSuffix";
static const char* TEST_PROTOCOL_GATEWAY_HOSTNAME = NULL;
static const char* TEST_MQTT_MESSAGE_TOPIC = "devices/jebrandoDevice/messages/devicebound/#";
static const char* TEST_MQTT_MSG_TOPIC = "devices/jebrandoDevice/messages/devicebound/iothub-ack=Full&%24.to=%2Fdevices%2FjebrandoDevice%2Fmessages%2FdeviceBound&%24.cid=123&%24.uid=456";
static const char* TEST_MQTT_MSG_TOPIC_W_1_PROP = "devices/jebrandoDevice/messages/devicebound/iothub-ack=Full&propName=PropValue&DeviceInfo=smokeTest&%24.to=%2Fdevices%2FjebrandoDevice%2Fmessages%2FdeviceBound&%24.cid&%24.uid";
static const char* TEST_MQTT_INPUT_QUEUE_SUBSCRIBE_NAME_1 = "devices/thisIsDeviceID/modules/thisIsModuleID/#";
static const char* TEST_MQTT_INPUT_1 = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs/input1/%24.cdid=connected_device&%24.cmid=connected_module/";
static const char* TEST_MQTT_INPUT_NO_PROPERTIES = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs/input1/";
static const char* TEST_MQTT_INPUT_MISSING_INPUT_QUEUE_NAME = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs";
static const char* TEST_INPUT_QUEUE_1 = "input1";

static const char* TEST_SAS_TOKEN = "Test_SAS_Token_value";

static const char* TEST_CONTENT_TYPE = "application/json";
static const char* TEST_CONTENT_ENCODING = "utf8";
static const char* TEST_DIAG_ID = "1234abcd";
static const char* TEST_DIAG_CREATION_TIME_UTC = "1506054516.100";
static const char* TEST_MESSAGE_CREATION_TIME_UTC = "2010-01-01T01:00:00.000Z";
static const char* TEST_OUTPUT_NAME = "TestOutputName";

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

static IOTHUB_CLIENT_CONFIG g_iothubClientConfig = { 0 };

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

static DLIST_ENTRY g_waitingToSend;



//Callbacks for Testing
static ON_MQTT_MESSAGE_RECV_CALLBACK g_fnMqttMsgRecv;
static ON_MQTT_OPERATION_CALLBACK g_fnMqttOperationCallback;
static ON_MQTT_ERROR_CALLBACK g_fnMqttErrorCallback;
static void* g_callbackCtx;
static void* g_errorcallbackCtx;
static bool g_nullMapVariable;
static ON_MQTT_DISCONNECTED_CALLBACK g_disconnect_callback;
static void* g_disconnect_callback_ctx;
static TRANSPORT_CALLBACKS_INFO transport_cb_info;
static void* transport_cb_ctx = (void*)0x499922;

#ifdef __cplusplus
extern "C"
{
#endif

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

static int my_Transport_DeviceMethod_Complete_Callback(const char* method_name, const unsigned char* payLoad, size_t size, METHOD_HANDLE response_id, void* ctx)
{
    (void)ctx;
    (void)method_name;
    (void)payLoad;
    (void)size;
    (void)response_id;
    return 0;
}

static bool my_Transport_MessageCallback(MESSAGE_CALLBACK_INFO* message_data, void* ctx)
{
    (void)ctx;

    IoTHubMessage_Destroy(message_data->messageHandle);
    free(message_data);

    return true;
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

static XIO_HANDLE get_IO_transport(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options)
{
    (void)fully_qualified_name;
    (void)mqtt_transport_proxy_options;
    return (XIO_HANDLE)my_gballoc_malloc(1);
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

BEGIN_TEST_SUITE(iothubtransport_mqtt_common_int_ut)

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
    REGISTER_UMOCK_ALIAS_TYPE(MQTT_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_OPERATION_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_ERROR_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_CLOSE_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(QOS_VALUE, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(MQTT_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_MESSAGE_RECV_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, int);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CREDENTIAL_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_DISCONNECTED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_CONTENT_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_TWIN_UPDATE_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(RETRY_CONTROL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(RETRY_ACTION, int);

    REGISTER_GLOBAL_MOCK_HOOK(Transport_GetOption_Product_Info_Callback, my_Transport_GetOption_Product_Info_Callback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_GetOption_Product_Info_Callback, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Transport_GetOption_Model_Id_Callback, my_Transport_GetOption_Model_Id_Callback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_GetOption_Model_Id_Callback, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHub_Transport_ValidateCallbacks, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHub_Transport_ValidateCallbacks, __LINE__);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceKey, TEST_DEVICE_KEY);

    REGISTER_GLOBAL_MOCK_HOOK(Transport_MessageCallback, my_Transport_MessageCallback);

    REGISTER_GLOBAL_MOCK_HOOK(Transport_DeviceMethod_Complete_Callback, my_Transport_DeviceMethod_Complete_Callback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_DeviceMethod_Complete_Callback, MU_FAILURE);

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

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_create, TEST_MQTT_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_create_in_place, TEST_MQTT_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_create_in_place, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_getApplicationMsg, &TEST_APP_PAYLOAD);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_getApplicationMsg, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_getTopicName, TEST_MQTT_MSG_TOPIC);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_getTopicName, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_SasToken, my_IoTHubClient_Auth_Get_SasToken);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_SasToken, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Is_SasToken_Valid, SAS_TOKEN_STATUS_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Is_SasToken_Valid, SAS_TOKEN_STATUS_FAILED);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_SasToken_Expiry, 3600);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_create, TEST_RETRY_CONTROL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_should_retry, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_should_retry, 1);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_set_option, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_set_option, 1);
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

    g_nullMapVariable = true;

    expected_MQTT_TRANSPORT_PROXY_OPTIONS = NULL;
    g_disconnect_callback = NULL;
    g_disconnect_callback_ctx = NULL;
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    TEST_MUTEX_ACQUIRE(test_serialize_mutex);

    reset_test_data();
    DList_InitializeListHead(&g_waitingToSend);

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
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

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_sys_Properties_succeed)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config ={ 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);
    umock_c_reset_all_calls();

    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

END_TEST_SUITE(iothubtransport_mqtt_common_int_ut)
