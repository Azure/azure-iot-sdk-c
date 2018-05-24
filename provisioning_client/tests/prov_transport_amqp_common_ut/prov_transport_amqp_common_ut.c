// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//#include <vld.h>

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#endif

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_stdint.h"
#include "umocktypes_bool.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/xio.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_prov_client/prov_transport.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/amqpvalue.h"
#include "azure_uamqp_c/link.h"
#undef ENABLE_MOCKS

#include "azure_prov_client/internal/prov_transport_amqp_common.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_prov_client/internal/prov_sasl_tpm.h"
#include "azure_c_shared_utility/buffer_.h"
#undef ENABLE_MOCKS

#define ENABLE_MOCKS

#include "azure_c_shared_utility/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, on_transport_register_data_cb, PROV_DEVICE_TRANSPORT_RESULT, transport_result, BUFFER_HANDLE, iothub_key, const char*, assigned_hub, const char*, device_id, void*, user_ctx);
MOCKABLE_FUNCTION(, void, on_transport_status_cb, PROV_DEVICE_TRANSPORT_STATUS, transport_status, void*, user_ctx);
MOCKABLE_FUNCTION(, char*, on_transport_challenge_callback, const unsigned char*, nonce, size_t, nonce_len, const char*, key_name, void*, user_ctx);
MOCKABLE_FUNCTION(, PROV_TRANSPORT_IO_INFO*, on_transport_io, const char*, fqdn, SASL_MECHANISM_HANDLE*, sasl_mechanism, const HTTP_PROXY_OPTIONS*, proxy_info);
MOCKABLE_FUNCTION(, PROV_JSON_INFO*, on_transport_json_parse, const char*, json_document, void*, user_ctx);

#undef ENABLE_MOCKS

#ifdef __cplusplus
extern "C"
{
#endif

    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...);
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

#ifdef __cplusplus
}
#endif

#define TEST_DPS_HANDLE     (PROV_DEVICE_TRANSPORT_HANDLE)0x11111111
#define TEST_BUFFER_VALUE   (BUFFER_HANDLE)0x11111112
#define TEST_INTERFACE_DESC (const SASL_MECHANISM_INTERFACE_DESCRIPTION*)0x11111113
#define TEST_XIO_HANDLE     (XIO_HANDLE)0x11111114
#define TEST_MESSAGE_HANDLE (MESSAGE_HANDLE)0x11111115
#define TEST_AMQP_VALUE     (AMQP_VALUE)0x11111116
#define TEST_DATA_VALUE     (unsigned char*)0x11111117
#define TEST_DATA_LENGTH    (size_t)128
#define TEST_BUFFER_HANDLE_VALUE     (BUFFER_HANDLE)0x1111111B

#define TEST_UNASSIGNED     (void*)0x11111118
#define TEST_ASSIGNED       (void*)0x11111119
#define TEST_ASSIGNING      (void*)0x1111111A

static const char* TEST_OPERATION_ID_VALUE = "operation_id";
static char* TEST_STRING_VALUE = "Test_String_Value";

static const char* TEST_URI_VALUE = "dps_uri";
static const char* TEST_DEVICE_ID_VALUE = "device_id";
static const char* TEST_AUTH_KEY_VALUE = "auth_key";
static const char* TEST_SCOPE_ID_VALUE = "scope_id";
static const char* TEST_REGISTRATION_ID_VALUE = "registration_id";
static const char* TEST_DPS_API_VALUE = "dps_api";
static const char* TEST_X509_CERT_VALUE = "x509_cert";
static const char* TEST_CERT_VALUE = "certificate";
static const char* TEST_PRIVATE_KEY_VALUE = "private_key";
static const char* TEST_HOST_ADDRESS_VALUE = "host_address";
static const char* TEST_SAS_TOKEN_VALUE = "sas_token";
static const char* TEST_UNASSIGNED_STATUS = "Unassigned";
static const char* TEST_ASSIGNED_STATUS = "Assigned";
static const char* TEST_OPERATION_ID = "operation_id";
static const char* TEST_USERNAME_VALUE = "username";
static const char* TEST_PASSWORD_VALUE = "password";
static const char* TEST_JSON_REPLY = "{ json_reply }";

static ON_MESSAGE_RECEIVER_STATE_CHANGED g_msg_rcvr_state_changed;
static void* g_msg_rcvr_state_changed_ctx;
static ON_MESSAGE_SENDER_STATE_CHANGED g_msg_sndr_state_changed;
static void* g_msg_sndr_state_changed_ctx;
static ON_MESSAGE_RECEIVED g_on_msg_recv;
static void* msg_recv_callback_context;
static TPM_CHALLENGE_CALLBACK g_challenge_cb;
static void* g_challenge_context;
static bool g_use_x509;

static PROV_DEVICE_TRANSPORT_STATUS g_target_transport_status;

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE_VALUES);

#ifdef __cplusplus
extern "C"
{
#endif
    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
    {
        (void)handle;
        (void)format;
        return 0;
    }

    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        return (STRING_HANDLE)my_gballoc_malloc(1);
    }
#ifdef __cplusplus
}
#endif

static void my_STRING_delete(STRING_HANDLE h)
{
    my_gballoc_free((void*)h);
}

static int my_message_get_application_properties(MESSAGE_HANDLE message, AMQP_VALUE* application_properties)
{
    (void)message;
    *application_properties = (AMQP_VALUE)my_gballoc_malloc(1);
    return 0;
}

static AMQP_VALUE my_amqpvalue_get_map_value(AMQP_VALUE map, AMQP_VALUE key)
{
    (void)map;
    (void)key;
    return (AMQP_VALUE)my_gballoc_malloc(1);
}

static int my_amqpvalue_get_string(AMQP_VALUE value, const char** string_value)
{
    (void)value;
    *string_value = TEST_UNASSIGNED_STATUS;
    return 0;
}

static int my_message_get_body_type(MESSAGE_HANDLE message, MESSAGE_BODY_TYPE* body_type)
{
    (void)message;
    *body_type = MESSAGE_BODY_TYPE_DATA;
    return 0;
}

static int my_message_get_body_amqp_data_in_place(MESSAGE_HANDLE message, size_t index, BINARY_DATA* amqp_data)
{
    (void)message;
    (void)index;
    amqp_data->length = strlen(TEST_JSON_REPLY);
    amqp_data->bytes = (unsigned char*)TEST_JSON_REPLY;
    return 0;
}

static char* my_on_transport_challenge_callback(const unsigned char* nonce, size_t nonce_len, const char* key_name, void* user_ctx)
{
    (void)nonce;
    (void)nonce_len;
    (void)key_name;
    (void)user_ctx;
    char* result;
    size_t src_len = strlen(TEST_SAS_TOKEN_VALUE);
    result = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(result, TEST_SAS_TOKEN_VALUE);
    return result;
}

static PROV_JSON_INFO* my_on_transport_json_parse(const char* json_document, void* user_ctx)
{
    (void)json_document;
    (void)user_ctx;
    PROV_JSON_INFO* result = (PROV_JSON_INFO*)my_gballoc_malloc(sizeof(PROV_JSON_INFO));
    if (g_target_transport_status == PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED)
    {
        result->prov_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED;
        result->authorization_key = (BUFFER_HANDLE)my_gballoc_malloc(1);
        result->iothub_uri = (char*)my_gballoc_malloc(strlen(TEST_URI_VALUE) + 1);
        strcpy(result->iothub_uri, TEST_URI_VALUE);

        result->device_id = (char*)my_gballoc_malloc(strlen(TEST_DEVICE_ID_VALUE) + 1);
        strcpy(result->device_id, TEST_DEVICE_ID_VALUE);
    }
    else if (g_target_transport_status == PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING)
    {
        result->prov_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        result->operation_id = (char*)my_gballoc_malloc(strlen(TEST_OPERATION_ID_VALUE) + 1);
        strcpy(result->operation_id, TEST_OPERATION_ID_VALUE);
    }
    else
    {
        result->prov_status = PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED;
        result->authorization_key = (BUFFER_HANDLE)my_gballoc_malloc(1);
        result->key_name = (char*)my_gballoc_malloc(strlen(TEST_STRING_VALUE) + 1);
        strcpy(result->key_name, TEST_STRING_VALUE);
    }
    return result;
}

static PROV_TRANSPORT_IO_INFO* my_on_transport_io(const char* fqdn, SASL_MECHANISM_HANDLE* sasl_mechanism, const HTTP_PROXY_OPTIONS* proxy_info)
{
    PROV_TRANSPORT_IO_INFO* result;
    (void)fqdn;
    (void)sasl_mechanism;
    (void)proxy_info;

    result = (PROV_TRANSPORT_IO_INFO*)my_gballoc_malloc(sizeof(PROV_TRANSPORT_IO_INFO) );
    result->transport_handle = (XIO_HANDLE)my_gballoc_malloc(1);
    if (!g_use_x509)
    {
        result->sasl_handle = (XIO_HANDLE)my_gballoc_malloc(1);
    }
    else
    {
        result->sasl_handle = NULL;
    }
    return result;
}

static int my_messagereceiver_open(MESSAGE_RECEIVER_HANDLE message_receiver, ON_MESSAGE_RECEIVED on_message_received, void* callback_context)
{
    (void)message_receiver;
    g_on_msg_recv = on_message_received;
    msg_recv_callback_context = callback_context;
    return 0;
}

static AMQP_VALUE my_amqpvalue_create_map(void)
{
    return (AMQP_VALUE)my_gballoc_malloc(1);
}

static MESSAGE_HANDLE my_message_create(void)
{
    return (MESSAGE_HANDLE)my_gballoc_malloc(1);
}

static MESSAGE_SENDER_HANDLE my_messagesender_create(LINK_HANDLE link, ON_MESSAGE_SENDER_STATE_CHANGED on_message_sender_state_changed, void* context)
{
    (void)link;
    g_msg_sndr_state_changed_ctx = context;
    g_msg_sndr_state_changed = on_message_sender_state_changed;
    return (MESSAGE_SENDER_HANDLE)my_gballoc_malloc(1);
}

static MESSAGE_RECEIVER_HANDLE my_messagereceiver_create(LINK_HANDLE link, ON_MESSAGE_RECEIVER_STATE_CHANGED on_message_receiver_state_changed, void* context)
{
    (void)link;
    g_msg_rcvr_state_changed_ctx = context;
    g_msg_rcvr_state_changed = on_message_receiver_state_changed;
    return (MESSAGE_RECEIVER_HANDLE)my_gballoc_malloc(1);
}

static SASL_MECHANISM_HANDLE my_saslmechanism_create(const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_mechanism_interface_description, void* sasl_mechanism_create_parameters)
{
    SASL_TPM_CONFIG_INFO* sasl_config = (SASL_TPM_CONFIG_INFO*)sasl_mechanism_create_parameters;
    (void)sasl_mechanism_interface_description;

    g_challenge_cb = sasl_config->challenge_cb;
    g_challenge_context = sasl_config->user_ctx;

    return (SASL_MECHANISM_HANDLE)my_gballoc_malloc(1);
}

static SESSION_HANDLE my_session_create(CONNECTION_HANDLE connection, ON_LINK_ATTACHED on_link_attached, void* callback_context)
{
    (void)connection;
    (void)on_link_attached;
    (void)callback_context;
    return (SESSION_HANDLE)my_gballoc_malloc(1);
}

static LINK_HANDLE my_link_create(SESSION_HANDLE session, const char* name, role role, AMQP_VALUE source, AMQP_VALUE target)
{
    (void)session;
    (void)name;
    (void)role;
    (void)source;
    (void)target;
    return (LINK_HANDLE)my_gballoc_malloc(1);
}

static void my_message_destroy(MESSAGE_HANDLE message)
{
    my_gballoc_free(message);
}

static void my_xio_destroy(XIO_HANDLE xio)
{
    my_gballoc_free(xio);
}

static void my_amqpvalue_destroy(AMQP_VALUE value)
{
    my_gballoc_free(value);
}

static AMQP_VALUE my_amqpvalue_create_string(const char* value)
{
    (void)value;
    return (AMQP_VALUE)my_gballoc_malloc(1);
}

static AMQP_VALUE my_messaging_create_source(const char* address)
{
    (void)address;
    return (AMQP_VALUE)my_gballoc_malloc(1);
}

static AMQP_VALUE my_messaging_create_target(const char* address)
{
    (void)address;
    return (AMQP_VALUE)my_gballoc_malloc(1);
}

static CONNECTION_HANDLE my_connection_create(XIO_HANDLE xio, const char* hostname, const char* container_id, ON_NEW_ENDPOINT on_new_endpoint, void* callback_context)
{
    (void)xio;
    (void)hostname;
    (void)container_id;
    (void)on_new_endpoint;
    (void)callback_context;
    return (CONNECTION_HANDLE)my_gballoc_malloc(1);
}

static void my_messagesender_destroy(MESSAGE_SENDER_HANDLE message_sender)
{
    my_gballoc_free(message_sender);
}

static void my_messagereceiver_destroy(MESSAGE_RECEIVER_HANDLE message_receiver)
{
    my_gballoc_free(message_receiver);
}

static void my_link_destroy(LINK_HANDLE link)
{
    my_gballoc_free(link);
}

static void my_saslmechanism_destroy(SASL_MECHANISM_HANDLE sasl_mechanism)
{
    my_gballoc_free(sasl_mechanism);
}

static void my_session_destroy(SESSION_HANDLE session)
{
    my_gballoc_free(session);
}

static void my_connection_destroy(CONNECTION_HANDLE connection)
{
    my_gballoc_free(connection);
}

static BUFFER_HANDLE my_BUFFER_clone(BUFFER_HANDLE handle)
{
    (void)handle;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    my_gballoc_free(handle);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static AMQP_VALUE my_amqpvalue_create_symbol(const char* value)
{
    (void)value;
    return (AMQP_VALUE)my_gballoc_malloc(1);
}

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(prov_transport_amqp_common_ut)

    TEST_SUITE_INITIALIZE(suite_init)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);
        (void)umocktypes_bool_register_types();
        (void)umocktypes_stdint_register_types();

        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT);
        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS);
        REGISTER_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE);

        REGISTER_UMOCK_ALIAS_TYPE(BINARY_DATA, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_SEND_COMPLETE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_STATUS_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_CHALLENGE_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(DPS_AMQP_TRANSPORT_IO, void*);
        REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_REQUEST_TYPE, int);
        REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_SENDER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_RECEIVER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(LINK_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SASL_MECHANISM_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(CONNECTION_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_NEW_ENDPOINT, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_LINK_ATTACHED, void*);
        REGISTER_UMOCK_ALIAS_TYPE(role, bool);
        REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(fields, void*);
        REGISTER_UMOCK_ALIAS_TYPE(receiver_settle_mode, uint8_t);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVER_STATE_CHANGED, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVED, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_SENDER_STATE_CHANGED, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(tickcounter_ms_t, uint32_t);


        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_clone, my_BUFFER_clone);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_clone, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

        REGISTER_GLOBAL_MOCK_HOOK(on_transport_io, my_on_transport_io);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_io, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(prov_sasltpm_get_interface, TEST_INTERFACE_DESC);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(prov_sasltpm_get_interface, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(saslmechanism_create, my_saslmechanism_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslmechanism_create, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(messaging_create_source, my_messaging_create_source);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_source, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(messaging_create_target, my_messaging_create_target);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_target, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(link_create, my_link_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_create, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(on_transport_challenge_callback, my_on_transport_challenge_callback);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_challenge_callback, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(on_transport_json_parse, my_on_transport_json_parse);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_json_parse, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(xio_destroy, my_xio_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(session_create, my_session_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(connection_create, my_connection_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(connection_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_create, my_messagereceiver_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(messagesender_create, my_messagesender_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_create, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_create_string, my_amqpvalue_create_string);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_string, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_create_map, my_amqpvalue_create_map);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_map, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_set_map_value, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_set_map_value, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(message_set_application_properties, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_set_application_properties, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(messagesender_send_async, (ASYNC_OPERATION_HANDLE)0x64);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_send_async, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_string, my_amqpvalue_get_string);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_string, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

        /*REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_map_value, my_amqpvalue_get_map_value);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_map_value, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(message_get_application_properties, my_message_get_application_properties);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_application_properties, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_inplace_described_value, TEST_AMQP_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_inplace_described_value, NULL);*/

        REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_create_symbol, my_amqpvalue_create_symbol);
        //REGISTER_GLOBAL_MOCK_RETURN(link_set_attach_properties, 0);
        REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_destroy, my_amqpvalue_destroy);

        REGISTER_GLOBAL_MOCK_HOOK(message_get_body_type, my_message_get_body_type);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_type, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(message_get_body_amqp_data_in_place, my_message_get_body_amqp_data_in_place);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_amqp_data_in_place, __LINE__);

        REGISTER_GLOBAL_MOCK_RETURN(messaging_delivery_accepted, TEST_AMQP_VALUE);

        REGISTER_GLOBAL_MOCK_HOOK(message_destroy, my_message_destroy);
        
        REGISTER_GLOBAL_MOCK_HOOK(message_create, my_message_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_rcv_settle_mode, 0);
        REGISTER_GLOBAL_MOCK_RETURN(link_set_rcv_settle_mode, __LINE__);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_create, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(messagesender_open, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagesender_open, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_open, my_messagereceiver_open);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_open, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(messagesender_destroy, my_messagesender_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_destroy, my_messagereceiver_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(link_destroy, my_link_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(saslmechanism_destroy, my_saslmechanism_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(session_destroy, my_session_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(connection_destroy, my_connection_destroy);
    }

    TEST_SUITE_CLEANUP(suite_cleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("Could not acquire test serialization mutex.");
        }
        umock_c_reset_all_calls();
        g_msg_rcvr_state_changed = NULL;
        g_msg_rcvr_state_changed_ctx = NULL;
        g_msg_sndr_state_changed = NULL;
        g_msg_sndr_state_changed_ctx = NULL;
        g_on_msg_recv = NULL;
        msg_recv_callback_context = NULL;
        g_challenge_cb = NULL;
        g_challenge_context = NULL;
        g_use_x509 = false;
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
    {
        int result = 0;
        for (size_t index = 0; index < length; index++)
        {
            if (current_index == skip_array[index])
            {
                result = __LINE__;
                break;
            }
        }
        return result;
    }

    static void setup_retrieve_amqp_property_mocks(const char* string_value)
    {
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_get_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_string_value(&string_value, sizeof(char*));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    }

    static void setup_on_message_recv_callback_mocks()
    {
        STRICT_EXPECTED_CALL(message_get_body_type(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(message_get_body_amqp_data_in_place(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(messaging_delivery_accepted());
    }

    static void setup_send_amqp_message_mocks(bool operation_id)
    {
        BINARY_DATA binary_data;
        binary_data.bytes = NULL;
        binary_data.length = 0;

        STRICT_EXPECTED_CALL(message_create());
        STRICT_EXPECTED_CALL(amqpvalue_create_map());
        STRICT_EXPECTED_CALL(message_add_body_amqp_data(IGNORED_PTR_ARG, binary_data)).IgnoreArgument_amqp_data();

        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

        if (operation_id)
        {
            STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(message_set_application_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagesender_send_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(message_destroy(IGNORED_PTR_ARG));
    }

    static void setup_create_sender_link_mocks(void)
    {
        STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, role_sender, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(amqpvalue_create_map());
        STRICT_EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(link_set_max_message_size(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(messagesender_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagesender_open(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    static void setup_create_receiver_link_mocks(void)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_create_source(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, role_receiver, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_set_rcv_settle_mode(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        STRICT_EXPECTED_CALL(amqpvalue_create_map());
        STRICT_EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_set_attach_properties(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(link_set_max_message_size(IGNORED_PTR_ARG, IGNORED_NUM_ARG)); //13
        STRICT_EXPECTED_CALL(messagereceiver_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagereceiver_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //18
    }

    static void setup_create_amqp_connection_mocks(bool use_x509)
    {
        STRICT_EXPECTED_CALL(on_transport_io(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (use_x509)
        {
            STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(connection_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(connection_set_trace(IGNORED_PTR_ARG, false));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_create_connection_mocks(bool use_x509)
    {
        STRICT_EXPECTED_CALL(prov_sasltpm_get_interface());
        if (!use_x509)
        {
            STRICT_EXPECTED_CALL(saslmechanism_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
        setup_create_amqp_connection_mocks(use_x509);

        STRICT_EXPECTED_CALL(session_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(session_set_incoming_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(session_set_outgoing_window(IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 7

        setup_create_receiver_link_mocks();
        setup_create_sender_link_mocks(); // 20
    }

    static void setup_prov_transport_common_amqp_create_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_URI_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_DPS_API_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SCOPE_ID_VALUE));
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_001: [ If uri, scope_id, registration_id, dps_api_version, or transport_io is NULL, prov_transport_common_amqp_create shall return NULL. ] */
    TEST_FUNCTION(prov_transport_common_amqp_create_uri_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(NULL, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_001: [ If uri, scope_id, registration_id, dps_api_version, or transport_io is NULL, prov_transport_common_amqp_create shall return NULL. ] */
    TEST_FUNCTION(prov_transport_common_amqp_create_scope_id_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, NULL, TEST_DPS_API_VALUE, on_transport_io);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_001: [ If uri, scope_id, registration_id, dps_api_version, or transport_io is NULL, prov_transport_common_amqp_create shall return NULL. ] */
    TEST_FUNCTION(prov_transport_common_amqp_create_api_version_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, NULL, on_transport_io);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_001: [ If uri, scope_id, registration_id, dps_api_version, or transport_io is NULL, prov_transport_common_amqp_create shall return NULL. ] */
    TEST_FUNCTION(prov_transport_common_amqp_create_transport_io_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_003: [ prov_transport_common_amqp_create shall allocate a DPS_TRANSPORT_AMQP_INFO structure ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_004: [ On success prov_transport_common_amqp_create shall allocate a new instance of PROV_DEVICE_TRANSPORT_HANDLE. ] */
    TEST_FUNCTION(prov_transport_common_amqp_create_fail)
    {
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_prov_transport_common_amqp_create_mocks();

        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "prov_transport_common_amqp_create failure in test %zu/%zu", index, count);

            //act
            PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);

            //assert
            ASSERT_IS_NULL_WITH_MSG(handle, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_002: [ If any error is encountered, prov_transport_common_amqp_create shall return NULL. ] */
    TEST_FUNCTION(prov_transport_common_amqp_create_success)
    {
        //arrange
        setup_prov_transport_common_amqp_create_mocks();

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);

        //assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_005: [ If handle is NULL, prov_transport_common_amqp_destroy shall do nothing. ] */
    TEST_FUNCTION(prov_transport_common_amqp_destroy_handle_NULL_fail)
    {
        //arrange

        //act
        prov_transport_common_amqp_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_006: [ prov_transport_common_amqp_destroy shall free all resources used in this module. ] */
    TEST_FUNCTION(prov_transport_common_amqp_destroy_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        prov_transport_common_amqp_destroy(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_007: [ If handle, data_callback, or status_cb is NULL, prov_transport_common_amqp_open shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_open_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_transport_common_amqp_open(NULL, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_amqp_open_registration_id_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_open(handle, NULL, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, NULL, NULL, on_transport_status_cb, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_007: [ If handle, data_callback, or status_cb is NULL, prov_transport_common_amqp_open shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_open_data_callback_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, NULL, NULL, on_transport_status_cb, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_007: [ If handle, data_callback, or status_cb is NULL, prov_transport_common_amqp_open shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_open_status_callback_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, NULL, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_008: [ If hsm_type is TRANSPORT_HSM_TYPE_TPM and ek or srk is NULL, prov_transport_common_amqp_open shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_open_ek_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_008: [ If hsm_type is TRANSPORT_HSM_TYPE_TPM and ek or srk is NULL, prov_transport_common_amqp_open shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_open_srk_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_010: [ When complete prov_transport_common_amqp_open shall send a status callback of PROV_DEVICE_TRANSPORT_STATUS_CONNECTED.] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_009: [ prov_transport_common_amqp_open shall clone the ek and srk values.] */
    TEST_FUNCTION(prov_transport_common_amqp_open_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_REGISTRATION_ID_VALUE));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, IGNORED_PTR_ARG));

        //act
        int result = prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_041: [ If a failure is encountered, prov_transport_common_amqp_open shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_open_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, IGNORED_PTR_ARG));

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 2 };

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            {
                continue;
            }

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "prov_transport_common_amqp_open failure in test %zu/%zu", index, count);

            int result = prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);

            //assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, tmp_msg);
        }

        //cleanup
        prov_transport_common_amqp_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_010: [ When complete prov_transport_common_amqp_open shall send a status callback of PROV_DEVICE_TRANSPORT_STATUS_CONNECTED.] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_009: [ prov_transport_common_amqp_open shall clone the ek and srk values.] */
    TEST_FUNCTION(prov_transport_common_amqp_open_x509_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_REGISTRATION_ID_VALUE));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, IGNORED_PTR_ARG));

        //act
        int result = prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_011: [ If handle is NULL, prov_transport_common_amqp_close shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_amqp_close_handle_NULL)
    {
        //arrange

        //act
        int result = prov_transport_common_amqp_close(NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_012: [ prov_transport_common_amqp_close shall close all links and connection associated with amqp communication. ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_013: [ On success prov_transport_common_amqp_close shall return a zero value. ] */
    TEST_FUNCTION(prov_transport_amqp_close_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagesender_close(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagereceiver_close(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(messagesender_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(messagereceiver_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(link_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(session_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(connection_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));

        //act
        int result = prov_transport_common_amqp_close(handle);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_014: [ If handle is NULL, prov_transport_common_amqp_register_device shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_register_device_handle_NULL_succeed)
    {
        //arrange

        //act
        int result = prov_transport_common_amqp_register_device(NULL, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_015: [ If hsm_type is of type TRANSPORT_HSM_TYPE_TPM and reg_challenge_cb is NULL, prov_transport_common_amqp_register_device shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_register_device_challenge_NULL_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_register_device(handle, NULL, NULL, on_transport_json_parse, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_015: [ If hsm_type is of type TRANSPORT_HSM_TYPE_TPM and reg_challenge_cb is NULL, prov_transport_common_amqp_register_device shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_register_device_x509_challenge_NULL_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_register_device(handle, NULL, NULL, on_transport_json_parse, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_017: [ On success prov_transport_common_amqp_register_device shall return a zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_register_device_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_061: [ If the transport_state is TRANSPORT_CLIENT_STATE_REG_SEND or the the operation_id is NULL, prov_transport_common_amqp_register_device shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_register_device_twice_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_018: [ If handle is NULL, prov_transport_common_amqp_get_operation_status shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_get_operation_status_handle_NULL_succeed)
    {
        //arrange

        //act
        int result = prov_transport_common_amqp_get_operation_status(NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_019: [ If the operation_id is NULL, prov_transport_common_amqp_get_operation_status shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_get_operation_status_operation_id_zero_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_get_operation_status(handle);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_021: [ prov_transport_common_amqp_get_operation_status shall set the transport_state to TRANSPORT_CLIENT_STATE_STATUS_SEND. ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_022: [ On success prov_transport_common_amqp_get_operation_status shall return a zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_get_operation_status_success)
    {
        int result;
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);
        prov_transport_common_amqp_dowork(handle);
        (void)g_on_msg_recv(msg_recv_callback_context, TEST_MESSAGE_HANDLE);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        result = prov_transport_common_amqp_get_operation_status(handle);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_042: [ If user_ctx is NULL, on_sasl_tpm_challenge_cb shall return NULL. ] */
    TEST_FUNCTION(prov_transport_common_amqp_challenge_cb_user_ctx_NULL_fail)
    {
        char* result;
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        result = g_challenge_cb(TEST_BUFFER_HANDLE_VALUE, NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_043: [ If the challenge_cb function is NULL, on_sasl_tpm_challenge_cb shall return NULL. ] */
    TEST_FUNCTION(prov_transport_common_amqp_challenge_cb_NULL_fail)
    {
        char* result;
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        result = g_challenge_cb(TEST_BUFFER_HANDLE_VALUE, g_challenge_context);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_045: [ on_sasl_tpm_challenge_cb shall call the challenge_cb returning the resulting value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_challenge_cb_succeed)
    {
        char* result;
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).SetReturn(TEST_DATA_VALUE);
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).SetReturn(TEST_DATA_LENGTH);
        STRICT_EXPECTED_CALL(on_transport_challenge_callback(TEST_DATA_VALUE, TEST_DATA_LENGTH, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        result = g_challenge_cb(TEST_BUFFER_HANDLE_VALUE, g_challenge_context);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
        my_gballoc_free(result);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_044: [ If any failure is encountered on_sasl_tpm_challenge_cb shall return NULL. ] */
    TEST_FUNCTION(prov_transport_common_amqp_challenge_cb_fail)
    {
        char* result;
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).SetReturn(TEST_DATA_VALUE);
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).SetReturn(TEST_DATA_LENGTH);
        STRICT_EXPECTED_CALL(on_transport_challenge_callback(TEST_DATA_VALUE, TEST_DATA_LENGTH, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL);

        //act
        result = g_challenge_cb(TEST_BUFFER_HANDLE_VALUE, g_challenge_context);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_046: [ If handle is NULL, prov_transport_common_amqp_dowork shall do nothing. ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_handle_NULL_fail)
    {
        //arrange

        //act
        prov_transport_common_amqp_dowork(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_047: [ If the amqp_state is AMQP_STATE_DISCONNECTED prov_transport_common_amqp_dowork shall attempt to connect the amqp connections and links. ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_connected_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        umock_c_reset_all_calls();

        //arrange
        setup_create_connection_mocks(false);

        //act
        prov_transport_common_amqp_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_047: [ If the amqp_state is AMQP_STATE_DISCONNECTED prov_transport_common_amqp_dowork shall attempt to connect the amqp connections and links. ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_x509_connected_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        umock_c_reset_all_calls();
        g_use_x509 = true;

        //arrange
        setup_create_connection_mocks(true);

        //act
        prov_transport_common_amqp_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_051: [ Once connected prov_transport_common_amqp_dowork shall call uamqp connection dowork function ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_052: [ Once the uamqp reciever and sender link are connected the amqp_state shall be set to AMQP_STATE_CONNECTED ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_idle_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG));

        //act
        prov_transport_common_amqp_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_050: [ The reciever and sender endpoints addresses shall be constructed in the following manner: amqps://[hostname]/[scope_id]/registrations/[registration_id] ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_053: [ When then transport_state is set to TRANSPORT_CLIENT_STATE_REG_SEND, prov_transport_common_amqp_dowork shall send a AMQP_REGISTER_ME message ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_register_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);

        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG));
        setup_send_amqp_message_mocks(false);

        //act
        prov_transport_common_amqp_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_050: [ The reciever and sender endpoints addresses shall be constructed in the following manner: amqps://[hostname]/[scope_id]/registrations/[registration_id] ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_053: [ When then transport_state is set to TRANSPORT_CLIENT_STATE_REG_SEND, prov_transport_common_amqp_dowork shall send a AMQP_REGISTER_ME message ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_054: [ Upon successful sending of a TRANSPORT_CLIENT_STATE_REG_SEND message, prov_transport_common_amqp_dowork shall set the transport_state to TRANSPORT_CLIENT_STATE_REG_SENT ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_register_recv_succeed)
    {
        AMQP_VALUE result;
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        setup_on_message_recv_callback_mocks();

        //act
        result = g_on_msg_recv(msg_recv_callback_context, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_amqp_dowork_register_recv_assigned_succeed)
    {
        AMQP_VALUE result;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED;
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        setup_on_message_recv_callback_mocks();

        //act
        result = g_on_msg_recv(msg_recv_callback_context, TEST_MESSAGE_HANDLE);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_055: [ When then transport_state is set to TRANSPORT_CLIENT_STATE_STATUS_SEND, prov_transport_common_amqp_dowork shall send a AMQP_OPERATION_STATUS message ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_056: [ Upon successful sending of a AMQP_OPERATION_STATUS message, prov_transport_common_amqp_dowork shall set the transport_state to TRANSPORT_CLIENT_STATE_STATUS_SENT ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_send_status_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);
        prov_transport_common_amqp_dowork(handle);
        (void)g_on_msg_recv(msg_recv_callback_context, TEST_MESSAGE_HANDLE);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        prov_transport_common_amqp_dowork(handle);
        (void)prov_transport_common_amqp_get_operation_status(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG));
        setup_send_amqp_message_mocks(true);

        //act
        prov_transport_common_amqp_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_amqp_dowork_device_assigned_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);
        prov_transport_common_amqp_dowork(handle);
        (void)g_on_msg_recv(msg_recv_callback_context, TEST_MESSAGE_HANDLE);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED;
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_json_parse(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_register_data_cb(PROV_DEVICE_TRANSPORT_RESULT_OK, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        prov_transport_common_amqp_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_049: [ If any error is encountered prov_transport_common_amqp_dowork shall set the amqp_state to AMQP_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_register_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        (void)prov_transport_common_amqp_register_device(handle, on_transport_challenge_callback, NULL, on_transport_json_parse, NULL);
        prov_transport_common_amqp_dowork(handle);
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG));
        setup_send_amqp_message_mocks(false);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 0, 8, 9, 10, 12 };

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            {
                continue;
            }

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "prov_transport_common_amqp_dowork failure in test %zu/%zu", index, count);

            //act
            prov_transport_common_amqp_dowork(handle);

            //assert
        }

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_049: [ If any error is encountered prov_transport_common_amqp_dowork shall set the amqp_state to AMQP_STATE_ERROR and the transport_state to TRANSPORT_CLIENT_STATE_ERROR. ] */
    TEST_FUNCTION(prov_transport_common_amqp_dowork_disconnected_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_create_connection_mocks(false);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 4, 6, 7, 8, 12, 16, 17, 18, 19, 24, 27, 28, 29 };

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            {
                continue;
            }

            (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            prov_transport_common_amqp_dowork(handle);
            prov_transport_common_amqp_dowork(handle);

            //assert
            (void)prov_transport_common_amqp_close(handle);
        }

        //cleanup
        prov_transport_common_amqp_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_052: [ Once the uamqp reciever and sender link are connected the amqp_state shall be set to AMQP_STATE_CONNECTED ] */
    TEST_FUNCTION(prov_transport_common_amqp_msg_recv_state_change_open_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_amqp_msg_recv_state_change_error_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(connection_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_register_data_cb(PROV_DEVICE_TRANSPORT_RESULT_ERROR, NULL, NULL, NULL, NULL));

        //act
        g_msg_rcvr_state_changed(g_msg_rcvr_state_changed_ctx, MESSAGE_RECEIVER_STATE_ERROR, MESSAGE_RECEIVER_STATE_OPENING);
        prov_transport_common_amqp_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_amqp_msg_recv_state_change_user_ctx_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        g_msg_rcvr_state_changed(NULL, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_OPENING);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_052: [ Once the uamqp reciever and sender link are connected the amqp_state shall be set to AMQP_STATE_CONNECTED ] */
    TEST_FUNCTION(prov_transport_common_amqp_msg_sender_state_change_open_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_amqp_msg_sender_state_change_error_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        g_msg_sndr_state_changed(g_msg_sndr_state_changed_ctx, MESSAGE_SENDER_STATE_ERROR, MESSAGE_SENDER_STATE_OPENING);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_amqp_msg_sender_state_change_user_ctx_NULL_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        (void)prov_transport_common_amqp_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL);
        prov_transport_common_amqp_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        g_msg_sndr_state_changed(NULL, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_OPENING);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_close(handle);
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_023: [ If handle is NULL, prov_transport_common_amqp_set_trace shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_trace_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_transport_common_amqp_set_trace(NULL, true);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_024: [ prov_transport_common_amqp_set_trace shall set the log_trace variable to trace_on. ]*/
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_025: [ On success prov_transport_common_amqp_set_trace shall return a zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_trace_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_set_trace(handle, true);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_026: [ If handle or certificate is NULL, prov_transport_common_amqp_x509_cert shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_x509_cert_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_transport_common_amqp_x509_cert(NULL, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_027: [ prov_transport_common_amqp_x509_cert shall copy the certificate and private_key values. ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_029: [ On success prov_transport_common_amqp_x509_cert shall return a zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_x509_cert_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_X509_CERT_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_PRIVATE_KEY_VALUE));

        //act
        int result = prov_transport_common_amqp_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_028: [ On any failure prov_transport_common_amqp_x509_cert, shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_x509_cert_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_X509_CERT_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_PRIVATE_KEY_VALUE));

        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "prov_transport_common_amqp_dowork failure in test %zu/%zu", index, count);

            //act
            int result = prov_transport_common_amqp_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

            //assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, tmp_msg);
        }

        //cleanup
        prov_transport_common_amqp_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_030: [ If handle or certificate is NULL, prov_transport_common_amqp_set_trusted_cert shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_trusted_cert_handle_NULL)
    {
        //arrange

        //act
        int result = prov_transport_common_amqp_set_trusted_cert(NULL, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_030: [ If handle or certificate is NULL, prov_transport_common_amqp_set_trusted_cert shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_trusted_cert_cert_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_set_trusted_cert(handle, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_031: [ prov_transport_common_amqp_set_trusted_cert shall copy the certificate value. ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_033: [ On success prov_transport_common_amqp_set_trusted_cert shall return a zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_trusted_cert_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT_VALUE));

        //act
        int result = prov_transport_common_amqp_set_trusted_cert(handle, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_032: [ On any failure prov_transport_common_amqp_set_trusted_cert, shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_trusted_cert_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT_VALUE)).SetReturn(__LINE__);

        //act
        int result = prov_transport_common_amqp_set_trusted_cert(handle, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_034: [ If handle or proxy_options is NULL, prov_transport_common_amqp_set_proxy shall return a non-zero value. ]*/
    TEST_FUNCTION(prov_transport_common_amqp_set_proxy_handle_NULL_fail)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;

        //act
        int result = prov_transport_common_amqp_set_proxy(NULL, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_034: [ If handle or proxy_options is NULL, prov_transport_common_amqp_set_proxy shall return a non-zero value. ]*/
    TEST_FUNCTION(prov_transport_common_amqp_set_proxy_proxy_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_amqp_set_proxy(handle, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_036: [ If HTTP_PROXY_OPTIONS password is not NULL and password is NULL, prov_transport_common_amqp_set_proxy shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_proxy_no_username_fail)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.password = TEST_PASSWORD_VALUE;

        //act
        int result = prov_transport_common_amqp_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_038: [ prov_transport_common_amqp_set_proxy shall copy the host_addess, username, or password variables ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_040: [ On success prov_transport_common_amqp_set_proxy shall return a zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_proxy_2_succeed)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.username = TEST_USERNAME_VALUE;
        proxy_options.password = TEST_PASSWORD_VALUE;

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.host_address));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.username));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.password));

        //act
        int result = prov_transport_common_amqp_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_035: [ If HTTP_PROXY_OPTIONS host_address is NULL, prov_transport_common_amqp_set_proxy shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_proxy_hostname_NULL_fail)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        proxy_options.port = 443;

        //act
        int result = prov_transport_common_amqp_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_039: [ On any failure prov_transport_common_amqp_set_proxy, shall return a non-zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_proxy_fail)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.username = TEST_USERNAME_VALUE;
        proxy_options.password = TEST_PASSWORD_VALUE;

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.host_address));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.username));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.password));

        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "prov_transport_common_amqp_set_proxy failure in test %zu/%zu", index, count);

            //act
            int result = prov_transport_common_amqp_set_proxy(handle, &proxy_options);

            //assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, tmp_msg);
        }

        //cleanup
        prov_transport_common_amqp_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_038: [ prov_transport_common_amqp_set_proxy shall copy the host_addess, username, or password variables ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_040: [ On success prov_transport_common_amqp_set_proxy shall return a zero value. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_proxy_succeed)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);
        umock_c_reset_all_calls();

        //arrange
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.host_address));

        //act
        int result = prov_transport_common_amqp_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_038: [ prov_transport_common_amqp_set_proxy shall copy the host_addess, username, or password variables ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_040: [ On success prov_transport_common_amqp_set_proxy shall return a zero value. ] */
    /* Tests_PROV_TRANSPORT_AMQP_COMMON_07_037: [ If any of the host_addess, username, or password variables are non-NULL, prov_transport_common_amqp_set_proxy shall free the memory. ] */
    TEST_FUNCTION(prov_transport_common_amqp_set_proxy_twice_succeed)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_amqp_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_transport_io);

        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.username = TEST_USERNAME_VALUE;
        proxy_options.password = TEST_PASSWORD_VALUE;
        (void)prov_transport_common_amqp_set_proxy(handle, &proxy_options);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.host_address));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.username));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.password));

        //act
        int result = prov_transport_common_amqp_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_amqp_destroy(handle);
    }

    END_TEST_SUITE(prov_transport_amqp_common_ut)
