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

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_prov_client/internal/prov_transport_amqp_common.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_prov_client/prov_transport.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/wsio.h"
#undef ENABLE_MOCKS

#include "azure_prov_client/prov_transport_amqp_ws_client.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/tlsio.h"

#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, on_transport_register_data_cb, PROV_DEVICE_TRANSPORT_RESULT, transport_result, BUFFER_HANDLE, iothub_key, const char*, assigned_hub, const char*, device_id, void*, user_ctx);
MOCKABLE_FUNCTION(, void, on_transport_status_cb, PROV_DEVICE_TRANSPORT_STATUS, transport_status, uint32_t, retry_interval, void*, user_ctx);
MOCKABLE_FUNCTION(, char*, on_transport_challenge_callback, const unsigned char*, nonce, size_t, nonce_len, const char*, key_name, void*, user_ctx);
MOCKABLE_FUNCTION(, XIO_HANDLE, on_amqp_transport_io, const char*, fqdn, SASL_MECHANISM_HANDLE*, sasl_mechanism, const HTTP_PROXY_IO_CONFIG*, proxy_info);
MOCKABLE_FUNCTION(, char*, on_transport_create_json_payload, const char*, ek_value, const char*, srk_value, void*, user_ctx);
MOCKABLE_FUNCTION(, PROV_JSON_INFO*, on_transport_json_parse, const char*, json_document, void*, user_ctx);
MOCKABLE_FUNCTION(, void, on_transport_error, PROV_DEVICE_TRANSPORT_ERROR, transport_error, void*, user_context);

#undef ENABLE_MOCKS

#define TEST_DPS_HANDLE     (PROV_DEVICE_TRANSPORT_HANDLE)0x11111111
#define TEST_BUFFER_VALUE   (BUFFER_HANDLE)0x11111112
#define TEST_INTERFACE_DESC (const IO_INTERFACE_DESCRIPTION*)0x11111113
#define TEST_XIO_HANDLE     (XIO_HANDLE)0x11111114
#define TEST_OPTION_VALUE   (void*)0x1111111B

static const char* TEST_URI_VALUE = "dps_uri";
static const char* TEST_SCOPE_ID_VALUE = "scope_id";
static const char* TEST_REGISTRATION_ID_VALUE = "registration_id";
static const char* TEST_DPS_API_VALUE = "dps_api";
static const char* TEST_X509_CERT_VALUE = "x509_cert";
static const char* TEST_CERT_VALUE = "certificate";
static const char* TEST_PRIVATE_KEY_VALUE = "private_key";
static const char* TEST_HOST_ADDRESS_VALUE = "host_address";
static const char* TEST_XIO_OPTION_NAME = "test_option";

PROV_AMQP_TRANSPORT_IO g_transport_io = NULL;

static pfprov_transport_create prov_amqp_ws_transport_create;
static pfprov_transport_destroy prov_amqp_ws_transport_destroy;
static pfprov_transport_open prov_amqp_ws_transport_open;
static pfprov_transport_close prov_amqp_ws_transport_close;
static pfprov_transport_register_device prov_amqp_ws_transport_register_device;
static pfprov_transport_get_operation_status prov_amqp_ws_transport_get_op_status;
static pfprov_transport_dowork prov_amqp_ws_transport_dowork;
static pfprov_transport_set_trace prov_amqp_ws_transport_set_trace;
static pfprov_transport_set_x509_cert prov_amqp_ws_transport_x509_cert;
static pfprov_transport_set_trusted_cert prov_amqp_ws_transport_trusted_cert;
static pfprov_transport_set_proxy prov_amqp_ws_transport_set_proxy;
static pfprov_transport_set_option prov_amqp_ws_transport_set_option;

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE_VALUES);

static PROV_DEVICE_TRANSPORT_HANDLE my_prov_transport_common_amqp_create(const char* uri, TRANSPORT_HSM_TYPE type, const char* scope_id, const char* dps_api_version, PROV_AMQP_TRANSPORT_IO transport_io, PROV_TRANSPORT_ERROR_CALLBACK error_cb, void* error_ctx)
{
    (void)uri;
    (void)type;
    (void)scope_id;
    (void)dps_api_version;
    (void)error_cb;
    (void)error_ctx;

    g_transport_io = transport_io;
    return TEST_DPS_HANDLE;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(prov_transport_amqp_ws_client_ut)

    TEST_SUITE_INITIALIZE(suite_init)
    {
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);
        (void)umocktypes_bool_register_types();

        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT);
        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS);
        REGISTER_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE);

        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_STATUS_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_CHALLENGE_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_AMQP_TRANSPORT_IO, void*);
        REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_JSON_PARSE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_ERROR_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_CREATE_JSON_PAYLOAD, void*);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_RETURN(wsio_get_interface_description, TEST_INTERFACE_DESC);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(wsio_get_interface_description, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_INTERFACE_DESC);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(platform_get_default_tlsio, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(http_proxy_io_get_interface_description, TEST_INTERFACE_DESC);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(http_proxy_io_get_interface_description, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(saslclientio_get_interface_description, TEST_INTERFACE_DESC);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslclientio_get_interface_description, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(xio_create, TEST_XIO_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_create, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(prov_transport_common_amqp_create, my_prov_transport_common_amqp_create);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_common_amqp_open, 0);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_common_amqp_close, 0);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_common_amqp_register_device, 0);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_common_amqp_get_operation_status, 0);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_common_amqp_set_trace, 0);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_common_amqp_x509_cert, 0);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_common_amqp_set_trusted_cert, 0);
        REGISTER_GLOBAL_MOCK_RETURN(prov_transport_common_amqp_set_proxy, 0);

        prov_amqp_ws_transport_create = Prov_Device_AMQP_WS_Protocol()->prov_transport_create;
        prov_amqp_ws_transport_destroy = Prov_Device_AMQP_WS_Protocol()->prov_transport_destroy;
        prov_amqp_ws_transport_open = Prov_Device_AMQP_WS_Protocol()->prov_transport_open;
        prov_amqp_ws_transport_close = Prov_Device_AMQP_WS_Protocol()->prov_transport_close;
        prov_amqp_ws_transport_register_device = Prov_Device_AMQP_WS_Protocol()->prov_transport_register;
        prov_amqp_ws_transport_get_op_status = Prov_Device_AMQP_WS_Protocol()->prov_transport_get_op_status;
        prov_amqp_ws_transport_dowork = Prov_Device_AMQP_WS_Protocol()->prov_transport_dowork;
        prov_amqp_ws_transport_set_trace = Prov_Device_AMQP_WS_Protocol()->prov_transport_set_trace;
        prov_amqp_ws_transport_x509_cert = Prov_Device_AMQP_WS_Protocol()->prov_transport_x509_cert;
        prov_amqp_ws_transport_trusted_cert = Prov_Device_AMQP_WS_Protocol()->prov_transport_trusted_cert;
        prov_amqp_ws_transport_set_proxy = Prov_Device_AMQP_WS_Protocol()->prov_transport_set_proxy;
        prov_amqp_ws_transport_set_option = Prov_Device_AMQP_WS_Protocol()->prov_transport_set_option;
    }

    TEST_SUITE_CLEANUP(suite_cleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("Could not acquire test serialization mutex.");
        }
        umock_c_reset_all_calls();
        g_transport_io = NULL;
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
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

    TEST_FUNCTION(prov_transport_amqp_create_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, IGNORED_PTR_ARG, on_transport_error, NULL));

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_amqp_ws_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);

        //assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_amqp_transport_ws_io_succeed)
    {
        SASL_MECHANISM_HANDLE sasl_mechanism = { 0 };
        PROV_TRANSPORT_IO_INFO* dps_io_info;
        (void)prov_amqp_ws_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(wsio_get_interface_description());
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(xio_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG));

        //act
        dps_io_info = g_transport_io(TEST_URI_VALUE, &sasl_mechanism, NULL);

        //assert
        ASSERT_IS_NOT_NULL(dps_io_info);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(dps_io_info);
    }

    TEST_FUNCTION(prov_amqp_transport_ws_io_x509_succeed)
    {
        PROV_TRANSPORT_IO_INFO* dps_io_info;
        (void)prov_amqp_ws_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(wsio_get_interface_description());
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG));

        //act
        dps_io_info = g_transport_io(TEST_URI_VALUE, NULL, NULL);

        //assert
        ASSERT_IS_NOT_NULL(dps_io_info);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(dps_io_info);
    }

    TEST_FUNCTION(prov_amqp_transport_ws_io_fail)
    {
        SASL_MECHANISM_HANDLE sasl_mechanism = { 0 };
        PROV_TRANSPORT_IO_INFO* dps_io_info;
        (void)prov_amqp_ws_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        STRICT_EXPECTED_CALL(wsio_get_interface_description());
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(xio_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG));

        size_t calls_cannot_fail[] = { 4 };

        umock_c_negative_tests_snapshot();

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
            sprintf(tmp_msg, "g_transport_io failure in test %zu/%zu", index, count);

            //act
            dps_io_info = g_transport_io(TEST_URI_VALUE, &sasl_mechanism, NULL);

            //assert
            ASSERT_IS_NULL(dps_io_info, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_amqp_transport_ws_io_w_http_proxy_succeed)
    {
        SASL_MECHANISM_HANDLE sasl_mechanism = { 0 };
        HTTP_PROXY_OPTIONS proxy_info;
        PROV_TRANSPORT_IO_INFO* dps_io_info;
        (void)prov_amqp_ws_transport_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(wsio_get_interface_description());
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(http_proxy_io_get_interface_description());
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(xio_create(TEST_INTERFACE_DESC, IGNORED_PTR_ARG));
        proxy_info.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_info.username = TEST_PRIVATE_KEY_VALUE;
        proxy_info.password = TEST_HOST_ADDRESS_VALUE;
        proxy_info.port = 2225;

        //act
        dps_io_info = g_transport_io(TEST_URI_VALUE, &sasl_mechanism, &proxy_info);

        //assert
        ASSERT_IS_NOT_NULL(dps_io_info);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(dps_io_info);
    }

    TEST_FUNCTION(prov_transport_amqp_destroy_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_destroy(TEST_DPS_HANDLE));

        //act
        prov_amqp_ws_transport_destroy(TEST_DPS_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_open_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_open(TEST_DPS_HANDLE, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL));

        //act
        int result = prov_amqp_ws_transport_open(TEST_DPS_HANDLE, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_close_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_close(TEST_DPS_HANDLE));

        //act
        int result = prov_amqp_ws_transport_close(TEST_DPS_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_register_device_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_register_device(TEST_DPS_HANDLE, on_transport_json_parse, on_transport_create_json_payload, NULL));

        //act
        int result = prov_amqp_ws_transport_register_device(TEST_DPS_HANDLE, on_transport_json_parse, on_transport_create_json_payload, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_get_operation_status_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_get_operation_status(TEST_DPS_HANDLE));

        //act
        int result = prov_amqp_ws_transport_get_op_status(TEST_DPS_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_dowork_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_dowork(TEST_DPS_HANDLE));

        //act
        prov_amqp_ws_transport_dowork(TEST_DPS_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_set_trace_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_set_trace(TEST_DPS_HANDLE, true));

        //act
        int result = prov_amqp_ws_transport_set_trace(TEST_DPS_HANDLE, true);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_x509_cert_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_x509_cert(TEST_DPS_HANDLE, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE));

        //act
        int result = prov_amqp_ws_transport_x509_cert(TEST_DPS_HANDLE, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_set_trusted_cert_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_set_trusted_cert(TEST_DPS_HANDLE, TEST_CERT_VALUE));

        //act
        int result = prov_amqp_ws_transport_trusted_cert(TEST_DPS_HANDLE, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_set_proxy_succeed)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;

        STRICT_EXPECTED_CALL(prov_transport_common_amqp_set_proxy(TEST_DPS_HANDLE, &proxy_options));

        //act
        int result = prov_amqp_ws_transport_set_proxy(TEST_DPS_HANDLE, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_amqp_set_option_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(prov_transport_common_amqp_set_option(TEST_DPS_HANDLE, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE));

        //act
        int result = prov_amqp_ws_transport_set_option(TEST_DPS_HANDLE, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    END_TEST_SUITE(prov_transport_amqp_ws_client_ut)
