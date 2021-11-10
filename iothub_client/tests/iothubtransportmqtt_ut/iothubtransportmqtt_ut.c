// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"

#if defined _MSC_VER
#pragma warning(disable: 4054) /* MSC incorrectly fires this */
#endif

static void* real_malloc(size_t size)
{
    return malloc(size);
}

static void real_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/platform.h"

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/tlsio.h"
#include "internal/iothubtransport_mqtt_common.h"

#undef ENABLE_MOCKS

#include "internal/iothubtransport.h"
#include "iothubtransportmqtt.h"

static const char* TEST_STRING_VALUE = "FULLY_QUALIFIED_HOSTNAME";
static const char* TEST_DEVICE_ID = "thisIsDeviceID";
static const char* TEST_DEVICE_KEY = "thisIsDeviceKey";
static const char* TEST_IOTHUB_NAME = "thisIsIotHubName";
static const char* TEST_IOTHUB_SUFFIX = "thisIsIotHubSuffix";
static const char* TEST_PROTOCOL_GATEWAY_HOSTNAME = NULL;
static const char* TEST_PROTOCOL_GATEWAY_HOSTNAME_NON_NULL = "ssl://thisIsAGatewayHostName.net";

static const char* TEST_OPTION_NAME = "TEST_OPTION_NAME";
static const char* TEST_OPTION_VALUE = "test_option_value";

static const TRANSPORT_LL_HANDLE TEST_TRANSPORT_HANDLE = (TRANSPORT_LL_HANDLE)0x4444;
static XIO_HANDLE TEST_XIO_HANDLE = (XIO_HANDLE)0x1126;
static IOTHUB_DEVICE_HANDLE TEST_DEVICE_HANDLE = (IOTHUB_DEVICE_HANDLE)0x1181;

static IO_INTERFACE_DESCRIPTION* TEST_WSIO_INTERFACE_DESCRIPTION = (IO_INTERFACE_DESCRIPTION*)0x1182;
static IO_INTERFACE_DESCRIPTION* TEST_TLSIO_INTERFACE_DESCRIPTION = (IO_INTERFACE_DESCRIPTION*)0x1183;
static IO_INTERFACE_DESCRIPTION* TEST_HTTP_PROXY_IO_INTERFACE_DESCRIPTION = (IO_INTERFACE_DESCRIPTION*)0x1185;

static TRANSPORT_CALLBACKS_INFO* g_transport_cb_info = (TRANSPORT_CALLBACKS_INFO*)0x227733;
static void* g_transport_cb_ctx = (void*)0x499922;

static IOTHUB_CLIENT_CONFIG g_iothubClientConfig = { 0 };
static DLIST_ENTRY g_waitingToSend;

static MQTT_GET_IO_TRANSPORT g_get_io_transport;

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

TEST_DEFINE_ENUM_TYPE(PLATFORM_INFO_OPTION, PLATFORM_INFO_OPTION_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PLATFORM_INFO_OPTION, PLATFORM_INFO_OPTION_VALUES);

static TEST_MUTEX_HANDLE test_serialize_mutex;

#define TEST_RETRY_POLICY IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define TEST_RETRY_TIMEOUT_SECS 60

#define TEST_DEVICE_STATUS_CODE             200
#define TEST_METHOD_ID                      (METHOD_HANDLE)0x61
static const unsigned char* TEST_DEVICE_METHOD_RESPONSE = (const unsigned char*)0x62;

//Callbacks for Testing
static void* g_callbackCtx;

static pfIotHubTransport_SendMessageDisposition     IoTHubTransportMqtt_SendMessageDisposition;
static pfIoTHubTransport_GetHostname                IoTHubTransportMqtt_GetHostname;
static pfIoTHubTransport_SetOption                  IoTHubTransportMqtt_SetOption;
static pfIoTHubTransport_Create                     IoTHubTransportMqtt_Create;
static pfIoTHubTransport_Destroy                    IoTHubTransportMqtt_Destroy;
static pfIotHubTransport_Register                   IoTHubTransportMqtt_Register;
static pfIotHubTransport_Unregister                 IoTHubTransportMqtt_Unregister;
static pfIoTHubTransport_Subscribe                  IoTHubTransportMqtt_Subscribe;
static pfIoTHubTransport_Unsubscribe                IoTHubTransportMqtt_Unsubscribe;
static pfIoTHubTransport_DoWork                     IoTHubTransportMqtt_DoWork;
static pfIoTHubTransport_SetRetryPolicy             IoTHubTransportMqtt_SetRetryPolicy;
static pfIoTHubTransport_GetSendStatus              IoTHubTransportMqtt_GetSendStatus;
static pfIoTHubTransport_Subscribe_DeviceTwin       IoTHubTransportMqtt_Subscribe_DeviceTwin;
static pfIoTHubTransport_Unsubscribe_DeviceTwin     IoTHubTransportMqtt_Unsubscribe_DeviceTwin;
static pfIoTHubTransport_GetTwinAsync         IoTHubTransportMqtt_GetTwinAsync;
static pfIoTHubTransport_Subscribe_DeviceMethod     IoTHubTransportMqtt_Subscribe_DeviceMethod;
static pfIoTHubTransport_Unsubscribe_DeviceMethod   IoTHubTransportMqtt_Unsubscribe_DeviceMethod;
static pfIoTHubTransport_DeviceMethod_Response      IoTHubTransportMqtt_DeviceMethod_Response;
static pfIoTHubTransport_ProcessItem                IoTHubTransportMqtt_ProcessItem;
static pfIoTHubTransport_Subscribe_InputQueue       IoTHubTransportMqtt_Subscribe_InputQueue;
static pfIoTHubTransport_Unsubscribe_InputQueue     IoTHubTransportMqtt_Unsubscribe_InputQueue;
static pfIoTHubTransport_SetCallbackContext         IoTHubTransportMqtt_SetCallbackContext;
static pfIoTHubTransport_GetSupportedPlatformInfo   IotHubTransportMqtt_GetSupportedPlatformInfo;

static TRANSPORT_LL_HANDLE my_IoTHubTransport_MQTT_Common_Create(const IOTHUBTRANSPORT_CONFIG* config, MQTT_GET_IO_TRANSPORT get_io_transport, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    (void)config;
    (void)cb_info;
    (void)ctx;
    g_get_io_transport = get_io_transport;
    return TEST_TRANSPORT_HANDLE;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

static int copy_string(char** destination, const char* source)
{
    int result;

    if (source == NULL)
    {
        *destination = NULL;
        result = 0;
    }
    else
    {
        size_t length = strlen(source);
        *destination = (char*)real_malloc(length + 1);
        if (*destination == NULL)
        {
            result = __LINE__;
        }
        else
        {
            (void)memcpy(*destination, source, length + 1);
            result = 0;
        }
    }

    return result;
}

static int umocktypes_copy_TLSIO_CONFIG_ptr(TLSIO_CONFIG** destination, const TLSIO_CONFIG** source)
{
    int result;

    if (*source == NULL)
    {
        *destination = NULL;
        result = 0;
    }
    else
    {
        *destination = (TLSIO_CONFIG*)real_malloc(sizeof(TLSIO_CONFIG));
        if (*destination == NULL)
        {
            result = __LINE__;
        }
        else
        {
            if (copy_string((char**)&((*destination)->hostname), (*source)->hostname) != 0)
            {
                real_free(*destination);
                result = __LINE__;
            }
            else
            {
                (*destination)->port = (*source)->port;
                (*destination)->underlying_io_interface = (*source)->underlying_io_interface;
                (*destination)->underlying_io_parameters = (*source)->underlying_io_parameters;
                if (((*destination)->underlying_io_interface != NULL) && (umocktypes_copy("HTTP_PROXY_IO_CONFIG*", &((*destination)->underlying_io_parameters), &((*source)->underlying_io_parameters)) != 0))
                {
                    real_free((char*)((*destination)->hostname));
                    real_free(*destination);
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }

    return result;
}

static void umocktypes_free_TLSIO_CONFIG_ptr(TLSIO_CONFIG** value)
{
    if (*value != NULL)
    {
        if ((*value)->underlying_io_interface != NULL)
        {
            umocktypes_free("HTTP_PROXY_IO_CONFIG*", &((*value)->underlying_io_parameters));
        }

        real_free((void*)(*value)->hostname);
        real_free(*value);
    }
}

static char* umocktypes_stringify_TLSIO_CONFIG_ptr(const TLSIO_CONFIG** value)
{
    char* result;
    if (*value == NULL)
    {
        result = (char*)real_malloc(5);
        if (result != NULL)
        {
            (void)memcpy(result, "NULL", 5);
        }
    }
    else
    {
        int length = snprintf(NULL, 0, "{ %p, %p, %s, %d }",
            (*value)->underlying_io_interface,
            (*value)->underlying_io_parameters,
            (*value)->hostname,
            (*value)->port);
        if (length < 0)
        {
            result = NULL;
        }
        else
        {
            result = (char*)real_malloc(length + 1);
            (void)snprintf(result, length + 1, "{ %p, %p, %s, %d }",
                (*value)->underlying_io_interface,
                (*value)->underlying_io_parameters,
                (*value)->hostname,
                (*value)->port);
        }
    }

    return result;
}

static int umocktypes_are_equal_TLSIO_CONFIG_ptr(TLSIO_CONFIG** left, TLSIO_CONFIG** right)
{
    int result;

    if (*left == *right)
    {
        result = 1;
    }
    else
    {
        if ((*left == NULL) ||
            (*right == NULL))
        {
            result = 0;
        }
        else
        {
            if (((*left)->port != (*right)->port) ||
                ((*left)->underlying_io_interface != (*right)->underlying_io_interface))
            {
                result = 0;
            }
            else
            {
                if ((strcmp((*left)->hostname, (*right)->hostname) != 0) ||
                    (((*left)->underlying_io_interface != NULL) && (umocktypes_are_equal("HTTP_PROXY_IO_CONFIG*", &((*left)->underlying_io_parameters), &((*right)->underlying_io_parameters)) != 1)))
                {
                    result = 0;
                }
                else
                {
                    result = 1;
                }
            }
        }
    }

    return result;
}

BEGIN_TEST_SUITE(iothubtransportmqtt_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    umock_c_init(on_umock_c_error);

    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(MQTT_GET_IO_TRANSPORT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, int);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_TYPE(TLSIO_CONFIG*, TLSIO_CONFIG_ptr);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubTransport_MQTT_Common_Create, my_IoTHubTransport_MQTT_Common_Create);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_SendMessageDisposition, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_Subscribe, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_GetSendStatus, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_Register, TEST_DEVICE_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_GetHostname, (STRING_HANDLE)0x1182);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_ProcessItem, IOTHUB_PROCESS_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_SetRetryPolicy, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_Subscribe_InputQueue, 0);

    REGISTER_GLOBAL_MOCK_RETURN(xio_create, TEST_XIO_HANDLE);

    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_TLSIO_INTERFACE_DESCRIPTION);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(platform_get_default_tlsio, NULL);

    IoTHubTransportMqtt_SendMessageDisposition = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_SendMessageDisposition;
    IoTHubTransportMqtt_GetHostname = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_GetHostname;
    IoTHubTransportMqtt_SetOption = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_SetOption;
    IoTHubTransportMqtt_Create = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Create;
    IoTHubTransportMqtt_Destroy = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Destroy;
    IoTHubTransportMqtt_Register = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Register;
    IoTHubTransportMqtt_Unregister = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Unregister;
    IoTHubTransportMqtt_Subscribe = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Subscribe;
    IoTHubTransportMqtt_Unsubscribe = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Unsubscribe;
    IoTHubTransportMqtt_DoWork = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_DoWork;
    IoTHubTransportMqtt_SetRetryPolicy = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_SetRetryPolicy;
    IoTHubTransportMqtt_GetSendStatus = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_GetSendStatus;
    IoTHubTransportMqtt_Subscribe_DeviceTwin = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Subscribe_DeviceTwin;
    IoTHubTransportMqtt_Unsubscribe_DeviceTwin = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Unsubscribe_DeviceTwin;
    IoTHubTransportMqtt_GetTwinAsync = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_GetTwinAsync;
    IoTHubTransportMqtt_Subscribe_DeviceMethod = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Subscribe_DeviceMethod;
    IoTHubTransportMqtt_Unsubscribe_DeviceMethod = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Unsubscribe_DeviceMethod;
    IoTHubTransportMqtt_DeviceMethod_Response = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_DeviceMethod_Response;
    IoTHubTransportMqtt_ProcessItem = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_ProcessItem;
    IoTHubTransportMqtt_Subscribe_InputQueue = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Subscribe_InputQueue;
    IoTHubTransportMqtt_Unsubscribe_InputQueue = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_Unsubscribe_InputQueue;
    IoTHubTransportMqtt_SetCallbackContext = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_SetCallbackContext;
    IotHubTransportMqtt_GetSupportedPlatformInfo = ((TRANSPORT_PROVIDER*)MQTT_Protocol())->IoTHubTransport_GetSupportedPlatformInfo;
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

static void reset_test_data()
{
    g_callbackCtx = NULL;
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    TEST_MUTEX_ACQUIRE(test_serialize_mutex);
    reset_test_data();

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

static void SetupIothubTransportConfig(IOTHUBTRANSPORT_CONFIG* config, const char* deviceId, const char* deviceKey, const char* iotHubName,
    const char* iotHubSuffix, const char* protocolGatewayHostName)
{
    g_iothubClientConfig.protocol = MQTT_Protocol;
    g_iothubClientConfig.deviceId = deviceId;
    g_iothubClientConfig.deviceKey = deviceKey;
    g_iothubClientConfig.deviceSasToken = NULL;
    g_iothubClientConfig.iotHubName = iotHubName;
    g_iothubClientConfig.iotHubSuffix = iotHubSuffix;
    g_iothubClientConfig.protocolGatewayHostName = protocolGatewayHostName;
    config->waitingToSend = &g_waitingToSend;
    config->upperConfig = &g_iothubClientConfig;
}

TEST_FUNCTION(IoTHubTransportMqtt_Create_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Create(&config, IGNORED_PTR_ARG, g_transport_cb_info, NULL));

    // act
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_getSocketsIOTransport_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    TLSIO_CONFIG tlsio_config;
    XIO_HANDLE xioTest;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    tlsio_config.hostname = TEST_STRING_VALUE;
    tlsio_config.port = 8883;
    tlsio_config.underlying_io_interface = NULL;
    tlsio_config.underlying_io_parameters = NULL;

    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, &tlsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(TLSIO_CONFIG*));

    ASSERT_IS_NOT_NULL(g_get_io_transport);

    // act
    xioTest = g_get_io_transport(TEST_STRING_VALUE, NULL);

    // assert
    ASSERT_IS_NOT_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_getSocketsIOTransport_ignores_proxy_options)
{
    // arrange
    MQTT_TRANSPORT_PROXY_OPTIONS mqtt_proxy_options;
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    TLSIO_CONFIG tlsio_config;
    XIO_HANDLE xioTest;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    tlsio_config.hostname = TEST_STRING_VALUE;
    tlsio_config.port = 8883;
    tlsio_config.underlying_io_interface = NULL;
    tlsio_config.underlying_io_parameters = NULL;

    mqtt_proxy_options.host_address = "some_host";
    mqtt_proxy_options.port = 444;
    mqtt_proxy_options.username = "me";
    mqtt_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, &tlsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(TLSIO_CONFIG*));

    ASSERT_IS_NOT_NULL(g_get_io_transport);

    // act
    xioTest = g_get_io_transport(TEST_STRING_VALUE, &mqtt_proxy_options);

    // assert
    ASSERT_IS_NOT_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubTransportMqtt_getSocketsIOTransport_platform_get_default_tlsio_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    XIO_HANDLE xioTest;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(platform_get_default_tlsio()).SetReturn(NULL);
    ASSERT_IS_NOT_NULL(g_get_io_transport);

    // act
    xioTest = g_get_io_transport(TEST_STRING_VALUE, NULL);

    // assert
    ASSERT_IS_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubTransportMqtt_Destroy_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Destroy(handle));

    // act
    IoTHubTransportMqtt_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_Subscribe_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Subscribe(handle));

    // act
    int result = IoTHubTransportMqtt_Subscribe(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_SetRetryPolicy_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS));

    // act
    int result = IoTHubTransportMqtt_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup

}

TEST_FUNCTION(IoTHubTransportMqtt_Subscribe_DeviceTwin_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle));

    int result = IoTHubTransportMqtt_Subscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_Unsubscribe_DeviceTwin_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(handle));

    // act
    IoTHubTransportMqtt_Unsubscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_GetTwinAsync_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_GetTwinAsync(handle, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportMqtt_GetTwinAsync(handle, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_Subscribe_Method_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle));

    int result = IoTHubTransportMqtt_Subscribe_DeviceMethod(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_Unsubscribe_Method_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod(handle));

    // act
    IoTHubTransportMqtt_Unsubscribe_DeviceMethod(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_Unsubscribe_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Unsubscribe(handle));

    IoTHubTransportMqtt_Unsubscribe(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_DoWork_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_DoWork(handle));

    // act
    IoTHubTransportMqtt_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_GetSendStatus_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS iotHubClientStatus;

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_GetSendStatus(handle, &iotHubClientStatus));

    IOTHUB_CLIENT_RESULT result = IoTHubTransportMqtt_GetSendStatus(handle, &iotHubClientStatus);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_SetOption_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_SetOption(handle, TEST_OPTION_NAME, TEST_OPTION_VALUE));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportMqtt_SetOption(handle, TEST_OPTION_NAME, TEST_OPTION_VALUE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_Register_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = NULL;
    deviceConfig.deviceSasToken = NULL;

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend));

    // act
    IOTHUB_DEVICE_HANDLE result = IoTHubTransportMqtt_Register(handle, &deviceConfig, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_Unregister_success)
{
    // arrange

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Unregister(TEST_DEVICE_HANDLE));

    // act
    IoTHubTransportMqtt_Unregister(TEST_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_DeviceMethod_Response_success)
{
    // arrange

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_DeviceMethod_Response(TEST_DEVICE_HANDLE, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, 1, TEST_DEVICE_STATUS_CODE));

    int result = IoTHubTransportMqtt_DeviceMethod_Response(TEST_DEVICE_HANDLE, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, 1, TEST_DEVICE_STATUS_CODE);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_GetHostname_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_GetHostname(handle));

    // act
    STRING_HANDLE result = IoTHubTransportMqtt_GetHostname(handle);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_SendMessageDisposition_success)
{
    // arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_SendMessageDisposition(TEST_DEVICE_HANDLE, IGNORED_PTR_ARG, IOTHUBMESSAGE_ACCEPTED))
        .IgnoreAllArguments();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportMqtt_SendMessageDisposition(TEST_DEVICE_HANDLE, NULL, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Subscribe_InputQueue_success)
{
    // arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Subscribe_InputQueue(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    // act
    int result = IoTHubTransportMqtt_Subscribe_InputQueue(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransport_MQTT_Unsubscribe_InputQueue_success)
{
    // arrange

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Unsubscribe_InputQueue(TEST_DEVICE_HANDLE));

    // act
    IoTHubTransportMqtt_Unsubscribe_InputQueue(TEST_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_SetCallbackContext_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_SetCallbackContext(IGNORED_PTR_ARG, g_transport_cb_ctx));

    // act
    int result = IoTHubTransportMqtt_SetCallbackContext(handle, g_transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_GetSupportedPlatformInfo)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);

    PLATFORM_INFO_OPTION expected_info = PLATFORM_INFO_OPTION_RETRIEVE_SQM;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_info(&expected_info, sizeof(expected_info))
        .SetReturn(0);

    // act
    PLATFORM_INFO_OPTION info;
    int result = IotHubTransportMqtt_GetSupportedPlatformInfo(handle, &info);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(int, info, PLATFORM_INFO_OPTION_RETRIEVE_SQM);

    // cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_GetSupportedPlatformInfo_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    PLATFORM_INFO_OPTION info;
    int result = IotHubTransportMqtt_GetSupportedPlatformInfo(NULL, &info);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_GetSupportedPlatformInfo_NULL_info)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_Create(&config, g_transport_cb_info, NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    int result = IotHubTransportMqtt_GetSupportedPlatformInfo(handle, NULL);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

END_TEST_SUITE(iothubtransportmqtt_ut)
