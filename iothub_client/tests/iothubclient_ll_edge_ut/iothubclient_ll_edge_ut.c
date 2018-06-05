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

void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umock_c_negative_tests.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/envvariable.h"
#include "iothub_client_ll.h"
#include "iothub_client_private.h"
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_c_shared_utility/gballoc.h"
#include "iothub_module_client_ll.h"
#undef ENABLE_MOCKS

#include "iothub_client_ll_edge.h"


static int bool_Compare(bool left, bool right)
{
    return left != right;
}

static void bool_ToString(char* string, size_t bufferSize, bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}

#ifndef __cplusplus
static int _Bool_Compare(_Bool left, _Bool right)
{
    return left != right;
}

static void _Bool_ToString(char* string, size_t bufferSize, _Bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}
#endif

#define TEST_ENV_IOTHUB_NAME "edgehubtest1"
#define TEST_ENV_IOTHUB_SUFFIX "azure-devices.net"
static const char* TEST_ENV_AUTHSCHEME = "sasToken";
static const char* TEST_ENV_AUTHSCHEME_INVALID = "SasToken_Invalid";
static const char* TEST_ENV_DEVICEID = "Test_DeviceId";
static const char* TEST_ENV_HOSTNAME = TEST_ENV_IOTHUB_NAME "." TEST_ENV_IOTHUB_SUFFIX;
static const char* TEST_ENV_EDGEGATEWAY = "127.0.0.1";
static const char* TEST_ENV_MODULEID = "Test_ModuleId";
static const char* TEST_ENV_HOSTNAME_NO_SEPARATOR = "there-is-no-period-here-not-legal-host";
static const char* TEST_ENV_HOSTNAME_NO_CONTENT_POST_SEPARATOR = "no-suffix-specified.";
static const char* TEST_ENV_CONNECTION_STRING = "Test_ConnectionString";

static const IOTHUB_CLIENT_LL_HANDLE TEST_CLIENT_HANDLE_FROM_CONNSTR =  (IOTHUB_CLIENT_LL_HANDLE)1;
static const IOTHUB_CLIENT_LL_HANDLE TEST_CLIENT_HANDLE_FROM_CREATE_MOD_INTERNAL =  (IOTHUB_CLIENT_LL_HANDLE)2;

static const IOTHUB_CLIENT_TRANSPORT_PROVIDER TEST_TRANSPORT_PROVIDER = (IOTHUB_CLIENT_TRANSPORT_PROVIDER)0x1110;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

IOTHUB_CLIENT_LL_HANDLE test_IoTHubClientCore_LL_CreateFromEnvironment(const IOTHUB_CLIENT_CONFIG* config, const char* module_id)
{
    ASSERT_ARE_EQUAL_WITH_MSG(bool, true, (config->protocol == TEST_TRANSPORT_PROVIDER), "Protocol to configure does not mach");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, strcmp(TEST_ENV_DEVICEID, config->deviceId), "DeviceIds don't match");
    ASSERT_ARE_EQUAL_WITH_MSG(bool, true, (config->deviceKey == NULL) ? true : false, "deviceKey is not NULL");
    ASSERT_ARE_EQUAL_WITH_MSG(bool, true, (config->deviceSasToken == NULL) ? true : false, "deviceSasToken is not NULL");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, strcmp(TEST_ENV_IOTHUB_NAME, config->iotHubName), "IoTHub names don't match");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, strcmp(TEST_ENV_IOTHUB_SUFFIX, config->iotHubSuffix), "IoTHub suffixes don't match");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, strcmp(TEST_ENV_EDGEGATEWAY, config->protocolGatewayHostName), "Protocol gateway hosts don't match");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, strcmp(TEST_ENV_MODULEID, module_id), "ModuleIds don't match");
    return TEST_CLIENT_HANDLE_FROM_CREATE_MOD_INTERNAL;
}

static int test_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

TEST_DEFINE_ENUM_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);


BEGIN_TEST_SUITE(iothubclient_ll_edge_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    umock_c_init(on_umock_c_error);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_GLOBAL_MOCK_RETURN(environment_get_variable, "");
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(environment_get_variable, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubModuleClient_LL_CreateFromConnectionString, (IOTHUB_MODULE_CLIENT_LL_HANDLE)1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubModuleClient_LL_CreateFromConnectionString, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_CreateFromEnvironment, test_IoTHubClientCore_LL_CreateFromEnvironment);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_CreateFromEnvironment, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, test_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 1);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(iothub_security_init, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(iothub_security_init, 1);

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_TRANSPORT_PROVIDER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MODULE_CLIENT_LL_HANDLE, void*);

    REGISTER_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
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


static void set_expected_environment_var_setup(const char* hostname_env, bool invoke_security_init)
{
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_AUTHSCHEME);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_DEVICEID);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(hostname_env);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_EDGEGATEWAY);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_MODULEID);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (invoke_security_init)
    {      
        STRICT_EXPECTED_CALL(iothub_security_init(IOTHUB_SECURITY_TYPE_HTTP_EDGE));
    }
}

// If the connection string environment variable is set, ignore all other environment variables and invoke IoTHubClient_LL_CreateFromConnectionString
TEST_FUNCTION(IoTHubClient_LL_CreateForModule_use_connection_string)
{
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_CONNECTION_STRING);
    STRICT_EXPECTED_CALL(IoTHubModuleClient_LL_CreateFromConnectionString(TEST_ENV_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER)).SetReturn(TEST_CLIENT_HANDLE_FROM_CONNSTR);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    IOTHUB_MODULE_CLIENT_LL_HANDLE h = IoTHubModuleClient_LL_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER);
    ASSERT_ARE_EQUAL_WITH_MSG(bool, true, (h == TEST_CLIENT_HANDLE_FROM_CONNSTR), "IoTHubModuleClient_LL_CreateFromEnvironment returns wrong handle");

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// All environment variables are specified
TEST_FUNCTION(IoTHubModuleClient_LL_CreateFromEnvironment_success)
{
    set_expected_environment_var_setup(TEST_ENV_HOSTNAME, true);
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateFromEnvironment(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    IOTHUB_MODULE_CLIENT_LL_HANDLE h = IoTHubModuleClient_LL_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER);
    ASSERT_ARE_EQUAL_WITH_MSG(bool, true, (h == TEST_CLIENT_HANDLE_FROM_CREATE_MOD_INTERNAL), "IoTHubModuleClient_LL_CreateFromEnvironment returns wrong handle");

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// The authscheme environment variable must be TEST_ENV_AUTHSCHEME, anything else causes a failure.
TEST_FUNCTION(IoTHubModuleClient_LL_CreateFromEnvironment_fail_bad_authscheme)
{
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_ENV_AUTHSCHEME_INVALID);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));


    IOTHUB_MODULE_CLIENT_LL_HANDLE h = IoTHubModuleClient_LL_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER);
    ASSERT_IS_NULL(h);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// The hostname must be in format "<hostName>.<suffix>".  Pass in with no "."
TEST_FUNCTION(IoTHubModuleClient_LL_CreateFromEnvironment_fail_no_hostname_separator)
{
    set_expected_environment_var_setup(TEST_ENV_HOSTNAME_NO_SEPARATOR, false);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    
    IOTHUB_MODULE_CLIENT_LL_HANDLE h = IoTHubModuleClient_LL_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER);
    ASSERT_IS_NULL(h);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// The hostname must be in format "<hostName>.<suffix>".  Pass in with no content after "."
TEST_FUNCTION(IoTHubModuleClient_LL_CreateFromEnvironment_fail_no_hostname_content_post_separator)
{
    set_expected_environment_var_setup(TEST_ENV_HOSTNAME_NO_CONTENT_POST_SEPARATOR, false);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    IOTHUB_MODULE_CLIENT_LL_HANDLE h = IoTHubModuleClient_LL_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER);
    ASSERT_IS_NULL(h);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// The hostname must be in format "<hostName>.<suffix>".  Pass in with no content after "."
TEST_FUNCTION(IoTHubModuleClient_LL_CreateFromEnvironment_failures)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_environment_var_setup(TEST_ENV_HOSTNAME, true);
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateFromEnvironment(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = {
        0, // Skip the call for retrieving connection string; in typical case it will return NULL
        9, // gballoc_free
    };

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
        

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubModuleClient_LL_CreateFromEnvironment failure in test %zu/%zu", index, count);
        IOTHUB_MODULE_CLIENT_LL_HANDLE h = IoTHubModuleClient_LL_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER);

        // assert
        ASSERT_IS_NULL_WITH_MSG(h, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(iothubclient_ll_edge_ut)
