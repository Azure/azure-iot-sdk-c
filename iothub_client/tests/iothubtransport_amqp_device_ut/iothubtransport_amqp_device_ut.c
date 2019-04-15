// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
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
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_uamqp_c/session.h"
#include "internal/iothub_client_private.h"
#include "internal/iothubtransport_amqp_telemetry_messenger.h"
#include "internal/iothubtransport_amqp_twin_messenger.h"
#include "internal/iothubtransport_amqp_cbs_auth.h"
#include "internal/iothub_client_authorization.h"


#undef ENABLE_MOCKS

#include "internal/iothubtransport_amqp_device.h"


static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

// ---------- Defines and Test Data ---------- //

// copied from iothubtransport_amqp_device.c
static const char* DEVICE_OPTION_SAVED_AUTH_OPTIONS = "saved_device_auth_options";
static const char* DEVICE_OPTION_SAVED_MESSENGER_OPTIONS = "saved_device_messenger_options";
#define DEFAULT_AUTH_STATE_CHANGED_TIMEOUT_SECS    60
#define DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS    60

#define INDEFINITE_TIME                                   ((time_t)-1)
#define TEST_DEVICE_ID_CHAR_PTR                           "bogus-device"
#define TEST_MODULE_ID_CHAR_PTR                           "bogus-module"
#define TEST_PRODUCT_INFO_CHAR_PTR                        "bogus-product_info"
#define TEST_IOTHUB_HOST_FQDN_CHAR_PTR                    "thisisabogus.azure-devices.net"
#define TEST_ON_STATE_CHANGED_CONTEXT                     (void*)0x7710
#define TEST_USER_DEFINED_SAS_TOKEN                       "blablabla"
#define TEST_USER_DEFINED_SAS_TOKEN_STRING_HANDLE         (STRING_HANDLE)0x7711
#define TEST_PRIMARY_DEVICE_KEY                           "MUhT4tkv1auVqZFQC0lyuHFf6dec+ZhWCgCZ0HcNPuW="
#define TEST_PRIMARY_DEVICE_KEY_STRING_HANDLE             (STRING_HANDLE)0x7712
#define TEST_SECONDARY_DEVICE_KEY                         "WCgCZ0HcNPuWMUhTdec+ZhVqZFQC4tkv1auHFf60lyu="
#define TEST_SECONDARY_DEVICE_KEY_STRING_HANDLE           (STRING_HANDLE)0x7713
#define TEST_AUTHENTICATION_HANDLE                        (AUTHENTICATION_HANDLE)0x7714
#define TEST_TELEMETRY_MESSENGER_HANDLE                   (TELEMETRY_MESSENGER_HANDLE)0x7715
#define TEST_TWIN_MESSENGER_HANDLE                        (TWIN_MESSENGER_HANDLE)0x7727
#define TEST_GENERIC_CHAR_PTR                             "some generic text"
#define TEST_STRING_HANDLE                                (STRING_HANDLE)0x7716
#define TEST_SESSION_HANDLE                               (SESSION_HANDLE)0x7717
#define TEST_CBS_HANDLE                                   (CBS_HANDLE)0x7718
#define TEST_IOTHUB_MESSAGE_HANDLE                        (IOTHUB_MESSAGE_HANDLE)0x7719
#define TEST_VOID_PTR                                     (void*)0x7720
#define TEST_OPTIONHANDLER_HANDLE                         (OPTIONHANDLER_HANDLE)0x7721
#define TEST_AUTH_OPTIONHANDLER_HANDLE                    (OPTIONHANDLER_HANDLE)0x7722
#define TEST_MSGR_OPTIONHANDLER_HANDLE                    (OPTIONHANDLER_HANDLE)0x7723
#define TEST_ON_DEVICE_EVENT_SEND_COMPLETE_CONTEXT        (void*)0x7724
#define TEST_IOTHUB_MESSAGE_LIST                          (IOTHUB_MESSAGE_LIST*)0x7725
#define TEST_AUTHORIZATION_HANDLE                         (IOTHUB_AUTHORIZATION_HANDLE)0x7726

static time_t TEST_current_time;

#define TEST_MESSAGE_SOURCE_NAME_CHAR_PTR                 "link_name"
static delivery_number TEST_MESSAGE_ID;

// ---------- Time-related Test Helpers ---------- //
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

// ---------- Test Hooks ---------- //
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
        if (saved_malloc_returns[i] == ptr)
        {
            real_free(ptr); // only free what we have allocated. Trick to help on TEST_mallocAndStrcpy_s.
            j++;
        }

        saved_malloc_returns[i] = saved_malloc_returns[j];
    }

    if (i != j) saved_malloc_returns_count--;
}

static int TEST_mallocAndStrcpy_s_return;
static int TEST_mallocAndStrcpy_s(char** destination, const char* source)
{
    *destination = (char*)source;
    return TEST_mallocAndStrcpy_s_return;
}

static TELEMETRY_MESSENGER_HANDLE TEST_telemetry_messenger_subscribe_for_messages_saved_messenger_handle;
static ON_TELEMETRY_MESSENGER_MESSAGE_RECEIVED TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback;
static void* TEST_telemetry_messenger_subscribe_for_messages_saved_context;
static int TEST_telemetry_messenger_subscribe_for_messages_return;
static int TEST_telemetry_messenger_subscribe_for_messages(TELEMETRY_MESSENGER_HANDLE messenger_handle, ON_TELEMETRY_MESSENGER_MESSAGE_RECEIVED on_message_received_callback, void* context)
{
    TEST_telemetry_messenger_subscribe_for_messages_saved_messenger_handle = messenger_handle;
    TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback = on_message_received_callback;
    TEST_telemetry_messenger_subscribe_for_messages_saved_context = context;

    return TEST_telemetry_messenger_subscribe_for_messages_return;
}

static double TEST_get_difftime(time_t end_time, time_t start_time)
{
    return difftime(end_time, start_time);
}

static ON_AUTHENTICATION_STATE_CHANGED_CALLBACK TEST_authentication_create_saved_on_authentication_changed_callback;
static void* TEST_authentication_create_saved_on_authentication_changed_context;
static ON_AUTHENTICATION_ERROR_CALLBACK TEST_authentication_create_saved_on_error_callback;
static void* TEST_authentication_create_saved_on_error_context;
static AUTHENTICATION_HANDLE TEST_authentication_create_return;
static AUTHENTICATION_HANDLE TEST_authentication_create(const AUTHENTICATION_CONFIG* config)
{
    TEST_authentication_create_saved_on_authentication_changed_callback = config->on_state_changed_callback;
    TEST_authentication_create_saved_on_authentication_changed_context = config->on_state_changed_callback_context;
    TEST_authentication_create_saved_on_error_callback = config->on_error_callback;
    TEST_authentication_create_saved_on_error_context = config->on_error_callback_context;
    return TEST_authentication_create_return;
}

static ON_TELEMETRY_MESSENGER_STATE_CHANGED_CALLBACK TEST_telemetry_messenger_create_saved_on_state_changed_callback;
static void* TEST_telemetry_messenger_create_saved_on_state_changed_context;
static TELEMETRY_MESSENGER_HANDLE TEST_telemetry_messenger_create_return;
static TELEMETRY_MESSENGER_HANDLE TEST_telemetry_messenger_create(const TELEMETRY_MESSENGER_CONFIG *config, const char* pfi)
{
    (void)pfi;
    TEST_telemetry_messenger_create_saved_on_state_changed_callback = config->on_state_changed_callback;
    TEST_telemetry_messenger_create_saved_on_state_changed_context = config->on_state_changed_context;
    return TEST_telemetry_messenger_create_return;
}

static TWIN_MESSENGER_STATE_CHANGED_CALLBACK TEST_twin_messenger_create_on_state_changed_callback;
static void* TEST_twin_messenger_create_on_state_changed_context;
static TWIN_MESSENGER_HANDLE TEST_twin_messenger_create_return;
static TWIN_MESSENGER_HANDLE TEST_twin_messenger_create(const TWIN_MESSENGER_CONFIG* messenger_config)
{
    TEST_twin_messenger_create_on_state_changed_callback = messenger_config->on_state_changed_callback;
    TEST_twin_messenger_create_on_state_changed_context = messenger_config->on_state_changed_context;
    return TEST_twin_messenger_create_return;
}

static IOTHUB_MESSAGE_LIST* TEST_telemetry_messenger_send_async_saved_message;
static ON_TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE TEST_telemetry_messenger_send_async_saved_callback;
static void* TEST_telemetry_messenger_send_async_saved_context;
static int TEST_telemetry_messenger_send_async_result;
static int TEST_telemetry_messenger_send_async(TELEMETRY_MESSENGER_HANDLE messenger_handle, IOTHUB_MESSAGE_LIST* message, ON_TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE on_messenger_event_send_complete_callback, void* context)
{
    (void)messenger_handle;
    TEST_telemetry_messenger_send_async_saved_message = message;
    TEST_telemetry_messenger_send_async_saved_callback = on_messenger_event_send_complete_callback;
    TEST_telemetry_messenger_send_async_saved_context = context;
    return TEST_telemetry_messenger_send_async_result;
}

static TWIN_MESSENGER_HANDLE get_twin_handle;
static TWIN_STATE_UPDATE_CALLBACK get_twin_callback;
static void* get_twin_context;
static int TEST_twin_messenger_get_twin_async(TWIN_MESSENGER_HANDLE twin_msgr_handle, TWIN_STATE_UPDATE_CALLBACK on_get_twin_completed_callback, void* context)
{
    get_twin_handle = twin_msgr_handle;
    get_twin_callback = on_get_twin_completed_callback;
    get_twin_context = context;

    return 0;
}

// ---------- Test Callbacks ---------- //
static void* TEST_on_state_changed_callback_saved_context;
static DEVICE_STATE TEST_on_state_changed_callback_saved_previous_state;
static DEVICE_STATE TEST_on_state_changed_callback_saved_new_state;
static void TEST_on_state_changed_callback(void* context, DEVICE_STATE previous_state, DEVICE_STATE new_state)
{
    TEST_on_state_changed_callback_saved_context = context;
    TEST_on_state_changed_callback_saved_previous_state = previous_state;
    TEST_on_state_changed_callback_saved_new_state = new_state;
}

static IOTHUB_MESSAGE_HANDLE TEST_on_message_received_saved_message;
static DEVICE_MESSAGE_DISPOSITION_INFO* TEST_on_message_received_saved_disposition_info;
static void* TEST_on_message_received_saved_context;
static DEVICE_MESSAGE_DISPOSITION_RESULT TEST_on_message_received_return;
static DEVICE_MESSAGE_DISPOSITION_RESULT TEST_on_message_received(IOTHUB_MESSAGE_HANDLE message, DEVICE_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
{
    TEST_on_message_received_saved_message = message;
    TEST_on_message_received_saved_disposition_info = disposition_info;
    TEST_on_message_received_saved_context = context;

    return TEST_on_message_received_return;
}

static IOTHUB_MESSAGE_LIST* TEST_on_device_d2c_event_send_complete_callback_saved_message;
static D2C_EVENT_SEND_RESULT TEST_on_device_d2c_event_send_complete_callback_saved_result;
static void* TEST_on_device_d2c_event_send_complete_callback_saved_context;
static void TEST_on_device_d2c_event_send_complete_callback(IOTHUB_MESSAGE_LIST* message, D2C_EVENT_SEND_RESULT result, void* context)
{
    TEST_on_device_d2c_event_send_complete_callback_saved_message = message;
    TEST_on_device_d2c_event_send_complete_callback_saved_result = result;
    TEST_on_device_d2c_event_send_complete_callback_saved_context = context;
}

static DEVICE_TWIN_UPDATE_TYPE dvc_get_twin_update_type;
static const unsigned char* dvc_get_twin_message;
static size_t dvc_get_twin_length;
static void* dvc_get_twin_context;
static void on_device_get_twin_completed_callback(DEVICE_TWIN_UPDATE_TYPE update_type, const unsigned char* message, size_t length, void* context)
{
    dvc_get_twin_update_type = update_type;
    dvc_get_twin_message = message;
    dvc_get_twin_length = length;
    dvc_get_twin_context = context;
}

// ---------- Test Helpers ---------- //
static AMQP_DEVICE_CONFIG TEST_device_config;
static AMQP_DEVICE_CONFIG* get_device_config(DEVICE_AUTH_MODE auth_mode)
{
    memset(&TEST_device_config, 0, sizeof(AMQP_DEVICE_CONFIG));
    TEST_device_config.device_id = TEST_DEVICE_ID_CHAR_PTR;
    TEST_device_config.product_info = TEST_PRODUCT_INFO_CHAR_PTR;
    TEST_device_config.iothub_host_fqdn = TEST_IOTHUB_HOST_FQDN_CHAR_PTR;
    TEST_device_config.authentication_mode = auth_mode;
    TEST_device_config.on_state_changed_callback = TEST_on_state_changed_callback;
    TEST_device_config.on_state_changed_context = TEST_ON_STATE_CHANGED_CONTEXT;
    TEST_device_config.authorization_module = TEST_AUTHORIZATION_HANDLE;

    return &TEST_device_config;
}

static AMQP_DEVICE_CONFIG* get_device_config_with_module_id(DEVICE_AUTH_MODE auth_mode)
{
    AMQP_DEVICE_CONFIG* config = get_device_config(auth_mode);
    config->module_id = TEST_MODULE_ID_CHAR_PTR;
    return config;
}

static void reset_test_data()
{
    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));

    TEST_mallocAndStrcpy_s_return = 0;

    TEST_on_state_changed_callback_saved_context = NULL;
    TEST_on_state_changed_callback_saved_previous_state = DEVICE_STATE_STOPPED;
    TEST_on_state_changed_callback_saved_new_state = DEVICE_STATE_STOPPED;

    TEST_current_time = time(NULL);

    TEST_on_message_received_saved_message = NULL;
    TEST_on_message_received_saved_disposition_info = NULL;
    TEST_on_message_received_saved_context = NULL;
    TEST_on_message_received_return = DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED;

    TEST_telemetry_messenger_subscribe_for_messages_saved_messenger_handle = NULL;
    TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback = NULL;
    TEST_telemetry_messenger_subscribe_for_messages_saved_context = NULL;
    TEST_telemetry_messenger_subscribe_for_messages_return = 0;

    TEST_authentication_create_saved_on_authentication_changed_callback = NULL;
    TEST_authentication_create_saved_on_authentication_changed_context = NULL;
    TEST_authentication_create_saved_on_error_callback = NULL;
    TEST_authentication_create_saved_on_error_context = NULL;
    TEST_authentication_create_return = TEST_AUTHENTICATION_HANDLE;

    TEST_telemetry_messenger_create_saved_on_state_changed_callback = NULL;
    TEST_telemetry_messenger_create_saved_on_state_changed_context = NULL;
    TEST_telemetry_messenger_create_return = TEST_TELEMETRY_MESSENGER_HANDLE;

    TEST_twin_messenger_create_on_state_changed_callback = NULL;
    TEST_twin_messenger_create_on_state_changed_context = NULL;
    TEST_twin_messenger_create_return = TEST_TWIN_MESSENGER_HANDLE;

    TEST_telemetry_messenger_send_async_saved_message = NULL;
    TEST_telemetry_messenger_send_async_saved_callback = NULL;
    TEST_telemetry_messenger_send_async_saved_context = NULL;
    TEST_telemetry_messenger_send_async_result = 0;

    TEST_on_device_d2c_event_send_complete_callback_saved_message = NULL;
    TEST_on_device_d2c_event_send_complete_callback_saved_result = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_UNKNOWN;
    TEST_on_device_d2c_event_send_complete_callback_saved_context = NULL;

    TEST_MESSAGE_ID = (delivery_number)33445566;

    get_twin_handle = NULL;
    get_twin_callback = NULL;
    get_twin_context = NULL;

    dvc_get_twin_update_type = DEVICE_TWIN_UPDATE_TYPE_COMPLETE;
    dvc_get_twin_message = NULL;
    dvc_get_twin_length = 0;
    dvc_get_twin_context = NULL;
}

static void register_umock_alias_types()
{
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AUTHENTICATION_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(const AUTHENTICATION_CONFIG*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AUTHENTICATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TELEMETRY_MESSENGER_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(const TELEMETRY_MESSENGER_CONFIG*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TELEMETRY_MESSENGER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TWIN_MESSENGER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TWIN_MESSENGER_STATE_CHANGED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TWIN_MESSENGER_REPORT_STATE_COMPLETE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TWIN_STATE_UPDATE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CBS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const CBS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(AUTHENTICATION_ERROR_CODE, int);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(pfCloneOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfDestroyOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfSetOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_TELEMETRY_MESSENGER_MESSAGE_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE, int);
    REGISTER_UMOCK_ALIAS_TYPE(TELEMETRY_MESSENGER_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_MESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK, void*);
}

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, TEST_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_HOOK(telemetry_messenger_subscribe_for_messages, TEST_telemetry_messenger_subscribe_for_messages);
    REGISTER_GLOBAL_MOCK_HOOK(get_difftime, TEST_get_difftime);
    REGISTER_GLOBAL_MOCK_HOOK(authentication_create, TEST_authentication_create);
    REGISTER_GLOBAL_MOCK_HOOK(telemetry_messenger_create, TEST_telemetry_messenger_create);
    REGISTER_GLOBAL_MOCK_HOOK(twin_messenger_create, TEST_twin_messenger_create);
    REGISTER_GLOBAL_MOCK_HOOK(telemetry_messenger_send_async, TEST_telemetry_messenger_send_async);
    REGISTER_GLOBAL_MOCK_HOOK(twin_messenger_get_twin_async, TEST_twin_messenger_get_twin_async);
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

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_AddOption, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_AddOption, OPTIONHANDLER_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(authentication_create, TEST_AUTHENTICATION_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(authentication_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(authentication_stop, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(authentication_stop, 1);

    REGISTER_GLOBAL_MOCK_RETURN(authentication_set_option, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(authentication_set_option, 1);

    REGISTER_GLOBAL_MOCK_RETURN(telemetry_messenger_create, TEST_TELEMETRY_MESSENGER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(telemetry_messenger_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(telemetry_messenger_start, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(telemetry_messenger_start, 1);

    REGISTER_GLOBAL_MOCK_RETURN(telemetry_messenger_stop, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(telemetry_messenger_stop, 1);

    REGISTER_GLOBAL_MOCK_RETURN(telemetry_messenger_set_option, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(telemetry_messenger_set_option, 1);

    REGISTER_GLOBAL_MOCK_RETURN(telemetry_messenger_send_async, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(telemetry_messenger_send_async, 1);

    REGISTER_GLOBAL_MOCK_RETURN(twin_messenger_start, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(twin_messenger_start, 1);

    REGISTER_GLOBAL_MOCK_RETURN(twin_messenger_stop, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(twin_messenger_stop, 1);

    REGISTER_GLOBAL_MOCK_RETURN(twin_messenger_set_option, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(twin_messenger_set_option, 1);

    REGISTER_GLOBAL_MOCK_RETURN(twin_messenger_report_state_async, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(twin_messenger_report_state_async, 1);

    REGISTER_GLOBAL_MOCK_RETURN(mallocAndStrcpy_s, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 1);

    REGISTER_GLOBAL_MOCK_RETURN(telemetry_messenger_send_message_disposition, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(telemetry_messenger_send_message_disposition, 1);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_DeviceId, TEST_DEVICE_ID_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Auth_Get_ModuleId, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(twin_messenger_get_twin_async, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(twin_messenger_get_twin_async, 1);
}

// ---------- Expected Call Helpers ---------- //

static void set_expected_calls_for_is_timeout_reached(time_t current_time)
{
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);

    if (current_time != INDEFINITE_TIME)
    {
        STRICT_EXPECTED_CALL(get_difftime(current_time, IGNORED_NUM_ARG)); // let the function hook calculate the actual difftime.
    }
}

static void set_expected_calls_for_clone_device_config(AMQP_DEVICE_CONFIG *config)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->product_info));

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->iothub_host_fqdn));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_DeviceId(TEST_AUTHORIZATION_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_ModuleId(TEST_AUTHORIZATION_HANDLE)).SetReturn(config->module_id);
}

static void set_expected_calls_for_create_authentication_instance(AMQP_DEVICE_CONFIG *config)
{
    (void)config;
    EXPECTED_CALL(authentication_create(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_create_messenger_instance(AMQP_DEVICE_CONFIG *config)
{
    (void)config;
    EXPECTED_CALL(telemetry_messenger_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void set_expected_calls_for_create_twin_messenger(AMQP_DEVICE_CONFIG *config)
{
    (void)config;
    EXPECTED_CALL(twin_messenger_create(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_device_create(AMQP_DEVICE_CONFIG *config, time_t current_time)
{
    (void)current_time;

    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));

    set_expected_calls_for_clone_device_config(config);

    if (config->authentication_mode == DEVICE_AUTH_MODE_CBS)
    {
        set_expected_calls_for_create_authentication_instance(config);
    }

    set_expected_calls_for_create_messenger_instance(config);

    set_expected_calls_for_create_twin_messenger(config);
}

static void set_expected_calls_for_device_start_async(AMQP_DEVICE_CONFIG* config, time_t current_time)
{
    (void)config;
    (void)current_time;
    // Nothing to expect from this function.
}

static void set_expected_calls_for_device_stop(AMQP_DEVICE_CONFIG* config, time_t current_time, AUTHENTICATION_STATE auth_state, TELEMETRY_MESSENGER_STATE messenger_state, TWIN_MESSENGER_STATE twin_msgr_state)
{
    (void)current_time;

    if (messenger_state != TELEMETRY_MESSENGER_STATE_STOPPED && messenger_state != TELEMETRY_MESSENGER_STATE_STOPPING)
    {
        STRICT_EXPECTED_CALL(telemetry_messenger_stop(TEST_TELEMETRY_MESSENGER_HANDLE));
    }

    if (twin_msgr_state != TWIN_MESSENGER_STATE_STOPPED && twin_msgr_state != TWIN_MESSENGER_STATE_STOPPING)
    {
        STRICT_EXPECTED_CALL(twin_messenger_stop(TEST_TWIN_MESSENGER_HANDLE));
    }

    if (config->authentication_mode == DEVICE_AUTH_MODE_CBS && auth_state != AUTHENTICATION_STATE_STOPPED)
    {
        STRICT_EXPECTED_CALL(authentication_stop(TEST_AUTHENTICATION_HANDLE));
    }
}

static void set_expected_calls_for_device_do_work(AMQP_DEVICE_CONFIG* config, time_t current_time, DEVICE_STATE device_state, AUTHENTICATION_STATE auth_state, TELEMETRY_MESSENGER_STATE msgr_state, TWIN_MESSENGER_STATE twin_msgr_state)
{
    if (device_state == DEVICE_STATE_STARTING)
    {
        if (config->authentication_mode == DEVICE_AUTH_MODE_CBS)
        {
            if (auth_state == AUTHENTICATION_STATE_STOPPED)
            {
                STRICT_EXPECTED_CALL(authentication_start(TEST_AUTHENTICATION_HANDLE, TEST_CBS_HANDLE)).SetReturn(0);
            }
            else if (auth_state == AUTHENTICATION_STATE_STARTING)
            {
                set_expected_calls_for_is_timeout_reached(current_time);
            }
        }

        if (config->authentication_mode == DEVICE_AUTH_MODE_X509 || auth_state == AUTHENTICATION_STATE_STARTED)
        {
            if (msgr_state == TELEMETRY_MESSENGER_STATE_STOPPED)
            {
                STRICT_EXPECTED_CALL(telemetry_messenger_start(TEST_TELEMETRY_MESSENGER_HANDLE, TEST_SESSION_HANDLE));
            }
            else if (msgr_state == TELEMETRY_MESSENGER_STATE_STARTING)
            {
                set_expected_calls_for_is_timeout_reached(current_time);
            }

            if (twin_msgr_state == TWIN_MESSENGER_STATE_STOPPED)
            {
                STRICT_EXPECTED_CALL(twin_messenger_start(TEST_TWIN_MESSENGER_HANDLE, TEST_SESSION_HANDLE));
            }
            else if (twin_msgr_state == TWIN_MESSENGER_STATE_STARTING)
            {
                set_expected_calls_for_is_timeout_reached(current_time);
            }
        }
    }

    if (config->authentication_mode == DEVICE_AUTH_MODE_CBS)
    {
        if (auth_state != AUTHENTICATION_STATE_STOPPED && auth_state != AUTHENTICATION_STATE_ERROR)
        {
            STRICT_EXPECTED_CALL(authentication_do_work(TEST_AUTHENTICATION_HANDLE));
        }
    }

    if (msgr_state != TELEMETRY_MESSENGER_STATE_STOPPED && msgr_state != TELEMETRY_MESSENGER_STATE_ERROR)
    {
        STRICT_EXPECTED_CALL(telemetry_messenger_do_work(TEST_TELEMETRY_MESSENGER_HANDLE));
    }

    if (twin_msgr_state != TWIN_MESSENGER_STATE_STOPPED && twin_msgr_state != TWIN_MESSENGER_STATE_ERROR)
    {
        STRICT_EXPECTED_CALL(twin_messenger_do_work(TEST_TWIN_MESSENGER_HANDLE));
    }
}

static void set_expected_calls_for_device_destroy(AMQP_DEVICE_HANDLE handle, AMQP_DEVICE_CONFIG *config, time_t current_time, DEVICE_STATE device_state, AUTHENTICATION_STATE auth_state, TELEMETRY_MESSENGER_STATE msgr_state, TWIN_MESSENGER_STATE twin_msgr_state)
{
    if (device_state == DEVICE_STATE_STARTED || device_state == DEVICE_STATE_STARTING)
    {
        set_expected_calls_for_device_stop(config, current_time, auth_state, msgr_state, twin_msgr_state);
    }

    STRICT_EXPECTED_CALL(telemetry_messenger_destroy(TEST_TELEMETRY_MESSENGER_HANDLE));
    STRICT_EXPECTED_CALL(twin_messenger_destroy(TEST_TWIN_MESSENGER_HANDLE));

    if (config->authentication_mode == DEVICE_AUTH_MODE_CBS && auth_state != AUTHENTICATION_STATE_STOPPED)
    {
        STRICT_EXPECTED_CALL(authentication_destroy(TEST_AUTHENTICATION_HANDLE));
    }

    // destroy config
    STRICT_EXPECTED_CALL(free(config->product_info));
    STRICT_EXPECTED_CALL(free(config->iothub_host_fqdn));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(free(handle));
}

static void set_expected_calls_for_device_retrieve_options(AMQP_DEVICE_CONFIG *config)
{
    EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(TEST_OPTIONHANDLER_HANDLE);

    if (config->authentication_mode == DEVICE_AUTH_MODE_CBS)
    {
        STRICT_EXPECTED_CALL(authentication_retrieve_options(TEST_AUTHENTICATION_HANDLE))
            .SetReturn(TEST_AUTH_OPTIONHANDLER_HANDLE);
        STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, DEVICE_OPTION_SAVED_AUTH_OPTIONS, TEST_AUTH_OPTIONHANDLER_HANDLE))
            .SetReturn(OPTIONHANDLER_OK);
    }

    STRICT_EXPECTED_CALL(telemetry_messenger_retrieve_options(TEST_TELEMETRY_MESSENGER_HANDLE))
        .SetReturn(TEST_MSGR_OPTIONHANDLER_HANDLE);
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, TEST_MSGR_OPTIONHANDLER_HANDLE))
        .SetReturn(OPTIONHANDLER_OK);
}

static void set_expected_calls_for_device_set_option(AMQP_DEVICE_HANDLE handle, AMQP_DEVICE_CONFIG *config, const char* option_name, void* option_value)
{
    if (config->authentication_mode == DEVICE_AUTH_MODE_CBS)
    {
        if (strcmp(DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, option_name) == 0 ||
            strcmp(DEVICE_OPTION_SAS_TOKEN_REFRESH_TIME_SECS, option_name) == 0 ||
            strcmp(DEVICE_OPTION_SAS_TOKEN_LIFETIME_SECS, option_name) == 0)
        {
            STRICT_EXPECTED_CALL(authentication_set_option(TEST_AUTHENTICATION_HANDLE, option_name, option_value));
        }
        else if (strcmp(DEVICE_OPTION_SAVED_AUTH_OPTIONS, option_name) == 0)
        {
            STRICT_EXPECTED_CALL(OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)option_value, TEST_AUTHENTICATION_HANDLE));
        }
    }

    if (strcmp(DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, option_name) == 0)
    {
        STRICT_EXPECTED_CALL(telemetry_messenger_set_option(TEST_TELEMETRY_MESSENGER_HANDLE, TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, option_value));
    }
    else if (strcmp(DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, option_name) == 0)
    {
        STRICT_EXPECTED_CALL(OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)option_value, TEST_TELEMETRY_MESSENGER_HANDLE));
    }
    else if (strcmp(DEVICE_OPTION_SAVED_OPTIONS, option_name) == 0)
    {
        STRICT_EXPECTED_CALL(OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)option_value, handle));
    }
}

static void set_expected_calls_for_device_send_async(AMQP_DEVICE_CONFIG *config)
{
    (void)config;
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(telemetry_messenger_send_async(TEST_TELEMETRY_MESSENGER_HANDLE, TEST_IOTHUB_MESSAGE_LIST, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .IgnoreArgument(4);
}


// ---------- set_expected*-dependent Test Helpers ---------- //

static AMQP_DEVICE_HANDLE create_device(AMQP_DEVICE_CONFIG* config, time_t current_time)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_device_create(config, current_time);
    return amqp_device_create(config);
}

static AMQP_DEVICE_HANDLE create_and_start_device(AMQP_DEVICE_CONFIG* config, time_t current_time)
{
    AMQP_DEVICE_HANDLE handle = create_device(config, current_time);

    set_expected_calls_for_device_start_async(config, current_time);

    if (config->authentication_mode == DEVICE_AUTH_MODE_CBS)
    {
        (void)amqp_device_start_async(handle, TEST_SESSION_HANDLE, TEST_CBS_HANDLE);
    }
    else
    {
        (void)amqp_device_start_async(handle, TEST_SESSION_HANDLE, NULL);
    }

    return handle;
}

static void crank_device_do_work(AMQP_DEVICE_HANDLE handle, AMQP_DEVICE_CONFIG* config, time_t current_time, DEVICE_STATE device_state, AUTHENTICATION_STATE auth_state, TELEMETRY_MESSENGER_STATE msgr_state, TWIN_MESSENGER_STATE twin_msgr_state)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_device_do_work(config, current_time, device_state, auth_state, msgr_state, twin_msgr_state);
    amqp_device_do_work(handle);
}

static void set_authentication_state(AUTHENTICATION_STATE previous_state, AUTHENTICATION_STATE new_state, time_t current_time)
{
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);

    TEST_authentication_create_saved_on_authentication_changed_callback(
        TEST_authentication_create_saved_on_authentication_changed_context,
        previous_state,
        new_state);
}

static void set_messenger_state(TELEMETRY_MESSENGER_STATE previous_state, TELEMETRY_MESSENGER_STATE new_state, time_t current_time)
{
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);

    TEST_telemetry_messenger_create_saved_on_state_changed_callback(
        TEST_telemetry_messenger_create_saved_on_state_changed_context,
        previous_state,
        new_state);
}

static void set_twin_messenger_state(TWIN_MESSENGER_STATE previous_state, TWIN_MESSENGER_STATE new_state, time_t current_time)
{
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);

    TEST_twin_messenger_create_on_state_changed_callback(
        TEST_twin_messenger_create_on_state_changed_context,
        previous_state,
        new_state);
}

static AMQP_DEVICE_HANDLE create_and_start_and_crank_device(AMQP_DEVICE_CONFIG* config, time_t current_time)
{
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, current_time);

    crank_device_do_work(handle, config, current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    set_authentication_state(AUTHENTICATION_STATE_STOPPED, AUTHENTICATION_STATE_STARTING, current_time);
    set_authentication_state(AUTHENTICATION_STATE_STARTING, AUTHENTICATION_STATE_STARTED, current_time);

    crank_device_do_work(handle, config, current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    set_messenger_state(TELEMETRY_MESSENGER_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STARTING, TEST_current_time);
    set_twin_messenger_state(TWIN_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STARTING, TEST_current_time);

    set_messenger_state(TELEMETRY_MESSENGER_STATE_STARTING, TELEMETRY_MESSENGER_STATE_STARTED, TEST_current_time);
    set_twin_messenger_state(TWIN_MESSENGER_STATE_STARTING, TWIN_MESSENGER_STATE_STARTED, TEST_current_time);

    crank_device_do_work(handle, config, current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STARTED, TWIN_MESSENGER_STATE_STARTED);

    return handle;
}


BEGIN_TEST_SUITE(iothubtransport_amqp_device_ut)

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

    register_umock_alias_types();
    register_global_mock_returns();
    register_global_mock_hooks();
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


// Tests_SRS_DEVICE_09_001: [If `config` or device_id or iothub_host_fqdn or on_state_changed_callback are NULL then amqp_device_create shall fail and return NULL]
TEST_FUNCTION(device_create_NULL_config)
{
    // arrange

    // act
    AMQP_DEVICE_HANDLE handle = amqp_device_create(NULL);

    // assert
    ASSERT_IS_NULL(handle);

    // cleanup
}

// Tests_SRS_DEVICE_09_001: [If `config` or iothub_host_fqdn or on_state_changed_callback are NULL then amqp_device_create shall fail and return NULL]
TEST_FUNCTION(device_create_NULL_config_iothub_host_fqdn)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    config->iothub_host_fqdn = NULL;

    // act
    AMQP_DEVICE_HANDLE handle = amqp_device_create(config);

    // assert
    ASSERT_IS_NULL(handle);

    // cleanup
}

// Tests_SRS_DEVICE_09_001: [If `config` or iothub_host_fqdn or on_state_changed_callback are NULL then amqp_device_create shall fail and return NULL]
TEST_FUNCTION(device_create_NULL_config_on_state_changed_callback)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    config->on_state_changed_callback = NULL;

    // act
    AMQP_DEVICE_HANDLE handle = amqp_device_create(config);

    // assert
    ASSERT_IS_NULL(handle);

    // cleanup
}

// Tests_SRS_DEVICE_09_002: [amqp_device_create shall allocate memory for the device instance structure]
// Tests_SRS_DEVICE_09_004: [All `config` parameters shall be saved into `instance`]
// Tests_SRS_DEVICE_09_006: [If `instance->authentication_mode` is DEVICE_AUTH_MODE_CBS, `instance->authentication_handle` shall be set using authentication_create()]
// Tests_SRS_DEVICE_09_008: [`instance->messenger_handle` shall be set using telemetry_messenger_create()]
// Tests_SRS_DEVICE_09_122: [`instance->twin_messenger_handle` shall be set using twin_messenger_create()]
// Tests_SRS_DEVICE_09_011: [If amqp_device_create succeeds it shall return a handle to its `instance` structure]
TEST_FUNCTION(device_create_succeeds)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);

    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");
    set_expected_calls_for_device_create(config, TEST_current_time);

    // act
    AMQP_DEVICE_HANDLE handle = amqp_device_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_002: [amqp_device_create shall allocate memory for the device instance structure]
// Tests_SRS_DEVICE_09_004: [All `config` parameters shall be saved into `instance`]
// Tests_SRS_DEVICE_09_006: [If `instance->authentication_mode` is DEVICE_AUTH_MODE_CBS, `instance->authentication_handle` shall be set using authentication_create()]
// Tests_SRS_DEVICE_09_008: [`instance->messenger_handle` shall be set using telemetry_messenger_create()]
// Tests_SRS_DEVICE_09_122: [`instance->twin_messenger_handle` shall be set using twin_messenger_create()]
// Tests_SRS_DEVICE_09_011: [If amqp_device_create succeeds it shall return a handle to its `instance` structure]
TEST_FUNCTION(device_create_with_module_succeeds)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config_with_module_id(DEVICE_AUTH_MODE_CBS);

    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");
    set_expected_calls_for_device_create(config, TEST_current_time);

    // act
    AMQP_DEVICE_HANDLE handle = amqp_device_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_003: [If malloc fails, amqp_device_create shall fail and return NULL]
// Tests_SRS_DEVICE_09_005: [If any `config` parameters fail to be saved into `instance`, amqp_device_create shall fail and return NULL]
// Tests_SRS_DEVICE_09_007: [If the AUTHENTICATION_HANDLE fails to be created, amqp_device_create shall fail and return NULL]
// Tests_SRS_DEVICE_09_009: [If the TELEMETRY_MESSENGER_HANDLE fails to be created, amqp_device_create shall fail and return NULL]
// Tests_SRS_DEVICE_09_123: [If the TWIN_MESSENGER_HANDLE fails to be created, amqp_device_create shall fail and return NULL]
// Tests_SRS_DEVICE_09_010: [If amqp_device_create fails it shall release all memory it has allocated]
TEST_FUNCTION(device_create_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    umock_c_reset_all_calls();
    set_expected_calls_for_device_create(config, TEST_current_time);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        if (i == 4 || i == 5)
        {
            // for the IoTHubClient_Auth_Get_DeviceId and IoTHubClient_Auth_Get_ModuleId
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        AMQP_DEVICE_HANDLE handle = amqp_device_create(config);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(handle, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();
}

// Tests_SRS_DEVICE_09_017: [If `handle` is NULL, amqp_device_start_async shall return a non-zero result]
TEST_FUNCTION(device_start_async_NULL_handle)
{
    // arrange

    // act
    int result = amqp_device_start_async(NULL, TEST_SESSION_HANDLE, TEST_CBS_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

// Tests_SRS_DEVICE_09_018: [If the device state is not DEVICE_STATE_STOPPED, amqp_device_start_async shall return a non-zero result]
TEST_FUNCTION(device_start_async_device_not_stopped)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_new_state);

    // act
    int result = amqp_device_start_async(handle, TEST_SESSION_HANDLE, TEST_CBS_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_019: [If `session_handle` is NULL, amqp_device_start_async shall return a non-zero result]
TEST_FUNCTION(device_start_async_NULL_session_handle)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_start_async(config, TEST_current_time);

    // act
    int result = amqp_device_start_async(handle, NULL, TEST_CBS_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_020: [If using CBS authentication and `cbs_handle` is NULL, amqp_device_start_async shall return a non-zero result]
TEST_FUNCTION(device_start_async_CBS_NULL_cbs_handle)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_start_async(config, TEST_current_time);

    // act
    int result = amqp_device_start_async(handle, TEST_SESSION_HANDLE, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_022: [The device state shall be updated to DEVICE_STATE_STARTING, and state changed callback invoked]
// Tests_SRS_DEVICE_09_023: [If no failures occur, amqp_device_start_async shall return 0]
TEST_FUNCTION(device_start_async_X509_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_X509);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_start_async(config, TEST_current_time);

    // act
    int result = amqp_device_start_async(handle, TEST_SESSION_HANDLE, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_022: [The device state shall be updated to DEVICE_STATE_STARTING, and state changed callback invoked]
// Tests_SRS_DEVICE_09_023: [If no failures occur, amqp_device_start_async shall return 0]
TEST_FUNCTION(device_start_async_X509_with_module_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config_with_module_id(DEVICE_AUTH_MODE_X509);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_start_async(config, TEST_current_time);

    // act
    int result = amqp_device_start_async(handle, TEST_SESSION_HANDLE, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}


// Tests_SRS_DEVICE_09_022: [The device state shall be updated to DEVICE_STATE_STARTING, and state changed callback invoked]
// Tests_SRS_DEVICE_09_023: [If no failures occur, amqp_device_start_async shall return 0]
TEST_FUNCTION(device_start_async_CBS_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_start_async(config, TEST_current_time);

    // act
    int result = amqp_device_start_async(handle, TEST_SESSION_HANDLE, TEST_CBS_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}


// Tests_SRS_DEVICE_09_024: [If `handle` is NULL, amqp_device_stop shall return a non-zero result]
TEST_FUNCTION(device_stop_NULL_handle)
{
    // arrange

    // act
    int result = amqp_device_stop(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

// Tests_SRS_DEVICE_09_025: [If the device state is already DEVICE_STATE_STOPPED or DEVICE_STATE_STOPPING, amqp_device_stop shall return a non-zero result]
TEST_FUNCTION(device_stop_device_already_stopped)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_stop(config, TEST_current_time, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    // act
    int result = amqp_device_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_028: [If telemetry_messenger_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_MSG and the function shall return non-zero result]
// Tests_SRS_DEVICE_09_132: [If twin_messenger_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_MSG and the function shall return non-zero result]
// Tests_SRS_DEVICE_09_030: [If authentication_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_AUTH and the function shall return non-zero result]
TEST_FUNCTION(device_stop_DEVICE_STATE_STARTED_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);

    size_t i, n;
    for (i = 0, n = 1; i < n; i++)
    {
        // arrange
        AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

        umock_c_reset_all_calls();
        set_expected_calls_for_device_stop(config, TEST_current_time, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STARTED, TWIN_MESSENGER_STATE_STARTED);
        umock_c_negative_tests_snapshot();
        n = umock_c_negative_tests_call_count();

        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        int result = amqp_device_stop(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);

        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
        ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPING, TEST_on_state_changed_callback_saved_previous_state, error_msg);
        ASSERT_IS_TRUE(DEVICE_STATE_ERROR_AUTH == TEST_on_state_changed_callback_saved_new_state || DEVICE_STATE_ERROR_MSG == TEST_on_state_changed_callback_saved_new_state, error_msg);
        ASSERT_IS_NOT_NULL(handle, error_msg);

        // cleanup
        amqp_device_destroy(handle);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();
}


// Tests_SRS_DEVICE_09_026: [The device state shall be updated to DEVICE_STATE_STOPPING, and state changed callback invoked]
// Tests_SRS_DEVICE_09_031: [The device state shall be updated to DEVICE_STATE_STOPPED, and state changed callback invoked]
// Tests_SRS_DEVICE_09_032: [If no failures occur, amqp_device_stop shall return 0]
TEST_FUNCTION(device_stop_DEVICE_STATE_STARTING_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_stop(config, TEST_current_time, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    // act
    int result = amqp_device_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_027: [If `instance->messenger_handle` state is not TELEMETRY_MESSENGER_STATE_STOPPED, telemetry_messenger_stop shall be invoked]
// Tests_SRS_DEVICE_09_131: [If `instance->twin_messenger_handle` state is not TWIN_MESSENGER_STATE_STOPPED, twin_messenger_stop shall be invoked]
// Tests_SRS_DEVICE_09_029: [If CBS authentication is used, if `instance->authentication_handle` state is not AUTHENTICATION_STATE_STOPPED, authentication_stop shall be invoked]
TEST_FUNCTION(device_stop_DEVICE_STATE_STARTED_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_stop(config, TEST_current_time, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STARTED, TWIN_MESSENGER_STATE_STARTED);

    // act
    int result = amqp_device_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_027: [If `instance->messenger_handle` state is not TELEMETRY_MESSENGER_STATE_STOPPED, telemetry_messenger_stop shall be invoked]
// Tests_SRS_DEVICE_09_131: [If `instance->twin_messenger_handle` state is not TWIN_MESSENGER_STATE_STOPPED, twin_messenger_stop shall be invoked]
// Tests_SRS_DEVICE_09_029: [If CBS authentication is used, if `instance->authentication_handle` state is not AUTHENTICATION_STATE_STOPPED, authentication_stop shall be invoked]
TEST_FUNCTION(device_stop_DEVICE_STATE_STARTED_with_module_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config_with_module_id(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_stop(config, TEST_current_time, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STARTED, TWIN_MESSENGER_STATE_STARTED);

    // act
    int result = amqp_device_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STOPPED, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}


// Tests_SRS_DEVICE_09_012: [If `handle` is NULL, amqp_device_destroy shall return]
TEST_FUNCTION(device_destroy_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    amqp_device_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_DEVICE_09_013: [If the device is in state DEVICE_STATE_STARTED or DEVICE_STATE_STARTING, amqp_device_stop() shall be invoked]
// Tests_SRS_DEVICE_09_014: [`instance->messenger_handle shall be destroyed using telemetry_messenger_destroy()`]
// Tests_SRS_DEVICE_09_015: [If created, `instance->authentication_handle` shall be destroyed using authentication_destroy()`]
// Tests_SRS_DEVICE_09_016: [The contents of `instance->config` shall be detroyed and then it shall be freed]
TEST_FUNCTION(device_destroy_DEVICE_STATE_STARTED_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_destroy(handle, config, TEST_current_time, DEVICE_STATE_STARTED, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STARTED, TWIN_MESSENGER_STATE_STARTED);

    // act
    amqp_device_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
}


// Tests_SRS_DEVICE_09_105: [If `handle` or `send_status` is NULL, amqp_device_get_send_status shall return a non-zero result]
TEST_FUNCTION(device_get_send_status_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    DEVICE_SEND_STATUS send_status;

    // act
    int result = amqp_device_get_send_status(NULL, &send_status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_DEVICE_09_105: [If `handle` or `send_status` is NULL, amqp_device_get_send_status shall return a non-zero result]
TEST_FUNCTION(device_get_send_status_NULL_send_status)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_get_send_status(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_106: [The status of `instance->messenger_handle` shall be obtained using telemetry_messenger_get_send_status]
// Tests_SRS_DEVICE_09_108: [If telemetry_messenger_get_send_status returns TELEMETRY_MESSENGER_SEND_STATUS_IDLE, amqp_device_get_send_status return status DEVICE_SEND_STATUS_IDLE]
// Tests_SRS_DEVICE_09_110: [If amqp_device_get_send_status succeeds, it shall return zero as result]
TEST_FUNCTION(device_get_send_status_IDLE_success)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    TELEMETRY_MESSENGER_SEND_STATUS telemetry_messenger_get_send_status_result = TELEMETRY_MESSENGER_SEND_STATUS_IDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(telemetry_messenger_get_send_status(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &telemetry_messenger_get_send_status_result, sizeof(TELEMETRY_MESSENGER_SEND_STATUS))
        .SetReturn(0);

    // act
    DEVICE_SEND_STATUS send_status;
    int result = amqp_device_get_send_status(handle, &send_status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_SEND_STATUS_IDLE, send_status);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_106: [The status of `instance->messenger_handle` shall be obtained using telemetry_messenger_get_send_status]
// Tests_SRS_DEVICE_09_108: [If telemetry_messenger_get_send_status returns TELEMETRY_MESSENGER_SEND_STATUS_IDLE, amqp_device_get_send_status return status DEVICE_SEND_STATUS_IDLE]
// Tests_SRS_DEVICE_09_110: [If amqp_device_get_send_status succeeds, it shall return zero as result]
TEST_FUNCTION(device_get_send_status_IDLE_with_module_success)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config_with_module_id(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    TELEMETRY_MESSENGER_SEND_STATUS telemetry_messenger_get_send_status_result = TELEMETRY_MESSENGER_SEND_STATUS_IDLE;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(telemetry_messenger_get_send_status(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &telemetry_messenger_get_send_status_result, sizeof(TELEMETRY_MESSENGER_SEND_STATUS))
        .SetReturn(0);

    // act
    DEVICE_SEND_STATUS send_status;
    int result = amqp_device_get_send_status(handle, &send_status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_SEND_STATUS_IDLE, send_status);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}


// Tests_SRS_DEVICE_09_109: [If telemetry_messenger_get_send_status returns TELEMETRY_MESSENGER_SEND_STATUS_BUSY, amqp_device_get_send_status return status DEVICE_SEND_STATUS_BUSY]
TEST_FUNCTION(device_get_send_status_BUSY_success)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    TELEMETRY_MESSENGER_SEND_STATUS telemetry_messenger_get_send_status_result = TELEMETRY_MESSENGER_SEND_STATUS_BUSY;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(telemetry_messenger_get_send_status(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &telemetry_messenger_get_send_status_result, sizeof(TELEMETRY_MESSENGER_SEND_STATUS))
        .SetReturn(0);

    // act
    DEVICE_SEND_STATUS send_status;
    int result = amqp_device_get_send_status(handle, &send_status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, DEVICE_SEND_STATUS_BUSY, send_status);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_107: [If telemetry_messenger_get_send_status fails, amqp_device_get_send_status shall return a non-zero result]
TEST_FUNCTION(device_get_send_status_failure_checks)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(telemetry_messenger_get_send_status(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(1);

    // act
    DEVICE_SEND_STATUS send_status;
    int result = amqp_device_get_send_status(handle, &send_status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_066: [If `handle` or `on_message_received_callback` or `context` is NULL, amqp_device_subscribe_message shall return a non-zero result]
TEST_FUNCTION(device_subscribe_message_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_device_subscribe_message(NULL, TEST_on_message_received, TEST_VOID_PTR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_DEVICE_09_066: [If `handle` or `on_message_received_callback` or `context` is NULL, amqp_device_subscribe_message shall return a non-zero result]
TEST_FUNCTION(device_subscribe_message_NULL_callback)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_subscribe_message(handle, NULL, handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_066: [If `handle` or `on_message_received_callback` or `context` is NULL, amqp_device_subscribe_message shall return a non-zero result]
TEST_FUNCTION(device_subscribe_message_NULL_context)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_subscribe_message(handle, TEST_on_message_received, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_067: [telemetry_messenger_subscribe_for_messages shall be invoked passing `on_messenger_message_received_callback` and the user callback and context]
// Tests_SRS_DEVICE_09_069: [If no failures occur, amqp_device_subscribe_message shall return 0]
TEST_FUNCTION(device_subscribe_message_succeess)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    TEST_telemetry_messenger_subscribe_for_messages_return = 0;
    STRICT_EXPECTED_CALL(telemetry_messenger_subscribe_for_messages(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    // act
    int result = amqp_device_subscribe_message(handle, TEST_on_message_received, handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_068: [If telemetry_messenger_subscribe_for_messages fails, amqp_device_subscribe_message shall return a non-zero result]
TEST_FUNCTION(device_subscribe_message_failure_checks)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(telemetry_messenger_subscribe_for_messages(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(1);

    // act
    int result = amqp_device_subscribe_message(handle, TEST_on_message_received, handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_080: [If `handle` is NULL, amqp_device_set_retry_policy shall return a non-zero result]
TEST_FUNCTION(device_set_retry_policy_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_device_set_retry_policy(NULL, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 300);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_DEVICE_09_081: [amqp_device_set_retry_policy shall return a non-zero result]
TEST_FUNCTION(device_set_retry_policy_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_set_retry_policy(handle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 300);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_093: [If `handle` is NULL, amqp_device_retrieve_options shall return NULL]
TEST_FUNCTION(device_retrieve_options_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    OPTIONHANDLER_HANDLE options = amqp_device_retrieve_options(NULL);

    // assert
    ASSERT_IS_NULL(options);

    // cleanup
}

// Tests_SRS_DEVICE_09_094: [A OPTIONHANDLER_HANDLE instance, aka `options` shall be created using OptionHandler_Create]
// Tests_SRS_DEVICE_09_096: [If CBS authentication is used, `instance->authentication_handle` options shall be retrieved using authentication_retrieve_options]
// Tests_SRS_DEVICE_09_098: [The authentication options shall be added to `options` using OptionHandler_AddOption as DEVICE_OPTION_SAVED_AUTH_OPTIONS]
// Tests_SRS_DEVICE_09_099: [`instance->messenger_handle` options shall be retrieved using telemetry_messenger_retrieve_options]
// Tests_SRS_DEVICE_09_101: [The messenger options shall be added to `options` using OptionHandler_AddOption as DEVICE_OPTION_SAVED_MESSENGER_OPTIONS]
// Tests_SRS_DEVICE_09_104: [If no failures occur, a handle to `options` shall be return]
TEST_FUNCTION(device_retrieve_options_CBS_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_retrieve_options(config);

    // act
    OPTIONHANDLER_HANDLE result = amqp_device_retrieve_options(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_OPTIONHANDLER_HANDLE, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_096: [If CBS authentication is used, `instance->authentication_handle` options shall be retrieved using authentication_retrieve_options]
TEST_FUNCTION(device_retrieve_options_X509_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_X509);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_retrieve_options(config);

    // act
    OPTIONHANDLER_HANDLE result = amqp_device_retrieve_options(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_OPTIONHANDLER_HANDLE, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_095: [If OptionHandler_Create fails, amqp_device_retrieve_options shall return NULL]
// Tests_SRS_DEVICE_09_097: [If authentication_retrieve_options fails, amqp_device_retrieve_options shall return NULL]
// Tests_SRS_DEVICE_09_100: [If telemetry_messenger_retrieve_options fails, amqp_device_retrieve_options shall return NULL]
// Tests_SRS_DEVICE_09_102: [If any call to OptionHandler_AddOption fails, amqp_device_retrieve_options shall return NULL]
// Tests_SRS_DEVICE_09_103: [If any failure occurs, any memory allocated by amqp_device_retrieve_options shall be destroyed]
TEST_FUNCTION(device_retrieve_options_CBS_failure_checks)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_retrieve_options(config);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        OPTIONHANDLER_HANDLE result = amqp_device_retrieve_options(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();
    amqp_device_destroy(handle);
}


// Tests_SRS_DEVICE_09_082: [If `handle` or `name` or `value` are NULL, amqp_device_set_option shall return a non-zero result]
// Tests_SRS_DEVICE_09_083: [If `name` refers to authentication but CBS authentication is not used, amqp_device_set_option shall return a non-zero result]
TEST_FUNCTION(device_set_option_X509_AUTH_fails)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_X509);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    size_t value = 400;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, &value);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_085: [If authentication_set_option fails, amqp_device_set_option shall return a non-zero result]
TEST_FUNCTION(device_set_option_saved_auth_options_fails)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    size_t value = 400;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, &value);
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();

    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_087: [If telemetry_messenger_set_option fails, amqp_device_set_option shall return a non-zero result]
TEST_FUNCTION(device_set_option_saved_msgr_options_fails)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    size_t value = 400;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, &value);
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();

    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_084: [If `name` refers to authentication, it shall be passed along with `value` to authentication_set_option]
// Tests_SRS_DEVICE_09_092: [If no failures occur, amqp_device_set_option shall return 0]
TEST_FUNCTION(device_set_option_CBS_AUTH_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    size_t value = 400;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, &value);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_086: [If `name` refers to messenger module, it shall be passed along with `value` to telemetry_messenger_set_option]
// Tests_SRS_DEVICE_09_092: [If no failures occur, amqp_device_set_option shall return 0]
TEST_FUNCTION(device_set_option_MSGR_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    size_t value = 400;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_088: [If `name` is DEVICE_OPTION_SAVED_AUTH_OPTIONS but CBS authentication is not being used, amqp_device_set_option shall return a non-zero result]
TEST_FUNCTION(device_set_option_X509_saved_auth_options)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_X509);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    OPTIONHANDLER_HANDLE value = TEST_AUTH_OPTIONHANDLER_HANDLE;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_SAVED_AUTH_OPTIONS, &value); // no-op

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_SAVED_AUTH_OPTIONS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

TEST_FUNCTION(device_set_option_AUTH_saved_auth_options_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    OPTIONHANDLER_HANDLE value = TEST_AUTH_OPTIONHANDLER_HANDLE;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_SAVED_AUTH_OPTIONS, &value);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_SAVED_AUTH_OPTIONS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_089: [If `name` is DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, `value` shall be fed to `instance->messenger_handle` using OptionHandler_FeedOptions]
TEST_FUNCTION(device_set_option_MSGR_saved_msgr_options_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    OPTIONHANDLER_HANDLE value = TEST_MSGR_OPTIONHANDLER_HANDLE;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, &value);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_090: [If `name` is DEVICE_OPTION_SAVED_OPTIONS, `value` shall be fed to `instance` using OptionHandler_FeedOptions]
TEST_FUNCTION(device_set_option_saved_device_options_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    OPTIONHANDLER_HANDLE value = TEST_OPTIONHANDLER_HANDLE;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_SAVED_OPTIONS, &value);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_SAVED_OPTIONS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_091: [If any call to OptionHandler_FeedOptions fails, amqp_device_set_option shall return a non-zero result]
TEST_FUNCTION(device_set_option_saved_device_options_fails)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    OPTIONHANDLER_HANDLE value = TEST_OPTIONHANDLER_HANDLE;

    umock_c_reset_all_calls();
    set_expected_calls_for_device_set_option(handle, config, DEVICE_OPTION_SAVED_OPTIONS, &value);
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    // act
    int result = amqp_device_set_option(handle, DEVICE_OPTION_SAVED_OPTIONS, &value);

    // assert
    //ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();

    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_076: [If `handle` is NULL, amqp_device_unsubscribe_message shall return a non-zero result]
TEST_FUNCTION(device_unsubscribe_message_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_device_unsubscribe_message(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_DEVICE_09_077: [telemetry_messenger_unsubscribe_for_messages shall be invoked passing `instance->messenger_handle`]
// Tests_SRS_DEVICE_09_079: [If no failures occur, amqp_device_unsubscribe_message shall return 0]
TEST_FUNCTION(device_unsubscribe_message_succeess)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(telemetry_messenger_unsubscribe_for_messages(TEST_TELEMETRY_MESSENGER_HANDLE))
        .SetReturn(0);

    // act
    int result = amqp_device_unsubscribe_message(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_078: [If telemetry_messenger_unsubscribe_for_messages fails, amqp_device_unsubscribe_message shall return a non-zero result]
TEST_FUNCTION(device_unsubscribe_message_failure_checks)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(telemetry_messenger_unsubscribe_for_messages(TEST_TELEMETRY_MESSENGER_HANDLE))
        .SetReturn(1);

    // act
    int result = amqp_device_unsubscribe_message(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_051: [If `handle` or `message` are NULL, amqp_device_send_event_async shall return a non-zero result]
TEST_FUNCTION(telemetry_messenger_send_async_NULL_handle)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_send_event_async(NULL, TEST_IOTHUB_MESSAGE_LIST, TEST_on_device_d2c_event_send_complete_callback, TEST_ON_DEVICE_EVENT_SEND_COMPLETE_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_051: [If `handle` or `message` are NULL, amqp_device_send_event_async shall return a non-zero result]
TEST_FUNCTION(telemetry_messenger_send_async_NULL_message)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_send_event_async(handle, NULL, TEST_on_device_d2c_event_send_complete_callback, TEST_ON_DEVICE_EVENT_SEND_COMPLETE_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_053: [If `send_task` fails to be created, amqp_device_send_event_async shall return a non-zero value]
// Tests_SRS_DEVICE_09_056: [If telemetry_messenger_send_async fails, amqp_device_send_event_async shall return a non-zero value]
// Tests_SRS_DEVICE_09_057: [If any failures occur, amqp_device_send_event_async shall release all memory it has allocated]
TEST_FUNCTION(telemetry_messenger_send_async_failure_checks)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_send_async(config);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        int result = amqp_device_send_event_async(handle, TEST_IOTHUB_MESSAGE_LIST, TEST_on_device_d2c_event_send_complete_callback, TEST_ON_DEVICE_EVENT_SEND_COMPLETE_CONTEXT);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();

    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_052: [A structure (`send_task`) shall be created to track the send state of the message]
// Tests_SRS_DEVICE_09_054: [`send_task` shall contain the user callback and the context provided]
// Tests_SRS_DEVICE_09_055: [The message shall be sent using telemetry_messenger_send_async, passing `on_event_send_complete_messenger_callback` and `send_task`]
// Tests_SRS_DEVICE_09_058: [If no failures occur, amqp_device_send_event_async shall return 0]
TEST_FUNCTION(telemetry_messenger_send_async_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_send_async(config);

    // act
    int result = amqp_device_send_event_async(handle, TEST_IOTHUB_MESSAGE_LIST, TEST_on_device_d2c_event_send_complete_callback, TEST_ON_DEVICE_EVENT_SEND_COMPLETE_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    TEST_telemetry_messenger_send_async_saved_callback(TEST_telemetry_messenger_send_async_saved_message, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_telemetry_messenger_send_async_saved_context);

    amqp_device_destroy(handle);
}


// Tests_SRS_DEVICE_09_033: [If `handle` is NULL, amqp_device_do_work shall return]
TEST_FUNCTION(device_do_work_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    amqp_device_do_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_DEVICE_09_034: [If CBS authentication is used and authentication state is AUTHENTICATION_STATE_STOPPED, authentication_start shall be invoked]
// Tests_SRS_DEVICE_09_035: [If authentication_start fails, the device state shall be updated to DEVICE_STATE_ERROR_AUTH]
TEST_FUNCTION(device_do_work_authentication_start_fails)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_do_work(config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_036: [If authentication state is AUTHENTICATION_STATE_STARTING, the device shall track the time since last event change and timeout if needed]
// Tests_SRS_DEVICE_09_037: [If authentication_start times out, the device state shall be updated to DEVICE_STATE_ERROR_AUTH_TIMEOUT]
TEST_FUNCTION(device_do_work_authentication_start_times_out)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    time_t next_time = add_seconds(TEST_current_time, DEFAULT_AUTH_STATE_CHANGED_TIMEOUT_SECS + 1);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_do_work(config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    amqp_device_do_work(handle);
    set_authentication_state(AUTHENTICATION_STATE_STOPPED, AUTHENTICATION_STATE_STARTING, TEST_current_time);

    set_expected_calls_for_device_do_work(config, next_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTING, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_ERROR_AUTH_TIMEOUT, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_038: [If authentication state is AUTHENTICATION_STATE_ERROR and error code is AUTH_FAILED, the device state shall be updated to DEVICE_STATE_ERROR_AUTH]
TEST_FUNCTION(device_do_work_authentication_start_AUTH_FAILED)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_do_work(config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);
    amqp_device_do_work(handle);

    ASSERT_IS_NOT_NULL(TEST_authentication_create_saved_on_authentication_changed_callback);
    ASSERT_IS_NOT_NULL(TEST_authentication_create_saved_on_error_callback);

    set_authentication_state(AUTHENTICATION_STATE_STARTING, AUTHENTICATION_STATE_ERROR, TEST_current_time);
    TEST_authentication_create_saved_on_error_callback(TEST_authentication_create_saved_on_error_context, AUTHENTICATION_ERROR_AUTH_FAILED);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_ERROR_AUTH, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_039: [If authentication state is AUTHENTICATION_STATE_ERROR and error code is TIMEOUT, the device state shall be updated to DEVICE_STATE_ERROR_AUTH_TIMEOUT]
TEST_FUNCTION(device_do_work_authentication_start_AUTH_TIMEOUT)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    set_expected_calls_for_device_do_work(config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);
    amqp_device_do_work(handle);

    ASSERT_IS_NOT_NULL(TEST_authentication_create_saved_on_authentication_changed_callback);
    ASSERT_IS_NOT_NULL(TEST_authentication_create_saved_on_error_callback);

    set_authentication_state(AUTHENTICATION_STATE_STARTING, AUTHENTICATION_STATE_ERROR, TEST_current_time);
    TEST_authentication_create_saved_on_error_callback(TEST_authentication_create_saved_on_error_context, AUTHENTICATION_ERROR_AUTH_TIMEOUT);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_ERROR_AUTH_TIMEOUT, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_041: [If messenger state is TELEMETRY_MESSENGER_STATE_STOPPED, telemetry_messenger_start shall be invoked]
// Tests_SRS_DEVICE_09_042: [If telemetry_messenger_start fails, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
// Tests_SRS_DEVICE_09_045: [If messenger state is TELEMETRY_MESSENGER_STATE_ERROR, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
// Tests_SRS_DEVICE_09_125: [If TWIN messenger state is TWIN_MESSENGER_STATE_STOPPED, twin_messenger_start shall be invoked]
// Tests_SRS_DEVICE_09_126: [If twin_messenger_start fails, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
TEST_FUNCTION(device_do_work_telemetry_messenger_start_FAILED)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    crank_device_do_work(handle, config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);
    set_authentication_state(AUTHENTICATION_STATE_STOPPED, AUTHENTICATION_STATE_STARTING, TEST_current_time);
    set_authentication_state(AUTHENTICATION_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TEST_current_time);

    crank_device_do_work(handle, config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    ASSERT_IS_NOT_NULL(TEST_telemetry_messenger_create_saved_on_state_changed_callback);

    set_messenger_state(TELEMETRY_MESSENGER_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STARTING, TEST_current_time);
    set_twin_messenger_state(TWIN_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STARTING, TEST_current_time);
    set_messenger_state(TELEMETRY_MESSENGER_STATE_STARTING, TELEMETRY_MESSENGER_STATE_ERROR, TEST_current_time);

    set_expected_calls_for_device_do_work(config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_ERROR, TWIN_MESSENGER_STATE_STARTING);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_ERROR_MSG, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_043: [If messenger state is TELEMETRY_MESSENGER_STATE_STARTING, the device shall track the time since last event change and timeout if needed]
// Tests_SRS_DEVICE_09_127: [If TWIN messenger state is TWIN_MESSENGER_STATE_STARTING, the device shall track the time since last event change and timeout if needed]
// Tests_SRS_DEVICE_09_044: [If telemetry_messenger_start times out, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
TEST_FUNCTION(device_do_work_telemetry_messenger_start_timeout)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    time_t next_time = add_seconds(TEST_current_time, DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS + 1);

    umock_c_reset_all_calls();
    crank_device_do_work(handle, config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);
    set_authentication_state(AUTHENTICATION_STATE_STOPPED, AUTHENTICATION_STATE_STARTING, TEST_current_time);
    set_authentication_state(AUTHENTICATION_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TEST_current_time);

    crank_device_do_work(handle, config, TEST_current_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);

    ASSERT_IS_NOT_NULL(TEST_telemetry_messenger_create_saved_on_state_changed_callback);
    ASSERT_IS_NOT_NULL(TEST_twin_messenger_create_on_state_changed_callback);
    set_messenger_state(TELEMETRY_MESSENGER_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STARTING, TEST_current_time);
    set_twin_messenger_state(TWIN_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STARTING, TEST_current_time);

    set_expected_calls_for_device_do_work(config, next_time, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STARTING, TWIN_MESSENGER_STATE_STARTING);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_ERROR_MSG, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_040: [Messenger shall not be started if using CBS authentication and authentication start has not completed yet]
// Tests_SRS_DEVICE_09_124: [TWIN Messenger shall not be started if using CBS authentication and authentication start has not completed yet]
// Tests_SRS_DEVICE_09_046: [If messenger state is TELEMETRY_MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_STARTED]
// Tests_SRS_DEVICE_09_130: [If TWIN messenger state is TWIN_MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_STARTED]
// Tests_SRS_DEVICE_09_049: [If CBS is used for authentication and `instance->authentication_handle` state is not STOPPED or ERROR, authentication_do_work shall be invoked]
// Tests_SRS_DEVICE_09_050: [If `instance->messenger_handle` state is not STOPPED or ERROR, telemetry_messenger_do_work shall be invoked]
// Tests_SRS_DEVICE_09_134: [If `instance->twin_messenger_handle` state is not STOPPED or ERROR, twin_messenger_do_work shall be invoked]
TEST_FUNCTION(device_do_work_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    time_t t0 = TEST_current_time;
    time_t t1 = add_seconds(t0, DEFAULT_AUTH_STATE_CHANGED_TIMEOUT_SECS - 1);
    time_t t2 = add_seconds(t1, DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS - 2);
    time_t t3 = add_seconds(t2, DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS - 1);
    time_t t4 = add_seconds(t3, DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS + 1);

    umock_c_reset_all_calls();
    crank_device_do_work(handle, config, t0, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);
    set_authentication_state(AUTHENTICATION_STATE_STOPPED, AUTHENTICATION_STATE_STARTING, t0);

    crank_device_do_work(handle, config, t1, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTING, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);
    set_authentication_state(AUTHENTICATION_STATE_STARTING, AUTHENTICATION_STATE_STARTED, t1);

    crank_device_do_work(handle, config, t2, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STOPPED);
    set_messenger_state(TELEMETRY_MESSENGER_STATE_STOPPED, TELEMETRY_MESSENGER_STATE_STARTING, t2);
    set_twin_messenger_state(TWIN_MESSENGER_STATE_STOPPED, TWIN_MESSENGER_STATE_STARTING, t2);

    crank_device_do_work(handle, config, t3, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STARTING, TWIN_MESSENGER_STATE_STARTING);
    set_messenger_state(TELEMETRY_MESSENGER_STATE_STARTING, TELEMETRY_MESSENGER_STATE_STARTED, t3);
    set_twin_messenger_state(TWIN_MESSENGER_STATE_STARTING, TWIN_MESSENGER_STATE_STARTED, t3);

    set_expected_calls_for_device_do_work(config, t4, DEVICE_STATE_STARTING, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_STARTED, TWIN_MESSENGER_STATE_STARTED);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTING, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTED, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}


// Tests_SRS_DEVICE_09_047: [If CBS authentication is used and authentication state is not AUTHENTICATION_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_AUTH]
TEST_FUNCTION(device_do_work_STARTED_auth_unexpected_state)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    TEST_authentication_create_saved_on_error_callback(TEST_authentication_create_saved_on_error_context, AUTHENTICATION_ERROR_SAS_REFRESH_TIMEOUT);
    set_authentication_state(AUTHENTICATION_STATE_STARTED, AUTHENTICATION_STATE_ERROR, TEST_current_time);

    set_expected_calls_for_device_do_work(config, TEST_current_time, DEVICE_STATE_STARTED, AUTHENTICATION_STATE_ERROR, TELEMETRY_MESSENGER_STATE_STARTED, TWIN_MESSENGER_STATE_STARTED);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTED, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_ERROR_AUTH, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_048: [If messenger state is not TELEMETRY_MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
TEST_FUNCTION(device_do_work_STARTED_messenger_unexpected_state)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    set_messenger_state(TELEMETRY_MESSENGER_STATE_STARTED, TELEMETRY_MESSENGER_STATE_ERROR, TEST_current_time);

    set_expected_calls_for_device_do_work(config, TEST_current_time, DEVICE_STATE_STARTED, AUTHENTICATION_STATE_STARTED, TELEMETRY_MESSENGER_STATE_ERROR, TWIN_MESSENGER_STATE_STARTED);

    // act
    amqp_device_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_STARTED, TEST_on_state_changed_callback_saved_previous_state);
    ASSERT_ARE_EQUAL(int, DEVICE_STATE_ERROR_MSG, TEST_on_state_changed_callback_saved_new_state);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_device_destroy(handle);
}


// Tests_SRS_DEVICE_09_059: [If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, D2C_EVENT_SEND_COMPLETE_RESULT_OK shall be reported as `event_send_complete`]
// Tests_SRS_DEVICE_09_060: [If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE shall be reported as `event_send_complete`]
// Tests_SRS_DEVICE_09_061: [If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING shall be reported as `event_send_complete`]
// Tests_SRS_DEVICE_09_062: [If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT shall be reported as `event_send_complete`]
// Tests_SRS_DEVICE_09_063: [If `ev_send_comp_result` is TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED, D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED shall be reported as `event_send_complete`]
// Tests_SRS_DEVICE_09_064: [If provided, the user callback and context saved in `send_task` shall be invoked passing the device `event_send_complete`]
// Tests_SRS_DEVICE_09_065: [The memory allocated for `send_task` shall be released]
TEST_FUNCTION(on_event_send_complete_messenger_callback_succeeds)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_device(config, TEST_current_time);

    TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT messenger_results[5];
    messenger_results[0] = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK;
    messenger_results[1] = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE;
    messenger_results[2] = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING;
    messenger_results[3] = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT;
    messenger_results[4] = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED;

    D2C_EVENT_SEND_RESULT device_results[5];
    device_results[0] = D2C_EVENT_SEND_COMPLETE_RESULT_OK;
    device_results[1] = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE;
    device_results[2] = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING;
    device_results[3] = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT;
    device_results[4] = D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED;

    // act
    int i;
    for (i = 0; i < 5; i++)
    {
        printf("Verifying on_event_send_complete for enum value %d\r\n", i);

        umock_c_reset_all_calls();
        set_expected_calls_for_device_send_async(config);

        int result = amqp_device_send_event_async(handle, TEST_IOTHUB_MESSAGE_LIST, TEST_on_device_d2c_event_send_complete_callback, TEST_ON_DEVICE_EVENT_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, 0, result);

        ASSERT_IS_NOT_NULL(TEST_telemetry_messenger_send_async_saved_callback);

        EXPECTED_CALL(free(IGNORED_PTR_ARG));
        TEST_telemetry_messenger_send_async_saved_callback(TEST_telemetry_messenger_send_async_saved_message, messenger_results[i], TEST_telemetry_messenger_send_async_saved_context);

        ASSERT_ARE_EQUAL(int, device_results[i], TEST_on_device_d2c_event_send_complete_callback_saved_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_070: [If `iothub_message_handle` or `context` is NULL, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED]
TEST_FUNCTION(on_messenger_message_received_callback_NULL_handle)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    TEST_telemetry_messenger_subscribe_for_messages_return = 0;
    STRICT_EXPECTED_CALL(telemetry_messenger_subscribe_for_messages(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    (void)amqp_device_subscribe_message(handle, TEST_on_message_received, handle);
    ASSERT_IS_NOT_NULL(TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback);

    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_SOURCE_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();

    // act
    TELEMETRY_MESSENGER_DISPOSITION_RESULT result = TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback(
        NULL,
        &disposition_info,
        TEST_telemetry_messenger_subscribe_for_messages_saved_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED, result);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_070: [If `iothub_message_handle` or `context` is NULL, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED]
TEST_FUNCTION(on_messenger_message_received_callback_NULL_context)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    TEST_telemetry_messenger_subscribe_for_messages_return = 0;
    STRICT_EXPECTED_CALL(telemetry_messenger_subscribe_for_messages(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    (void)amqp_device_subscribe_message(handle, TEST_on_message_received, handle);
    ASSERT_IS_NOT_NULL(TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback);

    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_SOURCE_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();

    // act
    TELEMETRY_MESSENGER_DISPOSITION_RESULT result = TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback(
        TEST_IOTHUB_MESSAGE_HANDLE,
        &disposition_info,
        NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED, result);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_071: [The user callback shall be invoked, passing the context it provided]
// Tests_SRS_DEVICE_09_072: [If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED]
// Tests_SRS_DEVICE_09_073: [If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED]
// Tests_SRS_DEVICE_09_074: [If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED]
// Tests_SRS_DEVICE_09_119: [A DEVICE_MESSAGE_DISPOSITION_INFO instance shall be created containing a copy of `disposition_info->source` and `disposition_info->message_id`]
// Tests_SRS_DEVICE_09_121: [on_messenger_message_received_callback shall release the memory allocated for DEVICE_MESSAGE_DISPOSITION_INFO]
TEST_FUNCTION(on_messenger_message_received_callback_succeess)
{
    // arrange
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    TELEMETRY_MESSENGER_DISPOSITION_RESULT messenger_results[3];
    messenger_results[0] = TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED;
    messenger_results[1] = TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED;
    messenger_results[2] = TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED;

    DEVICE_MESSAGE_DISPOSITION_RESULT device_results[3];
    device_results[0] = DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED;
    device_results[1] = DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED;
    device_results[2] = DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED;

    umock_c_reset_all_calls();
    TEST_telemetry_messenger_subscribe_for_messages_return = 0;
    STRICT_EXPECTED_CALL(telemetry_messenger_subscribe_for_messages(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    (void)amqp_device_subscribe_message(handle, TEST_on_message_received, handle);
    ASSERT_IS_NOT_NULL(TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback);

    // act
    int i;
    for (i = 0; i < 3; i++)
    {
        printf("Verifying on_messenger_message_received_callback for enum value %d\r\n", i);

        TEST_on_message_received_return = device_results[i];

        TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
        disposition_info.source = TEST_MESSAGE_SOURCE_NAME_CHAR_PTR;
        disposition_info.message_id = TEST_MESSAGE_ID;

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(malloc(sizeof(DEVICE_MESSAGE_DISPOSITION_INFO)));
        EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));

        TELEMETRY_MESSENGER_DISPOSITION_RESULT result = TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback(
            TEST_IOTHUB_MESSAGE_HANDLE,
            &disposition_info,
            TEST_telemetry_messenger_subscribe_for_messages_saved_context);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, messenger_results[i], result);
    }

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_120: [If the DEVICE_MESSAGE_DISPOSITION_INFO instance fails to be created, on_messenger_message_received_callback shall return TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED]
TEST_FUNCTION(on_messenger_message_received_callback_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_and_crank_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    TEST_telemetry_messenger_subscribe_for_messages_return = 0;
    STRICT_EXPECTED_CALL(telemetry_messenger_subscribe_for_messages(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    (void)amqp_device_subscribe_message(handle, TEST_on_message_received, handle);
    ASSERT_IS_NOT_NULL(TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback);

    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_SOURCE_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(sizeof(DEVICE_MESSAGE_DISPOSITION_INFO)));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 2 || i == 3)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        TELEMETRY_MESSENGER_DISPOSITION_RESULT result = TEST_telemetry_messenger_subscribe_for_messages_saved_on_message_received_callback(
            TEST_IOTHUB_MESSAGE_HANDLE,
            &disposition_info,
            TEST_telemetry_messenger_subscribe_for_messages_saved_context);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED, result, error_msg);
    }

    // cleanup
    amqp_device_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_DEVICE_09_113: [A TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance shall be created with a copy of the `source` and `message_id` contained in `disposition_info`]
// Tests_SRS_DEVICE_09_115: [`telemetry_messenger_send_message_disposition()` shall be invoked passing the TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance and the corresponding TELEMETRY_MESSENGER_DISPOSITION_RESULT]
// Tests_SRS_DEVICE_09_117: [amqp_device_send_message_disposition() shall destroy the TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO instance]
// Tests_SRS_DEVICE_09_118: [If no failures occurr, amqp_device_send_message_disposition() shall return 0]
TEST_FUNCTION(device_send_message_disposition_succeess)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    DEVICE_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_SOURCE_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(sizeof(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO)));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(telemetry_messenger_send_message_disposition(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED))
        .IgnoreArgument(2);
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    int result = amqp_device_send_message_disposition(handle, &disposition_info, DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_111: [If `device_handle` or `disposition_info` are NULL, amqp_device_send_message_disposition() shall fail and return MU_FAILURE]
TEST_FUNCTION(device_send_message_NULL_disposition_info)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_send_message_disposition(handle, NULL, DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_111: [If `device_handle` or `disposition_info` are NULL, amqp_device_send_message_disposition() shall fail and return MU_FAILURE]
TEST_FUNCTION(device_send_message_NULL_handle)
{
    // arrange
    DEVICE_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_SOURCE_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_send_message_disposition(NULL, &disposition_info, DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_DEVICE_09_112: [If `disposition_info->source` is NULL, amqp_device_send_message_disposition() shall fail and return MU_FAILURE]
TEST_FUNCTION(device_send_message_NULL_disposition_info_source)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    DEVICE_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = NULL;
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_send_message_disposition(handle, &disposition_info, DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_114: [If the TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO fails to be created, amqp_device_send_message_disposition() shall fail and return MU_FAILURE]
// Tests_SRS_DEVICE_09_116: [If `telemetry_messenger_send_message_disposition()` fails, amqp_device_send_message_disposition() shall fail and return MU_FAILURE]
TEST_FUNCTION(device_send_message_disposition_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    DEVICE_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_SOURCE_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(sizeof(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO)));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(telemetry_messenger_send_message_disposition(TEST_TELEMETRY_MESSENGER_HANDLE, IGNORED_PTR_ARG, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED))
        .IgnoreArgument(2);
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 3 || i == 4)
        {
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        int result = amqp_device_send_message_disposition(handle, &disposition_info, DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }


    // cleanup
    amqp_device_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_DEVICE_09_153: [twin_messenger_get_twin_async shall be invoked ]
// Tests_SRS_DEVICE_09_155: [If no failures occur, amqp_device_get_twin_async shall return 0]
TEST_FUNCTION(device_get_twin_async_succeess)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(twin_messenger_get_twin_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    int result = amqp_device_get_twin_async(handle, on_device_get_twin_completed_callback, (void*)0x4567);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(get_twin_context);

    // cleanup
    free(get_twin_context);
    amqp_device_destroy(handle);
}

TEST_FUNCTION(device_get_twin_async_callback_succeess)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(twin_messenger_get_twin_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    (void)amqp_device_get_twin_async(handle, on_device_get_twin_completed_callback, (void*)0x4567);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    ASSERT_IS_NOT_NULL(get_twin_callback);
    ASSERT_IS_NOT_NULL(get_twin_context);

    const char* payload = "{ 'a': 'b' }";
    size_t length = strlen(payload);

    // act
    get_twin_callback(TWIN_UPDATE_TYPE_COMPLETE, payload, length, get_twin_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, TWIN_UPDATE_TYPE_COMPLETE, dvc_get_twin_update_type);
    ASSERT_ARE_EQUAL(void_ptr, payload, dvc_get_twin_message);
    ASSERT_ARE_EQUAL(int, length, dvc_get_twin_length);
    ASSERT_ARE_EQUAL(void_ptr, (void*)0x4567, dvc_get_twin_context);

    // cleanup
    amqp_device_destroy(handle);
}

// Tests_SRS_DEVICE_09_154: [If twin_messenger_get_twin_async fails, amqp_device_get_twin_async shall return a non-zero value]
TEST_FUNCTION(device_get_twin_async_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(twin_messenger_get_twin_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (!umock_c_negative_tests_can_call_fail(index))
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        int result = amqp_device_get_twin_async(handle, on_device_get_twin_completed_callback, (void*)0x4567);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, tmp_msg);
    }

    // cleanup
    amqp_device_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_DEVICE_09_152: [If `handle` or `on_device_get_twin_completed_callback` are NULL, amqp_device_get_twin_async shall return a non-zero result]
TEST_FUNCTION(device_get_twin_async_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_device_get_twin_async(NULL, on_device_get_twin_completed_callback, (void*)0x4567);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_DEVICE_09_152: [If `handle` or `on_device_get_twin_completed_callback` are NULL, amqp_device_get_twin_async shall return a non-zero result]
TEST_FUNCTION(device_get_twin_async_NULL_callback)
{
    // arrange
    AMQP_DEVICE_CONFIG* config = get_device_config(DEVICE_AUTH_MODE_CBS);
    AMQP_DEVICE_HANDLE handle = create_and_start_device(config, TEST_current_time);

    umock_c_reset_all_calls();

    // act
    int result = amqp_device_get_twin_async(handle, NULL, (void*)0x4567);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_device_destroy(handle);
}

END_TEST_SUITE(iothubtransport_amqp_device_ut)
