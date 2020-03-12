// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/map.h"

#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_plain.h"
#include "azure_uamqp_c/cbs.h"
#include "azure_uamqp_c/link.h"

#include "parson.h"
#include "iothub_message.h"

MOCKABLE_FUNCTION(, JSON_Array*, json_array_get_array, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_array_get_object, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value*, value);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_array_clear, JSON_Array*, array);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
#undef ENABLE_MOCKS

#include "iothub_messaging_ll.h"

TEST_DEFINE_ENUM_TYPE(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_RESULT_VALUES);

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, void*, context);
MOCKABLE_FUNCTION(, void, TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK, void*, context, IOTHUB_MESSAGING_RESULT, messagingResult);
MOCKABLE_FUNCTION(, void, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, void*, context, IOTHUB_SERVICE_FEEDBACK_BATCH*, feedbackBatch);
#undef ENABLE_MOCKS



static TEST_MUTEX_HANDLE g_testByTest;

static STRING_HANDLE TEST_STRING_HANDLE = (STRING_HANDLE)0x4242;
#define TEST_ASYNC_HANDLE           (ASYNC_OPERATION_HANDLE)0x4246

STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)malloc(1);
}

void my_STRING_delete(STRING_HANDLE handle)
{
    free(handle);
}

static void* my_gballoc_malloc(size_t size)
{
    return (void*)malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    if (ptr != NULL)
    {
        free(ptr);
    }
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    char* p = (char*)malloc(strlen(source)+1);
    (void)memcpy(p, source, strlen(source) + 1);
    *destination = p;
    return 0;
}

typedef struct LIST_ITEM_INSTANCE_TAG
{
    const void* item;
    void* next;
} LIST_ITEM_INSTANCE;

typedef struct SINGLYLINKEDLIST_INSTANCE_TAG
{
    LIST_ITEM_INSTANCE* head;
} LIST_INSTANCE;

static SINGLYLINKEDLIST_HANDLE my_list_create(void)
{
    LIST_INSTANCE* result;

    result = (LIST_INSTANCE*)malloc(sizeof(LIST_INSTANCE));
    if (result != NULL)
    {
        result->head = NULL;
    }

    return result;
}

static void my_list_destroy(SINGLYLINKEDLIST_HANDLE list)
{
    if (list != NULL)
    {
        LIST_INSTANCE* list_instance = (LIST_INSTANCE*)list;

        while (list_instance->head != NULL)
        {
            LIST_ITEM_INSTANCE* current_item = list_instance->head;
            list_instance->head = (LIST_ITEM_INSTANCE*)current_item->next;
            free(current_item);
        }

        free(list_instance);
    }
}

static LIST_ITEM_HANDLE my_list_add(SINGLYLINKEDLIST_HANDLE list, const void* item)
{
    LIST_ITEM_INSTANCE* result;

    if ((list == NULL) ||
        (item == NULL))
    {
        result = NULL;
    }
    else
    {
        LIST_INSTANCE* list_instance = (LIST_INSTANCE*)list;
        result = (LIST_ITEM_INSTANCE*)malloc(sizeof(LIST_ITEM_INSTANCE));

        if (result == NULL)
        {
            result = NULL;
        }
        else
        {
            result->next = NULL;
            result->item = item;

            if (list_instance->head == NULL)
            {
                list_instance->head = result;
            }
            else
            {
                LIST_ITEM_INSTANCE* current = list_instance->head;
                while (current->next != NULL)
                {
                    current = (LIST_ITEM_INSTANCE*)current->next;
                }

                current->next = result;
            }
        }
    }

    return result;
}

static LIST_ITEM_HANDLE my_list_get_head_item(SINGLYLINKEDLIST_HANDLE list)
{
    LIST_ITEM_HANDLE result;

    if (list == NULL)
    {
        result = NULL;
    }
    else
    {
        LIST_INSTANCE* list_instance = (LIST_INSTANCE*)list;

        result = list_instance->head;
    }

    return result;
}

static LIST_ITEM_HANDLE my_list_get_next_item(LIST_ITEM_HANDLE item_handle)
{
    LIST_ITEM_HANDLE result;

    if (item_handle == NULL)
    {
        result = NULL;
    }
    else
    {
        result = (LIST_ITEM_HANDLE)((LIST_ITEM_INSTANCE*)item_handle)->next;
    }

    return result;
}

static const void* my_list_item_get_value(LIST_ITEM_HANDLE item_handle)
{
    const void* result;

    if (item_handle == NULL)
    {
        result = NULL;
    }
    else
    {
        result = ((LIST_ITEM_INSTANCE*)item_handle)->item;
    }

    return result;
}

static int my_message_get_body_amqp_data_in_place(MESSAGE_HANDLE message, size_t index, BINARY_DATA* binary_data)
{
    (void)index;
    (void)message;
    binary_data->bytes = NULL;
    binary_data->length = 1;
    return 0;
}

static STRING_HANDLE my_SASToken_Create(STRING_HANDLE key, STRING_HANDLE scope, STRING_HANDLE keyName, size_t expiry)
{
    (void)key;
    (void)scope;
    (void)keyName;
    (void)expiry;
    return my_STRING_construct("sas");
}

static ON_MESSAGE_SENDER_STATE_CHANGED onMessageSenderStateChangedCallback;
static MESSAGE_SENDER_HANDLE messagesender_create_return = NULL;
static void* msg_send_ctx = NULL;
MESSAGE_SENDER_HANDLE my_messagesender_create(LINK_HANDLE link, ON_MESSAGE_SENDER_STATE_CHANGED on_message_sender_state_changed, void* context)
{
    (void)link;
    msg_send_ctx = context;
    onMessageSenderStateChangedCallback = on_message_sender_state_changed;
    messagesender_create_return = (MESSAGE_SENDER_HANDLE)malloc(1);
    return messagesender_create_return;
}

void my_messagesender_destroy(MESSAGE_SENDER_HANDLE message_sender)
{
    if (message_sender != NULL)
    {
        free(message_sender);
    }
}

static MESSAGE_RECEIVER_HANDLE TEST_MESSAGE_RECEIVER_HANDLE = (MESSAGE_RECEIVER_HANDLE)0x5151;
static ON_MESSAGE_RECEIVER_STATE_CHANGED onMessageReceiverStateChangedCallback;
static MESSAGE_RECEIVER_HANDLE messagereceiver_create_return = NULL;
static void* msg_recv_ctx = NULL;
MESSAGE_RECEIVER_HANDLE my_messagereceiver_create(LINK_HANDLE link, ON_MESSAGE_RECEIVER_STATE_CHANGED on_message_receiver_state_changed, void* context)
{
    (void)link;
    msg_recv_ctx = context;
    onMessageReceiverStateChangedCallback = on_message_receiver_state_changed;
    messagereceiver_create_return = (MESSAGE_RECEIVER_HANDLE)malloc(2);
    return (MESSAGE_RECEIVER_HANDLE)messagereceiver_create_return;
}

void my_messagereceiver_destroy(MESSAGE_RECEIVER_HANDLE message_receiver)
{
    if (message_receiver != NULL)
    {
        free(message_receiver);
    }
}

static ON_MESSAGE_SEND_COMPLETE onMessageSendCompleteCallback;
static ASYNC_OPERATION_HANDLE my_messagesender_send_async(MESSAGE_SENDER_HANDLE message_sender, MESSAGE_HANDLE message, ON_MESSAGE_SEND_COMPLETE on_message_send_complete, void* callback_context, tickcounter_ms_t timeout)
{
    (void)timeout;
    (void)message;
    (void)message_sender;
    (void)callback_context;
    onMessageSendCompleteCallback = on_message_send_complete;
    return TEST_ASYNC_HANDLE;
}

static ON_MESSAGE_RECEIVED onMessageReceivedCallback;
static int my_messagereceiver_open(MESSAGE_RECEIVER_HANDLE message_receiver, ON_MESSAGE_RECEIVED on_message_received, void* callback_context)
{
    (void)message_receiver;
    (void)callback_context;
    onMessageReceivedCallback = on_message_received;
    return 0;
}

//static IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK on_feedback_message_received;

static IOTHUB_FEEDBACK_STATUS_CODE receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;
void my_on_feedback_message_received(void* context, IOTHUB_SERVICE_FEEDBACK_BATCH* feedbackBatch)
{
    (void)context;
    LIST_ITEM_HANDLE feedbackRecord = my_list_get_head_item(feedbackBatch->feedbackRecordList);
    while (feedbackRecord != NULL)
    {
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedback = (IOTHUB_SERVICE_FEEDBACK_RECORD*)my_list_item_get_value(feedbackRecord);
        receivedFeedbackStatusCode = feedback->statusCode;
        feedbackRecord = my_list_get_next_item(feedbackRecord);
    }
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#undef ENABLE_MOCKS

static const char* TEST_DEVICE_ID = "theDeviceId";
//static const char* TEST_MODULE_ID = "TestModuleId"; // Modules are not supported for sending messages.
static const char* TEST_PRIMARYKEY = "thePrimaryKey";
static const char* TEST_SECONDARYKEY = "theSecondaryKey";
static const char* TEST_GENERATIONID = "theGenerationId";
static const char* TEST_ETAG = "theEtag";

static char* TEST_HOSTNAME = "theHostName";
static char* TEST_IOTHUBNAME = "theIotHubName";
static char* TEST_IOTHUBSUFFIX = "theIotHubSuffix";
static char* TEST_SHAREDACCESSKEY = "theSharedAccessKey";
static char* TEST_SHAREDACCESSKEYNAME = "theSharedAccessKeyName";
static char* TEST_TRUSTED_CERT = "Test_trusted_cert";

static int TEST_ISOPENED = false;

static const char* TEST_FEEDBACK_RECORD_KEY_DEVICE_ID = "deviceId";
static const char* TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID = "deviceGenerationId";
static const char* TEST_FEEDBACK_RECORD_KEY_DESCRIPTION = "description";
static const char* TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC = "enqueuedTimeUtc";
static const char* TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID = "originalMessageId";

static const char* TEST_MAP_KEYS[] = { "Key1" };
static const char* TEST_MAP_VALUES[] = { "Val1" };
const char* const ** pTEST_MAP_KEYS = (const char* const **)&TEST_MAP_KEYS;
const char* const ** pTEST_MAP_VALUES = (const char* const **)&TEST_MAP_VALUES;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static IOTHUB_SERVICE_CLIENT_AUTH TEST_IOTHUB_SERVICE_CLIENT_AUTH;
static IOTHUB_SERVICE_CLIENT_AUTH_HANDLE TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE = &TEST_IOTHUB_SERVICE_CLIENT_AUTH;

typedef struct TEST_SASL_PLAIN_CONFIG_TAG
{
    const char* authcid;
    const char* passwd;
    const char* authzid;
} TEST_SASL_PLAIN_CONFIG;

static void* TEST_VOID_PTR = (void*)0x5454;
static char* TEST_CHAR_PTR = "TestString";
static const char* TEST_CONST_CHAR_PTR = "TestConstPtrString";
static TEST_SASL_PLAIN_CONFIG TEST_SASL_PLAIN_CONFIG_DATA;

static SASL_MECHANISM_INTERFACE_DESCRIPTION* TEST_SASL_MECHANISM_INTERFACE_DESCRIPTION = (SASL_MECHANISM_INTERFACE_DESCRIPTION*)0x4242;
static SASL_MECHANISM_HANDLE TEST_SASL_MECHANISM_HANDLE = (SASL_MECHANISM_HANDLE)0x4343;
static const IO_INTERFACE_DESCRIPTION* TEST_IO_INTERFACE_DESCRIPTION = (const IO_INTERFACE_DESCRIPTION*)0x4444;
static XIO_HANDLE TEST_XIO_HANDLE = (XIO_HANDLE)0x4545;
static CONNECTION_HANDLE TEST_CONNECTION_HANDLE = (CONNECTION_HANDLE)0x4646;
static SESSION_HANDLE TEST_SESSION_HANDLE = (SESSION_HANDLE)0x4747;
static AMQP_VALUE TEST_AMQP_VALUE = (AMQP_VALUE)0x4848;
static AMQP_VALUE TEST_AMQP_VALUE_NULL = (AMQP_VALUE)NULL;
static LINK_HANDLE TEST_LINK_HANDLE = (LINK_HANDLE)0x4949;
static IOTHUB_SEND_COMPLETE_CALLBACK TEST_IOTHUB_SEND_COMPLETE_CALLBACK = (IOTHUB_SEND_COMPLETE_CALLBACK)0x5353;
static BINARY_DATA TEST_BINARY_DATA_INST;
static PROPERTIES_HANDLE TEST_PROPERTIES_HANDLE = (PROPERTIES_HANDLE)0x5656;
static PROPERTIES_HANDLE TEST_PROPERTIES_HANDLE_NULL = (PROPERTIES_HANDLE)NULL;
static MESSAGE_HANDLE TEST_MESSAGE_HANDLE = (MESSAGE_HANDLE)0x5858;
static IOTHUB_OPEN_COMPLETE_CALLBACK TEST_IOTHUB_OPEN_COMPLETE_CALLBACK;
static IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK = (IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK)0x6060;
static MESSAGE_SENDER_STATE TEST_MESSAGE_SENDER_STATE = (MESSAGE_SENDER_STATE)0x6161;
static IOTHUB_MESSAGING_RESULT TEST_IOTHUB_MESSAGING_RESULT = (IOTHUB_MESSAGING_RESULT)0x6767;
static JSON_Value* TEST_JSON_VALUE = (JSON_Value*)0x5050;
static JSON_Object* TEST_JSON_OBJECT = (JSON_Object*)0x5151;
static JSON_Array* TEST_JSON_ARRAY = (JSON_Array*)0x5252;
static JSON_Status TEST_JSON_STATUS = 0;
static AMQP_VALUE TEST_AMQP_MAP = ((AMQP_VALUE)0x6258);
static MAP_HANDLE TEST_MAP_HANDLE = (MAP_HANDLE)0x103;
static IOTHUB_MESSAGE_HANDLE TEST_IOTHUB_MESSAGE_HANDLE = (IOTHUB_MESSAGE_HANDLE)0x4242;

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

BEGIN_TEST_SUITE(iothub_messaging_ll_ut)

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

        REGISTER_TYPE(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_RESULT);
        REGISTER_UMOCK_VALUE_TYPE(BINARY_DATA);

        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(fields, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(CONNECTION_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(LINK_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_SENDER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_RECEIVER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(LOGGER_LOG, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_LINK_ATTACHED, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_SENDER_STATE_CHANGED, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVER_STATE_CHANGED, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVED, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_NEW_ENDPOINT, void*);
        REGISTER_UMOCK_ALIAS_TYPE(sender_settle_mode, unsigned char);
        REGISTER_UMOCK_ALIAS_TYPE(role, bool);
        REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROPERTIES_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_SEND_COMPLETE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SASL_MECHANISM_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(JSON_Status, int);
        REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MAP_RESULT, int);
        REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(receiver_settle_mode, uint8_t);
        REGISTER_UMOCK_ALIAS_TYPE(tickcounter_ms_t, unsigned long long);

        REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, 42);

        REGISTER_GLOBAL_MOCK_HOOK(SASToken_Create, my_SASToken_Create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(SASToken_Create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(saslplain_get_interface, TEST_SASL_MECHANISM_INTERFACE_DESCRIPTION);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslplain_get_interface, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(saslmechanism_create, TEST_SASL_MECHANISM_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslmechanism_create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_IO_INTERFACE_DESCRIPTION);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(platform_get_default_tlsio, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(xio_create, TEST_XIO_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(saslclientio_get_interface_description, TEST_IO_INTERFACE_DESCRIPTION);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslclientio_get_interface_description, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(connection_create, TEST_CONNECTION_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(connection_create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(session_create, TEST_SESSION_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(session_set_incoming_window, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_set_incoming_window, 1);

        REGISTER_GLOBAL_MOCK_RETURN(session_set_outgoing_window, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_set_outgoing_window, 1);

        REGISTER_GLOBAL_MOCK_RETURN(messaging_create_source, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_source, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(messaging_create_target, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_target, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(link_create, TEST_LINK_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_map, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_map, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_symbol, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_symbol, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_set_map_value, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_set_map_value, 1);

        REGISTER_GLOBAL_MOCK_RETURN(link_set_attach_properties, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_attach_properties, 1);

        REGISTER_GLOBAL_MOCK_RETURN(link_set_snd_settle_mode, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_snd_settle_mode, 1);

        REGISTER_GLOBAL_MOCK_RETURN(link_set_max_message_size, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_max_message_size, 1);

        REGISTER_GLOBAL_MOCK_HOOK(messagesender_create, my_messagesender_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(messagesender_destroy, my_messagesender_destroy);

        REGISTER_GLOBAL_MOCK_RETURN(messagesender_open, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_open, 1);

        REGISTER_GLOBAL_MOCK_RETURN(messaging_create_source, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_source, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(messaging_create_target, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_target, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(link_set_rcv_settle_mode, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_rcv_settle_mode, 1);

        REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_create, my_messagereceiver_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_destroy, my_messagereceiver_destroy);

        REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_open, my_messagereceiver_open);
        REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_open, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_open, 1);

        REGISTER_GLOBAL_MOCK_RETURN(message_create, TEST_MESSAGE_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_ERROR);

        REGISTER_GLOBAL_MOCK_RETURN(message_add_body_amqp_data, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_add_body_amqp_data, 1);

        REGISTER_GLOBAL_MOCK_RETURN(properties_create, TEST_PROPERTIES_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_string, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_string, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(properties_set_to, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_set_to, 1);

        REGISTER_GLOBAL_MOCK_RETURN(message_set_properties, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_properties, 1);

        REGISTER_GLOBAL_MOCK_HOOK(messagesender_send_async, my_messagesender_send_async);
        REGISTER_GLOBAL_MOCK_RETURN(messagesender_send_async, (ASYNC_OPERATION_HANDLE)0x64);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_send_async, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(json_parse_string, TEST_JSON_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(json_value_get_array, TEST_JSON_ARRAY);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_array, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(json_array_get_object, TEST_JSON_OBJECT);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_array_get_object, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(json_array_get_count, 42);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_array_get_count, 0);

        REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, TEST_CONST_CHAR_PTR);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_create, my_list_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_create, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_add, my_list_add);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_head_item, my_list_get_head_item);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_get_head_item, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_next_item, my_list_get_next_item);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_get_next_item, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_item_get_value, my_list_item_get_value);

        REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_destroy, my_list_destroy);

        REGISTER_GLOBAL_MOCK_HOOK(message_get_body_amqp_data_in_place, my_message_get_body_amqp_data_in_place);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_amqp_data_in_place, 1);

        REGISTER_GLOBAL_MOCK_RETURN(messaging_delivery_accepted, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_delivery_accepted, TEST_AMQP_VALUE_NULL);

        REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, "");
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetContentType, IOTHUBMESSAGE_BYTEARRAY);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetContentType, IOTHUBMESSAGE_UNKNOWN);

        REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_Properties, TEST_MAP_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_Properties, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(Map_GetInternals, MAP_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetInternals, MAP_ERROR);

        REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetMessageId, TEST_CONST_CHAR_PTR);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetMessageId, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetCorrelationId, TEST_CONST_CHAR_PTR);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetCorrelationId, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, my_on_feedback_message_received);
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

        TEST_IOTHUB_SERVICE_CLIENT_AUTH.hostname = TEST_HOSTNAME;
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.iothubName = TEST_IOTHUBNAME;
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.iothubSuffix = TEST_IOTHUBSUFFIX;
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.keyName = TEST_SHAREDACCESSKEYNAME;
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.sharedAccessKey = TEST_SHAREDACCESSKEY;

        onMessageSenderStateChangedCallback = NULL;
        onMessageReceiverStateChangedCallback = NULL;
        onMessageSendCompleteCallback = NULL;
        onMessageReceivedCallback = NULL;
        messagereceiver_create_return = NULL;
        messagesender_create_return = NULL;

        receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_001: [ If the serviceClientHandle input parameter is NULL IoTHubMessaging_LL_Create shall return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Create_return_null_if_input_parameter_serviceClientHandle_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_064: [ If any member of the serviceClientHandle input parameter is NULL IoTHubMessaging_LL_Create shall return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Create_return_null_if_input_parameter_serviceClientHandle_hostName_is_NULL)
    {
        // arrange
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.hostname = NULL;

        // act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

        // assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_064: [ If any member of the serviceClientHandle input parameter is NULL IoTHubMessaging_LL_Create shall return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Create_return_null_if_input_parameter_serviceClientHandle_iothubName_is_NULL)
    {
        //// arrange
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.iothubName = NULL;

        // act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

        // assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_064: [ If any member of the serviceClientHandle input parameter is NULL IoTHubMessaging_LL_Create shall return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Create_return_null_if_input_parameter_serviceClientHandle_iothubSuffix_is_NULL)
    {
        //// arrange
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.iothubSuffix = NULL;

        // act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

        // assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_064: [ If any member of the serviceClientHandle input parameter is NULL IoTHubMessaging_LL_Create shall return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Create_return_null_if_input_parameter_serviceClientHandle_keyName_is_NULL)
    {
        //// arrange
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.keyName = NULL;

        // act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

        // assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_064: [ If any member of the serviceClientHandle input parameter is NULL IoTHubMessaging_LL_Create shall return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Create_return_null_if_input_parameter_serviceClientHandle_sharedAccessKey_is_NULL)
    {
        //// arrange
        TEST_IOTHUB_SERVICE_CLIENT_AUTH.sharedAccessKey = NULL;

        // act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

        // assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_002: [ IoTHubMessaging_LL_Create shall allocate memory for a new messaging instance ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_004: [ If the allocation and creation is successful, IoTHubMessaging_LL_Create shall return with the messaging instance, a non-NULL value ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_065: [ IoTHubMessaging_LL_Create shall allocate memory and copy hostName to result->hostName by calling mallocAndStrcpy_s ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_067: [ IoTHubMessaging_LL_Create shall allocate memory and copy iothubName to result->iothubName by calling mallocAndStrcpy_s ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_069: [ IoTHubMessaging_LL_Create shall allocate memory and copy iothubSuffix to result->iothubSuffix by calling mallocAndStrcpy_s ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_071: [ IoTHubMessaging_LL_Create shall allocate memory and copy sharedAccessKey to result->sharedAccessKey by calling mallocAndStrcpy_s ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_073: [ IoTHubMessaging_LL_Create shall allocate memory and copy keyName to result->keyName by calling mallocAndStrcpy_s ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_075: [ IoTHubMessaging_LL_Create shall set messaging isOpened flag to false ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_076: [ If create successfull IoTHubMessaging_LL_Create shall save the callback data return the valid messaging handle ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Create_happy_path)
    {
        // arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_HOSTNAME));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBNAME));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBSUFFIX));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEY));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEYNAME));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        // act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

        // assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubMessaging_LL_Destroy(result);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_003: [ If the allocation failed, IoTHubMessaging_LL_Create shall return NULL ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_066: [ If the mallocAndStrcpy_s fails, IoTHubMessaging_LL_Create shall do clean up and return NULL ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_068: [ If the mallocAndStrcpy_s fails, IoTHubMessaging_LL_Create shall do clean up and return NULL ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_070: [ If the mallocAndStrcpy_s fails, IoTHubMessaging_LL_Create shall do clean up and return NULL ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_072: [ If the mallocAndStrcpy_s fails, IoTHubMessaging_LL_Create shall do clean up and return NULL ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_074: [ If the mallocAndStrcpy_s fails, IoTHubMessaging_LL_Create shall do clean up and return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Create_non_happy_path)
    {
        // arrange
        int umockc_result = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, umockc_result);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_HOSTNAME))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBNAME))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBSUFFIX))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEYNAME))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEY))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        umock_c_negative_tests_snapshot();

        //act
        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            /// arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// act
            IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

            /// assert
            ASSERT_ARE_EQUAL(void_ptr, NULL, result);

            ///cleanup
        }
        umock_c_negative_tests_deinit();
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_005: [ If the messagingHandle input parameter is NULL IoTHubMessaging_LL_Destroy shall return ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Destroy_return_if_input_parameter_messagingHandle_is_NULL)
    {
        // arrange

        // act
        IoTHubMessaging_LL_Destroy(NULL);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_006: [ If the messagingHandle input parameter is not NULL IoTHubMessaging_LL_Destroy shall free all resources (memory) allocated by IoTHubMessaging_LL_Create ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Destroy_do_clean_up_and_return_if_input_parameter_messagingHandle_is_not_NULL)
    {
        // arrange
        IOTHUB_MESSAGING_HANDLE handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        // act
        IoTHubMessaging_LL_Destroy(handle);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_007: [ If the messagingHandle input parameter is NULL IoTHubMessaging_LL_Open shall return IOTHUB_MESSAGING_INVALID_ARG ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Open_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    {
        // arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(NULL, TEST_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)0x4242);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_008: [ If messaging is already opened IoTHubMessaging_LL_Open return shall IOTHUB_MESSAGING_OK ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Open_return_IOTHUB_MESSAGING_INVALID_OK_if_input_parameter_messagingHandle_isOpened_true)
    {
        // arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        umock_c_reset_all_calls();

        //act
        result = IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        // cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    static void addSetLinkCalls(void)
    {
        EXPECTED_CALL(amqpvalue_create_map());
        EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
        EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    }

    static void callsForOpen(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));    // Call #0

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_construct(TEST_HOSTNAME));
        STRICT_EXPECTED_CALL(STRING_construct(TEST_SHAREDACCESSKEY));
        STRICT_EXPECTED_CALL(STRING_construct(TEST_SHAREDACCESSKEYNAME));     // Call #5

        STRICT_EXPECTED_CALL(SASToken_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetReturn(TEST_CHAR_PTR);

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))     // Call #10
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(saslplain_get_interface());
        STRICT_EXPECTED_CALL(saslmechanism_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());     // Call #15

        STRICT_EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(saslclientio_get_interface_description());

        STRICT_EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(connection_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(session_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(session_set_incoming_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG))     // Call #21
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(session_set_outgoing_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, TEST_AMQP_VALUE_NULL, TEST_AMQP_VALUE_NULL))
            .IgnoreAllArguments();      // Call #25
        addSetLinkCalls();
        STRICT_EXPECTED_CALL(link_set_snd_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG))     // Call #34
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(messagesender_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(messagesender_open(IGNORED_NUM_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();      // Call #39
        addSetLinkCalls();
        STRICT_EXPECTED_CALL(link_set_rcv_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG))     // Call #48
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(messagereceiver_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(messagereceiver_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));     // Call #51
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_010: [ IoTHubMessaging_LL_Open shall create uAMQP PLAIN SASL mechanism by calling saslmechanism_create with the sasl plain interface ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_011: [ IoTHubMessaging_LL_Open shall create uAMQP TLSIO by calling the xio_create ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_012: [ IoTHubMessaging_LL_Open shall create uAMQP SASL IO by calling the xio_create with the previously created SASL mechanism and TLSIO] */
    /*Tests_SRS_IOTHUBMESSAGING_12_013: [ IoTHubMessaging_LL_Open shall create uAMQP connection by calling the connection_create with the previously created SASL IO ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_014: [ IoTHubMessaging_LL_Open shall create uAMQP session by calling the session_create ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_015: [ IoTHubMessaging_LL_Open shall set the AMQP incoming window to UINT32 maximum value by calling session_set_incoming_window ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_016: [ IoTHubMessaging_LL_Open shall set the AMQP outgoing window to UINT32 maximum value by calling session_set_outgoing_window ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_018: [ IoTHubMessaging_LL_Open shall create uAMQP sender link by calling the link_create ] */
    /*Tests_SRS_IOTHUBMESSAGING_06_001: [ IoTHubMessaging_LL_Open shall add the version property to the sender link by calling the link_set_attach_properties ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_019: [ IoTHubMessaging_LL_Open shall set the AMQP sender link settle mode to sender_settle_mode_unsettled  by calling link_set_snd_settle_mode ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_021: [ IoTHubMessaging_LL_Open shall create uAMQP messaging source for sender by calling the messaging_create_source ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_022: [ IoTHubMessaging_LL_Open shall create uAMQP messaging target for sender by calling the messaging_create_target ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_023: [ IoTHubMessaging_LL_Open shall create uAMQP message sender by calling the messagesender_create with the created sender link and the local IoTHubMessaging_LL_SenderStateChanged callback ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_024: [ IoTHubMessaging_LL_Open shall create uAMQP receiver link by calling the link_create ] */
    /*Tests_SRS_IOTHUBMESSAGING_06_002: [ IoTHubMessaging_LL_Open shall add the version property to the receiver by calling the link_set_attach_properties ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_025: [ IoTHubMessaging_LL_Open shall set the AMQP receiver link settle mode to receiver_settle_mode_first by calling link_set_rcv_settle_mode ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_027: [ IoTHubMessaging_LL_Open shall create uAMQP messaging source for receiver by calling the messaging_create_source ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_028: [ IoTHubMessaging_LL_Open shall create uAMQP messaging target for receiver by calling the messaging_create_target ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_029: [ IoTHubMessaging_LL_Open shall create uAMQP message receiver by calling the messagereceiver_create with the created sender link and the local IoTHubMessaging_LL_ReceiverStateChanged callback ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_031: [ If all of the uAMQP call return 0 (success) IoTHubMessaging_LL_Open shall return IOTHUB_MESSAGING_OK ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Open_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        umock_c_reset_all_calls();

        callsForOpen();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)0x4242);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_030: [ If any of the uAMQP call fails IoTHubMessaging_LL_Open shall return IOTHUB_MESSAGING_ERROR ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Open_non_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        umock_c_reset_all_calls();

        int umockc_result = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, umockc_result);

        //arrange
        callsForOpen();

        umock_c_negative_tests_snapshot();

        //act
        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            // arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            if (
                (i != 7) &&
                (i != 9) &&
                (i != 10) &&
                (i != 11) &&
                (i != 12) &&
                (i != 31) &&
                (i != 32) &&
                (i != 33) &&
                (i != 45) &&
                (i != 46) &&
                (i != 47) &&
                (i < 51)
                )
            {
                IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_IOTHUB_OPEN_COMPLETE_CALLBACK, NULL);

                //assert
                ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result);

                //cleanup
            }
        }
        umock_c_negative_tests_deinit();

        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_032: [ If the messagingHandle input parameter is NULL IoTHubMessaging_LL_Close shall return IOTHUB_MESSAGING_INVALID_ARG ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Close_return_if_input_parameter_messagingHandle_is_NULL)
    {
        // arrange

        // act
        IoTHubMessaging_LL_Close(NULL);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_033: [ IoTHubMessaging_LL_Close destroy the AMQP transportconnection by calling link_destroy, session_destroy, connection_destroy, xio_destroy, saslmechanism_destroy ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Close_happy_path)
    {
        // arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(messagesender_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagereceiver_destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(link_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(session_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(connection_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(saslmechanism_destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        // act
        IoTHubMessaging_LL_Close(iothub_messaging_handle);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_034: [ IoTHubMessaging_LL_Send shall verify the messagingHandle, deviceId, message input parameters and if any of them are NULL then return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(NULL, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_034: [ IoTHubMessaging_LL_Send shall verify the messagingHandle, deviceId, message input parameters and if any of them are NULL then return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_deviceId_is_NULL)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, NULL, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_034: [ IoTHubMessaging_LL_Send shall verify the messagingHandle, deviceId, message input parameters and if any of them are NULL then return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_message_is_NULL)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, NULL, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_035: [ IoTHubMessaging_LL_Send shall verify if the AMQP messaging has been established by a successfull call to _Open and if it is not then return IOTHUB_MESSAGING_ERROR ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_ERROR_if_messaging_is_not_opened)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_ERROR, result);

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_038: [ IoTHubMessaging_LL_Send shall set the uAMQP message properties to the given message properties by calling message_set_properties ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_036: [ IoTHubMessaging_LL_Send shall create a uAMQP message by calling message_create ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_037: [ IoTHubMessaging_LL_Send shall set the uAMQP message body to the given message content by calling message_add_body_amqp_data ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_038: [ IoTHubMessaging_LL_Send shall set the uAMQP message properties to the given message properties by calling message_set_properties ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_039: [ IoTHubMessaging_LL_Send shall call uAMQP messagesender_send with the created message with IoTHubMessaging_LL_SendComplete callback by which IoTHubMessaging is notified of completition of send ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_041: [ If all uAMQP call return 0 then IoTHubMessaging_LL_Send shall return IOTHUB_MESSAGING_OK  ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_079: [** The uAMQP message properties shall be retrieved using message_get_properties ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_080 : [** If UAMQP message properties were not present then a new properties container shall be created using properties_create ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_081 : [** Message - id from the IOTHUB_MESSAGE shall be read using IoTHubMessage_GetMessageId ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_082 : [** As message - id is optional field, if it is not set on the IOTHUB_MESSAGE, message_create_from_iothub_message shall ignore it and continue normally ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_083 : [** The message - id AMQP_VALUE shall be set on the uAMQP message using properties_set_message_id ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_084 : [** Correlation - id from the IOTHUB_MESSAGE shall be read using IoTHubMessage_GetCorrelationId ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_085 : [** As correlation - id is optional field, if it is not set on the IOTHUB_MESSAGE, message_create_from_iothub_message() shall ignore it and continue normally ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_086 : [** The correlation - id AMQP_VALUE shall be set on the uAMQP message using properties_set_correlation_id ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_087 : [** IoTHubMessaging_LL_Send shall set the uAMQP message TO property to the given message properties by calling properties_set_to ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_088 : [** The IOTHUB_MESSAGE_HANDLE properties shall be obtained by calling IoTHubMessage_Properties ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_089 : [** The actual keys and values, as well as the number of properties shall be obtained by calling Map_GetInternals on the handle obtained from IoTHubMessage_Properties ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_090 : [** If the number of properties is greater than 0, message_create_from_iothub_message() shall iterate through all the properties and add them to the uAMQP message ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_091 : [** A uAMQP property map shall be created by calling amqpvalue_create_map ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_092 : [** An AMQP_VALUE instance shall be created using amqpvalue_create_string() to hold each uAMQP property name ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_093 : [** An AMQP_VALUE instance shall be created using amqpvalue_create_string() to hold each uAMQP property value ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_094 : [** The property name and value(AMQP_VALUE instances) shall be added to the uAMQP property map by calling amqpvalue_map_set_value ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_095 : [** After adding the property name and value to the uAMQP property map, both AMQP_VALUE instances shall be destroyed using amqpvalue_destroy ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_096 : [** If no errors occurred processing the properties, the uAMQP properties map shall be set on the uAMQP message by calling message_set_application_properties ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_097 : [** If the number of properties is 0, no application properties shall be set on the uAMQP message and message_create_from_iothub_message() shall return with success ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Send_happy_path)
    {
        //arrange
        size_t number_of_arguments = 1;
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(message_create());
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(message_get_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .CopyOutArgumentBuffer_properties(&TEST_PROPERTIES_HANDLE_NULL, sizeof(TEST_PROPERTIES_HANDLE_NULL));
        STRICT_EXPECTED_CALL(properties_create());

        STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .SetReturn(TEST_CONST_CHAR_PTR);
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(properties_set_message_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .SetReturn(TEST_CONST_CHAR_PTR);
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(properties_set_correlation_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(properties_set_to(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(message_set_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(properties_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE))
            .IgnoreAllArguments()
            .SetReturn(TEST_MAP_HANDLE);

        STRICT_EXPECTED_CALL(Map_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .CopyOutArgumentBuffer_keys(&pTEST_MAP_KEYS, sizeof(pTEST_MAP_KEYS))
            .CopyOutArgumentBuffer_values(&pTEST_MAP_VALUES, sizeof(pTEST_MAP_VALUES))
            .CopyOutArgumentBuffer_count(&number_of_arguments, sizeof(size_t))
            .SetReturn(MAP_OK);

        STRICT_EXPECTED_CALL(amqpvalue_create_map());

        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(message_set_application_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(messagesender_send_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        //act
        IOTHUB_MESSAGING_RESULT result;
        result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_040: [ If any of the uAMQP call fails IoTHubMessaging_LL_Send shall return IOTHUB_MESSAGING_ERROR ] */
    TEST_FUNCTION(IoTHubMessaging_LL_Send_non_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        umock_c_reset_all_calls();

        int umockc_result = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, umockc_result);

        size_t doNotFailCalls[] =
        {
            6,  /*contentType*/
            8,  /*contentType*/
            9,  /*message_get_properties*/
            10, /*properties_create*/
            11, /*properties_set_message_id*/
            12, /*properties_set_correlation_id*/
            14, /*message_set_application_properties*/
            15, /*amqpvalue_destroy*/
            18, /*amqpvalue_destroy*/
            19, /*amqpvalue_destroy*/
            25, /*properties_destroy*/
            26, /*amqpvalue_destroy*/
            27, /*amqpvalue_destroy*/
            28, /*amqpvalue_destroy*/
            30  /*gballoc_free*/
        };

        size_t number_of_arguments = 1;

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(message_create());
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(message_get_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .CopyOutArgumentBuffer_properties(&TEST_PROPERTIES_HANDLE_NULL, sizeof(TEST_PROPERTIES_HANDLE_NULL));
        STRICT_EXPECTED_CALL(properties_create());

        STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .SetReturn(TEST_CONST_CHAR_PTR);
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(properties_set_message_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .SetReturn(TEST_CONST_CHAR_PTR);
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(properties_set_correlation_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(properties_set_to(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(message_set_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(properties_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE))
            .IgnoreAllArguments()
            .SetReturn(TEST_MAP_HANDLE);

        STRICT_EXPECTED_CALL(Map_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .CopyOutArgumentBuffer_keys(&pTEST_MAP_KEYS, sizeof(pTEST_MAP_KEYS))
            .CopyOutArgumentBuffer_values(&pTEST_MAP_VALUES, sizeof(pTEST_MAP_VALUES))
            .CopyOutArgumentBuffer_count(&number_of_arguments, sizeof(size_t))
            .SetReturn(MAP_OK);

        STRICT_EXPECTED_CALL(amqpvalue_create_map());

        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(message_set_application_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(messagesender_send_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        umock_c_negative_tests_snapshot();

        //act
        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            umock_c_negative_tests_reset();
            size_t j;
            for ( j = 0; j < sizeof(doNotFailCalls) / sizeof(doNotFailCalls[0]); j++)
            {
                if (doNotFailCalls[j] == i)
                {
                    break;
                }
            }

            if (j == sizeof(doNotFailCalls) / sizeof(doNotFailCalls[0]))
            {
                umock_c_negative_tests_fail_call(i);

                IOTHUB_MESSAGING_RESULT result;

                /* If modules are re-enabled, re-enable this code and add testing_module paramater to this function
                if (testing_module == true)
                {
                    result = IoTHubMessaging_LL_SendModule(TEST_IOTHUB_MESSAGING_HANDLE, TEST_CONST_CHAR_PTR, TEST_MODULE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);
                }
                */
                result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

                //assert
                ASSERT_ARE_NOT_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, result);
            }

        }
        umock_c_negative_tests_deinit();

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

#if 0
    // Module message support was removed from product.  If re-enabled, bring back in this code.
    /*Tests_SRS_IOTHUBMESSAGING_12_034: [ IoTHubMessaging_LL_Send shall verify the messagingHandle, deviceId, message input parameters and if any of them are NULL then return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SendModule_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SendModule(NULL, TEST_CONST_CHAR_PTR, TEST_MODULE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_034: [ IoTHubMessaging_LL_Send shall verify the messagingHandle, deviceId, message input parameters and if any of them are NULL then return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SendModule_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_deviceId_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SendModule(TEST_IOTHUB_MESSAGING_HANDLE, NULL, TEST_MODULE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_034: [ IoTHubMessaging_LL_Send shall verify the messagingHandle, deviceId, message input parameters and if any of them are NULL then return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SendModule_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_moduleId_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SendModule(TEST_IOTHUB_MESSAGING_HANDLE, TEST_CONST_CHAR_PTR, NULL, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    }


    /*Tests_SRS_IOTHUBMESSAGING_12_034: [ IoTHubMessaging_LL_Send shall verify the messagingHandle, deviceId, message input parameters and if any of them are NULL then return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SendModule_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_message_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SendModule(TEST_IOTHUB_MESSAGING_HANDLE, TEST_CONST_CHAR_PTR, TEST_MODULE_ID, NULL, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_035: [ IoTHubMessaging_LL_Send shall verify if the AMQP messaging has been established by a successfull call to _Open and if it is not then return IOTHUB_MESSAGING_ERROR ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SendModule_return_IOTHUB_MESSAGING_ERROR_if_messaging_is_not_opened)
    {
        //arrange
        TEST_IOTHUB_MESSAGING_DATA.isOpened = false;

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SendModule(TEST_IOTHUB_MESSAGING_HANDLE, TEST_CONST_CHAR_PTR, TEST_MODULE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_ERROR, result);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SendModule_happy_path)
    {
        IoTHubMessaging_LL_Send_happy_path(true);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SendModule_non_happy_path)
    {
        IoTHubMessaging_LL_Send_non_happy_path(true);
    }
#endif

    /*Tests_SRS_IOTHUBMESSAGING_12_042: [ IoTHubMessaging_LL_SetCallbacks shall verify the messagingHandle input parameter and if it is NULL then return NULL ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SetFeedbackMessageCallback_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetFeedbackMessageCallback(NULL, TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_043: [ IoTHubMessaging_LL_SetCallbacks shall save the given feedbackMessageReceivedCallback to use them in local callbacks ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_044: [ IoTHubMessaging_LL_Open shall return IOTHUB_MESSAGING_OK after the callbacks have been set ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SetFeedbackMessageCallback_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, TEST_VOID_PTR);

        //assert
        //ASSERT_IS_TRUE(TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK == TEST_IOTHUB_MESSAGING_DATA.callback_data->feedbackMessageCallback);
        //ASSERT_ARE_EQUAL(void_ptr, TEST_IOTHUB_MESSAGING_DATA.callback_data->feedbackUserContext, TEST_VOID_PTR);
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, result);

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_045: [ IoTHubMessaging_LL_DoWork shall verify if uAMQP transport has been initialized and if it is not then return immediately ] */
    TEST_FUNCTION(IoTHubMessaging_LL_DoWork_return_if_input_parameter_messagingHandle_is_NULL)
    {
        //arrange

        //act
        IoTHubMessaging_LL_DoWork(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_046: [ IoTHubMessaging_LL_DoWork shall call uAMQP connection_dowork ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_047: [ IoTHubMessaging_LL_SendMessageComplete callback given to messagesender_send will be called with MESSAGE_SEND_RESULT ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_048: [ If message has been received the IoTHubMessaging_LL_FeedbackMessageReceived callback given to messagesender_receive will be called with the received MESSAGE_HANDLE ] */
    TEST_FUNCTION(IoTHubMessaging_LL_DoWork_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        //act
        IoTHubMessaging_LL_DoWork(iothub_messaging_handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_049: [ IoTHubMessaging_LL_SenderStateChanged shall save the new_state to local variable ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_050: [ If both sender and receiver state is open IoTHubMessaging_LL_SenderStateChanged shall set the isOpened local variable to true ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_call_user_callback)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        onMessageReceiverStateChangedCallback(msg_recv_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_IDLE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPEN;
        MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

        //act
        onMessageSenderStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_049: [ IoTHubMessaging_LL_SenderStateChanged shall save the new_state to local variable ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_051: [ If neither sender_state nor receiver_state is open IoTHubMessaging_LL_SenderStateChanged shall set the local isOpened variable to false ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_messaging_is_not_opened)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);

        umock_c_reset_all_calls();

        MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPENING;
        MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

        //act
        onMessageSenderStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_049: [ IoTHubMessaging_LL_SenderStateChanged shall save the new_state to local variable ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_051: [ If neither sender_state nor receiver_state is open IoTHubMessaging_LL_SenderStateChanged shall set the local isOpened variable to false ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_openCompleteCompleteCallback_is_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        onMessageReceiverStateChangedCallback(msg_recv_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_IDLE);
        umock_c_reset_all_calls();

        MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPEN;
        MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

        //act
        onMessageSenderStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_052: [ IoTHubMessaging_LL_ReceiverStateChanged shall save the new_state to local variable ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_053: [ If both sender and receiver state is open IoTHubMessaging_LL_ReceiverStateChanged shall set the isOpened local variable to true ] */
    TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_call_user_callback)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        onMessageSenderStateChangedCallback(msg_send_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPEN;
        MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

        //act
        onMessageReceiverStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_052: [ IoTHubMessaging_LL_ReceiverStateChanged shall save the new_state to local variable ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_054: [ If neither sender_state nor receiver_state is open IoTHubMessaging_LL_ReceiverStateChanged shall set the local isOpened variable to false ] */
    TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_messaging_is_not_opened)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        onMessageSenderStateChangedCallback(msg_send_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        umock_c_reset_all_calls();

        MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPENING;
        MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

        //act
        onMessageReceiverStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_052: [ IoTHubMessaging_LL_ReceiverStateChanged shall save the new_state to local variable ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_054: [ If neither sender_state nor receiver_state is open IoTHubMessaging_LL_ReceiverStateChanged shall set the local isOpened variable to false ] */
    TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_openCompleteCompleteCallback_is_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
        onMessageSenderStateChangedCallback(msg_send_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        umock_c_reset_all_calls();

        MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPEN;
        MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

        //act
        onMessageReceiverStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_056: [ If context is NULL IoTHubMessaging_LL_SendMessageComplete shall return ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_055: [ If context is not NULL and IoTHubMessaging_LL_SendMessageComplete shall call user callback with user context and messaging result ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SendMessageComplete_context_is_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK, (void*)1);

        umock_c_reset_all_calls();

        MESSAGE_SEND_RESULT send_result = MESSAGE_SEND_OK;

        //act
        onMessageSendCompleteCallback((void*)NULL, send_result, TEST_AMQP_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_056: [ If context is NULL IoTHubMessaging_LL_SendMessageComplete shall return ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_055: [ If context is not NULL and IoTHubMessaging_LL_SendMessageComplete shall call user callback with user context and messaging result ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SendMessageComplete_sendCompleteCallback_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE, NULL, NULL);

        umock_c_reset_all_calls();

        MESSAGE_SEND_RESULT send_result = MESSAGE_SEND_OK;


        //act
        onMessageSendCompleteCallback(iothub_messaging_handle, send_result, TEST_AMQP_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_056: [ If context is NULL IoTHubMessaging_LL_SendMessageComplete shall return ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_055: [ If context is not NULL and IoTHubMessaging_LL_SendMessageComplete shall call user callback with user context and messaging result ] */
    TEST_FUNCTION(IoTHubMessaging_LL_SendMessageCompleteDevice_call_to_user_callback)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        /* If modules are re-enabled, re-enable this code and add testing_module paramater to this function
        if (testing_module == true)
        {
            (void)IoTHubMessaging_LL_SendModule(iothub_messaging_handle, TEST_DEVICE_ID, TEST_MODULE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK, (void*)1);
        }
        */
        (void)IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK, (void*)1);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK(IGNORED_PTR_ARG, IOTHUB_MESSAGING_OK));

        MESSAGE_SEND_RESULT send_result = MESSAGE_SEND_OK;

        //act
        onMessageSendCompleteCallback((void*)iothub_messaging_handle, send_result, TEST_AMQP_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

#if 0
    // Modules message sending not available
    TEST_FUNCTION(IoTHubMessaging_LL_SendModuleMessageComplete_call_to_user_callback)
    {
        IoTHubMessaging_LL_SendMessageCompleteDevice_call_to_user_callback(true);
    }
#endif


    /*Tests_SRS_IOTHUBMESSAGING_12_057: [ If context is NULL IoTHubMessaging_LL_FeedbackMessageReceived shall do nothing and return delivery_accepted ] */
    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_context_is_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        //act
        void* amqp_result = (void*)onMessageReceivedCallback(NULL, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_IS_NOT_NULL(amqp_result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_058: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall get the content string of the message by calling message_get_body_amqp_data_in_place ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_059: [ IoTHubMessaging_LL_FeedbackMessageReceived shall parse the response JSON to IOTHUB_SERVICE_FEEDBACK_BATCH struct ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_060: [ IoTHubMessaging_LL_FeedbackMessageReceived shall use the following parson APIs to parse the response string: json_parse_string, json_value_get_object, json_object_get_string, json_object_dotget_string  ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_062: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall call IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK with the received IOTHUB_SERVICE_FEEDBACK_BATCH ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_078: [** IoTHubMessaging_LL_FeedbackMessageReceived shall do clean up before exits ] */
    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_success)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

        receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;

        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
            .SetReturn(TEST_JSON_OBJECT);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
            .SetReturn("deviceId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
            .SetReturn("generationId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
            .SetReturn("SuCcEsS");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
            .SetReturn("time");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
            .SetReturn("originalMessageId");


        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG));

        //act
        onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS, receivedFeedbackStatusCode);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_058: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall get the content string of the message by calling message_get_body_amqp_data_in_place ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_059: [ IoTHubMessaging_LL_FeedbackMessageReceived shall parse the response JSON to IOTHUB_SERVICE_FEEDBACK_BATCH struct ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_060: [ IoTHubMessaging_LL_FeedbackMessageReceived shall use the following parson APIs to parse the response string: json_parse_string, json_value_get_object, json_object_get_string, json_object_dotget_string  ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_062: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall call IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK with the received IOTHUB_SERVICE_FEEDBACK_BATCH ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_078: [** IoTHubMessaging_LL_FeedbackMessageReceived shall do clean up before exits ] */
    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_expired)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

        receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;

        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
            .SetReturn(TEST_JSON_OBJECT);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
            .SetReturn("deviceId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
            .SetReturn("generationId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
            .SetReturn("ExPiReD");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
            .SetReturn("time");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
            .SetReturn("originalMessageId");


        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG));

        //act
        onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_EXPIRED, receivedFeedbackStatusCode);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_058: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall get the content string of the message by calling message_get_body_amqp_data_in_place ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_059: [ IoTHubMessaging_LL_FeedbackMessageReceived shall parse the response JSON to IOTHUB_SERVICE_FEEDBACK_BATCH struct ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_060: [ IoTHubMessaging_LL_FeedbackMessageReceived shall use the following parson APIs to parse the response string: json_parse_string, json_value_get_object, json_object_get_string, json_object_dotget_string  ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_062: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall call IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK with the received IOTHUB_SERVICE_FEEDBACK_BATCH ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_078: [** IoTHubMessaging_LL_FeedbackMessageReceived shall do clean up before exits ] */
    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_deliverycountexceeded)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

        receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;

        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
            .SetReturn(TEST_JSON_OBJECT);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
            .SetReturn("deviceId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
            .SetReturn("generationId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
            .SetReturn("DeLiVeRyCoUnTeXcEeDeD");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
            .SetReturn("time");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
            .SetReturn("originalMessageId");

        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        //act
        onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_DELIVER_COUNT_EXCEEDED, receivedFeedbackStatusCode);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_058: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall get the content string of the message by calling message_get_body_amqp_data_in_place ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_059: [ IoTHubMessaging_LL_FeedbackMessageReceived shall parse the response JSON to IOTHUB_SERVICE_FEEDBACK_BATCH struct ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_060: [ IoTHubMessaging_LL_FeedbackMessageReceived shall use the following parson APIs to parse the response string: json_parse_string, json_value_get_object, json_object_get_string, json_object_dotget_string  ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_062: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall call IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK with the received IOTHUB_SERVICE_FEEDBACK_BATCH ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_078: [** IoTHubMessaging_LL_FeedbackMessageReceived shall do clean up before exits ] */
    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_rejected)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

        receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;

        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
            .SetReturn(TEST_JSON_OBJECT);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
            .SetReturn("deviceId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
            .SetReturn("generationId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
            .SetReturn("ReJeCtEd");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
            .SetReturn("time");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
            .SetReturn("originalMessageId");


        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        //act
        onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_REJECTED, receivedFeedbackStatusCode);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }


    /*Tests_SRS_IOTHUBMESSAGING_12_058: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall get the content string of the message by calling message_get_body_amqp_data_in_place ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_059: [ IoTHubMessaging_LL_FeedbackMessageReceived shall parse the response JSON to IOTHUB_SERVICE_FEEDBACK_BATCH struct ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_060: [ IoTHubMessaging_LL_FeedbackMessageReceived shall use the following parson APIs to parse the response string: json_parse_string, json_value_get_object, json_object_get_string, json_object_dotget_string  ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_062: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall call IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK with the received IOTHUB_SERVICE_FEEDBACK_BATCH ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_078: [** IoTHubMessaging_LL_FeedbackMessageReceived shall do clean up before exits ] */
    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_unknown)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

        receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS;

        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
            .SetReturn(TEST_JSON_OBJECT);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
            .SetReturn("deviceId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
            .SetReturn("generationId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
            .SetReturn("XxX");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
            .SetReturn("time");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
            .SetReturn("originalMessageId");


        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        //act
        onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN, receivedFeedbackStatusCode);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_058: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall get the content string of the message by calling message_get_body_amqp_data_in_place ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_059: [ IoTHubMessaging_LL_FeedbackMessageReceived shall parse the response JSON to IOTHUB_SERVICE_FEEDBACK_BATCH struct ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_060: [ IoTHubMessaging_LL_FeedbackMessageReceived shall use the following parson APIs to parse the response string: json_parse_string, json_value_get_object, json_object_get_string, json_object_dotget_string  ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_062: [ If context is not NULL IoTHubMessaging_LL_FeedbackMessageReceived shall call IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK with the received IOTHUB_SERVICE_FEEDBACK_BATCH ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_078: [** IoTHubMessaging_LL_FeedbackMessageReceived shall do clean up before exits ] */
    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

        receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS;

        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
            .SetReturn(TEST_JSON_OBJECT);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
            .SetReturn("deviceId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
            .SetReturn("generationId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
            .SetReturn(NULL);
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
            .SetReturn("time");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
            .SetReturn("originalMessageId");

        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        //act
        onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN, receivedFeedbackStatusCode);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    /*Tests_SRS_IOTHUBMESSAGING_12_061: [ If any of the parson API fails, IoTHubMessaging_LL_FeedbackMessageReceived shall return IOTHUB_MESSAGING_INVALID_JSON ] */
    /*Tests_SRS_IOTHUBMESSAGING_12_078: [** IoTHubMessaging_LL_FeedbackMessageReceived shall do clean up before exits ] */
    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_non_happy_path)
    {
        //arrange
        int umockc_result = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, umockc_result);

        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
            .SetReturn(TEST_JSON_OBJECT);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
            .SetReturn("deviceId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
            .SetReturn("generationId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
            .SetReturn("success");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
            .SetReturn("time");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
            .SetReturn("originalMessageId");


        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        umock_c_negative_tests_snapshot();

        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            /// arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            if (i <= 9)
            {
                //act
                AMQP_VALUE amqp_result = onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

                //assert
                if (i < 7)
                {
                    if (amqp_result != NULL)
                    {
                        ASSERT_ARE_EQUAL(int, 0, 1);
                    }
                }
            }
        }
        umock_c_negative_tests_deinit();

        ///cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_success)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TRUSTED_CERT));

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_2_calls_success)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TRUSTED_CERT));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_handle_NULL_fail)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(NULL, TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_trusted_cert_NULL_fail)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
        IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    }

END_TEST_SUITE(iothub_messaging_ll_ut)
