// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <ctime>
#include <cstring>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
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

static int real_strcmp(const char* str1, const char* str2)
{
    return strcmp(str1, str2);
}

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"

#ifdef __cplusplus
#include <climits>
#else
#include <limits.h>
#endif

#define ENABLE_MOCKS
#include "iothub_transport_ll.h"
#include "azure_uamqp_c/async_operation.h"
#include "azure_c_shared_utility/optionhandler.h"

#undef ENABLE_MOCK_FILTERING_SWITCH
#define ENABLE_MOCK_FILTERING

#define please_mock_cbs_put_token_async MOCK_ENABLED
#include "azure_uamqp_c/cbs.h"

#undef ENABLE_MOCK_FILTERING_SWITCH
#undef ENABLE_MOCK_FILTERING

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"
#include "internal/iothub_client_authorization.h"
#undef ENABLE_MOCKS

#include "internal/iothubtransport_amqp_cbs_auth.h"

MOCKABLE_FUNCTION(, time_t, get_time, time_t*, currentTime);
MOCKABLE_FUNCTION(, double, get_difftime, time_t, stopTime, time_t, startTime);

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

// Control parameters

#define DEFAULT_CBS_REQUEST_TIMEOUT_SECS                  UINT32_MAX
#define DEFAULT_SAS_TOKEN_LIFETIME_SECS                   3600
#define DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS               1800
#define INDEFINITE_TIME                                   ((time_t)(-1))
#define TEST_GENERIC_CHAR_PTR                             "generic char* string"
#define TEST_DEVICE_ID                                    "my_device"
#define TEST_MODULE_ID                                    "my_module"
#define TEST_DEVICE_ID_STRING_HANDLE                      (STRING_HANDLE)0x4442
#define TEST_IOTHUB_HOST_FQDN                             "some.fqdn.com"
#define TEST_IOTHUB_HOST_FQDN_STRING_HANDLE               (STRING_HANDLE)0x4443
#define TEST_ON_STATE_CHANGED_CALLBACK_CONTEXT            (void*)0x4444
#define TEST_ON_ERROR_CALLBACK_CONTEXT                    (void*)0x4445
#define TEST_USER_DEFINED_SAS_TOKEN                       "blablabla"
#define TEST_USER_DEFINED_SAS_TOKEN_STRING_HANDLE         (STRING_HANDLE)0x4446
#define TEST_GENERATED_SAS_TOKEN                          "blablabla2"
#define TEST_GENERATED_SAS_TOKEN_STRING_HANDLE            (STRING_HANDLE)0x4447
#define TEST_PRIMARY_DEVICE_KEY                           "MUhT4tkv1auVqZFQC0lyuHFf6dec+ZhWCgCZ0HcNPuW="
#define TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE             (STRING_HANDLE)0x4449
#define TEST_SECONDARY_DEVICE_KEY                         "WCgCZ0HcNPuWMUhTdec+ZhVqZFQC4tkv1auHFf60lyu="
#define TEST_SECONDARY_DEVICE_KEY_STRING_HANDLE           (STRING_HANDLE)0x4450
#define TEST_STRING_HANDLE                                (STRING_HANDLE)0x4451
#define TEST_CBS_HANDLE                                   (CBS_HANDLE)0x4452
#define TEST_DEVICES_PATH                                 "some.fqdn.com/devices/my_device"
#define TEST_DEVICES_PATH_STRING_HANDLE                   (STRING_HANDLE)0x4453
#define SAS_TOKEN_TYPE                                    "servicebus.windows.net:sastoken"
#define TEST_OPTIONHANDLER_HANDLE                         (OPTIONHANDLER_HANDLE)0x4455
#define TEST_AUTHORIZATION_MODULE_HANDLE                  (IOTHUB_AUTHORIZATION_HANDLE)0x4456
#define TEST_PUT_TOKEN_RESULT                             (ASYNC_OPERATION_HANDLE)0x4457


static AUTHENTICATION_CONFIG global_auth_config;

// Function Hooks

static int saved_malloc_returns_count = 0;
static void* saved_malloc_returns[20];

static void* TEST_malloc(size_t size)
{
    saved_malloc_returns[saved_malloc_returns_count] = real_malloc(size);

    return saved_malloc_returns[saved_malloc_returns_count++];
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

static CBS_HANDLE saved_cbs_put_token_cbs;
static const char* saved_cbs_put_token_type;
static const char* saved_cbs_put_token_audience;
static const char* saved_cbs_put_token_token;
static ON_CBS_OPERATION_COMPLETE saved_cbs_put_token_on_operation_complete;
static void* saved_cbs_put_token_context;
static ASYNC_OPERATION_HANDLE TEST_cbs_put_token_async_return = TEST_PUT_TOKEN_RESULT;

static ASYNC_OPERATION_HANDLE TEST_cbs_put_token_async(CBS_HANDLE cbs, const char* type, const char* audience, const char* token, ON_CBS_OPERATION_COMPLETE on_operation_complete, void* context)
{
    saved_cbs_put_token_cbs = cbs;
    saved_cbs_put_token_type = type;
    saved_cbs_put_token_audience = audience;
    saved_cbs_put_token_token = token;
    saved_cbs_put_token_on_operation_complete = on_operation_complete;
    saved_cbs_put_token_context = context;

    return TEST_cbs_put_token_async_return;
}

static char* TEST_IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, uint64_t expiry_time_relative_seconds, const char* keyname)
{
    (void)handle;
    (void)scope;
    (void)expiry_time_relative_seconds;
    (void)keyname;

    char* result;
    size_t len = strlen(TEST_USER_DEFINED_SAS_TOKEN);
    result = (char*)real_malloc(len+1);
    strcpy(result, TEST_USER_DEFINED_SAS_TOKEN);
    return result;
}

#ifdef __cplusplus
extern "C"
{
#endif

static int g_STRING_sprintf_call_count;
static int g_STRING_sprintf_fail_on_count;
static STRING_HANDLE saved_STRING_sprintf_handle;

STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
{
    (void)format;
    return TEST_DEVICES_PATH_STRING_HANDLE;
}

int my_size_tToString(char* destination, size_t destinationSize, size_t value)
{
    (void)destinationSize;
    (void)value;
    sprintf(destination, "12345");
    return 0;
}

int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
{
    int result;
    saved_STRING_sprintf_handle = handle;
    (void)format;

    g_STRING_sprintf_call_count++;

    if (g_STRING_sprintf_call_count == g_STRING_sprintf_fail_on_count)
    {
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

#ifdef __cplusplus
}
#endif

static void register_umock_alias_types()
{
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AUTHENTICATION_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(CBS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_CBS_OPERATION_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(AUTHENTICATION_ERROR_CODE, int);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(pfCloneOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfDestroyOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfSetOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SAS_TOKEN_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CREDENTIAL_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(ASYNC_OPERATION_HANDLE, void*);
}

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(cbs_put_token_async, TEST_cbs_put_token_async);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_SasToken, TEST_IoTHubClient_Auth_Get_SasToken);
}

static void register_global_mock_returns()
{
    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_GENERIC_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(get_time, INDEFINITE_TIME);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_Create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_AddOption, OPTIONHANDLER_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceId, TEST_DEVICE_ID);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Is_SasToken_Valid, SAS_TOKEN_STATUS_VALID);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_SasToken_Expiry, 3600);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_ModuleId, NULL);

    
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_OK);
}

// Auxiliary Functions
typedef struct AUTHENTICATION_DO_WORK_EXPECTED_STATE_TAG
{
    AUTHENTICATION_STATE current_state;
    bool is_cbs_put_token_in_progress;
    bool is_sas_token_refresh_in_progress;
    time_t current_sas_token_put_time;
    STRING_HANDLE sas_token_to_use;
    uint64_t sastoken_expiration_time;
    uint64_t sas_token_refresh_time_in_seconds;
} AUTHENTICATION_DO_WORK_EXPECTED_STATE;

static AUTHENTICATION_DO_WORK_EXPECTED_STATE g_auth_do_work_exp_state;

static AUTHENTICATION_DO_WORK_EXPECTED_STATE* get_do_work_expected_state_struct()
{
    memset(&g_auth_do_work_exp_state, 0, sizeof(AUTHENTICATION_DO_WORK_EXPECTED_STATE));
    return &g_auth_do_work_exp_state;
}

static void* saved_on_state_changed_callback_context;
static AUTHENTICATION_STATE saved_on_state_changed_callback_previous_state;
static AUTHENTICATION_STATE saved_on_state_changed_callback_new_state;
static void TEST_on_state_changed_callback(void* context, AUTHENTICATION_STATE previous_state, AUTHENTICATION_STATE new_state)
{
    saved_on_state_changed_callback_context = context;
    saved_on_state_changed_callback_previous_state = previous_state;
    saved_on_state_changed_callback_new_state = new_state;
}

static void* saved_on_error_callback_context;
static AUTHENTICATION_ERROR_CODE saved_on_error_callback_error_code;
static void TEST_on_error_callback(void* context, AUTHENTICATION_ERROR_CODE error_code)
{
    saved_on_error_callback_context = context;
    saved_on_error_callback_error_code = error_code;
}

typedef enum USE_DEVICE_KEYS_OR_SAS_TOKEN_OPTION_TAG
{
    USE_DEVICE_KEYS,
    USE_DEVICE_SAS_TOKEN,
    USE_DEVICE_AUTH
} USE_DEVICE_KEYS_OR_SAS_TOKEN_OPTION;

static AUTHENTICATION_CONFIG* get_auth_config(USE_DEVICE_KEYS_OR_SAS_TOKEN_OPTION credential_option)
{
    memset(&global_auth_config, 0, sizeof(AUTHENTICATION_CONFIG));
    global_auth_config.device_id = TEST_DEVICE_ID;

    (void)credential_option;
    global_auth_config.iothub_host_fqdn = TEST_IOTHUB_HOST_FQDN;
    global_auth_config.on_state_changed_callback = TEST_on_state_changed_callback;
    global_auth_config.on_state_changed_callback_context = TEST_ON_STATE_CHANGED_CALLBACK_CONTEXT;
    global_auth_config.on_error_callback = TEST_on_error_callback;
    global_auth_config.on_error_callback_context = TEST_ON_ERROR_CALLBACK_CONTEXT;
    global_auth_config.authorization_module = TEST_AUTHORIZATION_MODULE_HANDLE;
    return &global_auth_config;
}

static time_t add_seconds(time_t base_time, unsigned int seconds)
{
    time_t new_time;
    struct tm *bd_new_time;

    if ((bd_new_time = localtime(&base_time)) == NULL)
    {
        new_time = INDEFINITE_TIME;
    }
    else
    {
        bd_new_time->tm_sec += seconds;
        new_time = mktime(bd_new_time);
    }

    return new_time;
}

static void set_expected_calls_for_authentication_create(AUTHENTICATION_CONFIG* config, bool testing_modules)
{
    (void)config;

    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceId(TEST_AUTHORIZATION_MODULE_HANDLE));
    STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUB_HOST_FQDN)).SetReturn(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE);

    if (testing_modules)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_ModuleId(TEST_AUTHORIZATION_MODULE_HANDLE)).SetReturn(TEST_MODULE_ID);
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_ModuleId(TEST_AUTHORIZATION_MODULE_HANDLE)).SetReturn(NULL);
    }
}

static void set_expected_calls_for_authentication_destroy(AUTHENTICATION_HANDLE handle)
{
    STRICT_EXPECTED_CALL(STRING_delete(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));
    STRICT_EXPECTED_CALL(free(handle));
}

static void set_expected_calls_for_put_SAS_token_to_cbs(AUTHENTICATION_HANDLE handle, time_t current_time, STRING_HANDLE sas_token)
{
    char* sas_token_char_ptr;
    IOTHUB_CREDENTIAL_TYPE cred_type;

    if (sas_token == TEST_USER_DEFINED_SAS_TOKEN_STRING_HANDLE)
    {
        cred_type = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
        sas_token_char_ptr = TEST_USER_DEFINED_SAS_TOKEN;
    }
    else
    {
        cred_type = IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY;
        sas_token_char_ptr = TEST_GENERATED_SAS_TOKEN;
    }

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(TEST_AUTHORIZATION_MODULE_HANDLE)).SetReturn(cred_type);
    if (sas_token == TEST_USER_DEFINED_SAS_TOKEN_STRING_HANDLE)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Is_SasToken_Valid(TEST_AUTHORIZATION_MODULE_HANDLE));
    }
    else
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken(TEST_AUTHORIZATION_MODULE_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICES_PATH_STRING_HANDLE)).SetReturn(TEST_DEVICES_PATH);
    STRICT_EXPECTED_CALL(cbs_put_token_async(TEST_CBS_HANDLE, SAS_TOKEN_TYPE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, handle));
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
}

static void set_expected_calls_for_create_and_put_sas_token(AUTHENTICATION_HANDLE handle, time_t current_time, AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_context)
{
    (void)exp_context;
    STRICT_EXPECTED_CALL(get_time(NULL)) //2
        .SetReturn(current_time);
    STRICT_EXPECTED_CALL(get_difftime(current_time, (time_t)0))
        .SetReturn(difftime(current_time, (time_t)0));
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));
    set_expected_calls_for_put_SAS_token_to_cbs(handle, current_time, TEST_GENERATED_SAS_TOKEN_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICES_PATH_STRING_HANDLE));
}

static void set_expected_calls_for_authentication_do_work(AUTHENTICATION_CONFIG* config, AUTHENTICATION_HANDLE handle, time_t current_time, AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_context, IOTHUB_CREDENTIAL_TYPE cred_type)
{
    (void)config;
    (void)handle;

    if (exp_context->is_cbs_put_token_in_progress)
    {
        if (exp_context->is_sas_token_refresh_in_progress)
        {
        }
        else
        {
            if (!exp_context->is_cbs_put_token_in_progress)
            {
                STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
            }
            STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
            double actual_difftime_result = difftime(current_time, exp_context->current_sas_token_put_time);
            STRICT_EXPECTED_CALL(get_difftime(current_time, IGNORED_NUM_ARG)).SetReturn(actual_difftime_result);
        }
    }
    else if (exp_context->current_state == AUTHENTICATION_STATE_STARTING)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));
        set_expected_calls_for_put_SAS_token_to_cbs(handle, current_time, exp_context->sas_token_to_use);
        STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICES_PATH_STRING_HANDLE));
    }
    else if (exp_context->current_state == AUTHENTICATION_STATE_STARTED)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(cred_type);
        if (!exp_context->is_cbs_put_token_in_progress)
        {
            STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_SasToken_Expiry(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
        STRICT_EXPECTED_CALL(get_difftime(current_time, exp_context->current_sas_token_put_time));
    }
}

static void crank_authentication_do_work(AUTHENTICATION_CONFIG* config, AUTHENTICATION_HANDLE handle, time_t current_time, AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_context, IOTHUB_CREDENTIAL_TYPE cred_type)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_context, cred_type);
    authentication_do_work(handle);
}

static AUTHENTICATION_HANDLE create_and_start_authentication(AUTHENTICATION_CONFIG* config, bool testing_modules)
{
    AUTHENTICATION_HANDLE handle;

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_create(config, testing_modules);

    handle = authentication_create(config);

    (void)authentication_start(handle, TEST_CBS_HANDLE);

    umock_c_reset_all_calls();

    return handle;
}

static void reset_test_data()
{
    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));

    saved_on_state_changed_callback_context = NULL;
    saved_on_state_changed_callback_previous_state = AUTHENTICATION_STATE_STOPPED;
    saved_on_state_changed_callback_new_state = AUTHENTICATION_STATE_STOPPED;

    saved_on_error_callback_context = NULL;
    saved_on_error_callback_error_code = AUTHENTICATION_ERROR_AUTH_FAILED;

    g_STRING_sprintf_call_count = 0;
    g_STRING_sprintf_fail_on_count = -1;
    saved_STRING_sprintf_handle = NULL;

    TEST_cbs_put_token_async_return = TEST_PUT_TOKEN_RESULT;
    saved_cbs_put_token_cbs = NULL;
    saved_cbs_put_token_type = NULL;
    saved_cbs_put_token_audience = NULL;
    saved_cbs_put_token_token = NULL;
    saved_cbs_put_token_on_operation_complete = NULL;
    saved_cbs_put_token_context = NULL;
}

BEGIN_TEST_SUITE(iothubtransport_amqp_cbs_auth_ut)

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
    result = umocktypes_c_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    register_umock_alias_types();
    register_global_mock_hooks();
    register_global_mock_returns();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
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

TEST_FUNCTION(authentication_create_NULL_config)
{
    // arrange

    // act
    AUTHENTICATION_HANDLE handle = authentication_create(NULL);

    // assert
    ASSERT_IS_NULL(handle);

    // cleanup
}

TEST_FUNCTION(authentication_create_NULL_iothub_host_fqdn)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    config->iothub_host_fqdn = NULL;

    // act
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    // assert
    ASSERT_IS_NULL(handle);

    // cleanup
}

TEST_FUNCTION(authentication_create_NULL_on_state_changed_callback)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    config->on_state_changed_callback = NULL;

    // act
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    // assert
    ASSERT_IS_NULL(handle);

    // cleanup
}

TEST_FUNCTION(authentication_create_DEVICE_KEYS_succeeds)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_create(config, false);

    // act
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_start_NULL_auth_handle)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_create(config, false);
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    umock_c_reset_all_calls();

    // act
    int result = authentication_start(NULL, TEST_CBS_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_start_NULL_cbs_handle)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_create(config, false);
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    umock_c_reset_all_calls();

    // act
    int result = authentication_start(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_start_succeeds)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_create(config, false);
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    umock_c_reset_all_calls();

    // act
    int result = authentication_start(handle, TEST_CBS_HANDLE);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_start_succeeds_with_module)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_create(config, true);
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    umock_c_reset_all_calls();

    // act
    int result = authentication_start(handle, TEST_CBS_HANDLE);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}



TEST_FUNCTION(authentication_start_already_started_fails)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    saved_on_state_changed_callback_previous_state = saved_on_state_changed_callback_new_state = AUTHENTICATION_STATE_STOPPED;

    // act
    int result = authentication_start(handle, TEST_CBS_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}


TEST_FUNCTION(authentication_stop_NULL_handle)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    saved_on_state_changed_callback_previous_state = saved_on_state_changed_callback_new_state = AUTHENTICATION_STATE_STOPPED;

    // act
    int result = authentication_stop(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_stop_already_stoppped)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_create(config, false);
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    umock_c_reset_all_calls();

    // act
    int result = authentication_stop(handle);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_stop_succeeds)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    // act
    int result = authentication_stop(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_destroy_NULL_handle)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();
    // act
    authentication_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_destroy_succeeds)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_destroy(handle);

    // act
    authentication_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_new_state);

    // cleanup
}

TEST_FUNCTION(authentication_destroy_with_pending_cbs_put_token_success)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(async_operation_cancel(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));


    // act
    authentication_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(authentication_do_work_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    authentication_do_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(authentication_do_work_not_started)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_create(config, false);
    AUTHENTICATION_HANDLE handle = authentication_create(config);

    umock_c_reset_all_calls();

    // act
    authentication_do_work(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_SAS_TOKEN_AUTHENTICATION_STATE_STARTING_success)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    // act
    authentication_do_work(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_AUTHENTICATION_STATE_STARTING_success)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;
    //exp_state
    exp_state->sastoken_expiration_time = (uint64_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    // act
    authentication_do_work(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_SAS_TOKEN_on_cbs_put_token_callback_success)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

    umock_c_reset_all_calls();

    // act
    saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTED, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_SAS_TOKEN_on_cbs_put_token_callback_success_with_module)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    config->module_id = TEST_MODULE_ID;
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, true);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

    umock_c_reset_all_calls();

    // act
    saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTED, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}


TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_primary_key_only_fallback)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    //config->device_secondary_key = NULL;
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;
    exp_state->sas_token_to_use = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
    exp_state->sastoken_expiration_time = (uint64_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

    umock_c_reset_all_calls();

    // act
    saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OPERATION_FAILED, 1, "bad token from primary device key");

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_AUTH_FAILED, saved_on_error_callback_error_code);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_SAS_TOKEN_AUTHENTICATION_STATE_STARTING_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->sas_token_to_use = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 0 || i == 1 || i == 2 || i == 4 || i == 5)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }
        else if (i == 3)
        {
            TEST_cbs_put_token_async_return = NULL;
        }
        else
        {
            TEST_cbs_put_token_async_return = TEST_PUT_TOKEN_RESULT;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        authentication_do_work(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_TRUE(NULL == saved_cbs_put_token_on_operation_complete, error_msg);
        ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state, error_msg);
        ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_AUTH_FAILED, saved_on_error_callback_error_code, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_SAS_TOKEN_on_cbs_put_token_callback_error)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

    umock_c_reset_all_calls();

    // act
    saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OPERATION_FAILED, 1, "bad token");

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_AUTH_FAILED, saved_on_error_callback_error_code);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_AUTHENTICATION_STATE_STARTING_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->sas_token_to_use = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 0 || i == 1 || i == 2 || i == 5 || i == 6 || i == 8 || i == 9 || i == 10)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }
        else if (i == 7)
        {
            TEST_cbs_put_token_async_return = NULL;
        }
        else
        {
            TEST_cbs_put_token_async_return = TEST_PUT_TOKEN_RESULT;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        authentication_do_work(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_TRUE(NULL == saved_cbs_put_token_on_operation_complete, error_msg);
        ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
        ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_AUTH_FAILED, saved_on_error_callback_error_code);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_SAS_TOKEN_next_calls_no_op)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);
    saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTED, saved_on_state_changed_callback_new_state);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG)).SetReturn(IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);

    // act
    authentication_do_work(handle);
    authentication_do_work(handle);
    authentication_do_work(handle);
    authentication_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_sas_token_refresh_check)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);
    time_t next_time = add_seconds(current_time, DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS - 1);
    ASSERT_IS_TRUE(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;
    exp_state->sas_token_to_use = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
    exp_state->sastoken_expiration_time = (uint64_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

    exp_state->current_state = AUTHENTICATION_STATE_STARTED;
    exp_state->current_sas_token_put_time = current_time;
    exp_state->sas_token_refresh_time_in_seconds = DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS;

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    // act
    authentication_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_DEVICE_AUTH_sas_token_refresh_check)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_AUTH);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);
    time_t next_time = add_seconds(current_time, DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS - 1);
    ASSERT_IS_TRUE(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;
    exp_state->sas_token_to_use = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
    exp_state->sastoken_expiration_time = (uint64_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH);
    saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

    exp_state->current_state = AUTHENTICATION_STATE_STARTED;
    exp_state->current_sas_token_put_time = current_time;
    exp_state->sas_token_refresh_time_in_seconds = DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS;

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH);

    // act
    authentication_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_sas_token_refresh)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);
    time_t next_time = add_seconds(current_time, 11);
    ASSERT_IS_TRUE(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;
    exp_state->sas_token_to_use = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
    exp_state->sastoken_expiration_time = (uint64_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

    exp_state->current_state = AUTHENTICATION_STATE_STARTED;
    exp_state->current_sas_token_put_time = current_time;
    exp_state->sas_token_refresh_time_in_seconds = 10;
    exp_state->sastoken_expiration_time = (uint64_t)(difftime(next_time, (time_t)0) + 123);

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    // act
    authentication_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_first_auth_timeout_check)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    time_t current_time = time(NULL);
    ASSERT_IS_TRUE(INDEFINITE_TIME != current_time, "current_time = time(NULL) failed");
    time_t next_time = add_seconds(current_time, 31536000); // one year has passed...
    ASSERT_IS_TRUE(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

    exp_state->is_cbs_put_token_in_progress = true;
    exp_state->current_sas_token_put_time = current_time;

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    // act
    authentication_do_work(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_new_state);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_do_work_first_auth_times_out)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    uint64_t timeout_secs = 10;
    int result = authentication_set_option(handle, AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS, &timeout_secs);
    ASSERT_ARE_EQUAL(int, 0, result);

    time_t current_time = time(NULL);
    ASSERT_IS_TRUE(INDEFINITE_TIME != current_time, "current_time = time(NULL) failed");
    time_t next_time = add_seconds(current_time, 11);
    ASSERT_IS_TRUE(INDEFINITE_TIME != next_time, "failed to compute 'next_time'");

    AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
    exp_state->current_state = AUTHENTICATION_STATE_STARTING;

    crank_authentication_do_work(config, handle, current_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

    exp_state->is_cbs_put_token_in_progress = true;
    exp_state->current_sas_token_put_time = current_time;

    umock_c_reset_all_calls();
    set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);

    // act
    authentication_do_work(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
    ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_AUTH_TIMEOUT, saved_on_error_callback_error_code);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_set_option_NULL_name)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();

    size_t value = 10;

    // act
    int result = authentication_set_option(handle, NULL, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_set_option_name_not_supported)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();

    size_t value = 10;

    // act
    int result = authentication_set_option(handle, "!viva el Chapolin Colorado!", &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_set_option_saved_options_succeeds)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();

    OPTIONHANDLER_HANDLE value = TEST_OPTIONHANDLER_HANDLE;
    STRICT_EXPECTED_CALL(OptionHandler_FeedOptions(value, handle));

    // act
    int result = authentication_set_option(handle, AUTHENTICATION_OPTION_SAVED_OPTIONS, &value);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_set_option_OptionHandler_FeedOptions_OPTIONHANDLER_ERROR)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();

    OPTIONHANDLER_HANDLE value = TEST_OPTIONHANDLER_HANDLE;
    STRICT_EXPECTED_CALL(OptionHandler_FeedOptions(value, handle)).SetReturn(OPTIONHANDLER_ERROR);

    // act
    int result = authentication_set_option(handle, AUTHENTICATION_OPTION_SAVED_OPTIONS, value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_set_option_OptionHandler_FeedOptions_OPTIONHANDLER_INVALIDARG)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();

    OPTIONHANDLER_HANDLE value = TEST_OPTIONHANDLER_HANDLE;
    STRICT_EXPECTED_CALL(OptionHandler_FeedOptions(value, handle)).SetReturn(OPTIONHANDLER_INVALIDARG);

    // act
    int result = authentication_set_option(handle, AUTHENTICATION_OPTION_SAVED_OPTIONS, value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    authentication_destroy(handle);
}

TEST_FUNCTION(authentication_retrieve_options_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    OPTIONHANDLER_HANDLE result = authentication_retrieve_options(NULL);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);

    // cleanup
}

TEST_FUNCTION(authentication_retrieve_options_succeeds)
{
    // arrange
    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();
    EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_OPTIONHANDLER_HANDLE);
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS, IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .SetReturn(OPTIONHANDLER_OK);

    // act
    OPTIONHANDLER_HANDLE result = authentication_retrieve_options(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_OPTIONHANDLER_HANDLE, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    authentication_destroy(handle);
}


TEST_FUNCTION(authentication_retrieve_options_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
    AUTHENTICATION_HANDLE handle = create_and_start_authentication(config, false);

    umock_c_reset_all_calls();
    EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_OPTIONHANDLER_HANDLE);
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS, IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .SetReturn(OPTIONHANDLER_OK);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        OPTIONHANDLER_HANDLE result = authentication_retrieve_options(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();
    authentication_destroy(handle);
}

END_TEST_SUITE(iothubtransport_amqp_cbs_auth_ut)
