// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void* real_malloc(size_t size)
{
	return malloc(size);
}

void real_free(void* ptr)
{
	free(ptr);
}


#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_stdint.h"
#include "umocktypes_bool.h"
#include "umock_c_negative_tests.h"
#include "umocktypes.h"
#include "umocktypes_c.h"
#include <limits.h>

#define ENABLE_MOCKS
#include "iothub_transport_ll.h"
#include "azure_uamqp_c/cbs.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/agenttime.h" 
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/sastoken.h"
#undef ENABLE_MOCKS

#include "iothubtransport_amqp_cbs_auth.h"

MOCKABLE_FUNCTION(, time_t, get_time, time_t*, currentTime);
MOCKABLE_FUNCTION(, double, get_difftime, time_t, stopTime, time_t, startTime);

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;


DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
	char temp_str[256];
	(void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
	ASSERT_FAIL(temp_str);
}


// Control parameters

#define DEFAULT_CBS_REQUEST_TIMEOUT_SECS                  UINT32_MAX
#define DEFAULT_SAS_TOKEN_LIFETIME_SECS                   3600
#define DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS               1800
#define INDEFINITE_TIME                                   ((time_t)(-1))
#define TEST_GENERIC_CHAR_PTR                             "generic char* string"
#define TEST_DEVICE_ID                                    "my_device"
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
#define TEST_SAS_TOKEN_KEY_NAME_STRING_HANDLE             (STRING_HANDLE)0x4454


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
static int TEST_cbs_put_token_return;

static int TEST_cbs_put_token(CBS_HANDLE cbs, const char* type, const char* audience, const char* token, ON_CBS_OPERATION_COMPLETE on_operation_complete, void* context)
{
	saved_cbs_put_token_cbs = cbs;
	saved_cbs_put_token_type = type;
	saved_cbs_put_token_audience = audience;
	saved_cbs_put_token_token = token;
	saved_cbs_put_token_on_operation_complete = on_operation_complete;
	saved_cbs_put_token_context = context;

	return TEST_cbs_put_token_return;
}

#ifdef __cplusplus
extern "C"
{
#endif

static int g_STRING_sprintf_call_count;
static int g_STRING_sprintf_fail_on_count;
static STRING_HANDLE saved_STRING_sprintf_handle;

int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
{
	int result;
	saved_STRING_sprintf_handle = handle;
	(void)format;

	g_STRING_sprintf_call_count++;

	if (g_STRING_sprintf_call_count == g_STRING_sprintf_fail_on_count)
	{
		result = __LINE__;
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
}

static void register_global_mock_hooks()
{
	REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
	REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
	REGISTER_GLOBAL_MOCK_HOOK(cbs_put_token, TEST_cbs_put_token);
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
}



// Auxiliary Functions

typedef struct AUTHENTICATION_DO_WORK_EXPECTED_STATE_TAG
{
	AUTHENTICATION_STATE current_state;
	bool is_cbs_put_token_in_progress;
	bool is_sas_token_refresh_in_progress;
	time_t current_sas_token_put_time;
	STRING_HANDLE device_key;
	size_t sastoken_expiration_time;
	size_t sas_token_refresh_time_in_seconds;
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
	USE_DEVICE_SAS_TOKEN
} USE_DEVICE_KEYS_OR_SAS_TOKEN_OPTION;

static AUTHENTICATION_CONFIG* get_auth_config(USE_DEVICE_KEYS_OR_SAS_TOKEN_OPTION credential_option)
{
	memset(&global_auth_config, 0, sizeof(AUTHENTICATION_CONFIG));
	global_auth_config.device_id = TEST_DEVICE_ID;

	if (credential_option == USE_DEVICE_KEYS)
	{
		global_auth_config.device_primary_key = TEST_PRIMARY_DEVICE_KEY;
		global_auth_config.device_secondary_key = TEST_SECONDARY_DEVICE_KEY;
	}
	else
	{
		global_auth_config.device_sas_token = TEST_USER_DEFINED_SAS_TOKEN;
	}

	global_auth_config.iothub_host_fqdn = TEST_IOTHUB_HOST_FQDN;
	global_auth_config.on_state_changed_callback = TEST_on_state_changed_callback;
	global_auth_config.on_state_changed_callback_context = TEST_ON_STATE_CHANGED_CALLBACK_CONTEXT;
	global_auth_config.on_error_callback = TEST_on_error_callback;
	global_auth_config.on_error_callback_context = TEST_ON_ERROR_CALLBACK_CONTEXT;

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

static void set_expected_calls_for_authentication_create(AUTHENTICATION_CONFIG* config)
{
	EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(STRING_construct(config->device_id)).SetReturn(TEST_DEVICE_ID_STRING_HANDLE);

	if (config->device_sas_token != NULL)
	{
		STRICT_EXPECTED_CALL(STRING_construct(TEST_USER_DEFINED_SAS_TOKEN)).SetReturn(TEST_USER_DEFINED_SAS_TOKEN_STRING_HANDLE);
	}
	else
	{
		if (config->device_primary_key != NULL)
			STRICT_EXPECTED_CALL(STRING_construct(TEST_PRIMARY_DEVICE_KEY)).SetReturn(TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE);
		
		if (config->device_secondary_key != NULL)
			STRICT_EXPECTED_CALL(STRING_construct(TEST_SECONDARY_DEVICE_KEY)).SetReturn(TEST_SECONDARY_DEVICE_KEY_STRING_HANDLE);
	}

	STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUB_HOST_FQDN)).SetReturn(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE);
}

static void set_expected_calls_for_authentication_destroy(AUTHENTICATION_CONFIG* config, AUTHENTICATION_HANDLE handle)
{
	STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICE_ID_STRING_HANDLE));

	if (config->device_sas_token != NULL)
	{
		STRICT_EXPECTED_CALL(STRING_delete(TEST_USER_DEFINED_SAS_TOKEN_STRING_HANDLE));
	}
	else
	{
		if (config->device_primary_key != NULL)
			STRICT_EXPECTED_CALL(STRING_delete(TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE));

		if (config->device_secondary_key != NULL)
			STRICT_EXPECTED_CALL(STRING_delete(TEST_SECONDARY_DEVICE_KEY_STRING_HANDLE));
	}

	STRICT_EXPECTED_CALL(STRING_delete(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));

	STRICT_EXPECTED_CALL(free(handle));
}

static void set_expected_calls_for_create_devices_path()
{
	STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_DEVICES_PATH_STRING_HANDLE);
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE));
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));
	// STRING_sprintf
}

static void set_expected_calls_for_put_SAS_token_to_cbs(AUTHENTICATION_HANDLE handle, time_t current_time, STRING_HANDLE sas_token)
{
	char* sas_token_char_ptr;
	if (sas_token == TEST_USER_DEFINED_SAS_TOKEN_STRING_HANDLE)
	{
		sas_token_char_ptr = TEST_USER_DEFINED_SAS_TOKEN;
	}
	else
	{
		sas_token_char_ptr = TEST_GENERATED_SAS_TOKEN;
	}

	STRICT_EXPECTED_CALL(STRING_c_str(sas_token)).SetReturn(sas_token_char_ptr);
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICES_PATH_STRING_HANDLE)).SetReturn(TEST_DEVICES_PATH);
	STRICT_EXPECTED_CALL(cbs_put_token(TEST_CBS_HANDLE, SAS_TOKEN_TYPE, TEST_DEVICES_PATH, sas_token_char_ptr, IGNORED_PTR_ARG, handle))
		.IgnoreArgument(5);
	STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
}

static void set_expected_calls_for_create_and_put_sas_token(AUTHENTICATION_HANDLE handle, time_t current_time, AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_context)
{
	STRICT_EXPECTED_CALL(get_time(NULL)) //2
		.SetReturn(current_time);
	STRICT_EXPECTED_CALL(get_difftime(current_time, (time_t)0))
		.SetReturn(difftime(current_time, (time_t)0));
	set_expected_calls_for_create_devices_path();
	STRICT_EXPECTED_CALL(STRING_new())
		.SetReturn(TEST_SAS_TOKEN_KEY_NAME_STRING_HANDLE);
	STRICT_EXPECTED_CALL(SASToken_Create(exp_context->device_key, TEST_DEVICES_PATH_STRING_HANDLE, TEST_SAS_TOKEN_KEY_NAME_STRING_HANDLE, exp_context->sastoken_expiration_time))
		.SetReturn(TEST_GENERATED_SAS_TOKEN_STRING_HANDLE);
	set_expected_calls_for_put_SAS_token_to_cbs(handle, current_time, TEST_GENERATED_SAS_TOKEN_STRING_HANDLE);
	STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICES_PATH_STRING_HANDLE));
	STRICT_EXPECTED_CALL(STRING_delete(TEST_SAS_TOKEN_KEY_NAME_STRING_HANDLE));
	STRICT_EXPECTED_CALL(STRING_delete(TEST_GENERATED_SAS_TOKEN_STRING_HANDLE));
}

static void set_expected_calls_for_authentication_do_work(AUTHENTICATION_CONFIG* config, AUTHENTICATION_HANDLE handle, time_t current_time, AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_context)
{
	if (exp_context->is_cbs_put_token_in_progress)
	{
		if (exp_context->is_sas_token_refresh_in_progress)
		{
		}
		else
		{
			STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
			double actual_difftime_result = difftime(current_time, exp_context->current_sas_token_put_time);
			STRICT_EXPECTED_CALL(get_difftime(current_time, exp_context->current_sas_token_put_time)).SetReturn(actual_difftime_result);
		}
	}
	else if (exp_context->current_state == AUTHENTICATION_STATE_STARTING)
	{
		if (config->device_sas_token != NULL)
		{
			set_expected_calls_for_create_devices_path();
			set_expected_calls_for_put_SAS_token_to_cbs(handle, current_time, TEST_USER_DEFINED_SAS_TOKEN_STRING_HANDLE);
			STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICES_PATH_STRING_HANDLE));
		}
		else
		{
			set_expected_calls_for_create_and_put_sas_token(handle, current_time, exp_context);
		}
	}
	else if (exp_context->current_state == AUTHENTICATION_STATE_STARTED)
	{
		if (config->device_sas_token == NULL) //, device keys are being used
		{
			STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
			double actual_difftime_result = difftime(current_time, exp_context->current_sas_token_put_time);
			STRICT_EXPECTED_CALL(get_difftime(current_time, exp_context->current_sas_token_put_time)).SetReturn(actual_difftime_result);

			if (actual_difftime_result >= exp_context->sas_token_refresh_time_in_seconds)
			{
				set_expected_calls_for_create_and_put_sas_token(handle, current_time, exp_context);
			}
		}
	}
}

static void crank_authentication_do_work(AUTHENTICATION_CONFIG* config, AUTHENTICATION_HANDLE handle, time_t current_time, AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_context)
{
	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_context);
	authentication_do_work(handle);
}

static AUTHENTICATION_HANDLE create_and_start_authentication(AUTHENTICATION_CONFIG* config)
{
	AUTHENTICATION_HANDLE handle;

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);

	handle = authentication_create(config);

	(void)authentication_start(handle, TEST_CBS_HANDLE);

	umock_c_reset_all_calls();

	return handle;
}

static void reset_parameters()
{
	saved_malloc_returns_count = 0;

	saved_on_state_changed_callback_context = NULL;
	saved_on_state_changed_callback_previous_state = AUTHENTICATION_STATE_STOPPED;
	saved_on_state_changed_callback_new_state = AUTHENTICATION_STATE_STOPPED;

	saved_on_error_callback_context = NULL;
	saved_on_error_callback_error_code = AUTHENTICATION_ERROR_AUTH_FAILED;

	g_STRING_sprintf_call_count = 0;
	g_STRING_sprintf_fail_on_count = -1;
	saved_STRING_sprintf_handle = NULL;

	TEST_cbs_put_token_return = 0;
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
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
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
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();

	reset_parameters();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_001: [If parameter `config` or `config->device_id` are NULL, authentication_create() shall fail and return NULL.]
TEST_FUNCTION(authentication_create_NULL_config)
{
	// arrange

	// act
	AUTHENTICATION_HANDLE handle = authentication_create(NULL);

	// assert
	ASSERT_IS_NULL(handle);

	// cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_001: [If parameter `config` or `config->device_id` are NULL, authentication_create() shall fail and return NULL.]
TEST_FUNCTION(authentication_create_NULL_config_device_id)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	config->device_id = NULL;

	// act
	AUTHENTICATION_HANDLE handle = authentication_create(config);

	// assert
	ASSERT_IS_NULL(handle);

	// cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_002: [If device keys and SAS token are NULL, authentication_create() shall fail and return NULL.]
TEST_FUNCTION(authentication_create_NULL_config_device_keys_and_sas_token)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	config->device_primary_key = NULL;
	config->device_secondary_key = NULL;
	config->device_sas_token = NULL;

	// act
	AUTHENTICATION_HANDLE handle = authentication_create(config);

	// assert
	ASSERT_IS_NULL(handle);

	// cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_003: [If device keys and SAS token are both provided, authentication_create() shall fail and return NULL.]
TEST_FUNCTION(authentication_create_BOTH_config_device_keys_and_sas_token)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	config->device_sas_token = TEST_USER_DEFINED_SAS_TOKEN;

	// act
	AUTHENTICATION_HANDLE handle = authentication_create(config);

	// assert
	ASSERT_IS_NULL(handle);

	// cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_004: [If `config->iothub_host_fqdn` is NULL, authentication_create() shall fail and return NULL.]
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_005: [If `config->on_state_changed_callback` is NULL, authentication_create() shall fail and return NULL]
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_006: [authentication_create() shall allocate memory for a new authenticate state structure AUTHENTICATION_INSTANCE.]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_123: [authentication_create() shall initialize all fields of `instance` with 0 using memset().]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_008: [authentication_create() shall save a copy of `config->device_id` into the `instance->device_id`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_012: [If provided, authentication_create() shall save a copy of `config->device_primary_key` into the `instance->device_primary_key`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_014: [If provided, authentication_create() shall save a copy of `config->device_secondary_key` into `instance->device_secondary_key`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_016: [If provided, authentication_create() shall save a copy of `config->iothub_host_fqdn` into `instance->iothub_host_fqdn`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_024: [If no failure occurs, authentication_create() shall return a reference to the AUTHENTICATION_INSTANCE handle]
TEST_FUNCTION(authentication_create_DEVICE_KEYS_succeeds)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);

	// act
	AUTHENTICATION_HANDLE handle = authentication_create(config);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_IS_NOT_NULL(handle);

	// cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_007: [If malloc() fails, authentication_create() shall fail and return NULL.]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_009: [If STRING_construct() fails, authentication_create() shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_013: [If STRING_construct() fails to copy `config->device_primary_key`, authentication_create() shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_015: [If STRING_construct() fails to copy `config->device_secondary_key`, authentication_create() shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_017: [If STRING_clone() fails to copy `config->iothub_host_fqdn`, authentication_create() shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_020: [If any failure occurs, authentication_create() shall free any memory it allocated previously]
TEST_FUNCTION(authentication_create_DEVICE_KEYS_failure_checks)
{
	// arrange
	ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);
	umock_c_negative_tests_snapshot();

	// act
	size_t i;
	for (i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		// arrange
		char error_msg[64];

		umock_c_negative_tests_reset();
		umock_c_negative_tests_fail_call(i);

		AUTHENTICATION_HANDLE handle = authentication_create(config);

		// assert
		sprintf(error_msg, "On failed call %zu", i);
		ASSERT_IS_NULL_WITH_MSG(handle, error_msg);
	}

	// cleanup
	umock_c_negative_tests_deinit();
	umock_c_reset_all_calls();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_010: [If `device_config->device_sas_token` is not NULL, authentication_create() shall save a copy into the `instance->device_sas_token`]
TEST_FUNCTION(authentication_create_SAS_TOKEN_succeeds)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);

	// act
	AUTHENTICATION_HANDLE handle = authentication_create(config);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_IS_NOT_NULL(handle);

	// cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_011: [If STRING_construct() fails, authentication_create() shall fail and return NULL]
TEST_FUNCTION(authentication_create_SAS_TOKENS_failure_checks)
{
	// arrange
	ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);
	umock_c_negative_tests_snapshot();

	// act
	size_t i;
	for (i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		// arrange
		char error_msg[64];

		umock_c_negative_tests_reset();
		umock_c_negative_tests_fail_call(i);

		AUTHENTICATION_HANDLE handle = authentication_create(config);

		// assert
		sprintf(error_msg, "On failed call %zu", i);
		ASSERT_IS_NULL_WITH_MSG(handle, error_msg);
	}

	// cleanup
	umock_c_negative_tests_deinit();
	umock_c_reset_all_calls();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_025: [If authentication_handle is NULL, authentication_start() shall fail and return __LINE__ as error code]
TEST_FUNCTION(authentication_start_NULL_auth_handle)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_026: [If `cbs_handle` is NULL, authentication_start() shall fail and return __LINE__ as error code]
TEST_FUNCTION(authentication_start_NULL_cbs_handle)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_029: [If no failures occur, `instance->state` shall be set to AUTHENTICATION_STATE_STARTING and `instance->on_state_changed_callback` invoked]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_030: [If no failures occur, authentication_start() shall return 0]
TEST_FUNCTION(authentication_start_succeeds)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_027: [If authenticate state has been started already, authentication_start() shall fail and return __LINE__ as error code]
TEST_FUNCTION(authentication_start_already_started_fails)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

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


// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_031: [If `authentication_handle` is NULL, authentication_stop() shall fail and return __LINE__]
TEST_FUNCTION(authentication_stop_NULL_handle)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_032: [If `instance->state` is AUTHENTICATION_STATE_STOPPED, authentication_stop() shall fail and return __LINE__]
TEST_FUNCTION(authentication_stop_already_stoppped)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);
	AUTHENTICATION_HANDLE handle = authentication_create(config);

	umock_c_reset_all_calls();

	// act
	int result = authentication_stop(handle);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);

	// cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_034: [`instance->state` shall be set to AUTHENTICATION_STATE_STOPPED and `instance->on_state_changed_callback` invoked]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_035: [authentication_stop() shall return success code 0]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_018: [authentication_create() shall save `config->on_state_changed_callback` and `config->on_state_changed_callback_context` into `instance->on_state_changed_callback` and `instance->on_state_changed_callback_context`.]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_033: [`instance->cbs_handle` shall be set to NULL] *
TEST_FUNCTION(authentication_stop_succeeds)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_106: [If authentication_handle is NULL, authentication_destroy() shall return]
TEST_FUNCTION(authentication_destroy_NULL_handle)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_107: [If `instance->state` is AUTHENTICATION_STATE_STARTING or AUTHENTICATION_STATE_STARTED, authentication_stop() shall be invoked and its result ignored]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_108: [authentication_destroy() shall destroy `instance->device_id` using STRING_delete()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_109: [authentication_destroy() shall destroy `instance->device_sas_token` using STRING_delete()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_110: [authentication_destroy() shall destroy `instance->device_primary_key` using STRING_delete()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_111: [authentication_destroy() shall destroy `instance->device_secondary_key` using STRING_delete()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_112: [authentication_destroy() shall destroy `instance->iothub_host_fqdn` using STRING_delete()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_113: [authentication_destroy() shall destroy `instance` using free()]
TEST_FUNCTION(authentication_destroy_succeeds)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_destroy(config, handle);

	// act
	authentication_destroy(handle);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STOPPED, saved_on_state_changed_callback_new_state);

	// cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_036: [If authentication_handle is NULL, authentication_do_work() shall fail and return]
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_037: [If `instance->state` is not AUTHENTICATION_STATE_STARTING or AUTHENTICATION_STATE_STARTED, authentication_do_work() shall fail and return]
TEST_FUNCTION(authentication_do_work_not_started)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	
	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_create(config);
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_041: [If `instance->device_sas_token` is provided, authentication_do_work() shall put it to CBS]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_044: [A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_046: [The SAS token provided shall be sent to CBS using cbs_put_token(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_047: [If cbs_put_token() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with current time]
TEST_FUNCTION(authentication_do_work_SAS_TOKEN_AUTHENTICATION_STATE_STARTING_success)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE* exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_state);

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



// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_042: [Otherwise, authentication_do_work() shall use device keys for CBS authentication]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_049: [authentication_do_work() shall create a SAS token using `instance->device_primary_key`, unless it has failed previously]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_052: [The SAS token expiration time shall be calculated adding `instance->sas_token_lifetime_secs` to the current number of seconds since epoch time UTC]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_053: [A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_115: [An empty STRING_HANDLE, referred to as `sasTokenKeyName`, shall be created using STRING_new()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_055: [The SAS token shall be created using SASToken_Create(), passing the selected device key, `device_path`, `sasTokenKeyName` and expiration time as arguments]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_057: [authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to TRUE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_058: [The SAS token shall be sent to CBS using cbs_put_token(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_059: [If cbs_put_token() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with current time]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_063: [authentication_do_work() shall free the memory it allocated for `devices_path`, `sasTokenKeyName` and SAS token]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_AUTHENTICATION_STATE_STARTING_success)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_state);

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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_028: [authentication_start() shall save `cbs_handle` on `instance->cbs_handle`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_091: [If `result` is CBS_OPERATION_RESULT_OK `instance->state` shall be set to AUTHENTICATION_STATE_STARTED and `instance->on_state_changed_callback` invoked]
TEST_FUNCTION(authentication_do_work_SAS_TOKEN_on_cbs_put_token_callback_success)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;

	crank_authentication_do_work(config, handle, current_time, exp_state);
	
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_051: [If using `instance->device_primary_key` has failed previously, a SAS token shall be created using `instance->device_secondary_key`]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_primary_secondary_fallback)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	crank_authentication_do_work(config, handle, current_time, exp_state);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	 
	ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID); // called on LogError()...
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID); // called on LogError()...

	// act
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OPERATION_FAILED, 1, "bad token from primary device key");

	exp_state->device_key = TEST_SECONDARY_DEVICE_KEY_STRING_HANDLE;
	crank_authentication_do_work(config, handle, current_time, exp_state);
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID); // called on LogError()...
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID); // called on LogError()...
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OPERATION_FAILED, 1, "bad token from secondary device key");

	// assert
	ASSERT_IS_NOT_NULL(handle);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_AUTH_FAILED, saved_on_error_callback_error_code);

	// cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_050: [If using `instance->device_primary_key` has failed previously and `instance->device_secondary_key` is not provided,  authentication_do_work() shall fail and return]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_primary_key_only_fallback)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	config->device_secondary_key = NULL;
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	crank_authentication_do_work(config, handle, current_time, exp_state);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID); // called on LogError()...
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID); // called on LogError()...

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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_045: [If `devices_path` failed to be created, authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to FALSE and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_048: [If cbs_put_token() failed, authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to FALSE, destroy `devices_path` and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_121: [If cbs_put_token() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_122: [If cbs_put_token() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED]
TEST_FUNCTION(authentication_do_work_SAS_TOKEN_AUTHENTICATION_STATE_STARTING_failure_checks)
{
	// arrange
	ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_state);
	umock_c_negative_tests_snapshot();

	// act
	size_t i;
	for (i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		if (i == 1 || i == 2 || i == 3 || i == 4 || i == 7)
		{
			// These expected calls do not cause the API to fail.
			continue;
		}
		else if (i == 5)
		{
			TEST_cbs_put_token_return = 1;
		}
		else
		{
			TEST_cbs_put_token_return = 0;
		}

		// arrange
		char error_msg[64];

		umock_c_negative_tests_reset();
		umock_c_negative_tests_fail_call(i);

		authentication_do_work(handle);

		// assert
		sprintf(error_msg, "On failed call %zu", i);
		ASSERT_IS_TRUE_WITH_MSG(NULL == saved_cbs_put_token_on_operation_complete, error_msg);
		ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
		ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_AUTH_FAILED, saved_on_error_callback_error_code);
	}

	// cleanup
	umock_c_negative_tests_deinit();
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_019: [authentication_create() shall save `config->on_error_callback` and `config->on_error_callback_context` into `instance->on_error_callback` and `instance->on_error_callback_context`.]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_092: [If `result` is not CBS_OPERATION_RESULT_OK `instance->state` shall be set to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_093: [If `result` is not CBS_OPERATION_RESULT_OK and `instance->is_sas_token_refresh_in_progress` is FALSE, `instance->on_error_callback`shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED]
TEST_FUNCTION(authentication_do_work_SAS_TOKEN_on_cbs_put_token_callback_error)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;

	crank_authentication_do_work(config, handle, current_time, exp_state);

	ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)); // called on LogError()...

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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_022: [authentication_create() shall set `instance->sas_token_lifetime_secs` with the default value of one hour]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_054: [If `devices_path` failed to be created, authentication_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_116: [If `sasTokenKeyName` failed to be created, authentication_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_056: [If SASToken_Create() fails, authentication_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_060: [If cbs_put_token() fails, `instance->is_cbs_put_token_in_progress` shall be set to FALSE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_061: [If cbs_put_token() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_062: [If cbs_put_token() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_AUTHENTICATION_STATE_STARTING_failure_checks)
{
	// arrange
	ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, current_time, exp_state);
	umock_c_negative_tests_snapshot();

	// act
	size_t i;
	for (i = 0; i < umock_c_negative_tests_call_count(); i++)
	{
		if (i == 1 || i == 3 || i == 4 || i == 7 || i == 8 || i == 10 || i == 11 || i == 12)
		{
			// These expected calls do not cause the API to fail.
			continue;
		}
		else if (i == 9)
		{
			TEST_cbs_put_token_return = 1;
		}
		else
		{
			TEST_cbs_put_token_return = 0;
		}

		// arrange
		char error_msg[64];

		umock_c_negative_tests_reset();
		umock_c_negative_tests_fail_call(i);

		authentication_do_work(handle);

		// assert
		sprintf(error_msg, "On failed call %zu", i);
		ASSERT_IS_TRUE_WITH_MSG(NULL == saved_cbs_put_token_on_operation_complete, error_msg);
		ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
		ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_AUTH_FAILED, saved_on_error_callback_error_code);
	}

	// cleanup
	umock_c_negative_tests_deinit();
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_040: [If `instance->state` is AUTHENTICATION_STATE_STARTED and user-provided SAS token was used, authentication_do_work() shall return]
TEST_FUNCTION(authentication_do_work_SAS_TOKEN_next_calls_no_op)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;

	crank_authentication_do_work(config, handle, current_time, exp_state);

	ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state);
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTED, saved_on_state_changed_callback_new_state);

	umock_c_reset_all_calls();
	
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_039: [If `instance->state` is AUTHENTICATION_STATE_STARTED and device keys were used, authentication_do_work() shall only verify the SAS token refresh time]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_023: [authentication_create() shall set `instance->sas_token_refresh_time_secs` with the default value of 30 minutes]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_065: [The SAS token shall be refreshed if the current time minus `instance->current_sas_token_put_time` equals or exceeds `instance->sas_token_refresh_time_secs`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_066: [If SAS token does not need to be refreshed, authentication_do_work() shall return]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_sas_token_refresh_check)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);
	time_t next_time = add_seconds(current_time, DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS - 1);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	crank_authentication_do_work(config, handle, current_time, exp_state);
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

	exp_state->current_state = AUTHENTICATION_STATE_STARTED;
	exp_state->current_sas_token_put_time = current_time;
	exp_state->sas_token_refresh_time_in_seconds = DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS;

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state);

	// act
	authentication_do_work(handle);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_101: [`value` shall be saved on `instance->sas_token_refresh_time_secs`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_102: [authentication_set_sas_token_refresh_time_secs() shall return 0]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_104: [`value` shall be saved on `instance->sas_token_lifetime_secs`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_105: [authentication_set_sas_token_lifetime_secs() shall return 0]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_067: [authentication_do_work() shall create a SAS token using `instance->device_primary_key`, unless it has failed previously]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_070: [The SAS token expiration time shall be calculated adding `instance->sas_token_lifetime_secs` to the current number of seconds since epoch time UTC]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_071: [A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_117: [An empty STRING_HANDLE, referred to as `sasTokenKeyName`, shall be created using STRING_new()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_073: [The SAS token shall be created using SASToken_Create(), passing the selected device key, device_path, sasTokenKeyName and expiration time as arguments]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_076: [The SAS token shall be sent to CBS using cbs_put_token(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_077: [If cbs_put_token() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with the current time]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_081: [authentication_do_work() shall free the memory it allocated for `devices_path`, `sasTokenKeyName` and SAS token]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_sas_token_refresh)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);
	time_t next_time = add_seconds(current_time, 11);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

	int result = authentication_set_sas_token_refresh_time_secs(handle, 10);
	ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "authentication_set_sas_token_refresh_time_secs() failed!");

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	crank_authentication_do_work(config, handle, current_time, exp_state);
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

	result = authentication_set_sas_token_lifetime_secs(handle, 123);
	ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "authentication_set_sas_token_lifetime_secs() failed!");

	exp_state->current_state = AUTHENTICATION_STATE_STARTED;
	exp_state->current_sas_token_put_time = current_time;
	exp_state->sas_token_refresh_time_in_seconds = 10;
	exp_state->sastoken_expiration_time = (size_t)(difftime(next_time, (time_t)0) + 123);

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state);

	// act
	authentication_do_work(handle);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_069: [If using `instance->device_primary_key` has failed previously, a SAS token shall be created using `instance->device_secondary_key`]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_sas_token_refresh_key_fallback)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);
	time_t next_time = add_seconds(current_time, 11);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

	int result = authentication_set_sas_token_refresh_time_secs(handle, 10);
	ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "authentication_set_sas_token_refresh_time_secs() failed!");

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	// This blocks authenticates the device.
	crank_authentication_do_work(config, handle, current_time, exp_state);
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

	exp_state->current_state = AUTHENTICATION_STATE_STARTED;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->current_sas_token_put_time = current_time;
	exp_state->sas_token_refresh_time_in_seconds = 10;
	exp_state->sastoken_expiration_time = (size_t)(difftime(next_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	// This sends a new token to CBS (refresh) based still on the primary device key.
	crank_authentication_do_work(config, handle, next_time, exp_state);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_CBS_ERROR, 1, "oh, no! bad token...");

	exp_state->current_sas_token_put_time = next_time;
	exp_state->device_key = TEST_SECONDARY_DEVICE_KEY_STRING_HANDLE;

	// This attemps to send a new token to CBS (refresh) based on the secondary device key (since put-token above failed).
	crank_authentication_do_work(config, handle, next_time, exp_state);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	umock_c_reset_all_calls();

	// act
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "ok, this other one is good");

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTING, saved_on_state_changed_callback_previous_state); // from before...
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTED, saved_on_state_changed_callback_new_state); // nothing has changed

	// cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_087: [If `instance->is_sas_token_refresh_in_progress` is TRUE, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_TIMEOUT]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_089: [`instance->is_sas_token_refresh_in_progress` shall be set to FALSE]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_sas_token_refresh_timeout_and_fallback_to_secondary_key)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);
	time_t refresh_time = add_seconds(current_time, 11);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != refresh_time, "failed to computer 'refresh_time'");
	time_t refresh_timeout_time = add_seconds(current_time, 31);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != refresh_timeout_time, "failed to computer 'refresh_timeout_time'");

	int result = authentication_set_sas_token_refresh_time_secs(handle, 10);
	ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "authentication_set_sas_token_refresh_time_secs() failed!");

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	// This blocks authenticates the device.
	crank_authentication_do_work(config, handle, current_time, exp_state);
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 0, "all good");

	exp_state->current_state = AUTHENTICATION_STATE_STARTED;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->current_sas_token_put_time = current_time;
	exp_state->sas_token_refresh_time_in_seconds = 10;
	exp_state->sastoken_expiration_time = (size_t)(difftime(refresh_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	// This sends a new token to CBS (refresh) based still on the primary device key.
	crank_authentication_do_work(config, handle, refresh_time, exp_state);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	exp_state->current_sas_token_put_time = refresh_time;

	result = authentication_set_cbs_request_timeout_secs(handle, 10);
	ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "authentication_set_cbs_request_timeout_secs() failed!");
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID);

	// act
	crank_authentication_do_work(config, handle, refresh_timeout_time, exp_state);

	exp_state->device_key = TEST_SECONDARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(refresh_timeout_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	crank_authentication_do_work(config, handle, refresh_timeout_time, exp_state);
	
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID);
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID);
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_CBS_ERROR, 1, "oh, no! bad token...");

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTED, saved_on_state_changed_callback_previous_state);
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_SAS_REFRESH_FAILED, saved_on_error_callback_error_code);

    // cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_068: [If using `instance->device_primary_key` has failed previously and `instance->device_secondary_key` is not provided,  authentication_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_094: [If `result` is not CBS_OPERATION_RESULT_OK and `instance->is_sas_token_refresh_in_progress` is TRUE, `instance->on_error_callback`shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_FAILED]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_095: [`instance->is_sas_token_refresh_in_progress` and `instance->is_cbs_put_token_in_progress` shall be set to FALSE]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_sas_token_refresh_key_fallback_no_secondary_key)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
	config->device_secondary_key = NULL;
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);
	time_t next_time = add_seconds(current_time, DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS + 1);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;
	exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
	exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	crank_authentication_do_work(config, handle, current_time, exp_state);
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 1, "all good");

	exp_state->current_state = AUTHENTICATION_STATE_STARTED;
	exp_state->current_sas_token_put_time = current_time;
	exp_state->sas_token_refresh_time_in_seconds = DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS;
	exp_state->sastoken_expiration_time = (size_t)(difftime(next_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

	crank_authentication_do_work(config, handle, next_time, exp_state);
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID); // called on LogError()...
	STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID); // called on LogError()...

	// act
	saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_CBS_ERROR, 1, "oh, no! bad token...");

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTED, saved_on_state_changed_callback_previous_state);
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
	ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_SAS_REFRESH_FAILED, saved_on_error_callback_error_code);

    // cleanup
	authentication_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_072: [If `devices_path` failed to be created, authentication_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_118: [If `sasTokenKeyName` failed to be created, authentication_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_074: [If SASToken_Create() fails, authentication_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_075: [authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to TRUE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_119: [authentication_do_work() shall set `instance->is_sas_token_refresh_in_progress` to TRUE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_078: [If cbs_put_token() fails, `instance->is_cbs_put_token_in_progress` shall be set to FALSE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_120: [If cbs_put_token() fails, `instance->is_sas_token_refresh_in_progress` shall be set to FALSE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_079: [If cbs_put_token() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_080: [If cbs_put_token() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_FAILED]
TEST_FUNCTION(authentication_do_work_DEVICE_KEYS_sas_token_refresh_failure_checks)
{
	int i;
	for (i = 0; i < 12; i++)
	{
		if (i == 0 || i == 1 || i == 3 || i == 5 || i == 6 || i == 9 || i == 10)
		{
			continue;
		}

		// arrange
		ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
		printf("Running step %d...\r\n", i);

		AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_KEYS);
		config->device_secondary_key = NULL;
		AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

		time_t current_time = time(NULL);
		time_t next_time = add_seconds(current_time, DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS + 1);
		ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

		AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
		exp_state->current_state = AUTHENTICATION_STATE_STARTING;
		exp_state->device_key = TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE;
		exp_state->sastoken_expiration_time = (size_t)(difftime(current_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

		// Authenticates
		crank_authentication_do_work(config, handle, current_time, exp_state);
		saved_cbs_put_token_on_operation_complete(saved_cbs_put_token_context, CBS_OPERATION_RESULT_OK, 1, "all good");

		exp_state->current_state = AUTHENTICATION_STATE_STARTED;
		exp_state->current_sas_token_put_time = current_time;
		exp_state->sas_token_refresh_time_in_seconds = DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS;
		exp_state->sastoken_expiration_time = (size_t)(difftime(next_time, (time_t)0) + DEFAULT_SAS_TOKEN_LIFETIME_SECS);

		umock_c_reset_all_calls();
		set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state);
		umock_c_negative_tests_snapshot();

		if (i == 11)
		{
			TEST_cbs_put_token_return = 1;
		}
		else
		{
			umock_c_negative_tests_reset();
			umock_c_negative_tests_fail_call(i);
		}

		// act
		authentication_do_work(handle); // Attempts to refresh the SAS token.

		// assert
		ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_STARTED, saved_on_state_changed_callback_previous_state);
		ASSERT_ARE_EQUAL(int, AUTHENTICATION_STATE_ERROR, saved_on_state_changed_callback_new_state);
		ASSERT_ARE_EQUAL(int, AUTHENTICATION_ERROR_SAS_REFRESH_FAILED, saved_on_error_callback_error_code);

		// cleanup
		authentication_destroy(handle);

		umock_c_negative_tests_deinit();
		umock_c_reset_all_calls();
	}
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_097: [If `authentication_handle` is NULL, authentication_set_cbs_request_timeout_secs() shall fail and return __LINE__]
TEST_FUNCTION(authentication_set_cbs_request_timeout_secs_NULL_handle)
{
	// arrange
	umock_c_reset_all_calls();

	// act
	int result = authentication_set_cbs_request_timeout_secs(NULL, 100);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_100: [If `authentication_handle` is NULL, authentication_set_sas_token_refresh_time_secs() shall fail and return __LINE__]
TEST_FUNCTION(authentication_set_sas_token_refresh_time_secs_NULL_handle)
{
	// arrange
	umock_c_reset_all_calls();

	// act
	int result = authentication_set_sas_token_refresh_time_secs(NULL, 100);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_103: [If `authentication_handle` is NULL, authentication_set_sas_token_lifetime_secs() shall fail and return __LINE__]
TEST_FUNCTION(authentication_set_sas_token_lifetime_secs_NULL_handle)
{
	// arrange
	umock_c_reset_all_calls();

	// act
	int result = authentication_set_sas_token_lifetime_secs(NULL, 100);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_021: [authentication_create() shall set `instance->cbs_request_timeout_secs` with the default value of UINT32_MAX]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_038: [If `instance->is_cbs_put_token_in_progress` is TRUE, authentication_do_work() shall only verify the authentication timeout]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_043: [authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to TRUE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_083: [authentication_do_work() shall check for authentication timeout comparing the current time since `instance->current_sas_token_put_time` to `instance->cbs_request_timeout_secs`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_084: [If no timeout has occurred, authentication_do_work() shall return]
TEST_FUNCTION(authentication_do_work_first_auth_timeout_check)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	time_t current_time = time(NULL);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != current_time, "current_time = time(NULL) failed");
	time_t next_time = add_seconds(current_time, 31536000); // one year has passed...
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;

	crank_authentication_do_work(config, handle, current_time, exp_state);
	ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

	exp_state->is_cbs_put_token_in_progress = true;
	exp_state->current_sas_token_put_time = current_time;

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state);

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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_085: [`instance->is_cbs_put_token_in_progress` shall be set to FALSE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_086: [`instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_098: [`value` shall be saved on `instance->cbs_request_timeout_secs`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_099: [authentication_set_cbs_request_timeout_secs() shall return 0]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_088: [If `instance->is_sas_token_refresh_in_progress` is FALSE, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_TIMEOUT]
TEST_FUNCTION(authentication_do_work_first_auth_times_out)
{
	// arrange
	AUTHENTICATION_CONFIG* config = get_auth_config(USE_DEVICE_SAS_TOKEN);
	AUTHENTICATION_HANDLE handle = create_and_start_authentication(config);

	int result = authentication_set_cbs_request_timeout_secs(handle, 10);
	ASSERT_ARE_EQUAL(int, 0, result);

	time_t current_time = time(NULL);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != current_time, "current_time = time(NULL) failed");
	time_t next_time = add_seconds(current_time, 11);
	ASSERT_IS_TRUE_WITH_MSG(INDEFINITE_TIME != next_time, "failed to computer 'next_time'");

	AUTHENTICATION_DO_WORK_EXPECTED_STATE *exp_state = get_do_work_expected_state_struct();
	exp_state->current_state = AUTHENTICATION_STATE_STARTING;

	crank_authentication_do_work(config, handle, current_time, exp_state);
	ASSERT_IS_NOT_NULL(saved_cbs_put_token_on_operation_complete);

	exp_state->is_cbs_put_token_in_progress = true;
	exp_state->current_sas_token_put_time = current_time;

	umock_c_reset_all_calls();
	set_expected_calls_for_authentication_do_work(config, handle, next_time, exp_state);

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

END_TEST_SUITE(iothubtransport_amqp_cbs_auth_ut)
