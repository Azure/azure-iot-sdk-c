// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#include "testrunnerswitcher.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"

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
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/wsio.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "internal/iothubtransport_amqp_common.h"
#include "internal/iothub_transport_ll_private.h"

MOCKABLE_FUNCTION(, bool, Transport_MessageCallbackFromInput, MESSAGE_CALLBACK_INFO*, messageData, void*, ctx);
MOCKABLE_FUNCTION(, bool, Transport_MessageCallback, MESSAGE_CALLBACK_INFO*, messageData, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_ConnectionStatusCallBack, IOTHUB_CLIENT_CONNECTION_STATUS, status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_SendComplete_Callback, PDLIST_ENTRY, completed, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, ctx);
MOCKABLE_FUNCTION(, const char*, Transport_GetOption_Product_Info_Callback, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_ReportedStateComplete_Callback, uint32_t, item_id, int, status_code, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_RetrievePropertyComplete_Callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, ctx);
MOCKABLE_FUNCTION(, int, Transport_DeviceMethod_Complete_Callback, const char*, method_name, const unsigned char*, payLoad, size_t, size, METHOD_HANDLE, response_id, void*, ctx);

#undef ENABLE_MOCKS

#include "iothubtransportamqp_websockets.h"

static TEST_MUTEX_HANDLE g_testByTest;

// Control parameters
MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(PLATFORM_INFO_OPTION, PLATFORM_INFO_OPTION_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PLATFORM_INFO_OPTION, PLATFORM_INFO_OPTION_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

#define TEST_IOTHUBTRANSPORT_CONFIG_HANDLE  ((IOTHUBTRANSPORT_CONFIG*)0x4240)
#define TEST_XIO_INTERFACE                  ((const IO_INTERFACE_DESCRIPTION*)0x4247)
#define TEST_XIO_HANDLE                     ((XIO_HANDLE)0x4248)
#define TEST_TRANSPORT_LL_HANDLE            ((TRANSPORT_LL_HANDLE)0x4249)
#define TEST_STRING_HANDLE                  ((STRING_HANDLE)0x4250)
#define TEST_STRING                         "SOME TEXT"
#define TEST_IOTHUB_DEVICE_CONFIG_HANDLE    ((const IOTHUB_DEVICE_CONFIG*)0x4444)
#define TEST_WAITING_TO_SEND_LIST           ((PDLIST_ENTRY)0x4445)
#define TEST_IOTHUB_DEVICE_HANDLE           ((IOTHUB_DEVICE_HANDLE)0x4446)
#define TEST_IOTHUB_IDENTITY_TYPE           IOTHUB_TYPE_DEVICE_TWIN
#define TEST_IOTHUB_IDENTITY_INFO_HANDLE    ((IOTHUB_IDENTITY_INFO*)0x4449)

static IO_INTERFACE_DESCRIPTION* TEST_WSIO_INTERFACE_DESCRIPTION = (IO_INTERFACE_DESCRIPTION*)0x1182;
static IO_INTERFACE_DESCRIPTION* TEST_TLSIO_INTERFACE_DESCRIPTION = (IO_INTERFACE_DESCRIPTION*)0x1183;
static IO_INTERFACE_DESCRIPTION* TEST_HTTP_PROXY_IO_INTERFACE_DESCRIPTION = (IO_INTERFACE_DESCRIPTION*)0x1185;

static const IOTHUBTRANSPORT_CONFIG* saved_IoTHubTransport_AMQP_Common_Create_config;
static AMQP_GET_IO_TRANSPORT saved_IoTHubTransport_AMQP_Common_Create_get_io_transport;

static TRANSPORT_CALLBACKS_INFO* g_transport_cb_info = (TRANSPORT_CALLBACKS_INFO*)0x227733;
static void* g_transport_cb_ctx = (void*)0x499922;

static TRANSPORT_LL_HANDLE TEST_IoTHubTransport_AMQP_Common_Create(const IOTHUBTRANSPORT_CONFIG* config, AMQP_GET_IO_TRANSPORT get_io_transport, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    (void)cb_info;
    (void)ctx;
    saved_IoTHubTransport_AMQP_Common_Create_config = config;
    saved_IoTHubTransport_AMQP_Common_Create_get_io_transport = get_io_transport;
    return TEST_TRANSPORT_LL_HANDLE;
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

BEGIN_TEST_SUITE(iothubtransportamqp_ws_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    int result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_IDENTITY_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_GET_IO_TRANSPORT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);
    REGISTER_TYPE(WSIO_CONFIG*, WSIO_CONFIG_ptr);
    REGISTER_TYPE(TLSIO_CONFIG*, TLSIO_CONFIG_ptr);
    REGISTER_TYPE(HTTP_PROXY_IO_CONFIG*, HTTP_PROXY_IO_CONFIG_ptr);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubTransport_AMQP_Common_Create, TEST_IoTHubTransport_AMQP_Common_Create);

    REGISTER_GLOBAL_MOCK_RETURN(wsio_get_interface_description, TEST_XIO_INTERFACE);
    REGISTER_GLOBAL_MOCK_RETURN(xio_create, TEST_XIO_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_GetHostname, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_Register, TEST_IOTHUB_DEVICE_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_Subscribe, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_ProcessItem, IOTHUB_PROCESS_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_GetSendStatus, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_GetTwinAsync, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubTransport_AMQP_Common_GetTwinAsync, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(wsio_get_interface_description, TEST_WSIO_INTERFACE_DESCRIPTION);
    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_TLSIO_INTERFACE_DESCRIPTION);
    REGISTER_GLOBAL_MOCK_RETURN(http_proxy_io_get_interface_description, TEST_HTTP_PROXY_IO_INTERFACE_DESCRIPTION);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

static void reset_test_data()
{
    saved_IoTHubTransport_AMQP_Common_Create_config = NULL;
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
    reset_test_data();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_001: [IoTHubTransportAMQP_WS_Create shall create a TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_AMQP_Common_Create function, passing `config` and getWebSocketsIOTransport.]
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_019: [This function shall return a pointer to a structure of type TRANSPORT_PROVIDER having the following values for it's fields:
IoTHubTransport_Subscribe_DeviceMethod = IoTHubTransportAMQP_WS_Subscribe_DeviceMethod
IoTHubTransport_Unsubscribe_DeviceMethod = IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod
IoTHubTransport_Subscribe_DeviceTwin = IoTHubTransportAMQP_WS_Subscribe_DeviceTwin
IoTHubTransport_Unsubscribe_DeviceTwin = IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin
IoTHubTransport_ProcessItem - IoTHubTransportAMQP_WS_ProcessItem
IoTHubTransport_GetHostname = IoTHubTransportAMQP_WS_GetHostname
IoTHubTransport_Create = IoTHubTransportAMQP_WS_Create
IoTHubTransport_Destroy = IoTHubTransportAMQP_WS_Destroy
IoTHubTransport_Subscribe = IoTHubTransportAMQP_WS_Subscribe
IoTHubTransport_Unsubscribe = IoTHubTransportAMQP_WS_Unsubscribe
IoTHubTransport_DoWork = IoTHubTransportAMQP_WS_DoWork
IoTHubTransport_SetRetryLogic = IoTHubTransportAMQP_WS_SetRetryLogic
IoTHubTransport_SetOption = IoTHubTransportAMQP_WS_SetOption]*/
TEST_FUNCTION(AMQP_Create)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, IGNORED_PTR_ARG, g_transport_cb_info, g_transport_cb_ctx));

    saved_IoTHubTransport_AMQP_Common_Create_get_io_transport = NULL;

    // act
    TRANSPORT_LL_HANDLE tr_hdl = provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, g_transport_cb_info, g_transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, tr_hdl, TEST_TRANSPORT_LL_HANDLE);
    ASSERT_IS_NOT_NULL(saved_IoTHubTransport_AMQP_Common_Create_get_io_transport);

    // cleanup
}

/* Tests_SRS_IoTHubTransportAMQP_WS_01_001: [ `getIoTransportProvider` shall obtain the WebSocket IO interface handle by calling `wsio_get_interface_description`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_002: [ `getIoTransportProvider` shall call `xio_create` while passing the WebSocket IO interface description to it and the WebSocket configuration as a WSIO_CONFIG structure, filled as below: ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_004: [getWebSocketsIOTransport shall return the XIO_HANDLE created using xio_create().] */
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_003: [ - `hostname` shall be set to `fqdn`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_004: [ - `port` shall be set to 443. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_005: [ - `protocol` shall be set to `AMQPWSB10`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_006: [ - `resource_name` shall be set to `/$iothub/websocket`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_007: [ - `underlying_io_interface` shall be set to the TLS IO interface description. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_008: [ - `underlying_io_parameters` shall be set to the TLS IO arguments. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_009: [ `getIoTransportProvider` shall obtain the TLS IO interface handle by calling `platform_get_default_tlsio`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_010: [ The TLS IO parameters shall be a `TLSIO_CONFIG` structure filled as below: ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_011: [ - `hostname` shall be set to `fqdn`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_012: [ - `port` shall be set to 443. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_013: [ - If `amqp_transport_proxy_options` is NULL, `underlying_io_interface` shall be set to NULL. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_014: [ - If `amqp_transport_proxy_options` is NULL `underlying_io_parameters` shall be set to NULL. ]*/
TEST_FUNCTION(AMQP_Create_getWebSocketsIOTransport_with_NULL_proxy_options_sets_up_wsio_over_tlsio_over_socketio)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();
    WSIO_CONFIG wsio_config;
    TLSIO_CONFIG tlsio_config;
    XIO_HANDLE underlying_io_transport;

    (void)provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, g_transport_cb_info, g_transport_cb_ctx);
    umock_c_reset_all_calls();

    tlsio_config.hostname = TEST_STRING;
    tlsio_config.port = 443;
    tlsio_config.underlying_io_interface = NULL;
    tlsio_config.underlying_io_parameters = NULL;

    wsio_config.hostname = TEST_STRING;
    wsio_config.port = 443;
    wsio_config.protocol = "AMQPWSB10";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = TEST_TLSIO_INTERFACE_DESCRIPTION;
    wsio_config.underlying_io_parameters = &tlsio_config;

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    // act
    underlying_io_transport = saved_IoTHubTransport_AMQP_Common_Create_get_io_transport(TEST_STRING, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, underlying_io_transport, TEST_XIO_HANDLE);
}

/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_003: [If `io_interface_description` is NULL getWebSocketsIOTransport shall return NULL.] */
TEST_FUNCTION(when_wsio_get_interface_description_returns_NULL_AMQP_Create_getWebSocketsIOTransport_returns_NULL)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();
    XIO_HANDLE underlying_io_transport;

    (void)provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, g_transport_cb_info, g_transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(wsio_get_interface_description())
        .SetReturn(NULL);

    // act
    underlying_io_transport = saved_IoTHubTransport_AMQP_Common_Create_get_io_transport(TEST_STRING, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(underlying_io_transport);
}

/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_029: [ If `platform_get_default_tlsio` returns NULL, NULL shall be set in the WebSocket IO parameters structure for the interface description and parameters. ]*/
TEST_FUNCTION(when_TLSIIO_interface_is_NULL_AMQP_Create_getWebSocketsIOTransport_passes_NULL_in_the_wsio_config)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();
    WSIO_CONFIG wsio_config;
    XIO_HANDLE underlying_io_transport;

    (void)provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, g_transport_cb_info, g_transport_cb_ctx);
    umock_c_reset_all_calls();

    wsio_config.hostname = TEST_STRING;
    wsio_config.port = 443;
    wsio_config.protocol = "AMQPWSB10";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = NULL;
    wsio_config.underlying_io_parameters = NULL;

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio())
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    // act
    underlying_io_transport = saved_IoTHubTransport_AMQP_Common_Create_get_io_transport(TEST_STRING, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, underlying_io_transport, TEST_XIO_HANDLE);
}

/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_015: [ - If `amqp_transport_proxy_options` is not NULL, `underlying_io_interface` shall be set to the HTTP proxy IO interface description. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_016: [ - If `amqp_transport_proxy_options` is not NULL `underlying_io_parameters` shall be set to the HTTP proxy IO arguments. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_022: [ `getIoTransportProvider` shall obtain the HTTP proxy IO interface handle by calling `http_proxy_io_get_interface_description`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_023: [ The HTTP proxy IO arguments shall be an `HTTP_PROXY_IO_CONFIG` structure, filled as below: ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_024: [ - `hostname` shall be set to `fully_qualified_name`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_025: [ - `port` shall be set to 443. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_026: [ - `proxy_hostname`, `proxy_port`, `username` and `password` shall be copied from the `mqtt_transport_proxy_options` argument. ]*/
TEST_FUNCTION(AMQP_Create_getWebSocketsIOTransport_with_proxy_options_sets_up_wsio_over_tlsio_over_http_proxy_io)
{
    // arrange
    AMQP_TRANSPORT_PROXY_OPTIONS amqp_proxy_options;
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();
    WSIO_CONFIG wsio_config;
    TLSIO_CONFIG tlsio_config;
    HTTP_PROXY_IO_CONFIG http_proxy_io_config;
    XIO_HANDLE underlying_io_transport;

    (void)provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, g_transport_cb_info, g_transport_cb_ctx);
    umock_c_reset_all_calls();

    http_proxy_io_config.hostname = TEST_STRING;
    http_proxy_io_config.port = 443;
    http_proxy_io_config.proxy_hostname = "some_host";
    http_proxy_io_config.proxy_port = 444;
    http_proxy_io_config.username = "me";
    http_proxy_io_config.password = "shhhh";

    tlsio_config.hostname = TEST_STRING;
    tlsio_config.port = 443;
    tlsio_config.underlying_io_interface = TEST_HTTP_PROXY_IO_INTERFACE_DESCRIPTION;
    tlsio_config.underlying_io_parameters = &http_proxy_io_config;

    wsio_config.hostname = TEST_STRING;
    wsio_config.port = 443;
    wsio_config.protocol = "AMQPWSB10";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = TEST_TLSIO_INTERFACE_DESCRIPTION;
    wsio_config.underlying_io_parameters = &tlsio_config;

    amqp_proxy_options.host_address = "some_host";
    amqp_proxy_options.port = 444;
    amqp_proxy_options.username = "me";
    amqp_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(http_proxy_io_get_interface_description());
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    // act
    underlying_io_transport = saved_IoTHubTransport_AMQP_Common_Create_get_io_transport(TEST_STRING, &amqp_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, underlying_io_transport, TEST_XIO_HANDLE);
}

/* Tests_SRS_IOTHUBTRANSPORTAMQP_WS_01_028: [ If `http_proxy_io_get_interface_description` returns NULL, NULL shall be set in the TLS IO parameters structure for the interface description and parameters. ]*/
TEST_FUNCTION(when_http_proxy_io_get_interface_description_returns_NULL_AMQP_Create_getWebSocketsIOTransport_with_proxy_options_sets_NULL_as_http_proxy_io_parameters)
{
    // arrange
    AMQP_TRANSPORT_PROXY_OPTIONS amqp_proxy_options;
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();
    WSIO_CONFIG wsio_config;
    TLSIO_CONFIG tlsio_config;
    XIO_HANDLE underlying_io_transport;

    (void)provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, g_transport_cb_info, g_transport_cb_ctx);
    umock_c_reset_all_calls();

    tlsio_config.hostname = TEST_STRING;
    tlsio_config.port = 443;
    tlsio_config.underlying_io_interface = NULL;
    tlsio_config.underlying_io_parameters = NULL;

    wsio_config.hostname = TEST_STRING;
    wsio_config.port = 443;
    wsio_config.protocol = "AMQPWSB10";
    wsio_config.resource_name = "/$iothub/websocket";
    wsio_config.underlying_io_interface = TEST_TLSIO_INTERFACE_DESCRIPTION;
    wsio_config.underlying_io_parameters = &tlsio_config;

    amqp_proxy_options.host_address = "some_host";
    amqp_proxy_options.port = 444;
    amqp_proxy_options.username = "me";
    amqp_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(wsio_get_interface_description());
    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(http_proxy_io_get_interface_description())
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(xio_create(TEST_WSIO_INTERFACE_DESCRIPTION, &wsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(WSIO_CONFIG*));

    // act
    underlying_io_transport = saved_IoTHubTransport_AMQP_Common_Create_get_io_transport(TEST_STRING, &amqp_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, underlying_io_transport, TEST_XIO_HANDLE);
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_015: [IoTHubTransportAMQP_WS_DoWork shall call into the IoTHubTransport_AMQP_Common_DoWork()]
TEST_FUNCTION(AMQP_DoWork)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_DoWork(TEST_TRANSPORT_LL_HANDLE));

    // act
    provider->IoTHubTransport_DoWork(TEST_TRANSPORT_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_006: [IoTHubTransportAMQP_WS_Register shall register the device by calling into the IoTHubTransport_AMQP_Common_Register().]
TEST_FUNCTION(AMQP_Register)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Register(TEST_TRANSPORT_LL_HANDLE, TEST_IOTHUB_DEVICE_CONFIG_HANDLE, TEST_WAITING_TO_SEND_LIST));

    // act
    IOTHUB_DEVICE_HANDLE dev_hdl = provider->IoTHubTransport_Register(TEST_TRANSPORT_LL_HANDLE, TEST_IOTHUB_DEVICE_CONFIG_HANDLE, TEST_WAITING_TO_SEND_LIST);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void_ptr)dev_hdl, (void_ptr)TEST_IOTHUB_DEVICE_HANDLE);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_007: [IoTHubTransportAMQP_WS_Unregister shall unregister the device by calling into the IoTHubTransport_AMQP_Common_Unregister().]
TEST_FUNCTION(AMQP_Unregister)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Unregister(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    provider->IoTHubTransport_Unregister(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_008: [IoTHubTransportAMQP_WS_Subscribe_DeviceTwin shall invoke IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin() and return its result.]
TEST_FUNCTION(AMQP_Subscribe_DeviceTwin)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    int result = provider->IoTHubTransport_Subscribe_DeviceTwin(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_009: [IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin shall invoke IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin()]
TEST_FUNCTION(AMQP_Unsubscribe_DeviceTwin)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    provider->IoTHubTransport_Unsubscribe_DeviceTwin(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_020: [IoTHubTransportAMQP_WS_GetTwinAsync shall invoke IoTHubTransport_AMQP_Common_GetTwinAsync()]
TEST_FUNCTION(AMQP_GetTwinAsync)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_GetTwinAsync(TEST_IOTHUB_DEVICE_HANDLE, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445));

    // act
    IOTHUB_CLIENT_RESULT result = provider->IoTHubTransport_GetTwinAsync(TEST_IOTHUB_DEVICE_HANDLE, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_010: [IoTHubTransportAMQP_WS_Subscribe_DeviceMethod shall invoke IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod() and return its result.]
TEST_FUNCTION(AMQP_Subscribe_DeviceMethod)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    int result = provider->IoTHubTransport_Subscribe_DeviceMethod(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_011: [IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod shall invoke IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod()]
TEST_FUNCTION(AMQP_Unsubscribe_DeviceMethod)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    provider->IoTHubTransport_Unsubscribe_DeviceMethod(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_012: [IoTHubTransportAMQP_WS_Subscribe shall subscribe for D2C messages by calling into the IoTHubTransport_AMQP_Common_Subscribe().]
TEST_FUNCTION(AMQP_Subscribe)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Subscribe(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    int result = provider->IoTHubTransport_Subscribe(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_013: [IoTHubTransportAMQP_WS_Unsubscribe shall subscribe for D2C messages by calling into the IoTHubTransport_AMQP_Common_Unsubscribe().]
TEST_FUNCTION(AMQP_Unsubscribe)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Unsubscribe(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    provider->IoTHubTransport_Unsubscribe(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_014: [IoTHubTransportAMQP_WS_ProcessItem shall invoke IoTHubTransport_AMQP_Common_ProcessItem() and return its result.]
TEST_FUNCTION(AMQP_ProcessItem)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_ProcessItem(TEST_TRANSPORT_LL_HANDLE, TEST_IOTHUB_IDENTITY_TYPE, TEST_IOTHUB_IDENTITY_INFO_HANDLE));

    // act
    IOTHUB_PROCESS_ITEM_RESULT result = provider->IoTHubTransport_ProcessItem(TEST_TRANSPORT_LL_HANDLE, TEST_IOTHUB_IDENTITY_TYPE, TEST_IOTHUB_IDENTITY_INFO_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, IOTHUB_PROCESS_OK);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_017: [IoTHubTransportAMQP_WS_SetOption shall set the options by calling into the IoTHubTransport_AMQP_Common_SetOption()]
TEST_FUNCTION(AMQP_SetOption)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_SetOption(TEST_TRANSPORT_LL_HANDLE, TEST_STRING, TEST_STRING_HANDLE));

    // act
    int result = provider->IoTHubTransport_SetOption(TEST_TRANSPORT_LL_HANDLE, TEST_STRING, TEST_STRING_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_016: [IoTHubTransportAMQP_WS_GetSendStatus shall get the send status by calling into the IoTHubTransport_AMQP_Common_GetSendStatus()]
TEST_FUNCTION(AMQP_GetSendStatus)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    IOTHUB_CLIENT_STATUS client_status;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_GetSendStatus(TEST_IOTHUB_DEVICE_HANDLE, &client_status));

    // act
    IOTHUB_CLIENT_RESULT result = provider->IoTHubTransport_GetSendStatus(TEST_IOTHUB_DEVICE_HANDLE, &client_status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, IOTHUB_CLIENT_OK);

    // cleanup
}


// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_018: [IoTHubTransportAMQP_WS_GetHostname shall get the hostname by calling into the IoTHubTransport_AMQP_Common_GetHostname()]
TEST_FUNCTION(AMQP_GetHostname)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_GetHostname(TEST_TRANSPORT_LL_HANDLE));

    // act
    STRING_HANDLE result = provider->IoTHubTransport_GetHostname(TEST_TRANSPORT_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_STRING_HANDLE);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_WS_09_005: [IoTHubTransportAMQP_WS_Destroy shall destroy the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_AMQP_Common_Destroy().]
TEST_FUNCTION(AMQP_Destroy)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Destroy(TEST_TRANSPORT_LL_HANDLE));

    // act
    provider->IoTHubTransport_Destroy(TEST_TRANSPORT_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubTransportAmqp_SetCallbackContext_success)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_SetCallbackContext(TEST_TRANSPORT_LL_HANDLE, g_transport_cb_ctx));

    // act
    provider->IoTHubTransport_SetCallbackContext(TEST_TRANSPORT_LL_HANDLE, g_transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}


TEST_FUNCTION(AMQP_GetSupportedPlatformInfo)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();
    PLATFORM_INFO_OPTION expected_info = PLATFORM_INFO_OPTION_RETRIEVE_SQM;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_info(&expected_info, sizeof(expected_info))
        .SetReturn(0);

    // act
    PLATFORM_INFO_OPTION info;
    int result = provider->IoTHubTransport_GetSupportedPlatformInfo(TEST_TRANSPORT_LL_HANDLE, &info);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(int, info, PLATFORM_INFO_OPTION_RETRIEVE_SQM);

    // cleanup
}

TEST_FUNCTION(AMQP_GetSupportedPlatformInfo_NULL_handle)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    PLATFORM_INFO_OPTION info;
    int result = provider->IoTHubTransport_GetSupportedPlatformInfo(NULL, &info);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(AMQP_GetSupportedPlatformInfo_NULL_info)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol_over_WebSocketsTls();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    int result = provider->IoTHubTransport_GetSupportedPlatformInfo(TEST_TRANSPORT_LL_HANDLE, NULL);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

END_TEST_SUITE(iothubtransportamqp_ws_ut)
