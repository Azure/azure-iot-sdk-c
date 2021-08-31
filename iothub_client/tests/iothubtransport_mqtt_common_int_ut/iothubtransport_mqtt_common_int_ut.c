// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Note that most of the tests for the IoTHub core / MQTT layer are in ../iothubtransport_mqtt_common_ut.
// These test all the scenarios of the MQTT layer, with a specific focus on the mockable framework
// and that calls happen as expected, varous error cases are processed correctly, etc.  iothubtransport_mqtt_common_ut
// tends to focus on verifying the code in isolated parts.
// 
// This tests a subset of MQTT with a stronger focus on integration of multiple parts of the SDK.  Mocking is 
// much less heavily relied on in this test suite.  The focus instead is on more traditional test strategy of providing
// input (frequently an MQTT TOPIC queue for the SDK to parse) and then comparing the output.

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

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

// While these header files are not directly used by this .c itself and this feels like an over-include,
// this is needed.  We need to include these before the ENABLE_MOCKS is defined because otherwise headers
// in ENABLE_MOCKS block will include them and they will then be mocked themselves.  Which we don't
// want as this test only uses mocks to setup callback and wants to use real c-utility otherwise.
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/tickcounter.h"

#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/urlencode.h"
#include "iothub_message.h"
#include "iothub_client_options.h"

#define ENABLE_MOCKS
#include "azure_umqtt_c/mqtt_client.h"
#include "azure_c_shared_utility/xio.h"
#include "internal/iothub_client_private.h"
#include "internal/iothub_client_retry_control.h"
#include "internal/iothub_transport_ll_private.h"

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

static const char* TEST_DEVICE_ID = "myDeviceId";
static const char* TEST_MODULE_ID = "thisIsModuleID";
static const char* TEST_DEVICE_KEY = "thisIsDeviceKey";
static const char* TEST_IOTHUB_NAME = "thisIsIotHubName";
static const char* TEST_IOTHUB_SUFFIX = "thisIsIotHubSuffix";
static const char* TEST_PROTOCOL_GATEWAY_HOSTNAME = NULL;
static const char* TEST_MQTT_MESSAGE_TOPIC = "devices/myDeviceId/messages/devicebound/#";
static const char* TEST_MQTT_MSG_TOPIC_W_1_PROP = "devices/myDeviceId/messages/devicebound/iothub-ack=Full&propName=PropValue&DeviceInfo=smokeTest&%24.to=%2Fdevices%2FmyDeviceId%2Fmessages%2FdeviceBound&%24.cid&%24.uid";
static const char* TEST_MQTT_INPUT_QUEUE_SUBSCRIBE_NAME_1 = "devices/thisIsDeviceID/modules/thisIsModuleID/#";
static const char* TEST_MQTT_INPUT_1 = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs/input1/%24.cdid=connected_device&%24.cmid=connected_module/";
static const char* TEST_MQTT_INPUT_NO_PROPERTIES = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs/input1/";
static const char* TEST_MQTT_INPUT_MISSING_INPUT_QUEUE_NAME = "devices/thisIsDeviceID/modules/thisIsModuleID/inputs";
#define TEST_INPUT_QUEUE_1 "input1"

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
static const MQTT_MESSAGE_HANDLE TEST_MQTT_MESSAGE_HANDLE = (MQTT_MESSAGE_HANDLE)0x1124;

static const IOTHUB_CLIENT_TRANSPORT_PROVIDER TEST_PROTOCOL = (IOTHUB_CLIENT_TRANSPORT_PROVIDER)0x1127;

static XIO_HANDLE TEST_XIO_HANDLE = (XIO_HANDLE)0x1126;

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

TEST_DEFINE_ENUM_TYPE(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_CONTENT_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_CONTENT_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES);


static TEST_MUTEX_HANDLE test_serialize_mutex;

static DLIST_ENTRY g_waitingToSend;


// Message delivered by mqtt_common layer to our mocked callback.
static IOTHUB_MESSAGE_HANDLE g_messageFromCallback;


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

typedef struct TEST_EXPECTED_APPLICATION_PROPERTIES_TAG
{
    char** keys;
    char** values;
    size_t keysLength;
} TEST_EXPECTED_APPLICATION_PROPERTIES;

typedef struct TEST_EXPECTED_MESSAGE_PROPERTIES_TAG
{
    char* contentType;
    char* contentEncoding;
    char* messageId;
    char* correlationId;
    char* inputName;
    char* connectionModuleId;
    char* connectionDeviceId;
    char* messageCreationTime;
    char* messageUserId;
    TEST_EXPECTED_APPLICATION_PROPERTIES* applicationProperties;
} TEST_EXPECTED_MESSAGE_PROPERTIES;

#define TEST_CORRELATION_PROPERTY "correlation%2FId%25Value"
#define TEST_MSG_USER_ID_VALUE "message%2FUserId%25Value"
#define TEST_MSG_ID_VALUE "message%2FId%25Value"
#define TEST_CONTENT_TYPE_VALUE "content%2FType%25Value"
#define TEST_CONTENT_ENCODING_VALUE "content%2FEncoding%25Value"
#define TEST_CONNECTION_DEVICE_VALUE "connection%2FDevice%25Value"
#define TEST_CONNECTION_MODULE_VALUE "module%2FDevice%25Value"
#define TEST_CREATION_TIME_VALUE "creation%2FTime%25Value"

static const char* TEST_MQTT_SYSTEM_TOPIC_1 = "devices/myDeviceId/messages/devicebound/iothub-ack=Full&%24.to=%2Fdevices%2FmyDeviceId%2Fmessages%2FdeviceBound&%24.cid=" TEST_CORRELATION_PROPERTY "&%24.uid=" TEST_MSG_USER_ID_VALUE;
TEST_EXPECTED_MESSAGE_PROPERTIES systemTopic1Expected = { NULL, NULL, NULL, TEST_CORRELATION_PROPERTY, NULL, NULL, NULL, NULL, TEST_MSG_USER_ID_VALUE, NULL};

static const char* TEST_MQTT_MSG_CORRELATION_ID_TOPIC = "devices/myDeviceId/messages/devicebound/%24.cid=" TEST_CORRELATION_PROPERTY;
TEST_EXPECTED_MESSAGE_PROPERTIES correlationIdSetExpected = { NULL, NULL, NULL, TEST_CORRELATION_PROPERTY, NULL, NULL, NULL, NULL, NULL, NULL};

static const char* TEST_MQTT_MSG_USER_ID_TOPIC = "devices/myDeviceId/messages/devicebound/%24.uid=" TEST_MSG_USER_ID_VALUE;
TEST_EXPECTED_MESSAGE_PROPERTIES messageUserIdExpected = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, TEST_MSG_USER_ID_VALUE, NULL };

static const char* TEST_MQTT_MSG_ID_TOPIC = "devices/myDeviceId/messages/devicebound/%24.mid=" TEST_MSG_ID_VALUE;
TEST_EXPECTED_MESSAGE_PROPERTIES messageIdExpected = { NULL, NULL, TEST_MSG_ID_VALUE, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static const char* TEST_MQTT_CONTENT_TYPE_TOPIC = "devices/myDeviceId/messages/devicebound/%24.ct=" TEST_CONTENT_TYPE_VALUE;
TEST_EXPECTED_MESSAGE_PROPERTIES contentTypeExpected = { TEST_CONTENT_TYPE_VALUE, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static const char* TEST_MQTT_CONTENT_ENCODING_TOPIC = "devices/myDeviceId/messages/devicebound/%24.ce=" TEST_CONTENT_ENCODING_VALUE;
TEST_EXPECTED_MESSAGE_PROPERTIES contentEncodingExpected = { NULL, TEST_CONTENT_ENCODING_VALUE, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static const char* TEST_MQTT_CONNECTION_DEVICE_ID_TOPIC = "devices/myDeviceId/messages/devicebound/%24.cdid=" TEST_CONNECTION_DEVICE_VALUE;
TEST_EXPECTED_MESSAGE_PROPERTIES connectionDeviceIdExpected = { NULL, NULL, NULL, NULL, NULL, NULL, TEST_CONNECTION_DEVICE_VALUE, NULL, NULL, NULL };

static const char* TEST_MQTT_CONNECTION_MODULE_ID_TOPIC = "devices/myDeviceId/messages/devicebound/%24.cmid=" TEST_CONNECTION_MODULE_VALUE;
TEST_EXPECTED_MESSAGE_PROPERTIES connectionModuleIdExpected = { NULL, NULL, NULL, NULL, NULL, TEST_CONNECTION_MODULE_VALUE, NULL, NULL, NULL, NULL };

static const char* TEST_MQTT_CONNECTION_CREATION_TIME_TOPIC = "devices/myDeviceId/messages/devicebound/%24.ctime=" TEST_CREATION_TIME_VALUE;
TEST_EXPECTED_MESSAGE_PROPERTIES creationTimeExpected = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, TEST_CREATION_TIME_VALUE, NULL, NULL };

#define TEST_ALL_SYSTEM_PROPERTIES "%24.cid=" TEST_CORRELATION_PROPERTY "&%24.uid=" TEST_MSG_USER_ID_VALUE \
                                   "&%24.mid=" TEST_MSG_ID_VALUE "&%24.ct=" TEST_CONTENT_TYPE_VALUE \
                                   "&%24.ce=" TEST_CONTENT_ENCODING_VALUE   "&%24.cdid="  TEST_CONNECTION_DEVICE_VALUE  \
                                   "&%24.cmid=" TEST_CONNECTION_MODULE_VALUE "&%24.ctime=" TEST_CREATION_TIME_VALUE

static const char* TEST_MQTT_MSG_ALL_SYSTEM_TOPIC = "devices/myDeviceId/messages/devicebound/" TEST_ALL_SYSTEM_PROPERTIES;
TEST_EXPECTED_MESSAGE_PROPERTIES allSystemPropertiesSet1Expected = { TEST_CONTENT_TYPE_VALUE, TEST_CONTENT_ENCODING_VALUE, TEST_MSG_ID_VALUE, TEST_CORRELATION_PROPERTY, NULL, TEST_CONNECTION_MODULE_VALUE, TEST_CONNECTION_DEVICE_VALUE, TEST_CREATION_TIME_VALUE, TEST_MSG_USER_ID_VALUE, NULL};

#define TEST_TOPICS_TO_IGNORE "iothub-operation=valueToIgnore&iothub-ack=valueToIgnore&%24.to=valueToIgnore&%24.on=valueToIgnore&%24.exp=valueToIgnore&devices/=valueToIgnore"
#define TEST_TOPICS_TO_NOT_IGNORE "&devices=valueToApp1&to=valueToApp2&exp=valueToApp3&on=valueToApp4"
static const char* TEST_MQTT_IGNORED_TOPICS= "devices/myDeviceId/messages/devicebound/" TEST_TOPICS_TO_IGNORE TEST_TOPICS_TO_NOT_IGNORE;

char* expectedNotIgnoredKeys[] = {"devices", "to", "exp", "on"};
char* expectedNotIgnoredValues[] = {"valueToApp1", "valueToApp2", "valueToApp3", "valueToApp4" };
TEST_EXPECTED_APPLICATION_PROPERTIES expectedNotIgnored = { expectedNotIgnoredKeys, expectedNotIgnoredValues, 4};
TEST_EXPECTED_MESSAGE_PROPERTIES mostlyIgnoredPropertiesExpected = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &expectedNotIgnored};

static const char* mqttNoMatchTopic[] = {
    "",
    "ThisIsNotCloseToBeingALegalTopic",
    "/device/",
    "devices/",
    "devices/myDeviceId/messages",
    "devices/myDeviceId/messages/deviceboun",
    "/devices/myDeviceId/messages/devicebound",
    // These are legal topics but as we're not subscribed to them they should be ignored.
    "$iothub/twin/twinData",
    "iothub/methods/methodData",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/"
};
static const size_t mqttNoMatchTopicLength = sizeof(mqttNoMatchTopic) / sizeof(mqttNoMatchTopic[0]);

static const char* emptyPropertyMQTTTopics[] = {
    "devices/myDeviceId/messages/devicebound/",
    "devices/myDeviceId/messages/devicebound/&",
    "devices/myDeviceId/messages/devicebound/&&",
    "devices/myDeviceId/messages/devicebound/&&&",
    "devices/myDeviceId/messages/devicebound/=",
    "devices/myDeviceId/messages/devicebound/fooBar",
};

static const size_t emptyMQTTTopicsLength = sizeof(emptyPropertyMQTTTopics) / sizeof(emptyPropertyMQTTTopics[0]);  
TEST_EXPECTED_MESSAGE_PROPERTIES noProperties = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static const char* TEST_MQTT_MESSAGE_APP_PROPERTIES_1 = "devices/myDeviceId/messages/devicebound/customKey1=customValue1";
char* expectedKey1[] = {"customKey1"};
char* expectedValue1[] = {"customValue1"};
TEST_EXPECTED_APPLICATION_PROPERTIES app1 = { expectedKey1, expectedValue1, 1};
TEST_EXPECTED_MESSAGE_PROPERTIES expectedAppProperties1 = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &app1};

static const char* TEST_MQTT_MESSAGE_APP_PROPERTIES_2 = "devices/myDeviceId/messages/devicebound/customKey1=customValue1&customKey2=customValue2";
char* expectedKey2[] = {"customKey1", "customKey2"};
char* expectedValue2[] = {"customValue1", "customValue2"};
TEST_EXPECTED_APPLICATION_PROPERTIES app2 = { expectedKey2, expectedValue2, 2};
TEST_EXPECTED_MESSAGE_PROPERTIES expectedAppProperties2 = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &app2};

#define TEST_APP_PROPERTY_KEY1 "temperature%2FAlert1"
#define TEST_APP_PROPERTY_KEY2 "temperature%2FAlert2"
#define TEST_APP_PROPERTY_KEY3 "temperature%2FAlert3"

#define TEST_APP_PROPERTY_VALUE1 "false1%25"
#define TEST_APP_PROPERTY_VALUE2 "false2%25"
#define TEST_APP_PROPERTY_VALUE3 "false3%25"

// MQTT Topic representation of 3 custom application key/value pairs above
#define TEST_APP_PROPERTIES_MQTT_STRING TEST_APP_PROPERTY_KEY1 "=" TEST_APP_PROPERTY_VALUE1 "&" TEST_APP_PROPERTY_KEY2 "=" TEST_APP_PROPERTY_VALUE2 "&" \
                                                                   TEST_APP_PROPERTY_KEY3 "=" TEST_APP_PROPERTY_VALUE3

static const char* TEST_MQTT_MESSAGE_APP_PROPERTIES_3 = "devices/myDeviceId/messages/devicebound/" TEST_APP_PROPERTIES_MQTT_STRING;

char* expectedKey3[] = {TEST_APP_PROPERTY_KEY1, TEST_APP_PROPERTY_KEY2, TEST_APP_PROPERTY_KEY3};
char* expectedValue3[] = {TEST_APP_PROPERTY_VALUE1, TEST_APP_PROPERTY_VALUE2, TEST_APP_PROPERTY_VALUE3};
TEST_EXPECTED_APPLICATION_PROPERTIES app3 = { expectedKey3, expectedValue3, 3};
TEST_EXPECTED_MESSAGE_PROPERTIES expectedAppProperties3 = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &app3};

TEST_EXPECTED_MESSAGE_PROPERTIES allSystemManyAppPropertiesSetExpected = { TEST_CONTENT_TYPE_VALUE, TEST_CONTENT_ENCODING_VALUE, TEST_MSG_ID_VALUE, TEST_CORRELATION_PROPERTY, NULL, TEST_CONNECTION_MODULE_VALUE, TEST_CONNECTION_DEVICE_VALUE, TEST_CREATION_TIME_VALUE, TEST_MSG_USER_ID_VALUE, &app3};
static const char* TEST_MQTT_MESSAGE_SYSTEM_MANY_APP = "devices/myDeviceId/messages/devicebound/" TEST_APP_PROPERTIES_MQTT_STRING "&" TEST_ALL_SYSTEM_PROPERTIES;


const char* TEST_INPUT_FILTER_1 = "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/" TEST_APP_PROPERTIES_MQTT_STRING "&%24.cdid=" TEST_CONNECTION_DEVICE_VALUE
                                  "&%24.cmid=" TEST_CONNECTION_MODULE_VALUE "&%24.cid=" TEST_CORRELATION_PROPERTY "&%24.mid=" TEST_MSG_ID_VALUE;

char* inputFilterExpectedKey1[] = {TEST_APP_PROPERTY_KEY1, TEST_APP_PROPERTY_KEY2, TEST_APP_PROPERTY_KEY3};
char* inputFilterExpectedValue1[] = {TEST_APP_PROPERTY_VALUE1, TEST_APP_PROPERTY_VALUE2, TEST_APP_PROPERTY_VALUE3};
TEST_EXPECTED_APPLICATION_PROPERTIES inputFilterAppProperties = { inputFilterExpectedKey1, inputFilterExpectedValue1, 3};

TEST_EXPECTED_MESSAGE_PROPERTIES testFilter1 = { NULL, NULL, TEST_MSG_ID_VALUE, TEST_CORRELATION_PROPERTY, TEST_INPUT_QUEUE_1, TEST_CONNECTION_MODULE_VALUE, TEST_CONNECTION_DEVICE_VALUE, NULL, NULL, &inputFilterAppProperties};


const char* TEST_INPUT_FILTER_NO_PROPERTIES = "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/";
TEST_EXPECTED_MESSAGE_PROPERTIES inputFilterNoProperties = { NULL, NULL, NULL, NULL, TEST_INPUT_QUEUE_1, NULL, NULL, NULL, NULL, NULL};

static const char* TEST_MQTT_INPUT_IGNORED_TOPICS= "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/" TEST_TOPICS_TO_IGNORE TEST_TOPICS_TO_NOT_IGNORE;

TEST_EXPECTED_MESSAGE_PROPERTIES mostlyIgnoredFilterProperties = { NULL, NULL, NULL, NULL, TEST_INPUT_QUEUE_1, NULL, NULL, NULL, NULL, &expectedNotIgnored};

static const char* TEST_MQTT_INPUT_ALL_SYSTEM_TOPIC = "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/" TEST_ALL_SYSTEM_PROPERTIES;

TEST_EXPECTED_MESSAGE_PROPERTIES allInputSystemPropertiesSet1 = { TEST_CONTENT_TYPE_VALUE, TEST_CONTENT_ENCODING_VALUE, TEST_MSG_ID_VALUE, TEST_CORRELATION_PROPERTY, TEST_INPUT_QUEUE_1, TEST_CONNECTION_MODULE_VALUE, TEST_CONNECTION_DEVICE_VALUE, TEST_CREATION_TIME_VALUE, TEST_MSG_USER_ID_VALUE, NULL};

static const char* mqttNoMatchInputTopic[] = {
    "",
    "ThisIsNotCloseToBeingALegalTopic",
    "/device/",
    "devices/",
    "devices/myDeviceId/modules",
    "devices/myDeviceId/modules/thisIsModuleID/inputs",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/inputWithNoTrailSlash",
    "/devices/myDeviceId/messages/devicebound",
    // These are legal topics but as we're not subscribed to them they should be ignored.
    "devices/myDeviceId/messages/devicebound",
    "$iothub/twin/twinData",
    "iothub/methods/methodData",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/"
};
static const size_t mqttNoMatchInputTopicLength = sizeof(mqttNoMatchInputTopic) / sizeof(mqttNoMatchInputTopic[0]);

static const char* emptyPropertyMQTTInputTopics[] = {
    "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/&",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/&&",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/&&&",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/=",
    "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/foobar"
};

static const size_t emptyMQTTInputTopicsLength = sizeof(emptyPropertyMQTTInputTopics) / sizeof(emptyPropertyMQTTInputTopics[0]);  
TEST_EXPECTED_MESSAGE_PROPERTIES noInputProperties = { NULL, NULL, NULL, NULL, TEST_INPUT_QUEUE_1, NULL, NULL, NULL, NULL, NULL};

const char* TEST_INPUT_FILTER_ALL_SYSTEM_MANY_APP_1 = "devices/myDeviceId/modules/thisIsModuleID/inputs/" TEST_INPUT_QUEUE_1 "/" TEST_APP_PROPERTIES_MQTT_STRING "&" TEST_ALL_SYSTEM_PROPERTIES;
TEST_EXPECTED_MESSAGE_PROPERTIES allInputSystemPropertiesManyApplicationSet1 = { TEST_CONTENT_TYPE_VALUE, TEST_CONTENT_ENCODING_VALUE, TEST_MSG_ID_VALUE, TEST_CORRELATION_PROPERTY, TEST_INPUT_QUEUE_1, TEST_CONNECTION_MODULE_VALUE, TEST_CONNECTION_DEVICE_VALUE, TEST_CREATION_TIME_VALUE, TEST_MSG_USER_ID_VALUE, &app3};

static const char* TEST_MQTT_TOPIC_DOUBLE_EQUAL_IN_PROPS = "devices/myDeviceId/messages/devicebound/==";
char* expectedEmptyStringKey[] = {""};
char* expectedEmptyStringValue[] = {"="};
TEST_EXPECTED_APPLICATION_PROPERTIES emptyProps = { expectedEmptyStringKey, expectedEmptyStringValue, 1};
TEST_EXPECTED_MESSAGE_PROPERTIES expectedEmptyStringProps = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &emptyProps};

static char* my_IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, uint64_t expiry_time_relative_seconds, const char* key_name)
{
    (void)handle;
    (void)scope;
    (void)expiry_time_relative_seconds;
    (void)key_name;

    char* result;
    size_t len = strlen(TEST_SAS_TOKEN);
    result = (char*)malloc(len+1);
    strcpy(result, TEST_SAS_TOKEN);
    return result;
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


// my_Transport_MessageCallback receives the message handle generated by the mqtt_common layer.  It stores it in a global for
// the test case to check the value.
static bool my_Transport_MessageCallback(IOTHUB_MESSAGE_HANDLE messageHandle, void* ctx)
{
    (void)ctx;
    g_messageFromCallback = messageHandle;
    return true;
}

// my_Transport_MessageCallbackFromInput role in unit test is analogous to my_Transport_MessageCallback, except
// it receives input queue callbacks.
static bool my_Transport_MessageCallbackFromInput(IOTHUB_MESSAGE_HANDLE messageHandle, void* ctx)
{
    (void)ctx;
    g_messageFromCallback = messageHandle;
    return true;
}

static MQTT_CLIENT_HANDLE my_mqtt_client_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* callbackCtx, ON_MQTT_ERROR_CALLBACK errorCallback, void* errorcallbackCtx)
{
    g_fnMqttMsgRecv = msgRecv;
    g_fnMqttOperationCallback = opCallback;
    g_callbackCtx = callbackCtx;
    g_fnMqttErrorCallback = errorCallback;
    g_errorcallbackCtx = errorcallbackCtx;
    return (MQTT_CLIENT_HANDLE)malloc(sizeof(MQTT_CLIENT_HANDLE));
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
    free(handle);
}

static XIO_HANDLE my_xio_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, const void* xio_create_parameters)
{
    (void)io_interface_description;
    (void)xio_create_parameters;
    return (XIO_HANDLE)malloc(sizeof(XIO_HANDLE));
}

static void my_xio_destroy(XIO_HANDLE ioHandle)
{
    free(ioHandle);
}

static XIO_HANDLE get_IO_transport(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options)
{
    (void)fully_qualified_name;
    (void)mqtt_transport_proxy_options;
    return (XIO_HANDLE)malloc(sizeof(XIO_HANDLE));
}

// g_mqttTopicToTest is set in our mocked implementation to return MQTT topic that the product implementation
// that parser MQTT PUBLISH's sent to the device should process.
static const char* g_mqttTopicToTest;

static const char* my_mqttmessage_getTopicName(MQTT_MESSAGE_HANDLE handle)
{
    (void)handle;
    return g_mqttTopicToTest;
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
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    
    REGISTER_GLOBAL_MOCK_HOOK(xio_create, my_xio_create);
    REGISTER_GLOBAL_MOCK_HOOK(xio_destroy, my_xio_destroy);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHub_Transport_ValidateCallbacks, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceKey, TEST_DEVICE_KEY);
    REGISTER_GLOBAL_MOCK_HOOK(Transport_MessageCallback, my_Transport_MessageCallback);
    REGISTER_GLOBAL_MOCK_HOOK(Transport_DeviceMethod_Complete_Callback, my_Transport_DeviceMethod_Complete_Callback);
    REGISTER_GLOBAL_MOCK_HOOK(Transport_MessageCallbackFromInput, my_Transport_MessageCallbackFromInput);
    REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_init, my_mqtt_client_init);
    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_connect, 0);

    REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_deinit, my_mqtt_client_deinit);
    REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_disconnect, my_mqtt_client_disconnect);
    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_subscribe, 0);
    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_unsubscribe, 0);
    REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_publish, 0);

    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_create, TEST_MQTT_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_create_in_place, TEST_MQTT_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(mqttmessage_getApplicationMsg, &TEST_APP_PAYLOAD);
    REGISTER_GLOBAL_MOCK_HOOK(mqttmessage_getTopicName, my_mqttmessage_getTopicName);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_Credential_Type, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_SasToken, my_IoTHubClient_Auth_Get_SasToken);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Is_SasToken_Valid, SAS_TOKEN_STATUS_VALID);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_SasToken_Expiry, 3600);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_create, TEST_RETRY_CONTROL_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(retry_control_should_retry, 0);
    REGISTER_GLOBAL_MOCK_RETURN(retry_control_set_option, 0);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

static void reset_data_from_callback()
{
    if (g_messageFromCallback != NULL)
    {
        IoTHubMessage_Destroy(g_messageFromCallback);
        g_messageFromCallback = NULL;
    }
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

    reset_data_from_callback();
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

//
// UrlDecodeTestHelper URL decodes the expected string
//
static char* UrlDecodeTestHelper(const char* stringToDecode)
{
    char* result;
    
    if (stringToDecode != NULL)
    {
        STRING_HANDLE h = URL_DecodeString(stringToDecode);
        ASSERT_IS_NOT_NULL(h);
        
        ASSERT_ARE_EQUAL(int, 0, mallocAndStrcpy_s(&result, STRING_c_str(h)));
        STRING_delete(h);
    }
    else
    {
        result = NULL;
    }
    return result;
}

//
// CreateUrlDecodedMessageProperties creates a URL decoded version of the expectedMessageProperties structure
//
static void CreateUrlDecodedMessageProperties(const TEST_EXPECTED_MESSAGE_PROPERTIES* expectedMessageProperties, TEST_EXPECTED_MESSAGE_PROPERTIES* expectedUrlDecodedMessageProperties)
{
	memset(expectedUrlDecodedMessageProperties, 0, sizeof(*expectedUrlDecodedMessageProperties));
    expectedUrlDecodedMessageProperties->contentType = UrlDecodeTestHelper(expectedMessageProperties->contentType);
    expectedUrlDecodedMessageProperties->contentEncoding = UrlDecodeTestHelper(expectedMessageProperties->contentEncoding);
    expectedUrlDecodedMessageProperties->messageId = UrlDecodeTestHelper(expectedMessageProperties->messageId);
    expectedUrlDecodedMessageProperties->correlationId = UrlDecodeTestHelper(expectedMessageProperties->correlationId);
    expectedUrlDecodedMessageProperties->inputName = UrlDecodeTestHelper(expectedMessageProperties->inputName);
    expectedUrlDecodedMessageProperties->connectionModuleId = UrlDecodeTestHelper(expectedMessageProperties->connectionModuleId);
    expectedUrlDecodedMessageProperties->connectionDeviceId = UrlDecodeTestHelper(expectedMessageProperties->connectionDeviceId);
    expectedUrlDecodedMessageProperties->messageCreationTime = UrlDecodeTestHelper(expectedMessageProperties->messageCreationTime);
    expectedUrlDecodedMessageProperties->messageUserId = UrlDecodeTestHelper(expectedMessageProperties->messageUserId);
    
    if (expectedMessageProperties->applicationProperties != NULL)
    {
        size_t numberOfApplicationProperties = expectedMessageProperties->applicationProperties->keysLength;

        expectedUrlDecodedMessageProperties->applicationProperties = (TEST_EXPECTED_APPLICATION_PROPERTIES*)calloc(1, sizeof(*expectedUrlDecodedMessageProperties->applicationProperties));
        ASSERT_IS_NOT_NULL(expectedUrlDecodedMessageProperties->applicationProperties);

        expectedUrlDecodedMessageProperties->applicationProperties->keysLength = numberOfApplicationProperties;
        expectedUrlDecodedMessageProperties->applicationProperties->keys = (char**)calloc(1, sizeof(char *) * numberOfApplicationProperties);
        ASSERT_IS_NOT_NULL(expectedUrlDecodedMessageProperties->applicationProperties->keys);
        expectedUrlDecodedMessageProperties->applicationProperties->values = (char**)calloc(1, sizeof(char *) * numberOfApplicationProperties);
        ASSERT_IS_NOT_NULL(expectedUrlDecodedMessageProperties->applicationProperties->values);

        for (size_t i = 0; i < numberOfApplicationProperties; i++)
        {
            expectedUrlDecodedMessageProperties->applicationProperties->keys[i] = UrlDecodeTestHelper(expectedMessageProperties->applicationProperties->keys[i]);
            expectedUrlDecodedMessageProperties->applicationProperties->values[i] = UrlDecodeTestHelper(expectedMessageProperties->applicationProperties->values[i]);
        }
    }
}

//
// FreeUrlDecodedMessageProperties frees the memory allocated in urlDeco
// 
static void FreeUrlDecodedMessageProperties(TEST_EXPECTED_MESSAGE_PROPERTIES* expectedUrlDecodedMessageProperties)
{
    if (expectedUrlDecodedMessageProperties)
    {
        free(expectedUrlDecodedMessageProperties->contentType);
        free(expectedUrlDecodedMessageProperties->contentEncoding);
        free(expectedUrlDecodedMessageProperties->messageId);
        free(expectedUrlDecodedMessageProperties->correlationId);
        free(expectedUrlDecodedMessageProperties->inputName);
        free(expectedUrlDecodedMessageProperties->connectionModuleId);
        free(expectedUrlDecodedMessageProperties->connectionDeviceId);
        free(expectedUrlDecodedMessageProperties->messageCreationTime);
        free(expectedUrlDecodedMessageProperties->messageUserId);

        
        if (expectedUrlDecodedMessageProperties->applicationProperties != NULL)
        {
            size_t numberOfApplicationProperties = expectedUrlDecodedMessageProperties->applicationProperties->keysLength;

            for (size_t i = 0; i < numberOfApplicationProperties; i++)
            {
                free(expectedUrlDecodedMessageProperties->applicationProperties->values[i]);
                free(expectedUrlDecodedMessageProperties->applicationProperties->keys[i]);
            }

            free(expectedUrlDecodedMessageProperties->applicationProperties->values);
            free(expectedUrlDecodedMessageProperties->applicationProperties->keys);
            free(expectedUrlDecodedMessageProperties->applicationProperties);
        }
    }
}


//
// VerifyExpectedMessageReceived checks that the message we've received on mock callback matches the expected for this test case.
//
static void VerifyExpectedMessageReceived(const char* topicToTest, const TEST_EXPECTED_MESSAGE_PROPERTIES* expectedMessageProperties)
{
    ASSERT_IS_NOT_NULL(g_messageFromCallback);

    // Messages are always delivered as byte arrrays to applications.
    ASSERT_ARE_EQUAL(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_BYTEARRAY, IoTHubMessage_GetContentType(g_messageFromCallback));

    const unsigned char* messageBody;
    size_t messageBodyLen;
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, IoTHubMessage_GetByteArray(g_messageFromCallback, &messageBody, &messageBodyLen), "can't get byte array for topic %s", topicToTest);
    ASSERT_ARE_EQUAL(int, TEST_APP_PAYLOAD.length, messageBodyLen, "payload lengths don't match for topic %s", topicToTest);
    ASSERT_ARE_EQUAL(int, 0, memcmp(TEST_APP_PAYLOAD.message, messageBody, TEST_APP_PAYLOAD.length), "payloads don't match for topic %s", topicToTest);

    const char* contentType = IoTHubMessage_GetContentTypeSystemProperty(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->contentType, contentType, "Content types don't match for topic %s", topicToTest);

    const char* contentEncoding = IoTHubMessage_GetContentEncodingSystemProperty(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->contentEncoding, contentEncoding, "Content encodings don't match");

    const char* messageId = IoTHubMessage_GetMessageId(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->messageId, messageId, "Message ids don't match for topic %s", topicToTest);

    const char* correlationId = IoTHubMessage_GetCorrelationId(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->correlationId, correlationId, "Correlation ids don't match for topic %s", topicToTest);

    const char* inputName = IoTHubMessage_GetInputName(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->inputName, inputName, "Input names don't match for topic %s", topicToTest);

    const char* connectionModuleId = IoTHubMessage_GetConnectionModuleId(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->connectionModuleId, connectionModuleId, "Connection module ids don't match for topic %s", topicToTest);

    const char* connectionDeviceId = IoTHubMessage_GetConnectionDeviceId(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->connectionDeviceId, connectionDeviceId, "Connection device ids don't match for topic %s", topicToTest);

    const char* messageCreationTime = IoTHubMessage_GetMessageCreationTimeUtcSystemProperty(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->messageCreationTime, messageCreationTime, "Message creation times don't match for topic %s", topicToTest);

    const char* messageUserId = IoTHubMessage_GetMessageUserIdSystemProperty(g_messageFromCallback);
    ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->messageUserId, messageUserId, "Message user ids don't match for topic %s", topicToTest);

    // These message properties can only be set by the device and then set to the MQTT server.  They are never
    // parsed on an MQTT PUBLISH to the device itself and hence in the IoTHubMessage layer they'll always be NULL.
    ASSERT_IS_NULL(IoTHubMessage_GetOutputName(g_messageFromCallback), "GetOutputName should have returned NULL for topic %s", topicToTest);
    ASSERT_IS_NULL(IoTHubMessage_GetDiagnosticPropertyData(g_messageFromCallback), "GetDiagnosticPropertyData should have returned NULL for topic %s", topicToTest);

    // Check application properties
    MAP_HANDLE mapHandle = IoTHubMessage_Properties(g_messageFromCallback);
    ASSERT_IS_NOT_NULL(mapHandle);

    const char* const* actualKeys;
    const char* const* actualValues;
    size_t actualKeysLen;
    size_t expectedKeyLen = (expectedMessageProperties->applicationProperties != NULL) ? expectedMessageProperties->applicationProperties->keysLength : 0;

    ASSERT_ARE_EQUAL(MAP_RESULT, MAP_OK, Map_GetInternals(mapHandle, &actualKeys, &actualValues, &actualKeysLen));
    ASSERT_ARE_EQUAL(int, expectedKeyLen, actualKeysLen, "Number of custom properties don't match for topic %s", topicToTest);

    for (size_t i = 0; i < expectedKeyLen; i++)
    {
        ASSERT_ARE_EQUAL(char_ptr, expectedMessageProperties->applicationProperties->values[i], IoTHubMessage_GetProperty(g_messageFromCallback, expectedMessageProperties->applicationProperties->keys[i]), "Properties don't match for topic %s", topicToTest);
    }
}


//
// TestMessageProcessing invokes the MQTT PUBLISH to device callback code, which will (on success) store
// the parsed message into the test's g_messageFromCallback.  TestMessageProcessing then verifies message is expected.
//
static void TestMessageProcessing(const char* topicToTest, const TEST_EXPECTED_MESSAGE_PROPERTIES* expectedMessageProperties, bool autoUrlEncodeDecode)
{
    // There is not a direct mechanism for this test to call into the product code's callback.  Instead what we do is 
    // invoke into the public interface of mqtt_common layer and use our mock (my_mqtt_client_init) to store the callback pointer
    // for later.
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    TEST_EXPECTED_MESSAGE_PROPERTIES expectedUrlDecodedMessageProperties;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, NULL);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    const TEST_EXPECTED_MESSAGE_PROPERTIES* propertiesToCheck;

    if (autoUrlEncodeDecode && (expectedMessageProperties != NULL))
    {
        IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &autoUrlEncodeDecode);
        CreateUrlDecodedMessageProperties(expectedMessageProperties, &expectedUrlDecodedMessageProperties);
        propertiesToCheck = &expectedUrlDecodedMessageProperties;
    }
    else
    {
        propertiesToCheck = expectedMessageProperties;
    }

    umock_c_reset_all_calls();

    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);

    // Saves the topic to test into a global that the mocked "get topic" implementation will return to product code.
    g_mqttTopicToTest = topicToTest;
    // Invokes the product code's parsing callback, which we stored away earlier.
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    if (expectedMessageProperties != NULL)
    {
        VerifyExpectedMessageReceived(topicToTest, propertiesToCheck);
    }
    else
    {
        ASSERT_IS_NULL(g_messageFromCallback, "message received from callback the product code should have failed.  topic=%s", topicToTest);
    }

    //cleanup
    if (autoUrlEncodeDecode)
    {
        FreeUrlDecodedMessageProperties(&expectedUrlDecodedMessageProperties);
    }

    IoTHubTransport_MQTT_Common_Destroy(handle);
}

//
// "Random" properties, inspired by original UT
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_sys_Properties1_succeed)
{
    TestMessageProcessing(TEST_MQTT_SYSTEM_TOPIC_1, &systemTopic1Expected, false);
}

//
// CorrelationIdValue
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_correlation_id_succeeds)
{
    TestMessageProcessing(TEST_MQTT_MSG_CORRELATION_ID_TOPIC, &correlationIdSetExpected, false);
}

//
// msgUserIdValue
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_message_user_id_succeeds)
{
    TestMessageProcessing(TEST_MQTT_MSG_USER_ID_TOPIC, &messageUserIdExpected, false);
}

//
// messageIdValue
//

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_message_id_succeeds)
{
    TestMessageProcessing(TEST_MQTT_MSG_ID_TOPIC, &messageIdExpected, false);
}

//
// contentTypeValue
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_contentType_succeeds)
{
    TestMessageProcessing(TEST_MQTT_CONTENT_TYPE_TOPIC, &contentTypeExpected, false);
}

//
// contentEncodingValue
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_contentEncoding_succeeds)
{
    TestMessageProcessing(TEST_MQTT_CONTENT_ENCODING_TOPIC, &contentEncodingExpected, false);
}


//
// connectionDeviceValue
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_ConnectionDeviceId_succeeds)
{
    TestMessageProcessing(TEST_MQTT_CONNECTION_DEVICE_ID_TOPIC, &connectionDeviceIdExpected, false);
}

//
// connectionModuleValue
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_ConnectionModuleId_succeeds)
{
    TestMessageProcessing(TEST_MQTT_CONNECTION_MODULE_ID_TOPIC, &connectionModuleIdExpected, false);
}

//
// creationTimeValue
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_ConnectionCreationTime_succeeds)
{
    TestMessageProcessing(TEST_MQTT_CONNECTION_CREATION_TIME_TOPIC, &creationTimeExpected, false);
}

//
// All system properties
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_sys_all_set)
{
    TestMessageProcessing(TEST_MQTT_MSG_ALL_SYSTEM_TOPIC, &allSystemPropertiesSet1Expected, false);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_sys_all_auto_decode_set)
{
    TestMessageProcessing(TEST_MQTT_MSG_ALL_SYSTEM_TOPIC, &allSystemPropertiesSet1Expected, true);
}


//
// MQTT ignores certain values to maintain compat with previous versions of parser.  This tests these values and also makes sure values that
// are similar to but not identical to are passed to application.
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_many_ignored_properties)
{
    TestMessageProcessing(TEST_MQTT_IGNORED_TOPICS, &mostlyIgnoredPropertiesExpected, false);
}

//
// Tests MQTT topics that should not match C2D message processor.  Some are legal MQTT we'd expect from IoT Hub, others are not.
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_nomatch_MQTT_topics_fail)
{
    for (size_t i = 0; i < mqttNoMatchTopicLength; i++)
    {
        TestMessageProcessing(mqttNoMatchTopic[i], NULL, false);
    }
}

//
// MQTT topics that are legal but do not contain properties.  The parser is fairly forgiving that once the MQTT TOPIC is matched,
// if the properties are off we'll deliver the message to application
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_with_empty_properties_succeed)
{
    for (size_t i = 0; i < emptyMQTTTopicsLength; i++)
    {
        reset_data_from_callback();
        TestMessageProcessing(emptyPropertyMQTTTopics[i], &noProperties, false);
    }
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_app_properties1_succeed)
{
    TestMessageProcessing(TEST_MQTT_MESSAGE_APP_PROPERTIES_1, &expectedAppProperties1, false);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_app_properties2_succeed)
{
    TestMessageProcessing(TEST_MQTT_MESSAGE_APP_PROPERTIES_2, &expectedAppProperties2, false);
}


TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_app_properties3_succeed)
{
    TestMessageProcessing(TEST_MQTT_MESSAGE_APP_PROPERTIES_3, &expectedAppProperties3, false);
}

//
// Tests all system properties and many app properties
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_all_sys_and_many_app_succeed)
{
    TestMessageProcessing(TEST_MQTT_MESSAGE_SYSTEM_MANY_APP, &allSystemManyAppPropertiesSetExpected, false);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_app_properties3_auto_decode_succeed)
{
    TestMessageProcessing(TEST_MQTT_MESSAGE_SYSTEM_MANY_APP, &allSystemManyAppPropertiesSetExpected, true);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_double_equals_in_props_succceed)
{
    TestMessageProcessing(TEST_MQTT_TOPIC_DOUBLE_EQUAL_IN_PROPS, &expectedEmptyStringProps, false);
}

//
//  IoT Edge module to module processing tests
//
//


//
// TestInputQueueProcessing invokes the MQTT PUBLISH to device callback code, which will (on success) store
// the parsed message into the test's g_messageFromCallback.  TestMessageProcessing then verifies message is expected.
//
static void TestInputQueueProcessing(const char* topicToTest, const TEST_EXPECTED_MESSAGE_PROPERTIES* expectedMessageProperties, bool autoUrlEncodeDecode)
{
    // There is not a direct mechanism for this test to call into the product code's callback.  Instead what we do is 
    // invoke into the public interface of mqtt_common layer and use our mock (my_mqtt_client_init) to store the callback pointer
    // for later.
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    TEST_EXPECTED_MESSAGE_PROPERTIES expectedUrlDecodedMessageProperties;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME, TEST_MODULE_ID);

    TRANSPORT_LL_HANDLE handle = IoTHubTransport_MQTT_Common_Create(&config, get_IO_transport, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransport_MQTT_Common_Subscribe_InputQueue(handle);
    IoTHubTransport_MQTT_Common_DoWork(handle);

    const TEST_EXPECTED_MESSAGE_PROPERTIES* propertiesToCheck;

    if (autoUrlEncodeDecode && (expectedMessageProperties != NULL))
    {
        IoTHubTransport_MQTT_Common_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &autoUrlEncodeDecode);
        CreateUrlDecodedMessageProperties(expectedMessageProperties, &expectedUrlDecodedMessageProperties);
        propertiesToCheck = &expectedUrlDecodedMessageProperties;
    }
    else
    {
        propertiesToCheck = expectedMessageProperties;
    }
    
    umock_c_reset_all_calls();

    ASSERT_IS_NOT_NULL(g_fnMqttMsgRecv);

    // Saves the topic to test into a global that the mocked "get topic" implementation will return to product code.
    g_mqttTopicToTest = topicToTest;
    // Invokes the product code's parsing callback, which we stored away earlier.
    g_fnMqttMsgRecv(TEST_MQTT_MESSAGE_HANDLE, g_callbackCtx);

    if (propertiesToCheck != NULL)
    {
        VerifyExpectedMessageReceived(topicToTest, propertiesToCheck);
    }
    else
    {
        ASSERT_IS_NULL(g_messageFromCallback, "message received from callback the product code should have failed.  topic=%s", topicToTest);
    }

    //cleanup
    if (autoUrlEncodeDecode)
    {
        FreeUrlDecodedMessageProperties(&expectedUrlDecodedMessageProperties);
    }

    IoTHubTransport_MQTT_Common_Destroy(handle);
}


//
// Tests a topic scraped from actual IoT Edge communication
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_InputQueue_1_success)
{
    TestInputQueueProcessing(TEST_INPUT_FILTER_1, &testFilter1, false);
}

//
// Tests topic with no properties
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_InputQueue_no_properties_success)
{
    TestInputQueueProcessing(TEST_INPUT_FILTER_NO_PROPERTIES, &inputFilterNoProperties, false);
}

//
// Input queue with ignored and not ignored topics
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_InputQueue_with_many_ignored_properties)
{
    TestInputQueueProcessing(TEST_MQTT_INPUT_IGNORED_TOPICS, &mostlyIgnoredFilterProperties, false);
}

//
// Input queue with all system properties
//
TEST_FUNCTION(IoTHubTransport_MQTT_Input_MessageRecv_with_sys_all_set)
{
    TestInputQueueProcessing(TEST_MQTT_INPUT_ALL_SYSTEM_TOPIC, &allInputSystemPropertiesSet1, false);
}

//
// Tests MQTT topics that should not match input message processor.  Some are legal MQTT we'd expect from IoT Hub, others are not.
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_nomatch_MQTT_Input_topics_fail)
{
    for (size_t i = 0; i < mqttNoMatchTopicLength; i++)
    {
        TestInputQueueProcessing(mqttNoMatchInputTopic[i], NULL, false);
    }
}

//
// MQTT topics that are legal but do not contain properties.  The parser is fairly forgiving that once the MQTT TOPIC is matched,
// if the properties are off we'll deliver the message to application
//
TEST_FUNCTION(IoTHubTransport_MQTT_Common_MessageRecv_Input_with_empty_properties_succeed)
{
    for (size_t i = 0; i < emptyMQTTInputTopicsLength; i++)
    {
        reset_data_from_callback();
        TestInputQueueProcessing(emptyPropertyMQTTInputTopics[i], &noInputProperties, false);
    }
}

TEST_FUNCTION(IoTHubTransport_MQTT_Input_MessageRecv_with_sys_all_set_auto_decode_success)
{
    TestInputQueueProcessing(TEST_MQTT_INPUT_ALL_SYSTEM_TOPIC, &allInputSystemPropertiesSet1, true);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Common_InputQueue_1_auto_decode_success)
{
    TestInputQueueProcessing(TEST_INPUT_FILTER_1, &testFilter1, true);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Input_MessageRecv_with_sys_all_many_app_set)
{
    TestInputQueueProcessing(TEST_INPUT_FILTER_ALL_SYSTEM_MANY_APP_1, &allInputSystemPropertiesManyApplicationSet1, false);
}

TEST_FUNCTION(IoTHubTransport_MQTT_Input_MessageRecv_with_sys_all_many_app_auto_decode_set)
{
    TestInputQueueProcessing(TEST_INPUT_FILTER_ALL_SYSTEM_MANY_APP_1, &allInputSystemPropertiesManyApplicationSet1, true);
}

END_TEST_SUITE(iothubtransport_mqtt_common_int_ut)
