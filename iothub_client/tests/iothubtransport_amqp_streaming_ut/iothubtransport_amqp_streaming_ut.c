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
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"
#include "iothub_client_options.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h" 
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/uuid.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_uamqp_c/amqp_definitions_fields.h"
#include "azure_uamqp_c/messaging.h"
#include "internal/iothub_client_private.h"
#include "internal/iothubtransport_amqp_messenger.h"
#include "iothub_client_streaming.h"

#undef ENABLE_MOCKS

#include "internal/iothubtransport_amqp_streaming.h"

MU_DEFINE_ENUM_STRINGS(AMQP_MESSENGER_SEND_STATUS, AMQP_MESSENGER_SEND_STATUS_VALUES);
MU_DEFINE_ENUM_STRINGS(AMQP_MESSENGER_SEND_RESULT, AMQP_MESSENGER_SEND_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(AMQP_MESSENGER_REASON, AMQP_MESSENGER_REASON_VALUES);
MU_DEFINE_ENUM_STRINGS(AMQP_MESSENGER_DISPOSITION_RESULT, AMQP_MESSENGER_DISPOSITION_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(AMQP_MESSENGER_STATE, AMQP_MESSENGER_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS(AMQP_STREAMING_CLIENT_STATE, AMQP_STREAMING_CLIENT_STATE_VALUES);


static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

#define UNIQUE_ID_BUFFER_SIZE                                37
static const char* TEST_UNIQUE_ID =                          "a123123-13123-1231";

#define TEST_DEVICE_ID                                       "my_device"
#define TEST_DEVICE_ID_STRING_HANDLE                         (STRING_HANDLE)0x4442
#define TEST_IOTHUB_HOST_FQDN_STRING_HANDLE                  (STRING_HANDLE)0x4443
#define TEST_IOTHUB_HOST_FQDN                                "some.fqdn.com"
static const char* TEST_CLIENT_VERSION_STR = "client x (version y)";
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
#define TEST_TICK_COUNTER_HANDLE                             (TICK_COUNTER_HANDLE)0x4493
#define TEST_UUID_AMQP_VALUE                                 (AMQP_VALUE)0x4494
#define TEST_INT_AMQP_VALUE                                  (AMQP_VALUE)0x4495
#define TEST_AMQP_VALUE                                      (AMQP_VALUE)0x4496

#define INDEFINITE_TIME                                      ((time_t)-1)
#define DEFAULT_STREAMS_SEND_LINK_SOURCE_NAME                "streams"
#define DEFAULT_STREAMS_RECEIVE_LINK_TARGET_NAME             "streams"

#define CLIENT_VERSION_PROPERTY_NAME                         "com.microsoft:client-version"
#define STREAM_CORRELATION_ID_PROPERTY_NAME                  "com.microsoft:channel-correlation-id"
#define STREAM_API_VERSION_PROPERTY_NAME                     "com.microsoft:api-version"
#define STREAM_API_VERSION_NUMBER                            "2016-11-14"

#define TEST_ATTACH_PROPERTIES                               (MAP_HANDLE)0x4444
#define UNIQUE_ID_BUFFER_SIZE                                37

static tickcounter_ms_t initial_time =                       0;

static const char* TEST_STREAM_NAME =                        "TestStream";
static const char* TEST_STREAM_URL =                         "wss://someserver.streaming.private.azure-devices-int.net:443/bridges/6c245132826cf2fa09694bc6b93f3548";
static const char* TEST_AUTHORIZATION_TOKEN =                "6c245132826cf2fa09694bc6b93f3548";
static const char* TEST_CUSTOM_DATA =                        "Some custom data";
static const char* TEST_CONTENT_TYPE =                       "text/plain";
static const char* TEST_CONTENT_ENCODING =                   "some encoding";

static const char* STREAM_PROP_NAME = "IoThub-streaming-name";
static const char* STREAM_PROP_HOSTNAME = "IoThub-streaming-hostname";
static const char* STREAM_PROP_PORT = "IoThub-streaming-port";
static const char* STREAM_PROP_URL = "IoThub-streaming-url";
static const char* STREAM_PROP_AUTH_TOKEN = "IoThub-streaming-auth-token";
static const char* STREAM_PROP_IS_ACCEPTED = "IoThub-streaming-is-accepted";


static CONSTBUFFER TEST_CONSTBUFFER;

typedef struct DOWORK_TEST_PROFILE_STRUCT
{
    AMQP_MESSENGER_STATE previous_state;
    AMQP_MESSENGER_STATE new_state;
    bool set_amqp_msgr_state;
    bool set_amqp_msgr_subscription_state;
    bool is_amqp_msgr_subscribed;
} DOWORK_TEST_PROFILE;


// ---------- General Helpers ---------- //

#ifdef __cplusplus
extern "C"
{
#endif

static int TEST_link_set_max_message_size_result;
static int TEST_amqpvalue_set_map_value_result;
static int TEST_link_set_attach_properties_result;

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
    TEST_amqp_messenger_create_config.prod_info_cb = messenger_config->prod_info_cb;
    TEST_amqp_messenger_create_config.prod_info_ctx = messenger_config->prod_info_ctx;

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

static AMQP_MESSENGER_HANDLE TEST_amqp_messenger_send_async_saved_messenger_handle;
static MESSAGE_HANDLE TEST_amqp_messenger_send_async_saved_message;
static AMQP_MESSENGER_SEND_COMPLETE_CALLBACK TEST_amqp_messenger_send_async_saved_on_user_defined_send_complete_callback;
static void* TEST_amqp_messenger_send_async_saved_user_context;
static int TEST_amqp_messenger_send_async_return;
static int TEST_amqp_messenger_send_async(AMQP_MESSENGER_HANDLE messenger_handle, MESSAGE_HANDLE message, AMQP_MESSENGER_SEND_COMPLETE_CALLBACK on_user_defined_send_complete_callback, void* user_context)
{
    TEST_amqp_messenger_send_async_saved_messenger_handle = messenger_handle;
    TEST_amqp_messenger_send_async_saved_message = message;
    TEST_amqp_messenger_send_async_saved_on_user_defined_send_complete_callback = on_user_defined_send_complete_callback;
    TEST_amqp_messenger_send_async_saved_user_context = user_context;

    return TEST_amqp_messenger_send_async_return;
}

static AMQP_MESSENGER_HANDLE TEST_amqp_messenger_subscribe_for_messages_saved_messenger_handle;
static ON_AMQP_MESSENGER_MESSAGE_RECEIVED TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback;
static void* TEST_amqp_messenger_subscribe_for_messages_saved_context;
static int TEST_amqp_messenger_subscribe_for_messages_result;
static int TEST_amqp_messenger_subscribe_for_messages(AMQP_MESSENGER_HANDLE messenger_handle, ON_AMQP_MESSENGER_MESSAGE_RECEIVED on_message_received_callback, void* context)
{
    TEST_amqp_messenger_subscribe_for_messages_saved_messenger_handle = messenger_handle;
    TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback = on_message_received_callback;
    TEST_amqp_messenger_subscribe_for_messages_saved_context = context;

    return TEST_amqp_messenger_subscribe_for_messages_result;
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

#ifdef __cplusplus
}
#endif


static void real_stream_c2d_request_destroy(DEVICE_STREAM_C2D_REQUEST* request)
{
    // Not destroying the inner fiels because they are mocked.
    real_free(request);
}


// ---------- Callbacks ---------- //

static const void* TEST_on_state_changed_callback_context;
static AMQP_STREAMING_CLIENT_STATE TEST_on_state_changed_callback_previous_state;
static AMQP_STREAMING_CLIENT_STATE TEST_on_state_changed_callback_new_state;
static void TEST_on_state_changed_callback(const void* context, AMQP_STREAMING_CLIENT_STATE previous_state, AMQP_STREAMING_CLIENT_STATE new_state)
{
    TEST_on_state_changed_callback_context = context;
    TEST_on_state_changed_callback_previous_state = previous_state;
    TEST_on_state_changed_callback_new_state = new_state;
}


// ---------- Expected Calls ---------- //

static void set_generate_unique_id_expected_calls()
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, UNIQUE_ID_BUFFER_SIZE))
        .CopyOutArgumentBuffer_uid(TEST_UNIQUE_ID, strlen(TEST_UNIQUE_ID) + 1);
}

static void set_generate_correlation_id_expected_calls()
{
    set_generate_unique_id_expected_calls();
    //STRICT_EXPECTED_CALL(strlen(IGNORED_NUM_ARG));
    //STRICT_EXPECTED_CALL(strlen(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    // sprintf
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG)); // 8
}

static void set_create_link_attach_properties_expected_calls(AMQP_STREAMING_CLIENT_CONFIG* config)
{
    (void)config;
    set_generate_correlation_id_expected_calls();
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(TEST_ATTACH_PROPERTIES);
    STRICT_EXPECTED_CALL(Map_Add(TEST_ATTACH_PROPERTIES, STREAM_CORRELATION_ID_PROPERTY_NAME, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Add(TEST_ATTACH_PROPERTIES, CLIENT_VERSION_PROPERTY_NAME, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Add(TEST_ATTACH_PROPERTIES, STREAM_API_VERSION_PROPERTY_NAME, STREAM_API_VERSION_NUMBER));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG)); // 13
}

static void set_destroy_link_attach_properties_expected_calls()
{
    STRICT_EXPECTED_CALL(Map_Destroy(TEST_ATTACH_PROPERTIES));
}

static void set_expected_calls_for_amqp_streaming_client_create(AMQP_STREAMING_CLIENT_CONFIG* config)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->device_id))
        .CopyOutArgumentBuffer(1, &config->device_id, sizeof(config->device_id));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->iothub_host_fqdn))
        .CopyOutArgumentBuffer(1, &config->iothub_host_fqdn, sizeof(config->iothub_host_fqdn));

    set_create_link_attach_properties_expected_calls(config);
    
    STRICT_EXPECTED_CALL(amqp_messenger_create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqp_messenger_subscribe_for_messages(TEST_AMQP_MESSENGER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    set_destroy_link_attach_properties_expected_calls();
}

static void set_twin_messenger_report_state_async_expected_calls(CONSTBUFFER_HANDLE report, time_t current_time)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_IncRef(report));
    STRICT_EXPECTED_CALL(get_time(NULL))
        .SetReturn(current_time);
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void set_expected_calls_for_amqp_streaming_client_start()
{
    STRICT_EXPECTED_CALL(amqp_messenger_start(IGNORED_PTR_ARG, TEST_SESSION_HANDLE));
}

static void set_expected_calls_for_amqp_streaming_client_stop()
{
    STRICT_EXPECTED_CALL(amqp_messenger_stop(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_amqp_streaming_client_retrieve_options()
{
    STRICT_EXPECTED_CALL(amqp_messenger_retrieve_options(TEST_AMQP_MESSENGER_HANDLE));
}

static void set_expected_calls_for_create_amqp_message_from_stream_d2c_request()
{
    STRICT_EXPECTED_CALL(message_create());
    STRICT_EXPECTED_CALL(message_get_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_create());

    STRICT_EXPECTED_CALL(UUID_from_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_create_uuid(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_set_correlation_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG)); // 6

    STRICT_EXPECTED_CALL(message_set_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_destroy(IGNORED_PTR_ARG)); // 8

    STRICT_EXPECTED_CALL(amqpvalue_create_map());

    // Stream name
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG)); // 13
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG)); // 14

                                                              // Hostname
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG)); // 18
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG)); // 19

                                                              // Port
    STRICT_EXPECTED_CALL(amqpvalue_create_int(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG)); // 23
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG)); // 24

    STRICT_EXPECTED_CALL(message_set_application_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    BINARY_DATA data;
    data.bytes = NULL;
    data.length = 0;
    STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, data))
        .IgnoreArgument_amqp_data();

    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG)); // 27
}

static void set_expected_calls_for_amqp_streaming_client_send_async()
{
    set_generate_unique_id_expected_calls();
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_UNIQUE_ID, sizeof(TEST_UNIQUE_ID));
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_TICK_COUNTER_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_current_ms(&initial_time, sizeof(initial_time));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    set_expected_calls_for_create_amqp_message_from_stream_d2c_request();
    STRICT_EXPECTED_CALL(amqp_messenger_send_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_process_timeouts(time_t current_time, size_t number_of_pending_patches, size_t number_of_expired_pending_patches, size_t number_of_pending_operations, size_t number_of_expired_pending_operations)
{
    size_t i;

    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    for (i = 0; i < number_of_expired_pending_patches; i++)
    {
        STRICT_EXPECTED_CALL(get_difftime(current_time, IGNORED_NUM_ARG)).SetReturn(10000000); // Simulate it's expired for sure.
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_PTR_ARG));
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

static void set_expected_calls_for_parse_amqp_message()
{
    // parse_message_properties()
    PROPERTIES_HANDLE properties = TEST_PROPERTIES_HANDLE;
    AMQP_VALUE uuid_value = TEST_UUID_AMQP_VALUE;
    AMQP_VALUE app_properties = TEST_AMQP_VALUE;
    uint32_t property_count = 3;
    AMQP_VALUE key_name = TEST_STRING_AMQP_VALUE;
    AMQP_VALUE key_value = TEST_STRING_AMQP_VALUE;

    BINARY_DATA body_content;
    body_content.bytes = (const unsigned char*)"ABCD EFGH";
    body_content.length = 9;

    STRICT_EXPECTED_CALL(message_get_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_properties(&properties, sizeof(properties));
    STRICT_EXPECTED_CALL(properties_get_correlation_id(properties, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_correlation_id_value(&uuid_value, sizeof(uuid_value));
    STRICT_EXPECTED_CALL(amqpvalue_get_type(IGNORED_PTR_ARG))
        .SetReturn(AMQP_TYPE_UUID);
    STRICT_EXPECTED_CALL(amqpvalue_get_uuid(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(UUID_to_string(IGNORED_PTR_ARG))
        .SetReturn((char*)TEST_UNIQUE_ID);


    STRICT_EXPECTED_CALL(properties_destroy(properties));

    // parse_message_application_properties()
    STRICT_EXPECTED_CALL(message_get_application_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_application_properties(&app_properties, sizeof(app_properties));
    STRICT_EXPECTED_CALL(amqpvalue_get_inplace_described_value(IGNORED_PTR_ARG))
        .SetReturn(app_properties);
    STRICT_EXPECTED_CALL(amqpvalue_get_map_pair_count(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_pair_count(&property_count, sizeof(property_count));

    // Loop
    STRICT_EXPECTED_CALL(amqpvalue_get_map_key_value_pair(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_key(&key_name, sizeof(key_name))
        .CopyOutArgumentBuffer_value(&key_value, sizeof(key_value));
    STRICT_EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &STREAM_PROP_NAME, sizeof(STREAM_PROP_NAME));
    STRICT_EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &TEST_STREAM_NAME, sizeof(TEST_STREAM_NAME));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(1, &TEST_STREAM_NAME, sizeof(TEST_STREAM_NAME));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(amqpvalue_get_map_key_value_pair(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_key(&key_name, sizeof(key_name))
        .CopyOutArgumentBuffer_value(&key_value, sizeof(key_value));
    STRICT_EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &STREAM_PROP_URL, sizeof(STREAM_PROP_URL));
    STRICT_EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &TEST_STREAM_URL, sizeof(TEST_STREAM_URL));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(1, &TEST_STREAM_URL, sizeof(TEST_STREAM_URL));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(amqpvalue_get_map_key_value_pair(IGNORED_PTR_ARG, 2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_key(&key_name, sizeof(key_name))
        .CopyOutArgumentBuffer_value(&key_value, sizeof(key_value));
    STRICT_EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &STREAM_PROP_AUTH_TOKEN, sizeof(STREAM_PROP_AUTH_TOKEN));
    STRICT_EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &TEST_AUTHORIZATION_TOKEN, sizeof(TEST_AUTHORIZATION_TOKEN));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(1, &TEST_AUTHORIZATION_TOKEN, sizeof(TEST_AUTHORIZATION_TOKEN));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_create_stream_d2c_response_from_parsed_info()
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_UNIQUE_ID, sizeof(TEST_UNIQUE_ID));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_STREAM_URL, sizeof(TEST_STREAM_URL));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_AUTHORIZATION_TOKEN, sizeof(TEST_AUTHORIZATION_TOKEN));
}

static void set_expected_calls_for_create_stream_c2d_request_from_parsed_info()
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_UNIQUE_ID, sizeof(TEST_UNIQUE_ID));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_STREAM_NAME, sizeof(TEST_STREAM_NAME));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_STREAM_URL, sizeof(TEST_STREAM_URL));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_AUTHORIZATION_TOKEN, sizeof(TEST_AUTHORIZATION_TOKEN));
}

static void set_expected_calls_for_destroy_parsed_info()
{
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG)); // req id
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG)); // url
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG)); // auth token
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG)); // name
}

static void set_expected_calls_for_on_amqp_message_received_callback()
{
    STRICT_EXPECTED_CALL(amqp_messenger_destroy_disposition_info(IGNORED_PTR_ARG));

    set_expected_calls_for_parse_amqp_message();

    set_expected_calls_for_create_stream_c2d_request_from_parsed_info();

    STRICT_EXPECTED_CALL(stream_c2d_request_destroy(IGNORED_PTR_ARG));

    set_expected_calls_for_destroy_parsed_info();
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
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_set_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_destroy(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_amqp_streaming_client_do_work()
{
    STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqp_messenger_do_work(TEST_AMQP_MESSENGER_HANDLE));
}


// ---------- Consolidated Helpers ---------- //

static const char* get_product_info(void* ctx)
{
    (void)ctx;
    return TEST_CLIENT_VERSION_STR;
}

static AMQP_STREAMING_CLIENT_CONFIG g_stream_client_config;
static AMQP_STREAMING_CLIENT_CONFIG* get_stream_client_config()
{
    g_stream_client_config.prod_info_cb = get_product_info;
    g_stream_client_config.prod_info_ctx = NULL;
    g_stream_client_config.device_id = TEST_DEVICE_ID;
    g_stream_client_config.iothub_host_fqdn = TEST_IOTHUB_HOST_FQDN;
    g_stream_client_config.on_state_changed_callback = TEST_on_state_changed_callback;
    g_stream_client_config.on_state_changed_context = TEST_ON_STATE_CHANGED_CB_CONTEXT;

    return &g_stream_client_config;
}

static DOWORK_TEST_PROFILE g_default_dowork_test_profile;
static DOWORK_TEST_PROFILE* get_default_dowork_test_profile()
{
    g_default_dowork_test_profile.set_amqp_msgr_state = false;
    g_default_dowork_test_profile.new_state = AMQP_MESSENGER_STATE_STOPPED;
    g_default_dowork_test_profile.previous_state = AMQP_MESSENGER_STATE_STOPPED;
    g_default_dowork_test_profile.set_amqp_msgr_subscription_state = false;
    g_default_dowork_test_profile.is_amqp_msgr_subscribed = false;
    return &g_default_dowork_test_profile;
}

static AMQP_STREAMING_CLIENT_HANDLE create_amqp_streaming_client(AMQP_STREAMING_CLIENT_CONFIG* config)
{
    if (config == NULL)
    {
        config = get_stream_client_config();
    }

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_create(config);
    return amqp_streaming_client_create(config);
}

static AMQP_STREAMING_CLIENT_HANDLE create_and_start_amqp_streaming_client(AMQP_STREAMING_CLIENT_CONFIG* config)
{
    AMQP_STREAMING_CLIENT_HANDLE result = create_amqp_streaming_client(config);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_start();
    (void)amqp_streaming_client_start(result, TEST_SESSION_HANDLE);

    TEST_amqp_messenger_create_config.on_state_changed_callback(
        TEST_amqp_messenger_create_config.on_state_changed_context, AMQP_MESSENGER_STATE_STOPPED, AMQP_MESSENGER_STATE_STARTING);

    return result;
}

static void crank_amqp_streaming_client_do_work(AMQP_STREAMING_CLIENT_HANDLE handle, DOWORK_TEST_PROFILE* test_profile)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_do_work();
    amqp_streaming_client_do_work(handle);

    if (test_profile != NULL)
    {
        if (test_profile->set_amqp_msgr_subscription_state)
        {
            TEST_amqp_messenger_create_config.on_subscription_changed_callback(
                TEST_amqp_messenger_create_config.on_subscription_changed_context, test_profile->is_amqp_msgr_subscribed);
        }

        if (test_profile->set_amqp_msgr_state)
        {
            TEST_amqp_messenger_create_config.on_state_changed_callback(
                TEST_amqp_messenger_create_config.on_state_changed_context, test_profile->previous_state, test_profile->new_state);
        }
    }
}

static AMQP_STREAMING_CLIENT_HANDLE create_start_and_crank_amqp_streaming_client(AMQP_STREAMING_CLIENT_CONFIG* config)
{
    AMQP_STREAMING_CLIENT_HANDLE result = create_and_start_amqp_streaming_client(config);

    DOWORK_TEST_PROFILE* test_profile = get_default_dowork_test_profile();
    test_profile->set_amqp_msgr_subscription_state = true;
    test_profile->is_amqp_msgr_subscribed = true;
    test_profile->set_amqp_msgr_state = true;
    test_profile->previous_state = AMQP_MESSENGER_STATE_STARTING;
    test_profile->new_state = AMQP_MESSENGER_STATE_STARTED;

    crank_amqp_streaming_client_do_work(result, test_profile);

    return result;
}

static DEVICE_STREAM_C2D_REQUEST saved_stream_c2d_requests[10];
static size_t saved_stream_c2d_requests_count;
static DEVICE_STREAM_C2D_RESPONSE* on_device_stream_c2d_request_received_result;
static DEVICE_STREAM_C2D_RESPONSE* on_device_stream_c2d_request_received(DEVICE_STREAM_C2D_REQUEST* request, void* context)
{
    (void)context;

    if (request->name != NULL)
    {
        size_t length = strlen(request->name) + 1;
        saved_stream_c2d_requests[saved_stream_c2d_requests_count].name = (char*)real_malloc(sizeof(char) * length);
        (void)memcpy(saved_stream_c2d_requests[saved_stream_c2d_requests_count].name, request->name, length);
    }

    if (request->authorization_token != NULL)
    {
        size_t length = strlen(request->authorization_token) + 1;
        saved_stream_c2d_requests[saved_stream_c2d_requests_count].authorization_token = (char*)real_malloc(sizeof(char) * length);
        (void)memcpy(saved_stream_c2d_requests[saved_stream_c2d_requests_count].authorization_token, request->authorization_token, length);
    }

    if (request->url != NULL)
    {
        size_t length = strlen(request->url) + 1;
        saved_stream_c2d_requests[saved_stream_c2d_requests_count].url = (char*)real_malloc(sizeof(char) * length);
        (void)memcpy(saved_stream_c2d_requests[saved_stream_c2d_requests_count].url, request->url, length);
    }

    if (request->request_id != NULL)
    {
        size_t length = strlen(request->request_id) + 1;
        saved_stream_c2d_requests[saved_stream_c2d_requests_count].request_id = (char*)real_malloc(sizeof(char) * length);
        (void)memcpy(saved_stream_c2d_requests[saved_stream_c2d_requests_count].request_id, request->request_id, length);
    }

    saved_stream_c2d_requests_count++;

    return on_device_stream_c2d_request_received_result;
}


// ---------- Mock Helpers ---------- //

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(amqp_messenger_create, TEST_amqp_messenger_create);
    REGISTER_GLOBAL_MOCK_HOOK(amqp_messenger_send_async, TEST_amqp_messenger_send_async);
    REGISTER_GLOBAL_MOCK_HOOK(amqp_messenger_subscribe_for_messages, TEST_amqp_messenger_subscribe_for_messages);
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
    REGISTER_GLOBAL_MOCK_HOOK(stream_c2d_request_destroy, real_stream_c2d_request_destroy);
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
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(uuid, void*);
    REGISTER_UMOCK_ALIAS_TYPE(UUID_T, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_TYPE, int);
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

    REGISTER_GLOBAL_MOCK_RETURN(amqp_messenger_set_option, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_messenger_set_option, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_messenger_send_async, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_messenger_send_async, 1);

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

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_uuid, TEST_UUID_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_uuid, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_int, TEST_INT_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_int, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_set_map_value, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_set_map_value, 1);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_type, AMQP_TYPE_NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_uuid, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_uuid, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_string, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_string, 1);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_inplace_described_value, NULL);
    
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_map_pair_count, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_map_pair_count, 1);
    
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_map_key_value_pair, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_map_key_value_pair, 1);

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

    REGISTER_GLOBAL_MOCK_RETURN(message_get_properties, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_set_properties, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_set_application_properties, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_application_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_get_application_properties, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_application_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_set_message_annotations, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_message_annotations, 1);
    
    REGISTER_GLOBAL_MOCK_RETURN(message_add_body_amqp_data, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_add_body_amqp_data, 1);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_type, MESSAGE_BODY_TYPE_NONE);

    REGISTER_GLOBAL_MOCK_RETURN(message_get_body_amqp_data_count, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_amqp_data_count, 1);
    
    REGISTER_GLOBAL_MOCK_RETURN(message_get_body_amqp_data_in_place, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_amqp_data_in_place, 1);

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

    REGISTER_GLOBAL_MOCK_RETURN(properties_set_correlation_id, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_set_correlation_id, 1);
    
    REGISTER_GLOBAL_MOCK_RETURN(properties_get_correlation_id, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_get_correlation_id, 1);

    REGISTER_GLOBAL_MOCK_RETURN(properties_get_content_type, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_get_content_type, 1);
    
    REGISTER_GLOBAL_MOCK_RETURN(properties_get_content_encoding, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_get_content_encoding, 1);

    // UniqueId
    REGISTER_GLOBAL_MOCK_RETURN(UniqueId_Generate, UNIQUEID_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UniqueId_Generate, UNIQUEID_ERROR);

    // UUID
    REGISTER_GLOBAL_MOCK_RETURN(UUID_from_string, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UUID_from_string, 1);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UUID_to_string, NULL);

    // AgentTime
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(get_time, INDEFINITE_TIME);

    // TickCounter
    REGISTER_GLOBAL_MOCK_RETURN(tickcounter_create, TEST_TICK_COUNTER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(tickcounter_get_current_ms, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_get_current_ms, 1);

    // gballoc
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
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

    TEST_on_state_changed_callback_context = NULL;
    TEST_on_state_changed_callback_previous_state = AMQP_STREAMING_CLIENT_STATE_STOPPED;
    TEST_on_state_changed_callback_new_state = AMQP_STREAMING_CLIENT_STATE_STOPPED;

    while (saved_stream_c2d_requests_count > 0)
    {
        saved_stream_c2d_requests_count--;

        if (saved_stream_c2d_requests[saved_stream_c2d_requests_count].name != NULL)
        {
            real_free(saved_stream_c2d_requests[saved_stream_c2d_requests_count].name);
            saved_stream_c2d_requests[saved_stream_c2d_requests_count].name = NULL;
        }

        if (saved_stream_c2d_requests[saved_stream_c2d_requests_count].url != NULL)
        {
            real_free(saved_stream_c2d_requests[saved_stream_c2d_requests_count].url);
            saved_stream_c2d_requests[saved_stream_c2d_requests_count].url = NULL;
        }

        if (saved_stream_c2d_requests[saved_stream_c2d_requests_count].authorization_token != NULL)
        {
            real_free(saved_stream_c2d_requests[saved_stream_c2d_requests_count].authorization_token);
            saved_stream_c2d_requests[saved_stream_c2d_requests_count].authorization_token = NULL;
        }

        if (saved_stream_c2d_requests[saved_stream_c2d_requests_count].request_id != NULL)
        {
            real_free(saved_stream_c2d_requests[saved_stream_c2d_requests_count].request_id);
            saved_stream_c2d_requests[saved_stream_c2d_requests_count].request_id = NULL;
        }
    }
    on_device_stream_c2d_request_received_result = NULL;

    TEST_amqp_messenger_send_async_saved_messenger_handle = NULL;
    TEST_amqp_messenger_send_async_saved_message = NULL;
    TEST_amqp_messenger_send_async_saved_on_user_defined_send_complete_callback = NULL;
    TEST_amqp_messenger_send_async_saved_user_context = NULL;
    TEST_amqp_messenger_send_async_return = 0;

    TEST_amqp_messenger_subscribe_for_messages_saved_messenger_handle = NULL;
    TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback = NULL;
    TEST_amqp_messenger_subscribe_for_messages_saved_context = NULL;
    TEST_amqp_messenger_subscribe_for_messages_result = 0;

    while (saved_malloc_returns_count > 0)
    {
        saved_malloc_returns_count--;
        free(saved_malloc_returns[saved_malloc_returns_count]);
        saved_malloc_returns[saved_malloc_returns_count] = NULL;
    }
}

BEGIN_TEST_SUITE(iothubtransport_amqp_streaming_ut)

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

    saved_stream_c2d_requests_count = 0;
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    reset_test_data();

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

    while (saved_stream_c2d_requests_count > 0)
    {
        saved_stream_c2d_requests_count--;
        free(saved_stream_c2d_requests[saved_stream_c2d_requests_count].name);
        free(saved_stream_c2d_requests[saved_stream_c2d_requests_count].url);
        free(saved_stream_c2d_requests[saved_stream_c2d_requests_count].authorization_token);
        free(saved_stream_c2d_requests[saved_stream_c2d_requests_count].request_id);
    }
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_003: [amqp_streaming_client_create() shall allocate memory for the messenger instance structure (aka `instance`)]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_005: [amqp_streaming_client_create() shall save a copy of `client_config` info into `instance`]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_007: [`instance->amqp_msgr` shall be set using amqp_messenger_create(), passing a AMQP_MESSENGER_CONFIG instance `amqp_msgr_config`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_008: [`amqp_msgr_config->client_version` shall be set with `instance->client_version`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_009: [`amqp_msgr_config->device_id` shall be set with `instance->device_id`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_010: [`amqp_msgr_config->iothub_host_fqdn` shall be set with `instance->iothub_host_fqdn`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_011: [`amqp_msgr_config` shall have "streams/" as send link target suffix and receive link source suffix]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_012: [`amqp_msgr_config` shall have send and receive link attach properties set as "com.microsoft:client-version" = `instance->client_version`, "com.microsoft:channel-correlation-id" = `stream:<UUID>`, "com.microsoft:api-version" = "2016-11-14"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_013: [`amqp_msgr_config` shall be set with `on_amqp_messenger_state_changed_callback` and `on_amqp_messenger_subscription_changed_callback` callbacks]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_015: [If no failures occur, amqp_streaming_client_create() shall return a handle to `instance`]  
TEST_FUNCTION(amqp_streaming_create_success)
{
    // arrange
    AMQP_STREAMING_CLIENT_CONFIG* config = get_stream_client_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_create(config);

    // act
    AMQP_STREAMING_CLIENT_HANDLE handle = amqp_streaming_client_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, TEST_DEVICE_ID, TEST_amqp_messenger_create_config.device_id);
    ASSERT_ARE_EQUAL(char_ptr, TEST_IOTHUB_HOST_FQDN, TEST_amqp_messenger_create_config.iothub_host_fqdn);
    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_create_config.prod_info_cb);
    ASSERT_ARE_EQUAL(void_ptr, (void*)TEST_ATTACH_PROPERTIES, (void*)TEST_amqp_messenger_create_config.send_link.attach_properties);
    ASSERT_ARE_EQUAL(void_ptr, (void*)TEST_ATTACH_PROPERTIES, (void*)TEST_amqp_messenger_create_config.receive_link.attach_properties);
    ASSERT_ARE_EQUAL(char_ptr, DEFAULT_STREAMS_SEND_LINK_SOURCE_NAME, (void*)TEST_amqp_messenger_create_config.send_link.target_suffix);
    ASSERT_ARE_EQUAL(char_ptr, DEFAULT_STREAMS_RECEIVE_LINK_TARGET_NAME, (void*)TEST_amqp_messenger_create_config.receive_link.source_suffix);
    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_create_config.on_state_changed_callback);
    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_create_config.on_subscription_changed_callback);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_004: [If malloc() fails, amqp_streaming_client_create() shall fail and return NULL]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_006: [If any `client_config` info fails to be copied, amqp_streaming_client_create() shall fail and return NULL]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_014: [If amqp_messenger_create() fails, amqp_streaming_client_create() shall fail and return NULL]  
TEST_FUNCTION(amqp_streaming_create_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_STREAMING_CLIENT_CONFIG* config = get_stream_client_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_create(config);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (!umock_c_negative_tests_can_call_fail(i))
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        AMQP_STREAMING_CLIENT_HANDLE handle = amqp_streaming_client_create(config);

        // assert
        sprintf(error_msg, "On failed call %zu", i);
        ASSERT_IS_NULL(handle, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_001: [If parameter `client_config` is NULL, amqp_streaming_client_create() shall return NULL]  
TEST_FUNCTION(amqp_streaming_create_NULL_client_config)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    AMQP_STREAMING_CLIENT_HANDLE handle = amqp_streaming_client_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(handle);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_002: [If `client_config`'s `device_id`, `iothub_host_fqdn` or `client_version` is NULL, amqp_streaming_client_create() shall return NULL]  
TEST_FUNCTION(amqp_streaming_create_NULL_client_config_field_device_id)
{
    // arrange
    AMQP_STREAMING_CLIENT_CONFIG* config = get_stream_client_config();
    config->device_id = NULL;

    umock_c_reset_all_calls();

    // act
    AMQP_STREAMING_CLIENT_HANDLE handle = amqp_streaming_client_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(handle);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_002: [If `client_config`'s `device_id`, `iothub_host_fqdn` or `client_version` is NULL, amqp_streaming_client_create() shall return NULL]  
TEST_FUNCTION(amqp_streaming_create_NULL_client_config_field_iothub_host_fqdn)
{
    // arrange
    AMQP_STREAMING_CLIENT_CONFIG* config = get_stream_client_config();
    config->iothub_host_fqdn = NULL;

    umock_c_reset_all_calls();

    // act
    AMQP_STREAMING_CLIENT_HANDLE handle = amqp_streaming_client_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(handle);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_002: [If `client_config`'s `device_id`, `iothub_host_fqdn` or `client_version` is NULL, amqp_streaming_client_create() shall return NULL]  
TEST_FUNCTION(amqp_streaming_create_NULL_client_config_field_client_version)
{
    // arrange
    AMQP_STREAMING_CLIENT_CONFIG* config = get_stream_client_config();
    config->prod_info_cb = NULL;

    umock_c_reset_all_calls();

    // act
    AMQP_STREAMING_CLIENT_HANDLE handle = amqp_streaming_client_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(handle);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_017: [amqp_messenger_start() shall be invoked passing `instance->amqp_msgr` and `session_handle`]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_019: [If no failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_STARTING, and `instance->on_state_changed_callback` invoked if provided]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_021: [If no failures occur, amqp_streaming_client_start() shall return 0]
TEST_FUNCTION(amqp_streaming_start_success)
{
    // arrange
    int result;
    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_start();

    // act
    result = amqp_streaming_client_start(handle, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, TEST_ON_STATE_CHANGED_CB_CONTEXT, TEST_on_state_changed_callback_context);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_STOPPED, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_STARTING, TEST_on_state_changed_callback_new_state);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_018: [If amqp_messenger_start() fails, amqp_streaming_client_start() fail and return a non-zero value]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_020: [If any failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided]
TEST_FUNCTION(amqp_streaming_start_failure_checks)
{
    // arrange
    int result;
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_start();
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    // act
    result = amqp_streaming_client_start(handle, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, TEST_ON_STATE_CHANGED_CB_CONTEXT, TEST_on_state_changed_callback_context);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_STARTING, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_ERROR, TEST_on_state_changed_callback_new_state);

    // cleanup
    amqp_streaming_client_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_016: [If `instance_handle` or `session_handle` are NULL, amqp_streaming_client_start() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_streaming_start_NULL_handle)
{
    // arrange
    int result;

    umock_c_reset_all_calls();

    // act
    result = amqp_streaming_client_start(NULL, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_016: [If `instance_handle` or `session_handle` are NULL, amqp_streaming_client_start() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_streaming_start_NULL_session_handle)
{
    // arrange
    int result;
    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();

    // act
    result = amqp_streaming_client_start(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_023: [amqp_messenger_stop() shall be invoked passing `instance->amqp_msgr`]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_025: [`instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_STOPPING, and `instance->on_state_changed_callback` invoked if provided]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_027: [If no failures occur, amqp_streaming_client_stop() shall return 0]
TEST_FUNCTION(amqp_streaming_stop_success)
{
    // arrange
    int result;
    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_stop();

    // act
    result = amqp_streaming_client_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, TEST_ON_STATE_CHANGED_CB_CONTEXT, TEST_on_state_changed_callback_context);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_STARTED, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_STOPPING, TEST_on_state_changed_callback_new_state);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_024: [If amqp_messenger_stop() fails, amqp_streaming_client_stop() fail and return a non-zero value]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_026: [If any failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided]
TEST_FUNCTION(amqp_streaming_stop_failure_checks)
{
    // arrange
    int result;
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_stop();
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    // act
    result = amqp_streaming_client_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, TEST_ON_STATE_CHANGED_CB_CONTEXT, TEST_on_state_changed_callback_context);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_STOPPING, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_ERROR, TEST_on_state_changed_callback_new_state);

    // cleanup
    amqp_streaming_client_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_022: [If `instance_handle` is NULL, amqp_streaming_client_stop() shall fail and return a non-zero value]  
TEST_FUNCTION(amqp_streaming_stop_NULL_handle)
{
    // arrange
    int result;

    umock_c_reset_all_calls();

    // act
    result = amqp_streaming_client_stop(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_033: [`instance->amqp_messenger` shall be destroyed using amqp_messenger_destroy()]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_034: [amqp_streaming_client_destroy() shall release all memory allocated for and within `instance`]  
TEST_FUNCTION(amqp_streaming_destroy_success)
{
    // arrange
    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_streaming_client_stop();
    STRICT_EXPECTED_CALL(amqp_messenger_destroy(TEST_AMQP_MESSENGER_HANDLE));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    amqp_streaming_client_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_ON_STATE_CHANGED_CB_CONTEXT, TEST_on_state_changed_callback_context);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_STARTED, TEST_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AMQP_STREAMING_CLIENT_STATE_STOPPING, TEST_on_state_changed_callback_new_state);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_032: [If `instance_handle` is NULL, amqp_streaming_client_destroy() shall return immediately]  
TEST_FUNCTION(amqp_streaming_destroy_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    amqp_streaming_client_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_036: [amqp_messenger_set_option() shall be invoked passing `name` and `option`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_038: [If no errors occur, amqp_streaming_client_set_option shall return zero]
TEST_FUNCTION(amqp_streaming_set_option_amqp_messenger_success)
{
    // arrange
    int result;
    int value = 123;
    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);
    
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(amqp_messenger_set_option(TEST_AMQP_MESSENGER_HANDLE, IGNORED_PTR_ARG, &value));

    // act
    result = amqp_streaming_client_set_option(handle, "some_amqp_messenger_option", &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_037: [If amqp_messenger_set_option() fails, amqp_streaming_client_set_option() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_streaming_set_option_failure_checks)
{
    // arrange
    int result;
    int value = 123;
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(amqp_messenger_set_option(TEST_AMQP_MESSENGER_HANDLE, IGNORED_PTR_ARG, &value));
    umock_c_negative_tests_snapshot();

    umock_c_negative_tests_reset();
    umock_c_negative_tests_fail_call(0);

    // act
    result = amqp_streaming_client_set_option(handle, "some_amqp_messenger_option", &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_streaming_client_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_035: [If `instance_handle` or `name` or `value` are NULL, amqp_streaming_client_set_option() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_streaming_set_option_NULL_handle)
{
    // arrange
    int result;
    int value = 123;

    umock_c_reset_all_calls();

    // act
    result = amqp_streaming_client_set_option(NULL, "some_amqp_messenger_option", &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_035: [If `instance_handle` or `name` or `value` are NULL, amqp_streaming_client_set_option() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_streaming_set_option_NULL_name)
{
    // arrange
    int result;
    int value = 123;
    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();

    // act
    result = amqp_streaming_client_set_option(handle, NULL, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_035: [If `instance_handle` or `name` or `value` are NULL, amqp_streaming_client_set_option() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_streaming_set_option_NULL_value)
{
    // arrange
    int result;
    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();

    // act
    result = amqp_streaming_client_set_option(handle, "some_amqp_messenger_option", NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_040: [amqp_streaming_client_retrieve_options() shall return the result of amqp_messenger_retrieve_options()]
TEST_FUNCTION(amqp_streaming_retrieve_options_success)
{
    // arrange
    OPTIONHANDLER_HANDLE result;
    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqp_messenger_retrieve_options(TEST_AMQP_MESSENGER_HANDLE));
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, IGNORED_PTR_ARG, TEST_OPTIONHANDLER_HANDLE));

    // act
    result = amqp_streaming_client_retrieve_options(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_OPTIONHANDLER_HANDLE, result);

    // cleanup
    amqp_streaming_client_destroy(handle);
}


TEST_FUNCTION(amqp_streaming_retrieve_options_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_STREAMING_CLIENT_HANDLE handle = create_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqp_messenger_retrieve_options(TEST_AMQP_MESSENGER_HANDLE));
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, IGNORED_PTR_ARG, TEST_OPTIONHANDLER_HANDLE));
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        OPTIONHANDLER_HANDLE result = amqp_streaming_client_retrieve_options(handle);

        // assert
        sprintf(error_msg, "On failed call %zu", i);
        ASSERT_IS_NULL(result, error_msg);
    }

    // cleanup
    amqp_streaming_client_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_039: [If `instance_handle` is NULL, amqp_streaming_client_retrieve_options shall fail and return NULL]
TEST_FUNCTION(amqp_streaming_retrieve_options_NULL_handle)
{
    // arrange
    OPTIONHANDLER_HANDLE result;

    umock_c_reset_all_calls();

    // act
    result = amqp_streaming_client_retrieve_options(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_042: [ `streamRequestCallback` and `context` shall be saved in `instance_handle` to be used in `on_amqp_message_received_callback` ]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_047: [ If no errors occur, the function shall return zero (success) ]
TEST_FUNCTION(amqp_streaming_client_set_stream_request_callback_success)
{
    // arrange
    char context = 'c';
    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();

    // act
    int result = amqp_streaming_client_set_stream_request_callback(handle, on_device_stream_c2d_request_received, &context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_042: [ `streamRequestCallback` and `context` shall be saved in `instance_handle` to be used in `on_amqp_message_received_callback` ]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_047: [ If no errors occur, the function shall return zero (success) ]
TEST_FUNCTION(amqp_streaming_client_set_stream_request_callback_NULL_callback)
{
    // arrange
    char context = 'c';
    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();

    // act
    int result = amqp_streaming_client_set_stream_request_callback(handle, NULL, &context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_041: [ If `instance_handle` is NULL, the function shall return a non-zero value (failure) ]
TEST_FUNCTION(amqp_streaming_client_set_stream_request_callback_NULL_handle)
{
    // arrange
    char context = 'c';

    umock_c_reset_all_calls();

    // act
    int result = amqp_streaming_client_set_stream_request_callback(NULL, on_device_stream_c2d_request_received, &context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_049: [ `streamRequestCallback` shall be invoked passing the parsed STREAM_REQUEST_INFO instance ]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_050: [ The response from `streamRequestCallback` shall be saved to be sent on the next call to _do_work() ]  
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_052: [ If no parsing errors occur, `on_amqp_message_received_callback` shall return AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED ]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_058: [ The AMQP message shall be parsed in to STREAM_INFO instance and passed to the invokation of the saved `streamRequestCallback` ] 
TEST_FUNCTION(on_amqp_message_received_callback_c2d_request)
{
    // arrange
    char context = 'c';
    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disp_info;

    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);
    (void)amqp_streaming_client_set_stream_request_callback(handle, on_device_stream_c2d_request_received, &context);

    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_amqp_message_received_callback();

    // act
    TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback(
        TEST_MESSAGE_HANDLE, &disp_info, TEST_amqp_messenger_subscribe_for_messages_saved_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 1, saved_stream_c2d_requests_count);
    ASSERT_ARE_EQUAL(char_ptr, TEST_UNIQUE_ID, saved_stream_c2d_requests[0].request_id);
    ASSERT_ARE_EQUAL(char_ptr, TEST_STREAM_NAME, saved_stream_c2d_requests[0].name);
    ASSERT_ARE_EQUAL(char_ptr, TEST_STREAM_URL, saved_stream_c2d_requests[0].url);
    ASSERT_ARE_EQUAL(char_ptr, TEST_AUTHORIZATION_TOKEN, saved_stream_c2d_requests[0].authorization_token);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_051: [ If any parsing errors occur, `on_amqp_message_received_callback` shall return AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED ]
TEST_FUNCTION(on_amqp_message_received_callback_c2d_request_failure_parsing)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    char context = 'c';
    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disp_info;

    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);
    (void)amqp_streaming_client_set_stream_request_callback(handle, on_device_stream_c2d_request_received, &context);

    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_amqp_message_received_callback();
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 5 || i == 7 || i == 8 || i == 16 || i == 17 || i == 22 || i == 23 || i == 28 || i == 29 || i == 30 || i == 31 || i == 44 || i >= 45 || i <= 51)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback(
            TEST_MESSAGE_HANDLE, &disp_info, TEST_amqp_messenger_subscribe_for_messages_saved_context);

        // assert
        (void)sprintf(error_msg, "Failed on call %zu", i);
        ASSERT_ARE_EQUAL(int, 0, saved_stream_c2d_requests_count, error_msg);
    }

    // cleanup
    amqp_streaming_client_destroy(handle);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_048: [If `message` or `context` are NULL, on_amqp_message_received_callback shall return immediately]
TEST_FUNCTION(on_amqp_message_received_callback_NULL_message)
{
    // arrange
    char context = 'c';
    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disp_info;

    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);
    (void)amqp_streaming_client_set_stream_request_callback(handle, on_device_stream_c2d_request_received, &context);

    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback);

    umock_c_reset_all_calls();

    // act
    TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback(
        NULL, &disp_info, TEST_amqp_messenger_subscribe_for_messages_saved_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, saved_stream_c2d_requests_count);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_048: [If `message` or `context` are NULL, on_amqp_message_received_callback shall return immediately]
TEST_FUNCTION(on_amqp_message_received_callback_NULL_context)
{
    // arrange
    char context = 'c';
    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disp_info;

    AMQP_STREAMING_CLIENT_HANDLE handle = create_start_and_crank_amqp_streaming_client(NULL);
    (void)amqp_streaming_client_set_stream_request_callback(handle, on_device_stream_c2d_request_received, &context);

    ASSERT_IS_NOT_NULL(TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback);

    umock_c_reset_all_calls();

    // act
    TEST_amqp_messenger_subscribe_for_messages_saved_on_message_received_callback(
        TEST_MESSAGE_HANDLE, &disp_info, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, saved_stream_c2d_requests_count);

    // cleanup
    amqp_streaming_client_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_028: [If `instance_handle` is NULL, amqp_streaming_client_do_work() shall return immediately]
TEST_FUNCTION(amqp_streaming_client_do_work_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    amqp_streaming_client_do_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_031: [amqp_streaming_client_do_work() shall invoke amqp_messenger_do_work() passing `instance->amqp_msgr`]  
TEST_FUNCTION(amqp_streaming_client_do_work_success)
{
    // arrange
    AMQP_STREAMING_CLIENT_HANDLE handle = create_and_start_amqp_streaming_client(NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(amqp_messenger_do_work(TEST_AMQP_MESSENGER_HANDLE));

    // act
    amqp_streaming_client_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_streaming_client_destroy(handle);
}

END_TEST_SUITE(iothubtransport_amqp_streaming_ut)
