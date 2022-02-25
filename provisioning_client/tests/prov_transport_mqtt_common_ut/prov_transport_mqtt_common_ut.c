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
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_prov_client/prov_transport.h"
#include "azure_umqtt_c/mqtt_client.h"
#undef ENABLE_MOCKS

#include "azure_prov_client/internal/prov_transport_mqtt_common.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/buffer_.h"
#include "umock_c/umock_c_prod.h"

MOCKABLE_FUNCTION(, void, on_transport_register_data_cb, PROV_DEVICE_TRANSPORT_RESULT, transport_result, BUFFER_HANDLE, iothub_key, const char*, assigned_hub, const char*, device_id, void*, user_ctx);
MOCKABLE_FUNCTION(, void, on_transport_status_cb, PROV_DEVICE_TRANSPORT_STATUS, transport_status, uint32_t, retry_interval, void*, user_ctx);
MOCKABLE_FUNCTION(, char*, on_transport_challenge_callback, const unsigned char*, nonce, size_t, nonce_len, const char*, key_name, void*, user_ctx);
MOCKABLE_FUNCTION(, XIO_HANDLE, on_mqtt_transport_io, const char*, fully_qualified_name, const HTTP_PROXY_OPTIONS*, proxy_info);
MOCKABLE_FUNCTION(, char*, on_transport_create_json_payload, const char*, ek_value, const char*, srk_value, void*, user_ctx);
MOCKABLE_FUNCTION(, PROV_JSON_INFO*, on_transport_json_parse, const char*, json_document, void*, user_ctx);
MOCKABLE_FUNCTION(, void, on_transport_error, PROV_DEVICE_TRANSPORT_ERROR, transport_error, void*, user_context);

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
#define TEST_MQTT_MESSAGE   (MQTT_MESSAGE_HANDLE)0x11111113
#define TEST_XIO_HANDLE     (XIO_HANDLE)0x11111114
#define TEST_MESSAGE_HANDLE (MESSAGE_HANDLE)0x11111115
#define TEST_DATA_VALUE     (unsigned char*)0x11111117
#define TEST_DATA_LENGTH    (size_t)128
#define TEST_BUFFER_HANDLE_VALUE     (BUFFER_HANDLE)0x1111111B

#define TEST_UNASSIGNED     (void*)0x11111118
#define TEST_ASSIGNED       (void*)0x11111119
#define TEST_ASSIGNING      (void*)0x1111111A
#define TEST_OPTION_VALUE   (void*)0x1111111B

static const MQTT_CLIENT_HANDLE TEST_MQTT_CLIENT_HANDLE = (MQTT_CLIENT_HANDLE)0x1111111B;
static const char* TEST_OPERATION_ID_VALUE = "operation_id";
static char* TEST_STRING_VALUE = "Test_String_Value";

static const char* TEST_URI_VALUE = "dps_uri.azure.com";
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
static const char* TEST_XIO_OPTION_NAME = "test_option";
static const char* TEST_CUSTOM_DATA = "custom data";
static const char* TEST_500_REGISTRATION_VALUE = "$dps/registrations/res/500/?$rid=1";
static const char* TEST_202_REGISTRATION_VALUE = "$dps/registrations/res/202/?$rid=1&retry-after=8";
static const char* TEST_429_REGISTRATION_VALUE = "$dps/registrations/res/429/?$rid=1";

static ON_MQTT_MESSAGE_RECV_CALLBACK g_on_msg_recv;
static void* g_msg_recv_callback_context;
static ON_MQTT_OPERATION_CALLBACK g_operation_cb;
static ON_MQTT_ERROR_CALLBACK g_on_error_cb;
static void* g_on_error_ctx;
static bool g_use_x509;
static APP_PAYLOAD g_app_msg;


static PROV_DEVICE_TRANSPORT_STATUS g_target_transport_status;

TEST_DEFINE_ENUM_TYPE(MQTT_CLIENT_EVENT_ERROR, MQTT_CLIENT_EVENT_ERROR_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MQTT_CLIENT_EVENT_ERROR, MQTT_CLIENT_EVENT_ERROR_VALUES);

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

static MQTT_CLIENT_HANDLE my_mqtt_client_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* opCallbackCtx, ON_MQTT_ERROR_CALLBACK onErrorCallBack, void* errorCBCtx)
{
    g_on_msg_recv = msgRecv;
    g_msg_recv_callback_context = opCallbackCtx;
    g_operation_cb = opCallback;
    g_on_error_cb = onErrorCallBack;
    g_on_error_ctx = errorCBCtx;
    return (MQTT_CLIENT_HANDLE)my_gballoc_malloc(1);
}

static void my_mqtt_client_deinit(MQTT_CLIENT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static MQTT_MESSAGE_HANDLE my_mqttmessage_create(uint16_t packetId, const char* topicName, QOS_VALUE qosValue, const uint8_t* appMsg, size_t appMsgLength)
{
    (void)packetId;
    (void)topicName;
    (void)qosValue;
    (void)appMsg;
    (void)appMsgLength;
    return (MQTT_MESSAGE_HANDLE)my_gballoc_malloc(1);
}

static MQTT_MESSAGE_HANDLE my_mqttmessage_create_in_place(uint16_t packetId, const char* topicName, QOS_VALUE qosValue, const uint8_t* appMsg, size_t appMsgLength)
{
    (void)packetId;
    (void)topicName;
    (void)qosValue;
    (void)appMsg;
    (void)appMsgLength;
    return (MQTT_MESSAGE_HANDLE)my_gballoc_malloc(1);
}

static void my_mqttmessage_destroy(MQTT_MESSAGE_HANDLE handle)
{
    my_gballoc_free(handle);
}

static const APP_PAYLOAD* my_mqttmessage_getApplicationMsg(MQTT_MESSAGE_HANDLE handle)
{
    (void)handle;
    g_app_msg.length = strlen(TEST_JSON_REPLY);
    g_app_msg.message = (unsigned char*)TEST_JSON_REPLY;
    return &g_app_msg;
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

static char* my_on_transport_create_json_payload(const char* ek_value, const char* srk_value, void* user_ctx)
{
    (void)ek_value;
    (void)srk_value;
    (void)user_ctx;
    char* result;
    size_t src_len = strlen(TEST_CUSTOM_DATA);
    result = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(result, TEST_CUSTOM_DATA);
    return result;
}

static XIO_HANDLE my_on_mqtt_transport_io(const char* fqdn, const HTTP_PROXY_OPTIONS* proxy_info)
{
    (void)fqdn;
    (void)proxy_info;

    return (XIO_HANDLE)my_gballoc_malloc(1);
}

static void my_xio_destroy(XIO_HANDLE xio)
{
    my_gballoc_free(xio);
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

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(prov_transport_mqtt_common_ut)

    TEST_SUITE_INITIALIZE(suite_init)
    {
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);
        (void)umocktypes_bool_register_types();
        (void)umocktypes_stdint_register_types();

        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT);
        REGISTER_TYPE(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS);
        REGISTER_TYPE(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE);

        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_DEVICE_TRANSPORT_STATUS_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(PROV_TRANSPORT_CHALLENGE_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MQTT_CLIENT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_MESSAGE_RECV_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_OPERATION_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_ERROR_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(QOS_VALUE, unsigned int);
        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(MQTT_MESSAGE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(ON_MQTT_DISCONNECTED_CALLBACK, void*);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_clone, my_BUFFER_clone);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_clone, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

        REGISTER_GLOBAL_MOCK_HOOK(on_mqtt_transport_io, my_on_mqtt_transport_io);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_mqtt_transport_io, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_init, my_mqtt_client_init);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_init, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(mqtt_client_deinit, my_mqtt_client_deinit);

        REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_connect, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_connect, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_disconnect, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_disconnect, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_subscribe, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_subscribe, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_unsubscribe, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_unsubscribe, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(mqtt_client_publish, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqtt_client_publish, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(mqttmessage_create, my_mqttmessage_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(mqttmessage_create_in_place, my_mqttmessage_create_in_place);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_create_in_place, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(mqttmessage_destroy, my_mqttmessage_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(mqttmessage_getApplicationMsg, my_mqttmessage_getApplicationMsg);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mqttmessage_getApplicationMsg, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(on_transport_challenge_callback, my_on_transport_challenge_callback);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_challenge_callback, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(on_transport_json_parse, my_on_transport_json_parse);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_json_parse, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(on_transport_create_json_payload, my_on_transport_create_json_payload);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_transport_create_json_payload, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(xio_setoption, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_setoption, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(xio_destroy, my_xio_destroy);
    }

    TEST_SUITE_CLEANUP(suite_cleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("Could not acquire test serialization mutex.");
        }
        umock_c_reset_all_calls();
        g_on_msg_recv = NULL;
        g_msg_recv_callback_context = NULL;
        g_operation_cb = NULL;
        g_on_error_cb = NULL;
        g_use_x509 = false;
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    static void setup_prov_transport_common_mqtt_create_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_URI_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_DPS_API_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SCOPE_ID_VALUE));
        STRICT_EXPECTED_CALL(mqtt_client_init(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    static void setup_create_connection_mocks(bool use_x509)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(on_mqtt_transport_io(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (use_x509)
        {
            STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(mqtt_client_set_trace(IGNORED_PTR_ARG, false, false));
        STRICT_EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_create_key_connection_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(on_mqtt_transport_io(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_challenge_callback(NULL, 0, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqtt_client_set_trace(IGNORED_PTR_ARG, false, false));
        STRICT_EXPECTED_CALL(mqtt_client_connect(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_on_message_recv_callback_mocks(void)
    {
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_json_parse(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_register_data_cb(PROV_DEVICE_TRANSPORT_RESULT_OK, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_send_mqtt_message_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(on_transport_create_json_payload(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqttmessage_create_in_place(IGNORED_NUM_ARG, IGNORED_PTR_ARG, DELIVER_AT_MOST_ONCE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mqtt_client_publish(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqttmessage_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
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

    TEST_FUNCTION(prov_transport_common_mqtt_create_uri_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(NULL, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_create_scope_id_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, NULL, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_create_api_version_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, NULL, on_mqtt_transport_io, on_transport_error, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_create_transport_io_NULL_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, NULL, on_transport_error, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_create_fail)
    {
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_prov_transport_common_mqtt_create_mocks();

        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "prov_transport_common_mqtt_create failure in test %zu/%zu", index, count);

            //act
            PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);

            //assert
            ASSERT_IS_NULL(handle, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_transport_common_mqtt_create_tpm_fail)
    {
        //arrange

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_TPM, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_create_succeed)
    {
        //arrange
        setup_prov_transport_common_mqtt_create_mocks();

        //act
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);

        //assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_destroy_handle_NULL_fail)
    {
        //arrange

        //act
        prov_transport_common_mqtt_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_destroy_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqtt_client_deinit(IGNORED_PTR_ARG));
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
        prov_transport_common_mqtt_destroy(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_transport_common_mqtt_open(NULL, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_registration_id_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, NULL, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_data_callback_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, NULL, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_status_callback_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, NULL, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_ek_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, TEST_BUFFER_VALUE, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_srk_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, TEST_BUFFER_VALUE, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_REGISTRATION_ID_VALUE));

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_REGISTRATION_ID_VALUE));

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "prov_transport_common_mqtt_open failure in test %zu/%zu", index, count);
        }

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_x509_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_REGISTRATION_ID_VALUE));

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_register_device_x509_challenge_NULL_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_REGISTRATION_ID_VALUE));

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, NULL, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_open_key_challenge_NULL_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_SYMM_KEY, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, NULL, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_mqtt_close_handle_NULL)
    {
        //arrange

        //act
        int result = prov_transport_common_mqtt_close(NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_mqtt_close_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqtt_client_disconnect(IGNORED_PTR_ARG, NULL, NULL));
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(xio_destroy(IGNORED_PTR_ARG));

        //act
        int result = prov_transport_common_mqtt_close(handle);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_register_device_handle_NULL_succeed)
    {
        //arrange

        //act
        int result = prov_transport_common_mqtt_register_device(NULL, on_transport_json_parse, on_transport_create_json_payload, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_register_device_cb_NULL_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, NULL, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_register_device_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_register_device_twice_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_get_operation_status_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_transport_common_mqtt_get_operation_status(NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_get_operation_status_operation_id_zero_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_get_operation_status(handle);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_get_operation_status_success)
    {
        int result;
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);

        prov_transport_common_mqtt_dowork(handle);
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);

        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        prov_transport_common_mqtt_dowork(handle);
        umock_c_reset_all_calls();

        //arrange

        //act
        result = prov_transport_common_mqtt_get_operation_status(handle);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_handle_NULL_fail)
    {
        //arrange

        //act
        prov_transport_common_mqtt_dowork(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_x509_connected_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        umock_c_reset_all_calls();
        g_use_x509 = true;

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_create_connection_mocks(true);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 3, 7, 8 };

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            {
                continue;
            }

            (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            //act
            prov_transport_common_mqtt_dowork(handle);
            prov_transport_common_mqtt_dowork(handle);

            //assert
            prov_transport_common_mqtt_close(handle);
        }

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_x509_connected_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        umock_c_reset_all_calls();
        g_use_x509 = true;

        //arrange
        setup_create_connection_mocks(true);

        //act
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_symm_key_connected_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_SYMM_KEY, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        umock_c_reset_all_calls();
        g_use_x509 = true;

        //arrange
        setup_create_key_connection_mocks();

        //act
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_idle_succeed)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));

        //act
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_subscribe_succeed)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);

        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqtt_client_subscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, IGNORED_NUM_ARG, IGNORED_PTR_ARG));

        //act
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_subscribe_fail)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);

        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqtt_client_subscribe(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_register_data_cb(PROV_DEVICE_TRANSPORT_RESULT_ERROR, NULL, NULL, NULL, IGNORED_PTR_ARG));

        //act
        prov_transport_common_mqtt_dowork(handle);
        prov_transport_common_mqtt_dowork(handle);


        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_register_recv_succeed)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqttmessage_getTopicName(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        //act
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_register_recv_app_msg_NULL_fail)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqttmessage_getTopicName(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(IGNORED_PTR_ARG)).SetReturn(NULL);

        //act
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_register_recv_malloc_NULL_fail)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqttmessage_getTopicName(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

        //act
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_register_recv_transient_error_succeed)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqttmessage_getTopicName(IGNORED_PTR_ARG)).SetReturn(TEST_500_REGISTRATION_VALUE);
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_TRANSIENT, IGNORED_NUM_ARG, IGNORED_PTR_ARG));

        //act
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_register_recv_retry_after_succeed)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqttmessage_getTopicName(IGNORED_PTR_ARG)).SetReturn(TEST_202_REGISTRATION_VALUE);
        STRICT_EXPECTED_CALL(mqttmessage_getApplicationMsg(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_json_parse(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING, 8, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_register_recv_transient_throttle_error_succeed)
    {
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqttmessage_getTopicName(IGNORED_PTR_ARG)).SetReturn(TEST_429_REGISTRATION_VALUE);
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_TRANSIENT, IGNORED_NUM_ARG, IGNORED_PTR_ARG));

        //act
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_register_recv_assigned_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle;
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED;
        umock_c_reset_all_calls();

        //arrange
        setup_on_message_recv_callback_mocks();

        //act
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_send_status_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle;
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        prov_transport_common_mqtt_dowork(handle);
        (void)prov_transport_common_mqtt_get_operation_status(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        setup_send_mqtt_message_mocks();

        //act
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_dowork_send_status_transient_error_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle;
        CONNECT_ACK connack = { true, CONNECTION_ACCEPTED };
        QOS_VALUE QosValue[] = { DELIVER_AT_LEAST_ONCE };
        SUBSCRIBE_ACK suback;
        suback.packetId = 1234;
        suback.qosCount = 1;
        suback.qosReturn = QosValue;

        handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        (void)prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);
        (void)prov_transport_common_mqtt_open(handle, TEST_REGISTRATION_ID_VALUE, NULL, NULL, on_transport_register_data_cb, NULL, on_transport_status_cb, NULL, on_transport_challenge_callback, NULL);
        (void)prov_transport_common_mqtt_register_device(handle, on_transport_json_parse, on_transport_create_json_payload, NULL);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_CONNACK, &connack, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_operation_cb(TEST_MQTT_CLIENT_HANDLE, MQTT_CLIENT_ON_SUBSCRIBE_ACK, &suback, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);
        g_target_transport_status = PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING;
        prov_transport_common_mqtt_dowork(handle);
        (void)prov_transport_common_mqtt_get_operation_status(handle);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqttmessage_getTopicName(IGNORED_PTR_ARG)).SetReturn(TEST_500_REGISTRATION_VALUE);
        STRICT_EXPECTED_CALL(mqtt_client_dowork(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(on_transport_status_cb(PROV_DEVICE_TRANSPORT_STATUS_TRANSIENT, IGNORED_NUM_ARG, IGNORED_PTR_ARG));

        //act
        g_on_msg_recv(TEST_MQTT_MESSAGE, g_msg_recv_callback_context);
        prov_transport_common_mqtt_dowork(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_close(handle);
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_trace_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_transport_common_mqtt_set_trace(NULL, true);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_trace_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mqtt_client_set_trace(IGNORED_PTR_ARG, true, false));

        //act
        int result = prov_transport_common_mqtt_set_trace(handle, true);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_x509_cert_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_transport_common_mqtt_x509_cert(NULL, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_x509_cert_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_X509_CERT_VALUE));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_PRIVATE_KEY_VALUE));

        //act
        int result = prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_x509_cert_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
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

            //act
            int result = prov_transport_common_mqtt_x509_cert(handle, TEST_X509_CERT_VALUE, TEST_PRIVATE_KEY_VALUE);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "prov_transport_common_mqtt_dowork failure in test %zu/%zu", index, count);
        }

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_trusted_cert_handle_NULL)
    {
        //arrange

        //act
        int result = prov_transport_common_mqtt_set_trusted_cert(NULL, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_trusted_cert_cert_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_set_trusted_cert(handle, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_trusted_cert_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT_VALUE));

        //act
        int result = prov_transport_common_mqtt_set_trusted_cert(handle, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_trusted_cert_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CERT_VALUE)).SetReturn(__LINE__);

        //act
        int result = prov_transport_common_mqtt_set_trusted_cert(handle, TEST_CERT_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_proxy_handle_NULL_fail)
    {
        //arrange
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;

        //act
        int result = prov_transport_common_mqtt_set_proxy(NULL, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_proxy_proxy_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_set_proxy(handle, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_proxy_no_username_fail)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.password = TEST_PASSWORD_VALUE;

        //act
        int result = prov_transport_common_mqtt_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_proxy_2_succeed)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
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
        int result = prov_transport_common_mqtt_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_proxy_hostname_NULL_fail)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        proxy_options.port = 443;

        //act
        int result = prov_transport_common_mqtt_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_proxy_fail)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
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

            //act
            int result = prov_transport_common_mqtt_set_proxy(handle, &proxy_options);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "prov_transport_common_mqtt_set_proxy failure in test %zu/%zu", index, count);
        }

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_proxy_succeed)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange
        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.host_address));

        //act
        int result = prov_transport_common_mqtt_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_proxy_twice_succeed)
    {
        HTTP_PROXY_OPTIONS proxy_options = { 0 };
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);

        proxy_options.host_address = TEST_HOST_ADDRESS_VALUE;
        proxy_options.port = 443;
        proxy_options.username = TEST_USERNAME_VALUE;
        proxy_options.password = TEST_PASSWORD_VALUE;
        (void)prov_transport_common_mqtt_set_proxy(handle, &proxy_options);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.host_address));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.username));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, proxy_options.password));

        //act
        int result = prov_transport_common_mqtt_set_proxy(handle, &proxy_options);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_option_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_transport_common_mqtt_set_option(NULL, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_option_option_NULL_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_transport_common_mqtt_set_option(handle, NULL, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_option_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);

        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(on_mqtt_transport_io(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE));

        //act
        int result = prov_transport_common_mqtt_set_option(handle, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_option_twice_succeed)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);
        int result = prov_transport_common_mqtt_set_option(handle, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(xio_setoption(IGNORED_PTR_ARG, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE));

        //act
        result = prov_transport_common_mqtt_set_option(handle, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    TEST_FUNCTION(prov_transport_common_mqtt_set_option_fail)
    {
        PROV_DEVICE_TRANSPORT_HANDLE handle = prov_transport_common_mqtt_create(TEST_URI_VALUE, TRANSPORT_HSM_TYPE_X509, TEST_SCOPE_ID_VALUE, TEST_DPS_API_VALUE, on_mqtt_transport_io, on_transport_error, NULL);

        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(on_mqtt_transport_io(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL);

        //act
        int result = prov_transport_common_mqtt_set_option(handle, TEST_XIO_OPTION_NAME, TEST_OPTION_VALUE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_transport_common_mqtt_destroy(handle);
    }

    END_TEST_SUITE(prov_transport_mqtt_common_ut)
