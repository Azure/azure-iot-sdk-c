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
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/socketio.h"
#include "internal/iothubtransport_amqp_common.h"
#undef ENABLE_MOCKS

#include "iothubtransportamqp.h"


static TEST_MUTEX_HANDLE g_testByTest;

// Control parameters
MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

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

static IO_INTERFACE_DESCRIPTION* TEST_TLSIO_INTERFACE_DESCRIPTION = (IO_INTERFACE_DESCRIPTION*)0x1183;

static const IOTHUBTRANSPORT_CONFIG* saved_IoTHubTransport_AMQP_Common_Create_config;
static AMQP_GET_IO_TRANSPORT saved_IoTHubTransport_AMQP_Common_Create_get_io_transport;

static TRANSPORT_CALLBACKS_INFO g_transport_cb_info;
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

BEGIN_TEST_SUITE(iothubtransportamqp_ut)

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
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IO_INTERFACE_DESCRIPTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_IDENTITY_TYPE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_GET_IO_TRANSPORT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);
    REGISTER_TYPE(TLSIO_CONFIG*, TLSIO_CONFIG_ptr);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubTransport_AMQP_Common_Create, TEST_IoTHubTransport_AMQP_Common_Create);

    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_TLSIO_INTERFACE_DESCRIPTION);
    REGISTER_GLOBAL_MOCK_RETURN(xio_create, TEST_XIO_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_SendMessageDisposition, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_GetHostname, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_Register, TEST_IOTHUB_DEVICE_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_Subscribe, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod, 0);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_ProcessItem, IOTHUB_PROCESS_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_AMQP_Common_GetSendStatus, IOTHUB_CLIENT_OK);
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

    reset_test_data();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_001: [IoTHubTransportAMQP_Create shall create a TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_AMQP_Common_Create function, passing `config` and getTLSIOTransport.]
/* Tests_SRS_IOTHUBTRANSPORTAMQP_09_019: [This function shall return a pointer to a structure of type TRANSPORT_PROVIDER having the following values for it's fields:
IoTHubTransport_SendMessageDisposition = IoTHubTransportAMQP_SendMessageDisposition
IoTHubTransport_Subscribe_DeviceMethod = IoTHubTransportAMQP_Subscribe_DeviceMethod
IoTHubTransport_Unsubscribe_DeviceMethod = IoTHubTransportAMQP_Unsubscribe_DeviceMethod
IoTHubTransport_Subscribe_DeviceTwin = IoTHubTransportAMQP_Subscribe_DeviceTwin
IoTHubTransport_Unsubscribe_DeviceTwin = IoTHubTransportAMQP_Unsubscribe_DeviceTwin
IoTHubTransport_ProcessItem - IoTHubTransportAMQP_ProcessItem
IoTHubTransport_GetHostname = IoTHubTransportAMQP_GetHostname
IoTHubTransport_Create = IoTHubTransportAMQP_Create
IoTHubTransport_Destroy = IoTHubTransportAMQP_Destroy
IoTHubTransport_Subscribe = IoTHubTransportAMQP_Subscribe
IoTHubTransport_Unsubscribe = IoTHubTransportAMQP_Unsubscribe
IoTHubTransport_DoWork = IoTHubTransportAMQP_DoWork
IoTHubTransport_SetRetryLogic = IoTHubTransportAMQP_SetRetryLogic
IoTHubTransport_SetOption = IoTHubTransportAMQP_SetOption]*/
TEST_FUNCTION(AMQP_Create)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, NULL, &g_transport_cb_info, g_transport_cb_ctx)).IgnoreArgument_get_io_transport();

    saved_IoTHubTransport_AMQP_Common_Create_get_io_transport = NULL;

    // act
    TRANSPORT_LL_HANDLE tr_hdl = provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, &g_transport_cb_info, g_transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, tr_hdl, TEST_TRANSPORT_LL_HANDLE);
    ASSERT_IS_NOT_NULL(saved_IoTHubTransport_AMQP_Common_Create_get_io_transport);

    // cleanup
}

/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_009: [ `getIoTransportProvider` shall obtain the TLS IO interface handle by calling `platform_get_default_tlsio`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_09_004: [`getTLSIOTransport` shall return the `XIO_HANDLE` created using `xio_create`.] */
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_010: [ The TLS IO parameters shall be a `TLSIO_CONFIG` structure filled as below: ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_011: [ - `hostname` shall be set to `fqdn`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_012: [ - `port` shall be set to 443. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_013: [ `underlying_io_interface` shall be set to NULL. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_014: [ `underlying_io_parameters` shall be set to NULL. ]*/
TEST_FUNCTION(AMQP_Create_getTLSIOTransport_sets_up_TLS_over_socket_IO)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();
    TLSIO_CONFIG tlsio_config;
    XIO_HANDLE underlying_io_transport;

    (void)provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, &g_transport_cb_info, g_transport_cb_ctx);
    umock_c_reset_all_calls();

    tlsio_config.hostname = TEST_STRING;
    tlsio_config.port = 5671;
    tlsio_config.underlying_io_interface = NULL;
    tlsio_config.underlying_io_parameters = NULL;

    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, &tlsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(TLSIO_CONFIG*));

    // act
    underlying_io_transport = saved_IoTHubTransport_AMQP_Common_Create_get_io_transport(TEST_STRING, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, underlying_io_transport, TEST_XIO_HANDLE);
}

/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_009: [ `getIoTransportProvider` shall obtain the TLS IO interface handle by calling `platform_get_default_tlsio`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_09_004: [`getTLSIOTransport` shall return the `XIO_HANDLE` created using `xio_create`.] */
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_010: [ The TLS IO parameters shall be a `TLSIO_CONFIG` structure filled as below: ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_011: [ - `hostname` shall be set to `fqdn`. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_012: [ - `port` shall be set to 443. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_013: [ `underlying_io_interface` shall be set to NULL. ]*/
/* Tests_SRS_IOTHUBTRANSPORTAMQP_01_014: [ `underlying_io_parameters` shall be set to NULL. ]*/
TEST_FUNCTION(AMQP_Create_getTLSIOTransport_ignores_the_http_proxy_seetings)
{
    // arrange
    AMQP_TRANSPORT_PROXY_OPTIONS amqp_proxy_options;
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();
    TLSIO_CONFIG tlsio_config;
    XIO_HANDLE underlying_io_transport;

    (void)provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, &g_transport_cb_info, g_transport_cb_ctx);
    umock_c_reset_all_calls();

    tlsio_config.hostname = TEST_STRING;
    tlsio_config.port = 5671;
    tlsio_config.underlying_io_interface = NULL;
    tlsio_config.underlying_io_parameters = NULL;

    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, &tlsio_config))
        .ValidateArgumentValue_io_create_parameters_AsType(UMOCK_TYPE(TLSIO_CONFIG*));

    amqp_proxy_options.host_address = "some_host";
    amqp_proxy_options.port = 444;
    amqp_proxy_options.username = "me";
    amqp_proxy_options.password = "shhhh";

    // act
    underlying_io_transport = saved_IoTHubTransport_AMQP_Common_Create_get_io_transport(TEST_STRING, &amqp_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, underlying_io_transport, TEST_XIO_HANDLE);
}

/* Tests_SRS_IOTHUBTRANSPORTAMQP_09_003: [If `platform_get_default_tlsio` returns NULL `getTLSIOTransport` shall return NULL.] */
TEST_FUNCTION(when_platform_get_default_tlsio_returns_NULL_AMQP_Create_getTLSIOTransport_returns_NULL)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();
    XIO_HANDLE underlying_io_transport;

    (void)provider->IoTHubTransport_Create(TEST_IOTHUBTRANSPORT_CONFIG_HANDLE, &g_transport_cb_info, g_transport_cb_ctx);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(platform_get_default_tlsio())
        .SetReturn(NULL);

    // act
    underlying_io_transport = saved_IoTHubTransport_AMQP_Common_Create_get_io_transport(TEST_STRING, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(underlying_io_transport);
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_015: [IoTHubTransportAMQP_DoWork shall call into the IoTHubTransport_AMQP_Common_DoWork()]
TEST_FUNCTION(AMQP_DoWork)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_DoWork(TEST_TRANSPORT_LL_HANDLE));

    // act
    provider->IoTHubTransport_DoWork(TEST_TRANSPORT_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_006: [IoTHubTransportAMQP_Register shall register the device by calling into the IoTHubTransport_AMQP_Common_Register().]
TEST_FUNCTION(AMQP_Register)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Register(TEST_TRANSPORT_LL_HANDLE, TEST_IOTHUB_DEVICE_CONFIG_HANDLE, TEST_WAITING_TO_SEND_LIST));

    // act
    IOTHUB_DEVICE_HANDLE dev_hdl = provider->IoTHubTransport_Register(TEST_TRANSPORT_LL_HANDLE, TEST_IOTHUB_DEVICE_CONFIG_HANDLE, TEST_WAITING_TO_SEND_LIST);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void_ptr)dev_hdl, (void_ptr)TEST_IOTHUB_DEVICE_HANDLE);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_007: [IoTHubTransportAMQP_Unregister shall unregister the device by calling into the IoTHubTransport_AMQP_Common_Unregister().]
TEST_FUNCTION(AMQP_Unregister)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Unregister(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    provider->IoTHubTransport_Unregister(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_008: [IoTHubTransportAMQP_Subscribe_DeviceTwin shall invoke IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin() and return its result.]
TEST_FUNCTION(AMQP_Subscribe_DeviceTwin)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    int result = provider->IoTHubTransport_Subscribe_DeviceTwin(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_009: [IoTHubTransportAMQP_Unsubscribe_DeviceTwin shall invoke IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin()]
TEST_FUNCTION(AMQP_Unsubscribe_DeviceTwin)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    provider->IoTHubTransport_Unsubscribe_DeviceTwin(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_021: [IoTHubTransportAMQP_GetTwinAsync shall invoke IoTHubTransport_AMQP_Common_GetTwinAsync()]
TEST_FUNCTION(AMQP_GetTwinAsync)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_GetTwinAsync(TEST_IOTHUB_DEVICE_HANDLE, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445));

    // act
    IOTHUB_CLIENT_RESULT result = provider->IoTHubTransport_GetTwinAsync(TEST_IOTHUB_DEVICE_HANDLE, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_010: [IoTHubTransportAMQP_Subscribe_DeviceMethod shall invoke IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod() and return its result.]
TEST_FUNCTION(AMQP_Subscribe_DeviceMethod)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    int result = provider->IoTHubTransport_Subscribe_DeviceMethod(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_011: [IoTHubTransportAMQP_Unsubscribe_DeviceMethod shall invoke IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod()]
TEST_FUNCTION(AMQP_Unsubscribe_DeviceMethod)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    provider->IoTHubTransport_Unsubscribe_DeviceMethod(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_012: [IoTHubTransportAMQP_Subscribe shall subscribe for D2C messages by calling into the IoTHubTransport_AMQP_Common_Subscribe().]
TEST_FUNCTION(AMQP_Subscribe)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Subscribe(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    int result = provider->IoTHubTransport_Subscribe(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_013: [IoTHubTransportAMQP_Unsubscribe shall subscribe for D2C messages by calling into the IoTHubTransport_AMQP_Common_Unsubscribe().]
TEST_FUNCTION(AMQP_Unsubscribe)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Unsubscribe(TEST_IOTHUB_DEVICE_HANDLE));

    // act
    provider->IoTHubTransport_Unsubscribe(TEST_IOTHUB_DEVICE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_014: [IoTHubTransportAMQP_ProcessItem shall invoke IoTHubTransport_AMQP_Common_ProcessItem() and return its result.]
TEST_FUNCTION(AMQP_ProcessItem)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_ProcessItem(TEST_TRANSPORT_LL_HANDLE, TEST_IOTHUB_IDENTITY_TYPE, TEST_IOTHUB_IDENTITY_INFO_HANDLE));

    // act
    IOTHUB_PROCESS_ITEM_RESULT result = provider->IoTHubTransport_ProcessItem(TEST_TRANSPORT_LL_HANDLE, TEST_IOTHUB_IDENTITY_TYPE, TEST_IOTHUB_IDENTITY_INFO_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, IOTHUB_PROCESS_OK);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_017: [IoTHubTransportAMQP_SetOption shall set the options by calling into the IoTHubTransport_AMQP_Common_SetOption()]
TEST_FUNCTION(AMQP_SetOption)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_SetOption(TEST_TRANSPORT_LL_HANDLE, TEST_STRING, TEST_STRING_HANDLE));

    // act
    int result = provider->IoTHubTransport_SetOption(TEST_TRANSPORT_LL_HANDLE, TEST_STRING, TEST_STRING_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_016: [IoTHubTransportAMQP_GetSendStatus shall get the send status by calling into the IoTHubTransport_AMQP_Common_GetSendStatus()]
TEST_FUNCTION(AMQP_GetSendStatus)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

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


// Tests_SRS_IOTHUBTRANSPORTAMQP_09_018: [IoTHubTransportAMQP_GetHostname shall get the hostname by calling into the IoTHubTransport_AMQP_Common_GetHostname()]
TEST_FUNCTION(AMQP_GetHostname)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_GetHostname(TEST_TRANSPORT_LL_HANDLE));

    // act
    STRING_HANDLE result = provider->IoTHubTransport_GetHostname(TEST_TRANSPORT_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_STRING_HANDLE);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_09_005: [IoTHubTransportAMQP_Destroy shall destroy the TRANSPORT_LL_HANDLE by calling into the IoTHubTransport_AMQP_Common_Destroy().]
TEST_FUNCTION(AMQP_Destroy)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_Destroy(TEST_TRANSPORT_LL_HANDLE));

    // act
    provider->IoTHubTransport_Destroy(TEST_TRANSPORT_LL_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_10_001: [IoTHubTransportAMQP_SendMessageDisposition shall get the hostname by calling into the IoTHubTransport_AMQP_Common_SendMessageDisposition().]
TEST_FUNCTION(AMQP_SendMessageDisposition)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubTransport_AMQP_Common_SendMessageDisposition(IGNORED_PTR_ARG, IOTHUBMESSAGE_ACCEPTED))
        .IgnoreAllArguments();

    // act
    IOTHUB_CLIENT_RESULT result = provider->IoTHubTransport_SendMessageDisposition(NULL, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_31_021: [IoTHubTransportAMQP_Subscribe_InputQueue shall return a failure as input queues are not implemented for AMQP]
TEST_FUNCTION(AMQP_Subscribe_InputQueue)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();

    // act
    int result = provider->IoTHubTransport_Subscribe_InputQueue(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORTAMQP_31_022: [IotHubTransportAMQP_Unsubscribe_InputQueue shall do nothing as input queues are not implemented for AMQP]
TEST_FUNCTION(AMQP_Unsubscribe_InputQueue)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

    umock_c_reset_all_calls();

    // act
    provider->IoTHubTransport_Unsubscribe_InputQueue(NULL);
}

TEST_FUNCTION(AMQP_GetSupportedPlatformInfo)
{
    // arrange
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();
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
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

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
    TRANSPORT_PROVIDER* provider = (TRANSPORT_PROVIDER*)AMQP_Protocol();

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

END_TEST_SUITE(iothubtransportamqp_ut)
