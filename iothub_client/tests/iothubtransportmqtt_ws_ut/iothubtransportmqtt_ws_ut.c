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
#include "umock_c/umocktypes_stdint.h"

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

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/wsio.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "internal/iothubtransport_mqtt_common.h"
#include "internal/iothubtransport.h"

#undef ENABLE_MOCKS

#include "iothubtransportmqtt_websockets.h"

static const char* TEST_STRING_VALUE = "FULLY_QUALIFIED_HOSTNAME";
static const char* TEST_DEVICE_ID = "thisIsDeviceID";
static const char* TEST_DEVICE_KEY = "thisIsDeviceKey";
static const char* TEST_DEVICE_SAS = "thisIsDeviceSasToken";
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

static TRANSPORT_CALLBACKS_INFO* transport_cb_info = (TRANSPORT_CALLBACKS_INFO*)0x227733;

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

static int umocktypes_copy_WSIO_CONFIG_ptr(WSIO_CONFIG** destination, const WSIO_CONFIG** source)
{
    int result;

    if (*source == NULL)
    {
        *destination = NULL;
        result = 0;
    }
    else
    {
        *destination = (WSIO_CONFIG*)real_malloc(sizeof(WSIO_CONFIG));
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
            else if (copy_string((char**)&((*destination)->resource_name), (*source)->resource_name) != 0)
            {
                real_free((char*)(*destination)->hostname);
                real_free(*destination);
                result = __LINE__;
            }
            else if (copy_string((char**)&((*destination)->protocol), (*source)->protocol) != 0)
            {
                real_free((char*)(*destination)->resource_name);
                real_free((char*)(*destination)->hostname);
                real_free(*destination);
                result = __LINE__;
            }
            else
            {
                (*destination)->port = (*source)->port;
                (*destination)->underlying_io_interface = (*source)->underlying_io_interface;
                if (umocktypes_copy("TLSIO_CONFIG*", &((*destination)->underlying_io_parameters), &((*source)->underlying_io_parameters)) != 0)
                {
                    real_free((char*)(*destination)->resource_name);
                    real_free((char*)(*destination)->hostname);
                    real_free((char*)(*destination)->protocol);
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

static void umocktypes_free_WSIO_CONFIG_ptr(WSIO_CONFIG** value)
{
    if (*value != NULL)
    {
        umocktypes_free("TLSIO_CONFIG*", &((*value)->underlying_io_parameters));
        real_free((void*)(*value)->hostname);
        real_free((void*)(*value)->resource_name);
        real_free((void*)(*value)->protocol);
        real_free(*value);
    }
}

static char* umocktypes_stringify_WSIO_CONFIG_ptr(const WSIO_CONFIG** value)
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
        int length = snprintf(NULL, 0, "{ %p, %p, %s, %d, %s, %s }",
            (*value)->underlying_io_interface,
            (*value)->underlying_io_parameters,
            (*value)->hostname,
            (*value)->port,
            (*value)->resource_name,
            (*value)->protocol);
        if (length < 0)
        {
            result = NULL;
        }
        else
        {
            result = (char*)real_malloc(length + 1);
            (void)snprintf(result, length + 1, "{ %p, %p, %s, %d, %s, %s }",
                (*value)->underlying_io_interface,
                (*value)->underlying_io_parameters,
                (*value)->hostname,
                (*value)->port,
                (*value)->resource_name,
                (*value)->protocol);
        }
    }

    return result;
}

static int umocktypes_are_equal_WSIO_CONFIG_ptr(WSIO_CONFIG** left, WSIO_CONFIG** right)
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
                    (strcmp((*left)->protocol, (*right)->protocol) != 0) ||
                    (strcmp((*left)->resource_name, (*right)->resource_name) != 0) ||
                    (umocktypes_are_equal("TLSIO_CONFIG*", &((*left)->underlying_io_parameters), &((*right)->underlying_io_parameters)) != 1))
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

static int umocktypes_copy_HTTP_PROXY_IO_CONFIG_ptr(HTTP_PROXY_IO_CONFIG** destination, const HTTP_PROXY_IO_CONFIG** source)
{
    int result;

    if (*source == NULL)
    {
        *destination = NULL;
        result = 0;
    }
    else
    {
        *destination = (HTTP_PROXY_IO_CONFIG*)real_malloc(sizeof(HTTP_PROXY_IO_CONFIG));
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
            else if (copy_string((char**)&((*destination)->proxy_hostname), (*source)->proxy_hostname) != 0)
            {
                real_free((char*)((*destination)->hostname));
                real_free(*destination);
                result = __LINE__;
            }
            else if (copy_string((char**)&((*destination)->username), (*source)->username) != 0)
            {
                real_free((char*)((*destination)->proxy_hostname));
                real_free((char*)((*destination)->hostname));
                real_free(*destination);
                result = __LINE__;
            }
            else if (copy_string((char**)&((*destination)->password), (*source)->password) != 0)
            {
                real_free((char*)((*destination)->username));
                real_free((char*)((*destination)->proxy_hostname));
                real_free((char*)((*destination)->hostname));
                real_free(*destination);
                result = __LINE__;
            }
            else
            {
                (*destination)->port = (*source)->port;
                (*destination)->proxy_port = (*source)->proxy_port;
                result = 0;
            }
        }
    }

    return result;
}

static void umocktypes_free_HTTP_PROXY_IO_CONFIG_ptr(HTTP_PROXY_IO_CONFIG** value)
{
    if (*value != NULL)
    {
        real_free((void*)(*value)->hostname);
        real_free((void*)(*value)->proxy_hostname);
        real_free((void*)(*value)->username);
        real_free((void*)(*value)->password);
        real_free(*value);
    }
}

static char* umocktypes_stringify_HTTP_PROXY_IO_CONFIG_ptr(const HTTP_PROXY_IO_CONFIG** value)
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
        int length = snprintf(NULL, 0, "{ %s, %d, %s, %d, %s, %s }",
            (*value)->hostname,
            (*value)->port,
            (*value)->proxy_hostname,
            (*value)->proxy_port,
            (*value)->username,
            (*value)->password);
        if (length < 0)
        {
            result = NULL;
        }
        else
        {
            result = (char*)real_malloc(length + 1);
            (void)snprintf(result, length + 1, "{ %s, %d, %s, %d, %s, %s }",
                (*value)->hostname,
                (*value)->port,
                (*value)->proxy_hostname,
                (*value)->proxy_port,
                (*value)->username,
                (*value)->password);
        }
    }

    return result;
}

static int umocktypes_are_equal_HTTP_PROXY_IO_CONFIG_ptr(HTTP_PROXY_IO_CONFIG** left, HTTP_PROXY_IO_CONFIG** right)
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
                ((*left)->proxy_port != (*right)->proxy_port))
            {
                result = 0;
            }
            else
            {
                if ((strcmp((*left)->hostname, (*right)->hostname) != 0) ||
                    (strcmp((*left)->proxy_hostname, (*right)->proxy_hostname) != 0) ||
                    (strcmp((*left)->username, (*right)->username) != 0) ||
                    (strcmp((*left)->password, (*right)->password) != 0))
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

//Callbacks for Testing
static void* g_callbackCtx;

static pfIoTHubTransport_GetHostname                IoTHubTransportMqtt_WS_GetHostname;
static pfIoTHubTransport_SetOption                  IoTHubTransportMqtt_WS_SetOption;
static pfIoTHubTransport_Create                     IoTHubTransportMqtt_WS_Create;
static pfIoTHubTransport_Destroy                    IoTHubTransportMqtt_WS_Destroy;
static pfIotHubTransport_Register                   IoTHubTransportMqtt_WS_Register;
static pfIotHubTransport_Unregister                 IoTHubTransportMqtt_WS_Unregister;
static pfIoTHubTransport_Subscribe                  IoTHubTransportMqtt_WS_Subscribe;
static pfIoTHubTransport_Unsubscribe                IoTHubTransportMqtt_WS_Unsubscribe;
static pfIoTHubTransport_DoWork                     IoTHubTransportMqtt_WS_DoWork;
static pfIoTHubTransport_SetRetryPolicy             IoTHubTransportMqtt_WS_SetRetryPolicy;
static pfIoTHubTransport_GetSendStatus              IoTHubTransportMqtt_WS_GetSendStatus;
static pfIoTHubTransport_Subscribe_DeviceTwin       IoTHubTransportMqtt_WS_Subscribe_DeviceTwin;
static pfIoTHubTransport_Unsubscribe_DeviceTwin     IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin;
static pfIoTHubTransport_GetTwinAsync         IoTHubTransportMqtt_WS_GetTwinAsync;
static pfIoTHubTransport_Subscribe_DeviceMethod     IoTHubTransportMqtt_WS_Subscribe_DeviceMethod;
static pfIoTHubTransport_Unsubscribe_DeviceMethod   IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod;
static pfIoTHubTransport_ProcessItem                IoTHubTransportMqtt_WS_ProcessItem;
static pfIoTHubTransport_SetCallbackContext         IotHubTransportMqtt_WS_SetCallbackContext;
static pfIoTHubTransport_GetSupportedPlatformInfo   IotHubTransportMqtt_WS_GetSupportedPlatformInfo;

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

BEGIN_TEST_SUITE(iothubtransportmqtt_ws_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    //int result;

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    umock_c_init(on_umock_c_error);

    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MQTT_GET_IO_TRANSPORT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);

    REGISTER_TYPE(WSIO_CONFIG*, WSIO_CONFIG_ptr);
    REGISTER_TYPE(TLSIO_CONFIG*, TLSIO_CONFIG_ptr);
    REGISTER_TYPE(HTTP_PROXY_IO_CONFIG*, HTTP_PROXY_IO_CONFIG_ptr);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubTransport_MQTT_Common_Create, my_IoTHubTransport_MQTT_Common_Create);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_Subscribe, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_GetSendStatus, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_Register, TEST_DEVICE_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_GetHostname, (STRING_HANDLE)0x1182);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_MQTT_Common_SetRetryPolicy, 0);

    REGISTER_GLOBAL_MOCK_RETURN(xio_create, TEST_XIO_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(wsio_get_interface_description, TEST_WSIO_INTERFACE_DESCRIPTION);
    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_TLSIO_INTERFACE_DESCRIPTION);
    REGISTER_GLOBAL_MOCK_RETURN(http_proxy_io_get_interface_description, TEST_HTTP_PROXY_IO_INTERFACE_DESCRIPTION);

    /* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_011: [ This function shall return a pointer to a structure of type TRANSPORT_PROVIDER having the following values for its fields:

    IoTHubTransport_GetHostname = IoTHubTransportMqtt_WS_GetHostname
    IoTHubTransport_Create = IoTHubTransportMqtt_WS_Create
    IoTHubTransport_Destroy = IoTHubTransportMqtt_WS_Destroy
    IoTHubTransport_Subscribe = IoTHubTransportMqtt_WS_Subscribe
    IoTHubTransport_Unsubscribe = IoTHubTransportMqtt_WS_Unsubscribe
    IoTHubTransport_DoWork = IoTHubTransportMqtt_WS_DoWork
    IoTHubTransport_SetOption = IoTHubTransportMqtt_WS_SetOption
    IoTHubTransport_Subscribe_DeviceTwin = IoTHubTransportMqtt_WS_Subscribe_DeviceTwin
    IoTHubTransport_Unsubscribe_DeviceTwin = IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin
    IoTHubTransport_Subscribe_DeviceMethod = IoTHubTransportMqtt_WS_Subscribe_DeviceMethod
    IoTHubTransport_Unsubscribe_DeviceMethod = IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod
    IoTHubTransport_ProcessItem = IoTHubTransportMqtt_WS_ProcessItem ] */
    IoTHubTransportMqtt_WS_GetHostname = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_GetHostname;
    IoTHubTransportMqtt_WS_SetOption = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_SetOption;
    IoTHubTransportMqtt_WS_Create = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Create;
    IoTHubTransportMqtt_WS_Destroy = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Destroy;
    IoTHubTransportMqtt_WS_Register = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Register;
    IoTHubTransportMqtt_WS_Unregister = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Unregister;
    IoTHubTransportMqtt_WS_Subscribe = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Subscribe;
    IoTHubTransportMqtt_WS_Unsubscribe = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Unsubscribe;
    IoTHubTransportMqtt_WS_DoWork = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_DoWork;
    IoTHubTransportMqtt_WS_SetRetryPolicy = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_SetRetryPolicy;
    IoTHubTransportMqtt_WS_GetSendStatus = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_GetSendStatus;
    IoTHubTransportMqtt_WS_Subscribe_DeviceTwin = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Subscribe_DeviceTwin;
    IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Unsubscribe_DeviceTwin;
    IoTHubTransportMqtt_WS_GetTwinAsync = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_GetTwinAsync;
    IoTHubTransportMqtt_WS_Subscribe_DeviceMethod = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Subscribe_DeviceMethod;
    IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_Unsubscribe_DeviceMethod;
    IoTHubTransportMqtt_WS_ProcessItem = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_ProcessItem;
    IotHubTransportMqtt_WS_SetCallbackContext = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_SetCallbackContext;
    IotHubTransportMqtt_WS_GetSupportedPlatformInfo = ((TRANSPORT_PROVIDER*)MQTT_WebSocket_Protocol())->IoTHubTransport_GetSupportedPlatformInfo;
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

static void reset_test_data()
{
    g_callbackCtx = NULL;
    memset(&g_waitingToSend, 0, sizeof(g_waitingToSend));
    memset(&g_get_io_transport, 0, sizeof(g_get_io_transport));
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
    g_iothubClientConfig.protocol = MQTT_WebSocket_Protocol;
    g_iothubClientConfig.deviceId = deviceId;
    g_iothubClientConfig.deviceKey = deviceKey;
    g_iothubClientConfig.deviceSasToken = NULL;
    g_iothubClientConfig.iotHubName = iotHubName;
    g_iothubClientConfig.iotHubSuffix = iotHubSuffix;
    g_iothubClientConfig.protocolGatewayHostName = protocolGatewayHostName;
    config->waitingToSend = &g_waitingToSend;
    config->upperConfig = &g_iothubClientConfig;
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_001: [ IoTHubTransportMqtt_WS_Create shall create a TRANSPORT_LL_HANDLE by calling into the IoTHubMqttAbstract_Create function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_Create_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Create(&config, IGNORED_PTR_ARG, transport_cb_info, NULL));

    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_012: [ `getIoTransportProvider` shall return the `XIO_HANDLE` returned by `xio_create`. ] */
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_002: [ `getIoTransportProvider` shall call `xio_create` while passing the WebSocket IO interface description to it and the WebSocket configuration as a WSIO_CONFIG structure, filled as below ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_001: [ `getIoTransportProvider` shall obtain the WebSocket IO interface handle by calling `wsio_get_interface_description`. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_003: [ - `hostname` shall be set to `fully_qualified_name`. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_004: [ - `port` shall be set to 443. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_005: [ - `protocol` shall be set to `MQTT`. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_006: [ - `resource_name` shall be set to `/$iothub/websocket`. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_007: [ - `underlying_io_interface` shall be set to the TLS IO interface description. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_008: [ - `underlying_io_parameters` shall be set to the TLS IO arguments. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_009: [ `getIoTransportProvider` shall obtain the TLS IO interface handle by calling `platform_get_default_tlsio`. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_010: [ The TLS IO parameters shall be a `TLSIO_CONFIG` structure filled as below: ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_011: [ - `hostname` shall be set to `fully_qualified_name`. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_012: [ - `port` shall be set to 443. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_013: [ - If `mqtt_transport_proxy_options` is NULL, `underlying_io_interface` shall be set to NULL ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_014: [ - If `mqtt_transport_proxy_options` is NULL `underlying_io_parameters` shall be set to NULL. ]*/
TEST_FUNCTION(IoTHubTransportMqtt_WS_getWebSocketsIOTransport_with_NULL_uses_a_socket_IO)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    XIO_HANDLE xioTest;
    WSIO_CONFIG wsio_config;
    TLSIO_CONFIG tlsio_config;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    tlsio_config.hostname = TEST_STRING_VALUE;
    tlsio_config.port = 443;
    tlsio_config.underlying_io_interface = NULL;
    tlsio_config.underlying_io_parameters = NULL;

    wsio_config.hostname = TEST_STRING_VALUE;
    wsio_config.port = 443;
    wsio_config.protocol = "MQTT";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = TEST_TLSIO_INTERFACE_DESCRIPTION;
    wsio_config.underlying_io_parameters = &tlsio_config;

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    ASSERT_IS_NOT_NULL(g_get_io_transport);

    // act
    xioTest = g_get_io_transport(TEST_STRING_VALUE, NULL);

    // assert
    ASSERT_IS_NOT_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_015: [ - If `mqtt_transport_proxy_options` is not NULL, `underlying_io_interface` shall be set to the HTTP proxy IO interface description. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_016: [ - If `mqtt_transport_proxy_options` is not NULL `underlying_io_parameters` shall be set to the HTTP proxy IO arguments. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_022: [ `getIoTransportProvider` shall obtain the HTTP proxy IO interface handle by calling `http_proxy_io_get_interface_description`. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_023: [ The HTTP proxy IO arguments shall be an `HTTP_PROXY_IO_CONFIG` structure, filled as below: ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_024: [ - `hostname` shall be set to `fully_qualified_name`. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_026: [ - `proxy_hostname`, `proxy_port`, `username` and `password` shall be copied from the `mqtt_transport_proxy_options` argument. ]*/
/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_025: [ - `port` shall be set to 443. ]*/
TEST_FUNCTION(IoTHubTransportMqtt_WS_getWebSocketsIOTransport_with_proxy_settings_uses_the_proxy_settings)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    MQTT_TRANSPORT_PROXY_OPTIONS mqtt_proxy_options;
    WSIO_CONFIG wsio_config;
    TLSIO_CONFIG tlsio_config;
    HTTP_PROXY_IO_CONFIG http_proxy_io_config;
    XIO_HANDLE xioTest;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    http_proxy_io_config.hostname = TEST_STRING_VALUE;
    http_proxy_io_config.port = 443;
    http_proxy_io_config.proxy_hostname = "some_host";
    http_proxy_io_config.proxy_port = 444;
    http_proxy_io_config.username = "me";
    http_proxy_io_config.password = "shhhh";

    tlsio_config.hostname = TEST_STRING_VALUE;
    tlsio_config.port = 443;
    tlsio_config.underlying_io_interface = TEST_HTTP_PROXY_IO_INTERFACE_DESCRIPTION;
    tlsio_config.underlying_io_parameters = &http_proxy_io_config;

    wsio_config.hostname = TEST_STRING_VALUE;
    wsio_config.port = 443;
    wsio_config.protocol = "MQTT";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = TEST_TLSIO_INTERFACE_DESCRIPTION;
    wsio_config.underlying_io_parameters = &tlsio_config;

    mqtt_proxy_options.host_address = "some_host";
    mqtt_proxy_options.port = 444;
    mqtt_proxy_options.username = "me";
    mqtt_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(http_proxy_io_get_interface_description());
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    ASSERT_IS_NOT_NULL(g_get_io_transport);

    // act
    xioTest = g_get_io_transport(TEST_STRING_VALUE, &mqtt_proxy_options);

    // assert
    ASSERT_IS_NOT_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_028: [ If `http_proxy_io_get_interface_description` returns NULL, NULL shall be set in the TLS IO parameters structure for the interface description and parameters. ]*/
TEST_FUNCTION(when_socket_io_interface_is_NULL_getWebSocketsIOTransport_with_proxy_settings_passes_down_NULL_as_http_proxy_io_interface)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    MQTT_TRANSPORT_PROXY_OPTIONS mqtt_proxy_options;
    WSIO_CONFIG wsio_config;
    TLSIO_CONFIG tlsio_config;
    HTTP_PROXY_IO_CONFIG http_proxy_io_config;
    XIO_HANDLE xioTest;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    http_proxy_io_config.hostname = TEST_STRING_VALUE;
    http_proxy_io_config.port = 443;
    http_proxy_io_config.proxy_hostname = "some_host";
    http_proxy_io_config.proxy_port = 444;
    http_proxy_io_config.username = "me";
    http_proxy_io_config.password = "shhhh";

    tlsio_config.hostname = TEST_STRING_VALUE;
    tlsio_config.port = 443;
    tlsio_config.underlying_io_interface = NULL;
    tlsio_config.underlying_io_parameters = &http_proxy_io_config;

    wsio_config.hostname = TEST_STRING_VALUE;
    wsio_config.port = 443;
    wsio_config.protocol = "MQTT";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = TEST_TLSIO_INTERFACE_DESCRIPTION;
    wsio_config.underlying_io_parameters = &tlsio_config;

    mqtt_proxy_options.host_address = "some_host";
    mqtt_proxy_options.port = 444;
    mqtt_proxy_options.username = "me";
    mqtt_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(http_proxy_io_get_interface_description())
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    ASSERT_IS_NOT_NULL(g_get_io_transport);

    // act
    xioTest = g_get_io_transport(TEST_STRING_VALUE, &mqtt_proxy_options);

    // assert
    ASSERT_IS_NOT_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_029: [ If `platform_get_default_tlsio` returns NULL, NULL shall be set in the WebSocket IO parameters structure for the interface description and parameters. ]*/
TEST_FUNCTION(when_tlsio_interface_is_NULL_getWebSocketsIOTransport_with_proxy_settings_passes_down_NULL_as_tlsio_interface)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    MQTT_TRANSPORT_PROXY_OPTIONS mqtt_proxy_options;
    WSIO_CONFIG wsio_config;
    XIO_HANDLE xioTest;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    wsio_config.hostname = TEST_STRING_VALUE;
    wsio_config.port = 443;
    wsio_config.protocol = "MQTT";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = NULL;
    wsio_config.underlying_io_parameters = NULL;

    mqtt_proxy_options.host_address = "some_host";
    mqtt_proxy_options.port = 444;
    mqtt_proxy_options.username = "me";
    mqtt_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio())
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    ASSERT_IS_NOT_NULL(g_get_io_transport);

    // act
    xioTest = g_get_io_transport(TEST_STRING_VALUE, &mqtt_proxy_options);

    // assert
    ASSERT_IS_NOT_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_01_029: [ If `platform_get_default_tlsio` returns NULL, NULL shall be set in the WebSocket IO parameters structure for the interface description and parameters. ]*/
TEST_FUNCTION(when_tlsio_interface_is_NULL_getWebSocketsIOTransport_with_NULL_proxy_settings_passes_down_NULL_as_tlsio_interface)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    WSIO_CONFIG wsio_config;
    XIO_HANDLE xioTest;
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    wsio_config.hostname = TEST_STRING_VALUE;
    wsio_config.port = 443;
    wsio_config.protocol = "MQTT";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = NULL;
    wsio_config.underlying_io_parameters = NULL;

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio())
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    ASSERT_IS_NOT_NULL(g_get_io_transport);

    // act
    xioTest = g_get_io_transport(TEST_STRING_VALUE, NULL);

    // assert
    ASSERT_IS_NOT_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_013: [ If `wsio_get_interface_description` returns NULL `getIoTransportProvider` shall return NULL. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_getWebSocketsIOTransport_wsio_get_interface_description_NULL_fail)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    (void)IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(wsio_get_interface_description()).SetReturn(NULL);

    ASSERT_IS_NOT_NULL(g_get_io_transport);
    XIO_HANDLE xioTest = g_get_io_transport(TEST_STRING_VALUE, NULL);

    // assert
    ASSERT_IS_NULL(xioTest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_002: [ IoTHubTransportMqtt_WS_Destroy shall destroy the TRANSPORT_LL_HANDLE by calling into the IoTHubMqttAbstract_Destroy function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_Destroy_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Destroy(handle));

    IoTHubTransportMqtt_WS_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_005: [ IoTHubTransportMqtt_WS_Subscribe shall subscribe the TRANSPORT_LL_HANDLE by calling into the IoTHubMqttAbstract_Subscribe function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_Subscribe_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Subscribe(handle));

    int result = IoTHubTransportMqtt_WS_Subscribe(handle);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_25_012: [** IoTHubTransportMqtt_WS_SetRetryPolicy shall call into the IoTHubMqttAbstract_SetRetryPolicy function.]*/
TEST_FUNCTION(IoTHubTransportMqtt_WS_SetRetryPolicy_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS));

    int result = IoTHubTransportMqtt_WS_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_006: [ IoTHubTransportMqtt_WS_Unsubscribe shall unsubscribe the TRANSPORT_LL_HANDLE by calling into the IoTHubMqttAbstract_Unsubscribe function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_Unsubscribe_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Unsubscribe(handle));

    IoTHubTransportMqtt_WS_Unsubscribe(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_007: [ IoTHubTransportMqtt_WS_DoWork shall call into the IoTHubMqttAbstract_DoWork function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_DoWork_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_DoWork(handle));

    IoTHubTransportMqtt_WS_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_008: [ IoTHubTransportMqtt_WS_GetSendStatus shall get the send status by calling into the IoTHubMqttAbstract_GetSendStatus function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_GetSendStatus_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS iotHubClientStatus;

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_GetSendStatus(handle, &iotHubClientStatus));

    IOTHUB_CLIENT_RESULT result = IoTHubTransportMqtt_WS_GetSendStatus(handle, &iotHubClientStatus);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_009: [ IoTHubTransportMqtt_WS_SetOption shall set the options by calling into the IoTHubMqttAbstract_SetOption function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_SetOption_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_SetOption(handle, TEST_OPTION_NAME, TEST_OPTION_VALUE));

    IOTHUB_CLIENT_RESULT result = IoTHubTransportMqtt_WS_SetOption(handle, TEST_OPTION_NAME, TEST_OPTION_VALUE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_003: [ IoTHubTransportMqtt_WS_Register shall register the TRANSPORT_LL_HANDLE by calling into the IoTHubMqttAbstract_Register function. ]*/
TEST_FUNCTION(IoTHubTransportMqtt_WS_Register_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    IOTHUB_DEVICE_CONFIG deviceConfig;
    deviceConfig.deviceId = TEST_DEVICE_ID;
    deviceConfig.deviceKey = NULL;
    deviceConfig.deviceSasToken = NULL;

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Register(handle, &deviceConfig, config.waitingToSend));

    IOTHUB_DEVICE_HANDLE result = IoTHubTransportMqtt_WS_Register(handle, &deviceConfig, config.waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_004: [ IoTHubTransportMqtt_WS_Unregister shall register the TRANSPORT_LL_HANDLE by calling into the IoTHubMqttAbstract_Unregister function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_Unregister_success)
{
    // arrange

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Unregister(TEST_DEVICE_HANDLE));

    IoTHubTransportMqtt_WS_Unregister(TEST_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_010: [ IoTHubTransportMqtt_WS_GetHostname shall get the hostname by calling into the IoTHubMqttAbstract_GetHostname function. ] */
TEST_FUNCTION(IoTHubTransportMqtt_WS_GetHostname_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    // act
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_GetHostname(handle));

    STRING_HANDLE result = IoTHubTransportMqtt_WS_GetHostname(handle);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_WS_SetCallbackContext_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_SetCallbackContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    int result = IotHubTransportMqtt_WS_SetCallbackContext(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_09_001: [ IoTHubTransportMqtt_WS_GetTwinAsync shall call into the IoTHubTransport_MQTT_Common_GetTwinAsync ]
TEST_FUNCTION(IoTHubTransportMqtt_WS_GetTwinAsync_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_GetTwinAsync(handle, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportMqtt_WS_GetTwinAsync(handle, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);

    // cleanup
    IoTHubTransportMqtt_WS_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_017: [ IoTHubTransportMqtt_WS_Subscribe_DeviceTwin shall call into the IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin ]
TEST_FUNCTION(IoTHubTransportMqtt_WS_Subscribe_DeviceTwin_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle));

    // act
    int result = IoTHubTransportMqtt_WS_Subscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    IoTHubTransportMqtt_WS_Destroy(handle);
}

// Tests_SRS_IOTHUB_MQTT_WEBSOCKET_TRANSPORT_07_018: [ IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin shall call into the IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin ]
TEST_FUNCTION(IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin_success)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(handle));

    // act
    IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransportMqtt_WS_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportMqtt_WS_GetSupportedPlatformInfo)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);

    PLATFORM_INFO_OPTION expected_info = PLATFORM_INFO_OPTION_RETRIEVE_SQM;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_info(&expected_info, sizeof(expected_info))
        .SetReturn(0);

    // act
    PLATFORM_INFO_OPTION info;
    int result = IotHubTransportMqtt_WS_GetSupportedPlatformInfo(handle, &info);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(int, info, PLATFORM_INFO_OPTION_RETRIEVE_SQM);

    // cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_WS_GetSupportedPlatformInfo_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    PLATFORM_INFO_OPTION info;
    int result = IotHubTransportMqtt_WS_GetSupportedPlatformInfo(NULL, &info);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(IoTHubTransportMqtt_WS_GetSupportedPlatformInfo_NULL_info)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config = { 0 };
    SetupIothubTransportConfig(&config, TEST_DEVICE_ID, TEST_DEVICE_KEY, TEST_IOTHUB_NAME, TEST_IOTHUB_SUFFIX, TEST_PROTOCOL_GATEWAY_HOSTNAME);
    TRANSPORT_LL_HANDLE handle = IoTHubTransportMqtt_WS_Create(&config, transport_cb_info, NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_MQTT_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    int result = IotHubTransportMqtt_WS_GetSupportedPlatformInfo(handle, NULL);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

END_TEST_SUITE(iothubtransportmqtt_ws_ut)
