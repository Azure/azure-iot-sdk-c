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

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void* my_gballoc_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

static void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
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

#undef ENABLE_MOCK_FILTERING_SWITCH
#define ENABLE_MOCK_FILTERING
#define please_mock_message_add_body_amqp_data MOCK_ENABLED
#undef ENABLE_MOCK_FILTERING_SWITCH
#undef ENABLE_MOCK_FILTERING

#include "iothub_messaging_ll.h"

TEST_DEFINE_ENUM_TYPE(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_RESULT_VALUES);

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
#undef ENABLE_MOCKS

static TEST_MUTEX_HANDLE g_testByTest;

static STRING_HANDLE TEST_STRING_HANDLE = (STRING_HANDLE)0x4242;
#define TEST_ASYNC_HANDLE           (ASYNC_OPERATION_HANDLE)0x4246

// Whenever it's changed, update this according with
// the current value of sizeof(IOTHUB_MESSAGING)
#define SIZE_OF_IOTHUB_MESSAGING_STRUCT 224
static uint8_t TEST_IOTHUB_MESSAGING_INSTANCE[SIZE_OF_IOTHUB_MESSAGING_STRUCT];

#define SOME_RANDOM_NUMBER 50304050

static uint8_t TEST_SEND_TARGET_ADDRESS_BUFFER[64];
static uint8_t TEST_RECEIVE_TARGET_ADDRESS_BUFFER[64];
static uint8_t TEST_AUTH_CID_BUFFER[128];
static uint8_t TEST_DEVICE_DESTINATION_BUFFER[64];
static uint8_t TEST_SEND_CALLBACK_DATA_BUFFER[64];
static uint8_t TEST_IOTHUB_SERVICE_FEEDBACK_BATCH_BUFFER[sizeof(IOTHUB_SERVICE_FEEDBACK_BATCH)];
static uint8_t TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER[sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD) * 10];

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
static char* TEST_SAS_TOKEN = "sas-token";
static char* TEST_STRING = "some-string";

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
static MESSAGE_SENDER_HANDLE TEST_MESSAGE_SENDER_HANDLE = (MESSAGE_SENDER_HANDLE)0x7000;
static MESSAGE_RECEIVER_HANDLE TEST_MESSAGE_RECEIVER_HANDLE = (MESSAGE_RECEIVER_HANDLE)0x7001;
static SINGLYLINKEDLIST_HANDLE TEST_SINGLYLINKEDLIST_HANDLE = (SINGLYLINKEDLIST_HANDLE)0x7002;
static LIST_ITEM_HANDLE TEST_LIST_ITEM_HANDLE = (LIST_ITEM_HANDLE)0x7003;
static AMQP_VALUE TEST_DELIVERY_REJECTED_AMQP_VALUE = (AMQP_VALUE)0x7004;

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
    memcpy(destination, source, sizeof(BINARY_DATA));
    int result = 0;
    return result;
}

void umock_free_BINARY_DATA(BINARY_DATA* value)
{
    //do nothing
    (void)value;
}

static IOTHUB_MESSAGING_HANDLE create_messaging_handle()
{
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetReturn(TEST_IOTHUB_MESSAGING_INSTANCE);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_HOSTNAME))
        .CopyOutArgumentBuffer_destination(&TEST_HOSTNAME, sizeof(TEST_HOSTNAME));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBNAME))
        .CopyOutArgumentBuffer_destination(&TEST_IOTHUBNAME, sizeof(TEST_IOTHUBNAME));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBSUFFIX))
        .CopyOutArgumentBuffer_destination(&TEST_IOTHUBSUFFIX, sizeof(TEST_IOTHUBSUFFIX));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEY))
        .CopyOutArgumentBuffer_destination(&TEST_SHAREDACCESSKEY, sizeof(TEST_SHAREDACCESSKEY));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEYNAME))
        .CopyOutArgumentBuffer_destination(&TEST_SHAREDACCESSKEYNAME, sizeof(TEST_SHAREDACCESSKEYNAME));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());

    // act
    return IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
}

static ON_MESSAGE_RECEIVER_STATE_CHANGED saved_on_message_receiver_state_changed;
static void* saved_messagereceiver_create_context;
static ON_MESSAGE_SENDER_STATE_CHANGED saved_on_message_sender_state_changed;
static void* saved_messagesender_create_context;
static ON_MESSAGE_RECEIVED saved_on_message_receiver_callback;
static void* saved_on_message_receiver_context;
static void set_IoTHubMessaging_LL_Open_expected_calls()
{
    // createSendTargetAddress
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetReturn(TEST_SEND_TARGET_ADDRESS_BUFFER);
    // createReceiveTargetAddress
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetReturn(TEST_RECEIVE_TARGET_ADDRESS_BUFFER);
    // createAuthCid
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetReturn(TEST_AUTH_CID_BUFFER);
    // createSasToken
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(SASToken_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_destination(&TEST_SAS_TOKEN, sizeof(TEST_SAS_TOKEN));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    // back to IoTHubMessaging_LL_Open
    STRICT_EXPECTED_CALL(saslplain_get_interface());
    STRICT_EXPECTED_CALL(saslmechanism_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(saslclientio_get_interface_description());
    STRICT_EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(connection_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(session_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(session_set_incoming_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(session_set_outgoing_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // attachServiceClientTypeToLink
    STRICT_EXPECTED_CALL(amqpvalue_create_map());
    STRICT_EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

    // back to IoTHubMessaging_LL_Open
    STRICT_EXPECTED_CALL(link_set_snd_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(messagesender_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CaptureArgumentValue_on_message_sender_state_changed(&saved_on_message_sender_state_changed)
        .CaptureArgumentValue_context(&saved_messagesender_create_context);
    STRICT_EXPECTED_CALL(messagesender_open(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    
    // attachServiceClientTypeToLink
    STRICT_EXPECTED_CALL(amqpvalue_create_map());
    STRICT_EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

    // back to IoTHubMessaging_LL_Open
    STRICT_EXPECTED_CALL(link_set_rcv_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(messagereceiver_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CaptureArgumentValue_on_message_receiver_state_changed(&saved_on_message_receiver_state_changed)
        .CaptureArgumentValue_context(&saved_messagereceiver_create_context);
    STRICT_EXPECTED_CALL(messagereceiver_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CaptureArgumentValue_on_message_received(&saved_on_message_receiver_callback)
        .CaptureArgumentValue_callback_context(&saved_on_message_receiver_context);
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static bool is_open_complete_callback_fired;
static void test_open_complete_callback(void* context)
{
    bool* flag = (bool*)context;
    *flag = true;
}

static int open_messaging_handle(
    IOTHUB_MESSAGING_HANDLE iothub_messaging_handle,
    bool setOpenCompleteCallback)
{
    umock_c_reset_all_calls();
    set_IoTHubMessaging_LL_Open_expected_calls();

    return IoTHubMessaging_LL_Open(iothub_messaging_handle,
        setOpenCompleteCallback ? test_open_complete_callback : NULL,
        setOpenCompleteCallback ? &is_open_complete_callback_fired : NULL);
}

static ON_MESSAGE_SEND_COMPLETE saved_on_message_send_complete_callback;
static void* saved_on_message_send_complete_context;
static void set_IoTHubMessaging_LL_Send_expected_calls()
{
    size_t number_of_arguments = 1;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetReturn(TEST_DEVICE_DESTINATION_BUFFER);
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(IGNORED_PTR_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_create());
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, TEST_BINARY_DATA_INST))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(message_get_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_properties(&TEST_PROPERTIES_HANDLE_NULL, sizeof(TEST_PROPERTIES_HANDLE_NULL));
    STRICT_EXPECTED_CALL(properties_create());
    STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(TEST_CONST_CHAR_PTR);
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_set_message_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG))
        .CallCannotFail()
        .SetReturn(TEST_CONST_CHAR_PTR);
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_set_correlation_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_set_to(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_set_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(properties_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE))
        .CallCannotFail()
        .SetReturn(TEST_MAP_HANDLE);
    STRICT_EXPECTED_CALL(Map_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_keys(&pTEST_MAP_KEYS, sizeof(pTEST_MAP_KEYS))
        .CopyOutArgumentBuffer_values(&pTEST_MAP_VALUES, sizeof(pTEST_MAP_VALUES))
        .CopyOutArgumentBuffer_count(&number_of_arguments, sizeof(size_t))
        .SetReturn(MAP_OK);
    STRICT_EXPECTED_CALL(amqpvalue_create_map());
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_set_application_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)) // SEND_CALLBACK_DATA
        .SetReturn(TEST_SEND_CALLBACK_DATA_BUFFER);
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messagesender_send_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .CaptureArgumentValue_on_message_send_complete(&saved_on_message_send_complete_callback)
        .CaptureArgumentValue_callback_context(&saved_on_message_send_complete_context);
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

typedef struct SEND_COMPLETE_INFO_STRUCT
{
    void* context;
    IOTHUB_MESSAGING_RESULT result;
} SEND_COMPLETE_INFO;

#define SAVED_SEND_COMPLETE_INFO_QUEUE_SIZE 10
static int saved_send_complete_info_count;
static SEND_COMPLETE_INFO saved_send_complete_info[SAVED_SEND_COMPLETE_INFO_QUEUE_SIZE];

static void test_iothub_send_complete_callback(void* context, IOTHUB_MESSAGING_RESULT messagingResult)
{
    if (saved_send_complete_info_count < SAVED_SEND_COMPLETE_INFO_QUEUE_SIZE)
    {
        saved_send_complete_info[saved_send_complete_info_count].context = context; 
        saved_send_complete_info[saved_send_complete_info_count].result = messagingResult; 
        saved_send_complete_info_count++;
    }
}

static bool saved_send_complete_info_contains(int contextInfo, IOTHUB_MESSAGING_RESULT result)
{
    for (int i = 0; i < saved_send_complete_info_count; i++)
    {
        if (contextInfo == (*(int*)saved_send_complete_info[i].context))
        {
            if (result == saved_send_complete_info[i].result)
            {
                return true;
            }
            else
            {
                printf("saved_send_complete_info_contains(%i, %s)\r\n", contextInfo, MU_ENUM_TO_STRING(IOTHUB_MESSAGING_RESULT, saved_send_complete_info[i].result));
                break;
            }
        }
    }

    return false;
}

static void reset_saved_send_complete_info()
{
    saved_send_complete_info_count = 0;
    (void)memset(saved_send_complete_info, 0, sizeof(SEND_COMPLETE_INFO) * SAVED_SEND_COMPLETE_INFO_QUEUE_SIZE);
}

static IOTHUB_MESSAGING_RESULT send_message(IOTHUB_MESSAGING_HANDLE messagingHandle, void* userContext)
{
    umock_c_reset_all_calls();
    set_IoTHubMessaging_LL_Send_expected_calls();
    return IoTHubMessaging_LL_Send(messagingHandle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE,  test_iothub_send_complete_callback, userContext);
}

typedef struct FEEDBACK_MESSAGE_RECEIVED_INFO_STRUCT
{
    void* context;
    IOTHUB_SERVICE_FEEDBACK_BATCH feedback;
} FEEDBACK_MESSAGE_RECEIVED_INFO;

#define SAVED_FEEDBACK_MESSAGE_RECEIVED_INFO_QUEUE_SIZE 10
static int saved_feedback_message_received_count;
static FEEDBACK_MESSAGE_RECEIVED_INFO saved_feedback_message_received[SAVED_FEEDBACK_MESSAGE_RECEIVED_INFO_QUEUE_SIZE];

static void test_func_iothub_feedback_message_received_callback(void* context, IOTHUB_SERVICE_FEEDBACK_BATCH* feedbackBatch)
{
    if (saved_feedback_message_received_count < SAVED_FEEDBACK_MESSAGE_RECEIVED_INFO_QUEUE_SIZE)
    {
        saved_feedback_message_received[saved_feedback_message_received_count].context = context;
        memcpy(&saved_feedback_message_received[saved_feedback_message_received_count].feedback, feedbackBatch, sizeof(IOTHUB_SERVICE_FEEDBACK_BATCH));
        saved_feedback_message_received_count++;
    }
}

static void reset_saved_feedback_message_received()
{
    saved_feedback_message_received_count = 0;
    (void)memset(saved_feedback_message_received, 0,
        sizeof(FEEDBACK_MESSAGE_RECEIVED_INFO) * SAVED_FEEDBACK_MESSAGE_RECEIVED_INFO_QUEUE_SIZE);
}

static void set_IoTHubMessaging_LL_FeedbackMessageReceived_expected_calls(int array_count, char** feedback_record_status)
{
    STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
        .SetReturn(array_count);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetReturn(TEST_IOTHUB_SERVICE_FEEDBACK_BATCH_BUFFER);


    STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
        .CallCannotFail()
        .SetReturn(array_count);
    STRICT_EXPECTED_CALL(singlylinkedlist_create());

    for (int i = 0; i < array_count; i++)
    {
        STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, IGNORED_NUM_ARG))
            .SetReturn(TEST_JSON_OBJECT);

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + i * sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD));
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
            .CallCannotFail()
            .SetReturn("deviceId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
            .CallCannotFail()
            .SetReturn("generationId");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
            .CallCannotFail()
            .SetReturn(feedback_record_status[i]);
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
            .CallCannotFail()
            .SetReturn("time");
        STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
            .CallCannotFail()
            .SetReturn("originalMessageId");
        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CallCannotFail();
    }

    STRICT_EXPECTED_CALL(messaging_delivery_accepted())
        .CallCannotFail();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
        .CallCannotFail();
    for (int i = 0; i < array_count; i++)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
            .CallCannotFail()
            .SetReturn(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + i * sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD));
        STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
            .CallCannotFail()
            .SetReturn(i < (array_count - 1) ? TEST_LIST_ITEM_HANDLE : NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG));
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
        REGISTER_UMOCK_ALIAS_TYPE(LIST_CONDITION_FUNCTION, void*);
        REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MAP_RESULT, int);
        REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(receiver_settle_mode, uint8_t);
        REGISTER_UMOCK_ALIAS_TYPE(tickcounter_ms_t, unsigned long long);
        REGISTER_UMOCK_ALIAS_TYPE(fields, void*);

        REGISTER_GLOBAL_MOCK_RETURNS(json_parse_string, TEST_JSON_VALUE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(json_value_get_array, TEST_JSON_ARRAY, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(json_array_get_object, TEST_JSON_OBJECT, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(json_array_get_count, 2, 0);
        REGISTER_GLOBAL_MOCK_RETURNS(json_object_get_string, TEST_CONST_CHAR_PTR, NULL);

        REGISTER_GLOBAL_MOCK_RETURNS(singlylinkedlist_create, TEST_SINGLYLINKEDLIST_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(mallocAndStrcpy_s, 0, 1);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_calloc, NULL);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_realloc, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(STRING_construct, TEST_STRING_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(STRING_c_str, TEST_STRING, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(SASToken_Create, TEST_STRING_HANDLE, NULL);

        REGISTER_GLOBAL_MOCK_RETURNS(saslplain_get_interface, TEST_SASL_MECHANISM_INTERFACE_DESCRIPTION, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(saslmechanism_create, TEST_SASL_MECHANISM_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(platform_get_default_tlsio, TEST_IO_INTERFACE_DESCRIPTION, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(xio_create, TEST_XIO_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(saslclientio_get_interface_description, TEST_IO_INTERFACE_DESCRIPTION, NULL);
    
        REGISTER_GLOBAL_MOCK_RETURNS(connection_create, TEST_CONNECTION_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(session_create, TEST_SESSION_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(session_set_incoming_window, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(session_set_outgoing_window, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(messaging_create_source, TEST_AMQP_VALUE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(messaging_create_target, TEST_AMQP_VALUE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(link_set_snd_settle_mode, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(link_set_rcv_settle_mode, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(messagesender_create, TEST_MESSAGE_SENDER_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(messagesender_open, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(messagesender_send_async, TEST_ASYNC_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(messagereceiver_create, TEST_MESSAGE_RECEIVER_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(messagereceiver_open, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(link_create, TEST_LINK_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(amqpvalue_create_map, TEST_AMQP_VALUE, NULL); // fields is an AMQP_VALUE
        REGISTER_GLOBAL_MOCK_RETURNS(amqpvalue_create_symbol, TEST_AMQP_VALUE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(amqpvalue_create_string, TEST_AMQP_VALUE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(amqpvalue_set_map_value, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(link_set_attach_properties, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(message_create, TEST_MESSAGE_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(message_get_properties, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(message_set_properties, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(message_set_application_properties, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(message_get_body_amqp_data_in_place, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(properties_create, TEST_PROPERTIES_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(properties_set_to, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(properties_set_message_id, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(properties_set_correlation_id, 0, 1);
        REGISTER_GLOBAL_MOCK_RETURNS(singlylinkedlist_add, TEST_LIST_ITEM_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(singlylinkedlist_get_head_item, TEST_LIST_ITEM_HANDLE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_OK, IOTHUB_MESSAGE_ERROR);
        REGISTER_GLOBAL_MOCK_RETURNS(Map_GetInternals, MAP_OK, MAP_ERROR);
        REGISTER_GLOBAL_MOCK_RETURNS(messaging_delivery_accepted, TEST_AMQP_VALUE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(messaging_delivery_rejected, TEST_DELIVERY_REJECTED_AMQP_VALUE, NULL);
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

        is_open_complete_callback_fired = false;

        saved_on_message_receiver_state_changed = NULL;
        saved_messagereceiver_create_context = NULL;
        saved_on_message_sender_state_changed = NULL;
        saved_messagesender_create_context = NULL;
        saved_on_message_send_complete_callback = NULL;
        saved_on_message_send_complete_context = NULL;

        reset_saved_send_complete_info();
        reset_saved_feedback_message_received();
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Create_return_null_if_input_parameter_serviceClientHandle_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NULL(result);
    }

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

    TEST_FUNCTION(IoTHubMessaging_LL_Create_happy_path)
    {
        // arrange
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn(TEST_IOTHUB_MESSAGING_INSTANCE);
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_HOSTNAME))
            .CopyOutArgumentBuffer_destination(&TEST_HOSTNAME, sizeof(TEST_HOSTNAME));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBNAME))
            .CopyOutArgumentBuffer_destination(&TEST_IOTHUBNAME, sizeof(TEST_IOTHUBNAME));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBSUFFIX))
            .CopyOutArgumentBuffer_destination(&TEST_IOTHUBSUFFIX, sizeof(TEST_IOTHUBSUFFIX));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEY))
            .CopyOutArgumentBuffer_destination(&TEST_SHAREDACCESSKEY, sizeof(TEST_SHAREDACCESSKEY));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEYNAME))
            .CopyOutArgumentBuffer_destination(&TEST_SHAREDACCESSKEYNAME, sizeof(TEST_SHAREDACCESSKEYNAME));
        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        // act
        IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(result);

        //cleanup
        IoTHubMessaging_LL_Destroy(result);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Create_non_happy_path)
    {
        // arrange
        ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn(TEST_IOTHUB_MESSAGING_INSTANCE);
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_HOSTNAME))
            .CopyOutArgumentBuffer_destination(&TEST_HOSTNAME, sizeof(TEST_HOSTNAME));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBNAME))
            .CopyOutArgumentBuffer_destination(&TEST_IOTHUBNAME, sizeof(TEST_IOTHUBNAME));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_IOTHUBSUFFIX))
            .CopyOutArgumentBuffer_destination(&TEST_IOTHUBSUFFIX, sizeof(TEST_IOTHUBSUFFIX));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEY))
            .CopyOutArgumentBuffer_destination(&TEST_SHAREDACCESSKEY, sizeof(TEST_SHAREDACCESSKEY));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SHAREDACCESSKEYNAME))
            .CopyOutArgumentBuffer_destination(&TEST_SHAREDACCESSKEYNAME, sizeof(TEST_SHAREDACCESSKEYNAME));
        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        umock_c_negative_tests_snapshot();

        //act
        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            /// arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            if (!umock_c_negative_tests_can_call_fail(i))
            {
                continue;
            }

            /// act
            IOTHUB_MESSAGING_HANDLE result = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);

            /// assert
            ASSERT_ARE_EQUAL(void_ptr, NULL, result);

            ///cleanup
        }

        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Destroy_return_if_input_parameter_messagingHandle_is_NULL)
    {
        // arrange

        // act
        IoTHubMessaging_LL_Destroy(NULL);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Destroy_success)
    {
        // arrange
        IOTHUB_MESSAGING_HANDLE handle = create_messaging_handle();

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG)); // send_callback_data
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); // hostname
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); // iothubName
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); // iothubSuffix
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); // sharedAccessKey
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); // keyName
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); // trusted_cert
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); // messHandle

        // act
        IoTHubMessaging_LL_Destroy(handle);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Open_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    {
        // arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(NULL, TEST_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)0x4242);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Open_success)
    {
        // arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();

        umock_c_reset_all_calls();
        // createSendTargetAddress
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn(TEST_SEND_TARGET_ADDRESS_BUFFER);
        // createReceiveTargetAddress
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn(TEST_RECEIVE_TARGET_ADDRESS_BUFFER);
        // createAuthCid
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn(TEST_AUTH_CID_BUFFER);
        // createSasToken
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(SASToken_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_destination(&TEST_SAS_TOKEN, sizeof(TEST_SAS_TOKEN));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        // back to IoTHubMessaging_LL_Open
        STRICT_EXPECTED_CALL(saslplain_get_interface());
        STRICT_EXPECTED_CALL(saslmechanism_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(connection_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(session_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(session_set_incoming_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(session_set_outgoing_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        // attachServiceClientTypeToLink
        STRICT_EXPECTED_CALL(amqpvalue_create_map());
        STRICT_EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

        // back to IoTHubMessaging_LL_Open
        STRICT_EXPECTED_CALL(link_set_snd_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(messagesender_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagesender_open(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        
        // attachServiceClientTypeToLink
        STRICT_EXPECTED_CALL(amqpvalue_create_map());
        STRICT_EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

        // back to IoTHubMessaging_LL_Open
        STRICT_EXPECTED_CALL(link_set_rcv_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(messagereceiver_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagereceiver_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        // cleanup
        IoTHubMessaging_LL_Close(iothub_messaging_handle);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Open_error_checks_fails)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();

        ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

        umock_c_reset_all_calls();
        set_IoTHubMessaging_LL_Open_expected_calls();

        umock_c_negative_tests_snapshot();
        size_t call_count = umock_c_negative_tests_call_count();

        //act
        for (size_t i = 0; i < call_count; i++)
        {
            if (umock_c_negative_tests_can_call_fail(i))
            {
                // arrange
                char error_msg[128];
                (void)sprintf(error_msg, "Failure in test %zu/%zu", i, call_count);

                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(i);

                IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_IOTHUB_OPEN_COMPLETE_CALLBACK, NULL);

                //assert
                ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result, error_msg);
            }
        }

        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Close_return_if_input_parameter_messagingHandle_is_NULL)
    {
        // arrange

        // act
        IoTHubMessaging_LL_Close(NULL);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Close_happy_path)
    {
        // arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

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
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(NULL, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_deviceId_is_NULL)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, NULL, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_message_is_NULL)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, NULL, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_ERROR_if_messaging_is_not_opened)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();

        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_ERROR, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Send_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        umock_c_reset_all_calls();
        set_IoTHubMessaging_LL_Send_expected_calls();

        //act
        IOTHUB_MESSAGING_RESULT result =
            IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_Send_error_checks_fails)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

        umock_c_reset_all_calls();
        set_IoTHubMessaging_LL_Send_expected_calls();

        umock_c_negative_tests_snapshot();
        size_t call_count = umock_c_negative_tests_call_count();

        //act
        for (size_t i = 0; i < call_count; i++)
        {
            if (umock_c_negative_tests_can_call_fail(i))
            {
                // arrange
                char error_msg[128];
                (void)sprintf(error_msg, "Failure in test %zu/%zu", i, call_count);

                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(i);

                IOTHUB_MESSAGING_RESULT result =
                    IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR,
                    TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

                //assert
                ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result, error_msg);
            }
        }

        umock_c_negative_tests_deinit();

    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetFeedbackMessageCallback_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetFeedbackMessageCallback(NULL, TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_INVALID_ARG, result);
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetFeedbackMessageCallback_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, TEST_VOID_PTR);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_DoWork_return_if_input_parameter_messagingHandle_is_NULL)
    {
        //arrange

        //act
        IoTHubMessaging_LL_DoWork(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    TEST_FUNCTION(IoTHubMessaging_LL_DoWork_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG));

        //act
        IoTHubMessaging_LL_DoWork(iothub_messaging_handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_call_user_callback)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));
        saved_on_message_receiver_state_changed(saved_messagereceiver_create_context,
            MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_IDLE);

        umock_c_reset_all_calls();

        MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPEN;
        MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

        //act
        saved_on_message_sender_state_changed(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_TRUE(is_open_complete_callback_fired);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_messaging_is_not_opened)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        umock_c_reset_all_calls();

        MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPENING;
        MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

        //act
        saved_on_message_sender_state_changed(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_FALSE(is_open_complete_callback_fired);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_openCompleteCompleteCallback_is_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));
        saved_on_message_receiver_state_changed(saved_messagereceiver_create_context, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_IDLE);

        umock_c_reset_all_calls();

        MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPEN;
        MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

        //act
        saved_on_message_sender_state_changed(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_FALSE(is_open_complete_callback_fired);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_call_user_callback)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));
        saved_on_message_sender_state_changed(saved_messagesender_create_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);

        umock_c_reset_all_calls();

        MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPEN;
        MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

        //act
        saved_on_message_receiver_state_changed(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_TRUE(is_open_complete_callback_fired);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_messaging_is_not_opened)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));
        saved_on_message_sender_state_changed(saved_messagesender_create_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);

        umock_c_reset_all_calls();

        MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPENING;
        MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

        //act
        saved_on_message_receiver_state_changed(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_FALSE(is_open_complete_callback_fired);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_openCompleteCompleteCallback_is_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));
        saved_on_message_sender_state_changed(saved_messagesender_create_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        umock_c_reset_all_calls();

        MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPEN;
        MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

        //act
        saved_on_message_receiver_state_changed(iothub_messaging_handle, new_state, previous_state);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_FALSE(is_open_complete_callback_fired);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SendMessageComplete_context_is_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        int context1 = 145897901;
        ASSERT_ARE_EQUAL(int, 0, send_message(iothub_messaging_handle, &context1));

        umock_c_reset_all_calls();

        //act
        saved_on_message_send_complete_callback((void*)NULL, MESSAGE_SEND_OK, TEST_AMQP_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, saved_send_complete_info_count);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SendMessageComplete_sendCompleteCallback_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        umock_c_reset_all_calls();
        set_IoTHubMessaging_LL_Send_expected_calls();
        (void)IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE, NULL, NULL);

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        saved_on_message_send_complete_callback(saved_on_message_send_complete_context, MESSAGE_SEND_OK, TEST_AMQP_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, saved_send_complete_info_count);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SendMessageCompleteDevice_call_to_user_callback)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));
        
        /* If modules are re-enabled, re-enable this code and add testing_module paramater to this function
        if (testing_module == true)
        {
            (void)IoTHubMessaging_LL_SendModule(iothub_messaging_handle, TEST_DEVICE_ID, TEST_MODULE_ID, TEST_IOTHUB_MESSAGE_HANDLE, test_iothub_send_complete_callback, (void*)1);
        }
        */

        int sendContext1 = 191919191;
        umock_c_reset_all_calls();
        set_IoTHubMessaging_LL_Send_expected_calls();
        IOTHUB_MESSAGING_RESULT sendApiResult = IoTHubMessaging_LL_Send(
            iothub_messaging_handle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE, test_iothub_send_complete_callback, &sendContext1);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, sendApiResult);

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(singlylinkedlist_remove_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        saved_on_message_send_complete_callback(saved_on_message_send_complete_context, MESSAGE_SEND_OK, TEST_AMQP_VALUE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 1, saved_send_complete_info_count);
        ASSERT_IS_TRUE(saved_send_complete_info_contains(sendContext1, IOTHUB_MESSAGING_OK));

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_context_is_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        int feedbackContext1 = SOME_RANDOM_NUMBER;
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(
            iothub_messaging_handle, test_func_iothub_feedback_message_received_callback, &feedbackContext1);

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());

        //act
        void* amqp_result = (void*)saved_on_message_receiver_callback(NULL, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(amqp_result);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_success)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        int feedbackContext1 = SOME_RANDOM_NUMBER;
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(
            iothub_messaging_handle, test_func_iothub_feedback_message_received_callback, &feedbackContext1);

        umock_c_reset_all_calls();
        int array_count = 2;

        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_value_get_array(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_array_get_count(IGNORED_PTR_ARG))
            .SetReturn(array_count);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn(TEST_IOTHUB_SERVICE_FEEDBACK_BATCH_BUFFER);


        STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
            .SetReturn(array_count);
        STRICT_EXPECTED_CALL(singlylinkedlist_create());

        for (int i = 0; i < array_count; i++)
        {
            STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, IGNORED_NUM_ARG))
                .SetReturn(TEST_JSON_OBJECT);

            STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
                .SetReturn(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + i * sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD));
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
        }

        STRICT_EXPECTED_CALL(messaging_delivery_accepted());
        STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
        for (int i = 0; i < array_count; i++)
        {
            STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
                .SetReturn(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + i * sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD));
            STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
                .SetReturn(i < (array_count - 1) ? TEST_LIST_ITEM_HANDLE : NULL);
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG));

        //act
        void* amqp_result = (void*)saved_on_message_receiver_callback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 1, saved_feedback_message_received_count);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord0 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord1 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD)));
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS, feedbackRecord0->statusCode);
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS, feedbackRecord1->statusCode);
        ASSERT_IS_NOT_NULL(amqp_result);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_expired)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        int feedbackContext1 = 50000001;
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(
            iothub_messaging_handle, test_func_iothub_feedback_message_received_callback, &feedbackContext1);

        umock_c_reset_all_calls();
        char* feedback_record_status[2] = { "expired", "ExPiReD" };
        set_IoTHubMessaging_LL_FeedbackMessageReceived_expected_calls(2, feedback_record_status);

        //act
        void* amqp_result = (void*)saved_on_message_receiver_callback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 1, saved_feedback_message_received_count);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord0 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord1 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD)));
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_EXPIRED, feedbackRecord0->statusCode);
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_EXPIRED, feedbackRecord1->statusCode);
        ASSERT_IS_NOT_NULL(amqp_result);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_deliverycountexceeded)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        int feedbackContext1 = 50000001;
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(
            iothub_messaging_handle, test_func_iothub_feedback_message_received_callback, &feedbackContext1);

        umock_c_reset_all_calls();
        char* feedback_record_status[2] = { "DeLiVeRyCoUnTeXcEeDeD", "deliverycountexceeded" };
        set_IoTHubMessaging_LL_FeedbackMessageReceived_expected_calls(2, feedback_record_status);

        //act
        void* amqp_result = (void*)saved_on_message_receiver_callback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 1, saved_feedback_message_received_count);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord0 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord1 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD)));
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_DELIVER_COUNT_EXCEEDED, feedbackRecord0->statusCode);
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_DELIVER_COUNT_EXCEEDED, feedbackRecord1->statusCode);
        ASSERT_IS_NOT_NULL(amqp_result);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_rejected)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        int feedbackContext1 = 50000001;
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(
            iothub_messaging_handle, test_func_iothub_feedback_message_received_callback, &feedbackContext1);

        umock_c_reset_all_calls();
        char* feedback_record_status[2] = { "ReJeCtEd", "rejected" };
        set_IoTHubMessaging_LL_FeedbackMessageReceived_expected_calls(2, feedback_record_status);

        //act
        void* amqp_result = (void*)saved_on_message_receiver_callback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 1, saved_feedback_message_received_count);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord0 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord1 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD)));
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_REJECTED, feedbackRecord0->statusCode);
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_REJECTED, feedbackRecord1->statusCode);
        ASSERT_IS_NOT_NULL(amqp_result);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_unknown)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        int feedbackContext1 = 50000001;
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(
            iothub_messaging_handle, test_func_iothub_feedback_message_received_callback, &feedbackContext1);

        umock_c_reset_all_calls();
        char* feedback_record_status[2] = { "successS", "rejected0" };
        set_IoTHubMessaging_LL_FeedbackMessageReceived_expected_calls(2, feedback_record_status);

        //act
        void* amqp_result = (void*)saved_on_message_receiver_callback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 1, saved_feedback_message_received_count);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord0 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord1 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)(TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER + sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD)));
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN, feedbackRecord0->statusCode);
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN, feedbackRecord1->statusCode);
        ASSERT_IS_NOT_NULL(amqp_result);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_null)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, false));

        int feedbackContext1 = 50000001;
        (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(
            iothub_messaging_handle, test_func_iothub_feedback_message_received_callback, &feedbackContext1);

        umock_c_reset_all_calls();
        char* feedback_record_status[1] = { NULL };
        set_IoTHubMessaging_LL_FeedbackMessageReceived_expected_calls(1, feedback_record_status);

        //act
        void* amqp_result = (void*)saved_on_message_receiver_callback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 1, saved_feedback_message_received_count);
        IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord0 = ((IOTHUB_SERVICE_FEEDBACK_RECORD*)TEST_IOTHUB_SERVICE_FEEDBACK_RECORD_BUFFER);
        ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN, feedbackRecord0->statusCode);
        ASSERT_IS_NOT_NULL(amqp_result);

        ///cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_non_happy_path)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

        umock_c_reset_all_calls();
        char* feedback_record_status[1] = { "success" };
        set_IoTHubMessaging_LL_FeedbackMessageReceived_expected_calls(1, feedback_record_status);
        umock_c_negative_tests_snapshot();
        size_t call_count = umock_c_negative_tests_call_count();

        //act
        for (size_t i = 0; i < call_count; i++)
        {
            if (umock_c_negative_tests_can_call_fail(i))
            {
                // arrange
                char error_msg[128];
                (void)sprintf(error_msg, "Failure in test %zu/%zu", i, call_count);

                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(i);

                void* amqp_result = (void*)saved_on_message_receiver_callback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

                //assert
                ASSERT_ARE_EQUAL(void_ptr, TEST_DELIVERY_REJECTED_AMQP_VALUE, amqp_result, error_msg);
                ASSERT_ARE_EQUAL(int, 0, saved_feedback_message_received_count);
            }
        }

        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_success)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TRUSTED_CERT));

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_2_calls_success)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TRUSTED_CERT));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_handle_NULL_fail)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(NULL, TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_trusted_cert_NULL_fail)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();
        ASSERT_ARE_EQUAL(int, 0, open_messaging_handle(iothub_messaging_handle, true));

        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetMaxSendQueueSize_success)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();

        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetMaxSendQueueSize(iothub_messaging_handle, 100);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetMaxSendQueueSize_NULL_handle_fails)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetMaxSendQueueSize(NULL, 100);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);

        //cleanup
    }

    TEST_FUNCTION(IoTHubMessaging_LL_SetMaxSendQueueSize_zero_size_fails)
    {
        //arrange
        IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = create_messaging_handle();

        umock_c_reset_all_calls();

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetMaxSendQueueSize(iothub_messaging_handle, 0);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);

        //cleanup
    }

END_TEST_SUITE(iothub_messaging_ll_ut)
