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

static void* real_malloc(size_t size)
{
    return malloc(size);
}

static void real_free(void* ptr)
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

#define ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "azure_c_shared_utility/map.h"
#include "azure_uamqp_c/session.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/message_receiver.h"
#include "internal/message_queue.h"
#include "internal/iothub_client_retry_control.h"

#undef ENABLE_MOCKS

#include "internal/iothubtransport_amqp_messenger.h"


static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

#define MESSENGER_IS_STARTED             true
#define MESSENGER_IS_NOT_STARTED         false

#define MESSENGER_IS_SUBSCRIBED          true
#define MESSENGER_IS_NOT_SUBSCRIBED      false

#define MESSAGE_RECEIVER_IS_CREATED      true
#define MESSAGE_RECEIVER_IS_NOT_CREATED  false

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

static const char* test_get_product_info(void* ctx)
{
    (void)ctx;
    return "test_product_info";
}

#define DEFAULT_EVENT_SEND_RETRY_LIMIT                    10
#define DEFAULT_EVENT_SEND_TIMEOUT_SECS 600

#define UNIQUE_ID_BUFFER_SIZE                             37
#define TEST_UNIQUE_ID                                    "A1234DE234A1234DE234A1234DE234A1234DEA1234DE234A1234DE234A1234DE234A1234DEA1234DE234A1234DE234A1234DE234A1234DE"

#define TEST_DEVICE_ID                                    "my_device"
#define TEST_DEVICE_ID_STRING_HANDLE                      (STRING_HANDLE)0x4442
#define TEST_IOTHUB_HOST_FQDN_STRING_HANDLE               (STRING_HANDLE)0x4443
#define TEST_MODULE_ID                                    "my_module"
#define TEST_IOTHUB_HOST_FQDN                             "some.fqdn.com"
#define TEST_ON_STATE_CHANGED_CB_CONTEXT                  (void*)0x4445
#define TEST_STRING_HANDLE                                (STRING_HANDLE)0x4446
#define TEST_SESSION_HANDLE                               (SESSION_HANDLE)0x4447
#define TEST_AMQP_MESSENGER_HANDLE                          (AMQP_MESSENGER_HANDLE)0x4448
#define TEST_DEVICES_PATH_STRING_HANDLE                   (STRING_HANDLE)0x4449
#define TEST_DEVICES_PATH_CHAR_PTR                        "iothub.azure-devices.net/devices/some-device-id"

#define TEST_SEND_LINK_TARGET_SUFFIX_CHAR_PTR              "messages/events"
#define TEST_RECEIVE_LINK_SOURCE_SUFFIX_CHAR_PTR          "messages/devicebound"

#define TEST_EVENT_SEND_ADDRESS_STRING_HANDLE             (STRING_HANDLE)0x4450
#define TEST_EVENT_SEND_ADDRESS_CHAR_PTR                  "amqps:/iothub.azure-devices.net/devices/some-device-id/" TEST_SEND_LINK_TARGET_SUFFIX_CHAR_PTR

#define TEST_MESSAGE_RECEIVE_ADDRESS_STRING_HANDLE        (STRING_HANDLE)0x4451
#define TEST_MESSAGE_RECEIVE_ADDRESS_CHAR_PTR             "amqps://iothub.azure-devices.net/devices/some-device-id/" TEST_RECEIVE_LINK_SOURCE_SUFFIX_CHAR_PTR

#define TEST_EVENT_SENDER_SOURCE_NAME_STRING_HANDLE       (STRING_HANDLE)0x4452
#define TEST_EVENT_SENDER_SOURCE_NAME_CHAR_PTR            "event_sender_source_name"
#define TEST_MESSAGE_RECEIVER_TARGET_NAME_STRING_HANDLE   (STRING_HANDLE)0x4453
#define TEST_MESSAGE_RECEIVER_TARGET_NAME_CHAR_PTR        "message_receiver_target_name"

#define TEST_SEND_LINK_SOURCE_AMQP_VALUE                  (AMQP_VALUE)0x4454
#define TEST_SEND_LINK_TARGET_AMQP_VALUE                  (AMQP_VALUE)0x4455

#define TEST_EVENT_SENDER_LINK_NAME_STRING_HANDLE         (STRING_HANDLE)0x4456
#define TEST_EVENT_SENDER_LINK_NAME_CHAR_PTR              "event_sender_link_name"
#define TEST_MESSAGE_RECEIVER_LINK_NAME_STRING_HANDLE     (STRING_HANDLE)0x4457
#define TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR          "message_receiver_link_name"
#define TEST_EVENT_SENDER_LINK_HANDLE                     (LINK_HANDLE)0x4458
#define MESSAGE_SENDER_MAX_LINK_SIZE                      UINT64_MAX
#define MESSAGE_RECEIVER_MAX_LINK_SIZE                    65536
#define TEST_MESSAGE_SENDER_HANDLE                        (MESSAGE_SENDER_HANDLE)0x4459
#define TEST_LINK_ATTACH_PROPERTIES                       (fields)0x4460
#define TEST_LINK_DEVICE_TYPE_NAME_AMQP_VALUE             (AMQP_VALUE)0x4461
#define TEST_LINK_DEVICE_TYPE_VALUE_AMQP_VALUE            (AMQP_VALUE)0x4462

#define TEST_MESSAGE_RECEIVER_HANDLE                      (MESSAGE_RECEIVER_HANDLE)0x4463
#define TEST_MESSAGE_RECEIVER_LINK_HANDLE                 (LINK_HANDLE)0x4464

#define TEST_MESSAGE_HANDLE                               (MESSAGE_HANDLE)0x4467
#define TEST_RECEIVE_LINK_SOURCE_AMQP_VALUE                  (AMQP_VALUE)0x4468
#define TEST_RECEIVE_LINK_TARGET_AMQP_VALUE                  (AMQP_VALUE)0x4469
#define TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT           (void*)0x4470
#define TEST_MESSAGE_DISPOSITION_ACCEPTED_AMQP_VALUE      (AMQP_VALUE)0x4471
#define TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE      (AMQP_VALUE)0x4472
#define TEST_MESSAGE_DISPOSITION_REJECTED_AMQP_VALUE      (AMQP_VALUE)0x4473
#define TEST_LIST_ITEM_HANDLE                             (LIST_ITEM_HANDLE)0x4477
#define TEST_SEND_EVENT_TASK                              (const void*)0x4478
#define TEST_IOTHUB_CLIENT_HANDLE                         (void*)0x4479
#define TEST_IN_PROGRESS_LIST1                            (SINGLYLINKEDLIST_HANDLE)0x4483
#define TEST_IN_PROGRESS_LIST2                            (SINGLYLINKEDLIST_HANDLE)0x4484
#define TEST_OPTIONHANDLER_HANDLE                         (OPTIONHANDLER_HANDLE)0x4485
#define TEST_MESSAGE_QUEUE_HANDLE                         (MESSAGE_QUEUE_HANDLE)0x4486
#define INDEFINITE_TIME                                   ((time_t)-1)
#define TEST_CLIENT_VERSION_STR                              "client x (version y)"
#define TEST_SEND_LINK_ATTACH_PROPERTIES                  (MAP_HANDLE)0x4487
#define TEST_RECEIVE_LINK_ATTACH_PROPERTIES                  (MAP_HANDLE)0x4488
#define TEST_MAP_HANDLE                                      (MAP_HANDLE)0x4489

static char* map_key = "abcdefghij";
static char* map_value = "0123456789";
static char* map_keys[8];
static char* map_values[8];

static delivery_number TEST_DELIVERY_NUMBER;


// Helpers

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

static void* saved_on_state_changed_callback_context;
static AMQP_MESSENGER_STATE saved_on_state_changed_callback_previous_state;
static AMQP_MESSENGER_STATE saved_on_state_changed_callback_new_state;

static void TEST_on_state_changed_callback(void* context, AMQP_MESSENGER_STATE previous_state, AMQP_MESSENGER_STATE new_state)
{
    saved_on_state_changed_callback_context = context;
    saved_on_state_changed_callback_previous_state = previous_state;
    saved_on_state_changed_callback_new_state = new_state;
}


static LINK_HANDLE saved_messagesender_create_link;
static ON_MESSAGE_SENDER_STATE_CHANGED saved_messagesender_create_on_message_sender_state_changed;
static void* saved_messagesender_create_context;

static MESSAGE_SENDER_HANDLE TEST_messagesender_create(LINK_HANDLE link, ON_MESSAGE_SENDER_STATE_CHANGED on_message_sender_state_changed, void* context)
{
    saved_messagesender_create_link = link;
    saved_messagesender_create_on_message_sender_state_changed = on_message_sender_state_changed;
    saved_messagesender_create_context = context;

    return TEST_MESSAGE_SENDER_HANDLE;
}

static MESSAGE_RECEIVER_HANDLE TEST_messagereceiver_create_result;
static LINK_HANDLE saved_messagereceiver_create_link;
static ON_MESSAGE_RECEIVER_STATE_CHANGED saved_messagereceiver_create_on_message_receiver_state_changed;
static void* saved_messagereceiver_create_context;
static MESSAGE_RECEIVER_HANDLE TEST_messagereceiver_create(LINK_HANDLE link, ON_MESSAGE_RECEIVER_STATE_CHANGED on_message_receiver_state_changed, void* context)
{
    saved_messagereceiver_create_link = link;
    saved_messagereceiver_create_on_message_receiver_state_changed = on_message_receiver_state_changed;
    saved_messagereceiver_create_context = context;

    return TEST_messagereceiver_create_result;
}


static MESSAGE_HANDLE saved_on_new_message_received_callback_message;
static void* saved_on_new_message_received_callback_context;
static AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* saved_on_new_message_received_callback_disposition_info;
static AMQP_MESSENGER_DISPOSITION_RESULT TEST_on_new_message_received_callback_result;
static AMQP_MESSENGER_DISPOSITION_RESULT TEST_on_new_message_received_callback(MESSAGE_HANDLE message, AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
{
    saved_on_new_message_received_callback_message = message;
    saved_on_new_message_received_callback_disposition_info = disposition_info;
    saved_on_new_message_received_callback_context = context;
    return TEST_on_new_message_received_callback_result;
}


static int TEST_messagereceiver_open_result;
static MESSAGE_RECEIVER_HANDLE saved_messagereceiver_open_message_receiver;
static ON_MESSAGE_RECEIVED saved_messagereceiver_open_on_message_received;
static void* saved_messagereceiver_open_callback_context;
static int TEST_messagereceiver_open(MESSAGE_RECEIVER_HANDLE message_receiver, ON_MESSAGE_RECEIVED on_message_received, void* callback_context)
{
    saved_messagereceiver_open_message_receiver = message_receiver;
    saved_messagereceiver_open_on_message_received = on_message_received;
    saved_messagereceiver_open_callback_context = callback_context;

    return TEST_messagereceiver_open_result;
}

#define TEST_MESSAGE_QUEUE_ADD_BUFFERS_SIZE 10
static size_t TEST_message_queue_add_count;
static MESSAGE_QUEUE_HANDLE TEST_message_queue_add_message_queue[TEST_MESSAGE_QUEUE_ADD_BUFFERS_SIZE];
static MQ_MESSAGE_HANDLE TEST_message_queue_add_message[TEST_MESSAGE_QUEUE_ADD_BUFFERS_SIZE];
static MESSAGE_PROCESSING_COMPLETED_CALLBACK TEST_message_queue_add_on_message_processing_completed_callback[TEST_MESSAGE_QUEUE_ADD_BUFFERS_SIZE];
static void* TEST_message_queue_add_user_context[TEST_MESSAGE_QUEUE_ADD_BUFFERS_SIZE];
static int TEST_message_queue_add_return;
static int TEST_message_queue_add(MESSAGE_QUEUE_HANDLE message_queue, MQ_MESSAGE_HANDLE message, MESSAGE_PROCESSING_COMPLETED_CALLBACK on_message_processing_completed_callback, void* user_context)
{
    TEST_message_queue_add_message_queue[TEST_message_queue_add_count] = message_queue;
    TEST_message_queue_add_message[TEST_message_queue_add_count] = message;
    TEST_message_queue_add_on_message_processing_completed_callback[TEST_message_queue_add_count] = on_message_processing_completed_callback;
    TEST_message_queue_add_user_context[TEST_message_queue_add_count] = user_context;
    TEST_message_queue_add_count++;

    return TEST_message_queue_add_return;
}

static PROCESS_MESSAGE_CALLBACK TEST_on_process_message_callback;
static MESSAGE_QUEUE_HANDLE TEST_message_queue_create(MESSAGE_QUEUE_CONFIG* config)
{
    TEST_on_process_message_callback = config->on_process_message_callback;
    return TEST_MESSAGE_QUEUE_HANDLE;
}

static void TEST_remove_message_queue_first_item()
{
    if (TEST_message_queue_add_count > 0)
    {
        size_t c, d;

        for (c = 0, d = 1; d < TEST_MESSAGE_QUEUE_ADD_BUFFERS_SIZE; c++, d++)
        {
            TEST_message_queue_add_message_queue[c] = TEST_message_queue_add_message_queue[d];
            TEST_message_queue_add_message[c] = TEST_message_queue_add_message[d];
            TEST_message_queue_add_on_message_processing_completed_callback[c] = TEST_message_queue_add_on_message_processing_completed_callback[d];
            TEST_message_queue_add_user_context[c] = TEST_message_queue_add_user_context[d];
        }

        TEST_message_queue_add_message_queue[c] = NULL;
        TEST_message_queue_add_message[c] = NULL;
        TEST_message_queue_add_on_message_processing_completed_callback[c] = NULL;
        TEST_message_queue_add_user_context[c] = NULL;

        TEST_message_queue_add_count--;
    }
}

static void TEST_message_queue_destroy(MESSAGE_QUEUE_HANDLE mq_handle)
{
    (void)mq_handle;

    while (TEST_message_queue_add_count > 0)
    {
        TEST_message_queue_add_on_message_processing_completed_callback[0](TEST_message_queue_add_message[0], MESSAGE_QUEUE_CANCELLED, NULL, TEST_message_queue_add_user_context[0]);

        TEST_remove_message_queue_first_item();
    }
}

static AMQP_MESSENGER_CONFIG g_messenger_config;
static AMQP_MESSENGER_CONFIG* get_messenger_config()
{
    memset(&g_messenger_config, 0, sizeof(AMQP_MESSENGER_CONFIG));

    g_messenger_config.device_id = TEST_DEVICE_ID;
    g_messenger_config.iothub_host_fqdn = TEST_IOTHUB_HOST_FQDN;
    g_messenger_config.on_state_changed_callback = TEST_on_state_changed_callback;
    g_messenger_config.on_state_changed_context = TEST_ON_STATE_CHANGED_CB_CONTEXT;
    g_messenger_config.prod_info_cb = test_get_product_info;
    g_messenger_config.prod_info_ctx = NULL;

    g_messenger_config.send_link.target_suffix = TEST_SEND_LINK_TARGET_SUFFIX_CHAR_PTR;
    g_messenger_config.send_link.rcv_settle_mode = sender_settle_mode_settled;
    g_messenger_config.send_link.snd_settle_mode = receiver_settle_mode_first;
    g_messenger_config.send_link.attach_properties = TEST_SEND_LINK_ATTACH_PROPERTIES;

    g_messenger_config.receive_link.source_suffix = TEST_RECEIVE_LINK_SOURCE_SUFFIX_CHAR_PTR;
    g_messenger_config.receive_link.rcv_settle_mode = sender_settle_mode_settled;
    g_messenger_config.receive_link.snd_settle_mode = receiver_settle_mode_first;
    g_messenger_config.receive_link.attach_properties = TEST_RECEIVE_LINK_ATTACH_PROPERTIES;

    return &g_messenger_config;
}

static AMQP_MESSENGER_CONFIG* get_messenger_config_with_module()
{
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    config->module_id = TEST_MODULE_ID;
    return config;
}

typedef struct MESSENGER_DO_WORK_EXP_CALL_PROFILE_STRUCT
{
    AMQP_MESSENGER_STATE current_state;
    bool create_message_sender;
    bool destroy_message_sender;
    bool create_message_receiver;
    bool destroy_message_receiver;
    size_t send_event_timeout_secs;
    time_t current_time;
    size_t outgoing_messages_pending;
    size_t outgoing_messages_in_progress;
} MESSENGER_DO_WORK_EXP_CALL_PROFILE;

static MESSENGER_DO_WORK_EXP_CALL_PROFILE g_do_work_profile;

static MESSENGER_DO_WORK_EXP_CALL_PROFILE* get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE current_state, bool is_subscribed_for_messages, bool is_msg_rcvr_created, int wts_list_length, int ip_list_length, time_t current_time, size_t event_send_timeout_secs)
{
    memset(&g_do_work_profile, 0, sizeof(MESSENGER_DO_WORK_EXP_CALL_PROFILE));
    g_do_work_profile.current_state = current_state;
    g_do_work_profile.current_time = current_time;
    g_do_work_profile.send_event_timeout_secs = event_send_timeout_secs;
    g_do_work_profile.outgoing_messages_pending = wts_list_length;
    g_do_work_profile.outgoing_messages_in_progress = ip_list_length;

    if (g_do_work_profile.current_state == AMQP_MESSENGER_STATE_STARTING)
    {
        g_do_work_profile.create_message_sender = true;
    }
    else if (g_do_work_profile.current_state == AMQP_MESSENGER_STATE_STOPPING)
    {
        g_do_work_profile.destroy_message_sender = true;

        if (is_msg_rcvr_created)
        {
            g_do_work_profile.destroy_message_receiver = true;
        }
    }
    else if (g_do_work_profile.current_state == AMQP_MESSENGER_STATE_STARTED)
    {
        if (is_subscribed_for_messages && !is_msg_rcvr_created)
        {
            g_do_work_profile.create_message_receiver = true;
        }
        else if (!is_subscribed_for_messages && is_msg_rcvr_created)
        {
            g_do_work_profile.destroy_message_receiver = true;
        }
    }

    return &g_do_work_profile;
}

static IOTHUB_MESSAGE_HANDLE saved_message_create_from_iothub_message;
static int TEST_message_create_from_iothub_message_return;
static int TEST_message_create_from_iothub_message(IOTHUB_MESSAGE_HANDLE iothub_message, MESSAGE_HANDLE* uamqp_message)
{
    saved_message_create_from_iothub_message = iothub_message;

    if (TEST_message_create_from_iothub_message_return == 0)
    {
        *uamqp_message = TEST_MESSAGE_HANDLE;
    }

    return TEST_message_create_from_iothub_message_return;
}

static MESSAGE_SENDER_HANDLE saved_messagesender_send_message_sender;
static MESSAGE_HANDLE saved_messagesender_send_message;
static ON_MESSAGE_SEND_COMPLETE saved_messagesender_send_on_message_send_complete;
static void* saved_messagesender_send_callback_context;

static ASYNC_OPERATION_HANDLE TEST_messagesender_send(MESSAGE_SENDER_HANDLE message_sender, MESSAGE_HANDLE message, ON_MESSAGE_SEND_COMPLETE on_message_send_complete, void* callback_context, tickcounter_ms_t timeout)
{
    (void)timeout;
    saved_messagesender_send_message_sender = message_sender;
    saved_messagesender_send_message = message;
    saved_messagesender_send_on_message_send_complete = on_message_send_complete;
    saved_messagesender_send_callback_context = callback_context;

    return (ASYNC_OPERATION_HANDLE)0x64;
}

static void set_clone_link_configuration_expected_calls(role link_role, AMQP_MESSENGER_LINK_CONFIG* config)
{
    if (link_role == role_sender)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->target_suffix))
            .CopyOutArgumentBuffer(1, &config->target_suffix, sizeof(config->target_suffix));
    }
    else
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->source_suffix))
            .CopyOutArgumentBuffer(1, &config->source_suffix, sizeof(config->source_suffix));
    }

    STRICT_EXPECTED_CALL(Map_Clone(config->attach_properties));
}

static void set_clone_configuration_expected_calls(AMQP_MESSENGER_CONFIG* config)
{
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->device_id))
        .CopyOutArgumentBuffer(1, &config->device_id, sizeof(config->device_id));
    if (config->module_id != NULL)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->module_id))
            .CopyOutArgumentBuffer(1, &config->module_id, sizeof(config->module_id));
    }
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, config->iothub_host_fqdn))
        .CopyOutArgumentBuffer(1, &config->iothub_host_fqdn, sizeof(config->iothub_host_fqdn));
    set_clone_link_configuration_expected_calls(role_sender, &config->send_link);
    set_clone_link_configuration_expected_calls(role_receiver, &config->receive_link);
}

static void set_expected_calls_for_amqp_messenger_create(AMQP_MESSENGER_CONFIG* config)
{
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    set_clone_configuration_expected_calls(config);
    // memset() - not mocked.
    STRICT_EXPECTED_CALL(message_queue_create(IGNORED_PTR_ARG));
}

static void set_create_link_address_expected_calls(role link_role)
{
    (void)link_role;
    STRICT_EXPECTED_CALL(STRING_new());
    // EXPECTED: STRING_sprintf
}

static void set_create_link_name_expected_calls(role link_role)
{
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, UNIQUE_ID_BUFFER_SIZE));

    if (link_role == role_sender)
    {
        STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_EVENT_SENDER_LINK_NAME_STRING_HANDLE);
    }
    else
    {
        STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_MESSAGE_RECEIVER_LINK_NAME_STRING_HANDLE);
    }

    // EXPECTED: STRING_sprintf
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_attach_properties_expected_calls(const char* const** attach_property_keys, const char* const** attach_property_values, size_t number_of_attach_properties)
{
    size_t i;

    STRICT_EXPECTED_CALL(amqpvalue_create_map());
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &attach_property_keys, sizeof(attach_property_keys))
        .CopyOutArgumentBuffer(3, &attach_property_values, sizeof(attach_property_values))
        .CopyOutArgumentBuffer(4, &number_of_attach_properties, sizeof(size_t));

    for (i = 0; i < number_of_attach_properties; i++)
    {
        STRICT_EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_set_map_value(TEST_LINK_ATTACH_PROPERTIES, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, TEST_LINK_ATTACH_PROPERTIES));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_LINK_ATTACH_PROPERTIES));
}

static void set_create_link_expected_calls(role link_role, const char* const** attach_property_keys, const char* const** attach_property_values, size_t number_of_attach_properties)
{
    set_create_link_address_expected_calls(link_role);
    set_create_link_name_expected_calls(link_role);

    if (link_role == role_sender)
    {
        STRICT_EXPECTED_CALL(STRING_new());
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        // STRICT_EXPECTED_CALL(STRING_sprintf(TEST_STRING_HANDLE, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG)).SetReturn(TEST_SEND_LINK_SOURCE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG)).SetReturn(TEST_SEND_LINK_TARGET_AMQP_VALUE);

        STRICT_EXPECTED_CALL(STRING_delete(TEST_STRING_HANDLE));
    }
    else
    {
        STRICT_EXPECTED_CALL(STRING_new());
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        // STRICT_EXPECTED_CALL(STRING_sprintf(TEST_STRING_HANDLE, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG)).SetReturn(TEST_RECEIVE_LINK_SOURCE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG)).SetReturn(TEST_RECEIVE_LINK_TARGET_AMQP_VALUE);

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(link_create(TEST_SESSION_HANDLE, IGNORED_PTR_ARG, link_role, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(link_role == role_sender ? TEST_EVENT_SENDER_LINK_HANDLE : TEST_MESSAGE_RECEIVER_LINK_HANDLE);

    STRICT_EXPECTED_CALL(link_set_max_message_size(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    set_attach_properties_expected_calls(attach_property_keys, attach_property_values, number_of_attach_properties);

    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_message_receiver_create(const char* const** attach_property_keys, const char* const** attach_property_values, size_t number_of_attach_properties)
{
    set_create_link_expected_calls(role_receiver, attach_property_keys, attach_property_values, number_of_attach_properties);

    STRICT_EXPECTED_CALL(messagereceiver_create(TEST_MESSAGE_RECEIVER_LINK_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messagereceiver_open(TEST_MESSAGE_RECEIVER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void set_expected_calls_for_message_receiver_destroy()
{
    STRICT_EXPECTED_CALL(messagereceiver_close(TEST_MESSAGE_RECEIVER_HANDLE));
    STRICT_EXPECTED_CALL(messagereceiver_destroy(TEST_MESSAGE_RECEIVER_HANDLE));
    STRICT_EXPECTED_CALL(link_destroy(TEST_MESSAGE_RECEIVER_LINK_HANDLE));
}

static MQ_MESSAGE_HANDLE TEST_on_message_processing_completed_callback_message;
static MESSAGE_QUEUE_RESULT TEST_on_message_processing_completed_callback_result;
static USER_DEFINED_REASON TEST_on_message_processing_completed_callback_reason;
static void* TEST_on_message_processing_completed_callback_message_context;
static void TEST_on_message_processing_completed_callback(MQ_MESSAGE_HANDLE message, MESSAGE_QUEUE_RESULT result, USER_DEFINED_REASON reason, void* message_context)
{
    TEST_on_message_processing_completed_callback_message = message;
    TEST_on_message_processing_completed_callback_result = result;
    TEST_on_message_processing_completed_callback_reason = reason;
    TEST_on_message_processing_completed_callback_message_context = message_context;
}

static void set_expected_calls_for_amqp_messenger_send_async()
{
    STRICT_EXPECTED_CALL(message_clone(IGNORED_NUM_ARG));
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG)); // create_message_send_context
    STRICT_EXPECTED_CALL(message_queue_add(TEST_MESSAGE_QUEUE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static AMQP_MESSENGER_SEND_RESULT TEST_on_event_send_complete_result;
static AMQP_MESSENGER_REASON TEST_on_event_send_complete_reason;
static void* TEST_on_event_send_complete_context;
static void TEST_on_event_send_complete(AMQP_MESSENGER_SEND_RESULT result, AMQP_MESSENGER_REASON reason, void* context)
{
    TEST_on_event_send_complete_result = result;
    TEST_on_event_send_complete_reason = reason;
    TEST_on_event_send_complete_context = context;
}

static int send_events(AMQP_MESSENGER_HANDLE handle, int number_of_events)
{
    int events_sent = 0;

    while (number_of_events > 0)
    {
        set_expected_calls_for_amqp_messenger_send_async();
        if (amqp_messenger_send_async(handle, TEST_MESSAGE_HANDLE, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE) == 0)
        {
            events_sent++;
        }

        number_of_events--;
    }

    return events_sent;
}

static void set_expected_calls_for_message_sender_create(const char* const** attach_property_keys, const char* const** attach_property_values, size_t number_of_attach_properties)
{
    set_create_link_expected_calls(role_sender, attach_property_keys, attach_property_values, number_of_attach_properties);

    STRICT_EXPECTED_CALL(messagesender_create(TEST_EVENT_SENDER_LINK_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messagesender_open(TEST_MESSAGE_SENDER_HANDLE));
}

static void set_expected_calls_for_amqp_messenger_start(AMQP_MESSENGER_CONFIG* config, AMQP_MESSENGER_HANDLE messenger_handle)
{
    (void)config;
    (void)messenger_handle;
}

static void set_expected_calls_for_message_sender_destroy()
{
    STRICT_EXPECTED_CALL(messagesender_destroy(TEST_MESSAGE_SENDER_HANDLE));
    STRICT_EXPECTED_CALL(link_destroy(TEST_EVENT_SENDER_LINK_HANDLE));
}

static void set_expected_calls_for_amqp_messenger_stop(size_t wait_to_send_list_length, size_t in_progress_list_length, bool destroy_message_receiver)
{
    (void)wait_to_send_list_length;
    (void)in_progress_list_length;

    set_expected_calls_for_message_sender_destroy();

    if (destroy_message_receiver)
    {
        set_expected_calls_for_message_receiver_destroy();
    }

    STRICT_EXPECTED_CALL(message_queue_move_all_back_to_pending(TEST_MESSAGE_QUEUE_HANDLE));
}

static void set_expected_calls_for_on_message_send_complete()
{
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_message_do_work_send_pending_events(int number_of_events_pending, time_t current_time)
{
    int i;
    for (i = 0; i < number_of_events_pending; i++)
    {
        STRICT_EXPECTED_CALL(messagesender_send_async(TEST_MESSAGE_SENDER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(2).IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);

        EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
    }

}

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

static void set_expected_calls_for_process_event_send_timeouts(size_t in_progress_list_length, size_t send_event_timeout_secs, time_t current_time)
{
    if (in_progress_list_length <= 0)
    {
    }
    else
    {
        time_t send_time = add_seconds(current_time, -1 * (int)send_event_timeout_secs);


        for (; in_progress_list_length > 0; in_progress_list_length--)
        {
            STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
            EXPECTED_CALL(get_difftime(current_time, send_time)).SetReturn(difftime(current_time, send_time));
        }

    }
}

static void set_expected_calls_for_amqp_messenger_do_work(MESSENGER_DO_WORK_EXP_CALL_PROFILE *profile)
{
    if (profile->current_state == AMQP_MESSENGER_STATE_STARTING)
    {
        set_expected_calls_for_message_sender_create((const char* const**)&map_keys, (const char* const**)&map_values, 3);
    }
    else if (profile->current_state == AMQP_MESSENGER_STATE_STARTED)
    {
        if (profile->create_message_receiver)
        {
            set_expected_calls_for_message_receiver_create((const char* const**)&map_keys, (const char* const**)&map_values, 3);
        }
        else if (profile->destroy_message_receiver)
        {
            set_expected_calls_for_message_receiver_destroy();
        }

        STRICT_EXPECTED_CALL(message_queue_do_work(TEST_MESSAGE_QUEUE_HANDLE));
    }
}

static void set_detroy_configuration_expected_calls(bool testing_modules)
{
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    if (testing_modules == true)
    {
        STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_amqp_messenger_destroy(AMQP_MESSENGER_CONFIG* config, AMQP_MESSENGER_HANDLE messenger_handle, bool destroy_message_sender, bool destroy_message_receiver, size_t wait_to_send_list_length, size_t in_progress_list_length, bool testing_modules)
{
    (void)config;
    (void)destroy_message_sender;
    size_t i;

    set_expected_calls_for_amqp_messenger_stop(wait_to_send_list_length, in_progress_list_length, destroy_message_receiver);
    STRICT_EXPECTED_CALL(message_queue_destroy(TEST_MESSAGE_QUEUE_HANDLE));

    for (i = 0; i < wait_to_send_list_length; i++)
    {
        STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    }

    for (i = 0; i < in_progress_list_length; i++)
    {
        STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    }

    set_detroy_configuration_expected_calls(testing_modules);
    STRICT_EXPECTED_CALL(free(messenger_handle));
}

static char* TEST_messagereceiver_get_link_name_link_name;
static int TEST_messagereceiver_get_link_name_result;
static int TEST_messagereceiver_get_link_name(MESSAGE_RECEIVER_HANDLE message_receiver, const char** link_name)
{
    (void)message_receiver;
    *link_name = TEST_messagereceiver_get_link_name_link_name;
    return TEST_messagereceiver_get_link_name_result;
}

static void set_expected_calls_for_create_message_disposition_info()
{
    STRICT_EXPECTED_CALL(malloc(sizeof(AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO)));
    STRICT_EXPECTED_CALL(messagereceiver_get_received_message_id(TEST_MESSAGE_RECEIVER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .CopyOutArgumentBuffer(2, &TEST_DELIVERY_NUMBER, sizeof(delivery_number));

    STRICT_EXPECTED_CALL(messagereceiver_get_link_name(TEST_MESSAGE_RECEIVER_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    static char* link_name = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&link_name, sizeof(char*));
}

static void set_expected_calls_for_destroy_message_disposition_info()
{
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_on_message_received_internal_callback(AMQP_MESSENGER_DISPOSITION_RESULT disposition_result)
{
    TEST_on_new_message_received_callback_result = disposition_result;

    set_expected_calls_for_create_message_disposition_info();

    if (disposition_result == AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());
    }
    else if (disposition_result == AMQP_MESSENGER_DISPOSITION_RESULT_RELEASED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_released());
    }
    else if (disposition_result == AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_rejected("Rejected by application", "Rejected by application"));
    }
}

static void set_expected_calls_for_amqp_messenger_send_message_disposition(AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT disposition_result)
{
    AMQP_VALUE uamqp_disposition_result = NULL;

    if (disposition_result == AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());
        uamqp_disposition_result = TEST_MESSAGE_DISPOSITION_ACCEPTED_AMQP_VALUE;
    }
    else if (disposition_result == AMQP_MESSENGER_DISPOSITION_RESULT_RELEASED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_released());
        uamqp_disposition_result = TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE;
    }
    else if (disposition_result == AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_rejected("Rejected by application", "Rejected by application"));
        uamqp_disposition_result = TEST_MESSAGE_DISPOSITION_REJECTED_AMQP_VALUE;
    }

    STRICT_EXPECTED_CALL(messagereceiver_send_message_disposition(TEST_MESSAGE_RECEIVER_HANDLE, disposition_info->source, disposition_info->message_id, uamqp_disposition_result));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(uamqp_disposition_result));
}

static AMQP_MESSENGER_HANDLE create_and_start_messenger(AMQP_MESSENGER_CONFIG* config)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_create(config);
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_start(config, handle);
    (void)amqp_messenger_start(handle, TEST_SESSION_HANDLE);

    return handle;
}

static void crank_message_queue_do_work(size_t outgoing_messages_pending)
{
    (void)outgoing_messages_pending;
}

static void crank_amqp_messenger_do_work(AMQP_MESSENGER_HANDLE handle, MESSENGER_DO_WORK_EXP_CALL_PROFILE *profile)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_do_work(profile);
    (void)amqp_messenger_do_work(handle);

    if (profile->create_message_sender && saved_messagesender_create_on_message_sender_state_changed != NULL)
    {
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(profile->current_time);
        saved_messagesender_create_on_message_sender_state_changed(saved_messagesender_create_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
    }

    if (profile->create_message_receiver && saved_messagereceiver_create_on_message_receiver_state_changed != NULL)
    {
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(profile->current_time);
        saved_messagereceiver_create_on_message_receiver_state_changed(saved_messagereceiver_create_context, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_IDLE);
    }

    crank_message_queue_do_work(profile->outgoing_messages_pending);
}

static AMQP_MESSENGER_HANDLE create_and_start_messenger2(AMQP_MESSENGER_CONFIG* config, bool subscribe_for_messages)
{
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    time_t current_time = time(NULL);

    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTING, false, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    do_work_profile->create_message_sender = true;
    crank_amqp_messenger_do_work(handle, do_work_profile);

    if (subscribe_for_messages)
    {
        (void)amqp_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

        do_work_profile = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTED, true, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
        do_work_profile->create_message_receiver = true;
        crank_amqp_messenger_do_work(handle, do_work_profile);
    }

    return handle;
}

char* umock_stringify_AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO(const AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* value)
{
    (void)value;
    char* result = "AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO";
    return result;
}

int umock_are_equal_AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO(const AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* left, const AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* right)
{
    int result;

    if (((left == NULL) && (right != NULL)) || ((right == NULL) && (left != NULL)))
    {
        result = 0;
    }
    else if ((left == NULL) && (right == NULL))
    {
        result = 1;
    }
    else
    {
        if ((strcmp(left->source, right->source) != 0) || (left->message_id != right->message_id))
        {
            result = 0;
        }
        else
        {
            result = 1;
        }
    }

    return result;
}

int umock_copy_AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO(AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* destination, const AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* source)
{
    int result;

    if ((source == NULL) || (destination == NULL))
    {
        result = -1;
    }
    else
    {
        if (source->source == NULL)
        {
            destination->source = NULL;
        }
        else
        {
            (void)strcpy(destination->source, source->source);
        }
        destination->message_id = source->message_id;
        result = 0;
    }

    return result;
}

void umock_free_AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO(AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* value)
{
    //do nothing
    (void)value;
}

static void register_global_mock_returns()
{
    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_IOTHUB_HOST_FQDN);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_new, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_source, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(link_set_max_message_size, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_max_message_size, 1);

    REGISTER_GLOBAL_MOCK_RETURN(messagesender_create, TEST_MESSAGE_SENDER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(messagesender_open, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_open, 1);

    REGISTER_GLOBAL_MOCK_RETURN(messagesender_send_async, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_send_async, (ASYNC_OPERATION_HANDLE)0x64);

    REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_create, TEST_MESSAGE_RECEIVER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_open, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_open, 1);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_target, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_source, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_map, TEST_LINK_ATTACH_PROPERTIES);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_map, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_symbol, TEST_LINK_DEVICE_TYPE_NAME_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_symbol, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_string, TEST_LINK_DEVICE_TYPE_VALUE_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_set_map_value, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_set_map_value, 1);

    REGISTER_GLOBAL_MOCK_RETURN(link_set_attach_properties, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_attach_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(UniqueId_Generate, UNIQUEID_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UniqueId_Generate, UNIQUEID_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(link_set_rcv_settle_mode, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_rcv_settle_mode, 1);

    REGISTER_GLOBAL_MOCK_RETURN(messaging_delivery_accepted, TEST_MESSAGE_DISPOSITION_ACCEPTED_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_delivery_accepted, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(messaging_delivery_released, TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_delivery_released, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(messaging_delivery_rejected, TEST_MESSAGE_DISPOSITION_REJECTED_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_delivery_rejected, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(message_clone, TEST_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_clone, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(mallocAndStrcpy_s, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 1);

    REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_get_link_name, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_get_link_name, 1);

    REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_get_received_message_id, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_get_received_message_id, 1);

    REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_send_message_disposition, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_send_message_disposition, 1);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_Create, TEST_OPTIONHANDLER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_AddOption, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_AddOption, OPTIONHANDLER_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(message_queue_create, TEST_MESSAGE_QUEUE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_queue_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(message_queue_retrieve_options, TEST_OPTIONHANDLER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_queue_retrieve_options, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(message_queue_set_max_message_enqueued_time_secs, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_queue_set_max_message_enqueued_time_secs, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_queue_is_empty, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_queue_is_empty, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_queue_add, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_queue_add, 1);

    REGISTER_GLOBAL_MOCK_RETURN(Map_Clone, TEST_MAP_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Clone, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(Map_GetInternals, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetInternals, MAP_ERROR);
}

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(messagesender_create, TEST_messagesender_create);
    REGISTER_GLOBAL_MOCK_HOOK(messagesender_send_async, TEST_messagesender_send);
    REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_create, TEST_messagereceiver_create);
    REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_open, TEST_messagereceiver_open);
    REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_get_link_name, TEST_messagereceiver_get_link_name);
    REGISTER_GLOBAL_MOCK_HOOK(message_queue_create, TEST_message_queue_create);
    REGISTER_GLOBAL_MOCK_HOOK(message_queue_add, TEST_message_queue_add);
    REGISTER_GLOBAL_MOCK_HOOK(message_queue_destroy, TEST_message_queue_destroy);
}

static void register_mock_aliases()
{
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(UNIQUEID_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_SENDER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_RECEIVER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LINK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(role, bool);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_SENDER_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(fields, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_SEND_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVER_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(receiver_settle_mode, unsigned char);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_MESSENGER_SEND_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(pfCloneOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfDestroyOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfSetOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(delivery_number, int);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_QUEUE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MQ_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_PROCESSING_COMPLETED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
}

static void register_mock_value_types()
{
    REGISTER_UMOCK_VALUE_TYPE(AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO);
}

static void reset_test_data()
{
    size_t i;

    g_STRING_sprintf_call_count = 0;
    g_STRING_sprintf_fail_on_count = -1;
    saved_STRING_sprintf_handle = NULL;

    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));

    saved_messagesender_create_link = NULL;
    saved_messagesender_create_on_message_sender_state_changed = NULL;
    saved_messagesender_create_context = NULL;

    saved_message_create_from_iothub_message = NULL;
    TEST_message_create_from_iothub_message_return = 0;

    saved_messagesender_send_message_sender = NULL;
    saved_messagesender_send_message = NULL;
    saved_messagesender_send_on_message_send_complete = NULL;
    saved_messagesender_send_callback_context = NULL;

    saved_messagereceiver_create_link = NULL;
    saved_messagereceiver_create_on_message_receiver_state_changed = NULL;
    saved_messagereceiver_create_context = NULL;
    TEST_messagereceiver_create_result = TEST_MESSAGE_RECEIVER_HANDLE;

    saved_messagereceiver_open_message_receiver = NULL;
    saved_messagereceiver_open_on_message_received = NULL;
    saved_messagereceiver_open_callback_context = NULL;
    TEST_messagereceiver_open_result = 0;

    saved_on_new_message_received_callback_message = NULL;
    saved_on_new_message_received_callback_context = NULL;
    TEST_on_new_message_received_callback_result = AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED;

    TEST_link_set_max_message_size_result = 0;
    TEST_amqpvalue_set_map_value_result = 0;
    TEST_link_set_attach_properties_result = 0;

    TEST_on_event_send_complete_result = AMQP_MESSENGER_SEND_RESULT_SUCCESS;
    TEST_on_event_send_complete_reason = AMQP_MESSENGER_REASON_NONE;
    TEST_on_event_send_complete_context = NULL;

    TEST_DELIVERY_NUMBER = (delivery_number)1234;
    TEST_messagereceiver_get_link_name_link_name = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;

    for (i = 0; i < TEST_MESSAGE_QUEUE_ADD_BUFFERS_SIZE; i++)
    {
        TEST_message_queue_add_message_queue[i] = NULL;
        TEST_message_queue_add_message[i] = NULL;
        TEST_message_queue_add_on_message_processing_completed_callback[i] = NULL;
        TEST_message_queue_add_user_context[i] = NULL;
    }
    TEST_message_queue_add_count = 0;
    TEST_message_queue_add_return = 0;

    saved_on_state_changed_callback_context = NULL;
    saved_on_new_message_received_callback_disposition_info = NULL;

    TEST_on_message_processing_completed_callback_message = NULL;
    TEST_on_message_processing_completed_callback_reason = NULL;
}

BEGIN_TEST_SUITE(iothubtr_amqp_msgr_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    size_t i;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    int result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    register_mock_aliases();
    register_mock_value_types();
    register_global_mock_hooks();
    register_global_mock_returns();

    for (i = 0; i < 8; i++)
    {
        map_keys[i] = map_key + i;
        map_values[i] = map_value + i;
    }
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

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_001: [If `messenger_config` is NULL, amqp_messenger_create() shall return NULL]
TEST_FUNCTION(amqp_messenger_create_NULL_config)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, handle, NULL);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_002: [If `messenger_config`'s `device_id`, `iothub_host_fqdn`, `receive_link.source_suffix` or `send_link.target_suffix` are NULL, amqp_messenger_create() shall return NULL]
TEST_FUNCTION(amqp_messenger_create_config_NULL_device_id)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    config->device_id = NULL;

    umock_c_reset_all_calls();

    // act
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, handle, NULL);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_002: [If `messenger_config`'s `device_id`, `iothub_host_fqdn`, `receive_link.source_suffix` or `send_link.target_suffix` are NULL, amqp_messenger_create() shall return NULL]
TEST_FUNCTION(amqp_messenger_create_config_NULL_iothub_host_fqdn)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    config->iothub_host_fqdn = NULL;

    umock_c_reset_all_calls();

    // act
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, handle, NULL);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_003: [amqp_messenger_create() shall allocate memory for the messenger instance structure (aka `instance`)]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_005: [amqp_messenger_create() shall save a copy of `messenger_config` into `instance`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_007: [`instance->send_queue` shall be set using message_queue_create(), passing `on_process_message_callback`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_009: [If no failures occurr, amqp_messenger_create() shall return a handle to `instance`]
TEST_FUNCTION(amqp_messenger_create_success)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_create(config);

    // act
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_003: [amqp_messenger_create() shall allocate memory for the messenger instance structure (aka `instance`)]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_005: [amqp_messenger_create() shall save a copy of `messenger_config` into `instance`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_007: [`instance->send_queue` shall be set using message_queue_create(), passing `on_process_message_callback`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_009: [If no failures occurr, amqp_messenger_create() shall return a handle to `instance`]
TEST_FUNCTION(amqp_messenger_create_with_module_success)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config_with_module();

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_create(config);

    // act
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_004: [If malloc() fails, amqp_messenger_create() shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_006: [If the copy fails, amqp_messenger_create() shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_008: [If message_queue_create() fails, amqp_messenger_create() shall fail and return NULL]
TEST_FUNCTION(amqp_messenger_create_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_MESSENGER_CONFIG* config = get_messenger_config_with_module();

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_create(config);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 3)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(handle, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_051: [If `messenger_handle` or `session_handle` are NULL, amqp_messenger_start() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_start_NULL_messenger_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_start(NULL, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_051: [If `messenger_handle` or `session_handle` are NULL, amqp_messenger_start() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_start_NULL_session_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_start(TEST_AMQP_MESSENGER_HANDLE, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_052: [If `instance->state` is not AMQP_MESSENGER_STATE_STOPPED, amqp_messenger_start() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_start_messenger_not_stopped)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    // act
    int result = amqp_messenger_start(handle, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_053: [`session_handle` shall be saved on `instance->session_handle`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_054: [If no failures occurr, `instance->state` shall be set to AMQP_MESSENGER_STATE_STARTING, and `instance->on_state_changed_callback` invoked if provided]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_055: [If no failures occurr, amqp_messenger_start() shall return 0]
TEST_FUNCTION(amqp_messenger_start_succeeds)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_create(config);
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_start(config, handle);

    TEST_link_set_attach_properties_result = 1;

    // act
    int result = amqp_messenger_start(handle, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(int, AMQP_MESSENGER_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AMQP_MESSENGER_STATE_STARTING, saved_on_state_changed_callback_new_state);

    // cleanup
    amqp_messenger_destroy(handle);
}

static void messenger_state_on_event_sender_state_changed_callback_OPEN_impl(bool testing_modules)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = testing_modules ? get_messenger_config_with_module() : get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    time_t current_time = time(NULL);

    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTING, false, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    set_expected_calls_for_amqp_messenger_do_work(do_work_profile);
    amqp_messenger_do_work(handle);

    // act
    ASSERT_IS_NOT_NULL(saved_messagesender_create_on_message_sender_state_changed);

    saved_messagesender_create_on_message_sender_state_changed((void*)handle, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
    crank_amqp_messenger_do_work(handle, do_work_profile);

    // assert
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_previous_state, AMQP_MESSENGER_STATE_STARTING);
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_new_state, AMQP_MESSENGER_STATE_STARTED);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_088: [`new_state`, `previous_state` shall be saved into `instance->message_sender_previous_state` and `instance->message_sender_current_state`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_089: [`instance->last_message_sender_state_change_time` shall be set using get_time()]
TEST_FUNCTION(messenger_state_on_event_sender_state_changed_callback_OPEN)
{
    messenger_state_on_event_sender_state_changed_callback_OPEN_impl(false);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_088: [`new_state`, `previous_state` shall be saved into `instance->message_sender_previous_state` and `instance->message_sender_current_state`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_089: [`instance->last_message_sender_state_change_time` shall be set using get_time()]
TEST_FUNCTION(messenger_state_on_event_sender_state_changed_callback_with_module_OPEN)
{
    messenger_state_on_event_sender_state_changed_callback_OPEN_impl(true);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_088: [`new_state`, `previous_state` shall be saved into `instance->message_sender_previous_state` and `instance->message_sender_current_state`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_089: [`instance->last_message_sender_state_change_time` shall be set using get_time()]
TEST_FUNCTION(messenger_state_on_event_sender_state_changed_callback_ERROR)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTING, false, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    set_expected_calls_for_amqp_messenger_do_work(do_work_profile);
    amqp_messenger_do_work(handle);

    // act
    ASSERT_IS_NOT_NULL(saved_messagesender_create_on_message_sender_state_changed);

    saved_messagesender_create_on_message_sender_state_changed((void*)handle, MESSAGE_SENDER_STATE_ERROR, MESSAGE_SENDER_STATE_IDLE);
    crank_amqp_messenger_do_work(handle, do_work_profile);

    // assert
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_previous_state, AMQP_MESSENGER_STATE_STARTING);
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_new_state, AMQP_MESSENGER_STATE_ERROR);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_056: [If `messenger_handle` is NULL, amqp_messenger_stop() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_stop_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_stop(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_057: [If `instance->state` is AMQP_MESSENGER_STATE_STOPPED, amqp_messenger_stop() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_stop_messenger_not_started)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_create(config);
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_058: [`instance->state` shall be set to AMQP_MESSENGER_STATE_STOPPING, and `instance->on_state_changed_callback` invoked if provided]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_059: [`instance->message_sender` and `instance->message_receiver` shall be destroyed along with all its links]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_060: [`instance->send_queue` items shall be rolled back using message_queue_move_all_back_to_pending()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_062: [`instance->state` shall be set to AMQP_MESSENGER_STATE_STOPPED, and `instance->on_state_changed_callback` invoked if provided]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_063: [If no failures occurr, amqp_messenger_stop() shall return 0]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_090: [`instance->message_sender` shall be destroyed using messagesender_destroy()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_091: [`instance->sender_link` shall be destroyed using link_destroy()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_113: [`instance->message_receiver` shall be closed using messagereceiver_close()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_114: [If messagereceiver_close() fails, it shall be logged and ignored]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_115: [`instance->message_receiver` shall be destroyed using messagereceiver_destroy()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_116: [`instance->receiver_link` shall be destroyed using link_destroy()]
TEST_FUNCTION(amqp_messenger_stop_succeeds)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_stop(0, 0, true);

    // act
    int result = amqp_messenger_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_123: [If `messenger_handle` is NULL, amqp_messenger_destroy() shall fail and return]
TEST_FUNCTION(amqp_messenger_destroy_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    amqp_messenger_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

static void amqp_messenger_destroy_succeeds_impl(bool testing_modules)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = testing_modules ? get_messenger_config_with_module() : get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    ASSERT_ARE_EQUAL(int, 1, send_events(handle, 1));

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE* mdecp = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTED, true, true, 1, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    crank_amqp_messenger_do_work(handle, mdecp);

    ASSERT_ARE_EQUAL(int, 1, send_events(handle, 1));

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_destroy(config, handle, true, true, 1, 1, testing_modules);

    // act
    amqp_messenger_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_124: [If `instance->state` is not AMQP_MESSENGER_STATE_STOPPED, amqp_messenger_destroy() shall invoke amqp_messenger_stop()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_125: [message_queue_destroy() shall be invoked for `instance->send_queue`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_126: [amqp_messenger_destroy() shall release all the memory allocated for `instance`]
TEST_FUNCTION(amqp_messenger_destroy_succeeds)
{
    amqp_messenger_destroy_succeeds_impl(false);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_124: [If `instance->state` is not AMQP_MESSENGER_STATE_STOPPED, amqp_messenger_destroy() shall invoke amqp_messenger_stop()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_125: [message_queue_destroy() shall be invoked for `instance->send_queue`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_126: [amqp_messenger_destroy() shall release all the memory allocated for `instance`]
TEST_FUNCTION(amqp_messenger_destroy_with_module_succeeds)
{
    amqp_messenger_destroy_succeeds_impl(true);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_061: [If message_queue_move_all_back_to_pending() fails, amqp_messenger_stop() shall change the messenger state to AMQP_MESSENGER_STATE_ERROR and return a non-zero value]
TEST_FUNCTION(amqp_messenger_destroy_FAIL_TO_ROLLBACK_EVENTS)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE* mdecp = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTED, true, true, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    crank_amqp_messenger_do_work(handle, mdecp);

    umock_c_reset_all_calls();
    set_expected_calls_for_message_sender_destroy();
    set_expected_calls_for_message_receiver_destroy();
    STRICT_EXPECTED_CALL(message_queue_move_all_back_to_pending(TEST_MESSAGE_QUEUE_HANDLE)).SetReturn(1);

    // act
    amqp_messenger_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(int, AMQP_MESSENGER_STATE_STOPPING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, AMQP_MESSENGER_STATE_ERROR, saved_on_state_changed_callback_new_state);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_064: [If `messenger_handle` is NULL, amqp_messenger_do_work() shall fail and return]
TEST_FUNCTION(amqp_messenger_do_work_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    amqp_messenger_do_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(amqp_messenger_do_work_not_started)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_create(config);
    AMQP_MESSENGER_HANDLE handle = amqp_messenger_create(config);

    umock_c_reset_all_calls();

    // act
    amqp_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_messenger_destroy(handle);
}

static void amqp_messenger_do_work_send_events_success_impl(bool testing_modules)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = testing_modules ? get_messenger_config_with_module() : get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    set_expected_calls_for_amqp_messenger_send_async();
    int result = amqp_messenger_send_async(handle, TEST_MESSAGE_HANDLE, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTED, false, false, 1, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_do_work(do_work_profile);

    // act
    amqp_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_messenger_destroy(handle);

}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_065: [amqp_messenger_do_work() shall update the current state according to the states of message sender and receiver]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_066: [If the current state is AMQP_MESSENGER_STARTING, amqp_messenger_do_work() shall create and start `instance->message_sender`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_068: [If the message sender state changes to MESSAGE_SENDER_STATE_OPEN, amqp_messenger_do_work() shall set the state to AMQP_MESSENGER_STATE_STARTED]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_073: [amqp_messenger_do_work() shall invoke message_queue_do_work() on `instance->send_queue`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_075: [The AMQP link address shall be defined as "amqps://<`iothub_host_fqdn`>/devices/<`device_id`>/<`instance-config->send_link.source_suffix`>"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_076: [The AMQP link name shall be defined as "link-snd-<`device_id`>-<locally generated UUID>"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_077: [The AMQP link source shall be defined as "<link name>-source"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_078: [The AMQP link target shall be defined as <link address>]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_080: [The AMQP link shall have its ATTACH properties set using `instance->config->send_link.attach_properties`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_082: [The AMQP link maximum message size shall be set to UINT64_MAX using link_set_max_message_size()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_084: [`instance->message_sender` shall be created using messagesender_create(), passing the `instance->sender_link` and `on_message_sender_state_changed_callback`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_086: [`instance->message_sender` shall be opened using messagesender_open()]
TEST_FUNCTION(amqp_messenger_do_work_send_events_success)
{
    amqp_messenger_do_work_send_events_success_impl(false);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_065: [amqp_messenger_do_work() shall update the current state according to the states of message sender and receiver]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_066: [If the current state is AMQP_MESSENGER_STARTING, amqp_messenger_do_work() shall create and start `instance->message_sender`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_068: [If the message sender state changes to MESSAGE_SENDER_STATE_OPEN, amqp_messenger_do_work() shall set the state to AMQP_MESSENGER_STATE_STARTED]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_073: [amqp_messenger_do_work() shall invoke message_queue_do_work() on `instance->send_queue`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_075: [The AMQP link address shall be defined as "amqps://<`iothub_host_fqdn`>/devices/<`device_id`>/<`instance-config->send_link.source_suffix`>"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_076: [The AMQP link name shall be defined as "link-snd-<`device_id`>-<locally generated UUID>"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_077: [The AMQP link source shall be defined as "<link name>-source"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_078: [The AMQP link target shall be defined as <link address>]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_080: [The AMQP link shall have its ATTACH properties set using `instance->config->send_link.attach_properties`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_082: [The AMQP link maximum message size shall be set to UINT64_MAX using link_set_max_message_size()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_084: [`instance->message_sender` shall be created using messagesender_create(), passing the `instance->sender_link` and `on_message_sender_state_changed_callback`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_086: [`instance->message_sender` shall be opened using messagesender_open()]
TEST_FUNCTION(amqp_messenger_do_work_send_events_with_module_success)
{
    amqp_messenger_do_work_send_events_success_impl(true);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_092: [The AMQP link address shall be defined as "amqps://<`iothub_host_fqdn`>/devices/<`device_id`>/<`instance-config->receive_link.target_suffix`>"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_093: [The AMQP link name shall be defined as "link-rcv-<`device_id`>-<locally generated UUID>"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_094: [The AMQP link source shall be defined as <link address>]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_095: [The AMQP link target shall be defined as "<link name>-target"]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_097: [The AMQP link shall have its ATTACH properties set using `instance->config->receive_link.attach_properties`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_099: [The AMQP link maximum message size shall be set to UINT64_MAX using link_set_max_message_size()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_101: [`instance->message_receiver` shall be created using messagereceiver_create(), passing the `instance->receiver_link` and `on_message_receiver_state_changed_callback`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_103: [`instance->message_receiver` shall be opened using messagereceiver_open() passing `on_message_received_internal_callback`]
TEST_FUNCTION(amqp_messenger_do_work_create_message_receiver)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    (void)amqp_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTED, true, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_do_work(do_work_profile);

    // act
    amqp_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_070: [If the `instance->message_receiver` fails to be created/started, amqp_messenger_do_work() shall set the state to AMQP_MESSENGER_STATE_ERROR]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_096: [If the link fails to be created, amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_098: [If the AMQP link attach properties fail to be set, amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_100: [If link_set_max_message_size() fails, it shall be logged and ignored.]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_102: [If messagereceiver_create() fails, amqp_messenger_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_104: [If messagereceiver_open() fails, amqp_messenger_do_work() shall fail and return]
TEST_FUNCTION(amqp_messenger_do_work_create_message_receiver_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_MESSENGER_CONFIG* config = get_messenger_config_with_module();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    (void)amqp_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    umock_c_reset_all_calls();
    set_expected_calls_for_message_receiver_create((const char* const**)&map_keys, (const char* const**)&map_values, 3);
    umock_c_negative_tests_snapshot();

    saved_messagereceiver_open_on_message_received = NULL;

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 1 || i == 2 || i == 4 || i == 5 || i == 9 || i == 11 || i == 12 ||
            i == 14 || i == 16 || i == 19 || (i >= 20 && i <= 27) || (i >= 30 && i <= 35))
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        amqp_messenger_do_work(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_TRUE(saved_messagereceiver_open_on_message_received == NULL, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_067: [If the `instance->message_sender` fails to be created/started, amqp_messenger_do_work() shall set the state to AMQP_MESSENGER_STATE_ERROR]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_079: [If the link fails to be created, amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_081: [If the AMQP link attach properties fail to be set, amqp_messenger_do_work() shall change the state to AMQP_MESSENGER_STATE_ERROR]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_083: [If link_set_max_message_size() fails, it shall be logged and ignored.]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_085: [If messagesender_create() fails, amqp_messenger_do_work() shall fail and return]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_087: [If messagesender_open() fails, amqp_messenger_do_work() shall fail and return]
TEST_FUNCTION(amqp_messenger_do_work_create_message_sender_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_MESSENGER_CONFIG* config = get_messenger_config();

    // act
    size_t i;
    size_t n = 100;
    for (i = 0; i < n; i++)
    {
        if (i == 4 || i == 6 || i == 7 || i == 8 || i == 11 || i == 12 || i == 14 || i == 20 || i == 21 || i == 25 || i == 26 || i == 30 || i == 31 || i == 33 || i >= 34)
        {
            continue; // These expected calls do not cause the API to fail.
        }

        // arrange
        set_expected_calls_for_amqp_messenger_create(config);
        AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

        if (n == 100) // So we do it only once.
        {
            umock_c_reset_all_calls();
            set_expected_calls_for_message_sender_create((const char* const**)&map_keys, (const char* const**)&map_values, 3);
            umock_c_negative_tests_snapshot();
            n = umock_c_negative_tests_call_count();
        }

        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        amqp_messenger_do_work(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);

        ASSERT_ARE_EQUAL(int, AMQP_MESSENGER_STATE_STARTING, saved_on_state_changed_callback_previous_state, error_msg);
        ASSERT_ARE_EQUAL(int, AMQP_MESSENGER_STATE_ERROR, saved_on_state_changed_callback_new_state, error_msg);

        // cleanup
        amqp_messenger_destroy(handle);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(messenger_state_on_message_receiver_state_changed_callback_ERROR)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(time(NULL));

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_create_on_message_receiver_state_changed);

    saved_messagereceiver_create_on_message_receiver_state_changed(saved_messagereceiver_create_context, MESSAGE_RECEIVER_STATE_ERROR, MESSAGE_RECEIVER_STATE_OPEN);

    umock_c_reset_all_calls();
    amqp_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_previous_state, AMQP_MESSENGER_STATE_STARTED);
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_new_state, AMQP_MESSENGER_STATE_ERROR);

    // cleanup
    amqp_messenger_destroy(handle);
}

TEST_FUNCTION(amqp_messenger_do_work_destroy_message_receiver)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    (void)amqp_messenger_unsubscribe_for_messages(handle);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTED, false, true, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_do_work(do_work_profile);

    // act
    amqp_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_035: [If `messenger_handle` or `on_message_received_callback` are NULL, amqp_messenger_subscribe_for_messages() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_subscribe_for_messages_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_subscribe_for_messages(NULL, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_035: [If `messenger_handle` or `on_message_received_callback` are NULL, amqp_messenger_subscribe_for_messages() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_subscribe_for_messages_NULL_callback)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_subscribe_for_messages(handle, NULL, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_036: [`on_message_received_callback` and `context` shall be saved on `instance->on_message_received_callback`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_037: [amqp_messenger_subscribe_for_messages() shall set `instance->receive_messages` to true]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_038: [If no failures occurr, amqp_messenger_subscribe_for_messages() shall return 0]
TEST_FUNCTION(amqp_messenger_subscribe_for_messages_already_subscribed)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    (void)amqp_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);
    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_039: [If `messenger_handle` is NULL, amqp_messenger_unsubscribe_for_messages() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_unsubscribe_for_messages_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_unsubscribe_for_messages(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_042: [If no failures occurr, amqp_messenger_unsubscribe_for_messages() shall return 0]
TEST_FUNCTION(amqp_messenger_unsubscribe_for_messages_not_subscribed)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_unsubscribe_for_messages(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
    amqp_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_ACCEPTED)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_message_received_internal_callback(AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_ACCEPTED_AMQP_VALUE);

    // cleanup
    amqp_messenger_destroy_disposition_info(saved_on_new_message_received_callback_disposition_info);
    amqp_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_RELEASED)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_message_received_internal_callback(AMQP_MESSENGER_DISPOSITION_RESULT_RELEASED);

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE);

    // cleanup
    amqp_messenger_destroy_disposition_info(saved_on_new_message_received_callback_disposition_info);
    amqp_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_REJECTED)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_message_received_internal_callback(AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED);

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_REJECTED_AMQP_VALUE);

    // cleanup
    amqp_messenger_destroy_disposition_info(saved_on_new_message_received_callback_disposition_info);
    amqp_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_create_AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO_fails)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    TEST_on_new_message_received_callback_result = AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED;
    STRICT_EXPECTED_CALL(malloc(sizeof(AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO))).SetReturn(NULL);
    STRICT_EXPECTED_CALL(messaging_delivery_released());

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE);

    // cleanup
    amqp_messenger_destroy(handle);
}


static void amqp_messenger_unsubscribe_for_messages_success_impl(bool testing_modules)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = testing_modules ? get_messenger_config_with_module() : get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();

    // act
    int unsubscription_result = amqp_messenger_unsubscribe_for_messages(handle);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(AMQP_MESSENGER_STATE_STARTED, false, true, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    crank_amqp_messenger_do_work(handle, do_work_profile);

    // assert
    ASSERT_ARE_EQUAL(int, unsubscription_result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_040: [`instance->receive_messages` shall be saved to false]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_041: [`instance->on_message_received_callback` and `instance->on_message_received_context` shall be set to NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_042: [If no failures occurr, amqp_messenger_unsubscribe_for_messages() shall return 0]
TEST_FUNCTION(amqp_messenger_unsubscribe_for_messages_success)
{
    amqp_messenger_unsubscribe_for_messages_success_impl(false);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_040: [`instance->receive_messages` shall be saved to false]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_041: [`instance->on_message_received_callback` and `instance->on_message_received_context` shall be set to NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_042: [If no failures occurr, amqp_messenger_unsubscribe_for_messages() shall return 0]
TEST_FUNCTION(amqp_messenger_unsubscribe_for_messages_with_module_success)
{
    amqp_messenger_unsubscribe_for_messages_success_impl(true);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_010: [If `messenger_handle`, `message` or `on_event_send_complete_callback` are NULL, amqp_messenger_send_async() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_send_async_NULL_handle)
{
    // arrange

    // act
    int result = amqp_messenger_send_async(NULL, TEST_MESSAGE_HANDLE, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_010: [If `messenger_handle`, `message` or `on_event_send_complete_callback` are NULL, amqp_messenger_send_async() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_send_async_NULL_message)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    // act
    int result = amqp_messenger_send_async(handle, NULL, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_010: [If `messenger_handle`, `message` or `on_event_send_complete_callback` are NULL, amqp_messenger_send_async() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_send_async_NULL_callback)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    // act
    int result = amqp_messenger_send_async(handle, TEST_MESSAGE_HANDLE, NULL, TEST_IOTHUB_CLIENT_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_012: [If message_clone() fails, amqp_messenger_send_async() shall fail and return a non-zero value]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_014: [If malloc() fails, amqp_messenger_send_async() shall fail and return a non-zero value]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_017: [If message_queue_add() fails, amqp_messenger_send_async() shall fail and return a non-zero value]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_018: [If any failure occurs, amqp_messenger_send_async() shall free any memory it has allocated]
TEST_FUNCTION(amqp_messenger_send_async_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_MESSENGER_CONFIG* config = get_messenger_config_with_module();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_send_async();
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        int result = amqp_messenger_send_async(handle, TEST_MESSAGE_HANDLE, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_029: [If `messenger_handle` or `send_status` are NULL, amqp_messenger_get_send_status() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_get_send_status_NULL_handle)
{
    // act
    AMQP_MESSENGER_SEND_STATUS send_status;
    int result = amqp_messenger_get_send_status(NULL, &send_status);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_029: [If `messenger_handle` or `send_status` are NULL, amqp_messenger_get_send_status() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_get_send_status_NULL_send_status)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    // act
    int result = amqp_messenger_get_send_status(handle, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_031: [If message_queue_is_empty() fails, amqp_messenger_get_send_status() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_get_send_status_failure_checks)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(message_queue_is_empty(TEST_MESSAGE_QUEUE_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    AMQP_MESSENGER_SEND_STATUS send_status;
    int result = amqp_messenger_get_send_status(handle, &send_status);


    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_030: [message_queue_is_empty() shall be invoked for `instance->send_queue`]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_032: [If message_queue_is_empty() returns `true`, `send_status` shall be set to AMQP_MESSENGER_SEND_STATUS_IDLE]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_034: [If no failures occur, amqp_messenger_get_send_status() shall return 0]
TEST_FUNCTION(amqp_messenger_get_send_status_IDLE_succeeds)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);
    bool is_empty = true;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(message_queue_is_empty(TEST_MESSAGE_QUEUE_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &is_empty, sizeof(bool));

    // act
    AMQP_MESSENGER_SEND_STATUS send_status;
    int result = amqp_messenger_get_send_status(handle, &send_status);


    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, AMQP_MESSENGER_SEND_STATUS_IDLE, send_status);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_033: [Otherwise, `send_status` shall be set to AMQP_MESSENGER_SEND_STATUS_BUSY]
TEST_FUNCTION(amqp_messenger_get_send_status_BUSY_succeeds)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);
    bool is_empty = false;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(message_queue_is_empty(TEST_MESSAGE_QUEUE_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &is_empty, sizeof(bool));

    // act
    AMQP_MESSENGER_SEND_STATUS send_status;
    int result = amqp_messenger_get_send_status(handle, &send_status);


    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, AMQP_MESSENGER_SEND_STATUS_BUSY, send_status);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_127: [If `messenger_handle` or `name` or `value` is NULL, amqp_messenger_set_option shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_set_option_NULL_handle)
{
    // arrange
    size_t value = 100;

    // act
    int result = amqp_messenger_set_option(NULL, AMQP_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_127: [If `messenger_handle` or `name` or `value` is NULL, amqp_messenger_set_option shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_set_option_NULL_name)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    size_t value = 100;

    // act
    int result = amqp_messenger_set_option(handle, NULL, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_127: [If `messenger_handle` or `name` or `value` is NULL, amqp_messenger_set_option shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_set_option_NULL_value)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    // act
    int result = amqp_messenger_set_option(handle, AMQP_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_128: [If name matches AMQP_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, `value` shall be set on `instance->send_queue` using message_queue_set_max_message_enqueued_time_secs()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_131: [If no errors occur, amqp_messenger_set_option shall return 0]
TEST_FUNCTION(amqp_messenger_set_option_EVENT_SEND_TIMEOUT_SECS)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    size_t value = 100;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(message_queue_set_max_message_enqueued_time_secs(TEST_MESSAGE_QUEUE_HANDLE, value));

    // act
    int result = amqp_messenger_set_option(handle, AMQP_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_130: [If `name` does not match any supported option, amqp_messenger_set_option() shall fail and return a non-zero value]
TEST_FUNCTION(amqp_messenger_set_option_name_not_supported)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    size_t value = 100;

    // act
    int result = amqp_messenger_set_option(handle, "Bernie Sanders Forever!", &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_043: [If `messenger_handle` or `disposition_info` are NULL, amqp_messenger_send_message_disposition() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_send_message_disposition_NULL_messenger_handle)
{
    // arrange
    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    // act
    int result = amqp_messenger_send_message_disposition(NULL, &disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_043: [If `messenger_handle` or `disposition_info` are NULL, amqp_messenger_send_message_disposition() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_send_message_disposition_NULL_disposition_info)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    // act
    int result = amqp_messenger_send_message_disposition(handle, NULL, AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_044: [If `disposition_info->source` is NULL, amqp_messenger_send_message_disposition() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_send_message_disposition_NULL_source)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = NULL;

    // act
    int result = amqp_messenger_send_message_disposition(handle, &disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_046: [An AMQP_VALUE disposition result shall be created corresponding to the `disposition_result` provided]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_047: [`messagereceiver_send_message_disposition()` shall be invoked passing `disposition_info->source`, `disposition_info->message_id` and the AMQP_VALUE disposition result]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_049: [amqp_messenger_send_message_disposition() shall destroy the AMQP_VALUE disposition result]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_050: [If no failures occurr, amqp_messenger_send_message_disposition() shall return 0]
TEST_FUNCTION(amqp_messenger_send_message_disposition_ACCEPTED_succeeds)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_send_message_disposition(&disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // act
    int result = amqp_messenger_send_message_disposition(handle, &disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_046: [An AMQP_VALUE disposition result shall be created corresponding to the `disposition_result` provided]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_047: [`messagereceiver_send_message_disposition()` shall be invoked passing `disposition_info->source`, `disposition_info->message_id` and the AMQP_VALUE disposition result]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_049: [amqp_messenger_send_message_disposition() shall destroy the AMQP_VALUE disposition result]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_050: [If no failures occurr, amqp_messenger_send_message_disposition() shall return 0]
TEST_FUNCTION(amqp_messenger_send_message_disposition_RELEASED_succeeds)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_send_message_disposition(&disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_RELEASED);

    // act
    int result = amqp_messenger_send_message_disposition(handle, &disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_RELEASED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

TEST_FUNCTION(amqp_messenger_send_message_disposition_NOT_SUBSCRIBED)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false); // this "false" is what arranges this test

    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    umock_c_reset_all_calls();

    // act
    int result = amqp_messenger_send_message_disposition(handle, &disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_048: [If `messagereceiver_send_message_disposition()` fails, amqp_messenger_send_message_disposition() shall fail and return non-zero value]
TEST_FUNCTION(amqp_messenger_send_message_disposition_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_MESSENGER_CONFIG* config = get_messenger_config_with_module();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_send_message_disposition(&disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);
    umock_c_negative_tests_snapshot();

    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 2 || i == 3 || i == 4)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        int result = amqp_messenger_send_message_disposition(handle, &disposition_info, AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }


    // cleanup
    amqp_messenger_destroy(handle);
    umock_c_negative_tests_deinit();
}

static void set_expected_calls_for_amqp_messenger_retrieve_options()
{
    EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_queue_retrieve_options(TEST_MESSAGE_QUEUE_HANDLE));
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_132: [If `messenger_handle` is NULL, amqp_messenger_retrieve_options shall fail and return NULL]
TEST_FUNCTION(amqp_messenger_retrieve_options_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    OPTIONHANDLER_HANDLE result = amqp_messenger_retrieve_options(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_133: [An OPTIONHANDLER_HANDLE instance shall be created using OptionHandler_Create]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_135: [`instance->send_queue` options shall be retrieved using message_queue_retrieve_options()]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_137: [Each option of `instance` shall be added to the OPTIONHANDLER_HANDLE instance using OptionHandler_AddOption]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_140: [If no failures occur, amqp_messenger_retrieve_options shall return the OPTIONHANDLER_HANDLE instance]
TEST_FUNCTION(amqp_messenger_retrieve_options_succeeds)
{
    // arrange
    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_retrieve_options();

    // act
    OPTIONHANDLER_HANDLE result = amqp_messenger_retrieve_options(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_OPTIONHANDLER_HANDLE, result);

    // cleanup
    amqp_messenger_destroy(handle);
}

// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_134: [If an OPTIONHANDLER_HANDLE instance fails to be created, amqp_messenger_retrieve_options shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_136: [If message_queue_retrieve_options() fails, amqp_messenger_retrieve_options shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_138: [If OptionHandler_AddOption fails, amqp_messenger_retrieve_options shall fail and return NULL]
// Tests_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_139: [If amqp_messenger_retrieve_options fails, any allocated memory shall be freed]
TEST_FUNCTION(amqp_messenger_retrieve_options_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    AMQP_MESSENGER_CONFIG* config = get_messenger_config();
    AMQP_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_amqp_messenger_retrieve_options();
    umock_c_negative_tests_snapshot();

    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        OPTIONHANDLER_HANDLE result = amqp_messenger_retrieve_options(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(result, error_msg);
    }

    // cleanup
    amqp_messenger_destroy(handle);
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(iothubtr_amqp_msgr_ut)
