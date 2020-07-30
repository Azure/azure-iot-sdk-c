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

static void* real_malloc(size_t size)
{
    return malloc(size);
}

static void real_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"
#include "umock_c/umocktypes_bool.h"

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
            real_free(ptr);
            j++;
        }

        saved_malloc_returns[i] = saved_malloc_returns[j];
    }

    if (i != j) saved_malloc_returns_count--;
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/uuid.h"

#include "iothub_message.h"
#include "azure_uamqp_c/amqp_definitions_application_properties.h"
#include "azure_uamqp_c/amqp_definitions_data.h"
#include "azure_uamqp_c/message.h"

#undef ENABLE_MOCKS

#include "internal/uamqp_messaging.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

#define TEST_IOTHUB_MESSAGE_HANDLE (IOTHUB_MESSAGE_HANDLE)0x100
#define TEST_MESSAGE_HANDLE (MESSAGE_HANDLE)0x101
#define TEST_STRING "Test string!! $%^%2F0x011"
#define TEST_MAP_HANDLE (MAP_HANDLE)0x103
#define TEST_AMQP_VALUE (AMQP_VALUE)0x104
#define TEST_PROPERTIES_HANDLE (PROPERTIES_HANDLE)0x107
#define TEST_CORRELATION_ID "Test Correlation Id"

#define TEST_AMQP_ENCODING_SIZE 5

static char g_encoding_buffer[TEST_AMQP_ENCODING_SIZE * 3];

#define UUID_N_OF_OCTECTS 16
#define UUID_STRING_SIZE 37

static char* TEST_UUID_STRING = "7f907d75-5e13-44cf-a1a3-19a01a2b4528";
static uuid TEST_UUID_BYTES = { 222,193,74,152,197,252,67,14,180,227,51,193,196,52,220,175 };


static char** TEST_MAP_KEYS;
static char** TEST_MAP_VALUES;
static AMQP_VALUE TEST_AMQP_VALUE2 = TEST_AMQP_VALUE;
static PROPERTIES_HANDLE TEST_PROPERTIES_HANDLE_PTR = TEST_PROPERTIES_HANDLE;

static PROPERTIES_HANDLE saved_properties_get_message_id_properties;
static AMQP_VALUE saved_properties_get_message_id_message_id_value = TEST_AMQP_VALUE;
static int saved_properties_get_message_id_return = 0;

static PROPERTIES_HANDLE saved_properties_get_correlation_id_properties;
static AMQP_VALUE saved_properties_get_correlation_id_correlation_id_value = TEST_AMQP_VALUE;
static int saved_properties_get_correlation_id_return = 0;

static AMQP_VALUE saved_amqpvalue_get_string_value;
static const char* saved_amqpvalue_get_string_string_value = TEST_STRING;
static int saved_amqpvalue_get_string_return = 0;

static const char* TEST_CONTENT_TYPE = "text/plain";
static const char* TEST_CONTENT_ENCODING = "utf8";
static IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA TEST_DIAGNOSTIC_DATA = { "12345678",  "1506054179" };


static int test_properties_get_message_id(PROPERTIES_HANDLE properties, AMQP_VALUE* message_id_value)
{
    saved_properties_get_message_id_properties = properties;
    *message_id_value = saved_properties_get_message_id_message_id_value;
    return saved_properties_get_message_id_return;
}

static int test_properties_get_correlation_id(PROPERTIES_HANDLE properties, AMQP_VALUE* correlation_id_value)
{
    saved_properties_get_correlation_id_properties = properties;
    *correlation_id_value = saved_properties_get_correlation_id_correlation_id_value;
    return saved_properties_get_correlation_id_return;
}

static int test_amqpvalue_get_string(AMQP_VALUE value, const char** string_value)
{
    saved_amqpvalue_get_string_value = value;
    *string_value = saved_amqpvalue_get_string_string_value;
    return saved_amqpvalue_get_string_return;
}

static AMQP_VALUE saved_amqpvalue_get_ulong_value = NULL;
static uint64_t test_amqpvalue_get_ulong_ulong_value = 10;
static int test_amqpvalue_get_ulong_return = 0;

static int test_amqpvalue_get_ulong(AMQP_VALUE value, uint64_t* ulong_value)
{
    saved_amqpvalue_get_ulong_value = value;
    *ulong_value = test_amqpvalue_get_ulong_ulong_value;
    return test_amqpvalue_get_ulong_return;
}

static AMQP_VALUE saved_amqpvalue_get_uuid_value = NULL;
static uuid* test_amqpvalue_get_uuid_uuid_value = &TEST_UUID_BYTES;
static int test_amqpvalue_get_uuid_return = 0;

static int test_amqpvalue_get_uuid(AMQP_VALUE value, uuid* uuid_value)
{
    saved_amqpvalue_get_uuid_value = value;
    uuid_value = test_amqpvalue_get_uuid_uuid_value;
    return test_amqpvalue_get_uuid_return;
}

static void set_add_map_item(void)
{
    STRICT_EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(TEST_AMQP_VALUE, TEST_AMQP_VALUE, TEST_AMQP_VALUE));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
}

static void set_exp_calls_for_create_encoded_annotations_properties(bool has_diagnostic_properties, bool has_security_props)
{
    size_t encoding_size = TEST_AMQP_ENCODING_SIZE;

    if (has_diagnostic_properties)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetDiagnosticPropertyData(TEST_IOTHUB_MESSAGE_HANDLE)).CallCannotFail();
        STRICT_EXPECTED_CALL(amqpvalue_create_map());

        set_add_map_item();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        // Add Map Item
        set_add_map_item();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetDiagnosticPropertyData(TEST_IOTHUB_MESSAGE_HANDLE)).SetReturn(NULL);
    }

    STRICT_EXPECTED_CALL(IoTHubMessage_IsSecurityMessage(TEST_IOTHUB_MESSAGE_HANDLE)).CallCannotFail().SetReturn(has_security_props);
    if (has_security_props)
    {
        set_add_map_item();
    }

    if (has_diagnostic_properties || has_security_props)
    {
        STRICT_EXPECTED_CALL(amqpvalue_create_message_annotations(TEST_AMQP_VALUE));
        STRICT_EXPECTED_CALL(amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &encoding_size, sizeof(encoding_size));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
    }
}

static void set_exp_calls_for_create_encoded_message_properties(bool has_message_id, bool has_correlation_id, const char* content_type, const char* content_encoding)
{
    size_t encoding_size = TEST_AMQP_ENCODING_SIZE;

    STRICT_EXPECTED_CALL(properties_create());

    if (has_message_id)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(TEST_IOTHUB_MESSAGE_HANDLE)).CallCannotFail();
        STRICT_EXPECTED_CALL(amqpvalue_create_string(TEST_STRING));
        STRICT_EXPECTED_CALL(properties_set_message_id(TEST_PROPERTIES_HANDLE, TEST_AMQP_VALUE));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetMessageId(TEST_IOTHUB_MESSAGE_HANDLE)).SetReturn(NULL);
    }

    if (has_correlation_id)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(TEST_IOTHUB_MESSAGE_HANDLE)).CallCannotFail();
        STRICT_EXPECTED_CALL(amqpvalue_create_string(TEST_CORRELATION_ID));
        STRICT_EXPECTED_CALL(properties_set_correlation_id(TEST_PROPERTIES_HANDLE, TEST_AMQP_VALUE));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetCorrelationId(TEST_IOTHUB_MESSAGE_HANDLE)).SetReturn(NULL);
    }

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(TEST_IOTHUB_MESSAGE_HANDLE)).CallCannotFail()
        .SetReturn(content_type);

    if (content_type != NULL)
    {
        STRICT_EXPECTED_CALL(properties_set_content_type(IGNORED_PTR_ARG, content_type));
    }

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(TEST_IOTHUB_MESSAGE_HANDLE)).CallCannotFail()
        .SetReturn(content_encoding);

    if (content_encoding != NULL)
    {
        STRICT_EXPECTED_CALL(properties_set_content_encoding(IGNORED_PTR_ARG, content_encoding));
    }

    STRICT_EXPECTED_CALL(amqpvalue_create_properties(TEST_PROPERTIES_HANDLE));
    STRICT_EXPECTED_CALL(amqpvalue_get_encoded_size(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &encoding_size, sizeof(encoding_size));
    STRICT_EXPECTED_CALL(properties_destroy(TEST_PROPERTIES_HANDLE));
}

static void set_exp_calls_for_create_encoded_application_properties(size_t number_of_app_properties)
{
    size_t encoding_size = TEST_AMQP_ENCODING_SIZE;

    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE)); //16
    STRICT_EXPECTED_CALL(Map_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &TEST_MAP_KEYS, sizeof(TEST_MAP_KEYS))
        .CopyOutArgumentBuffer(3, &TEST_MAP_VALUES, sizeof(TEST_MAP_VALUES))
        .CopyOutArgumentBuffer(4, &number_of_app_properties, sizeof(number_of_app_properties));

    if (number_of_app_properties > 0)
    {
        STRICT_EXPECTED_CALL(amqpvalue_create_map());

        for (size_t i = 0; i < number_of_app_properties; i++)
        {
            STRICT_EXPECTED_CALL(amqpvalue_create_string(TEST_MAP_KEYS[i]));
            STRICT_EXPECTED_CALL(amqpvalue_create_string(TEST_MAP_VALUES[i]));
            STRICT_EXPECTED_CALL(amqpvalue_set_map_value(TEST_AMQP_VALUE, TEST_AMQP_VALUE, TEST_AMQP_VALUE));
            STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
            STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
        }

        STRICT_EXPECTED_CALL(amqpvalue_create_application_properties(TEST_AMQP_VALUE));
        STRICT_EXPECTED_CALL(amqpvalue_get_encoded_size(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &encoding_size, sizeof(encoding_size));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    }
}

static void set_exp_calls_for_create_encoded_data(IOTHUBMESSAGE_CONTENT_TYPE msg_content_type)
{
    size_t encoding_size = TEST_AMQP_ENCODING_SIZE;

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE)).SetReturn(msg_content_type);

    if (msg_content_type == IOTHUBMESSAGE_BYTEARRAY)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else if (msg_content_type == IOTHUBMESSAGE_STRING)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(TEST_IOTHUB_MESSAGE_HANDLE));
    }

    data d;
    memset(&d, 0, sizeof(d));
    STRICT_EXPECTED_CALL(amqpvalue_create_data(d)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(amqpvalue_get_encoded_size(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &encoding_size, sizeof(encoding_size));
}

static void set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(size_t number_of_app_properties, IOTHUBMESSAGE_CONTENT_TYPE msg_content_type, bool has_message_id, bool has_correlation_id, bool has_diag_properties, bool has_security_props, const char* content_type, const char* content_encoding)
{
    set_exp_calls_for_create_encoded_message_properties(has_message_id, has_correlation_id, content_type, content_encoding);
    set_exp_calls_for_create_encoded_application_properties(number_of_app_properties);
    set_exp_calls_for_create_encoded_annotations_properties(has_diag_properties, has_security_props);

    set_exp_calls_for_create_encoded_data(msg_content_type);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetReturn(g_encoding_buffer);
    STRICT_EXPECTED_CALL(amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    if (number_of_app_properties > 0)
    {
        STRICT_EXPECTED_CALL(amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    if (has_diag_properties)
    {
        STRICT_EXPECTED_CALL(amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
    if (number_of_app_properties > 0)
    {
        STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
    }
    if (has_diag_properties)
    {
        STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
    }
    STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
}

static void set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(
    size_t number_of_properties,
    bool has_message_id,
    AMQP_TYPE message_id_type,
    bool has_correlation_id,
    AMQP_TYPE correlation_id_type,
    bool has_properties,
    const char* content_type,
    const char* content_encoding)
{
    static BINARY_DATA test_binary_data;
    test_binary_data.bytes = (const unsigned char*)&TEST_STRING;
    test_binary_data.length = strlen(TEST_STRING);

    MESSAGE_BODY_TYPE body_type = MESSAGE_BODY_TYPE_DATA;

    STRICT_EXPECTED_CALL(message_get_body_type(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument_body_type()
        .CopyOutArgumentBuffer_body_type(&body_type, sizeof(MESSAGE_BODY_TYPE));
    STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(TEST_MESSAGE_HANDLE, 0, IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .CopyOutArgumentBuffer_amqp_data(&test_binary_data, sizeof (BINARY_DATA));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).IgnoreAllArguments();

    // readPropertiesFromuAMQPMessage
    STRICT_EXPECTED_CALL(message_get_properties(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument_properties()
        .CopyOutArgumentBuffer_properties(&TEST_PROPERTIES_HANDLE_PTR, sizeof(PROPERTIES_HANDLE));

    STRICT_EXPECTED_CALL(properties_get_message_id(TEST_PROPERTIES_HANDLE, IGNORED_PTR_ARG)).IgnoreArgument_message_id_value();

    if (has_message_id)
    {
        STRICT_EXPECTED_CALL(amqpvalue_get_type(TEST_AMQP_VALUE)).SetReturn(message_id_type);

        if (message_id_type == AMQP_TYPE_STRING)
        {
            STRICT_EXPECTED_CALL(amqpvalue_get_string(TEST_AMQP_VALUE, IGNORED_PTR_ARG));
        }
        else if (message_id_type == AMQP_TYPE_ULONG)
        {
            STRICT_EXPECTED_CALL(amqpvalue_get_ulong(TEST_AMQP_VALUE, IGNORED_PTR_ARG));
        }
        else if (message_id_type == AMQP_TYPE_UUID)
        {
            STRICT_EXPECTED_CALL(amqpvalue_get_uuid(TEST_AMQP_VALUE, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(UUID_to_string(IGNORED_PTR_ARG));
        }

        STRICT_EXPECTED_CALL(IoTHubMessage_SetMessageId(TEST_IOTHUB_MESSAGE_HANDLE, IGNORED_PTR_ARG)).SetReturn(IOTHUB_MESSAGE_OK);

        if (message_id_type == AMQP_TYPE_UUID)
        {
            STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
        }
    }
    else
    {
        STRICT_EXPECTED_CALL(amqpvalue_get_type(TEST_AMQP_VALUE)).SetReturn(AMQP_TYPE_NULL);
    }

    STRICT_EXPECTED_CALL(properties_get_correlation_id(TEST_PROPERTIES_HANDLE, IGNORED_PTR_ARG)).IgnoreArgument_correlation_id_value();

    if (has_correlation_id)
    {
        STRICT_EXPECTED_CALL(amqpvalue_get_type(TEST_AMQP_VALUE)).SetReturn(message_id_type);

        if (correlation_id_type == AMQP_TYPE_STRING)
        {
            STRICT_EXPECTED_CALL(amqpvalue_get_string(TEST_AMQP_VALUE, IGNORED_PTR_ARG));
        }
        else if (correlation_id_type == AMQP_TYPE_ULONG)
        {
            STRICT_EXPECTED_CALL(amqpvalue_get_ulong(TEST_AMQP_VALUE, IGNORED_PTR_ARG));
        }
        else if (correlation_id_type == AMQP_TYPE_UUID)
        {
            STRICT_EXPECTED_CALL(amqpvalue_get_uuid(TEST_AMQP_VALUE, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(UUID_to_string(IGNORED_PTR_ARG));
        }

        STRICT_EXPECTED_CALL(IoTHubMessage_SetCorrelationId(TEST_IOTHUB_MESSAGE_HANDLE, IGNORED_PTR_ARG)).SetReturn(IOTHUB_MESSAGE_OK);

        if (message_id_type == AMQP_TYPE_UUID)
        {
            STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
        }
    }
    else
    {
        STRICT_EXPECTED_CALL(amqpvalue_get_type(TEST_AMQP_VALUE)).SetReturn(AMQP_TYPE_NULL);
    }

    STRICT_EXPECTED_CALL(properties_get_content_type(TEST_PROPERTIES_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &content_type, sizeof(content_type));

    if (content_type != NULL)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_SetContentTypeSystemProperty(TEST_IOTHUB_MESSAGE_HANDLE, content_type));
    }

    STRICT_EXPECTED_CALL(properties_get_content_encoding(TEST_PROPERTIES_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &content_encoding, sizeof(content_encoding));

    if (content_encoding != NULL)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_SetContentEncodingSystemProperty(TEST_IOTHUB_MESSAGE_HANDLE, content_encoding));
    }

    STRICT_EXPECTED_CALL(properties_destroy(TEST_PROPERTIES_HANDLE));

    // readApplicationPropertiesFromuAMQPMessage
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE)).SetReturn(TEST_MAP_HANDLE);

    if (has_properties)
    {
        STRICT_EXPECTED_CALL(message_get_application_properties(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .CopyOutArgumentBuffer_application_properties(&TEST_AMQP_VALUE2, sizeof(AMQP_VALUE));
        STRICT_EXPECTED_CALL(amqpvalue_get_inplace_described_value(TEST_AMQP_VALUE));
        STRICT_EXPECTED_CALL(amqpvalue_get_map_pair_count(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .CopyOutArgumentBuffer_pair_count((uint32_t *)&number_of_properties, sizeof(uint32_t));

        size_t i;
        for (i = 0; i < number_of_properties; i++)
        {
            STRICT_EXPECTED_CALL(amqpvalue_get_map_key_value_pair(TEST_AMQP_VALUE, (uint32_t)i, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument_key().IgnoreArgument_value()
                .CopyOutArgumentBuffer_key(&TEST_AMQP_VALUE2, sizeof(AMQP_VALUE))
                .CopyOutArgumentBuffer_value(&TEST_AMQP_VALUE2, sizeof(AMQP_VALUE));
            STRICT_EXPECTED_CALL(amqpvalue_get_string(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
                .IgnoreArgument_string_value().CopyOutArgumentBuffer_string_value(&TEST_MAP_KEYS[i], sizeof(char*));
            STRICT_EXPECTED_CALL(amqpvalue_get_string(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
                .IgnoreArgument_string_value().CopyOutArgumentBuffer_string_value(&TEST_MAP_VALUES[i], sizeof(char*));
            STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_HANDLE, TEST_MAP_KEYS[i], TEST_MAP_VALUES[i]));
            STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
            STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
        }

        STRICT_EXPECTED_CALL(amqpvalue_destroy(TEST_AMQP_VALUE));
    }
    else
    {
        STRICT_EXPECTED_CALL(message_get_application_properties(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG)).IgnoreArgument(2);
    }
}

static void reset_test_data()
{
    saved_amqpvalue_get_ulong_value = NULL;
    test_amqpvalue_get_ulong_ulong_value = 10;
    test_amqpvalue_get_ulong_return = 0;

    saved_amqpvalue_get_uuid_value = NULL;
    test_amqpvalue_get_uuid_uuid_value = &TEST_UUID_BYTES;
    test_amqpvalue_get_uuid_return = 0;

    saved_malloc_returns_count = 0;
    memset(saved_malloc_returns, 0, sizeof(saved_malloc_returns));
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

// ---------- amqp_binary data Structure Shell functions ---------- //
char* umock_stringify_data(const data* value)
{
    (void)value;
    char* result = "amqp_binary";
    return result;
}

int umock_are_equal_data(const data* left, const data* right)
{
    //force fall through to success bypassing access violation
    (void)left;
    (void)right;
    int result = 1;
    return result;
}

int umock_copy_data(data* destination, const data* source)
{
    //force fall through to success bypassing access violation
    (void)destination;
    (void)source;
    int result = 0;
    return result;
}

void umock_free_data(data* value)
{
    //do nothing
    (void)value;
}

BEGIN_TEST_SUITE(uamqp_messaging_ut)

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

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_CONTENT_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PROPERTIES_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(AMQPVALUE_ENCODER_OUTPUT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(message_annotations, void*);

    REGISTER_UMOCK_VALUE_TYPE(BINARY_DATA);
    REGISTER_UMOCK_VALUE_TYPE(data);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, TEST_free);

    REGISTER_GLOBAL_MOCK_HOOK(properties_get_message_id, test_properties_get_message_id);
    REGISTER_GLOBAL_MOCK_HOOK(properties_get_correlation_id, test_properties_get_correlation_id);
    REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_string, test_amqpvalue_get_string);

    REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_ulong, test_amqpvalue_get_ulong);
    REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_uuid, test_amqpvalue_get_uuid);

    REGISTER_GLOBAL_MOCK_RETURN(message_get_properties, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetMessageId, TEST_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetMessageId, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(properties_set_message_id, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_set_message_id, 1);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetCorrelationId, TEST_CORRELATION_ID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetCorrelationId, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_string, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_string, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_set_correlation_id, 1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_Properties, TEST_MAP_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_Properties, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_map, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_map, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_encode, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_encode, 1);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetInternals, MAP_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_set_map_value, 1);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetString, TEST_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetString, 0);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetContentType, IOTHUBMESSAGE_UNKNOWN);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_OK);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_add_body_amqp_data, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_set_application_properties, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_application_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_get_application_properties, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_application_properties, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_map_pair_count, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_map_pair_count, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_inplace_described_value, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_inplace_described_value, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_map_key_value_pair, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_map_key_value_pair, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_string, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_string, 1);

    REGISTER_GLOBAL_MOCK_RETURN(Map_AddOrUpdate, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_AddOrUpdate, MAP_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_CreateFromByteArray, TEST_IOTHUB_MESSAGE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_CreateFromByteArray, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(message_get_body_type, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_type, 1);

    REGISTER_GLOBAL_MOCK_RETURN(message_get_body_amqp_data_in_place, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_amqp_data_in_place, 1);

    REGISTER_GLOBAL_MOCK_RETURN(properties_create, TEST_PROPERTIES_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_properties, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_properties, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_encoded_size, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_encoded_size, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_data, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_data, 0);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_application_properties, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_application_properties, 0);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_null, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_null, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_symbol, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_symbol, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_message_annotations, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_message_annotations, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetContentTypeSystemProperty, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetContentEncodingSystemProperty, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(properties_set_content_type, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_set_content_type, 1);
    REGISTER_GLOBAL_MOCK_RETURN(properties_set_content_encoding, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_set_content_encoding, 1);
    REGISTER_GLOBAL_MOCK_RETURN(properties_get_content_type, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_get_content_type, 1);
    REGISTER_GLOBAL_MOCK_RETURN(properties_get_content_encoding, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(properties_get_content_encoding, 1);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetMessageId, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetMessageId, IOTHUB_MESSAGE_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetCorrelationId, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetCorrelationId, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetContentTypeSystemProperty, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetContentTypeSystemProperty, IOTHUB_MESSAGE_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetContentEncodingSystemProperty, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetContentEncodingSystemProperty, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_IsSecurityMessage, false);

    REGISTER_GLOBAL_MOCK_RETURN(UUID_to_string, TEST_UUID_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UUID_to_string, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetDiagnosticPropertyData, &TEST_DIAGNOSTIC_DATA);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetDiagnosticPropertyData, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(Map_GetInternals, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetInternals, MAP_ERROR);

    // Initialization of variables.
    TEST_MAP_KEYS = (char**)real_malloc(sizeof(char*) * 5);
    ASSERT_IS_NOT_NULL(TEST_MAP_KEYS, "Could not allocate memory for TEST_MAP_KEYS");
    TEST_MAP_KEYS[0] = "PROPERTY1";
    TEST_MAP_KEYS[1] = "Property2";
    TEST_MAP_KEYS[2] = " prop(3): ";
    TEST_MAP_KEYS[3] = "A!;";
    TEST_MAP_KEYS[4] = "\r\n\t";

    TEST_MAP_VALUES = (char**)real_malloc(sizeof(char*) * 5);
    ASSERT_IS_NOT_NULL(TEST_MAP_VALUES, "Could not allocate memory for TEST_MAP_VALUES");
    TEST_MAP_VALUES[0] = "sdfksdfjjjjlsdf";
    TEST_MAP_VALUES[1] = "23,424,355,543,534,535.0";
    TEST_MAP_VALUES[2] = "@#$$$ @_=-09!!^;:";
    TEST_MAP_VALUES[3] = "     \t\r\n      ";
    TEST_MAP_VALUES[4] = "-------------";
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    real_free(TEST_MAP_KEYS);
    real_free(TEST_MAP_VALUES);

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

// Tests_SRS_UAMQP_MESSAGING_31_118: [Gets data associated with IOTHUB_MESSAGE_HANDLE to encode, either from underlying byte array or string format.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_120: [Create a blob that contains AMQP encoding of IOTHUB_MESSAGE_HANDLE.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_112: [If optional message-id is present in the message, encode it into the AMQP message.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_113: [If optional correlation-id is present in the message, encode it into the AMQP message.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_114: [If optional content-type is present in the message, encode it into the AMQP message.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_115: [If optional content-encoding is present in the message, encode it into the AMQP message.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_116: [Gets message properties associated with the IOTHUB_MESSAGE_HANDLE to encode, returning the properties and their encoded length.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_117: [Get application message properties associated with the IOTHUB_MESSAGE_HANDLE to encode, returning the properties and their encoded length.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_32_001: [If optional diagnostic properties are present in the iot hub message, encode them into the AMQP message as annotation properties. Errors stop processing on this message.]
TEST_FUNCTION(message_create_uamqp_encoding_from_iothub_message_bytearray_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_BYTEARRAY, true, true, true, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    // act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_117: [Get application message properties associated with the IOTHUB_MESSAGE_HANDLE to encode, returning the properties and their encoded length.  Errors stop processing on this message.]
TEST_FUNCTION(message_create_from_iothub_message_zero_app_properties_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(0, IOTHUBMESSAGE_BYTEARRAY, true, true, true, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    // act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_118: [Gets data associated with IOTHUB_MESSAGE_HANDLE to encode, either from underlying byte array or string format.  Errors stop processing on this message.]
TEST_FUNCTION(message_create_from_iothub_message_string_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_STRING, true, true, true, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    ///act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_112: [If optional message-id is present in the message, encode it into the AMQP message.  Errors stop processing on this message.]
TEST_FUNCTION(message_create_from_iothub_message_no_message_id_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_STRING, false, true, true, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    ///act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_32_002: [If optional diagnostic properties are not present in the iot hub message, no error should happen.]
TEST_FUNCTION(message_create_from_iothub_message_no_diagnostic_properties_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_STRING, true, true, false, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    ///act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_113: [If optional correlation-id is present in the message, encode it into the AMQP message.  Errors stop processing on this message.]
TEST_FUNCTION(message_create_from_iothub_message_no_correlation_id_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_STRING, true, false, true, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    ///act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_114: [If optional content-type is present in the message, encode it into the AMQP message.  Errors stop processing on this message.]
TEST_FUNCTION(message_create_from_iothub_message_no_content_type_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_STRING, true, false, true, false, NULL, TEST_CONTENT_ENCODING);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    ///act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

TEST_FUNCTION(message_create_from_iothub_message_security_msg_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_STRING, true, false, true, true, NULL, TEST_CONTENT_ENCODING);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    ///act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_115: [If optional content-encoding is present in the message, encode it into the AMQP message.  Errors stop processing on this message.]
TEST_FUNCTION(message_create_from_iothub_message_no_content_encoding_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_STRING, true, false, true, false, TEST_CONTENT_TYPE, NULL);

    BINARY_DATA binary_data;
    memset(&binary_data, 0, sizeof(binary_data));

    ///act
    int result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_120: [Create a blob that contains AMQP encoding of IOTHUB_MESSAGE_HANDLE.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_121: [Any errors during `message_create_uamqp_encoding_from_iothub_message` stop processing on this message.]
TEST_FUNCTION(message_create_from_iothub_message_BYTEARRAY_return_errors_fails)
{
    // arrange
    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_BYTEARRAY, true, true, true, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        if (!umock_c_negative_tests_can_call_fail(i))
        {
            continue; // these lines have functions that do not return anything (void).
        }

        BINARY_DATA binary_data;
        memset(&binary_data, 0, sizeof(binary_data));

        result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

        ASSERT_ARE_NOT_EQUAL(int, result, 0, "On failed call %lu", (unsigned long)i);
    }

    // cleanup
    umock_c_negative_tests_reset();
    umock_c_negative_tests_deinit();
}

// Tests_SRS_UAMQP_MESSAGING_31_118: [Gets data associated with IOTHUB_MESSAGE_HANDLE to encode, either from underlying byte array or string format.  Errors stop processing on this message.]
// Tests_SRS_UAMQP_MESSAGING_31_121: [Any errors during `message_create_uamqp_encoding_from_iothub_message` stop processing on this message.]
TEST_FUNCTION(message_create_from_iothub_message_STRING_return_errors_fails)
{
    // arrange
    int result = 0;
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result);

    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_uamqp_encoding_from_iothub_message(1, IOTHUBMESSAGE_STRING, true, true, true, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    umock_c_negative_tests_snapshot();

    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        // arrange
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        if (!umock_c_negative_tests_can_call_fail(i))
        {
            continue; // these lines have functions that do not return anything (void).
        }

        BINARY_DATA binary_data;
        memset(&binary_data, 0, sizeof(binary_data));

        result = message_create_uamqp_encoding_from_iothub_message(NULL, TEST_IOTHUB_MESSAGE_HANDLE, &binary_data);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, result, 0, "On failed call %lu", (unsigned long)i);
    }

    // cleanup
    umock_c_negative_tests_reset();
    umock_c_negative_tests_deinit();
}

// Tests_SRS_UAMQP_MESSAGING_09_001: [The body type of the uAMQP message shall be retrieved using message_get_body_type().]
// Tests_SRS_UAMQP_MESSAGING_09_003: [If the uAMQP message body type is MESSAGE_BODY_TYPE_DATA, the body data shall be treated as binary data.]
// Tests_SRS_UAMQP_MESSAGING_09_004: [The uAMQP message body data shall be retrieved using message_get_body_amqp_data_in_place().]
// Tests_SRS_UAMQP_MESSAGING_09_006: [The IOTHUB_MESSAGE instance shall be created using IoTHubMessage_CreateFromByteArray(), passing the uAMQP body bytes as parameter.]
// Tests_SRS_UAMQP_MESSAGING_09_008: [The uAMQP message properties shall be retrieved using message_get_properties().]
// Tests_SRS_UAMQP_MESSAGING_09_010: [The message-id property shall be read from the uAMQP message by calling properties_get_message_id.]
// Tests_SRS_UAMQP_MESSAGING_09_012: [The type of the message-id property value shall be obtained using amqpvalue_get_type().]
// Tests_SRS_UAMQP_MESSAGING_09_014: [The message-id value shall be retrieved from the AMQP_VALUE as char sequence]
// Tests_SRS_UAMQP_MESSAGING_09_016: [The message-id property shall be set on the IOTHUB_MESSAGE_HANDLE instance by calling IoTHubMessage_SetMessageId(), passing the value read from the uAMQP message.]
// Tests_SRS_UAMQP_MESSAGING_09_018: [The correlation-id property shall be read from the uAMQP message by calling properties_get_correlation_id.]
// Tests_SRS_UAMQP_MESSAGING_09_020: [The type of the correlation-id property value shall be obtained using amqpvalue_get_type().]
// Tests_SRS_UAMQP_MESSAGING_09_022: [The correlation-id value shall be retrieved from the AMQP_VALUE as char* by calling amqpvalue_get_string.]
// Tests_SRS_UAMQP_MESSAGING_09_024: [The correlation-id property shall be set on the IOTHUB_MESSAGE_HANDLE by calling IoTHubMessage_SetCorrelationId, passing the value read from the uAMQP message.]
// Tests_SRS_UAMQP_MESSAGING_09_026: [message_create_IoTHubMessage_from_uamqp_message() shall destroy the uAMQP message properties (obtained with message_get_properties()) by calling properties_destroy().]
// Tests_SRS_UAMQP_MESSAGING_09_027: [The IOTHUB_MESSAGE_HANDLE properties shall be retrieved using IoTHubMessage_Properties.]
// Tests_SRS_UAMQP_MESSAGING_09_029: [The uAMQP message application properties shall be retrieved using message_get_application_properties.]
// Tests_SRS_UAMQP_MESSAGING_09_032: [The actual uAMQP message application properties should be extracted from the result of message_get_application_properties using amqpvalue_get_inplace_described_value.]
// Tests_SRS_UAMQP_MESSAGING_09_034: [The number of items in the uAMQP message application properties shall be obtained using amqpvalue_get_map_pair_count.]
// Tests_SRS_UAMQP_MESSAGING_09_036: [message_create_IoTHubMessage_from_uamqp_message() shall iterate through each uAMQP application property and add it to IOTHUB_MESSAGE_HANDLE properties.]
// Tests_SRS_UAMQP_MESSAGING_09_037: [The uAMQP application property name and value shall be obtained using amqpvalue_get_map_key_value_pair.]
// Tests_SRS_UAMQP_MESSAGING_09_039: [The uAMQP application property name shall be extracted as string using amqpvalue_get_string.]
// Tests_SRS_UAMQP_MESSAGING_09_041: [The uAMQP application property value shall be extracted as string using amqpvalue_get_string.]
// Tests_SRS_UAMQP_MESSAGING_09_043: [The application property name and value shall be added to IOTHUB_MESSAGE_HANDLE properties using Map_AddOrUpdate.]
// Tests_SRS_UAMQP_MESSAGING_09_045: [message_create_IoTHubMessage_from_uamqp_message() shall destroy the uAMQP message property name and value (obtained with amqpvalue_get_string) by calling amqpvalue_destroy().]
// Tests_SRS_UAMQP_MESSAGING_09_046: [message_create_IoTHubMessage_from_uamqp_message() shall destroy the uAMQP message property (obtained with message_get_application_properties) by calling amqpvalue_destroy().]
// Tests_SRS_UAMQP_MESSAGING_09_100: [If the uamqp message contains property `content-type`, it shall be set on IOTHUB_MESSAGE_HANDLE]
// Tests_SRS_UAMQP_MESSAGING_09_103: [If the uAMQP message contains property `content-encoding`, it shall be set on IOTHUB_MESSAGE_HANDLE]
TEST_FUNCTION(message_create_IoTHubMessage_from_uamqp_message_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(1, true, AMQP_TYPE_STRING, true, AMQP_TYPE_STRING, true, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    // act
    IOTHUB_MESSAGE_HANDLE iothub_client_message = NULL;
    int result = message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, &iothub_client_message);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, (void*)iothub_client_message, (void*)TEST_IOTHUB_MESSAGE_HANDLE);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_09_014: [The message-id value shall be retrieved from the AMQP_VALUE as char sequence]
// Tests_SRS_UAMQP_MESSAGING_09_022: [The correlation-id value shall be retrieved from the AMQP_VALUE as char* by calling amqpvalue_get_string.]
TEST_FUNCTION(message_create_IoTHubMessage_from_uamqp_message_ULONG_message_id_correlation_id_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(1, true, AMQP_TYPE_ULONG, true, AMQP_TYPE_ULONG, true, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    // act
    IOTHUB_MESSAGE_HANDLE iothub_client_message = NULL;
    int result = message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, &iothub_client_message);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, (void*)iothub_client_message, (void*)TEST_IOTHUB_MESSAGE_HANDLE);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_09_014: [The message-id value shall be retrieved from the AMQP_VALUE as char sequence]
// Tests_SRS_UAMQP_MESSAGING_09_022: [The correlation-id value shall be retrieved from the AMQP_VALUE as char* by calling amqpvalue_get_string.]
TEST_FUNCTION(message_create_IoTHubMessage_from_uamqp_message_UUID_message_id_correlation_id_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(1, true, AMQP_TYPE_UUID, true, AMQP_TYPE_UUID, true, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    // act
    IOTHUB_MESSAGE_HANDLE iothub_client_message = NULL;
    int result = message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, &iothub_client_message);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, (void*)iothub_client_message, (void*)TEST_IOTHUB_MESSAGE_HANDLE);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_09_013: [If the type of the message-id property value is AMQP_TYPE_NULL, message_create_IoTHubMessage_from_uamqp_message() shall skip processing the message-id (as it is optional) and continue normally.]
TEST_FUNCTION(message_create_IoTHubMessage_from_uamqp_message_no_message_id_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(1, false, AMQP_TYPE_STRING, true, AMQP_TYPE_STRING, true, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    // act
    IOTHUB_MESSAGE_HANDLE iothub_client_message = NULL;
    int result = message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, &iothub_client_message);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, (void*)iothub_client_message, (void*)TEST_IOTHUB_MESSAGE_HANDLE);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_09_021: [If the type of the correlation-id property value is AMQP_TYPE_NULL, message_create_IoTHubMessage_from_uamqp_message() shall skip processing the correlation-id (as it is optional) and continue normally.]
TEST_FUNCTION(message_create_IoTHubMessage_from_uamqp_message_no_correlation_id_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(1, true, AMQP_TYPE_STRING, false, AMQP_TYPE_STRING, true, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    // act
    IOTHUB_MESSAGE_HANDLE iothub_client_message = NULL;
    int result = message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, &iothub_client_message);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, (void*)iothub_client_message, (void*)TEST_IOTHUB_MESSAGE_HANDLE);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_09_031: [If message_get_application_properties succeeds but returns a NULL application properties map (there are no properties), message_create_IoTHubMessage_from_uamqp_message() shall skip processing the properties and continue normally.]
TEST_FUNCTION(message_create_IoTHubMessage_from_uamqp_message_no_app_properties_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(0, true, AMQP_TYPE_STRING, true, AMQP_TYPE_STRING, false, TEST_CONTENT_TYPE, TEST_CONTENT_ENCODING);

    // act
    IOTHUB_MESSAGE_HANDLE iothub_client_message = NULL;
    int result = message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, &iothub_client_message);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, (void*)iothub_client_message, (void*)TEST_IOTHUB_MESSAGE_HANDLE);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_122: [If the uAMQP message does not contain property `content-type`, it shall be skipped as it is optional]
TEST_FUNCTION(message_create_IoTHubMessage_from_uamqp_message_no_content_type_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(1, true, AMQP_TYPE_STRING, true, AMQP_TYPE_STRING, true, NULL, TEST_CONTENT_ENCODING);

    // act
    IOTHUB_MESSAGE_HANDLE iothub_client_message = NULL;
    int result = message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, &iothub_client_message);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, (void*)iothub_client_message, (void*)TEST_IOTHUB_MESSAGE_HANDLE);

    // cleanup
}

// Tests_SRS_UAMQP_MESSAGING_31_123: [If the uAMQP message does not contain property `content-encoding`, it shall be skipped as it is optional]
TEST_FUNCTION(message_create_IoTHubMessage_from_uamqp_message_no_content_encoding_success)
{
    // arrange
    umock_c_reset_all_calls();
    set_exp_calls_for_message_create_IoTHubMessage_from_uamqp_message(1, true, AMQP_TYPE_STRING, true, AMQP_TYPE_STRING, true, TEST_CONTENT_TYPE, NULL);

    // act
    IOTHUB_MESSAGE_HANDLE iothub_client_message = NULL;
    int result = message_create_IoTHubMessage_from_uamqp_message(TEST_MESSAGE_HANDLE, &iothub_client_message);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(void_ptr, (void*)iothub_client_message, (void*)TEST_IOTHUB_MESSAGE_HANDLE);

    // cleanup
}

END_TEST_SUITE(uamqp_messaging_ut)

