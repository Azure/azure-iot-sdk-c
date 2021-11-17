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

#define ENABLE_MOCKS

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "azure_c_shared_utility/agenttime.h"

#undef ENABLE_MOCK_FILTERING_SWITCH
#define ENABLE_MOCK_FILTERING

#define please_mock_amqpvalue_create_map MOCK_ENABLED
#define please_mock_amqpvalue_create_string MOCK_ENABLED
#define please_mock_amqpvalue_create_symbol MOCK_ENABLED
#define please_mock_amqpvalue_destroy MOCK_ENABLED
#define please_mock_amqpvalue_set_map_value MOCK_ENABLED
#define please_mock_link_create MOCK_ENABLED
#define please_mock_link_destroy MOCK_ENABLED
#define please_mock_link_get_peer_max_message_size MOCK_ENABLED
#define please_mock_link_set_attach_properties MOCK_ENABLED
#define please_mock_link_set_max_message_size MOCK_ENABLED
#define please_mock_link_set_rcv_settle_mode MOCK_ENABLED
#define please_mock_message_add_body_amqp_data MOCK_ENABLED
#define please_mock_message_create MOCK_ENABLED
#define please_mock_message_destroy MOCK_ENABLED
#define please_mock_message_set_message_format MOCK_ENABLED
#define please_mock_messagereceiver_close MOCK_ENABLED
#define please_mock_messagereceiver_create MOCK_ENABLED
#define please_mock_messagereceiver_destroy MOCK_ENABLED
#define please_mock_messagereceiver_get_link_name MOCK_ENABLED
#define please_mock_messagereceiver_get_received_message_id MOCK_ENABLED
#define please_mock_messagereceiver_open MOCK_ENABLED
#define please_mock_messagereceiver_send_message_disposition MOCK_ENABLED
#define please_mock_messagesender_create MOCK_ENABLED
#define please_mock_messagesender_destroy MOCK_ENABLED
#define please_mock_messagesender_open MOCK_ENABLED
#define please_mock_messagesender_send_async MOCK_ENABLED
#define please_mock_messaging_create_source MOCK_ENABLED
#define please_mock_messaging_create_target MOCK_ENABLED
#define please_mock_messaging_delivery_accepted MOCK_ENABLED
#define please_mock_messaging_delivery_rejected MOCK_ENABLED
#define please_mock_messaging_delivery_released MOCK_ENABLED

#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/message_receiver.h"

#undef ENABLE_MOCK_FILTERING_SWITCH
#undef ENABLE_MOCK_FILTERING

#include "internal/iothub_client_private.h"
#include "iothub_client_version.h"
#include "internal/uamqp_messaging.h"

#undef ENABLE_MOCKS

#include "internal/iothubtransport_amqp_telemetry_messenger.h"


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
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

#define DEFAULT_EVENT_SEND_RETRY_LIMIT                    10
#define DEFAULT_EVENT_SEND_TIMEOUT_SECS 600

#define UNIQUE_ID_BUFFER_SIZE                             37
#define TEST_UNIQUE_ID                                    "A1234DE234A1234DE234A1234DE234A1234DEA1234DE234A1234DE234A1234DE234A1234DEA1234DE234A1234DE234A1234DE234A1234DE"
#define TEST_CLIENT_VERSION_STR                           "client x (version y)"

#define TEST_DEVICE_ID                                    "my_device"
#define TEST_DEVICE_ID_STRING_HANDLE                      (STRING_HANDLE)0x4442
#define TEST_MODULE_ID                                    "my_module"
#define TEST_MODULE_ID_STRING_HANDLE                      (STRING_HANDLE)0x4489
#define TEST_IOTHUB_HOST_FQDN_STRING_HANDLE               (STRING_HANDLE)0x4443
#define TEST_IOTHUB_HOST_FQDN                             "some.fqdn.com"
#define TEST_ON_STATE_CHANGED_CB_CONTEXT                  (void*)0x4445
#define TEST_STRING_HANDLE                                (STRING_HANDLE)0x4446
#define TEST_SESSION_HANDLE                               (SESSION_HANDLE)0x4447
#define TEST_TELEMETRY_MESSENGER_HANDLE                   (TELEMETRY_MESSENGER_HANDLE)0x4448
#define TEST_DEVICES_PATH_STRING_HANDLE                   (STRING_HANDLE)0x4449
#define TEST_DEVICES_PATH_CHAR_PTR                        "iothub.azure-devices.net/devices/some-device-id"
#define TEST_EVENT_SEND_ADDRESS_STRING_HANDLE             (STRING_HANDLE)0x4450
#define TEST_EVENT_SEND_ADDRESS_CHAR_PTR                  "amqps:/iothub.azure-devices.net/devices/some-device-id/messages/events"

#define TEST_MESSAGE_RECEIVE_ADDRESS_STRING_HANDLE        (STRING_HANDLE)0x4451
#define TEST_MESSAGE_RECEIVE_ADDRESS_CHAR_PTR             "amqps://iothub.azure-devices.net/devices/some-device-id/messages/devicebound"

#define TEST_EVENT_SENDER_SOURCE_NAME_STRING_HANDLE       (STRING_HANDLE)0x4452
#define TEST_EVENT_SENDER_SOURCE_NAME_CHAR_PTR            "event_sender_source_name"
#define TEST_MESSAGE_RECEIVER_TARGET_NAME_STRING_HANDLE   (STRING_HANDLE)0x4453
#define TEST_MESSAGE_RECEIVER_TARGET_NAME_CHAR_PTR        "message_receiver_target_name"

#define TEST_EVENT_SENDER_SOURCE_AMQP_VALUE               (AMQP_VALUE)0x4454
#define TEST_EVENT_SENDER_TARGET_AMQP_VALUE               (AMQP_VALUE)0x4455

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

#define TEST_IOTHUB_MESSAGE_HANDLE                        (IOTHUB_MESSAGE_HANDLE)0x4466
#define TEST_MESSAGE_HANDLE                               (MESSAGE_HANDLE)0x4467
#define TEST_MESSAGE_RECEIVER_SOURCE_AMQP_VALUE           (AMQP_VALUE)0x4468
#define TEST_MESSAGE_RECEIVER_TARGET_AMQP_VALUE           (AMQP_VALUE)0x4469
#define TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT           (void*)0x4470
#define TEST_MESSAGE_DISPOSITION_ACCEPTED_AMQP_VALUE      (AMQP_VALUE)0x4471
#define TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE      (AMQP_VALUE)0x4472
#define TEST_MESSAGE_DISPOSITION_REJECTED_AMQP_VALUE      (AMQP_VALUE)0x4473
#define TEST_SINGLYLINKEDLIST_HANDLE                      (SINGLYLINKEDLIST_HANDLE)0x4476
#define TEST_LIST_ITEM_HANDLE                             (LIST_ITEM_HANDLE)0x4477
#define TEST_SEND_EVENT_TASK                              (const void*)0x4478
#define TEST_IOTHUB_CLIENT_HANDLE                         (void*)0x4479
static IOTHUB_MESSAGE_LIST* TEST_IOTHUB_MESSAGE_LIST_HANDLE;
static SINGLYLINKEDLIST_HANDLE TEST_WAIT_TO_SEND_LIST;
static SINGLYLINKEDLIST_HANDLE TEST_IN_PROGRESS_LIST;
#define TEST_WAIT_TO_SEND_LIST1                           (SINGLYLINKEDLIST_HANDLE)0x4481
#define TEST_WAIT_TO_SEND_LIST2                           (SINGLYLINKEDLIST_HANDLE)0x4482
#define TEST_IN_PROGRESS_LIST1                            (SINGLYLINKEDLIST_HANDLE)0x4483
#define TEST_IN_PROGRESS_LIST2                            (SINGLYLINKEDLIST_HANDLE)0x4484
#define TEST_OPTIONHANDLER_HANDLE                         (OPTIONHANDLER_HANDLE)0x4485
#define TEST_CALLBACK_LIST1                               (SINGLYLINKEDLIST_HANDLE)0x4486
#define INDEFINITE_TIME                                   ((time_t)-1)
#define TEST_DISPOSITION_AMQP_VALUE                       (AMQP_VALUE)0x4487

static delivery_number TEST_DELIVERY_NUMBER;

// Expected behavior on sending SendEvent's of various lengths
typedef enum SEND_PENDING_EXPECTED_ACTION_TAG
{
    SEND_PENDING_EXPECT_ADD,
    SEND_PENDING_EXPECT_ROLLOVER,
    SEND_PENDING_EXPECT_ERROR_TOO_LARGE,
    SEND_PENDING_EXPECT_CREATE_MESSAGE_FAILURE
}
SEND_PENDING_EXPECTED_ACTION;

// Simulates sending events, namely bytes encoded and what we expect to happen based on current state.
typedef struct SEND_PENDING_TEST_EVENTS_TAG
{
    int number_bytes_encoded;
    SEND_PENDING_EXPECTED_ACTION expected_action;
} SEND_PENDING_TEST_EVENTS;

// Stores information from callbacks
typedef struct TEST_ON_SEND_COMPLETE_DATA_TAG
{
    IOTHUB_MESSAGE_LIST* message;
    TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT result;
    void* context;
} TEST_ON_SEND_COMPLETE_DATA;

// Configuration for a SendEvent UT.
typedef struct SEND_PENDING_EVENTS_TEST_CONFIG_TAG
{
    uint64_t                    peer_max_message_size;
    SEND_PENDING_TEST_EVENTS*   test_events;
    int                         number_test_events;
    bool                        send_outside_working_loop;
    TEST_ON_SEND_COMPLETE_DATA* expected_on_send_complete_data;
    int                         number_expected_on_send_complete_data;
} SEND_PENDING_EVENTS_TEST_CONFIG;


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

static const char* test_get_product_info(void* ctx)
{
    (void)ctx;
    return TEST_CLIENT_VERSION_STR;
}

static int saved_wait_to_send_list_count;
static const void* saved_wait_to_send_list[20];

static int saved_wait_to_send_list_count2;
static const void* saved_wait_to_send_list2[20];

static int saved_in_progress_list_count;
static const void* saved_in_progress_list[20];

static int saved_in_progress_list_count2;
static const void* saved_in_progress_list2[20];

static int saved_callback_list_count1;
static const void* saved_callback_list1[20];

static void* saved_on_state_changed_callback_context;
static TELEMETRY_MESSENGER_STATE saved_on_state_changed_callback_previous_state;
static TELEMETRY_MESSENGER_STATE saved_on_state_changed_callback_new_state;

static void TEST_on_state_changed_callback(void* context, TELEMETRY_MESSENGER_STATE previous_state, TELEMETRY_MESSENGER_STATE new_state)
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


static IOTHUB_MESSAGE_HANDLE saved_on_new_message_received_callback_message;
static void* saved_on_new_message_received_callback_context;
static TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* saved_on_new_message_received_callback_disposition_info;
static TELEMETRY_MESSENGER_DISPOSITION_RESULT TEST_on_new_message_received_callback_result;
static TELEMETRY_MESSENGER_DISPOSITION_RESULT TEST_on_new_message_received_callback(IOTHUB_MESSAGE_HANDLE message, TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
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


static TELEMETRY_MESSENGER_CONFIG g_messenger_config;

static TELEMETRY_MESSENGER_CONFIG* get_messenger_config()
{
    memset(&g_messenger_config, 0, sizeof(TELEMETRY_MESSENGER_CONFIG));

    g_messenger_config.device_id = TEST_DEVICE_ID;
    g_messenger_config.iothub_host_fqdn = TEST_IOTHUB_HOST_FQDN;
    g_messenger_config.on_state_changed_callback = TEST_on_state_changed_callback;
    g_messenger_config.on_state_changed_context = TEST_ON_STATE_CHANGED_CB_CONTEXT;

    return &g_messenger_config;
}

typedef struct MESSENGER_DO_WORK_EXP_CALL_PROFILE_STRUCT
{
    TELEMETRY_MESSENGER_STATE current_state;
    bool create_message_sender;
    bool destroy_message_sender;
    bool create_message_receiver;
    bool destroy_message_receiver;
    int wait_to_send_list_length;
    int in_progress_list_length;
    size_t send_event_timeout_secs;
    time_t current_time;
    SEND_PENDING_EVENTS_TEST_CONFIG *send_pending_events_test_config;
    bool testing_modules;
} MESSENGER_DO_WORK_EXP_CALL_PROFILE;


//
//  Tests zero messages available in queue
//
static SEND_PENDING_EVENTS_TEST_CONFIG test_send_zero_message_config = {
    100,
    NULL,
    0,
    true,
    NULL,
    0,
};


//
//  Tests sending exactly one message which is nowhere close to rollover logic
//
static SEND_PENDING_TEST_EVENTS test_send_one_message_events[] = {
    { 10, SEND_PENDING_EXPECT_ADD }
};

static TEST_ON_SEND_COMPLETE_DATA test_send_one_message_expected_callbacks[] = {
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE }
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_one_message_config = {
    100,
    test_send_one_message_events,
    COUNT_OF(test_send_one_message_events),
    true,
    test_send_one_message_expected_callbacks,
    COUNT_OF(test_send_one_message_expected_callbacks),
};

static MESSENGER_DO_WORK_EXP_CALL_PROFILE g_do_work_profile;

static MESSENGER_DO_WORK_EXP_CALL_PROFILE* get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE current_state, bool is_subscribed_for_messages, bool is_msg_rcvr_created, int wts_list_length, int ip_list_length, time_t current_time, size_t event_send_timeout_secs)
{
    memset(&g_do_work_profile, 0, sizeof(MESSENGER_DO_WORK_EXP_CALL_PROFILE));
    g_do_work_profile.current_state = current_state;
    g_do_work_profile.wait_to_send_list_length = wts_list_length;
    g_do_work_profile.in_progress_list_length = ip_list_length;
    g_do_work_profile.current_time = current_time;
    g_do_work_profile.send_event_timeout_secs = event_send_timeout_secs;
    g_do_work_profile.testing_modules = false;

    if (g_do_work_profile.current_state == TELEMETRY_MESSENGER_STATE_STARTING)
    {
        g_do_work_profile.create_message_sender = true;
        g_do_work_profile.send_pending_events_test_config = NULL;
    }
    else if (g_do_work_profile.current_state == TELEMETRY_MESSENGER_STATE_STOPPING)
    {
        g_do_work_profile.destroy_message_sender = true;

        if (is_msg_rcvr_created)
        {
            g_do_work_profile.destroy_message_receiver = true;
        }

        g_do_work_profile.send_pending_events_test_config = NULL;
    }
    else if (g_do_work_profile.current_state == TELEMETRY_MESSENGER_STATE_STARTED)
    {
        if (is_subscribed_for_messages && !is_msg_rcvr_created)
        {
            g_do_work_profile.create_message_receiver = true;
        }
        else if (!is_subscribed_for_messages && is_msg_rcvr_created)
        {
            g_do_work_profile.destroy_message_receiver = true;
        }

        if (wts_list_length >= 1)
        {
            g_do_work_profile.send_pending_events_test_config = &test_send_one_message_config;
        }
        else
        {
            g_do_work_profile.send_pending_events_test_config = &test_send_zero_message_config;
        }
    }

    return &g_do_work_profile;
}

static int TEST_message_create_uamqp_encoding_from_iothub_message(MESSAGE_HANDLE message_batch_container, IOTHUB_MESSAGE_HANDLE message_handle, BINARY_DATA* body_binary_data)
{
    (void)message_batch_container;
    (void)message_handle;
    (void)body_binary_data;
    return 0;
}


static MESSAGE_HANDLE saved_message_create_IoTHubMessage_from_uamqp_message_uamqp_message;
static int TEST_message_create_IoTHubMessage_from_uamqp_message_return;
static int TEST_message_create_IoTHubMessage_from_uamqp_message(MESSAGE_HANDLE uamqp_message, IOTHUB_MESSAGE_HANDLE* iothub_message)
{
    saved_message_create_IoTHubMessage_from_uamqp_message_uamqp_message = uamqp_message;

    if (TEST_message_create_IoTHubMessage_from_uamqp_message_return == 0)
    {
        *iothub_message = TEST_IOTHUB_MESSAGE_HANDLE;
    }

    return TEST_message_create_IoTHubMessage_from_uamqp_message_return;
}

static MESSAGE_SENDER_HANDLE saved_messagesender_send_message_sender;
static MESSAGE_HANDLE saved_messagesender_send_message;
static ON_MESSAGE_SEND_COMPLETE saved_messagesender_send_on_message_send_complete;
static void* saved_messagesender_send_callback_context;

static ASYNC_OPERATION_HANDLE TEST_messagesender_send_async(MESSAGE_SENDER_HANDLE message_sender, MESSAGE_HANDLE message, ON_MESSAGE_SEND_COMPLETE on_message_send_complete, void* callback_context, tickcounter_ms_t timeout)
{
    (void)timeout;
    saved_messagesender_send_message_sender = message_sender;
    saved_messagesender_send_message = message;
    saved_messagesender_send_on_message_send_complete = on_message_send_complete;
    saved_messagesender_send_callback_context = callback_context;

    return (ASYNC_OPERATION_HANDLE)0x64;
}


static bool TEST_singlylinkedlist_add_fail_return = false;
static LIST_ITEM_HANDLE TEST_singlylinkedlist_add(SINGLYLINKEDLIST_HANDLE list, const void* item)
{
    if (list == TEST_WAIT_TO_SEND_LIST1)
    {
        saved_wait_to_send_list[saved_wait_to_send_list_count++] = item;
    }
    else if (list == TEST_WAIT_TO_SEND_LIST2)
    {
        saved_wait_to_send_list2[saved_wait_to_send_list_count2++] = item;
    }
    else if (list == TEST_IN_PROGRESS_LIST1)
    {
        saved_in_progress_list[saved_in_progress_list_count++] = item;
    }
    else if (list == TEST_IN_PROGRESS_LIST2)
    {
        saved_in_progress_list2[saved_in_progress_list_count2++] = item;
    }
    else if (list == TEST_CALLBACK_LIST1)
    {
        saved_callback_list1[saved_callback_list_count1++] = item;
    }

    return TEST_singlylinkedlist_add_fail_return ? NULL : (LIST_ITEM_HANDLE)item;
}

static int TEST_singlylinkedlist_foreach(SINGLYLINKEDLIST_HANDLE list, LIST_ACTION_FUNCTION action_function, const void* action_context)
{
    if (list != TEST_CALLBACK_LIST1)
    {
        ASSERT_FAIL("foreach only currently implemented for TEST_CALLBACK_LIST1");
    }
    else
    {
        for (int i = 0; i < saved_callback_list_count1; i++)
        {
            bool continue_processing = false;
            action_function(saved_callback_list1[i], action_context, &continue_processing);

            if (false == continue_processing)
            {
                break;
            }
        }
    }

    return 0;
}


static int TEST_singlylinkedlist_remove_return = 0;
static int TEST_singlylinkedlist_remove(SINGLYLINKEDLIST_HANDLE list, LIST_ITEM_HANDLE item)
{
    const void** TEST_list = NULL;
    int* TEST_list_count = NULL;

    if (list == TEST_WAIT_TO_SEND_LIST1)
    {
        TEST_list = saved_wait_to_send_list;
        TEST_list_count = &saved_wait_to_send_list_count;
    }
    else if (list == TEST_WAIT_TO_SEND_LIST2)
    {
        TEST_list = saved_wait_to_send_list2;
        TEST_list_count = &saved_wait_to_send_list_count2;
    }
    else if (list == TEST_IN_PROGRESS_LIST1)
    {
        TEST_list = saved_in_progress_list;
        TEST_list_count = &saved_in_progress_list_count;
    }
    else if (list == TEST_CALLBACK_LIST1)
    {
        TEST_list = saved_callback_list1;
        TEST_list_count = &saved_callback_list_count1;
    }
    else // i.e., "if (list == TEST_IN_PROGRESS_LIST2)"
    {
        TEST_list = saved_in_progress_list2;
        TEST_list_count = &saved_in_progress_list_count2;
    }

    int i;
    int item_found = 0;
    for (i = 0; i < *TEST_list_count; i++)
    {
        if (item_found)
        {
            TEST_list[i - 1] = TEST_list[i];
        }
        else if (TEST_list[i] == item)
        {
            item_found = 1;
        }
    }

    if (item_found)
    {
        (*TEST_list_count)--;
    }

    return item_found == 1 ? 0 : 1;
}

static LIST_ITEM_HANDLE TEST_singlylinkedlist_find(SINGLYLINKEDLIST_HANDLE list, LIST_MATCH_FUNCTION match_function, const void* match_context)
{
    (void)list;
    (void)match_function;
    return (LIST_ITEM_HANDLE)match_context;
}

static const void* TEST_singlylinkedlist_item_get_value(LIST_ITEM_HANDLE item_handle)
{
    return (const void*)item_handle;
}

static LIST_ITEM_HANDLE TEST_singlylinkedlist_get_head_item(SINGLYLINKEDLIST_HANDLE list)
{
    LIST_ITEM_HANDLE list_item;

    if (list == TEST_WAIT_TO_SEND_LIST1)
    {
        if (saved_wait_to_send_list_count <= 0)
        {
            list_item = NULL;
        }
        else
        {
            list_item = (LIST_ITEM_HANDLE)saved_wait_to_send_list[0];
        }
    }
    else if (list == TEST_WAIT_TO_SEND_LIST2)
    {
        if (saved_wait_to_send_list_count2 <= 0)
        {
            list_item = NULL;
        }
        else
        {
            list_item = (LIST_ITEM_HANDLE)saved_wait_to_send_list2[0];
        }
    }
    else if (list == TEST_IN_PROGRESS_LIST1)
    {
        if (saved_in_progress_list_count <= 0)
        {
            list_item = NULL;
        }
        else
        {
            list_item = (LIST_ITEM_HANDLE)saved_in_progress_list[0];
        }
    }
    else if (list == TEST_IN_PROGRESS_LIST2)
    {
        if (saved_in_progress_list_count2 <= 0)
        {
            list_item = NULL;
        }
        else
        {
            list_item = (LIST_ITEM_HANDLE)saved_in_progress_list2[0];
        }
    }
    else if (list == TEST_CALLBACK_LIST1)
    {
        if (saved_callback_list_count1 <= 0)
        {
            list_item = NULL;
        }
        else
        {
            list_item = (LIST_ITEM_HANDLE)saved_callback_list1[0];
        }
    }
    else
    {
        list_item = NULL;
    }

    return list_item;
}

static LIST_ITEM_HANDLE TEST_singlylinkedlist_get_next_item(LIST_ITEM_HANDLE item_handle)
{
    LIST_ITEM_HANDLE next_item = NULL;

    int i;
    int item_found = 0;
    for (i = 0; i < saved_in_progress_list_count; i++)
    {
        if (item_found)
        {
            next_item = (LIST_ITEM_HANDLE)saved_in_progress_list[i];
            break;
        }
        else if (saved_in_progress_list[i] == (void*)item_handle)
        {
            item_found = 1;
        }
    }

    if (item_found == 0)
    {
        for (i = 0; i < saved_in_progress_list_count2; i++)
        {
            if (item_found)
            {
                next_item = (LIST_ITEM_HANDLE)saved_in_progress_list2[i];
                break;
            }
            else if (saved_in_progress_list2[i] == (void*)item_handle)
            {
                item_found = 1;
            }
        }
    }

    if (item_found == 0)
    {
        for (i = 0; i < saved_wait_to_send_list_count2; i++)
        {
            if (item_found)
            {
                next_item = (LIST_ITEM_HANDLE)saved_wait_to_send_list2[i];
                break;
            }
            else if (saved_wait_to_send_list2[i] == (void*)item_handle)
            {
                item_found = 1;
            }
        }
    }

    if (item_found == 0)
    {
        for (i = 0; i < saved_wait_to_send_list_count; i++)
        {
            if (item_found)
            {
                next_item = (LIST_ITEM_HANDLE)saved_wait_to_send_list[i];
                break;
            }
            else if (saved_wait_to_send_list[i] == (void*)item_handle)
            {
                item_found = 1;
            }
        }
    }

    if (item_found == 0)
    {
        for (i = 0; i < saved_callback_list_count1; i++)
        {
            if (item_found)
            {
                next_item = (LIST_ITEM_HANDLE)saved_callback_list1[i];
                break;
            }
            else if (saved_callback_list1[i] == (void*)item_handle)
            {
                item_found = 1;
            }
        }
    }

    return next_item;
}


static void set_expected_calls_for_telemetry_messenger_create(TELEMETRY_MESSENGER_CONFIG* config)
{
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    // memset() - not mocked.
    STRICT_EXPECTED_CALL(STRING_construct(config->device_id)).SetReturn(TEST_DEVICE_ID_STRING_HANDLE);
    if (config->module_id)
    {
        STRICT_EXPECTED_CALL(STRING_construct(config->module_id)).SetReturn(TEST_MODULE_ID_STRING_HANDLE);
    }
    STRICT_EXPECTED_CALL(STRING_construct(config->iothub_host_fqdn)).SetReturn(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE);
    STRICT_EXPECTED_CALL(singlylinkedlist_create()).SetReturn(TEST_WAIT_TO_SEND_LIST);
    STRICT_EXPECTED_CALL(singlylinkedlist_create()).SetReturn(TEST_IN_PROGRESS_LIST);
}

static void set_expected_calls_for_attach_device_client_type_to_link(LINK_HANDLE link_handle, int amqpvalue_set_map_value_result, int link_set_attach_properties_result)
{
    STRICT_EXPECTED_CALL(amqpvalue_create_map());
    STRICT_EXPECTED_CALL(amqpvalue_create_symbol("com.microsoft:client-version"));
    STRICT_EXPECTED_CALL(amqpvalue_create_string(TEST_CLIENT_VERSION_STR));

    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(TEST_LINK_ATTACH_PROPERTIES, TEST_LINK_DEVICE_TYPE_NAME_AMQP_VALUE, TEST_LINK_DEVICE_TYPE_VALUE_AMQP_VALUE)).SetReturn(amqpvalue_set_map_value_result);

    if (amqpvalue_set_map_value_result == 0)
    {
        STRICT_EXPECTED_CALL(link_set_attach_properties(link_handle, TEST_LINK_ATTACH_PROPERTIES)).SetReturn(link_set_attach_properties_result);
    }

    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_LINK_DEVICE_TYPE_VALUE_AMQP_VALUE));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_LINK_DEVICE_TYPE_NAME_AMQP_VALUE));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_LINK_ATTACH_PROPERTIES));
}

static void set_expected_calls_for_message_receiver_create(bool testing_modules)
{
    // create_event_sender()
    // create_devices_path()
    STRING_HANDLE module_handle = testing_modules ? TEST_MODULE_ID_STRING_HANDLE : NULL;
    const char* module_name = testing_modules ? TEST_MODULE_ID : NULL;
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_DEVICES_PATH_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE)).SetReturn(TEST_IOTHUB_HOST_FQDN);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(STRING_c_str(module_handle)).SetReturn(module_name);
    // EXPECTED: STRING_sprintf

    // create_message_receive_address()
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_MESSAGE_RECEIVE_ADDRESS_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICES_PATH_STRING_HANDLE)).SetReturn(TEST_DEVICES_PATH_CHAR_PTR);
    // EXPECTED: STRING_sprintf

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID);

    // create_link_name()
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, UNIQUE_ID_BUFFER_SIZE)).IgnoreArgument_uid();
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_MESSAGE_RECEIVER_LINK_NAME_STRING_HANDLE);
    // EXPECTED: STRING_sprintf
    EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // create_message_receiver_target_name()
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_MESSAGE_RECEIVER_TARGET_NAME_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_MESSAGE_RECEIVER_LINK_NAME_STRING_HANDLE)).SetReturn(TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR);
    // EXPECTED: STRING_sprintf

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_MESSAGE_RECEIVER_TARGET_NAME_STRING_HANDLE)).SetReturn(TEST_MESSAGE_RECEIVER_TARGET_NAME_CHAR_PTR);
    STRICT_EXPECTED_CALL(messaging_create_target(TEST_MESSAGE_RECEIVER_TARGET_NAME_CHAR_PTR)).SetReturn(TEST_MESSAGE_RECEIVER_TARGET_AMQP_VALUE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_MESSAGE_RECEIVE_ADDRESS_STRING_HANDLE)).SetReturn(TEST_MESSAGE_RECEIVE_ADDRESS_CHAR_PTR);
    STRICT_EXPECTED_CALL(messaging_create_source(TEST_MESSAGE_RECEIVE_ADDRESS_CHAR_PTR)).SetReturn(TEST_MESSAGE_RECEIVER_SOURCE_AMQP_VALUE);

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_MESSAGE_RECEIVER_LINK_NAME_STRING_HANDLE)).SetReturn(TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR);
    STRICT_EXPECTED_CALL(link_create(TEST_SESSION_HANDLE, TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR, role_receiver, TEST_MESSAGE_RECEIVER_SOURCE_AMQP_VALUE, TEST_MESSAGE_RECEIVER_TARGET_AMQP_VALUE))
        .SetReturn(TEST_MESSAGE_RECEIVER_LINK_HANDLE);

    STRICT_EXPECTED_CALL(link_set_rcv_settle_mode(TEST_MESSAGE_RECEIVER_LINK_HANDLE, receiver_settle_mode_first)).IgnoreArgument(2);

    STRICT_EXPECTED_CALL(link_set_max_message_size(TEST_MESSAGE_RECEIVER_LINK_HANDLE, MESSAGE_RECEIVER_MAX_LINK_SIZE));

    set_expected_calls_for_attach_device_client_type_to_link(TEST_MESSAGE_RECEIVER_LINK_HANDLE, 0, 0);

    STRICT_EXPECTED_CALL(messagereceiver_create(TEST_MESSAGE_RECEIVER_LINK_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(messagereceiver_open(TEST_MESSAGE_RECEIVER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICES_PATH_STRING_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_MESSAGE_RECEIVE_ADDRESS_STRING_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_MESSAGE_RECEIVER_LINK_NAME_STRING_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_MESSAGE_RECEIVER_TARGET_NAME_STRING_HANDLE));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_MESSAGE_RECEIVER_SOURCE_AMQP_VALUE));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_MESSAGE_RECEIVER_TARGET_AMQP_VALUE));
}

static void set_expected_calls_for_message_receiver_destroy()
{
    STRICT_EXPECTED_CALL(messagereceiver_close(TEST_MESSAGE_RECEIVER_HANDLE));
    STRICT_EXPECTED_CALL(messagereceiver_destroy(TEST_MESSAGE_RECEIVER_HANDLE));
    STRICT_EXPECTED_CALL(link_destroy(TEST_MESSAGE_RECEIVER_LINK_HANDLE));
}

static void set_expected_calls_for_telemetry_messenger_send_async()
{
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(singlylinkedlist_add(TEST_IN_PROGRESS_LIST, IGNORED_PTR_ARG));
}

#define MAXIMUM_TEST_COMPLETE_DATA   20

static TEST_ON_SEND_COMPLETE_DATA TEST_on_send_complete_data[MAXIMUM_TEST_COMPLETE_DATA];
static int TEST_number_test_on_send_complete_data;


static IOTHUB_MESSAGE_LIST* TEST_on_event_send_complete_message;
static TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT TEST_on_event_send_complete_result;
static void* TEST_on_event_send_complete_context;
static void TEST_on_event_send_complete(IOTHUB_MESSAGE_LIST* message, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT result, void* context)
{
    ASSERT_ARE_EQUAL(int, 1, (TEST_number_test_on_send_complete_data < MAXIMUM_TEST_COMPLETE_DATA));

    TEST_on_send_complete_data[TEST_number_test_on_send_complete_data].message = message;
    TEST_on_send_complete_data[TEST_number_test_on_send_complete_data].result = result;
    TEST_on_send_complete_data[TEST_number_test_on_send_complete_data].context = context;
    TEST_number_test_on_send_complete_data++;
}

// Verify_Expected_Callbacks comparares the actual data we received (built up during TEST_on_send_complete_data callbacks) to
// what the caller has specified in expected_on_send_complete_data
void verify_expected_callbacks_received(TEST_ON_SEND_COMPLETE_DATA *expected_on_send_complete_data, int number_expected_on_send_complete_data, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT expected_result)
{
    ASSERT_ARE_EQUAL(int, number_expected_on_send_complete_data, TEST_number_test_on_send_complete_data);

    for (int i = 0; i < TEST_number_test_on_send_complete_data; i++)
    {
        ASSERT_ARE_EQUAL(void_ptr, TEST_on_send_complete_data[i].message, TEST_IOTHUB_MESSAGE_LIST_HANDLE);
        ASSERT_ARE_EQUAL(int, TEST_on_send_complete_data[i].result, expected_result);
        ASSERT_ARE_EQUAL(void_ptr, TEST_on_send_complete_data[i].context, expected_on_send_complete_data[i].context);
    }
}

static int send_events(TELEMETRY_MESSENGER_HANDLE handle, int number_of_events)
{
    int events_sent = 0;
    TEST_number_test_on_send_complete_data = 0;

    while (number_of_events > 0)
    {
        set_expected_calls_for_telemetry_messenger_send_async();
        if (telemetry_messenger_send_async(handle, TEST_IOTHUB_MESSAGE_LIST_HANDLE, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE) == 0)
        {
            events_sent++;
        }

        number_of_events--;
    }

    return events_sent;
}

static void set_expected_calls_for_message_sender_create(bool testing_modules)
{
    // create_event_sender()
    // create_devices_path()
    STRING_HANDLE module_handle = testing_modules ? TEST_MODULE_ID_STRING_HANDLE : NULL;
    const char* module_name = testing_modules ? TEST_MODULE_ID : NULL;

    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_DEVICES_PATH_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE)).SetReturn(TEST_IOTHUB_HOST_FQDN);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE)).SetReturn(TEST_DEVICE_ID);
    STRICT_EXPECTED_CALL(STRING_c_str(module_handle)).SetReturn(module_name);
    // EXPECTED: STRING_sprintf

    // create_event_send_address()
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_EVENT_SEND_ADDRESS_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICES_PATH_STRING_HANDLE)).SetReturn(TEST_DEVICES_PATH_CHAR_PTR);
    // EXPECTED: STRING_sprintf

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE));

    // create_link_name()
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(UniqueId_Generate(IGNORED_PTR_ARG, UNIQUE_ID_BUFFER_SIZE)).IgnoreArgument_uid();
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_EVENT_SENDER_LINK_NAME_STRING_HANDLE);
    // EXPECTED: STRING_sprintf
    EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // create_event_sender_source_name()
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(TEST_EVENT_SENDER_SOURCE_NAME_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_EVENT_SENDER_LINK_NAME_STRING_HANDLE)).SetReturn(TEST_EVENT_SENDER_LINK_NAME_CHAR_PTR);
    // EXPECTED: STRING_sprintf

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_EVENT_SENDER_SOURCE_NAME_STRING_HANDLE)).SetReturn(TEST_EVENT_SENDER_SOURCE_NAME_CHAR_PTR);
    STRICT_EXPECTED_CALL(messaging_create_source(TEST_EVENT_SENDER_SOURCE_NAME_CHAR_PTR)).SetReturn(TEST_EVENT_SENDER_SOURCE_AMQP_VALUE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_EVENT_SEND_ADDRESS_STRING_HANDLE)).SetReturn(TEST_EVENT_SEND_ADDRESS_CHAR_PTR);
    STRICT_EXPECTED_CALL(messaging_create_target(TEST_EVENT_SEND_ADDRESS_CHAR_PTR)).SetReturn(TEST_EVENT_SENDER_TARGET_AMQP_VALUE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_EVENT_SENDER_LINK_NAME_STRING_HANDLE)).SetReturn(TEST_EVENT_SENDER_LINK_NAME_CHAR_PTR);
    STRICT_EXPECTED_CALL(link_create(TEST_SESSION_HANDLE, TEST_EVENT_SENDER_LINK_NAME_CHAR_PTR, role_sender, TEST_EVENT_SENDER_SOURCE_AMQP_VALUE, TEST_EVENT_SENDER_TARGET_AMQP_VALUE))
        .SetReturn(TEST_EVENT_SENDER_LINK_HANDLE);

    STRICT_EXPECTED_CALL(link_set_max_message_size(TEST_EVENT_SENDER_LINK_HANDLE, MESSAGE_SENDER_MAX_LINK_SIZE)).SetReturn(TEST_link_set_max_message_size_result);

    // attach_device_client_type_to_link()
    set_expected_calls_for_attach_device_client_type_to_link(TEST_EVENT_SENDER_LINK_HANDLE, 0, 0);

    STRICT_EXPECTED_CALL(messagesender_create(TEST_EVENT_SENDER_LINK_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
    STRICT_EXPECTED_CALL(messagesender_open(TEST_MESSAGE_SENDER_HANDLE));

    STRICT_EXPECTED_CALL(STRING_delete(TEST_EVENT_SENDER_LINK_NAME_STRING_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_EVENT_SENDER_SOURCE_NAME_STRING_HANDLE));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_EVENT_SENDER_SOURCE_AMQP_VALUE));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_EVENT_SENDER_TARGET_AMQP_VALUE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICES_PATH_STRING_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_EVENT_SEND_ADDRESS_STRING_HANDLE));
}

static void set_expected_calls_for_telemetry_messenger_start(TELEMETRY_MESSENGER_CONFIG* config, TELEMETRY_MESSENGER_HANDLE messenger_handle)
{
    (void)config;
    (void)messenger_handle;
}

static void set_expected_calls_for_message_sender_destroy()
{
    STRICT_EXPECTED_CALL(messagesender_destroy(TEST_MESSAGE_SENDER_HANDLE));
    STRICT_EXPECTED_CALL(link_destroy(TEST_EVENT_SENDER_LINK_HANDLE));
}

static void set_expected_calls_for_copy_events_from_in_progress_to_waiting_list(int in_progress_list_length)
{
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST));

    int i;
    for (i = 0; i < in_progress_list_length; i++)
    {
        EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG)).SetReturn(NULL);
        EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));
        EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));
        EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void set_expected_calls_for_telemetry_messenger_stop(int wait_to_send_list_length, int in_progress_list_length, bool destroy_message_receiver)
{
    set_expected_calls_for_message_sender_destroy();

    if (destroy_message_receiver)
    {
        set_expected_calls_for_message_receiver_destroy();
    }

    // remove timed out events
    if (in_progress_list_length <= 0)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST)).SetReturn(NULL);
    }
    else
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST));

        int i;
        for (i = 0; i < in_progress_list_length; i++)
        {
            EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

            if (i < (in_progress_list_length - 1))
            {
                EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));
            }
            else
            {
                EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG)).SetReturn(NULL);
            }
        }
    }

    // Move events to wts list
    if (in_progress_list_length <= 0)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST)).SetReturn(NULL);
    }
    else
    {
        SINGLYLINKEDLIST_HANDLE new_wts_list = (TEST_WAIT_TO_SEND_LIST == TEST_WAIT_TO_SEND_LIST1 ? TEST_WAIT_TO_SEND_LIST2 : TEST_WAIT_TO_SEND_LIST1);
        SINGLYLINKEDLIST_HANDLE new_ip_list = (TEST_IN_PROGRESS_LIST == TEST_IN_PROGRESS_LIST1 ? TEST_IN_PROGRESS_LIST2 : TEST_IN_PROGRESS_LIST1);

        // rest of function
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST));
        STRICT_EXPECTED_CALL(singlylinkedlist_create()).SetReturn(new_wts_list);

        // Moving in_progress_list items to the new wts list.
        set_expected_calls_for_copy_events_from_in_progress_to_waiting_list(in_progress_list_length);

        // Moving wts (wait to send) list items to the new wts list.
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_WAIT_TO_SEND_LIST));
        int i;
        for (i = 0; i < wait_to_send_list_length; i++)
        {
            EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
            EXPECTED_CALL(singlylinkedlist_add(new_wts_list, IGNORED_PTR_ARG));
            EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));
        }

        STRICT_EXPECTED_CALL(singlylinkedlist_create()).SetReturn(new_ip_list);
        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_WAIT_TO_SEND_LIST));
        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_IN_PROGRESS_LIST));
        TEST_WAIT_TO_SEND_LIST = new_wts_list;
        TEST_IN_PROGRESS_LIST = new_ip_list;
    }
}

static void set_expected_calls_free_task(int number_callbacks)
{
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    for (int i = 0; i < number_callbacks; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));

    EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_on_message_send_complete(int number_callbacks)
{
    STRICT_EXPECTED_CALL(singlylinkedlist_foreach(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(singlylinkedlist_find(TEST_IN_PROGRESS_LIST, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_match_context()
        .IgnoreArgument_match_function();
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(TEST_IN_PROGRESS_LIST, IGNORED_PTR_ARG)).IgnoreArgument_item_handle();

    set_expected_calls_free_task(number_callbacks);
}

static void set_expected_calls_for_create_send_pending_events_state()
{
    // create_send_pending_events_state itself
    STRICT_EXPECTED_CALL(message_create());
    STRICT_EXPECTED_CALL(message_set_message_format(IGNORED_PTR_ARG, 0x80013700));
    // create_task callee
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create()).SetReturn(TEST_CALLBACK_LIST1);
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void set_expected_calls_for_send_batched_message_and_reset_state(time_t current_time)
{
    STRICT_EXPECTED_CALL(messagesender_send_async(TEST_MESSAGE_SENDER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
    STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
}

BINARY_DATA TEST_amqp_data = { NULL, 100 };


//
//  Tests sending multiple messages that are just one byte shy of hitting a rollover
//
static SEND_PENDING_TEST_EVENTS test_send_just_under_rollover_events[] = {
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
};

static TEST_ON_SEND_COMPLETE_DATA test_send_just_under_rollover_config_callbacks[] = {
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
    { NULL, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, TEST_IOTHUB_CLIENT_HANDLE },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_just_under_rollover_config = {
    100,
    test_send_just_under_rollover_events,
    COUNT_OF(test_send_just_under_rollover_events),
    true,
    test_send_just_under_rollover_config_callbacks,
    COUNT_OF(test_send_just_under_rollover_config_callbacks),
};

//
//  Tests sending multiple message where the final one causes a rollover
//
static SEND_PENDING_TEST_EVENTS test_send_rollover_events[] = {
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 10, SEND_PENDING_EXPECT_ADD },
    { 11, SEND_PENDING_EXPECT_ROLLOVER },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_rollover_config = {
    100,
    test_send_rollover_events,
    COUNT_OF(test_send_rollover_events),
    true,
    NULL,
    0
};

//
//  Tests where there is roll over and one message after
//
static SEND_PENDING_TEST_EVENTS test_send_rollover_one_message_after_events[] = {
    { 95,  SEND_PENDING_EXPECT_ADD  },
    { 10,  SEND_PENDING_EXPECT_ROLLOVER },
    { 10,  SEND_PENDING_EXPECT_ADD },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_rollover_one_message_after_config = {
    100,
    test_send_rollover_one_message_after_events,
    COUNT_OF(test_send_rollover_one_message_after_events),
    true,
    NULL,
    0
};

//
//  Tests where there is roll over and multiple messages after
//
static SEND_PENDING_TEST_EVENTS test_send_rollover_multiple_messages_after_events[] = {
    { 95,  SEND_PENDING_EXPECT_ADD  },
    { 10,  SEND_PENDING_EXPECT_ROLLOVER },
    { 10,  SEND_PENDING_EXPECT_ADD },
    { 10,  SEND_PENDING_EXPECT_ADD },
    { 10,  SEND_PENDING_EXPECT_ADD },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_rollover_multiple_messages_after_config = {
    100,
    test_send_rollover_multiple_messages_after_events,
    COUNT_OF(test_send_rollover_multiple_messages_after_events),
    true,
    NULL,
    0
};


//
//  Tests where a single message is too big.  Note that while these messages being too large
//  should be *extremely* rare, it's an interesting case for a lot of our error handling generally.
//
static SEND_PENDING_TEST_EVENTS test_send_only_message_too_big_events[] = {
    { 105,  SEND_PENDING_EXPECT_ERROR_TOO_LARGE  },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_only_message_too_big_config = {
    100,
    test_send_only_message_too_big_events,
    COUNT_OF(test_send_only_message_too_big_events),
    false,
    NULL,
    0
};

//
//  The first message is too large but there is one after that's not
//
static SEND_PENDING_TEST_EVENTS test_send_first_message_too_big_events[] = {
    { 105, SEND_PENDING_EXPECT_ERROR_TOO_LARGE  },
    { 10,  SEND_PENDING_EXPECT_ADD  },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_first_message_too_big_config = {
    100,
    test_send_first_message_too_big_events,
    COUNT_OF(test_send_first_message_too_big_events),
    true,
    NULL,
    0
};

//
//  The first message is OK but the last message is too large
//
static SEND_PENDING_TEST_EVENTS test_send_last_message_too_big_events[] = {
    { 10,  SEND_PENDING_EXPECT_ADD  },
    { 105, SEND_PENDING_EXPECT_ERROR_TOO_LARGE  },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_last_message_too_big_config = {
    100,
    test_send_last_message_too_big_events,
    COUNT_OF(test_send_last_message_too_big_events),
    true,
    NULL,
    0
};

//
//  The middle message is too large but ones one sides are OK
//
static SEND_PENDING_TEST_EVENTS test_send_middle_message_too_big_events[] = {
    { 10,  SEND_PENDING_EXPECT_ADD  },
    { 105, SEND_PENDING_EXPECT_ERROR_TOO_LARGE  },
    { 10,  SEND_PENDING_EXPECT_ADD  },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_middle_message_too_big_config = {
    100,
    test_send_middle_message_too_big_events,
    COUNT_OF(test_send_middle_message_too_big_events),
    true,
    NULL,
    0
};

//
//  The middle message is too large but ones one sides are OK, and there's a roll-over to boot
//
static SEND_PENDING_TEST_EVENTS test_send_middle_message_too_big_and_rollover_events[] = {
    { 10,  SEND_PENDING_EXPECT_ADD  },
    { 105, SEND_PENDING_EXPECT_ERROR_TOO_LARGE  },
    { 95,  SEND_PENDING_EXPECT_ROLLOVER  },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_send_middle_message_too_big_and_rollover_config = {
    100,
    test_send_middle_message_too_big_and_rollover_events,
    COUNT_OF(test_send_middle_message_too_big_and_rollover_events),
    true,
    NULL,
    0
};


//
//  We fail call to message_create_uamqp_encoding_from_iothub_message
//
static SEND_PENDING_TEST_EVENTS test_create_message_failure_events[] = {
    { 10,  SEND_PENDING_EXPECT_CREATE_MESSAGE_FAILURE  },
};

static SEND_PENDING_EVENTS_TEST_CONFIG test_create_message_failure_config = {
    100,
    test_create_message_failure_events,
    COUNT_OF(test_create_message_failure_events),
    false,
    NULL,
    0
};



// Note: This does NOT handle roll-over test paths.  These are handled with different test path.
static void set_expected_calls_for_message_do_work_send_pending_events(SEND_PENDING_EVENTS_TEST_CONFIG *test_config, time_t current_time)
{
    bool callback_cleanup_needed = false;

    if (NULL == test_config)
    {
        return;
    }
    else if (0 == test_config->number_test_events)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_WAIT_TO_SEND_LIST));
        return;
    }

    int i;
    for (i = 0; i < test_config->number_test_events; i++)
    {
        const SEND_PENDING_EXPECTED_ACTION expected_action = test_config->test_events[i].expected_action;
        const int message_create_uamqp_encoding_from_iothub_message_return = (expected_action == SEND_PENDING_EXPECT_CREATE_MESSAGE_FAILURE) ? 1 : 0;

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_WAIT_TO_SEND_LIST));
        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_remove(TEST_WAIT_TO_SEND_LIST, IGNORED_PTR_ARG));

        if (i == 0)
        {
            // Product code factors in AMQP_BATCHING_RESERVE_SIZE bytes and won't go beneath this.
            // Account for this in test here.
            uint64_t peer_max_message_size = test_config->peer_max_message_size + AMQP_BATCHING_RESERVE_SIZE;
            STRICT_EXPECTED_CALL(link_get_peer_max_message_size(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .CopyOutArgumentBuffer(2, &peer_max_message_size, sizeof(peer_max_message_size));
            set_expected_calls_for_create_send_pending_events_state();
        }

        TEST_amqp_data.length = test_config->test_events[i].number_bytes_encoded;

        STRICT_EXPECTED_CALL(message_create_uamqp_encoding_from_iothub_message(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(3, &TEST_amqp_data, sizeof(TEST_amqp_data)).SetReturn(message_create_uamqp_encoding_from_iothub_message_return);

        if ((SEND_PENDING_EXPECT_ERROR_TOO_LARGE == expected_action) || (SEND_PENDING_EXPECT_CREATE_MESSAGE_FAILURE == expected_action))
        {
            STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
            continue;
        }

        callback_cleanup_needed = true;

        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        if (SEND_PENDING_EXPECT_ROLLOVER == expected_action)
        {
            set_expected_calls_for_send_batched_message_and_reset_state(current_time);
            set_expected_calls_for_create_send_pending_events_state();
        }

        if ((SEND_PENDING_EXPECT_ROLLOVER == expected_action) || (SEND_PENDING_EXPECT_ADD == expected_action))
        {
            BINARY_DATA binary_data;
            memset(&binary_data, 0, sizeof(binary_data));

            STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, binary_data));
        }
    }

    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_WAIT_TO_SEND_LIST));

    if (test_config->send_outside_working_loop)
    {
        // We hit this case if we have not done a send in the main loop.  This is the common path;
        // there are rare cases where we last message(s) have errors that this won't be invoked.
        set_expected_calls_for_send_batched_message_and_reset_state(current_time);
    }
    else
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_foreach(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        set_expected_calls_free_task(callback_cleanup_needed ? 1 : 0);
        STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
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
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST)).SetReturn(NULL);
    }
    else
    {
        time_t send_time = add_seconds(current_time, -1 * (int)send_event_timeout_secs);

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST));

        for (; in_progress_list_length > 0; in_progress_list_length--)
        {
            EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
            EXPECTED_CALL(get_difftime(current_time, send_time)).SetReturn(difftime(current_time, send_time));
            EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));
        }

        EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG)).SetReturn(NULL);
    }
}

static void set_expected_calls_for_telemetry_messenger_do_work(MESSENGER_DO_WORK_EXP_CALL_PROFILE *profile)
{
    if (profile->current_state == TELEMETRY_MESSENGER_STATE_STARTING)
    {
        set_expected_calls_for_message_sender_create(profile->testing_modules);
    }
    else if (profile->current_state == TELEMETRY_MESSENGER_STATE_STARTED)
    {
        if (profile->create_message_receiver)
        {
            set_expected_calls_for_message_receiver_create(profile->testing_modules);
        }
        else if (profile->destroy_message_receiver)
        {
            set_expected_calls_for_message_receiver_destroy();
        }

        set_expected_calls_for_process_event_send_timeouts(profile->in_progress_list_length, profile->send_event_timeout_secs, profile->current_time);

        set_expected_calls_for_message_do_work_send_pending_events(profile->send_pending_events_test_config, profile->current_time);
    }
}

static void set_expected_calls_for_telemetry_messenger_destroy(TELEMETRY_MESSENGER_CONFIG* config, TELEMETRY_MESSENGER_HANDLE messenger_handle, bool destroy_message_sender, bool destroy_message_receiver, int wait_to_send_list_length, int in_progress_list_length, bool testing_modules)
{
    (void)config;

    set_expected_calls_for_telemetry_messenger_stop(wait_to_send_list_length, in_progress_list_length, destroy_message_receiver);

    time_t current_time = time(NULL);

    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STOPPING, false, false, wait_to_send_list_length, in_progress_list_length, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    do_work_profile->testing_modules = testing_modules;
    do_work_profile->destroy_message_sender = destroy_message_sender;
    do_work_profile->destroy_message_receiver = destroy_message_receiver;
    set_expected_calls_for_telemetry_messenger_do_work(do_work_profile);

    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST));

    wait_to_send_list_length += in_progress_list_length; // all events from in_progress_list should have been moved to wts list.

    while (wait_to_send_list_length > 0)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_WAIT_TO_SEND_LIST));
        EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_remove(TEST_WAIT_TO_SEND_LIST, IGNORED_PTR_ARG)).IgnoreArgument(2);
        EXPECTED_CALL(free(IGNORED_PTR_ARG)); // Freeing the SEND_EVENT_TASK instance.

        wait_to_send_list_length--;
    }

    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_WAIT_TO_SEND_LIST)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_WAIT_TO_SEND_LIST));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_IN_PROGRESS_LIST));

    STRICT_EXPECTED_CALL(STRING_delete(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICE_ID_STRING_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(testing_modules ? TEST_MODULE_ID_STRING_HANDLE : NULL));
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
    STRICT_EXPECTED_CALL(malloc(sizeof(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO)));
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

static void set_expected_calls_for_on_message_received_internal_callback(TELEMETRY_MESSENGER_DISPOSITION_RESULT disposition_result)
{
    TEST_on_new_message_received_callback_result = disposition_result;
    STRICT_EXPECTED_CALL(message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG)).IgnoreArgument(2);

    set_expected_calls_for_create_message_disposition_info();

    set_expected_calls_for_destroy_message_disposition_info();

    if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());
    }
    else if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_released());
    }
    else if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_rejected("Rejected by application", "Rejected by application"));
    }
}

static void set_expected_calls_for_telemetry_messenger_send_message_disposition(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT disposition_result)
{
    AMQP_VALUE uamqp_disposition_result = NULL;

    if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());
        uamqp_disposition_result = TEST_MESSAGE_DISPOSITION_ACCEPTED_AMQP_VALUE;
    }
    else if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_released());
        uamqp_disposition_result = TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE;
    }
    else if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED)
    {
        STRICT_EXPECTED_CALL(messaging_delivery_rejected("Rejected by application", "Rejected by application"));
        uamqp_disposition_result = TEST_MESSAGE_DISPOSITION_REJECTED_AMQP_VALUE;
    }

    STRICT_EXPECTED_CALL(messagereceiver_send_message_disposition(TEST_MESSAGE_RECEIVER_HANDLE, disposition_info->source, disposition_info->message_id, uamqp_disposition_result));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(uamqp_disposition_result));
}

static TELEMETRY_MESSENGER_HANDLE create_and_start_messenger(TELEMETRY_MESSENGER_CONFIG* config)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_create(config);
    TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(config, test_get_product_info, NULL);

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_start(config, handle);
    (void)telemetry_messenger_start(handle, TEST_SESSION_HANDLE);

    return handle;
}

static void crank_telemetry_messenger_do_work(TELEMETRY_MESSENGER_HANDLE handle, MESSENGER_DO_WORK_EXP_CALL_PROFILE *profile)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_do_work(profile);
    (void)telemetry_messenger_do_work(handle);

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
}

static TELEMETRY_MESSENGER_HANDLE create_and_start_messenger2(TELEMETRY_MESSENGER_CONFIG* config, bool subscribe_for_messages)
{
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    time_t current_time = time(NULL);

    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTING, false, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    do_work_profile->create_message_sender = true;
    do_work_profile->testing_modules = (config->module_id != NULL);
    crank_telemetry_messenger_do_work(handle, do_work_profile);

    if (subscribe_for_messages)
    {
        (void)telemetry_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

        do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, true, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
        do_work_profile->create_message_receiver = true;
        do_work_profile->testing_modules = (config->module_id != NULL);
        crank_telemetry_messenger_do_work(handle, do_work_profile);
    }

    return handle;
}

// ---------- Binary Data Structure Shell functions ---------- //
char* umock_stringify_BINARY_DATA(const BINARY_DATA* value)
{
    (void)value;
    char* result = "BINARY_DATA";
    return result;
}

int umock_are_equal_BINARY_DATA(const BINARY_DATA* left, const BINARY_DATA* right)
{
    //force fall through to success bypassing access violation
    (void)left;
    (void)right;
    int result = 1;
    return result;
}

int umock_copy_BINARY_DATA(BINARY_DATA* destination, const BINARY_DATA* source)
{
    //force fall through to success bypassing access violation
    (void)destination;
    (void)source;
    int result = 0;
    return result;
}

void umock_free_BINARY_DATA(BINARY_DATA* value)
{
    //do nothing
    (void)value;
}

char* umock_stringify_TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO(const TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* value)
{
    (void)value;
    char* result = "TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO";
    return result;
}

int umock_are_equal_TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO(const TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* left, const TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* right)
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

int umock_copy_TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* destination, const TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* source)
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

void umock_free_TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* value)
{
    //do nothing
    (void)value;
}

BEGIN_TEST_SUITE(iothubtr_amqp_tel_msgr_ut)

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
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_MATCH_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TELEMETRY_MESSENGER_SEND_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(pfCloneOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfDestroyOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfSetOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(delivery_number, int);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ACTION_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(tickcounter_ms_t, unsigned long long);

    REGISTER_UMOCK_VALUE_TYPE(BINARY_DATA);
    REGISTER_UMOCK_VALUE_TYPE(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO);

    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(messagesender_create, TEST_messagesender_create);
    REGISTER_GLOBAL_MOCK_HOOK(messagesender_send_async, TEST_messagesender_send_async);
    REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_create, TEST_messagereceiver_create);
    REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_open, TEST_messagereceiver_open);
    REGISTER_GLOBAL_MOCK_HOOK(message_create_uamqp_encoding_from_iothub_message, TEST_message_create_uamqp_encoding_from_iothub_message);
    REGISTER_GLOBAL_MOCK_HOOK(message_create_IoTHubMessage_from_uamqp_message, TEST_message_create_IoTHubMessage_from_uamqp_message);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_add, TEST_singlylinkedlist_add);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_head_item, TEST_singlylinkedlist_get_head_item);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_remove, TEST_singlylinkedlist_remove);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_item_get_value, TEST_singlylinkedlist_item_get_value);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_next_item, TEST_singlylinkedlist_get_next_item);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_find, TEST_singlylinkedlist_find);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_foreach, TEST_singlylinkedlist_foreach);

    REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_get_link_name, TEST_messagereceiver_get_link_name);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_remove, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_remove, 555);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_add, TEST_LIST_ITEM_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_IOTHUB_HOST_FQDN);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_source, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(link_set_max_message_size, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_max_message_size, 1);

    REGISTER_GLOBAL_MOCK_RETURN(messagesender_create, TEST_MESSAGE_SENDER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(messagesender_open, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_open, 1);

    REGISTER_GLOBAL_MOCK_RETURN(messagesender_send_async, (ASYNC_OPERATION_HANDLE)0x64);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_send_async, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_create, TEST_MESSAGE_RECEIVER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_open, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_open, 1);

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

    REGISTER_GLOBAL_MOCK_RETURN(message_create, TEST_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_create, 0);

    REGISTER_GLOBAL_MOCK_RETURN(message_set_message_format, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_message_format, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_add_body_amqp_data, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_add_body_amqp_data, 1);

    TEST_IOTHUB_MESSAGE_LIST_HANDLE = (IOTHUB_MESSAGE_LIST*)real_malloc(sizeof(IOTHUB_MESSAGE_LIST));
    ASSERT_IS_NOT_NULL(TEST_IOTHUB_MESSAGE_LIST_HANDLE);
    TEST_IOTHUB_MESSAGE_LIST_HANDLE->messageHandle = TEST_IOTHUB_MESSAGE_HANDLE;
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    free(TEST_IOTHUB_MESSAGE_LIST_HANDLE);

    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

static void reset_test_data()
{
    g_STRING_sprintf_call_count = 0;
    g_STRING_sprintf_fail_on_count = -1;

    saved_malloc_returns_count = 0;

    TEST_WAIT_TO_SEND_LIST = TEST_WAIT_TO_SEND_LIST1;
    TEST_IN_PROGRESS_LIST = TEST_IN_PROGRESS_LIST1;

    TEST_singlylinkedlist_add_fail_return = false;
    saved_wait_to_send_list_count = 0;
    saved_wait_to_send_list_count2 = 0;
    saved_in_progress_list_count = 0;
    saved_in_progress_list_count2 = 0;
    saved_callback_list_count1 = 0;

    saved_messagesender_create_link = NULL;
    saved_messagesender_create_on_message_sender_state_changed = NULL;
    saved_messagesender_create_context = NULL;

    saved_message_create_IoTHubMessage_from_uamqp_message_uamqp_message = NULL;
    TEST_message_create_IoTHubMessage_from_uamqp_message_return = 0;


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
    TEST_on_new_message_received_callback_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED;

    TEST_link_set_max_message_size_result = 0;
    TEST_amqpvalue_set_map_value_result = 0;
    TEST_link_set_attach_properties_result = 0;

    TEST_number_test_on_send_complete_data = 0;

    TEST_DELIVERY_NUMBER = (delivery_number)1234;
    TEST_messagereceiver_get_link_name_link_name = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;

    // Zero out the data after each test to make sure that a lingering global reference here
    // doesn't contfuse Valgrind into thinking the data is still legit.
    memset((void*)saved_wait_to_send_list, 0, sizeof(saved_wait_to_send_list));
    memset((void*)saved_wait_to_send_list2, 0, sizeof(saved_wait_to_send_list2));
    memset((void*)saved_in_progress_list, 0, sizeof(saved_in_progress_list));
    memset((void*)saved_in_progress_list2, 0, sizeof(saved_in_progress_list2));
    memset((void*)saved_callback_list1, 0, sizeof(saved_callback_list1));
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

TEST_FUNCTION(telemetry_messenger_create_NULL_config)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(NULL, test_get_product_info, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, handle, NULL);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_create_config_NULL_device_id)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    config->device_id = NULL;

    umock_c_reset_all_calls();

    // act
    TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(config, test_get_product_info, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, handle, NULL);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_create_config_NULL_iothub_host_fqdn)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    config->iothub_host_fqdn = NULL;

    umock_c_reset_all_calls();

    // act
    TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(config, test_get_product_info, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, handle, NULL);

    // cleanup
}

static void telemetry_messenger_create_success_impl(bool testing_modules)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    if (testing_modules == true)
    {
        config->module_id = TEST_MODULE_ID;
    }

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_create(config);

    // act
    TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(config, test_get_product_info, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}


TEST_FUNCTION(telemetry_messenger_create_success)
{
    telemetry_messenger_create_success_impl(false);
}

TEST_FUNCTION(telemetry_messenger_create_with_module_success)
{
    telemetry_messenger_create_success_impl(true);
}

TEST_FUNCTION(telemetry_messenger_create_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_create(config);
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

        TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(config, test_get_product_info, NULL);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(handle, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(telemetry_messenger_start_NULL_messenger_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_start(NULL, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_start_NULL_session_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_start(TEST_TELEMETRY_MESSENGER_HANDLE, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_start_messenger_not_stopped)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    // act
    int result = telemetry_messenger_start(handle, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_start_succeeds)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_create(config);
    TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(config, test_get_product_info, NULL);

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_start(config, handle);

    TEST_link_set_attach_properties_result = 1;

    // act
    int result = telemetry_messenger_start(handle, TEST_SESSION_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_STATE_STOPPED, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_STATE_STARTING, saved_on_state_changed_callback_new_state);

    // cleanup
    telemetry_messenger_destroy(handle);
}


TEST_FUNCTION(messenger_state_on_event_sender_state_changed_callback_OPEN)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    time_t current_time = time(NULL);

    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTING, false, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    set_expected_calls_for_telemetry_messenger_do_work(do_work_profile);
    telemetry_messenger_do_work(handle);

    // act
    ASSERT_IS_NOT_NULL(saved_messagesender_create_on_message_sender_state_changed);

    saved_messagesender_create_on_message_sender_state_changed((void*)handle, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
    crank_telemetry_messenger_do_work(handle, do_work_profile);

    // assert
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_previous_state, TELEMETRY_MESSENGER_STATE_STARTING);
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_new_state, TELEMETRY_MESSENGER_STATE_STARTED);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_state_on_event_sender_state_changed_callback_ERROR)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTING, false, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    set_expected_calls_for_telemetry_messenger_do_work(do_work_profile);
    telemetry_messenger_do_work(handle);

    // act
    ASSERT_IS_NOT_NULL(saved_messagesender_create_on_message_sender_state_changed);

    saved_messagesender_create_on_message_sender_state_changed((void*)handle, MESSAGE_SENDER_STATE_ERROR, MESSAGE_SENDER_STATE_IDLE);
    crank_telemetry_messenger_do_work(handle, do_work_profile);

    // assert
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_previous_state, TELEMETRY_MESSENGER_STATE_STARTING);
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_new_state, TELEMETRY_MESSENGER_STATE_ERROR);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_stop_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_stop(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_stop_messenger_not_started)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_create(config);
    TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(config, test_get_product_info, NULL);

    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_stop_succeeds)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_stop(0, 0, true);

    // act
    int result = telemetry_messenger_stop(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_destroy_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    telemetry_messenger_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

static void telemetry_messenger_destroy_succeeds_impl(bool testing_modules)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    if (testing_modules == true)
    {
        config->module_id = TEST_MODULE_ID;
    }

    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    ASSERT_ARE_EQUAL(int, 1, send_events(handle, 1));

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE* mdecp = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, true, true, 1, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    mdecp->testing_modules = testing_modules;
    crank_telemetry_messenger_do_work(handle, mdecp);

    ASSERT_ARE_EQUAL(int, 1, send_events(handle, 1));

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_destroy(config, handle, true, true, 1, 1, testing_modules);

    // act
    telemetry_messenger_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_destroy_succeeds)
{
    telemetry_messenger_destroy_succeeds_impl(false);
}

TEST_FUNCTION(telemetry_messenger_destroy_with_module_succeeds)
{
    telemetry_messenger_destroy_succeeds_impl(true);
}

TEST_FUNCTION(telemetry_messenger_destroy_FAIL_TO_ROLLBACK_EVENTS)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    ASSERT_ARE_EQUAL(int, 1, send_events(handle, 1));

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE* mdecp = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, true, true, 1, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    crank_telemetry_messenger_do_work(handle, mdecp);

    ASSERT_ARE_EQUAL(int, 1, send_events(handle, 1));

    umock_c_reset_all_calls();
    set_expected_calls_for_message_sender_destroy();
    set_expected_calls_for_message_receiver_destroy();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST));
    STRICT_EXPECTED_CALL(singlylinkedlist_create()).SetReturn(NULL);

    // act
    telemetry_messenger_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_STATE_STOPPING, saved_on_state_changed_callback_previous_state);
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_STATE_ERROR, saved_on_state_changed_callback_new_state);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_do_work_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    telemetry_messenger_do_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_do_work_not_started)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_create(config);
    TELEMETRY_MESSENGER_HANDLE handle = telemetry_messenger_create(config, test_get_product_info, NULL);

    umock_c_reset_all_calls();

    // act
    telemetry_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    telemetry_messenger_destroy(handle);
}

void test_send_events(SEND_PENDING_EVENTS_TEST_CONFIG *test_config, bool testing_modules)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    ASSERT_ARE_EQUAL(int, test_config->number_test_events, send_events(handle, test_config->number_test_events));

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, false, false, 1, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    do_work_profile->send_pending_events_test_config = test_config;
    do_work_profile->testing_modules = testing_modules;

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_do_work(do_work_profile);

    // act
    telemetry_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_one_message_success)
{
    test_send_events(&test_send_one_message_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_one_message_with_module_success)
{
    test_send_events(&test_send_one_message_config, true);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_just_under_rollover_with_module_success)
{
    test_send_events(&test_send_just_under_rollover_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_just_under_rollover_success)
{
    test_send_events(&test_send_just_under_rollover_config, true);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_just_rollover_success)
{
    test_send_events(&test_send_rollover_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_rollover_one_message_after_config)
{
    test_send_events(&test_send_rollover_one_message_after_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_rollover_multiple_message_after_config)
{
    test_send_events(&test_send_rollover_multiple_messages_after_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_single_message_too_big_config)
{
    test_send_events(&test_send_only_message_too_big_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_first_message_too_big_config)
{
    test_send_events(&test_send_first_message_too_big_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_last_message_too_big_config)
{
    test_send_events(&test_send_last_message_too_big_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_middle_message_too_big_events)
{
    test_send_events(&test_send_middle_message_too_big_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_middle_message_too_big_and_rollover_config)
{
    test_send_events(&test_send_middle_message_too_big_and_rollover_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_middle_message_too_big_and_rollover_with_module_config)
{
    test_send_events(&test_send_middle_message_too_big_and_rollover_config, true);
}


TEST_FUNCTION(telemetry_messenger_do_work_create_message_receiver)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    (void)telemetry_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, true, false, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_do_work(do_work_profile);

    // act
    telemetry_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_do_work_create_message_receiver_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    (void)telemetry_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    umock_c_reset_all_calls();
    set_expected_calls_for_message_receiver_create(false);
    umock_c_negative_tests_snapshot();

    saved_messagereceiver_open_on_message_received = NULL;

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 1 || i == 2 || i == 4 || i == 5 || i == 9 || i == 11 || i == 12 ||
            i == 14 || i == 16 || i == 19 || (i >= 20 && i <= 27) || (i >= 30 && i <= 35) )
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        telemetry_messenger_do_work(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_TRUE(saved_messagereceiver_open_on_message_received == NULL, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_do_work_create_message_sender_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();

    // act
    size_t i;
    size_t n = 10;
    for (i = 0; i < n; i++)
    {
        if (i == 1 || i == 2 || i == 3 || i == 5 || i == 6 || i == 10 || i == 12 || i == 13 ||
            i == 15 || i == 17 || i == 19 || (i >= 20 && i <= 36))
        {
            continue; // These expected calls do not cause the API to fail.
        }

        // arrange
        set_expected_calls_for_telemetry_messenger_create(config);
        TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

        umock_c_reset_all_calls();
        set_expected_calls_for_message_sender_create(false);
        umock_c_negative_tests_snapshot();
        n = umock_c_negative_tests_call_count();

        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        telemetry_messenger_do_work(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);

        ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_STATE_STARTING, saved_on_state_changed_callback_previous_state, error_msg);
        ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_STATE_ERROR, saved_on_state_changed_callback_new_state, error_msg);

        // cleanup
        telemetry_messenger_destroy(handle);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(messenger_state_on_message_receiver_state_changed_callback_ERROR)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(time(NULL));

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_create_on_message_receiver_state_changed);

    saved_messagereceiver_create_on_message_receiver_state_changed(saved_messagereceiver_create_context, MESSAGE_RECEIVER_STATE_ERROR, MESSAGE_RECEIVER_STATE_OPEN);

    umock_c_reset_all_calls();
    telemetry_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_previous_state, TELEMETRY_MESSENGER_STATE_STARTED);
    ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_new_state, TELEMETRY_MESSENGER_STATE_ERROR);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_do_work_destroy_message_receiver)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    (void)telemetry_messenger_unsubscribe_for_messages(handle);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, false, true, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_do_work(do_work_profile);

    // act
    telemetry_messenger_do_work(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    telemetry_messenger_destroy(handle);
}

static void test_send_events_for_callbacks(MESSAGE_SEND_RESULT message_send_result, SEND_PENDING_EVENTS_TEST_CONFIG *test_config)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);
    TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT expected_result = (MESSAGE_SEND_OK == message_send_result) ? TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK : TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING;

    ASSERT_ARE_EQUAL(int, test_config->number_test_events, send_events(handle, test_config->number_test_events));

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *mdwp = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, false, false, 1, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    mdwp->send_pending_events_test_config = test_config;

    crank_telemetry_messenger_do_work(handle, mdwp);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_message_send_complete(test_config->number_test_events);

    // act
    ASSERT_IS_NOT_NULL(saved_messagesender_send_on_message_send_complete);

    saved_messagesender_send_on_message_send_complete(saved_messagesender_send_callback_context, message_send_result, TEST_DISPOSITION_AMQP_VALUE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    verify_expected_callbacks_received(test_config->expected_on_send_complete_data, test_config->number_expected_on_send_complete_data, expected_result);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_do_work_on_event_send_complete_OK)
{
    test_send_events_for_callbacks(MESSAGE_SEND_OK, &test_send_one_message_config);
}

TEST_FUNCTION(telemetry_messenger_do_work_on_multiple_callbacks_OK)
{
    test_send_events_for_callbacks(MESSAGE_SEND_OK, &test_send_just_under_rollover_config);
}

TEST_FUNCTION(telemetry_messenger_do_work_on_event_send_complete_ERROR)
{
    test_send_events_for_callbacks(MESSAGE_SEND_ERROR, &test_send_one_message_config);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_message_create_from_iothub_message_fails)
{
    test_send_events(&test_create_message_failure_config, false);
}

TEST_FUNCTION(telemetry_messenger_do_work_send_events_messagesender_send_fails)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    umock_c_reset_all_calls();

    int i;
    for (i = 0; i < DEFAULT_EVENT_SEND_RETRY_LIMIT; i++)
    {
        ASSERT_ARE_EQUAL(int, 1, send_events(handle, 1));

        umock_c_reset_all_calls();
        TEST_number_test_on_send_complete_data = 0;

        // timeout checks
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_IN_PROGRESS_LIST)).SetReturn(NULL);

        // send events
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_WAIT_TO_SEND_LIST));
        EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_remove(TEST_WAIT_TO_SEND_LIST, IGNORED_PTR_ARG))
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(link_get_peer_max_message_size(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

        // act
        telemetry_messenger_do_work(handle);

        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING, TEST_on_send_complete_data[0].result);

        if (i < (DEFAULT_EVENT_SEND_RETRY_LIMIT - 1))
        {
            ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_new_state, TELEMETRY_MESSENGER_STATE_STARTED);
        }
        else
        {
            ASSERT_ARE_EQUAL(int, saved_on_state_changed_callback_new_state, TELEMETRY_MESSENGER_STATE_ERROR);
        }
    }

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_subscribe_for_messages_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_subscribe_for_messages(NULL, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_subscribe_for_messages_NULL_callback)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_subscribe_for_messages(handle, NULL, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_subscribe_for_messages_already_subscribed)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    (void)telemetry_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);
    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_subscribe_for_messages(handle, TEST_on_new_message_received_callback, TEST_ON_NEW_MESSAGE_RECEIVED_CB_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_unsubscribe_for_messages_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_unsubscribe_for_messages(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_unsubscribe_for_messages_not_subscribed)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger(config);

    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_unsubscribe_for_messages(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_ACCEPTED)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_message_received_internal_callback(TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_ACCEPTED_AMQP_VALUE);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_RELEASED)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_message_received_internal_callback(TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED);

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_REJECTED)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_message_received_internal_callback(TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED);

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_REJECTED_AMQP_VALUE);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_message_create_IoTHubMessage_from_uamqp_message_fails)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    TEST_on_new_message_received_callback_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED;
    STRICT_EXPECTED_CALL(message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(1);
    STRICT_EXPECTED_CALL(messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed reading AMQP message"));

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_REJECTED_AMQP_VALUE);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(messenger_on_message_received_internal_callback_create_TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO_fails)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    TEST_on_new_message_received_callback_result = TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED;
    STRICT_EXPECTED_CALL(message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG)).IgnoreArgument(2);
    STRICT_EXPECTED_CALL(malloc(sizeof(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO))).SetReturn(NULL);
    STRICT_EXPECTED_CALL(messaging_delivery_released());

    // act
    ASSERT_IS_NOT_NULL(saved_messagereceiver_open_on_message_received);

    AMQP_VALUE result = saved_messagereceiver_open_on_message_received(saved_messagereceiver_open_callback_context, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, result, TEST_MESSAGE_DISPOSITION_RELEASED_AMQP_VALUE);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_unsubscribe_for_messages_success)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();

    // act
    int unsubscription_result = telemetry_messenger_unsubscribe_for_messages(handle);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *do_work_profile = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, false, true, 0, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    crank_telemetry_messenger_do_work(handle, do_work_profile);

    // assert
    ASSERT_ARE_EQUAL(int, unsubscription_result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_async_NULL_handle)
{
    // arrange

    // act
    int result = telemetry_messenger_send_async(NULL, TEST_IOTHUB_MESSAGE_LIST_HANDLE, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_send_async_NULL_message)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    // act
    int result = telemetry_messenger_send_async(handle, NULL, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_async_NULL_callback)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    // act
    int result = telemetry_messenger_send_async(handle, TEST_IOTHUB_MESSAGE_LIST_HANDLE, NULL, TEST_IOTHUB_CLIENT_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_async_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_send_async();
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
        int result = telemetry_messenger_send_async(handle, TEST_IOTHUB_MESSAGE_LIST_HANDLE, TEST_on_event_send_complete, TEST_IOTHUB_CLIENT_HANDLE);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_get_send_status_NULL_handle)
{
    // act
    TELEMETRY_MESSENGER_SEND_STATUS send_status;
    int result = telemetry_messenger_get_send_status(NULL, &send_status);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

TEST_FUNCTION(telemetry_messenger_get_send_status_NULL_send_status)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    // act
    int result = telemetry_messenger_get_send_status(handle, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_get_send_status_IDLE_succeeds)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    // act
    TELEMETRY_MESSENGER_SEND_STATUS send_status;
    int result = telemetry_messenger_get_send_status(handle, &send_status);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_SEND_STATUS_IDLE, send_status);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_get_send_status_BUSY_succeeds)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    ASSERT_ARE_EQUAL(int, 1, send_events(handle, 1));

    // act
    TELEMETRY_MESSENGER_SEND_STATUS send_status_wts;
    int result_wts = telemetry_messenger_get_send_status(handle, &send_status_wts);

    time_t current_time = time(NULL);
    MESSENGER_DO_WORK_EXP_CALL_PROFILE *mdwp = get_msgr_do_work_exp_call_profile(TELEMETRY_MESSENGER_STATE_STARTED, false, false, 1, 0, current_time, DEFAULT_EVENT_SEND_TIMEOUT_SECS);
    crank_telemetry_messenger_do_work(handle, mdwp);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    TELEMETRY_MESSENGER_SEND_STATUS send_status_ip;
    int result_ip = telemetry_messenger_get_send_status(handle, &send_status_ip);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result_wts);
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_SEND_STATUS_BUSY, send_status_wts);
    ASSERT_ARE_EQUAL(int, 0, result_ip);
    ASSERT_ARE_EQUAL(int, TELEMETRY_MESSENGER_SEND_STATUS_BUSY, send_status_ip);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_set_option_NULL_handle)
{
    // arrange
    size_t value = 100;

    // act
    int result = telemetry_messenger_set_option(NULL, TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_set_option_NULL_name)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    size_t value = 100;

    // act
    int result = telemetry_messenger_set_option(handle, NULL, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_set_option_NULL_value)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    // act
    int result = telemetry_messenger_set_option(handle, TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_set_option_EVENT_SEND_TIMEOUT_SECS)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    size_t value = 100;

    // act
    int result = telemetry_messenger_set_option(handle, TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_set_option_SAVED_OPTIONS)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    OPTIONHANDLER_HANDLE value = TEST_OPTIONHANDLER_HANDLE;
    STRICT_EXPECTED_CALL(OptionHandler_FeedOptions(value, handle)).SetReturn(OPTIONHANDLER_OK);

    // act
    int result = telemetry_messenger_set_option(handle, TELEMETRY_MESSENGER_OPTION_SAVED_OPTIONS, value);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_set_option_OptionHandler_FeedOptions_fails)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);
    umock_c_reset_all_calls();

    OPTIONHANDLER_HANDLE value = TEST_OPTIONHANDLER_HANDLE;
    STRICT_EXPECTED_CALL(OptionHandler_FeedOptions(value, handle)).SetReturn(OPTIONHANDLER_ERROR);

    // act
    int result = telemetry_messenger_set_option(handle, TELEMETRY_MESSENGER_OPTION_SAVED_OPTIONS, value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_set_option_name_not_supported)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    size_t value = 100;

    // act
    int result = telemetry_messenger_set_option(handle, "Bernie Sanders Forever!", &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_message_disposition_NULL_messenger_handle)
{
    // arrange
    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    // act
    int result = telemetry_messenger_send_message_disposition(NULL, &disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_send_message_disposition_NULL_disposition_info)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    // act
    int result = telemetry_messenger_send_message_disposition(handle, NULL, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_message_disposition_NULL_source)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false);

    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = NULL;

    // act
    int result = telemetry_messenger_send_message_disposition(handle, &disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_message_disposition_ACCEPTED_succeeds)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_send_message_disposition(&disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // act
    int result = telemetry_messenger_send_message_disposition(handle, &disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_message_disposition_RELEASED_succeeds)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_send_message_disposition(&disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED);

    // act
    int result = telemetry_messenger_send_message_disposition(handle, &disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_message_disposition_NOT_SUBSCRIBED)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, false); // this "false" is what arranges this test

    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    umock_c_reset_all_calls();

    // act
    int result = telemetry_messenger_send_message_disposition(handle, &disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(handle);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_send_message_disposition_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = TEST_MESSAGE_RECEIVER_LINK_NAME_CHAR_PTR;
    disposition_info.message_id = TEST_DELIVERY_NUMBER;

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_send_message_disposition(&disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);
    umock_c_negative_tests_snapshot();

    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 2)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        int result = telemetry_messenger_send_message_disposition(handle, &disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }


    // cleanup
    telemetry_messenger_destroy(handle);
    umock_c_negative_tests_deinit();
}

static void set_expected_calls_for_telemetry_messenger_retrieve_options()
{
    EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, IGNORED_PTR_ARG))
        .IgnoreArgument(3);
}

TEST_FUNCTION(telemetry_messenger_retrieve_options_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    OPTIONHANDLER_HANDLE result = telemetry_messenger_retrieve_options(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
}

TEST_FUNCTION(telemetry_messenger_retrieve_options_succeeds)
{
    // arrange
    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_retrieve_options();

    // act
    OPTIONHANDLER_HANDLE result = telemetry_messenger_retrieve_options(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, TEST_OPTIONHANDLER_HANDLE, result);

    // cleanup
    telemetry_messenger_destroy(handle);
}

TEST_FUNCTION(telemetry_messenger_retrieve_options_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TELEMETRY_MESSENGER_CONFIG* config = get_messenger_config();
    TELEMETRY_MESSENGER_HANDLE handle = create_and_start_messenger2(config, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_telemetry_messenger_retrieve_options();
    umock_c_negative_tests_snapshot();

    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        OPTIONHANDLER_HANDLE result = telemetry_messenger_retrieve_options(handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(result, error_msg);
    }

    // cleanup
    telemetry_messenger_destroy(handle);
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(iothubtr_amqp_tel_msgr_ut)

