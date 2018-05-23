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

void* real_malloc(size_t size)
{
    return malloc(size);
}

void real_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"
#include "umock_c_negative_tests.h"
#include "umocktypes.h"
#include "umocktypes_c.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h" 
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_uamqp_c/amqp_definitions_fields.h"
#include "azure_uamqp_c/messaging.h"
#include "internal/iothub_client_private.h"
#include "internal/iothubtransport_amqp_messenger.h"

#undef ENABLE_MOCKS

#include "internal/iothubtransport_amqp_twin_messenger.h"

DEFINE_ENUM_STRINGS(AMQP_MESSENGER_SEND_STATUS, AMQP_MESSENGER_SEND_STATUS_VALUES);
DEFINE_ENUM_STRINGS(AMQP_MESSENGER_SEND_RESULT, AMQP_MESSENGER_SEND_RESULT_VALUES);
DEFINE_ENUM_STRINGS(AMQP_MESSENGER_REASON, AMQP_MESSENGER_REASON_VALUES);
DEFINE_ENUM_STRINGS(AMQP_MESSENGER_DISPOSITION_RESULT, AMQP_MESSENGER_DISPOSITION_RESULT_VALUES);
DEFINE_ENUM_STRINGS(AMQP_MESSENGER_STATE, AMQP_MESSENGER_STATE_VALUES);

typedef enum TWIN_OPERATION_TYPE_TAG
{
    TWIN_OPERATION_TYPE_PATCH,
    TWIN_OPERATION_TYPE_GET,
    TWIN_OPERATION_TYPE_PUT,
    TWIN_OPERATION_TYPE_DELETE
} TWIN_OPERATION_TYPE;

typedef enum TWIN_SUBSCRIPTION_STATE_TAG
{
    TWIN_SUBSCRIPTION_STATE_NOT_SUBSCRIBED,
    TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES,
    TWIN_SUBSCRIPTION_STATE_GETTING_COMPLETE_PROPERTIES,
    TWIN_SUBSCRIPTION_STATE_SUBSCRIBE_FOR_UPDATES,
    TWIN_SUBSCRIPTION_STATE_SUBSCRIBING,
    TWIN_SUBSCRIPTION_STATE_SUBSCRIBED,
    TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBE,
    TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBING
} TWIN_SUBSCRIPTION_STATE;

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}



#define UNIQUE_ID_BUFFER_SIZE                                37
#define TEST_UNIQUE_ID                                       "A1234DE234A1234DE234A1234DE234A1234DEA1234DE234A1234DE234A1234DE234A1234DEA1234DE234A1234DE234A1234DE234A1234DE"

#define TEST_DEVICE_ID                                       "my_device"
#define TEST_DEVICE_ID_STRING_HANDLE                         (STRING_HANDLE)0x4442
#define TEST_IOTHUB_HOST_FQDN_STRING_HANDLE                  (STRING_HANDLE)0x4443
#define TEST_IOTHUB_HOST_FQDN                                "some.fqdn.com"
#define TEST_CLIENT_VERSION_STR                              "client x (version y)"
#define TEST_ON_STATE_CHANGED_CB_CONTEXT                     (void*)0x4445
#define TEST_STRING_HANDLE                                   (STRING_HANDLE)0x4446
#define TEST_SESSION_HANDLE                                  (SESSION_HANDLE)0x4447
#define TEST_AMQP_MESSENGER_HANDLE                           (AMQP_MESSENGER_HANDLE)0x4448
#define TEST_LIST_ITEM_HANDLE                                (LIST_ITEM_HANDLE)0x4477
#define TEST_OPTIONHANDLER_HANDLE                            (OPTIONHANDLER_HANDLE)0x4485
#define TEST_MESSAGE_HANDLE                                  (MESSAGE_HANDLE)0x4486
#define TEST_MAP_AMQP_VALUE                                  (AMQP_VALUE)0x4487
#define TEST_NULL_AMQP_VALUE                                 (AMQP_VALUE)0x4488
#define TEST_STRING_AMQP_VALUE                               (AMQP_VALUE)0x4489
#define TEST_SYMBOL_AMQP_VALUE                               (AMQP_VALUE)0x4490
#define TEST_MSG_ANNOTATIONS_AMQP_VALUE                      (AMQP_VALUE)0x4491
#define TEST_PROPERTIES_HANDLE                               (PROPERTIES_HANDLE)0x4492

#define INDEFINITE_TIME                                      ((time_t)-1)
#define DEFAULT_TWIN_SEND_LINK_SOURCE_NAME                   "twin"
#define DEFAULT_TWIN_RECEIVE_LINK_TARGET_NAME                "twin"

#define TWIN_MESSAGE_PROPERTY_OPERATION                      "operation"
#define TWIN_MESSAGE_PROPERTY_RESOURCE                       "resource"
#define TWIN_MESSAGE_PROPERTY_VERSION                        "version"
#define TWIN_MESSAGE_PROPERTY_STATUS                         "status"

#define TWIN_RESOURCE_DESIRED                                "/notifications/twin/properties/desired"
#define TWIN_RESOURCE_REPORTED                               "/properties/reported"

#define CLIENT_VERSION_PROPERTY_NAME                         "com.microsoft:client-version"
#define TWIN_CORRELATION_ID_PROPERTY_NAME                    "com.microsoft:channel-correlation-id"
#define TWIN_API_VERSION_PROPERTY_NAME                       "com.microsoft:api-version"
#define TWIN_API_VERSION_NUMBER                              "2016-11-14"

#define TEST_ATTACH_PROPERTIES                               (MAP_HANDLE)0x4444
#define UNIQUE_ID_BUFFER_SIZE                                37

static const char* TWIN_OPERATION_PATCH = "PATCH";
static const char* TWIN_OPERATION_GET = "GET";
static const char* TWIN_OPERATION_PUT = "PUT";
static const char* TWIN_OPERATION_DELETE = "DELETE";

static const unsigned char* TWIN_REPORTED_PROPERTIES = (const unsigned char*)"{ \"reportedStateProperty0\": \"reportedStateProperty0\", \"reportedStateProperty1\": \"reportedStateProperty1\" }";
static int TWIN_REPORTED_PROPERTIES_LENGTH = 117;

static time_t g_initial_time;
static time_t g_initial_time_plus_30_secs;
static time_t g_initial_time_plus_60_secs;
static time_t g_initial_time_plus_90_secs;
static time_t g_initial_time_plus_300_secs;

static CONSTBUFFER TEST_CONSTBUFFER;


// ---------- General Helpers ---------- //

#ifdef __cplusplus
extern "C"
{
#endif

static int TEST_link_set_max_message_size_result;
int TEST_amqpvalue_set_map_value_result;
int TEST_link_set_attach_properties_result;

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
        result = __FAILURE__;
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

    if (i != j)
    {
        saved_malloc_returns_count--;
        real_free(ptr);
    }
}

static AMQP_MESSENGER_CONFIG TEST_amqp_messenger_create_config;
static AMQP_MESSENGER_HANDLE TEST_amqp_messenger_create_return;
static AMQP_MESSENGER_HANDLE TEST_amqp_messenger_create(const AMQP_MESSENGER_CONFIG* messenger_config)
{
    TEST_amqp_messenger_create_config.device_id = (char*)messenger_config->device_id;
    TEST_amqp_messenger_create_config.iothub_host_fqdn = messenger_config->iothub_host_fqdn;
    TEST_amqp_messenger_create_config.client_version = messenger_config->client_version;

    TEST_amqp_messenger_create_config.send_link.target_suffix = messenger_config->send_link.target_suffix;
    TEST_amqp_messenger_create_config.send_link.attach_properties = messenger_config->send_link.attach_properties;
    TEST_amqp_messenger_create_config.send_link.snd_settle_mode = messenger_config->send_link.snd_settle_mode;
    TEST_amqp_messenger_create_config.send_link.rcv_settle_mode = messenger_config->send_link.rcv_settle_mode;
    TEST_amqp_messenger_create_config.receive_link.source_suffix = messenger_config->receive_link.source_suffix;
    TEST_amqp_messenger_create_config.receive_link.attach_properties = messenger_config->receive_link.attach_properties;
    TEST_amqp_messenger_create_config.receive_link.snd_settle_mode = messenger_config->receive_link.snd_settle_mode;
    TEST_amqp_messenger_create_config.receive_link.rcv_settle_mode = messenger_config->receive_link.rcv_settle_mode;
    TEST_amqp_messenger_create_config.on_state_changed_callback = messenger_config->on_state_changed_callback;
    TEST_amqp_messenger_create_config.on_state_changed_context = messenger_config->on_state_changed_context;
    TEST_amqp_messenger_create_config.on_subscription_changed_callback = messenger_config->on_subscription_changed_callback;
    TEST_amqp_messenger_create_config.on_subscription_changed_context = messenger_config->on_subscription_changed_context;

    return TEST_amqp_messenger_create_return;
}

#ifdef __cplusplus
extern "C"
{
#endif

    SINGLYLINKEDLIST_HANDLE real_singlylinkedlist_create(void);
    void real_singlylinkedlist_destroy(SINGLYLINKEDLIST_HANDLE list);
    LIST_ITEM_HANDLE real_singlylinkedlist_add(SINGLYLINKEDLIST_HANDLE list, const void* item);
    int real_singlylinkedlist_remove(SINGLYLINKEDLIST_HANDLE list, LIST_ITEM_HANDLE item_handle);
    LIST_ITEM_HANDLE real_singlylinkedlist_get_head_item(SINGLYLINKEDLIST_HANDLE list);
    LIST_ITEM_HANDLE real_singlylinkedlist_get_next_item(LIST_ITEM_HANDLE item_handle);
    LIST_ITEM_HANDLE real_singlylinkedlist_find(SINGLYLINKEDLIST_HANDLE list, LIST_MATCH_FUNCTION match_function, const void* match_context);
    const void* real_singlylinkedlist_item_get_value(LIST_ITEM_HANDLE item_handle);
    int real_singlylinkedlist_foreach(SINGLYLINKEDLIST_HANDLE list, LIST_ACTION_FUNCTION action_function, const void* match_context);
    int real_singlylinkedlist_remove_if(SINGLYLINKEDLIST_HANDLE list, LIST_CONDITION_FUNCTION condition_function, const void* match_context);

    CONSTBUFFER_HANDLE real_CONSTBUFFER_Create(const unsigned char* source, size_t size);
    CONSTBUFFER_HANDLE real_CONSTBUFFER_CreateFromBuffer(BUFFER_HANDLE buffer);
    CONSTBUFFER_HANDLE real_CONSTBUFFER_Clone(CONSTBUFFER_HANDLE constbufferHandle);
    const CONSTBUFFER* real_CONSTBUFFER_GetContent(CONSTBUFFER_HANDLE constbufferHandle);
    void real_CONSTBUFFER_Destroy(CONSTBUFFER_HANDLE constbufferHandle);

#ifdef __cplusplus
}
#endif


static time_t add_seconds(time_t base_time, int seconds)
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

// ---------- Callbacks ---------- //

static void* TEST_on_state_changed_callback_context;
static TWIN_MESSENGER_STATE TEST_on_state_changed_callback_previous_state;
static TWIN_MESSENGER_STATE TEST_on_state_changed_callback_new_state;
static void TEST_on_state_changed_callback(void* context, TWIN_MESSENGER_STATE previous_state, TWIN_MESSENGER_STATE new_state)
{
    TEST_on_state_changed_callback_context = context;
    TEST_on_state_changed_callback_previous_state = previous_state;
    TEST_on_state_changed_callback_new_state = new_state;
}

static TWIN_REPORT_STATE_RESULT TEST_on_report_state_complete_callback_result;
static TWIN_REPORT_STATE_REASON TEST_on_report_state_complete_callback_reason;
static int TEST_on_report_state_complete_callback_status_code;
static const void* TEST_on_report_state_complete_callback_context;
static size_t TEST_on_report_state_complete_callback_reason_NONE_count;
static size_t TEST_on_report_state_complete_callback_reason_INTERNAL_ERROR_count;
static size_t TEST_on_report_state_complete_callback_reason_TIMEOUT_count;
static size_t TEST_on_report_state_complete_callback_reason_FAIL_SENDING_count;
static size_t TEST_on_report_state_complete_callback_reason_INVALID_RESPONSE_count;
static size_t TEST_on_report_state_complete_callback_reason_MESSENGER_DESTROYED_count;
static size_t TEST_on_report_state_complete_callback_result_SUCCESS_count;
static size_t TEST_on_report_state_complete_callback_result_ERROR_count;
static size_t TEST_on_report_state_complete_callback_result_CANCELLED_count;
static void TEST_on_report_state_complete_callback(TWIN_REPORT_STATE_RESULT result, TWIN_REPORT_STATE_REASON reason, int status_code, const void* context)
{
    TEST_on_report_state_complete_callback_result = result;
    TEST_on_report_state_complete_callback_reason = reason;
    TEST_on_report_state_complete_callback_status_code = status_code;
    TEST_on_report_state_complete_callback_context = context;

    switch (result)
    {
    case TWIN_REPORT_STATE_RESULT_SUCCESS:
        TEST_on_report_state_complete_callback_result_SUCCESS_count++;
        break;
    case TWIN_REPORT_STATE_RESULT_ERROR:
        TEST_on_report_state_complete_callback_result_ERROR_count++;
        break;
    case TWIN_REPORT_STATE_RESULT_CANCELLED:
        TEST_on_report_state_complete_callback_result_CANCELLED_count++;
        break;
    default:
        break;
    }

    switch (reason)
    {
    case TWIN_REPORT_STATE_REASON_NONE:
        TEST_on_report_state_complete_callback_reason_NONE_count++;
        break;
    case TWIN_REPORT_STATE_REASON_INTERNAL_ERROR:
        TEST_on_report_state_complete_callback_reason_INTERNAL_ERROR_count++;
        break;
    case TWIN_REPORT_STATE_REASON_TIMEOUT:
        TEST_on_report_state_complete_callback_reason_TIMEOUT_count++;
        break;
    case TWIN_REPORT_STATE_REASON_FAIL_SENDING:
        TEST_on_report_state_complete_callback_reason_FAIL_SENDING_count++;
        break;
    case TWIN_REPORT_STATE_REASON_INVALID_RESPONSE:
        TEST_on_report_state_complete_callback_reason_INVALID_RESPONSE_count++;
        break;
    case TWIN_REPORT_STATE_REASON_MESSENGER_DESTROYED:
        TEST_on_report_state_complete_callback_reason_MESSENGER_DESTROYED_count++;
        break;
    default:
        break;
    }
}

typedef struct DOWORK_TEST_PROFILE_TAG
{
    time_t current_time;
    TWIN_MESSENGER_STATE current_state;
    TWIN_SUBSCRIPTION_STATE subscription_state;
    size_t number_of_pending_patches;
    size_t number_of_expired_pending_patches;
    size_t number_of_pending_operations;
    size_t number_of_expired_pending_operations;
} DOWORK_TEST_PROFILE;

static void reset_dowork_test_profile(DOWORK_TEST_PROFILE* dwtp)
{
    dwtp->current_time = g_initial_time;
    dwtp->current_state = TWIN_MESSENGER_STATE_STOPPED;
    dwtp->subscription_state = TWIN_SUBSCRIPTION_STATE_NOT_SUBSCRIBED;
    dwtp->number_of_pending_patches = 0;
    dwtp->number_of_expired_pending_patches = 0;
    dwtp->number_of_pending_operations = 0;
    dwtp->number_of_expired_pending_operations = 0;
}


// ---------- Expected Calls ---------- //

static void set_generate_unique_id_expected_calls()
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, UNIQUE_ID_BUFFER_SIZE));
}

static void set_generate_twin_correlation_id_expected_calls()
{
    set_generate_unique_id_expected_calls();
    //STRICT_EXPECTED_CALL(strlen(IGNORED_NUM_ARG));
    //STRICT_EXPECTED_CALL(strlen(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    // sprintf
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_create_link_attach_properties_expected_calls(TWIN_MESSENGER_CONFIG* config)
{
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(TEST_ATTACH_PROPERTIES);
    set_generate_twin_correlation_id_expected_calls();
    STRICT_EXPECTED_CALL(Map_Add(TEST_ATTACH_PROPERTIES, CLIENT_VERSION_PROPERTY_NAME, config->client_version));
    STRICT_EXPECTED_CALL(Map_Add(TEST_ATTACH_PROPERTIES, TWIN_CORRELATION_ID_PROPERTY_NAME, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Add(TEST_ATTACH_PROPERTIES, TWIN_API_VERSION_PROPERTY_NAME, TWIN_API_VERSION_NUMBER));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_destroy_link_attach_properties_expected_calls()
{
    STRICT_EXPECTED_CALL(Map_Destroy(TEST_ATTACH_PROPERTIES));
}

static void set_expected_calls_for_twin_messenger_create(TWIN_MESSENGER_CONFIG* config)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->client_version))
        .CopyOutArgumentBuffer(1, &config->client_version, sizeof(config->client_version));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->device_id))
        .CopyOutArgumentBuffer(1, &config->device_id, sizeof(config->device_id));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->iothub_host_fqdn))
        .CopyOutArgumentBuffer(1, &config->iothub_host_fqdn, sizeof(config->iothub_host_fqdn));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    STRICT_EXPECTED_CALL(singlylinkedlist_create());

    set_create_link_attach_properties_expected_calls(config);

    STRICT_EXPECTED_CALL(amqp_messenger_create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqp_messenger_subscribe_for_messages(TEST_AMQP_MESSENGER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    set_destroy_link_attach_properties_expected_calls();
}

static void set_twin_messenger_report_state_async_expected_calls(CONSTBUFFER_HANDLE report, time_t current_time)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_Clone(report));
    STRICT_EXPECTED_CALL(get_time(NULL))
        .SetReturn(current_time);
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void set_twin_messenger_start_expected_calls()
{
    STRICT_EXPECTED_CALL(amqp_messenger_start(IGNORED_PTR_ARG, TEST_SESSION_HANDLE));
}

static void set_twin_messenger_stop_expected_calls()
{
    STRICT_EXPECTED_CALL(amqp_messenger_stop(IGNORED_PTR_ARG));
}

static void set_twin_messenger_retrieve_options_expected_calls()
{
    STRICT_EXPECTED_CALL(amqp_messenger_retrieve_options(TEST_AMQP_MESSENGER_HANDLE));
}

static void set_process_timeouts_expected_calls(time_t current_time, size_t number_of_pending_patches, size_t number_of_expired_pending_patches, size_t number_of_pending_operations, size_t number_of_expired_pending_operations)
{
    size_t i;

    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    for (i = 0; i < number_of_expired_pending_patches; i++)
    {
        STRICT_EXPECTED_CALL(get_difftime(current_time, IGNORED_NUM_ARG)).SetReturn(10000000); // Simulate it's expired for sure.
        STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    }

    if (number_of_pending_patches > number_of_expired_pending_patches)
    {
        STRICT_EXPECTED_CALL(get_difftime(current_time, IGNORED_NUM_ARG)).SetReturn(0); // Simulate it's not expired.
    }


    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    for (i = 0; i < number_of_expired_pending_operations; i++)
    {
        STRICT_EXPECTED_CALL(get_difftime(current_time, IGNORED_NUM_ARG)).SetReturn(10000000); // Simulate it's expired for sure.
        STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG)); // correlation id
        STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    }

    if (number_of_pending_operations > number_of_expired_pending_operations)
    {
        STRICT_EXPECTED_CALL(get_difftime(current_time, IGNORED_NUM_ARG)).SetReturn(0); // Simulate it's not expired.
    }


}

static void set_create_twin_operation_context_expected_calls()
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    set_generate_unique_id_expected_calls();
}

static void set_add_map_item_expected_calls(const char* name, const char* value)
{
    STRICT_EXPECTED_CALL(amqpvalue_create_symbol(name));
    
    if (value == NULL)
    {

        STRICT_EXPECTED_CALL(amqpvalue_create_null());
    }
    else
    {
        STRICT_EXPECTED_CALL(amqpvalue_create_string(value));
    }

    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
}

static void set_add_amqp_message_annotation_expected_calls()
{
    STRICT_EXPECTED_CALL(amqpvalue_create_message_annotations(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_set_message_annotations(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
}

static void set_set_message_correlation_id_expected_calls()
{
    STRICT_EXPECTED_CALL(message_get_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_create());
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_set_correlation_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_set_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_destroy(IGNORED_PTR_ARG));
}

static void set_create_amqp_message_for_twin_operation_expected_calls(TWIN_OPERATION_TYPE twin_op_type)
{
    STRICT_EXPECTED_CALL(message_create());
    STRICT_EXPECTED_CALL(amqpvalue_create_map());

    switch (twin_op_type)
    {
    case TWIN_OPERATION_TYPE_PATCH:
        set_add_map_item_expected_calls(TWIN_MESSAGE_PROPERTY_OPERATION, TWIN_OPERATION_PATCH);
        break;
    case TWIN_OPERATION_TYPE_GET:
        set_add_map_item_expected_calls(TWIN_MESSAGE_PROPERTY_OPERATION, TWIN_OPERATION_GET);
        break;
    case TWIN_OPERATION_TYPE_PUT:
        set_add_map_item_expected_calls(TWIN_MESSAGE_PROPERTY_OPERATION, TWIN_OPERATION_PUT);
        break;
    case TWIN_OPERATION_TYPE_DELETE:
        set_add_map_item_expected_calls(TWIN_MESSAGE_PROPERTY_OPERATION, TWIN_OPERATION_DELETE);
        break;
    default:
        break;
    };

    if (twin_op_type == TWIN_OPERATION_TYPE_PATCH)
    {
        set_add_map_item_expected_calls(TWIN_MESSAGE_PROPERTY_RESOURCE, TWIN_RESOURCE_REPORTED);
    }
    else if (twin_op_type == TWIN_OPERATION_TYPE_PUT || twin_op_type == TWIN_OPERATION_TYPE_DELETE)
    {
        set_add_map_item_expected_calls(TWIN_MESSAGE_PROPERTY_RESOURCE, TWIN_RESOURCE_DESIRED);
    }

    set_add_amqp_message_annotation_expected_calls();
    set_set_message_correlation_id_expected_calls();

    if (twin_op_type == TWIN_OPERATION_TYPE_PATCH)
    {
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_PTR_ARG)).SetReturn(&TEST_CONSTBUFFER);
    }

    BINARY_DATA data;
    data.bytes = NULL;
    data.length = 0;
    STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, data))
        .IgnoreArgument_amqp_data();
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
}

static void set_send_twin_operation_request_expected_calls(time_t current_time)
{
    set_create_amqp_message_for_twin_operation_expected_calls(TWIN_OPERATION_TYPE_PATCH);
    STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG)).SetReturn(current_time);
    STRICT_EXPECTED_CALL(amqp_messenger_send_async(TEST_AMQP_MESSENGER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
}

static void set_twin_messenger_do_work_expected_calls(DOWORK_TEST_PROFILE* dwtp)
{
    if (dwtp->current_state == TWIN_MESSENGER_STATE_STARTED)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        while (dwtp->number_of_pending_patches > 0)
        {
            set_create_twin_operation_context_expected_calls();

            STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

            set_send_twin_operation_request_expected_calls(dwtp->current_time);

            STRICT_EXPECTED_CALL(CONSTBUFFER_Destroy(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
            
            dwtp->number_of_pending_patches--;
            dwtp->number_of_pending_operations++;
        }
    }

    set_process_timeouts_expected_calls(dwtp->current_time, dwtp->number_of_pending_patches, dwtp->number_of_expired_pending_patches, dwtp->number_of_pending_operations, dwtp->number_of_expired_pending_operations);

    STRICT_EXPECTED_CALL(amqp_messenger_do_work(TEST_AMQP_MESSENGER_HANDLE));
}


// ---------- Consolidated Helpers ---------- //

static TWIN_MESSENGER_CONFIG g_twin_msgr_config;
static TWIN_MESSENGER_CONFIG* get_twin_messenger_config()
{
    g_twin_msgr_config.client_version = TEST_CLIENT_VERSION_STR;
    g_twin_msgr_config.device_id = TEST_DEVICE_ID;
    g_twin_msgr_config.iothub_host_fqdn = TEST_IOTHUB_HOST_FQDN;
    g_twin_msgr_config.on_state_changed_callback = TEST_on_state_changed_callback;
    g_twin_msgr_config.on_state_changed_context = TEST_ON_STATE_CHANGED_CB_CONTEXT;

    return &g_twin_msgr_config;
}

static TWIN_MESSENGER_HANDLE create_twin_messenger(TWIN_MESSENGER_CONFIG* config)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_twin_messenger_create(config);
    return twin_messenger_create(config);
}

static void send_one_report_patch(TWIN_MESSENGER_HANDLE handle, time_t current_time)
{
    const unsigned char* buffer = (unsigned char*)TWIN_REPORTED_PROPERTIES;
    size_t size = TWIN_REPORTED_PROPERTIES_LENGTH;
    CONSTBUFFER_HANDLE report = real_CONSTBUFFER_Create(buffer, size);

    umock_c_reset_all_calls();
    set_twin_messenger_report_state_async_expected_calls(report, current_time);
    (void)twin_messenger_report_state_async(handle, report, TEST_on_report_state_complete_callback, NULL);

    real_CONSTBUFFER_Destroy(report);
}

static void crank_twin_messenger_do_work(TWIN_MESSENGER_HANDLE handle, TWIN_MESSENGER_CONFIG* config, DOWORK_TEST_PROFILE* dwtp)
{
    (void)config;
    
    umock_c_reset_all_calls();
    set_twin_messenger_do_work_expected_calls(dwtp);
    twin_messenger_do_work(handle);

}

static TWIN_MESSENGER_HANDLE create_and_start_twin_messenger(TWIN_MESSENGER_CONFIG* config)
{
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    set_twin_messenger_start_expected_calls();
    (void)twin_messenger_start(handle, TEST_SESSION_HANDLE);

    TEST_amqp_messenger_create_config.on_state_changed_callback(
        TEST_amqp_messenger_create_config.on_state_changed_context, AMQP_MESSENGER_STATE_STOPPED, AMQP_MESSENGER_STATE_STARTING);

    DOWORK_TEST_PROFILE dwtp;
    reset_dowork_test_profile(&dwtp);
    crank_twin_messenger_do_work(handle, config, &dwtp);

    TEST_amqp_messenger_create_config.on_state_changed_callback(TEST_amqp_messenger_create_config.on_state_changed_context, AMQP_MESSENGER_STATE_STARTING, AMQP_MESSENGER_STATE_STARTED);
    TEST_amqp_messenger_create_config.on_subscription_changed_callback(TEST_amqp_messenger_create_config.on_subscription_changed_context, true);

    return handle;
}


// ---------- Mock Helpers ---------- //

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(amqp_messenger_create, TEST_amqp_messenger_create);
    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Clone, real_CONSTBUFFER_Clone);
    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Destroy, real_CONSTBUFFER_Destroy);
    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_GetContent, real_CONSTBUFFER_GetContent);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_create, real_singlylinkedlist_create);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_destroy, real_singlylinkedlist_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_add, real_singlylinkedlist_add);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_head_item, real_singlylinkedlist_get_head_item);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_remove, real_singlylinkedlist_remove);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_item_get_value, real_singlylinkedlist_item_get_value);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_next_item, real_singlylinkedlist_get_next_item);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_find, real_singlylinkedlist_find);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_foreach, real_singlylinkedlist_foreach);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_remove_if, real_singlylinkedlist_remove_if);
}

static void register_global_mock_aliases()
{
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(UNIQUEID_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(role, bool);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(message_annotations, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PROPERTIES_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BINARY_DATA, void*);
    REGISTER_UMOCK_ALIAS_TYPE(receiver_settle_mode, int);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_MATCH_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(pfCloneOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfDestroyOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfSetOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_FILTER_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_MESSENGER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_AMQP_MESSENGER_MESSAGE_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_MESSENGER_SEND_COMPLETE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_MESSENGER_SUBSCRIPTION_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_MESSENGER_STATE_CHANGED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_MATCH_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ACTION_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_CONDITION_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_HANDLE, void*);
}

static void register_global_mock_returns()
{
    // amqp_messenger
    REGISTER_GLOBAL_MOCK_RETURN(amqp_messenger_create, TEST_AMQP_MESSENGER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_messenger_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_messenger_subscribe_for_messages, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_messenger_subscribe_for_messages, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_messenger_start, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_messenger_start, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_messenger_stop, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_messenger_stop, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_messenger_retrieve_options, TEST_OPTIONHANDLER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_messenger_retrieve_options, NULL);

    // amqpvalue
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_map, TEST_MAP_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_map, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_null, TEST_NULL_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_null, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_string, TEST_STRING_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_symbol, TEST_SYMBOL_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_symbol, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_message_annotations, TEST_MSG_ANNOTATIONS_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_message_annotations, NULL);

    // CRT
    REGISTER_GLOBAL_MOCK_RETURN(mallocAndStrcpy_s, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 1);

    // Map
    REGISTER_GLOBAL_MOCK_RETURN(Map_Create, TEST_ATTACH_PROPERTIES);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(Map_Add, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Add, MAP_ERROR);

    // message
    REGISTER_GLOBAL_MOCK_RETURN(message_create, TEST_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(message_set_message_annotations, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_message_annotations, 1);

    // singlylinkedlist
    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_remove, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_remove, 555);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_add, TEST_LIST_ITEM_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);

    // STRING
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_IOTHUB_HOST_FQDN);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);

    // OptionHandler
    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_Create, TEST_OPTIONHANDLER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_AddOption, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_AddOption, OPTIONHANDLER_ERROR);

    // properties
    REGISTER_GLOBAL_MOCK_RETURN(properties_create, TEST_PROPERTIES_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_create, NULL);

    // UniqueId
    REGISTER_GLOBAL_MOCK_RETURN(UniqueId_Generate, UNIQUEID_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UniqueId_Generate, UNIQUEID_ERROR);

    // AgentTime
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(get_time, INDEFINITE_TIME);
}

static void reset_test_data()
{
    g_STRING_sprintf_call_count = 0;
    g_STRING_sprintf_fail_on_count = -1;
    saved_STRING_sprintf_handle = NULL;

    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));

    memset(&TEST_amqp_messenger_create_config, 0, sizeof(TEST_amqp_messenger_create_config));
    TEST_amqp_messenger_create_return = TEST_AMQP_MESSENGER_HANDLE;

    TEST_on_report_state_complete_callback_result = TWIN_REPORT_STATE_RESULT_SUCCESS;
    TEST_on_report_state_complete_callback_reason = TWIN_REPORT_STATE_REASON_NONE;
    TEST_on_report_state_complete_callback_status_code = 0;
    TEST_on_report_state_complete_callback_context = NULL;

    TEST_on_report_state_complete_callback_reason_NONE_count= 0;
    TEST_on_report_state_complete_callback_reason_INTERNAL_ERROR_count= 0;
    TEST_on_report_state_complete_callback_reason_TIMEOUT_count= 0;
    TEST_on_report_state_complete_callback_reason_FAIL_SENDING_count= 0;
    TEST_on_report_state_complete_callback_reason_INVALID_RESPONSE_count= 0;
    TEST_on_report_state_complete_callback_reason_MESSENGER_DESTROYED_count= 0;
    TEST_on_report_state_complete_callback_result_SUCCESS_count= 0;
    TEST_on_report_state_complete_callback_result_ERROR_count= 0;
    TEST_on_report_state_complete_callback_result_CANCELLED_count= 0;

    TEST_CONSTBUFFER.buffer = TWIN_REPORTED_PROPERTIES;
    TEST_CONSTBUFFER.size = TWIN_REPORTED_PROPERTIES_LENGTH;

    TEST_on_state_changed_callback_context = NULL;
}

BEGIN_TEST_SUITE(iothubtr_amqp_twin_msgr_ut)

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

    register_global_mock_aliases();
    register_global_mock_hooks();
    register_global_mock_returns();

    g_initial_time = time(NULL);
    g_initial_time_plus_30_secs = add_seconds(g_initial_time, 30);
    g_initial_time_plus_60_secs = add_seconds(g_initial_time, 60);
    g_initial_time_plus_90_secs = add_seconds(g_initial_time, 90);
    g_initial_time_plus_300_secs = add_seconds(g_initial_time, 300);
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

    reset_test_data();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_001: [If parameter `messenger_config` is NULL, twin_messenger_create() shall return NULL]
TEST_FUNCTION(twin_msgr_create_NULL_configuration)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    TWIN_MESSENGER_HANDLE handle = twin_messenger_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(handle);

    // cleanup
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_002: [If `messenger_config`'s `device_id`, `iothub_host_fqdn` or `client_version` is NULL, twin_messenger_create() shall return NULL]  
TEST_FUNCTION(twin_msgr_create_NULL_device_id)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    config->device_id = NULL;

    umock_c_reset_all_calls();

    // act
    TWIN_MESSENGER_HANDLE handle = twin_messenger_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(handle);

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_003: [twin_messenger_create() shall allocate memory for the messenger instance structure (aka `twin_msgr`)]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_005: [twin_messenger_create() shall save a copy of `messenger_config` info into `twin_msgr`]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_007: [`twin_msgr->pending_patches` shall be set using singlylinkedlist_create()]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_009: [`twin_msgr->operations` shall be set using singlylinkedlist_create()]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_011: [`twin_msgr->amqp_msgr` shall be set using amqp_messenger_create(), passing a AMQP_MESSENGER_CONFIG instance `amqp_msgr_config`]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_012: [`amqp_msgr_config->client_version` shall be set with `twin_msgr->client_version`]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_013: [`amqp_msgr_config->device_id` shall be set with `twin_msgr->device_id`]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_014: [`amqp_msgr_config->iothub_host_fqdn` shall be set with `twin_msgr->iothub_host_fqdn`]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_015: [`amqp_msgr_config` shall have "twin/" as send link target suffix and receive link source suffix]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_016: [`amqp_msgr_config` shall have send and receive link attach properties set as "com.microsoft:client-version" = `twin_msgr->client_version`, "com.microsoft:channel-correlation-id" = `twin:<UUID>`, "com.microsoft:api-version" = "2016-11-14"]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_017: [`amqp_msgr_config` shall be set with `on_amqp_messenger_state_changed_callback` and `on_amqp_messenger_subscription_changed_callback` callbacks]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_019: [`twin_msgr->amqp_msgr` shall subscribe for AMQP messages by calling amqp_messenger_subscribe_for_messages() passing `on_amqp_message_received`]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_021: [If no failures occurr, twin_messenger_create() shall return a handle to `twin_msgr`] 
TEST_FUNCTION(twin_msgr_create_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_twin_messenger_create(config);

    // act
    TWIN_MESSENGER_HANDLE handle = twin_messenger_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, TEST_DEVICE_ID, TEST_amqp_messenger_create_config.device_id);
    ASSERT_ARE_EQUAL(char_ptr, TEST_IOTHUB_HOST_FQDN, TEST_amqp_messenger_create_config.iothub_host_fqdn);
    ASSERT_ARE_EQUAL(char_ptr, TEST_CLIENT_VERSION_STR, TEST_amqp_messenger_create_config.client_version);
    ASSERT_ARE_EQUAL(void_ptr, (void*)TEST_ATTACH_PROPERTIES, (void*)TEST_amqp_messenger_create_config.send_link.attach_properties);
    ASSERT_ARE_EQUAL(void_ptr, (void*)TEST_ATTACH_PROPERTIES, (void*)TEST_amqp_messenger_create_config.receive_link.attach_properties);
    ASSERT_ARE_EQUAL(char_ptr, DEFAULT_TWIN_SEND_LINK_SOURCE_NAME, (void*)TEST_amqp_messenger_create_config.send_link.target_suffix);
    ASSERT_ARE_EQUAL(char_ptr, DEFAULT_TWIN_RECEIVE_LINK_TARGET_NAME, (void*)TEST_amqp_messenger_create_config.receive_link.source_suffix);
    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_create_config.on_state_changed_callback);
    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_create_config.on_subscription_changed_callback);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_004: [If malloc() fails, twin_messenger_create() shall fail and return NULL]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_006: [If any `messenger_config` info fails to be copied, twin_messenger_create() shall fail and return NULL]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_008: [If singlylinkedlist_create() fails, twin_messenger_create() shall fail and return NULL]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_010: [If singlylinkedlist_create() fails, twin_messenger_create() shall fail and return NULL]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_018: [If amqp_messenger_create() fails, twin_messenger_create() shall fail and return NULL]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_020: [If amqp_messenger_subscribe_for_messages() fails, twin_messenger_create() shall fail and return NULL] 
TEST_FUNCTION(twin_msgr_create_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_twin_messenger_create(config);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 10 || i == 14 || i == 17)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        TWIN_MESSENGER_HANDLE handle = twin_messenger_create(config);

        // assert
        sprintf(error_msg, "On failed call %zu", i);
        ASSERT_IS_NULL_WITH_MSG(handle, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_022: [If `twin_msgr_handle` or `data` are NULL, twin_messenger_report_state_async() shall fail and return a non-zero value]  
TEST_FUNCTION(twin_msgr_report_state_async_NULL_handle)
{
    // arrange
    const unsigned char* buffer = (unsigned char*)TWIN_REPORTED_PROPERTIES;
    size_t size = TWIN_REPORTED_PROPERTIES_LENGTH;
    CONSTBUFFER_HANDLE report = real_CONSTBUFFER_Create(buffer, size);

    umock_c_reset_all_calls();

    // act
    int result = twin_messenger_report_state_async(NULL, report, TEST_on_report_state_complete_callback, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    real_CONSTBUFFER_Destroy(report);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_022: [If `twin_msgr_handle` or `data` are NULL, twin_messenger_report_state_async() shall fail and return a non-zero value]  
TEST_FUNCTION(twin_msgr_report_state_async_NULL_data)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    umock_c_reset_all_calls();

    // act
    int result = twin_messenger_report_state_async(handle, NULL, TEST_on_report_state_complete_callback, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_023: [twin_messenger_report_state_async() shall allocate memory for a TWIN_PATCH_OPERATION_CONTEXT structure (aka `twin_op_ctx`)]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_025: [`twin_op_ctx` shall have a copy of `data`]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_027: [`twin_op_ctx->time_enqueued` shall be set using get_time]    
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_029: [`twin_op_ctx` shall be added to `twin_msgr->pending_patches` using singlylinkedlist_add()]    
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_032: [If no failures occur, twin_messenger_report_state_async() shall return zero]  
TEST_FUNCTION(twin_msgr_report_state_async_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    const unsigned char* buffer = (unsigned char*)TWIN_REPORTED_PROPERTIES;
    size_t size = TWIN_REPORTED_PROPERTIES_LENGTH;
    CONSTBUFFER_HANDLE report = real_CONSTBUFFER_Create(buffer, size);

    umock_c_reset_all_calls();
    set_twin_messenger_report_state_async_expected_calls(report, g_initial_time);

    // act
    int result = twin_messenger_report_state_async(handle, report, TEST_on_report_state_complete_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    real_CONSTBUFFER_Destroy(report);
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_024: [If malloc() fails, twin_messenger_report_state_async() shall fail and return a non-zero value]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_026: [If `data` fails to be copied, twin_messenger_report_state_async() shall fail and return a non-zero value]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_028: [If `twin_op_ctx->time_enqueued` fails to be set, twin_messenger_report_state_async() shall fail and return a non-zero value]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_030: [If singlylinkedlist_add() fails, twin_messenger_report_state_async() shall fail and return a non-zero value]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_031: [If any failure occurs, twin_messenger_report_state_async() shall free any memory it has allocated]
TEST_FUNCTION(twin_msgr_report_state_async_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    const unsigned char* buffer = (unsigned char*)TWIN_REPORTED_PROPERTIES;
    size_t size = TWIN_REPORTED_PROPERTIES_LENGTH;
    CONSTBUFFER_HANDLE report = real_CONSTBUFFER_Create(buffer, size);

    umock_c_reset_all_calls();
    set_twin_messenger_report_state_async_expected_calls(report, g_initial_time);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        int result = twin_messenger_report_state_async(handle, report, TEST_on_report_state_complete_callback, NULL);

        // assert
        sprintf(error_msg, "On failed call %zu", i);
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, error_msg);
    }

    // cleanup
    real_CONSTBUFFER_Destroy(report);
    twin_messenger_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_033: [If `twin_msgr_handle` or `send_status` are NULL, twin_messenger_get_send_status() shall fail and return a non-zero value] 
TEST_FUNCTION(twin_msgr_get_send_status_NULL_handle)
{
    // arrange
    TWIN_MESSENGER_SEND_STATUS send_status;

    umock_c_reset_all_calls();

    // act
    int result = twin_messenger_get_send_status(NULL, &send_status);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_033: [If `twin_msgr_handle` or `send_status` are NULL, twin_messenger_get_send_status() shall fail and return a non-zero value] 
TEST_FUNCTION(twin_msgr_get_send_status_NULL_send_status)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    umock_c_reset_all_calls();

    // act
    int result = twin_messenger_get_send_status(handle, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_035: [Otherwise, send_status shall be set to TWIN_MESSENGER_SEND_STATUS_IDLE] 
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_036: [If no failures occur, twin_messenger_get_send_status() shall return 0] 
TEST_FUNCTION(twin_msgr_get_send_status_IDLE_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);
    TWIN_MESSENGER_SEND_STATUS send_status;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    int result = twin_messenger_get_send_status(handle, &send_status);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_SEND_STATUS_IDLE, send_status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_034: [If `twin_msgr->pending_patches` or `twin_msgr->operations` have any TWIN patch requests, send_status shall be set to TWIN_MESSENGER_SEND_STATUS_BUSY] 
TEST_FUNCTION(twin_msgr_get_send_status_BUSY_pending_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);
    TWIN_MESSENGER_SEND_STATUS send_status;

    send_one_report_patch(handle, g_initial_time);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    // act
    int result = twin_messenger_get_send_status(handle, &send_status);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_SEND_STATUS_BUSY, send_status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_034: [If `twin_msgr->pending_patches` or `twin_msgr->operations` have any TWIN patch requests, send_status shall be set to TWIN_MESSENGER_SEND_STATUS_BUSY] 
TEST_FUNCTION(twin_msgr_get_send_status_BUSY_in_progress_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_and_start_twin_messenger(config);
    TWIN_MESSENGER_SEND_STATUS send_status;

    send_one_report_patch(handle, g_initial_time);

    DOWORK_TEST_PROFILE dwtp;
    reset_dowork_test_profile(&dwtp);
    dwtp.number_of_pending_patches = 1;

    crank_twin_messenger_do_work(handle, config, &dwtp);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

    // act
    int result = twin_messenger_get_send_status(handle, &send_status);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_SEND_STATUS_BUSY, send_status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_045: [If `twin_msgr_handle` or `session_handle` are NULL, twin_messenger_start() shall fail and return a non-zero value]
TEST_FUNCTION(twin_msgr_start_NULL_msgr_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = twin_messenger_start(NULL, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_045: [If `twin_msgr_handle` or `session_handle` are NULL, twin_messenger_start() shall fail and return a non-zero value]
TEST_FUNCTION(twin_msgr_start_NULL_session_handle)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    umock_c_reset_all_calls();

    // act
    int result = twin_messenger_start(handle, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_046: [amqp_messenger_start() shall be invoked passing `twin_msgr->amqp_msgr` and `session_handle`]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_048: [If no failures occurr, `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_STARTING, and `twin_msgr->on_state_changed_callback` invoked if provided]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_050: [If no failures occurr, twin_messenger_start() shall return 0]
TEST_FUNCTION(twin_msgr_start_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    umock_c_reset_all_calls();
    set_twin_messenger_start_expected_calls();

    // act
    int result = twin_messenger_start(handle, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_STATE_STOPPED, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_STATE_STARTING, TEST_on_state_changed_callback_new_state);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_047: [If amqp_messenger_start() fails, twin_messenger_start() fail and return a non-zero value]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_049: [If any failures occurr, `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_ERROR, and `twin_msgr->on_state_changed_callback` invoked if provided]
TEST_FUNCTION(twin_msgr_start_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    umock_c_reset_all_calls();
    set_twin_messenger_start_expected_calls();
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    // act
    int result = twin_messenger_start(handle, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_STATE_STARTING, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_STATE_ERROR, TEST_on_state_changed_callback_new_state);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_051: [If `twin_msgr_handle` is NULL, twin_messenger_stop() shall fail and return a non-zero value]  
TEST_FUNCTION(twin_msgr_stop_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = twin_messenger_stop(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_052: [amqp_messenger_stop() shall be invoked passing `twin_msgr->amqp_msgr`]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_054: [`twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_STOPPING, and `twin_msgr->on_state_changed_callback` invoked if provided]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_056: [If no failures occurr, twin_messenger_stop() shall return 0]
TEST_FUNCTION(twin_msgr_stop_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);
    (void)twin_messenger_start(handle, TEST_SESSION_HANDLE);

    umock_c_reset_all_calls();
    set_twin_messenger_stop_expected_calls();

    // act
    int result = twin_messenger_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_STATE_STARTING, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_STATE_STOPPING, TEST_on_state_changed_callback_new_state);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_053: [If amqp_messenger_stop() fails, twin_messenger_stop() fail and return a non-zero value]
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_055: [If any failures occurr, `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_ERROR, and `twin_msgr->on_state_changed_callback` invoked if provided]
TEST_FUNCTION(twin_msgr_stop_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);
    (void)twin_messenger_start(handle, TEST_SESSION_HANDLE);

    umock_c_reset_all_calls();
    set_twin_messenger_stop_expected_calls();
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    // act
    int result = twin_messenger_stop(handle);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_STATE_STOPPING, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, TWIN_MESSENGER_STATE_ERROR, TEST_on_state_changed_callback_new_state);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_107: [If `twin_msgr_handle` is NULL, twin_messenger_retrieve_options shall fail and return NULL]
TEST_FUNCTION(twin_msgr_retrieve_options_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    OPTIONHANDLER_HANDLE result = twin_messenger_retrieve_options(NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_108: [twin_messenger_retrieve_options() shall return the result of amqp_messenger_retrieve_options()]
TEST_FUNCTION(twin_msgr_retrieve_options_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    umock_c_reset_all_calls();
    set_twin_messenger_retrieve_options_expected_calls();

    // act
    OPTIONHANDLER_HANDLE result = twin_messenger_retrieve_options(handle);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_OPTIONHANDLER_HANDLE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_083: [twin_messenger_do_work() shall invoke amqp_messenger_do_work() passing `twin_msgr->amqp_msgr`]  
TEST_FUNCTION(twin_msgr_do_work_not_started_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    DOWORK_TEST_PROFILE dwtp;
    reset_dowork_test_profile(&dwtp);

    umock_c_reset_all_calls();
    set_twin_messenger_do_work_expected_calls(&dwtp);

    // act
    twin_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_080: [twin_messenger_do_work() shall remove and destroy any timed out items from `twin_msgr->pending_patches` and `twin_msgr->operations`]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_081: [If a timed-out item is a reported property PATCH, `on_report_state_complete_callback` shall be invoked with RESULT_ERROR and REASON_TIMEOUT]
TEST_FUNCTION(twin_msgr_do_work_not_started_with_EXPIRED_pending_patches_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_twin_messenger(config);

    send_one_report_patch(handle, g_initial_time);
    send_one_report_patch(handle, g_initial_time);

    DOWORK_TEST_PROFILE dwtp;
    reset_dowork_test_profile(&dwtp);
    dwtp.number_of_pending_patches = 2;
    dwtp.number_of_expired_pending_patches = 2;

    umock_c_reset_all_calls();
    set_twin_messenger_do_work_expected_calls(&dwtp);

    // act
    twin_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_result_SUCCESS_count);
    ASSERT_ARE_EQUAL(size_t, 2, TEST_on_report_state_complete_callback_result_ERROR_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_result_CANCELLED_count);

    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_NONE_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_INTERNAL_ERROR_count);
    ASSERT_ARE_EQUAL(size_t, 2, TEST_on_report_state_complete_callback_reason_TIMEOUT_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_FAIL_SENDING_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_INVALID_RESPONSE_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_MESSENGER_DESTROYED_count);

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_080: [twin_messenger_do_work() shall remove and destroy any timed out items from `twin_msgr->pending_patches` and `twin_msgr->operations`]  
// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_081: [If a timed-out item is a reported property PATCH, `on_report_state_complete_callback` shall be invoked with RESULT_ERROR and REASON_TIMEOUT]  
TEST_FUNCTION(twin_msgr_do_work_started_with_EXPIRED_in_progress_patches_success)
{
    // arrange
    TWIN_MESSENGER_CONFIG* config = get_twin_messenger_config();
    TWIN_MESSENGER_HANDLE handle = create_and_start_twin_messenger(config);

    send_one_report_patch(handle, g_initial_time);

    DOWORK_TEST_PROFILE dwtp;
    reset_dowork_test_profile(&dwtp);
    dwtp.current_state = TWIN_MESSENGER_STATE_STARTED;
    dwtp.number_of_pending_patches = 1;

    crank_twin_messenger_do_work(handle, config, &dwtp);

    umock_c_reset_all_calls();
    dwtp.current_time = g_initial_time_plus_300_secs;
    dwtp.number_of_pending_patches = 0;
    dwtp.number_of_pending_operations = 1;
    dwtp.number_of_expired_pending_operations = 1;
    set_twin_messenger_do_work_expected_calls(&dwtp);

    // act
    twin_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_result_SUCCESS_count);
    ASSERT_ARE_EQUAL(size_t, 1, TEST_on_report_state_complete_callback_result_ERROR_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_result_CANCELLED_count);

    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_NONE_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_INTERNAL_ERROR_count);
    ASSERT_ARE_EQUAL(size_t, 1, TEST_on_report_state_complete_callback_reason_TIMEOUT_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_FAIL_SENDING_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_INVALID_RESPONSE_count);
    ASSERT_ARE_EQUAL(size_t, 0, TEST_on_report_state_complete_callback_reason_MESSENGER_DESTROYED_count);

    // cleanup
    twin_messenger_destroy(handle);
}

// Tests_IOTHUBTRANSPORT_AMQP_TWIN_MESSENGER_09_082: [If any failure occurs while verifying/removing timed-out items `twin_msgr->state` shall be set to TWIN_MESSENGER_STATE_ERROR and user informed]  


END_TEST_SUITE(iothubtr_amqp_twin_msgr_ut)
