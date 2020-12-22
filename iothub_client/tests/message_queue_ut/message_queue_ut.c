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
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"
#include <limits.h>

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#undef ENABLE_MOCKS

#include "internal/message_queue.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}


// Data definitions

#define INDEFINITE_TIME                     ((time_t)-1)
#define TEST_OPTIONHANDLER_HANDLE           (OPTIONHANDLER_HANDLE)0x7771
#define TEST_PROCESS_MESSAGE_CONTEXT        (void*)0x7772
#define TEST_PROCESS_COMPLETE_CONTEXT       (void*)0x7773
#define TEST_USER_CONTEXT                   (void*)0x7773
#define USE_DEFAULT_CONFIG                  NULL
#define TEST_SOME_OTHER_MESSAGE             (MQ_MESSAGE_HANDLE)0x7777
#define TEST_MQ_MESSAGE_HANDLE_2            (MQ_MESSAGE_HANDLE)0x7778
#define TEST_LIST_ITEM_HANDLE               (LIST_ITEM_HANDLE)0x7779
#define TEST_LIST_ITEM_VALUE                (void*)0x7780
#define TEST_REASON                         (void*)0x7781


static MQ_MESSAGE_HANDLE TEST_BASE_MQ_MESSAGE_HANDLE[10];
static time_t TEST_current_time;


typedef struct TEST_MESSAGE_EXPIRATION_PROFILE_TAG
{
    double max_message_enqueued_time_secs;
    double max_message_processing_time_secs;
    size_t* expired_pending_messages;
    size_t expired_pending_messages_size;
    size_t* expired_enqueued_in_progress_messages;
    size_t expired_enqueued_in_progress_messages_size;
    size_t* expired_in_progress_messages;
    size_t expired_in_progress_messages_size;
} TEST_MESSAGE_EXPIRATION_PROFILE;

static TEST_MESSAGE_EXPIRATION_PROFILE TEST_test_message_expiration_profile;

// Helpers
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


static unsigned int TEST_OptionHandler_AddOption_saved_value;
static OPTIONHANDLER_RESULT TEST_OptionHandler_AddOption_result;
static OPTIONHANDLER_RESULT TEST_OptionHandler_AddOption(OPTIONHANDLER_HANDLE handle, const char* name, const void* value)
{
    (void)handle;
    (void)name;
    TEST_OptionHandler_AddOption_saved_value = *(const unsigned int*)value;
    return TEST_OptionHandler_AddOption_result;
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

static MESSAGE_QUEUE_HANDLE TEST_on_process_message_callback_message_queue;
static MQ_MESSAGE_HANDLE TEST_on_process_message_callback_message;
static PROCESS_MESSAGE_COMPLETED_CALLBACK TEST_on_process_message_callback_on_process_message_completed_callback;
static void* TEST_on_process_message_callback_context;
static void TEST_on_process_message_callback(MESSAGE_QUEUE_HANDLE message_queue, MQ_MESSAGE_HANDLE message, PROCESS_MESSAGE_COMPLETED_CALLBACK on_process_message_completed_callback, void* user_context)
{
    TEST_on_process_message_callback_message_queue = message_queue;
    TEST_on_process_message_callback_message = message;
    TEST_on_process_message_callback_on_process_message_completed_callback = on_process_message_completed_callback;
    TEST_on_process_message_callback_context = user_context;
}

static MQ_MESSAGE_HANDLE TEST_on_message_processing_completed_callback_message;
static MESSAGE_QUEUE_RESULT TEST_on_message_processing_completed_callback_result;
static void* TEST_on_message_processing_completed_callback_reason;
static void* TEST_on_message_processing_completed_callback_context;
static size_t TEST_on_message_processing_completed_callback_SUCCESS_result_count;
static size_t TEST_on_message_processing_completed_callback_CANCELLED_result_count;
static size_t TEST_on_message_processing_completed_callback_ERROR_result_count;
static size_t TEST_on_message_processing_completed_callback_TIMEOUT_result_count;
static void TEST_on_message_processing_completed_callback(MQ_MESSAGE_HANDLE message, MESSAGE_QUEUE_RESULT result, void* reason, void* context)
{
    TEST_on_message_processing_completed_callback_message = message;
    TEST_on_message_processing_completed_callback_result = result;
    TEST_on_message_processing_completed_callback_reason = reason;
    TEST_on_message_processing_completed_callback_context = context;

    if (result == MESSAGE_QUEUE_SUCCESS)
    {
        TEST_on_message_processing_completed_callback_SUCCESS_result_count++;
    }
    else if (result == MESSAGE_QUEUE_CANCELLED)
    {
        TEST_on_message_processing_completed_callback_CANCELLED_result_count++;
    }
    else if (result == MESSAGE_QUEUE_ERROR)
    {
        TEST_on_message_processing_completed_callback_ERROR_result_count++;
    }
    else if (result == MESSAGE_QUEUE_TIMEOUT)
    {
        TEST_on_message_processing_completed_callback_TIMEOUT_result_count++;
    }
}

static MESSAGE_QUEUE_CONFIG g_config;
static MESSAGE_QUEUE_CONFIG* get_message_queue_config()
{
    g_config.max_message_enqueued_time_secs = 0;
    g_config.max_message_processing_time_secs = 0;
    g_config.max_retry_count = 0;
    g_config.on_process_message_callback = TEST_on_process_message_callback;

    return &g_config;
}

static void set_message_queue_create_expected_calls()
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
}

static void set_dequeue_message_and_fire_callback_expected_calls()
{
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_retry_sending_message_expected_calls()
{
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void set_on_message_processing_completed_callback_expected_calls(int number_of_messages, int message_order_in_list, bool should_retry)
{
    int i;

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    for (i = 0; i < number_of_messages; i++)
    {
        // Invoked by singlylinkedlist_find.
        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

        if (i == message_order_in_list) break;
    }

    if (i == message_order_in_list)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

        if (should_retry)
        {
            set_retry_sending_message_expected_calls();
        }
        else
        {
            set_dequeue_message_and_fire_callback_expected_calls();
        }
    }
}

static void set_message_queue_remove_all_expected_calls(size_t number_of_messages_pending, size_t number_of_messages_in_progress)
{
    size_t i;

    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    for (i = 0; i < number_of_messages_pending; i++)
    {
        set_dequeue_message_and_fire_callback_expected_calls();
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    for (i = 0; i < number_of_messages_in_progress; i++)
    {
        set_dequeue_message_and_fire_callback_expected_calls();
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    }
}

static void set_message_queue_destroy_expected_calls(size_t number_of_messages_pending, size_t number_of_messages_in_progress)
{
    set_message_queue_remove_all_expected_calls(number_of_messages_pending, number_of_messages_in_progress);

    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_message_queue_add_expected_calls(time_t current_time)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void add_messages(MESSAGE_QUEUE_HANDLE mq, size_t number_of_messages, time_t current_time)
{
    size_t i;
    for (i = 0; i < number_of_messages; i++)
    {
        umock_c_reset_all_calls();
        set_message_queue_add_expected_calls(current_time);
        int result = message_queue_add(mq, TEST_BASE_MQ_MESSAGE_HANDLE[i], TEST_on_message_processing_completed_callback, TEST_USER_CONTEXT);
        ASSERT_ARE_EQUAL(int, 0, result, "failed adding message to queue");
    }
}

static MESSAGE_QUEUE_HANDLE create_message_queue(MESSAGE_QUEUE_CONFIG* config)
{
    if (config == USE_DEFAULT_CONFIG)
    {
        config = get_message_queue_config();
    }

    umock_c_reset_all_calls();
    set_message_queue_create_expected_calls();

    return message_queue_create(config);
}

static void set_process_timeouts_expected_calls(MESSAGE_QUEUE_HANDLE mq, time_t current_time,
    size_t number_of_messages_pending, size_t number_of_messages_in_progress,
    TEST_MESSAGE_EXPIRATION_PROFILE* expiration_profile
    )
{
    (void)mq;
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);

    if (expiration_profile->max_message_enqueued_time_secs > 0)
    {
        size_t i, j;

        // pending messages, max queued time
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

        for (i = 0, j = 0; i < number_of_messages_pending; i++)
        {
            STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));

            if (j < expiration_profile->expired_pending_messages_size && i == expiration_profile->expired_pending_messages[j])
            {
                STRICT_EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(expiration_profile->max_message_enqueued_time_secs + 1);
                set_dequeue_message_and_fire_callback_expected_calls();
                number_of_messages_pending--;
                j++;
            }
            else
            {
                STRICT_EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(0);
                break;
            }
        }

        // in progress messages, max queued time
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

        for (i = 0, j = 0; i < number_of_messages_in_progress; i++)
        {
            STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));

            if (j < expiration_profile->expired_enqueued_in_progress_messages_size && i == expiration_profile->expired_enqueued_in_progress_messages[j])
            {
                STRICT_EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(expiration_profile->max_message_enqueued_time_secs + 1);
                set_dequeue_message_and_fire_callback_expected_calls();
                //set_dequeue_message_and_fire_callback_expected_calls(number_of_messages_in_progress, expiration_profile->expired_enqueued_in_progress_messages[j] - i, false);
                number_of_messages_in_progress--;
                j++;
            }
            else
            {
                STRICT_EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(0);
            }
        }
    }

    if (expiration_profile->max_message_processing_time_secs > 0)
    {
        size_t i, j;

        // in progress messages, max in progress time
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

        for (i = 0, j = 0; i < number_of_messages_in_progress; i++)
        {
            STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));

            if (j < expiration_profile->expired_in_progress_messages_size && i == expiration_profile->expired_in_progress_messages[j])
            {
                STRICT_EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(expiration_profile->max_message_processing_time_secs + 1);
                set_dequeue_message_and_fire_callback_expected_calls();
                number_of_messages_in_progress--;
                j++;
            }
            else
            {
                STRICT_EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(0);
                break;
            }
        }
    }
}

static void set_process_pending_messages_calls(MESSAGE_QUEUE_HANDLE mq, time_t current_time, size_t number_of_messages_pending)
{
    (void)mq;
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    size_t i;

    for (i = 0; i < number_of_messages_pending; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    }
}

static void set_message_queue_do_work_expected_calls(MESSAGE_QUEUE_HANDLE mq, time_t current_time,
    size_t number_of_messages_pending, size_t number_of_messages_in_progress,
    TEST_MESSAGE_EXPIRATION_PROFILE* expiration_profile)
{
    set_process_timeouts_expected_calls(mq, current_time, number_of_messages_pending, number_of_messages_in_progress, expiration_profile);
    set_process_pending_messages_calls(mq, current_time, number_of_messages_pending);
}

static void crank_message_queue(MESSAGE_QUEUE_HANDLE mq, time_t current_time,
    size_t number_of_messages_pending, size_t number_of_messages_in_progress,
    TEST_MESSAGE_EXPIRATION_PROFILE* expiration_profile)
{
    if (expiration_profile == NULL)
    {
        expiration_profile = &TEST_test_message_expiration_profile;
    }

    umock_c_reset_all_calls();
    set_message_queue_do_work_expected_calls(mq, current_time, number_of_messages_pending, number_of_messages_in_progress, expiration_profile);
    message_queue_do_work(mq);
}

static void set_message_queue_is_empty_expected_calls(size_t number_of_messages_pending, size_t number_of_messages_in_progress)
{
    (void)number_of_messages_in_progress;

    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    if (number_of_messages_pending == 0) // this statement will only be evaluated if the first boolean check (in code) succeeds.
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    }
}

static void set_message_queue_retrieve_options_expected_calls()
{
    STRICT_EXPECTED_CALL(OptionHandler_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(OptionHandler_AddOption(TEST_OPTIONHANDLER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void reset_test_data()
{
    TEST_current_time = time(NULL);

    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));

    TEST_OptionHandler_AddOption_saved_value = 0;
    TEST_OptionHandler_AddOption_result = OPTIONHANDLER_OK;

    TEST_on_process_message_callback_message_queue = NULL;
    TEST_on_process_message_callback_message = NULL;
    TEST_on_process_message_callback_on_process_message_completed_callback = NULL;
    TEST_on_process_message_callback_context = NULL;

    TEST_on_message_processing_completed_callback_message = NULL;
    TEST_on_message_processing_completed_callback_result = MESSAGE_QUEUE_SUCCESS;
    TEST_on_message_processing_completed_callback_reason = NULL;
    TEST_on_message_processing_completed_callback_context = NULL;
    TEST_on_message_processing_completed_callback_SUCCESS_result_count = 0;
    TEST_on_message_processing_completed_callback_CANCELLED_result_count = 0;
    TEST_on_message_processing_completed_callback_ERROR_result_count = 0;
    TEST_on_message_processing_completed_callback_TIMEOUT_result_count = 0;

    TEST_test_message_expiration_profile.expired_enqueued_in_progress_messages = NULL;
    TEST_test_message_expiration_profile.expired_enqueued_in_progress_messages_size = 0;
    TEST_test_message_expiration_profile.expired_in_progress_messages = NULL;
    TEST_test_message_expiration_profile.expired_in_progress_messages_size = 0;
    TEST_test_message_expiration_profile.expired_pending_messages = NULL;
    TEST_test_message_expiration_profile.expired_pending_messages_size = 0;
    TEST_test_message_expiration_profile.max_message_enqueued_time_secs = 0;
    TEST_test_message_expiration_profile.max_message_processing_time_secs = 0;
}

static void register_umock_alias_types()
{
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(pfCloneOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfDestroyOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pfSetOption, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_MATCH_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MQ_MESSAGE_HANDLE, void*);
}

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(OptionHandler_AddOption, TEST_OptionHandler_AddOption);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_create, real_singlylinkedlist_create);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_destroy, real_singlylinkedlist_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_add, real_singlylinkedlist_add);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_remove, real_singlylinkedlist_remove);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_head_item, real_singlylinkedlist_get_head_item);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_next_item, real_singlylinkedlist_get_next_item);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_find, real_singlylinkedlist_find);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_item_get_value, real_singlylinkedlist_item_get_value);
}

static void register_global_mock_returns()
{
    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_Create, TEST_OPTIONHANDLER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_Create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_AddOption, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_AddOption, OPTIONHANDLER_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_add, TEST_LIST_ITEM_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_remove, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_remove, 1);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_get_head_item, TEST_LIST_ITEM_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_get_head_item, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_item_get_value, TEST_LIST_ITEM_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_item_get_value, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(get_time, INDEFINITE_TIME);
}




BEGIN_TEST_SUITE(message_queue_ut)

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

    register_umock_alias_types();
    register_global_mock_returns();
    register_global_mock_hooks();

    for (i = 0; i < 10; i++)
    {
        TEST_BASE_MQ_MESSAGE_HANDLE[i] = (MQ_MESSAGE_HANDLE)real_malloc(sizeof(char));
        ASSERT_IS_NOT_NULL(TEST_BASE_MQ_MESSAGE_HANDLE[i], "Failed setting up TEST_BASE_MQ_MESSAGE_HANDLE");
    }
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    size_t i;

    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);

    for (i = 0; i < 10; i++)
    {
        real_free(TEST_BASE_MQ_MESSAGE_HANDLE[i]);
    }
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
    umock_c_negative_tests_deinit();

    reset_test_data();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}


// Tests_SRS_MESSAGE_QUEUE_09_001: [If `config` is NULL, message_queue_create shall fail and return NULL]
TEST_FUNCTION(create_NULL_config)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    MESSAGE_QUEUE_HANDLE mq = message_queue_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(mq);

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_002: [If `config->on_process_message_callback` is NULL, message_queue_create shall fail and return NULL]
TEST_FUNCTION(create_NULL_on_process_message_callback)
{
    // arrange
    MESSAGE_QUEUE_CONFIG* config = get_message_queue_config();
    config->on_process_message_callback = NULL;

    umock_c_reset_all_calls();

    // act
    MESSAGE_QUEUE_HANDLE mq = message_queue_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(mq);

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_005: [If `instance` cannot be allocated, message_queue_create shall fail and return NULL]
// Tests_SRS_MESSAGE_QUEUE_09_007: [If singlylinkedlist_create fails, message_queue_create shall fail and return NULL]
// Tests_SRS_MESSAGE_QUEUE_09_009: [If singlylinkedlist_create fails, message_queue_create shall fail and return NULL]
// Tests_SRS_MESSAGE_QUEUE_09_011: [If any failures occur, message_queue_create shall release all memory it has allocated]
TEST_FUNCTION(create_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
    set_message_queue_create_expected_calls();
    umock_c_negative_tests_snapshot();

    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        MESSAGE_QUEUE_CONFIG* config = get_message_queue_config();

        MESSAGE_QUEUE_HANDLE mq = message_queue_create(config);

        // assert
        ASSERT_IS_NULL(mq, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

// Tests_SRS_MESSAGE_QUEUE_09_004: [Memory shall be allocated for the MESSAGE_QUEUE data structure (aka `message_queue`)]
// Tests_SRS_MESSAGE_QUEUE_09_006: [`message_queue->pending` shall be set using singlylinkedlist_create()]
// Tests_SRS_MESSAGE_QUEUE_09_008: [`message_queue->in_progress` shall be set using singlylinkedlist_create()]
// Tests_SRS_MESSAGE_QUEUE_09_010: [All arguments in `config` shall be saved into `message_queue`]
// Tests_SRS_MESSAGE_QUEUE_09_012: [If no failures occur, message_queue_create shall return the `message_queue` pointer]
TEST_FUNCTION(create_success)
{
    // arrange
    MESSAGE_QUEUE_CONFIG* config = get_message_queue_config();

    umock_c_reset_all_calls();
    set_message_queue_create_expected_calls();

    // act
    MESSAGE_QUEUE_HANDLE mq = message_queue_create(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(mq);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_013: [If `message_queue` is NULL, message_queue_destroy shall return immediately]
TEST_FUNCTION(destroy_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    message_queue_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_014: [message_queue_destroy shall invoke message_queue_remove_all]
// Tests_SRS_MESSAGE_QUEUE_09_015: [message_queue_destroy shall free all memory allocated and pointed by `message_queue`]
TEST_FUNCTION(destroy_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);
    add_messages(mq, 1, TEST_current_time);

    umock_c_reset_all_calls();
    set_message_queue_destroy_expected_calls(1, 1);

    // act
    message_queue_destroy(mq);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_026: [If `message_queue` is NULL, message_queue_retrieve_options shall return]
TEST_FUNCTION(remove_all_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    message_queue_remove_all(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_027: [Each `mq_item` in `message_queue->pending` and `message_queue->in_progress` lists shall be removed]
// Tests_SRS_MESSAGE_QUEUE_09_028: [`message_queue->on_message_processing_completed_callback` shall be invoked with MESSAGE_QUEUE_CANCELLED for each `mq_item` removed]
// Tests_SRS_MESSAGE_QUEUE_09_029: [Each `mq_item` shall be freed]
TEST_FUNCTION(remove_all_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);
    add_messages(mq, 1, TEST_current_time);

    umock_c_reset_all_calls();
    set_message_queue_remove_all_expected_calls(1, 1);

    // act
    message_queue_remove_all(mq);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(size_t, 2, TEST_on_message_processing_completed_callback_CANCELLED_result_count);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_016: [If `message_queue` or `message` are NULL, message_queue_add shall fail and return non-zero]
TEST_FUNCTION(add_NULL_mq_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = message_queue_add(NULL, TEST_BASE_MQ_MESSAGE_HANDLE[0], TEST_on_message_processing_completed_callback, TEST_USER_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_016: [If `message_queue` or `message` are NULL, message_queue_add shall fail and return non-zero]
TEST_FUNCTION(add_NULL_message_handle)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();

    // act
    int result = message_queue_add(mq, NULL, TEST_on_message_processing_completed_callback, TEST_USER_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_017: [message_queue_add shall allocate a structure (aka `mq_item`) to save the `message`]
// Tests_SRS_MESSAGE_QUEUE_09_019: [`mq_item->enqueue_time` shall be set using get_time()]
// Tests_SRS_MESSAGE_QUEUE_09_021: [`mq_item` shall be added to `message_queue->pending` list]
// Tests_SRS_MESSAGE_QUEUE_09_023: [`message` shall be saved into `mq_item->message`]
// Tests_SRS_MESSAGE_QUEUE_09_025: [If no failures occur, message_queue_add shall return 0]
TEST_FUNCTION(add_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();
    set_message_queue_add_expected_calls(TEST_current_time);

    // act
    int result = message_queue_add(mq, TEST_BASE_MQ_MESSAGE_HANDLE[0], TEST_on_message_processing_completed_callback, TEST_USER_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_018: [If `mq_item` cannot be allocated, message_queue_add shall fail and return non-zero]
// Tests_SRS_MESSAGE_QUEUE_09_020: [If get_time fails, message_queue_add shall fail and return non-zero]
// Tests_SRS_MESSAGE_QUEUE_09_022: [`mq_item` fails to be added to `message_queue->pending`, message_queue_add shall fail and return non-zero]
// Tests_SRS_MESSAGE_QUEUE_09_024: [If any failures occur, message_queue_add shall release all memory it has allocated]
TEST_FUNCTION(add_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();
    set_message_queue_add_expected_calls(TEST_current_time);
    umock_c_negative_tests_snapshot();

    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        int result = message_queue_add(mq, TEST_BASE_MQ_MESSAGE_HANDLE[0], TEST_on_message_processing_completed_callback, TEST_USER_CONTEXT);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    message_queue_destroy(mq);
}


// Tests_SRS_MESSAGE_QUEUE_09_030: [If `message_queue` or `is_empty` are NULL, message_queue_is_empty shall fail and return non-zero]
TEST_FUNCTION(is_empty_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    bool is_empty;
    int result = message_queue_is_empty(NULL, &is_empty);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_030: [If `message_queue` or `is_empty` are NULL, message_queue_is_empty shall fail and return non-zero]
TEST_FUNCTION(is_empty_NULL_is_empty)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();

    // act
    int result = message_queue_is_empty(mq, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_031: [If `message_queue->pending` and `message_queue->in_progress` are empty, `is_empty` shall be set to true]
// Tests_SRS_MESSAGE_QUEUE_09_033: [If no failures occur, message_queue_is_empty shall return 0]
TEST_FUNCTION(is_empty_0_pending_0_in_progress_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();
    set_message_queue_is_empty_expected_calls(0, 0);

    // act
    bool is_empty;
    int result = message_queue_is_empty(mq, &is_empty);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(is_empty);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_032: [Otherwise `is_empty` shall be set to false]
// Tests_SRS_MESSAGE_QUEUE_09_033: [If no failures occur, message_queue_is_empty shall return 0]
TEST_FUNCTION(is_empty_1_pending_0_in_progress_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);

    umock_c_reset_all_calls();
    set_message_queue_is_empty_expected_calls(1, 0);

    // act
    bool is_empty;
    int result = message_queue_is_empty(mq, &is_empty);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_FALSE(is_empty);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_032: [Otherwise `is_empty` shall be set to false]
// Tests_SRS_MESSAGE_QUEUE_09_033: [If no failures occur, message_queue_is_empty shall return 0]
TEST_FUNCTION(is_empty_0_pending_1_in_progress_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);

    umock_c_reset_all_calls();
    set_message_queue_is_empty_expected_calls(0, 1);

    // act
    bool is_empty;
    int result = message_queue_is_empty(mq, &is_empty);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_FALSE(is_empty);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_032: [Otherwise `is_empty` shall be set to false]
// Tests_SRS_MESSAGE_QUEUE_09_033: [If no failures occur, message_queue_is_empty shall return 0]
TEST_FUNCTION(is_empty_1_pending_1_in_progress_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);
    add_messages(mq, 1, TEST_current_time);

    umock_c_reset_all_calls();
    set_message_queue_is_empty_expected_calls(1, 1);

    // act
    bool is_empty;
    int result = message_queue_is_empty(mq, &is_empty);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_FALSE(is_empty);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_034: [If `message_queue` is NULL, message_queue_do_work shall return immediately]
TEST_FUNCTION(do_work_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    message_queue_do_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_039: [Each `mq_item` in `message_queue->pending` shall be moved to `message_queue->in_progress`]
// Tests_SRS_MESSAGE_QUEUE_09_040: [`mq_item->processing_start_time` shall be set using get_time()]
// Tests_SRS_MESSAGE_QUEUE_09_043: [If no failures occur, `message_queue->on_process_message_callback` shall be invoked passing `mq_item->message` and `on_process_message_completed_callback`]
TEST_FUNCTION(do_work_NO_EXPIRATION_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);

    umock_c_reset_all_calls();
    set_message_queue_do_work_expected_calls(mq, TEST_current_time, 1, 0, &TEST_test_message_expiration_profile);

    // act
    message_queue_do_work(mq);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, (void_ptr)TEST_BASE_MQ_MESSAGE_HANDLE[0], (void_ptr)TEST_on_process_message_callback_message);
    ASSERT_IS_NOT_NULL(TEST_on_process_message_callback_on_process_message_completed_callback);
    ASSERT_IS_NOT_NULL(TEST_on_process_message_callback_context);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_041: [If get_time() fails, `mq_item` shall be removed from `message_queue->in_progress`]
// Tests_SRS_MESSAGE_QUEUE_09_042: [If any failures occur, `mq_item->on_message_processing_completed_callback` shall be invoked with MESSAGE_QUEUE_ERROR and `mq_item` freed]
TEST_FUNCTION(do_work_NO_EXPIRATION_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    size_t i, j;
    for (i = 0, j = 1; i < j; i++)
    {
        if (i == 6)
        {
            continue;
        }

        // arrange
        TEST_on_message_processing_completed_callback_message = NULL;
        TEST_on_message_processing_completed_callback_result = MESSAGE_QUEUE_TIMEOUT;

        char error_msg[64];
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);

        if (i >= 3 && i != 4)
        {
            add_messages(mq, 1, TEST_current_time);
        }

        umock_c_reset_all_calls();
        set_message_queue_do_work_expected_calls(mq, TEST_current_time, 1, 0, &TEST_test_message_expiration_profile);
        umock_c_negative_tests_snapshot();

        j = umock_c_negative_tests_call_count();

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        message_queue_do_work(mq);

        // assert
        if (i == 1 || i == 2)
        {
            ASSERT_IS_NULL(TEST_on_process_message_callback_message, error_msg);
            ASSERT_IS_NULL(TEST_on_message_processing_completed_callback_message, error_msg);
        }
        else if (i >= 3)
        {
            ASSERT_IS_NOT_NULL(TEST_on_message_processing_completed_callback_message, error_msg);
            ASSERT_ARE_EQUAL(int, (int)MESSAGE_QUEUE_ERROR, (int)TEST_on_message_processing_completed_callback_result, error_msg);
        }

        umock_c_reset_all_calls();
    }

    // cleanup
    message_queue_destroy(mq);
    umock_c_negative_tests_deinit();
}

// Tests_SRS_MESSAGE_QUEUE_09_059: [If `message_queue` is NULL, message_queue_set_max_retry_count shall fail and return non-zero]
TEST_FUNCTION(message_queue_set_max_retry_count_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = message_queue_set_max_retry_count(NULL, 2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_060: [`max_retry_count` shall be saved into `message_queue->max_retry_count`]
// Tests_SRS_MESSAGE_QUEUE_09_061: [If no failures occur, message_queue_set_max_retry_count shall return 0]
TEST_FUNCTION(message_queue_set_max_retry_count_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();

    // act
    int result = message_queue_set_max_retry_count(mq, 2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_055: [If `message_queue` is NULL, message_queue_set_max_message_processing_time_secs shall fail and return non-zero]
TEST_FUNCTION(message_queue_set_max_message_processing_time_secs_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = message_queue_set_max_message_processing_time_secs(NULL, 60);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_057: [`seconds` shall be saved into `message_queue->max_message_processing_time_secs`]
// Tests_SRS_MESSAGE_QUEUE_09_058: [If no failures occur, message_queue_set_max_message_processing_time_secs shall return 0]
TEST_FUNCTION(message_queue_set_max_message_processing_time_secs_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();

    // act
    int result = message_queue_set_max_message_processing_time_secs(mq, 60);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_051: [If `message_queue` is NULL, message_queue_set_max_message_enqueued_time_secs shall fail and return non-zero]
TEST_FUNCTION(message_queue_set_max_message_enqueued_time_secs_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    int result = message_queue_set_max_message_enqueued_time_secs(NULL, 60);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_053: [`seconds` shall be saved into `message_queue->max_message_enqueued_time_secs`]
// Tests_SRS_MESSAGE_QUEUE_09_054: [If no failures occur, message_queue_set_max_message_enqueued_time_secs shall return 0]
TEST_FUNCTION(message_queue_set_max_message_enqueued_time_secs_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();

    // act
    int result = message_queue_set_max_message_enqueued_time_secs(mq, 60);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    message_queue_destroy(mq);
}


// Tests_SRS_MESSAGE_QUEUE_09_062: [If `message_queue` is NULL, message_queue_retrieve_options shall fail and return NULL]
TEST_FUNCTION(message_queue_retrieve_options_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    OPTIONHANDLER_HANDLE result = message_queue_retrieve_options(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_09_063: [An OPTIONHANDLER_HANDLE instance shall be created using OptionHandler_Create]
// Tests_SRS_MESSAGE_QUEUE_09_065: [Each option of `instance` shall be added to the OPTIONHANDLER_HANDLE instance using OptionHandler_AddOption]
// Tests_SRS_MESSAGE_QUEUE_09_068: [If no failures occur, message_queue_retrieve_options shall return the OPTIONHANDLER_HANDLE instance]
TEST_FUNCTION(message_queue_retrieve_options_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();
    set_message_queue_retrieve_options_expected_calls();

    // act
    OPTIONHANDLER_HANDLE result = message_queue_retrieve_options(mq);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_064: [If an OPTIONHANDLER_HANDLE instance fails to be created, message_queue_retrieve_options shall fail and return NULL]
// Tests_SRS_MESSAGE_QUEUE_09_066: [If OptionHandler_AddOption fails, message_queue_retrieve_options shall fail and return NULL]
// Tests_SRS_MESSAGE_QUEUE_09_067: [If message_queue_retrieve_options fails, any allocated memory shall be freed]
TEST_FUNCTION(message_queue_retrieve_options_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    umock_c_reset_all_calls();
    set_message_queue_retrieve_options_expected_calls();
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        char error_msg[64];
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        OPTIONHANDLER_HANDLE result = message_queue_retrieve_options(mq);

        // assert
        ASSERT_IS_NULL(result, error_msg);
    }


    // cleanup
    umock_c_negative_tests_deinit();
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_069: [If `message` or `message_queue` are NULL, on_process_message_completed_callback shall return immediately]
TEST_FUNCTION(on_message_processing_completed_callback_NULL_message)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);

    umock_c_reset_all_calls();

    // act
    TEST_on_process_message_callback_on_process_message_completed_callback(mq, NULL, MESSAGE_QUEUE_SUCCESS, TEST_REASON);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_SUCCESS_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_ERROR_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_CANCELLED_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_TIMEOUT_result_count);
    ASSERT_ARE_EQUAL(void_ptr, NULL, TEST_on_message_processing_completed_callback_reason);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_069: [If `message` or `message_queue` are NULL, on_process_message_completed_callback shall return immediately]
TEST_FUNCTION(on_message_processing_completed_callback_NULL_message_queue)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);

    umock_c_reset_all_calls();

    // act
    TEST_on_process_message_callback_on_process_message_completed_callback(NULL, TEST_on_process_message_callback_message, MESSAGE_QUEUE_SUCCESS, TEST_REASON);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_SUCCESS_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_ERROR_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_CANCELLED_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_TIMEOUT_result_count);
    ASSERT_ARE_EQUAL(void_ptr, NULL, TEST_on_message_processing_completed_callback_reason);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_044: [If `message` is not present in `message_queue->in_progress`, it shall be ignored]
TEST_FUNCTION(on_message_processing_completed_callback_MESSAGE_not_present)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);

    umock_c_reset_all_calls();
    set_on_message_processing_completed_callback_expected_calls(1, -1, false);

    // act
    TEST_on_process_message_callback_on_process_message_completed_callback(mq, TEST_SOME_OTHER_MESSAGE, MESSAGE_QUEUE_SUCCESS, TEST_REASON);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_SUCCESS_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_ERROR_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_CANCELLED_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_TIMEOUT_result_count);
    ASSERT_ARE_EQUAL(void_ptr, NULL, TEST_on_message_processing_completed_callback_reason);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_045: [If `message` is present in `message_queue->in_progress`, it shall be removed]
// Tests_SRS_MESSAGE_QUEUE_09_049: [Otherwise `mq_item->on_message_processing_completed_callback` shall be invoked passing `mq_item->message`, `result`, `reason` and `mq_item->user_context`]
// Tests_SRS_MESSAGE_QUEUE_09_050: [The `mq_item` related to `message` shall be freed]
TEST_FUNCTION(on_message_processing_completed_callback_success)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);
    ASSERT_ARE_EQUAL(void_ptr, (void*)TEST_BASE_MQ_MESSAGE_HANDLE[0], (void*)TEST_on_process_message_callback_message);
    ASSERT_IS_NOT_NULL(TEST_on_process_message_callback_on_process_message_completed_callback);
    ASSERT_ARE_EQUAL(void_ptr, (void*)TEST_USER_CONTEXT, (void*)TEST_on_process_message_callback_context);

    umock_c_reset_all_calls();
    set_on_message_processing_completed_callback_expected_calls(1, 0, false);

    // act
    TEST_on_process_message_callback_on_process_message_completed_callback(mq, TEST_on_process_message_callback_message, MESSAGE_QUEUE_SUCCESS, TEST_REASON);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 1, (int)TEST_on_message_processing_completed_callback_SUCCESS_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_ERROR_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_CANCELLED_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_TIMEOUT_result_count);
    ASSERT_ARE_EQUAL(void_ptr, TEST_REASON, TEST_on_message_processing_completed_callback_reason);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_047: [If `result` is MESSAGE_QUEUE_RETRYABLE_ERROR and `mq_item->number_of_attempts` is less than or equal `message_queue->max_retry_count`, the `message` shall be moved to `message_queue->pending` to be re-sent]
// Tests_SRS_MESSAGE_QUEUE_09_048: [If `result` is MESSAGE_QUEUE_RETRYABLE_ERROR and `mq_item->number_of_attempts` is greater than `message_queue->max_retry_count`, result shall be changed to MESSAGE_QUEUE_ERROR]
TEST_FUNCTION(on_message_processing_completed_callback_RETRYABLE_ERROR)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);
    (void)message_queue_set_max_retry_count(mq, 2);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);

    umock_c_reset_all_calls();
    set_on_message_processing_completed_callback_expected_calls(1, 0, true);
    set_message_queue_do_work_expected_calls(mq, TEST_current_time, 1, 0, &TEST_test_message_expiration_profile);
    set_on_message_processing_completed_callback_expected_calls(1, 0, true);
    set_message_queue_do_work_expected_calls(mq, TEST_current_time, 1, 0, &TEST_test_message_expiration_profile);
    set_on_message_processing_completed_callback_expected_calls(1, 0, false);

    // act
    TEST_on_process_message_callback_on_process_message_completed_callback(mq,
        TEST_on_process_message_callback_message, MESSAGE_QUEUE_RETRYABLE_ERROR, NULL);
    message_queue_do_work(mq);
    TEST_on_process_message_callback_on_process_message_completed_callback(mq,
        TEST_on_process_message_callback_message, MESSAGE_QUEUE_RETRYABLE_ERROR, NULL);
    message_queue_do_work(mq);
    TEST_on_process_message_callback_on_process_message_completed_callback(mq,
        TEST_on_process_message_callback_message, MESSAGE_QUEUE_RETRYABLE_ERROR, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_SUCCESS_result_count);
    ASSERT_ARE_EQUAL(int, 1, (int)TEST_on_message_processing_completed_callback_ERROR_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_CANCELLED_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_TIMEOUT_result_count);
    ASSERT_ARE_EQUAL(void_ptr, NULL, TEST_on_message_processing_completed_callback_reason);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_035: [If `message_queue->max_message_enqueued_time_secs` is greater than zero, `message_queue->in_progress` and `message_queue->pending` items shall be checked for timeout]
// Tests_SRS_MESSAGE_QUEUE_09_036: [If any items are in `message_queue` lists for `message_queue->max_message_enqueued_time_secs` or more, they shall be removed and `message_queue->on_message_processing_completed_callback` invoked with MESSAGE_QUEUE_TIMEOUT]
TEST_FUNCTION(do_work_pending_queue_timeout)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);
    (void)message_queue_set_max_message_enqueued_time_secs(mq, 10);

    add_messages(mq, 1, TEST_current_time);

    time_t t1 = add_seconds(TEST_current_time, 10);

    TEST_MESSAGE_EXPIRATION_PROFILE exp_prof;
    exp_prof.max_message_enqueued_time_secs = 10;
    exp_prof.max_message_processing_time_secs = 0;
    size_t expired_pending_messages[] = { 0 };
    exp_prof.expired_pending_messages = expired_pending_messages;
    exp_prof.expired_pending_messages_size = 1;
    exp_prof.expired_in_progress_messages = NULL;
    exp_prof.expired_in_progress_messages_size = 0;
    exp_prof.expired_enqueued_in_progress_messages = NULL;
    exp_prof.expired_enqueued_in_progress_messages_size = 0;

    umock_c_reset_all_calls();
    set_process_timeouts_expected_calls(mq, t1, 1, 0, &exp_prof);
    set_process_pending_messages_calls(mq, t1, 0);

    // act
    message_queue_do_work(mq);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_SUCCESS_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_ERROR_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_CANCELLED_result_count);
    ASSERT_ARE_EQUAL(int, 1, (int)TEST_on_message_processing_completed_callback_TIMEOUT_result_count);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_037: [If `message_queue->max_message_processing_time_secs` is greater than zero, `message_queue->in_progress` items shall be checked for timeout]
// Tests_SRS_MESSAGE_QUEUE_09_038: [If any items are in `message_queue->in_progress` for `message_queue->max_message_processing_time_secs` or more, they shall be removed and `message_queue->on_message_processing_completed_callback` invoked with MESSAGE_QUEUE_TIMEOUT]
TEST_FUNCTION(do_work_in_progress_processing_timeout)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);

    (void)message_queue_set_max_message_processing_time_secs(mq, 10);

    time_t t1 = add_seconds(TEST_current_time, 10);

    TEST_MESSAGE_EXPIRATION_PROFILE exp_prof;
    exp_prof.max_message_enqueued_time_secs = 0;
    exp_prof.max_message_processing_time_secs = 10;
    exp_prof.expired_pending_messages = NULL;
    exp_prof.expired_pending_messages_size = 0;
    size_t expired_in_progress_messages[] = { 0 };
    exp_prof.expired_in_progress_messages = expired_in_progress_messages;
    exp_prof.expired_in_progress_messages_size = 1;
    exp_prof.expired_enqueued_in_progress_messages = NULL;
    exp_prof.expired_enqueued_in_progress_messages_size = 0;

    umock_c_reset_all_calls();
    set_process_timeouts_expected_calls(mq, t1, 0, 1, &exp_prof);
    set_process_pending_messages_calls(mq, t1, 0);

    // act
    message_queue_do_work(mq);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_SUCCESS_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_ERROR_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_CANCELLED_result_count);
    ASSERT_ARE_EQUAL(int, 1, (int)TEST_on_message_processing_completed_callback_TIMEOUT_result_count);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_09_035: [If `message_queue->max_message_enqueued_time_secs` is greater than zero, `message_queue->in_progress` and `message_queue->pending` items shall be checked for timeout]
// Tests_SRS_MESSAGE_QUEUE_09_036: [If any items are in `message_queue` lists for `message_queue->max_message_enqueued_time_secs` or more, they shall be removed and `message_queue->on_message_processing_completed_callback` invoked with MESSAGE_QUEUE_TIMEOUT]
TEST_FUNCTION(do_work_in_progress_queue_timeout)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 1, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);

    (void)message_queue_set_max_message_enqueued_time_secs(mq, 10);

    time_t t1 = add_seconds(TEST_current_time, 10);

    TEST_MESSAGE_EXPIRATION_PROFILE exp_prof;
    exp_prof.max_message_enqueued_time_secs = 10;
    exp_prof.max_message_processing_time_secs = 0;
    exp_prof.expired_pending_messages = NULL;
    exp_prof.expired_pending_messages_size = 0;
    exp_prof.expired_in_progress_messages = NULL;
    exp_prof.expired_in_progress_messages_size = 0;
    size_t expired_enqueued_in_progress_messages[] = { 0 };
    exp_prof.expired_enqueued_in_progress_messages = expired_enqueued_in_progress_messages;
    exp_prof.expired_enqueued_in_progress_messages_size = 1;

    umock_c_reset_all_calls();
    set_process_timeouts_expected_calls(mq, t1, 0, 1, &exp_prof);
    set_process_pending_messages_calls(mq, t1, 0);

    // act
    message_queue_do_work(mq);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_SUCCESS_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_ERROR_result_count);
    ASSERT_ARE_EQUAL(int, 0, (int)TEST_on_message_processing_completed_callback_CANCELLED_result_count);
    ASSERT_ARE_EQUAL(int, 1, (int)TEST_on_message_processing_completed_callback_TIMEOUT_result_count);

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_21_070: [The message_queue_move_all_back_to_pending shall add all in_progress message in front of the pending messages.]
TEST_FUNCTION(message_queue_move_all_back_to_pending_with_in_progress_and_pending_succeed)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 2, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);
    add_messages(mq, 2, TEST_current_time);

    umock_c_reset_all_calls();
    for (int i = 0; i < 2; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_retry_sending_message_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    for (int i = 0; i < 4; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_retry_sending_message_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    // act
    int result = message_queue_move_all_back_to_pending(mq);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    message_queue_destroy(mq);
}

TEST_FUNCTION(message_queue_move_all_back_to_pending_with_only_in_progress_succeed)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 2, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    for (int i = 0; i < 2; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_retry_sending_message_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    // act
    int result = message_queue_move_all_back_to_pending(mq);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    message_queue_destroy(mq);
}

TEST_FUNCTION(message_queue_move_all_back_to_pending_with_only_pending_succeed)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 2, TEST_current_time);

    umock_c_reset_all_calls();
    for (int i = 0; i < 2; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_retry_sending_message_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    for (int i = 0; i < 2; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_retry_sending_message_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    // act
    int result = message_queue_move_all_back_to_pending(mq);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_21_071: [If the message_queue is NULL, the message_queue_move_all_back_to_pending shall return non-zero result.]
TEST_FUNCTION(message_queue_move_all_back_to_pending_null_message_queue_failed)
{
    // arrange

    // act
    int result = message_queue_move_all_back_to_pending(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_MESSAGE_QUEUE_21_072: [If move pending messages failed, the message_queue_move_all_back_to_pending shall delete all elements in the queue and return non-zero.]
TEST_FUNCTION(message_queue_move_all_back_to_pending_move_pending_failed)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 2, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);
    add_messages(mq, 2, TEST_current_time);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(1);
    for (int i = 0; i < 2; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_dequeue_message_and_fire_callback_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    for (int i = 0; i < 2; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_dequeue_message_and_fire_callback_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    // act
    int result = message_queue_move_all_back_to_pending(mq);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    message_queue_destroy(mq);
}

// Tests_SRS_MESSAGE_QUEUE_21_073: [If move in_progress messages failed, the message_queue_move_all_back_to_pending shall delete all elements in the queue and return non-zero.]
TEST_FUNCTION(message_queue_move_all_back_to_pending_move_in_progress_failed)
{
    // arrange
    MESSAGE_QUEUE_HANDLE mq = create_message_queue(USE_DEFAULT_CONFIG);

    add_messages(mq, 2, TEST_current_time);
    crank_message_queue(mq, TEST_current_time, 1, 0, NULL);
    add_messages(mq, 2, TEST_current_time);

    umock_c_reset_all_calls();
    for (int i = 0; i < 2; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_retry_sending_message_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(1);
    for (int i = 0; i < 4; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        set_dequeue_message_and_fire_callback_expected_calls();
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    // act
    int result = message_queue_move_all_back_to_pending(mq);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    message_queue_destroy(mq);
}

END_TEST_SUITE(message_queue_ut)
