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

// static IOTHUB_FEEDBACK_STATUS_CODE receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;
// void my_on_feedback_message_received(void* context, IOTHUB_SERVICE_FEEDBACK_BATCH* feedbackBatch)
// {
//     (void)context;
//     LIST_ITEM_HANDLE feedbackRecord = my_list_get_head_item(feedbackBatch->feedbackRecordList);
//     while (feedbackRecord != NULL)
//     {
//         IOTHUB_SERVICE_FEEDBACK_RECORD* feedback = (IOTHUB_SERVICE_FEEDBACK_RECORD*)my_list_item_get_value(feedbackRecord);
//         receivedFeedbackStatusCode = feedback->statusCode;
//         feedbackRecord = my_list_get_next_item(feedbackRecord);
//     }
// }

// Whenever it's changed, update this according with
// the current value of sizeof(IOTHUB_MESSAGING)
#define SIZE_OF_IOTHUB_MESSAGING_STRUCT 224
static uint8_t TEST_IOTHUB_MESSAGING_INSTANCE[SIZE_OF_IOTHUB_MESSAGING_STRUCT];


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

static SINGLYLINKEDLIST_HANDLE TEST_SINGLYLINKEDLIST_HANDLE = (SINGLYLINKEDLIST_HANDLE)0x5001;

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

        REGISTER_GLOBAL_MOCK_RETURNS(json_parse_string, TEST_JSON_VALUE, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(json_value_get_array, TEST_JSON_ARRAY, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(json_array_get_object, TEST_JSON_OBJECT, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(json_array_get_count, 42, 0);
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
    

        // REGISTER_GLOBAL_MOCK_HOOK(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, my_on_feedback_message_received);
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

        // onMessageSenderStateChangedCallback = NULL;
        // onMessageReceiverStateChangedCallback = NULL;
        // onMessageSendCompleteCallback = NULL;
        // onMessageReceivedCallback = NULL;
        // messagereceiver_create_return = NULL;
        // messagesender_create_return = NULL;

        // receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;
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
        uint8_t TEST_SEND_TARGET_ADDRESS_BUFFER[64];
        uint8_t TEST_RECEIVE_TARGET_ADDRESS_BUFFER[64];
        uint8_t TEST_AUTH_CID_BUFFER[128];

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

    // static void addSetLinkCalls(void)
    // {
    //     EXPECTED_CALL(amqpvalue_create_map());
    //     EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
    //     EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    //     EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    //     EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    //     EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    //     EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    //     EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    // }

    // static void callsForOpen(void)
    // {
    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));    // Call #0

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(STRING_construct(TEST_HOSTNAME));
    //     STRICT_EXPECTED_CALL(STRING_construct(TEST_SHAREDACCESSKEY));
    //     STRICT_EXPECTED_CALL(STRING_construct(TEST_SHAREDACCESSKEYNAME));     // Call #5

    //     STRICT_EXPECTED_CALL(SASToken_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1)
    //         .SetReturn(TEST_CHAR_PTR);

    //     STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))     // Call #10
    //         .IgnoreArgument(1);
    //     STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);
    //     STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(saslplain_get_interface());
    //     STRICT_EXPECTED_CALL(saslmechanism_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(platform_get_default_tlsio());     // Call #15

    //     STRICT_EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreArgument(1)
    //         .IgnoreArgument(2);

    //     STRICT_EXPECTED_CALL(saslclientio_get_interface_description());

    //     STRICT_EXPECTED_CALL(xio_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreArgument(1)
    //         .IgnoreArgument(2);

    //     STRICT_EXPECTED_CALL(connection_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(session_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(session_set_incoming_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG))     // Call #21
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(session_set_outgoing_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, TEST_AMQP_VALUE_NULL, TEST_AMQP_VALUE_NULL))
    //         .IgnoreAllArguments();      // Call #25
    //     addSetLinkCalls();
    //     STRICT_EXPECTED_CALL(link_set_snd_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG))     // Call #34
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(messagesender_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(messagesender_open(IGNORED_NUM_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();      // Call #39
    //     addSetLinkCalls();
    //     STRICT_EXPECTED_CALL(link_set_rcv_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG))     // Call #48
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(messagereceiver_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(messagereceiver_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));     // Call #51
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Open_happy_path)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     umock_c_reset_all_calls();

    //     callsForOpen();

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)0x4242);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    //     ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Open_non_happy_path)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     umock_c_reset_all_calls();

    //     int umockc_result = umock_c_negative_tests_init();
    //     ASSERT_ARE_EQUAL(int, 0, umockc_result);

    //     //arrange
    //     callsForOpen();

    //     umock_c_negative_tests_snapshot();

    //     //act
    //     for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    //     {
    //         // arrange
    //         umock_c_negative_tests_reset();
    //         umock_c_negative_tests_fail_call(i);

    //         if (
    //             (i != 7) &&
    //             (i != 9) &&
    //             (i != 10) &&
    //             (i != 11) &&
    //             (i != 12) &&
    //             (i != 31) &&
    //             (i != 32) &&
    //             (i != 33) &&
    //             (i != 45) &&
    //             (i != 46) &&
    //             (i != 47) &&
    //             (i < 51)
    //             )
    //         {
    //             IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_IOTHUB_OPEN_COMPLETE_CALLBACK, NULL);

    //             //assert
    //             ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result);

    //             //cleanup
    //         }
    //     }
    //     umock_c_negative_tests_deinit();

    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Close_return_if_input_parameter_messagingHandle_is_NULL)
    // {
    //     // arrange

    //     // act
    //     IoTHubMessaging_LL_Close(NULL);

    //     // assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Close_happy_path)
    // {
    //     // arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(messagesender_destroy(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(messagereceiver_destroy(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(link_destroy(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(link_destroy(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(session_destroy(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(connection_destroy(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(saslmechanism_destroy(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //     // act
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);

    //     // assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     //cleanup
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    // {
    //     //arrange

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(NULL, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_deviceId_is_NULL)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
    //     umock_c_reset_all_calls();

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, NULL, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_message_is_NULL)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
    //     umock_c_reset_all_calls();

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, NULL, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_INVALID_ARG, result);

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Send_return_IOTHUB_MESSAGING_ERROR_if_messaging_is_not_opened)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     umock_c_reset_all_calls();

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_ERROR, result);

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Send_happy_path)
    // {
    //     //arrange
    //     size_t number_of_arguments = 1;
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(message_create());
    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(message_get_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments()
    //         .CopyOutArgumentBuffer_properties(&TEST_PROPERTIES_HANDLE_NULL, sizeof(TEST_PROPERTIES_HANDLE_NULL));
    //     STRICT_EXPECTED_CALL(properties_create());

    //     STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments()
    //         .SetReturn(TEST_CONST_CHAR_PTR);
    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(properties_set_message_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments()
    //         .SetReturn(TEST_CONST_CHAR_PTR);
    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(properties_set_correlation_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(properties_set_to(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(message_set_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(properties_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE))
    //         .IgnoreAllArguments()
    //         .SetReturn(TEST_MAP_HANDLE);

    //     STRICT_EXPECTED_CALL(Map_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments()
    //         .CopyOutArgumentBuffer_keys(&pTEST_MAP_KEYS, sizeof(pTEST_MAP_KEYS))
    //         .CopyOutArgumentBuffer_values(&pTEST_MAP_VALUES, sizeof(pTEST_MAP_VALUES))
    //         .CopyOutArgumentBuffer_count(&number_of_arguments, sizeof(size_t))
    //         .SetReturn(MAP_OK);

    //     STRICT_EXPECTED_CALL(amqpvalue_create_map());

    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(message_set_application_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(messagesender_send_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     //act
    //     IOTHUB_MESSAGING_RESULT result;
    //     result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_Send_non_happy_path)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
    //     umock_c_reset_all_calls();

    //     int umockc_result = umock_c_negative_tests_init();
    //     ASSERT_ARE_EQUAL(int, 0, umockc_result);

    //     size_t doNotFailCalls[] =
    //     {
    //         6,  /*contentType*/
    //         8,  /*contentType*/
    //         9,  /*message_get_properties*/
    //         10, /*properties_create*/
    //         11, /*properties_set_message_id*/
    //         12, /*properties_set_correlation_id*/
    //         14, /*message_set_application_properties*/
    //         15, /*amqpvalue_destroy*/
    //         18, /*amqpvalue_destroy*/
    //         19, /*amqpvalue_destroy*/
    //         25, /*properties_destroy*/
    //         26, /*amqpvalue_destroy*/
    //         27, /*amqpvalue_destroy*/
    //         28, /*amqpvalue_destroy*/
    //         30  /*gballoc_free*/
    //     };

    //     size_t number_of_arguments = 1;

    //     umock_c_reset_all_calls();
    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    //     STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(message_create());
    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(message_get_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments()
    //         .CopyOutArgumentBuffer_properties(&TEST_PROPERTIES_HANDLE_NULL, sizeof(TEST_PROPERTIES_HANDLE_NULL));
    //     STRICT_EXPECTED_CALL(properties_create());

    //     STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments()
    //         .SetReturn(TEST_CONST_CHAR_PTR);
    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(properties_set_message_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments()
    //         .SetReturn(TEST_CONST_CHAR_PTR);
    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(properties_set_correlation_id(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(properties_set_to(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(message_set_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(properties_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE))
    //         .IgnoreAllArguments()
    //         .SetReturn(TEST_MAP_HANDLE);

    //     STRICT_EXPECTED_CALL(Map_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments()
    //         .CopyOutArgumentBuffer_keys(&pTEST_MAP_KEYS, sizeof(pTEST_MAP_KEYS))
    //         .CopyOutArgumentBuffer_values(&pTEST_MAP_VALUES, sizeof(pTEST_MAP_VALUES))
    //         .CopyOutArgumentBuffer_count(&number_of_arguments, sizeof(size_t))
    //         .SetReturn(MAP_OK);

    //     STRICT_EXPECTED_CALL(amqpvalue_create_map());

    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(message_set_application_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();
    //     STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(messagesender_send_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     umock_c_negative_tests_snapshot();

    //     //act
    //     for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    //     {
    //         umock_c_negative_tests_reset();
    //         size_t j;
    //         for ( j = 0; j < sizeof(doNotFailCalls) / sizeof(doNotFailCalls[0]); j++)
    //         {
    //             if (doNotFailCalls[j] == i)
    //             {
    //                 break;
    //             }
    //         }

    //         if (j == sizeof(doNotFailCalls) / sizeof(doNotFailCalls[0]))
    //         {
    //             umock_c_negative_tests_fail_call(i);

    //             IOTHUB_MESSAGING_RESULT result;

    //             /* If modules are re-enabled, re-enable this code and add testing_module paramater to this function
    //             if (testing_module == true)
    //             {
    //                 result = IoTHubMessaging_LL_SendModule(TEST_IOTHUB_MESSAGING_HANDLE, TEST_CONST_CHAR_PTR, TEST_MODULE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);
    //             }
    //             */
    //             result = IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_CONST_CHAR_PTR, TEST_IOTHUB_MESSAGE_HANDLE, TEST_IOTHUB_SEND_COMPLETE_CALLBACK, TEST_VOID_PTR);

    //             //assert
    //             ASSERT_ARE_NOT_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, result);
    //         }

    //     }
    //     umock_c_negative_tests_deinit();

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SetFeedbackMessageCallback_return_IOTHUB_MESSAGING_INVALID_ARG_if_input_parameter_messagingHandle_is_NULL)
    // {
    //     //arrange

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetFeedbackMessageCallback(NULL, TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, TEST_VOID_PTR);

    //     //assert
    //     ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_INVALID_ARG, result);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SetFeedbackMessageCallback_happy_path)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
    //     umock_c_reset_all_calls();

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, TEST_VOID_PTR);

    //     //assert
    //     //ASSERT_IS_TRUE(TEST_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK == TEST_IOTHUB_MESSAGING_DATA.callback_data->feedbackMessageCallback);
    //     //ASSERT_ARE_EQUAL(void_ptr, TEST_IOTHUB_MESSAGING_DATA.callback_data->feedbackUserContext, TEST_VOID_PTR);
    //     ASSERT_ARE_EQUAL(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_OK, result);

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_DoWork_return_if_input_parameter_messagingHandle_is_NULL)
    // {
    //     //arrange

    //     //act
    //     IoTHubMessaging_LL_DoWork(NULL);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_DoWork_happy_path)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     //act
    //     IoTHubMessaging_LL_DoWork(iothub_messaging_handle);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_call_user_callback)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     onMessageReceiverStateChangedCallback(msg_recv_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_IDLE);
    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPEN;
    //     MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

    //     //act
    //     onMessageSenderStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_messaging_is_not_opened)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);

    //     umock_c_reset_all_calls();

    //     MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPENING;
    //     MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

    //     //act
    //     onMessageSenderStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SenderStateChanged_openCompleteCompleteCallback_is_null)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
    //     onMessageReceiverStateChangedCallback(msg_recv_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_IDLE);
    //     umock_c_reset_all_calls();

    //     MESSAGE_SENDER_STATE new_state = MESSAGE_SENDER_STATE_OPEN;
    //     MESSAGE_SENDER_STATE previous_state = MESSAGE_SENDER_STATE_IDLE;

    //     //act
    //     onMessageSenderStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_call_user_callback)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     onMessageSenderStateChangedCallback(msg_send_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPEN;
    //     MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

    //     //act
    //     onMessageReceiverStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_messaging_is_not_opened)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     onMessageSenderStateChangedCallback(msg_send_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
    //     umock_c_reset_all_calls();

    //     MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPENING;
    //     MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

    //     //act
    //     onMessageReceiverStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_ReceiverStateChanged_openCompleteCompleteCallback_is_null)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, NULL, NULL);
    //     onMessageSenderStateChangedCallback(msg_send_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
    //     umock_c_reset_all_calls();

    //     MESSAGE_RECEIVER_STATE new_state = MESSAGE_RECEIVER_STATE_OPEN;
    //     MESSAGE_RECEIVER_STATE previous_state = MESSAGE_RECEIVER_STATE_IDLE;

    //     //act
    //     onMessageReceiverStateChangedCallback(iothub_messaging_handle, new_state, previous_state);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SendMessageComplete_context_is_null)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK, (void*)1);

    //     umock_c_reset_all_calls();

    //     MESSAGE_SEND_RESULT send_result = MESSAGE_SEND_OK;

    //     //act
    //     onMessageSendCompleteCallback((void*)NULL, send_result, TEST_AMQP_VALUE);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SendMessageComplete_sendCompleteCallback_null)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE, NULL, NULL);

    //     umock_c_reset_all_calls();

    //     MESSAGE_SEND_RESULT send_result = MESSAGE_SEND_OK;


    //     //act
    //     onMessageSendCompleteCallback(iothub_messaging_handle, send_result, TEST_AMQP_VALUE);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     //cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SendMessageCompleteDevice_call_to_user_callback)
    // {
    //     //arrange
    //     umock_c_reset_all_calls();

    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     /* If modules are re-enabled, re-enable this code and add testing_module paramater to this function
    //     if (testing_module == true)
    //     {
    //         (void)IoTHubMessaging_LL_SendModule(iothub_messaging_handle, TEST_DEVICE_ID, TEST_MODULE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK, (void*)1);
    //     }
    //     */
    //     (void)IoTHubMessaging_LL_Send(iothub_messaging_handle, TEST_DEVICE_ID, TEST_IOTHUB_MESSAGE_HANDLE, TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK, (void*)1);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_SEND_COMPLETE_CALLBACK(IGNORED_PTR_ARG, IOTHUB_MESSAGING_OK));

    //     MESSAGE_SEND_RESULT send_result = MESSAGE_SEND_OK;

    //     //act
    //     onMessageSendCompleteCallback((void*)iothub_messaging_handle, send_result, TEST_AMQP_VALUE);

    //     //assert
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_context_is_null)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(messaging_delivery_accepted());

    //     //act
    //     void* amqp_result = (void*)onMessageReceivedCallback(NULL, TEST_MESSAGE_HANDLE);

    //     //assert
    //     ASSERT_IS_NOT_NULL(amqp_result);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_success)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

    //     receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;

    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_create());

    //     STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
    //         .SetReturn(TEST_JSON_OBJECT);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
    //         .SetReturn("deviceId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
    //         .SetReturn("generationId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
    //         .SetReturn("SuCcEsS");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
    //         .SetReturn("time");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
    //         .SetReturn("originalMessageId");


    //     STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(messaging_delivery_accepted());

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG));
    //     STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG));

    //     //act
    //     onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS, receivedFeedbackStatusCode);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_expired)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

    //     receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;

    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_create());

    //     STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
    //         .SetReturn(TEST_JSON_OBJECT);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
    //         .SetReturn("deviceId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
    //         .SetReturn("generationId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
    //         .SetReturn("ExPiReD");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
    //         .SetReturn("time");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
    //         .SetReturn("originalMessageId");


    //     STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(messaging_delivery_accepted());

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG));
    //     STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG));

    //     //act
    //     onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_EXPIRED, receivedFeedbackStatusCode);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_deliverycountexceeded)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

    //     receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;

    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_create());

    //     STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
    //         .SetReturn(TEST_JSON_OBJECT);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
    //         .SetReturn("deviceId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
    //         .SetReturn("generationId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
    //         .SetReturn("DeLiVeRyCoUnTeXcEeDeD");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
    //         .SetReturn("time");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
    //         .SetReturn("originalMessageId");

    //     STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //     STRICT_EXPECTED_CALL(messaging_delivery_accepted());

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);
    //     STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     //act
    //     onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_DELIVER_COUNT_EXCEEDED, receivedFeedbackStatusCode);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_rejected)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

    //     receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;

    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_create());

    //     STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
    //         .SetReturn(TEST_JSON_OBJECT);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
    //         .SetReturn("deviceId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
    //         .SetReturn("generationId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
    //         .SetReturn("ReJeCtEd");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
    //         .SetReturn("time");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
    //         .SetReturn("originalMessageId");


    //     STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(messaging_delivery_accepted());

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);
    //     STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     //act
    //     onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_REJECTED, receivedFeedbackStatusCode);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }


    // TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_unknown)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

    //     receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS;

    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_create());

    //     STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
    //         .SetReturn(TEST_JSON_OBJECT);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
    //         .SetReturn("deviceId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
    //         .SetReturn("generationId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
    //         .SetReturn("XxX");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
    //         .SetReturn("time");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
    //         .SetReturn("originalMessageId");


    //     STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(messaging_delivery_accepted());

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);
    //     STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     //act
    //     onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN, receivedFeedbackStatusCode);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_happy_path_feedback_null)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

    //     receivedFeedbackStatusCode = IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS;

    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_create());

    //     STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
    //         .SetReturn(TEST_JSON_OBJECT);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
    //         .SetReturn("deviceId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
    //         .SetReturn("generationId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
    //         .SetReturn(NULL);
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
    //         .SetReturn("time");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
    //         .SetReturn("originalMessageId");

    //     STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    //     STRICT_EXPECTED_CALL(messaging_delivery_accepted());

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_destroy(IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_clear(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);
    //     STRICT_EXPECTED_CALL(json_value_free(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     //act
    //     onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN, receivedFeedbackStatusCode);
    //     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_FeedbackMessageReceived_non_happy_path)
    // {
    //     //arrange
    //     int umockc_result = umock_c_negative_tests_init();
    //     ASSERT_ARE_EQUAL(int, 0, umockc_result);

    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     (void)IoTHubMessaging_LL_Open(iothub_messaging_handle, TEST_FUNC_IOTHUB_OPEN_COMPLETE_CALLBACK, (void*)1);
    //     (void)IoTHubMessaging_LL_SetFeedbackMessageCallback(iothub_messaging_handle, TEST_FUNC_IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK, (void*)1);

    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, IGNORED_NUM_ARG, &TEST_BINARY_DATA_INST))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_value_get_array(TEST_JSON_VALUE))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_array_get_count(TEST_JSON_ARRAY))
    //         .SetReturn(1);

    //     STRICT_EXPECTED_CALL(singlylinkedlist_create());

    //     STRICT_EXPECTED_CALL(json_array_get_object(TEST_JSON_ARRAY, 0))
    //         .SetReturn(TEST_JSON_OBJECT);

    //     STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
    //         .IgnoreArgument(1);

    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_ID))
    //         .SetReturn("deviceId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID))
    //         .SetReturn("generationId");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_DESCRIPTION))
    //         .SetReturn("success");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC))
    //         .SetReturn("time");
    //     STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, TEST_FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID))
    //         .SetReturn("originalMessageId");


    //     STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    //         .IgnoreAllArguments();

    //     STRICT_EXPECTED_CALL(messaging_delivery_accepted());

    //     umock_c_negative_tests_snapshot();

    //     for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    //     {
    //         /// arrange
    //         umock_c_negative_tests_reset();
    //         umock_c_negative_tests_fail_call(i);

    //         if (i <= 9)
    //         {
    //             //act
    //             AMQP_VALUE amqp_result = onMessageReceivedCallback(iothub_messaging_handle, TEST_MESSAGE_HANDLE);

    //             //assert
    //             if (i < 7)
    //             {
    //                 if (amqp_result != NULL)
    //                 {
    //                     ASSERT_ARE_EQUAL(int, 0, 1);
    //                 }
    //             }
    //         }
    //     }
    //     umock_c_negative_tests_deinit();

    //     ///cleanup
    //     IoTHubMessaging_LL_Close(iothub_messaging_handle);
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_success)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TRUSTED_CERT));

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

    //     //cleanup
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    // TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_2_calls_success)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);
    //     umock_c_reset_all_calls();

    //     STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TRUSTED_CERT));
    //     STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //     //act
    //     result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, TEST_TRUSTED_CERT);

    //     //assert
    //     ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, result);

    //     //cleanup
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

    TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_handle_NULL_fail)
    {
        //arrange

        //act
        IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(NULL, TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result);

        //cleanup
    }

    // TEST_FUNCTION(IoTHubMessaging_LL_SetTrustedCert_trusted_cert_NULL_fail)
    // {
    //     //arrange
    //     IOTHUB_MESSAGING_HANDLE iothub_messaging_handle = IoTHubMessaging_LL_Create(TEST_IOTHUB_SERVICE_CLIENT_AUTH_HANDLE);
    //     umock_c_reset_all_calls();

    //     //act
    //     IOTHUB_MESSAGING_RESULT result = IoTHubMessaging_LL_SetTrustedCert(iothub_messaging_handle, NULL);

    //     //assert
    //     ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGING_OK, result);

    //     //cleanup
    //     IoTHubMessaging_LL_Destroy(iothub_messaging_handle);
    // }

END_TEST_SUITE(iothub_messaging_ll_ut)
