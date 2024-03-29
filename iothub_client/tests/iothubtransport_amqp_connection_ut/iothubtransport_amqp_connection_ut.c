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
#endif

#if defined _MSC_VER
#pragma warning(disable: 4054) /* MSC incorrectly fires this */
#endif

void* real_malloc(size_t size)
{
    return malloc(size);
}

void real_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"

#ifdef __cplusplus
#include <climits>
#else
#include <limits.h>
#endif

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/strings.h"

#undef ENABLE_MOCK_FILTERING_SWITCH
#define ENABLE_MOCK_FILTERING

#define please_mock_cbs_create MOCK_ENABLED
#define please_mock_cbs_destroy MOCK_ENABLED
#define please_mock_cbs_open_async MOCK_ENABLED
#define please_mock_connection_create2 MOCK_ENABLED
#define please_mock_connection_destroy MOCK_ENABLED
#define please_mock_connection_dowork MOCK_ENABLED
#define please_mock_connection_set_idle_timeout MOCK_ENABLED
#define please_mock_connection_set_remote_idle_timeout_empty_frame_send_ratio MOCK_ENABLED
#define please_mock_connection_set_trace MOCK_ENABLED
#define please_mock_saslclientio_get_interface_description MOCK_ENABLED
#define please_mock_saslmechanism_create MOCK_ENABLED
#define please_mock_saslmechanism_destroy MOCK_ENABLED
#define please_mock_saslmssbcbs_get_interface MOCK_ENABLED
#define please_mock_session_create MOCK_ENABLED
#define please_mock_session_destroy MOCK_ENABLED
#define please_mock_session_set_incoming_window MOCK_ENABLED
#define please_mock_session_set_outgoing_window MOCK_ENABLED
#define please_mock_UniqueId_Generate MOCK_ENABLED

#include "azure_uamqp_c/session.h"
#include "azure_uamqp_c/cbs.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_mssbcbs.h"
#include "azure_uamqp_c/connection.h"

#undef ENABLE_MOCK_FILTERING_SWITCH
#undef ENABLE_MOCK_FILTERING
#undef ENABLE_MOCKS

#include "internal/iothubtransport_amqp_connection.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

#define DEFAULT_INCOMING_WINDOW_SIZE                      UINT_MAX
#define DEFAULT_OUTGOING_WINDOW_SIZE                      100

#define TEST_STRING                                       "Test string!! $%^%2F0x011"
#define TEST_IOTHUB_HOST_FQDN                             "some.fqdn.com"
#define TEST_UNDERLYING_IO_TRANSPORT                      (XIO_HANDLE)0x4444
#define TEST_SASL_INTERFACE_HANDLE                        (SASL_MECHANISM_INTERFACE_DESCRIPTION*)0x4446
#define TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION_HANDLE    (IO_INTERFACE_DESCRIPTION*)0x4447
#define TEST_STRING_HANDLE                                (STRING_HANDLE)0x4448
#define TEST_SASL_MECHANISM_HANDLE                        (SASL_MECHANISM_HANDLE)0x4449
#define TEST_SASL_IO_HANDLE                               (XIO_HANDLE)0x4450
#define TEST_CHAR_PTR                                     (char*)0x4451
#define TEST_CONNECTION_HANDLE                            (CONNECTION_HANDLE)0x4452
#define TEST_UNIQUE_ID                                    "ab345cd00829ef12"
#define TEST_SESSION_HANDLE                               (SESSION_HANDLE)0x4453
#define TEST_CBS_HANDLE                                   (CBS_HANDLE)0x4454

// Helpers
static int saved_malloc_returns_count = 0;
static void* saved_malloc_returns[20];

static void* TEST_malloc(size_t size)
{
    saved_malloc_returns[saved_malloc_returns_count] = real_malloc(size);

    return saved_malloc_returns[saved_malloc_returns_count++];
}

static void* TEST_calloc(size_t num, size_t size)
{
    void* ptr = TEST_malloc(size);
    memset(ptr, 0, size * num);
    return ptr;
}

static void TEST_free(void* ptr)
{
    int i, j;
    for (i = 0, j = 0; j < saved_malloc_returns_count; i++, j++)
    {
        if (saved_malloc_returns[i] == ptr) j++;

        saved_malloc_returns[i] = saved_malloc_returns[j];
    }

    if (i != j) saved_malloc_returns_count--;

    real_free(ptr);
}

static ON_CBS_OPEN_COMPLETE saved_on_cbs_open_complete;
static void* saved_on_cbs_open_complete_context;
static ON_CBS_ERROR saved_on_cbs_error;
static void* saved_on_cbs_error_context;
static int TEST_cbs_open_async(CBS_HANDLE cbs, ON_CBS_OPEN_COMPLETE on_cbs_open_complete, void* on_cbs_open_complete_context, ON_CBS_ERROR on_cbs_error, void* on_cbs_error_context)
{
    (void)cbs;
    saved_on_cbs_open_complete = on_cbs_open_complete;
    saved_on_cbs_open_complete_context = on_cbs_open_complete_context;
    saved_on_cbs_error = on_cbs_error;
    saved_on_cbs_error_context = on_cbs_error_context;
    return 0;
}

static ON_CONNECTION_STATE_CHANGED connection_create2_on_connection_state_changed;
static void* connection_create2_on_connection_state_changed_context;
static ON_IO_ERROR connection_create2_on_io_error;
static void* connection_create2_on_io_error_context;
static CONNECTION_HANDLE TEST_connection_create2_result;
static CONNECTION_HANDLE TEST_connection_create2(XIO_HANDLE xio, const char* hostname, const char* container_id, ON_NEW_ENDPOINT on_new_endpoint, void* callback_context, ON_CONNECTION_STATE_CHANGED on_connection_state_changed, void* on_connection_state_changed_context, ON_IO_ERROR on_io_error, void* on_io_error_context)
{
    (void)xio;
    (void)hostname;
    (void)container_id;
    (void)on_new_endpoint;
    (void)callback_context;

    connection_create2_on_connection_state_changed = on_connection_state_changed;
    connection_create2_on_connection_state_changed_context = on_connection_state_changed_context;
    connection_create2_on_io_error = on_io_error;
    connection_create2_on_io_error_context = on_io_error_context;

    return TEST_connection_create2_result;
}

static const void* on_state_changed_callback_context;
static AMQP_CONNECTION_STATE on_state_changed_callback_previous_state;
static AMQP_CONNECTION_STATE on_state_changed_callback_new_state;
static void on_state_changed_callback(const void* context, AMQP_CONNECTION_STATE previous_state, AMQP_CONNECTION_STATE new_state)
{
    on_state_changed_callback_context = context;
    on_state_changed_callback_previous_state = previous_state;
    on_state_changed_callback_new_state = new_state;
}

static AMQP_CONNECTION_CONFIG global_amqp_connection_config;
static AMQP_CONNECTION_CONFIG* get_amqp_connection_config()
{
    global_amqp_connection_config.iothub_host_fqdn = TEST_IOTHUB_HOST_FQDN;
    global_amqp_connection_config.underlying_io_transport = TEST_UNDERLYING_IO_TRANSPORT;
    global_amqp_connection_config.on_state_changed_callback = on_state_changed_callback;
    global_amqp_connection_config.create_sasl_io = true;
    global_amqp_connection_config.create_cbs_connection = true;
    global_amqp_connection_config.is_trace_on = true;
    global_amqp_connection_config.svc2cl_keep_alive_timeout_secs = 123;
    global_amqp_connection_config.cl2svc_keep_alive_send_ratio   = 0.5;

    return &global_amqp_connection_config;
}

static void set_exp_calls_for_amqp_connection_create(AMQP_CONNECTION_CONFIG* amqp_connection_config)
{
    XIO_HANDLE target_underlying_io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct(amqp_connection_config->iothub_host_fqdn));

    // SASL
    if (amqp_connection_config->create_sasl_io || amqp_connection_config->create_cbs_connection)
    {
        STRICT_EXPECTED_CALL(saslmssbcbs_get_interface());
        STRICT_EXPECTED_CALL(saslmechanism_create(TEST_SASL_INTERFACE_HANDLE, NULL));
        STRICT_EXPECTED_CALL(saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION_HANDLE, IGNORED_PTR_ARG))
            .IgnoreArgument_io_create_parameters()
            .SetReturn(TEST_SASL_IO_HANDLE);
        STRICT_EXPECTED_CALL(xio_setoption(TEST_SASL_IO_HANDLE, "logtrace", IGNORED_PTR_ARG))
            .IgnoreArgument_value();
    }

    // Connection
    EXPECTED_CALL(calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)); // UniqueId container.
    STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, 40)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_STRING_HANDLE)).SetReturn(TEST_IOTHUB_HOST_FQDN);

    if (amqp_connection_config->create_sasl_io || amqp_connection_config->create_cbs_connection)
    {
        target_underlying_io = TEST_SASL_IO_HANDLE;
    }
    else
    {
        target_underlying_io = TEST_UNDERLYING_IO_TRANSPORT;
    }

    STRICT_EXPECTED_CALL(connection_create2(target_underlying_io, TEST_IOTHUB_HOST_FQDN, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_container_id()
        .IgnoreArgument_on_connection_state_changed()
        .IgnoreArgument_on_connection_state_changed_context()
        .IgnoreArgument_on_io_error()
        .IgnoreArgument_on_io_error_context();

    STRICT_EXPECTED_CALL(connection_set_idle_timeout(TEST_CONNECTION_HANDLE, (milliseconds)(1000 * amqp_connection_config->svc2cl_keep_alive_timeout_secs)));
    STRICT_EXPECTED_CALL(connection_set_remote_idle_timeout_empty_frame_send_ratio(TEST_CONNECTION_HANDLE, amqp_connection_config->cl2svc_keep_alive_send_ratio));
    STRICT_EXPECTED_CALL(connection_set_trace(TEST_CONNECTION_HANDLE, amqp_connection_config->is_trace_on));

    EXPECTED_CALL(free(IGNORED_PTR_ARG)); // UniqueId container.

    // Session
    STRICT_EXPECTED_CALL(session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
    STRICT_EXPECTED_CALL(session_set_incoming_window(TEST_SESSION_HANDLE, (uint32_t)DEFAULT_INCOMING_WINDOW_SIZE));
    STRICT_EXPECTED_CALL(session_set_outgoing_window(TEST_SESSION_HANDLE, (uint32_t)DEFAULT_OUTGOING_WINDOW_SIZE));

    // CBS
    if (amqp_connection_config->create_cbs_connection)
    {
        STRICT_EXPECTED_CALL(cbs_create(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(cbs_open_async(TEST_CBS_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void set_exp_calls_for_amqp_connection_destroy(AMQP_CONNECTION_CONFIG* config, AMQP_CONNECTION_HANDLE handle)
{
    if (config->create_cbs_connection)
    {
        STRICT_EXPECTED_CALL(cbs_destroy(TEST_CBS_HANDLE));
    }

    STRICT_EXPECTED_CALL(session_destroy(TEST_SESSION_HANDLE));
    STRICT_EXPECTED_CALL(connection_destroy(TEST_CONNECTION_HANDLE));

    if (config->create_sasl_io || config->create_cbs_connection)
    {
        STRICT_EXPECTED_CALL(xio_destroy(TEST_SASL_IO_HANDLE));
        STRICT_EXPECTED_CALL(saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));
    }

    STRICT_EXPECTED_CALL(STRING_delete(TEST_STRING_HANDLE));
    STRICT_EXPECTED_CALL(free(handle));
}


BEGIN_TEST_SUITE(iothubtransport_amqp_connection_ut)

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
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_CONNECTION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SASL_MECHANISM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_NEW_ENDPOINT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_CONNECTION_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_ERROR, void*);
    REGISTER_UMOCK_ALIAS_TYPE(milliseconds, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(UNIQUEID_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(CONNECTION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CBS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_LINK_ATTACHED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_CBS_OPEN_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_CBS_ERROR, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(calloc, TEST_calloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_RETURN(cbs_create, TEST_CBS_HANDLE);
    REGISTER_GLOBAL_MOCK_HOOK(cbs_open_async, TEST_cbs_open_async);
    REGISTER_GLOBAL_MOCK_HOOK(connection_create2, TEST_connection_create2);


    REGISTER_GLOBAL_MOCK_RETURN(saslmssbcbs_get_interface, TEST_SASL_INTERFACE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslmssbcbs_get_interface, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(saslmechanism_create, TEST_SASL_MECHANISM_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslmechanism_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(saslclientio_get_interface_description, TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslclientio_get_interface_description, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(UniqueId_Generate, UNIQUEID_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UniqueId_Generate, UNIQUEID_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(connection_create2, TEST_CONNECTION_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connection_create2, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(connection_set_idle_timeout, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connection_set_idle_timeout, 1);

    REGISTER_GLOBAL_MOCK_RETURN(connection_set_remote_idle_timeout_empty_frame_send_ratio, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connection_set_remote_idle_timeout_empty_frame_send_ratio, 1);

    REGISTER_GLOBAL_MOCK_RETURN(session_create, TEST_SESSION_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(session_set_incoming_window, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_set_incoming_window, 1);

    REGISTER_GLOBAL_MOCK_RETURN(session_set_outgoing_window, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_set_outgoing_window, 1);

    REGISTER_GLOBAL_MOCK_RETURN(cbs_create, TEST_CBS_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(cbs_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(cbs_open_async, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(cbs_open_async, 1);

    REGISTER_GLOBAL_MOCK_RETURN(xio_setoption, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_setoption, 1);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_IOTHUB_HOST_FQDN);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

static void reset_test_data()
{
    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));
    saved_on_cbs_open_complete_context = NULL;
    saved_on_cbs_error_context = NULL;
    connection_create2_on_connection_state_changed_context = NULL;
    connection_create2_on_io_error_context = NULL;
    on_state_changed_callback_context = NULL;
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();

    TEST_connection_create2_result = TEST_CONNECTION_HANDLE;
    reset_test_data();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(amqp_connection_create_NULL_config)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void*)handle, NULL);

    // cleanup
}

TEST_FUNCTION(amqp_connection_create_NULL_iothub_fqdn)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();
    config->iothub_host_fqdn = NULL;

    umock_c_reset_all_calls();

    // act
    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void*)handle, NULL);

    // cleanup
}

TEST_FUNCTION(amqp_connection_create_NULL_underlying_io_transport)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();
    config->underlying_io_transport = NULL;

    // act
    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void*)handle, NULL);

    // cleanup
}






TEST_FUNCTION(amqp_connection_create_SASL_and_CBS_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    // act
    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void*)handle, (void*)saved_malloc_returns[0]);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_create_SASL_only_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();
    config->create_cbs_connection = false;

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    // act
    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void*)handle, (void*)saved_malloc_returns[0]);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_create_no_SASL_no_CBS_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();
    config->create_sasl_io = false;
    config->create_cbs_connection = false;

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    // act
    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void*)handle, (void*)saved_malloc_returns[0]);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_create_SASL_and_CBS_negative_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);
    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[128];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        TEST_connection_create2_result = TEST_CONNECTION_HANDLE;

        if (i == 2  || // saslmssbcbs_get_interface
            i == 4  || // saslclientio_get_interface_description
            i == 9  || // STRING_c_str(instance->iothub_fqdn) for connection_create2
            i == 13 || // connection_set_trace
            i == 14 || // free(unique_container_id)
            i == 16 || // session_set_incoming_window
            i == 17)   // session_set_outgoing_window
        {
            continue; // these lines have functions that do not return anything (void) or do not cause failures.
        }
        else if (i == 10) // connection_create2
        {
            TEST_connection_create2_result = NULL;
        }

        AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(handle, error_msg);
    }

    // cleanup
    umock_c_negative_tests_reset();
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(amqp_connection_create_on_connection_state_changed)
{
    // arrange
    const int NUMBER_OF_TRANSITIONS = 8;

    CONNECTION_STATE previous_connection_state[] = {
        CONNECTION_STATE_END,           // 0
        CONNECTION_STATE_OPENED,        // 1
        CONNECTION_STATE_ERROR,         // 2
        CONNECTION_STATE_END,           // 3
        CONNECTION_STATE_ERROR,         // 4
        CONNECTION_STATE_DISCARDING,    // 5
        CONNECTION_STATE_END,           // 6
        CONNECTION_STATE_OPENED,        // 7
    };

    CONNECTION_STATE new_connection_state[] = {
        CONNECTION_STATE_OPENED,        // 0
        CONNECTION_STATE_ERROR,         // 1
        CONNECTION_STATE_END,           // 2
        CONNECTION_STATE_ERROR,         // 3
        CONNECTION_STATE_DISCARDING,    // 4
        CONNECTION_STATE_END,           // 5
        CONNECTION_STATE_OPENED,        // 6
        CONNECTION_STATE_DISCARDING,    // 7
    };

    AMQP_CONNECTION_STATE previous_amqp_connection_state[] = {
        AMQP_CONNECTION_STATE_CLOSED,   // 0
        AMQP_CONNECTION_STATE_OPENED,   // 1
        AMQP_CONNECTION_STATE_ERROR,    // 2
        AMQP_CONNECTION_STATE_CLOSED,   // 3
        AMQP_CONNECTION_STATE_CLOSED,   // 4
        AMQP_CONNECTION_STATE_ERROR,    // 5
        AMQP_CONNECTION_STATE_CLOSED,   // 6
        AMQP_CONNECTION_STATE_OPENED,   // 7
    };

    AMQP_CONNECTION_STATE new_amqp_connection_state[] = {
        AMQP_CONNECTION_STATE_OPENED,   // 0
        AMQP_CONNECTION_STATE_ERROR,    // 1
        AMQP_CONNECTION_STATE_CLOSED,   // 2
        AMQP_CONNECTION_STATE_ERROR,    // 3
        AMQP_CONNECTION_STATE_ERROR,    // 4
        AMQP_CONNECTION_STATE_CLOSED,   // 5
        AMQP_CONNECTION_STATE_OPENED,   // 6
        AMQP_CONNECTION_STATE_ERROR,    // 7
    };

    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    connection_create2_on_connection_state_changed = NULL;
    connection_create2_on_connection_state_changed_context = NULL;

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    ASSERT_IS_NOT_NULL(connection_create2_on_connection_state_changed);

    // act
    int i;
    for (i = 0; i < NUMBER_OF_TRANSITIONS; i++)
    {
        // act
        connection_create2_on_connection_state_changed(handle, new_connection_state[i], previous_connection_state[i]);

        // assert
        ASSERT_ARE_EQUAL(int, on_state_changed_callback_previous_state, previous_amqp_connection_state[i]);
        ASSERT_ARE_EQUAL(int, on_state_changed_callback_new_state, new_amqp_connection_state[i]);
    }

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_create_no_sasl_no_cbs_on_connection_state_changed)
{
    // arrange
    const int NUMBER_OF_TRANSITIONS = 8;

    CONNECTION_STATE previous_connection_state[] = {
        CONNECTION_STATE_END,        // 0
        CONNECTION_STATE_START,      // 1
        CONNECTION_STATE_ERROR,      // 2
        CONNECTION_STATE_END,        // 3
        CONNECTION_STATE_ERROR,      // 4
        CONNECTION_STATE_DISCARDING, // 5
        CONNECTION_STATE_END,        // 6
        CONNECTION_STATE_START       // 7
    };

    CONNECTION_STATE new_connection_state[] = {
        CONNECTION_STATE_START,      // 0
        CONNECTION_STATE_ERROR,      // 1
        CONNECTION_STATE_END,        // 2
        CONNECTION_STATE_ERROR,      // 3
        CONNECTION_STATE_DISCARDING, // 4
        CONNECTION_STATE_END,        // 5
        CONNECTION_STATE_START,      // 6
        CONNECTION_STATE_DISCARDING  // 8
    };

    AMQP_CONNECTION_STATE previous_amqp_connection_state[] = {
        AMQP_CONNECTION_STATE_CLOSED,  // 0
        AMQP_CONNECTION_STATE_OPENED,  // 1
        AMQP_CONNECTION_STATE_ERROR,   // 2
        AMQP_CONNECTION_STATE_CLOSED,  // 3
        AMQP_CONNECTION_STATE_CLOSED,  // 4
        AMQP_CONNECTION_STATE_ERROR,   // 5
        AMQP_CONNECTION_STATE_CLOSED,  // 6
        AMQP_CONNECTION_STATE_OPENED,  // 7
    };

    AMQP_CONNECTION_STATE new_amqp_connection_state[] = {
        AMQP_CONNECTION_STATE_OPENED,  // 0
        AMQP_CONNECTION_STATE_ERROR,   // 1
        AMQP_CONNECTION_STATE_CLOSED,  // 2
        AMQP_CONNECTION_STATE_ERROR,   // 3
        AMQP_CONNECTION_STATE_ERROR,   // 4
        AMQP_CONNECTION_STATE_CLOSED,  // 5
        AMQP_CONNECTION_STATE_OPENED,  // 6
        AMQP_CONNECTION_STATE_ERROR,   // 7
    };

    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();
    config->create_sasl_io = false;
    config->create_cbs_connection = false;

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    connection_create2_on_connection_state_changed = NULL;
    connection_create2_on_connection_state_changed_context = NULL;

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    ASSERT_IS_NOT_NULL(connection_create2_on_connection_state_changed);

    // act
    int i;
    for (i = 0; i < NUMBER_OF_TRANSITIONS; i++)
    {
        // act
        connection_create2_on_connection_state_changed(handle, new_connection_state[i], previous_connection_state[i]);

        // assert
        ASSERT_ARE_EQUAL(int, on_state_changed_callback_previous_state, previous_amqp_connection_state[i]);
        ASSERT_ARE_EQUAL(int, on_state_changed_callback_new_state, new_amqp_connection_state[i]);
    }

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_create_on_io_error)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    connection_create2_on_connection_state_changed = NULL;
    connection_create2_on_connection_state_changed_context = NULL;
    on_state_changed_callback_previous_state = AMQP_CONNECTION_STATE_CLOSED;
    on_state_changed_callback_new_state = AMQP_CONNECTION_STATE_CLOSED;

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    ASSERT_IS_NOT_NULL(connection_create2_on_io_error);

    // act
    connection_create2_on_io_error(handle);

    // assert
    ASSERT_ARE_EQUAL(int, on_state_changed_callback_previous_state, AMQP_CONNECTION_STATE_CLOSED);
    ASSERT_ARE_EQUAL(int, on_state_changed_callback_new_state, AMQP_CONNECTION_STATE_ERROR);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_destroy_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    amqp_connection_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(amqp_connection_destroy_SASL_and_CBS_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_destroy(config, handle);

    // act
    amqp_connection_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(amqp_connection_destroy_no_SASL_no_CBS_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();
    config->create_sasl_io = false;
    config->create_cbs_connection = false;

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_destroy(config, handle);

    // act
    amqp_connection_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(amqp_connection_do_work_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    amqp_connection_do_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(amqp_connection_do_work_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(connection_dowork(TEST_CONNECTION_HANDLE));

    // act
    amqp_connection_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_get_session_handle_NULL_handle)
{
    // arrange
    SESSION_HANDLE session_handle;

    // act
    int result = amqp_connection_get_session_handle(NULL, &session_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(amqp_connection_get_session_handle_NULL_session_handle)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();

    // act
    int result = amqp_connection_get_session_handle(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_get_session_handle_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();

    SESSION_HANDLE session_handle;

    // act
    int result = amqp_connection_get_session_handle(handle, &session_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, session_handle, TEST_SESSION_HANDLE);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_get_cbs_handle_NULL_handle)
{
    // arrange
    CBS_HANDLE cbs_handle;

    // act
    int result = amqp_connection_get_cbs_handle(NULL, &cbs_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(amqp_connection_get_cbs_handle_NULL_cbs_handle)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();

    // act
    int result = amqp_connection_get_cbs_handle(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_get_cbs_handle_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();

    CBS_HANDLE cbs_handle;

    // act
    int result = amqp_connection_get_cbs_handle(handle, &cbs_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, cbs_handle, TEST_CBS_HANDLE);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_get_cbs_handle_no_CBS)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();
    config->create_sasl_io = false;
    config->create_cbs_connection = false;

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();

    CBS_HANDLE cbs_handle = NULL;

    // act
    int result = amqp_connection_get_cbs_handle(handle, &cbs_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, cbs_handle, NULL);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_set_logging_NULL_handle)
{
    // act
    int result = amqp_connection_set_logging(NULL, true);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(amqp_connection_set_logging_SASL_and_CBS_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(xio_setoption(TEST_SASL_IO_HANDLE, "logtrace", IGNORED_PTR_ARG)).IgnoreArgument(3);
    STRICT_EXPECTED_CALL(connection_set_trace(TEST_CONNECTION_HANDLE, true));

    // act
    int result = amqp_connection_set_logging(handle, true);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_set_logging_SASL_and_CBS_xio_setoption_fails)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(xio_setoption(TEST_SASL_IO_HANDLE, "logtrace", IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .SetReturn(1);

    // act
    int result = amqp_connection_set_logging(handle, true);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    amqp_connection_destroy(handle);
}

TEST_FUNCTION(amqp_connection_set_logging_no_SASL_no_CBS_success)
{
    // arrange
    AMQP_CONNECTION_CONFIG* config = get_amqp_connection_config();
    config->create_sasl_io = false;
    config->create_cbs_connection = false;

    umock_c_reset_all_calls();
    set_exp_calls_for_amqp_connection_create(config);

    AMQP_CONNECTION_HANDLE handle = amqp_connection_create(config);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(connection_set_trace(TEST_CONNECTION_HANDLE, true));

    // act
    int result = amqp_connection_set_logging(handle, true);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
    amqp_connection_destroy(handle);
}

END_TEST_SUITE(iothubtransport_amqp_connection_ut)
