// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdbool>
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

void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/envvariable.h"

#include "iothub_client_version.h"
#include "iothub_message.h"
#include "internal/iothub_client_authorization.h"
#include "internal/iothub_client_diagnostic.h"

#ifdef USE_EDGE_MODULES
#include "internal/iothub_client_edge.h"
#include "azure_prov_client/iothub_security_factory.h"
#endif

#include "iothub_transport_ll.h"
#include "iothub_client_core_common.h"
#undef ENABLE_MOCKS

#include "internal/iothub_transport_ll_private.h"

#include "iothub_client_core_ll.h"
#include "internal/iothub_client_private.h"
#include "iothub_client_options.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"

#ifndef DONT_USE_UPLOADTOBLOB
#include "internal/iothub_client_ll_uploadtoblob.h"
#endif

MOCKABLE_FUNCTION(, void, test_event_confirmation_callback, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, userContextCallback);
MOCKABLE_FUNCTION(, IOTHUBMESSAGE_DISPOSITION_RESULT, test_message_callback_async, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, iothub_reported_state_callback, int, status_code, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, iothub_device_twin_callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, deviceMethodCallback, const char*, method_name, const unsigned char*, payload, size_t, size, unsigned char**, response, size_t*, resp_size, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, iothub_client_inbound_device_method_callback, const char*, method_name, const unsigned char*, payload, size_t, size, METHOD_HANDLE, method_id, void*, userContextCallback);

MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_SendMessageDisposition, IOTHUB_DEVICE_HANDLE, handle,  IOTHUB_MESSAGE_HANDLE, message, IOTHUBMESSAGE_DISPOSITION_RESULT, disposition);
MOCKABLE_FUNCTION(, STRING_HANDLE, FAKE_IoTHubTransport_GetHostname, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_SetOption, TRANSPORT_LL_HANDLE, handle, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, TRANSPORT_LL_HANDLE, FAKE_IoTHubTransport_Create, const IOTHUBTRANSPORT_CONFIG*, config, TRANSPORT_CALLBACKS_INFO*, cb_info, void*, ctx);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Destroy, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_HANDLE, FAKE_IoTHubTransport_Register, TRANSPORT_LL_HANDLE, handle, const IOTHUB_DEVICE_CONFIG*, device, PDLIST_ENTRY, waitingToSend);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unregister, IOTHUB_DEVICE_HANDLE, deviceHandle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_DoWork, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_SetRetryPolicy, TRANSPORT_LL_HANDLE, handle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_GetSendStatus, IOTHUB_DEVICE_HANDLE, handle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Common_Subscribe_InputQueue, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Common_Unsubscribe_InputQueue, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe_DeviceTwin, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe_DeviceTwin, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, FAKE_IoTHubTransport_GetTwinAsync, IOTHUB_DEVICE_HANDLE, handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, completionCallback, void*, callbackContext);
MOCKABLE_FUNCTION(, const char*, FAKE_IoTHubMessage_GetMessageId, IOTHUB_MESSAGE_HANDLE, message);
MOCKABLE_FUNCTION(, IOTHUB_PROCESS_ITEM_RESULT, FAKE_IoTHubTransport_ProcessItem, TRANSPORT_LL_HANDLE, handle, IOTHUB_IDENTITY_TYPE, item_type, IOTHUB_IDENTITY_INFO*, iothub_item);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_Subscribe_DeviceMethod, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IoTHubTransport_Unsubscribe_DeviceMethod, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, connectionStatusCallback, IOTHUB_CLIENT_CONNECTION_STATUS, result3, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, userContextCallback);
MOCKABLE_FUNCTION(, IOTHUBMESSAGE_DISPOSITION_RESULT, messageCallback, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback);
MOCKABLE_FUNCTION(, bool, messageCallbackEx, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, eventConfirmationCallback, IOTHUB_CLIENT_CONFIRMATION_RESULT, result2, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_DeviceMethod_Response, IOTHUB_DEVICE_HANDLE, handle, METHOD_HANDLE, methodId, const unsigned char*, response, size_t, resp_size, int, status_response);
MOCKABLE_FUNCTION(, int, FAKE_IotHubTransport_Subscribe_InputQueue, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, FAKE_IotHubTransport_Unsubscribe_InputQueue, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_SetCallbackContext, TRANSPORT_LL_HANDLE, handle, void*, ctx);
MOCKABLE_FUNCTION(, int, FAKE_IoTHubTransport_GetSupportedPlatformInfo, TRANSPORT_LL_HANDLE, handle, PLATFORM_INFO_OPTION*, info);
MOCKABLE_FUNCTION(, bool, messageInputCallbackEx, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback);

MOCKABLE_FUNCTION(, bool, Transport_MessageCallbackFromInput, IOTHUB_MESSAGE_HANDLE, message, void*, ctx);
MOCKABLE_FUNCTION(, bool, Transport_MessageCallback, IOTHUB_MESSAGE_HANDLE, message, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_ConnectionStatusCallBack, IOTHUB_CLIENT_CONNECTION_STATUS, status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_SendComplete_Callback, PDLIST_ENTRY, completed, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, ctx);
MOCKABLE_FUNCTION(, const char*, Transport_GetOption_Product_Info_Callback, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_ReportedStateComplete_Callback, uint32_t, item_id, int, status_code, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_RetrievePropertyComplete_Callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, ctx);
MOCKABLE_FUNCTION(, int, Transport_DeviceMethod_Complete_Callback, const char*, method_name, const unsigned char*, payLoad, size_t, size, METHOD_HANDLE, response_id, void*, ctx);
MOCKABLE_FUNCTION(, const char*, Transport_GetOption_Model_Id_Callback, void*, ctx);

static int bool_Compare(bool left, bool right)
{
    return left != right;
}

static void bool_ToString(char* string, size_t bufferSize, bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}

#ifndef __cplusplus
static int _Bool_Compare(_Bool left, _Bool right)
{
    return left != right;
}

static void _Bool_ToString(char* string, size_t bufferSize, _Bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}
#endif
#undef ENABLE_MOCKS

TEST_DEFINE_ENUM_TYPE(IOTHUB_PROCESS_ITEM_RESULT, IOTHUB_PROCESS_ITEM_RESULT_VALUE);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_PROCESS_ITEM_RESULT, IOTHUB_PROCESS_ITEM_RESULT_VALUE);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

static TEST_MUTEX_HANDLE test_serialize_mutex;

bool g_fail_string_construct_sprintf;
bool g_fail_platform_get_platform_info;
bool g_fail_string_concat_with_string;
bool g_fail_string_construct;

static const char* TEST_STRING_VALUE = "Test string value";

#define TEST_DEVICE_ID "theidofTheDevice"
#define TEST_DEVICE_KEY "theKeyoftheDevice"
#define TEST_DEVICE_SAS "theSasOfTheDevice"
#define TEST_IOTHUBNAME "theNameoftheIotHub"
#define TEST_IOTHUBSUFFIX "theSuffixoftheIotHubHostname"
#define TEST_AUTHORIZATIONKEY "theAuthorizationKey"

#define TEST_HOSTNAME_TOKEN "HostName"
#define TEST_HOSTNAME_VALUE "theNameoftheIotHub.theSuffixoftheIotHubHostname"

#define TEST_DEVICEID_TOKEN "DeviceId"
#define TEST_DEVICEKEY_TOKEN "SharedAccessKey"
#define TEST_DEVICESAS_TOKEN "SharedAccessSignature"
#define TEST_PROTOCOL_GATEWAY_HOST_NAME_TOKEN "GatewayHostName"
#define TEST_INVALID_TOKEN1 "InvalidToken1"
#define TEST_INVALID_TOKEN2 "InvalidToken2"

#define TEST_X509 "x509"
#define TEST_MODULE_ID_TOKEN "ModuleId"
#define TEST_PROVISIONING_TOKEN "UseProvisioning"

#define ENVVARIABLE "VALUE"

#define TEST_DEVICEMESSAGE_HANDLE (IOTHUB_MESSAGE_HANDLE)0x52
#define TEST_DEVICEMESSAGE_HANDLE_2 (IOTHUB_MESSAGE_HANDLE)0x53
#define TEST_IOTHUB_CLIENT_CORE_LL_HANDLE    (IOTHUB_CLIENT_CORE_LL_HANDLE)0x4242

#define TEST_STRING_HANDLE (STRING_HANDLE)0x46
#define TEST_STRING_TOKENIZER_HANDLE (STRING_TOKENIZER_HANDLE)0x48

#define TEST_DEVICE_STATUS_CODE        200


#define TEST_TRANSPORT_LL_HANDLE            (TRANSPORT_LL_HANDLE)0x49
#define TEST_IOTHUB_DEVICE_HANDLE           (IOTHUB_DEVICE_HANDLE)0x50
#define TEST_MESSAGE_HANDLE                 (IOTHUB_MESSAGE_HANDLE)0x51
#define TEST_TIME_VALUE                     (time_t)123456

#define TEST_BUFFER_HANDLE                  (BUFFER_HANDLE)0x52
#define TEST_RETRY_POLICY                   IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define TEST_RETRY_TIMEOUT_SECS             60

#define TEST_METHOD_ID                      (METHOD_HANDLE)0x61
#define TEST_IOTHUB_AUTH_HANDLE        (IOTHUB_AUTHORIZATION_HANDLE)0x62

static const char* TEST_PROV_URI = "global.azure-devices-provisioning.net";

static const char* TEST_METHOD_NAME = "method_name";
static const char* TEST_CHAR = "TestChar";
static tickcounter_ms_t g_current_ms = 0;
static const char* TEST_DEVICE_METHOD_RESPONSE = "{device:method, response:true}";

static const char* TEST_OUTPUT_NAME = "TestOutputName";
static const char* TEST_INPUT_NAME = "TestInputName";
static const char* TEST_INPUT_NAME2 = "TestInputName2";
static const char* TEST_INPUT_NAME3 = "TestInputName3";
static const char* TEST_INPUT_NAME_NOTFOUND = "TestInputNameNotFound";
static const char* TEST_MODULE_ID = "TestModuleId";

static const char* TEST_CERTIFICATE = "TestCertificateData";

static const char* TEST_EDGEHUB_CONNECTIONSTRING = "testEdgehubConnString";
static const char* TEST_EDGEHUB_CACERTIFICATEFILE = "testEdgehubCACertFile";
static const char* TEST_SAS_TOKEN_AUTH = "sasToken";
static const char* TEST_VAR_DEVICEID = "testDeviceId";
static const char* TEST_VAR_EDGEHOSTNAME = "testEdgeHost.host";
static const char* TEST_VAR_EDGEGATEWAYHOST = "testEdgeGatewayHost";
static const char* TEST_VAR_MODULEID = "testModuleId";

static SINGLYLINKEDLIST_HANDLE test_singlylinkedlist_handle = (SINGLYLINKEDLIST_HANDLE)0x4243;
static LIST_ITEM_HANDLE find_item_handle = (LIST_ITEM_HANDLE)0x4244;
static LIST_ITEM_HANDLE add_item_handle = (LIST_ITEM_HANDLE)0x4245;

static TRANSPORT_CALLBACKS_INFO g_transport_cb_info;
static void* g_transport_cb_ctx = (void*)0x499922;

static const unsigned char TEST_REPORTED_STATE[] = { 0x01, 0x02, 0x03 };
static const size_t TEST_REPORTED_SIZE = sizeof(TEST_REPORTED_STATE) / sizeof(TEST_REPORTED_STATE[0]);

static const TRANSPORT_PROVIDER* provideFAKE(void);

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_NULL_protocol =
{
    NULL,                   /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    TEST_DEVICE_KEY,        /* const char* deviceKey;                       */
    TEST_IOTHUBNAME,        /* const char* iotHubName;                      */
    TEST_IOTHUBSUFFIX,      /* const char* iotHubSuffix;                    */
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG =
{
    provideFAKE,            /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,         /* const char* deviceId;                        */
    TEST_DEVICE_KEY,        /* const char* deviceKey;                       */
    TEST_DEVICE_SAS,        /* const char* deviceSasToken;                  */
    TEST_IOTHUBNAME,        /* const char* iotHubName;                      */
    TEST_IOTHUBSUFFIX,      /* const char* iotHubSuffix;                    */
};

#define FAKE_TRANSPORT_HANDLE (TRANSPORT_LL_HANDLE)0xDEAD
#define FAKE_TRANSPORT_PROVIDER (IOTHUB_CLIENT_TRANSPORT_PROVIDER)0xFFFF

static const IOTHUB_CLIENT_DEVICE_CONFIG TEST_DEVICE_CONFIG =
{
    provideFAKE,
    FAKE_TRANSPORT_HANDLE,
    TEST_DEVICE_ID,
    TEST_DEVICE_KEY
};

static const IOTHUB_CLIENT_DEVICE_CONFIG TEST_DEVICE_CONFIG_null_protocol =
{
    NULL,
    FAKE_TRANSPORT_HANDLE,
    TEST_DEVICE_ID,
    TEST_DEVICE_KEY
};

static const IOTHUB_CLIENT_DEVICE_CONFIG TEST_DEVICE_CONFIG_null_handle =
{
    provideFAKE,
    NULL,
    TEST_DEVICE_ID,
    TEST_DEVICE_KEY
};

static const IOTHUB_CLIENT_DEVICE_CONFIG TEST_DEVICE_CONFIG_NULL_device_key_NULL_sas_token =
{
    provideFAKE,
    FAKE_TRANSPORT_HANDLE,
    NULL,
    NULL
};

static IOTHUB_AUTHORIZATION_HANDLE my_IoTHubClient_Auth_Create(const char* device_key, const char* device_id, const char* sas_token, const char* module_id)
{
    (void)device_key;
    (void)device_id;
    (void)sas_token;
    (void)module_id;
    return (IOTHUB_AUTHORIZATION_HANDLE)my_gballoc_malloc(1);
}

static char* my_IoTHubClient_Auth_Get_TrustBundle(IOTHUB_AUTHORIZATION_HANDLE handle, const char* certificate_file_name)
{
    (void)handle;
    (void)certificate_file_name;
    return (char*)my_gballoc_malloc(1);
}

static void my_IoTHubClient_Auth_Destroy(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    my_gballoc_free(handle);
}

static IOTHUB_AUTHORIZATION_HANDLE my_IoTHubClient_Auth_CreateFromDeviceAuth(const char* device_id, const char* module_id)
{
    (void)device_id;
    (void)module_id;
    return (IOTHUB_AUTHORIZATION_HANDLE)my_gballoc_malloc(1);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    size_t len = strlen(source);
    *destination = (char*)my_gballoc_malloc(len+1);
    (void)strcpy(*destination, source);
    return 0;
}


static STRING_HANDLE my_STRING_new(void)
{
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    STRING_HANDLE result;
    if (g_fail_string_construct)
    {
        result = (STRING_HANDLE)NULL;
    }
    else
    {
        result = (STRING_HANDLE)my_gballoc_malloc(1);
    }
    return result;
}

STRING_HANDLE STRING_construct_sprintf(const char* psz, ...)
{
    (void)psz;
    STRING_HANDLE result;
    if (g_fail_string_construct_sprintf)
    {
        result = (STRING_HANDLE)NULL;
    }
    else
    {
        result = (STRING_HANDLE)my_gballoc_malloc(1);
    }
    return result;
}

static int m_STRING_concat_with_STRING(STRING_HANDLE handle, STRING_HANDLE arg1)
{
    (void)handle;
    (void)arg1;
    int result;
    if (g_fail_string_concat_with_string)
    {
        result = -1;
    }
    else
    {
        result = 0;
    }
    return result;
}

static STRING_HANDLE my_STRING_clone(STRING_HANDLE handle)
{
    (void)handle;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    (void)handle;

    if (handle != TEST_STRING_HANDLE)
    {
        my_gballoc_free(handle);
    }
}

static STRING_TOKENIZER_HANDLE my_STRING_TOKENIZER_create(STRING_HANDLE handle)
{
    (void)handle;
    return (STRING_TOKENIZER_HANDLE)my_gballoc_malloc(1);
}

static int my_STRING_TOKENIZER_get_next_token(STRING_TOKENIZER_HANDLE t, STRING_HANDLE output, const char* delimiters)
{
    (void)delimiters;
    (void)t;
    (void)output;
    return 0;
}

static void my_STRING_TOKENIZER_destroy(STRING_TOKENIZER_HANDLE handle)
{
    my_gballoc_free(handle);
}

static TICK_COUNTER_HANDLE my_tickcounter_create(void)
{
    return (TICK_COUNTER_HANDLE)my_gballoc_malloc(1);
}

static int my_tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)
{
    (void)tick_counter;
    g_current_ms += 1000;
    *current_ms = g_current_ms;
    return 0;
}

static void my_tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)
{
    my_gballoc_free(tick_counter);
}

static CONSTBUFFER_HANDLE my_CONSTBUFFER_Create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (CONSTBUFFER_HANDLE)my_gballoc_malloc(1);
}

static void my_CONSTBUFFER_DecRef(CONSTBUFFER_HANDLE constbufferHandle)
{
    my_gballoc_free(constbufferHandle);
}

#ifndef DONT_USE_UPLOADTOBLOB
static IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE my_IoTHubClient_LL_UploadToBlob_Create(const IOTHUB_CLIENT_CONFIG* config, IOTHUB_AUTHORIZATION_HANDLE auth_handle)
{
    (void)config;
    (void)auth_handle;
    return (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE)my_gballoc_malloc(1);
}

static void my_IoTHubClient_LL_UploadToBlob_Destroy(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle)
{
    my_gballoc_free(handle);
}
#endif

#ifdef USE_EDGE_MODULES
static IOTHUB_CLIENT_EDGE_HANDLE my_IoTHubModuleClient_LL_MethodHandle_Create(const IOTHUB_CLIENT_CONFIG* config, IOTHUB_AUTHORIZATION_HANDLE authorizationHandle, const char* moduleId)
{
    (void)config;
    (void)authorizationHandle;
    (void)moduleId;
    return (IOTHUB_CLIENT_EDGE_HANDLE)my_gballoc_malloc(1);
}

static void my_IoTHubModuleClient_LL_MethodHandle_Destroy(IOTHUB_CLIENT_EDGE_HANDLE handle)
{
    my_gballoc_free(handle);
}
#endif

static TRANSPORT_LL_HANDLE my_FAKE_IoTHubTransport_Create(const IOTHUBTRANSPORT_CONFIG* config, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    (void)config;
    g_transport_cb_ctx = ctx;
    g_transport_cb_info.msg_input_cb = cb_info->msg_input_cb;
    g_transport_cb_info.msg_cb = cb_info->msg_cb;
    g_transport_cb_info.connection_status_cb = cb_info->connection_status_cb;
    g_transport_cb_info.send_complete_cb = cb_info->send_complete_cb;
    g_transport_cb_info.prod_info_cb = cb_info->prod_info_cb;
    g_transport_cb_info.twin_rpt_state_complete_cb = cb_info->twin_rpt_state_complete_cb;
    g_transport_cb_info.twin_retrieve_prop_complete_cb = cb_info->twin_retrieve_prop_complete_cb;
    g_transport_cb_info.method_complete_cb = cb_info->method_complete_cb;

    return TEST_TRANSPORT_LL_HANDLE;
}

static IOTHUB_DEVICE_HANDLE my_FAKE_IoTHubTransport_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend)
{
    (void)handle;
    (void)device;
    (void)waitingToSend;
    return (IOTHUB_DEVICE_HANDLE)my_gballoc_malloc(1);
}

static void my_FAKE_IoTHubTransport_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    my_gballoc_free(deviceHandle);
}

static IOTHUB_CLIENT_RESULT my_FAKE_IoTHubTransport_GetSendStatus(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_STATUS* iotHubClientStatus)
{
    (void)handle;
    (void)iotHubClientStatus;
    return IOTHUB_CLIENT_OK;
}

static int my_FAKE_IoTHubTransport_Common_Subscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    return 0;
}

static void my_FAKE_IoTHubTransport_Common_Unsubscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
}

static int my_FAKE_IoTHubTransport_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    (void)handle;
    (void)retryPolicy;
    (void)retryTimeoutLimitInSeconds;
    return 0;
}

static int my_FAKE_DeviceMethod_Response(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t resp_size, int status_response)
{
    (void)handle;
    (void)methodId;
    (void)response;
    (void)resp_size;
    (void)status_response;
    return 0;
}

STRING_HANDLE my_FAKE_IoTHubTransport_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    (void)handle;
    return TEST_STRING_HANDLE;
}

static IOTHUB_CLIENT_RESULT my_FAKE_IoTHubTransport_GetTwinAsync_result;
static IOTHUB_DEVICE_HANDLE my_FAKE_IoTHubTransport_GetTwinAsync_handle;
static IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK my_FAKE_IoTHubTransport_GetTwinAsync_completionCallback;
static void* my_FAKE_IoTHubTransport_GetTwinAsync_callbackContext;
static IOTHUB_CLIENT_RESULT my_FAKE_IoTHubTransport_GetTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
{
    my_FAKE_IoTHubTransport_GetTwinAsync_handle = handle;
    my_FAKE_IoTHubTransport_GetTwinAsync_completionCallback = completionCallback;
    my_FAKE_IoTHubTransport_GetTwinAsync_callbackContext = callbackContext;
    return my_FAKE_IoTHubTransport_GetTwinAsync_result;
}


STRING_HANDLE my_plafrom_get_platform_info(PLATFORM_INFO_OPTION options)
{
    (void)options;
    STRING_HANDLE result;
    if (g_fail_platform_get_platform_info)
    {
        result = NULL;
    }
    else
    {
        result = (STRING_HANDLE)my_gballoc_malloc(1);
    }
    return result;
}

static TRANSPORT_PROVIDER FAKE_transport_provider =
{
    FAKE_IoTHubTransport_SendMessageDisposition,   /*pfIotHubTransport_SendMessageDisposition IoTHubTransport_SendMessageDisposition;*/
    FAKE_IoTHubTransport_Subscribe_DeviceMethod, /*pfIoTHubTransport_Subscribe_DeviceMethod IoTHubTransport_Subscribe_DeviceMethod;*/
    FAKE_IoTHubTransport_Unsubscribe_DeviceMethod, /*pfIoTHubTransport_Unsubscribe_DeviceMethod IoTHubTransport_Unsubscribe_DeviceMethod;*/
    FAKE_IoTHubTransport_DeviceMethod_Response, /*pfIoTHubTransport_DeviceMethod_Response IoTHubTransport_DeviceMethod_Response;*/
    FAKE_IoTHubTransport_Subscribe_DeviceTwin, /*pfIoTHubTransport_Subscribe_DeviceTwin IoTHubTransport_Subscribe_DeviceTwin; */
    FAKE_IoTHubTransport_Unsubscribe_DeviceTwin, /*pfIoTHubTransport_Unsubscribe_DeviceTwin IoTHubTransport_Unsubscribe_DeviceTwin; */
    FAKE_IoTHubTransport_ProcessItem,   /*pfIoTHubTransport_ProcessItem IoTHubTransport_ProcessItem     */
    FAKE_IoTHubTransport_GetHostname,   /*pfIoTHubTransport_GetHostname IoTHubTransport_GetHostname     */
    FAKE_IoTHubTransport_SetOption,     /*pfIoTHubTransport_SetOption IoTHubTransport_SetOption;        */
    FAKE_IoTHubTransport_Create,        /*pfIoTHubTransport_Create IoTHubTransport_Create;              */
    FAKE_IoTHubTransport_Destroy,       /*pfIoTHubTransport_Destroy IoTHubTransport_Destroy;            */
    FAKE_IoTHubTransport_Register,      /*pfIotHubTransport_Register IoTHubTransport_Register;          */
    FAKE_IoTHubTransport_Unregister,    /*pfIotHubTransport_Unregister IoTHubTransport_Unregister;       */
    FAKE_IoTHubTransport_Subscribe,     /*pfIoTHubTransport_Subscribe IoTHubTransport_Subscribe;        */
    FAKE_IoTHubTransport_Unsubscribe,   /*pfIoTHubTransport_Unsubscribe IoTHubTransport_Unsubscribe;    */
    FAKE_IoTHubTransport_DoWork,        /*pfIoTHubTransport_DoWork IoTHubTransport_DoWork;              */
    FAKE_IoTHubTransport_SetRetryPolicy,/*pfIoTHubTransport_SetRetryPolicy IoTHubTransport_SetRetryPolicy;*/
    FAKE_IoTHubTransport_GetSendStatus, /*pfIoTHubTransport_GetSendStatus IoTHubTransport_GetSendStatus;*/
    FAKE_IotHubTransport_Subscribe_InputQueue, /*pfIoTHubTransport_Subscribe_InputQueue IoTHubTransport_Subscribe_InputQueue; */
    FAKE_IotHubTransport_Unsubscribe_InputQueue, /*pfIoTHubTransport_Unsubscribe_InputQueue IoTHubTransport_Unsubscribe_InputQueue; */
    FAKE_IoTHubTransport_SetCallbackContext,
    FAKE_IoTHubTransport_GetTwinAsync,   /*pfIoTHubTransport_GetTwinAsync IoTHubTransport_GetTwinAsync;*/
    FAKE_IoTHubTransport_GetSupportedPlatformInfo
};

static const TRANSPORT_PROVIDER* provideFAKE(void)
{
    return &FAKE_transport_provider;
}

#ifndef DONT_USE_UPLOADTOBLOB
static void my_FileUpload_GetData_Callback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    (void)data;
    (void)size;
    (void)context;
    (void)result;
}

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT my_FileUpload_GetData_CallbackEx(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    (void)data;
    (void)size;
    (void)context;
    (void)result;
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}
#endif /* DONT_USE_UPLOADTOBLOB */

typedef struct MESSAGE_INPUT_CALLBACK_CONTEXT_TAG
{
    int i1;
    int i2;
    int i3;
    bool returnValue;
} MESSAGE_INPUT_CALLBACK_CONTEXT;

MESSAGE_INPUT_CALLBACK_CONTEXT expectedContextData;

void SetMessageInputContextValues(MESSAGE_INPUT_CALLBACK_CONTEXT *messageInputCallbackContext, int i1, int i2, int i3, bool returnValue)
{
    messageInputCallbackContext->i1 = i1;
    messageInputCallbackContext->i2 = i2;
    messageInputCallbackContext->i3 = i3;
    messageInputCallbackContext->returnValue = returnValue;
}

void VerifyMessageInputContextEqual(MESSAGE_INPUT_CALLBACK_CONTEXT *messageInputCallbackContextExpected, MESSAGE_INPUT_CALLBACK_CONTEXT *messageInputCallbackActual)
{
    ASSERT_ARE_EQUAL(int, messageInputCallbackContextExpected->i1, messageInputCallbackActual->i1);
    ASSERT_ARE_EQUAL(int, messageInputCallbackContextExpected->i2, messageInputCallbackActual->i2);
    ASSERT_ARE_EQUAL(int, messageInputCallbackContextExpected->i3, messageInputCallbackActual->i3);
    ASSERT_ARE_EQUAL(bool, messageInputCallbackContextExpected->returnValue, messageInputCallbackActual->returnValue);
}

bool real_messageInputCallbackEx(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    (void)message;
    MESSAGE_INPUT_CALLBACK_CONTEXT* actualUserContextCallback = (MESSAGE_INPUT_CALLBACK_CONTEXT*)userContextCallback;
    VerifyMessageInputContextEqual(&expectedContextData, actualUserContextCallback);
    return actualUserContextCallback->returnValue;
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

    void real_DList_InitializeListHead(PDLIST_ENTRY listHead);
    int real_DList_IsListEmpty(const PDLIST_ENTRY listHead);
    void real_DList_InsertTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
    void real_DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
    void real_DList_AppendTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY ListToAppend);
    int real_DList_RemoveEntryList(PDLIST_ENTRY listEntry);
    PDLIST_ENTRY real_DList_RemoveHeadList(PDLIST_ENTRY listHead);

#ifdef __cplusplus
}
#endif

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(iothub_client_core_ll_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    umock_c_init(on_umock_c_error);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_TOKENIZER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_TRANSPORT_PROVIDER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_IDENTITY_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PLATFORM_INFO_OPTION, int);

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_PROCESS_ITEM_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_TWIN_UPDATE_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, int);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_MATCH_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ACTION_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_CONDITION_FUNCTION, void*);

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);

#ifndef DONT_USE_UPLOADTOBLOB
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, void*);
#endif // DONT_USE_UPLOADTOBLOB

#ifdef USE_EDGE_MODULES
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_EDGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_SECURITY_TYPE, int);
#endif // USE_EDGE_MODULES

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_GetVersionString, "version 1.0");

    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_Subscribe_DeviceTwin, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Subscribe_DeviceTwin, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_GetTwinAsync, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_GetTwinAsync, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_GetTwinAsync, my_FAKE_IoTHubTransport_GetTwinAsync);

    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_ProcessItem, IOTHUB_PROCESS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_ProcessItem, IOTHUB_PROCESS_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_GetHostname, my_FAKE_IoTHubTransport_GetHostname);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_GetHostname, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_SetOption, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_Create, my_FAKE_IoTHubTransport_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_Register, my_FAKE_IoTHubTransport_Register);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Register, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_Unregister, my_FAKE_IoTHubTransport_Unregister);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_Subscribe, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Subscribe, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_SetRetryPolicy, my_FAKE_IoTHubTransport_SetRetryPolicy);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_SetRetryPolicy, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_GetSendStatus, my_FAKE_IoTHubTransport_GetSendStatus);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_GetSendStatus, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_Subscribe_DeviceMethod, 0);

    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IotHubTransport_Subscribe_InputQueue, my_FAKE_IoTHubTransport_Common_Subscribe_InputQueue);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IotHubTransport_Subscribe_InputQueue, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IotHubTransport_Unsubscribe_InputQueue, my_FAKE_IoTHubTransport_Common_Unsubscribe_InputQueue);

    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_SetCallbackContext, 0);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_GetSupportedPlatformInfo, 0);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_Subscribe_DeviceMethod, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubMessage_GetMessageId, "1");
    REGISTER_GLOBAL_MOCK_RETURN(FAKE_IoTHubTransport_SendMessageDisposition, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_SendMessageDisposition, IOTHUB_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(FAKE_IoTHubTransport_DeviceMethod_Response, my_FAKE_DeviceMethod_Response);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(FAKE_IoTHubTransport_DeviceMethod_Response, MU_FAILURE);

#ifndef DONT_USE_UPLOADTOBLOB
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_UploadToBlob_Create, my_IoTHubClient_LL_UploadToBlob_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_LL_UploadToBlob_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_UploadToBlob_Destroy, my_IoTHubClient_LL_UploadToBlob_Destroy);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_UploadToBlob_SetOption, IOTHUB_CLIENT_OK);
#endif

#ifdef USE_EDGE_MODULES
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_EdgeHandle_Destroy, my_IoTHubModuleClient_LL_MethodHandle_Destroy);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_EdgeHandle_Create, my_IoTHubModuleClient_LL_MethodHandle_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_EdgeHandle_Create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Edge_ModuleMethodInvoke, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Edge_ModuleMethodInvoke, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(iothub_security_init, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(iothub_security_init, 1);
#endif

    REGISTER_GLOBAL_MOCK_RETURN(environment_get_variable, ENVVARIABLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(environment_get_variable, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(deviceMethodCallback, 200);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_CreateFromString, (IOTHUB_MESSAGE_HANDLE)0x44);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_Clone, (IOTHUB_MESSAGE_HANDLE)0x44);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_Clone, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetOutputName, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetOutputName, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetInputName, TEST_INPUT_NAME);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetInputName, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_Diagnostic_AddIfNecessary, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Diagnostic_AddIfNecessary, 100);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_CreateFromDeviceAuth, my_IoTHubClient_Auth_CreateFromDeviceAuth);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_CreateFromDeviceAuth, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, (time_t)TEST_TIME_VALUE);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_concat_with_STRING, m_STRING_concat_with_STRING);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, my_STRING_clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_clone, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_build, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_build, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_Create, my_CONSTBUFFER_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(CONSTBUFFER_Create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(CONSTBUFFER_DecRef, my_CONSTBUFFER_DecRef);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_create, my_STRING_TOKENIZER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_get_next_token, my_STRING_TOKENIZER_get_next_token);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_TOKENIZER_get_next_token, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_TOKENIZER_destroy, my_STRING_TOKENIZER_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_create, my_tickcounter_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_destroy, my_tickcounter_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_get_current_ms, my_tickcounter_get_current_ms);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_get_current_ms, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(DList_InitializeListHead, real_DList_InitializeListHead);
    REGISTER_GLOBAL_MOCK_HOOK(DList_IsListEmpty, real_DList_IsListEmpty);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertTailList, real_DList_InsertTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertHeadList, real_DList_InsertHeadList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_AppendTailList, real_DList_AppendTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveEntryList, real_DList_RemoveEntryList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveHeadList, real_DList_RemoveHeadList);

    REGISTER_GLOBAL_MOCK_RETURN(test_message_callback_async, IOTHUBMESSAGE_ACCEPTED);
    REGISTER_GLOBAL_MOCK_RETURN(messageCallback, IOTHUBMESSAGE_ACCEPTED);
    REGISTER_GLOBAL_MOCK_RETURN(messageCallbackEx, true);
    REGISTER_GLOBAL_MOCK_HOOK(messageInputCallbackEx, real_messageInputCallbackEx);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Create, my_IoTHubClient_Auth_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Create, NULL);

#ifdef USE_EDGE_MODULES
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Get_TrustBundle, my_IoTHubClient_Auth_Get_TrustBundle);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClient_Auth_Get_TrustBundle, NULL);
#endif

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_Auth_Destroy, my_IoTHubClient_Auth_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(platform_get_platform_info, my_plafrom_get_platform_info);

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

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_UploadToBlob_Impl, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl, IOTHUB_CLIENT_OK);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    TEST_MUTEX_ACQUIRE(test_serialize_mutex);
    umock_c_reset_all_calls();
    g_fail_string_construct_sprintf = false;
    g_fail_platform_get_platform_info = false;
    g_fail_string_concat_with_string = false;
    g_fail_string_construct = false;

    g_transport_cb_ctx = NULL;
    memset(&g_transport_cb_info, 0, sizeof(TRANSPORT_CALLBACKS_INFO));

    my_FAKE_IoTHubTransport_GetTwinAsync_result = IOTHUB_CLIENT_OK;
    my_FAKE_IoTHubTransport_GetTwinAsync_handle = NULL;
    my_FAKE_IoTHubTransport_GetTwinAsync_completionCallback = NULL;
    my_FAKE_IoTHubTransport_GetTwinAsync_callbackContext = NULL;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

static void setup_IoTHubClientCore_LL_create_mocks(bool use_device_config, bool is_edge_module)
{
    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    if (use_device_config)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_CreateFromDeviceAuth(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        //STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        //STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/

#ifdef USE_EDGE_MODULES
    if (is_edge_module)
    {
        STRICT_EXPECTED_CALL(IoTHubClient_EdgeHandle_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
#else
        (void)is_edge_module;
#endif /*USE_EDGE_MODULES*/

    STRICT_EXPECTED_CALL(tickcounter_create());

    PLATFORM_INFO_OPTION supportedPlatformInfo = PLATFORM_INFO_OPTION_RETRIEVE_SQM;
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_RETRIEVE_SQM));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Register(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, TEST_RETRY_POLICY, 0));
}

static void setup_IoTHubClientCore_LL_sendreportedstate_mocks()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_Create(TEST_REPORTED_STATE, TEST_REPORTED_SIZE));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceTwin(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void setup_IoTHubClientCore_LL_sendeventasync_mocks(bool invoke_tickcounter)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);

    if (invoke_tickcounter)
    {
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubClient_Diagnostic_AddIfNecessary(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
}

static void setup_IoTHubClientCore_LL_createfromconnectionstring_2_mocks(const char* device_token, bool provisioning)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_HOSTNAME_TOKEN).CallCannotFail();
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_HOSTNAME_TOKEN).CallCannotFail();;
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle().CallCannotFail();;
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEID_TOKEN).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);  // 20
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().CallCannotFail();;

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(device_token).CallCannotFail();;
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn("true").CallCannotFail();;

    /* loop exit */
    // Mark CallCannotFail because this is final pass through loop, expected to bail out, so shouldn't failure test it.
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(1).CallCannotFail();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(NULL).CallCannotFail();

    setup_IoTHubClientCore_LL_create_mocks(provisioning, false);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();
}

static void setup_IoTHubClientCore_LL_createfromconnectionstring_mocks(const char* device_token, const char* token_value, bool include_invalid_tokens)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_HOSTNAME_TOKEN).CallCannotFail();
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail(); // 14
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_DEVICEID_TOKEN).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));  // 19
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(device_token).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(token_value).CallCannotFail(); // 25

    if (include_invalid_tokens)
    {
        EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INVALID_TOKEN1).CallCannotFail();

        EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INVALID_TOKEN2).CallCannotFail();
    }

    /* loop exit */
    // Mark CallCannotFail because this is final pass through loop, expected to bail out, so shouldn't failure test it.
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__).CallCannotFail(); // 26

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL).CallCannotFail(); // 27

    setup_IoTHubClientCore_LL_create_mocks(false, false);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // 38
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_with_DeviceKey_fail)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    //arrange
    setup_IoTHubClientCore_LL_createfromconnectionstring_mocks(TEST_DEVICEKEY_TOKEN, TEST_STRING_VALUE, false);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_LL_CreateFromConnectionString failure in test %zu/%zu", index, count);

            //act
            IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

            ///assert
            ASSERT_IS_NULL(result, tmp_msg);
        }
    }

    ///cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_with_DeviceKey_succeeds)
{
    //arrange
    setup_IoTHubClientCore_LL_createfromconnectionstring_mocks(TEST_DEVICEKEY_TOKEN, TEST_STRING_VALUE, false);

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_with_provisioning_succeeds)
{
    //arrange
    setup_IoTHubClientCore_LL_createfromconnectionstring_2_mocks(TEST_PROVISIONING_TOKEN, true);

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_with_DeviceSasToken_succeeds)
{
    //arrange
    setup_IoTHubClientCore_LL_createfromconnectionstring_mocks(TEST_DEVICESAS_TOKEN, TEST_STRING_VALUE, false);

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    ///assert
    ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_with_DeviceSasToken_with_invalid_tokens_succeeds)
{
    //arrange
    setup_IoTHubClientCore_LL_createfromconnectionstring_mocks(TEST_DEVICESAS_TOKEN, TEST_STRING_VALUE, true);

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    ///assert
    ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(result);
}


TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_withGatewayHostName_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_HOSTNAME_TOKEN);
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEID_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);  // 20
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEKEY_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 4*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(0);
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_PROTOCOL_GATEWAY_HOST_NAME_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop exit */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(1); // 27

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(NULL);

    setup_IoTHubClientCore_LL_create_mocks(false, false);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // 36
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(result);
}


TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_if_input_parameter_connectionString_is_NULL_then_return_NULL)
{
    //arrange

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(NULL, provideFAKE);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_if_input_parameter_protocol_is_NULL_then_return_NULL)
{
    //arrange

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, NULL);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_with_x509_true_succeeds)
{
    //arrange
    setup_IoTHubClientCore_LL_createfromconnectionstring_2_mocks(TEST_X509, false);

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);

}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_with_x509_test_string_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_HOSTNAME_TOKEN);
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEID_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);  // 20
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_X509);
    /* loop exit */
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_STRING_VALUE);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); // 36
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);

}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromConnectionString_with_ModuleId_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreArgument_size();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG)).IgnoreArgument_handle();
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());
    STRICT_EXPECTED_CALL(STRING_new());

    /* loop 1 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_HOSTNAME_TOKEN);
    EXPECTED_CALL(STRING_TOKENIZER_create(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();
    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    /* loop 2 */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEID_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);  // 20
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 3*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_DEVICEKEY_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle();

    /* loop 4*/
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(0);
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_MODULE_ID_TOKEN);
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG)).IgnoreArgument(1);

    /* loop exit */
    EXPECTED_CALL(STRING_TOKENIZER_get_next_token(TEST_STRING_TOKENIZER_HANDLE, TEST_STRING_HANDLE, IGNORED_PTR_ARG))
        .SetReturn(1); // 33

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).IgnoreArgument_handle().SetReturn(TEST_MODULE_ID_TOKEN);

    setup_IoTHubClientCore_LL_create_mocks(false, false);

    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)).IgnoreArgument_ptr();

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromConnectionString(TEST_CHAR, provideFAKE);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);

}


TEST_FUNCTION(IoTHubClientCore_LL_Create_with_NULL_parameter_fails)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_Create(NULL);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

TEST_FUNCTION(IoTHubClientCore_LL_Create_with_NULL_protocol_fails)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_Create(&TEST_CONFIG_NULL_protocol);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

TEST_FUNCTION(IoTHubClientCore_LL_Create_succeeds)
{
    //arrange
    IOTHUB_DEVICE_CONFIG device;
    device.deviceId = TEST_CONFIG.deviceId;
    device.deviceKey = TEST_CONFIG.deviceKey;
    device.deviceSasToken = NULL;

    setup_IoTHubClientCore_LL_create_mocks(false, false);

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    ///assert
    ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_Create_fail)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_DEVICE_CONFIG device;
    device.deviceId = TEST_CONFIG.deviceId;
    device.deviceKey = TEST_CONFIG.deviceKey;
    device.deviceSasToken = NULL;
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_create_mocks(false, false);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_Create(&TEST_CONFIG);

            //assert
            ASSERT_ARE_EQUAL(void_ptr, NULL, result, "IoTHubClientCore_LL_Create failure in test %zu/%zu", index, count);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_null_config_fails)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(NULL);

    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_null_protocol_fails)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG_null_protocol);

    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_null_transportHandle_fails)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG_null_handle);

    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_with_NULL_deviceKey_AND_NULL_sas_token_fails)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG_NULL_device_key_NULL_sas_token);

    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_Succeeds)
{
    //arrange
    IOTHUB_DEVICE_CONFIG device;
    device.deviceId = TEST_DEVICE_CONFIG.deviceId;
    device.deviceKey = TEST_DEVICE_CONFIG.deviceKey;
    device.deviceSasToken = NULL;

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetCallbackContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetHostname(IGNORED_PTR_ARG)); /*this is getting the hostname as STRING_HANDLE*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the hostname as const char* */
        .SetReturn(TEST_HOSTNAME_VALUE);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/
    STRICT_EXPECTED_CALL(tickcounter_create());
    PLATFORM_INFO_OPTION supportedPlatformInfo = PLATFORM_INFO_OPTION_RETRIEVE_SQM;
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_RETRIEVE_SQM));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Register(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, TEST_RETRY_POLICY, 0));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}

#ifdef USE_PROV_MODULE
TEST_FUNCTION(IoTHubClientCore_LL_CreateFromDeviceAuth_URI_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromDeviceAuth(NULL, TEST_DEVICEID_TOKEN, provideFAKE);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromDeviceAuth_device_id_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromDeviceAuth(TEST_PROV_URI, NULL, provideFAKE);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromDeviceAuth_protocol_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromDeviceAuth(TEST_PROV_URI, TEST_DEVICEID_TOKEN, NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromDeviceAuth_bad_uri_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromDeviceAuth(TEST_CHAR, TEST_DEVICEID_TOKEN, provideFAKE);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromDeviceAuth_Succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    setup_IoTHubClientCore_LL_create_mocks(true, false);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromDeviceAuth(TEST_PROV_URI, TEST_DEVICEID_TOKEN, provideFAKE);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}
#endif


TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_fail)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    //arrange
    IOTHUB_DEVICE_CONFIG device;
    device.deviceId = TEST_DEVICE_CONFIG.deviceId;
    device.deviceKey = TEST_DEVICE_CONFIG.deviceKey;
    device.deviceSasToken = NULL;

    setup_IoTHubClientCore_LL_create_mocks(true, false);

    //act
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "IoTHubClientCore_LL_CreateWithTransport failure in test %zu/%zu", index, count);

            IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

            //assert
            ASSERT_ARE_EQUAL(void_ptr, NULL, result, tmp_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_create_tickcounter_fails_shared_transport_is_not_destroyed)
{
    //arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetCallbackContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetHostname(IGNORED_PTR_ARG)); /*this is getting the hostname as STRING_HANDLE*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the hostname as const char* */
        .SetReturn(TEST_HOSTNAME_VALUE);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/

    STRICT_EXPECTED_CALL(tickcounter_create())
        .SetReturn(NULL);

    // Failure cleanup calls
    // Note: not destroying the shared transport!
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/
#ifdef USE_EDGE_MODULES
    STRICT_EXPECTED_CALL(IoTHubClient_EdgeHandle_Destroy(IGNORED_PTR_ARG));
#endif /*USE_EDGE_MODULES*/
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

    //assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_register_fails_shared_transport_is_not_destroyed)
{
    //arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetCallbackContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetHostname(IGNORED_PTR_ARG)); /*this is getting the hostname as STRING_HANDLE*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the hostname as const char* */
        .SetReturn(TEST_HOSTNAME_VALUE);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/

    STRICT_EXPECTED_CALL(tickcounter_create());
    PLATFORM_INFO_OPTION supportedPlatformInfo = PLATFORM_INFO_OPTION_RETRIEVE_SQM;
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_RETRIEVE_SQM));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Register(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(NULL);

    // Failure cleanup calls
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    // Note: not destroying the shared transport!
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/
#ifdef USE_EDGE_MODULES
    STRICT_EXPECTED_CALL(IoTHubClient_EdgeHandle_Destroy(IGNORED_PTR_ARG));
#endif /*USE_EDGE_MODULES*/
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateWithTransport_set_retry_policy_fails_shared_transport_is_not_destroyed)
{
    //arrange
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetCallbackContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetHostname(IGNORED_PTR_ARG)); /*this is getting the hostname as STRING_HANDLE*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*this is getting the hostname as const char* */
        .SetReturn(TEST_HOSTNAME_VALUE);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/

    STRICT_EXPECTED_CALL(tickcounter_create());
    PLATFORM_INFO_OPTION supportedPlatformInfo = PLATFORM_INFO_OPTION_RETRIEVE_SQM;
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_RETRIEVE_SQM));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Register(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, TEST_RETRY_POLICY, 0))
        .SetReturn(IOTHUB_CLIENT_ERROR);

    // Failure cleanup calls
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    // Note: not destroying the shared transport!
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif /*DONT_USE_UPLOADTOBLOB*/
#ifdef USE_EDGE_MODULES
    STRICT_EXPECTED_CALL(IoTHubClient_EdgeHandle_Destroy(IGNORED_PTR_ARG));
#endif /*USE_EDGE_MODULES*/
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_Destroy_with_NULL_succeeds)
{
    //arrange

    //act
    IoTHubClientCore_LL_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_Destroys_the_underlying_transport_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(TEST_HOSTNAME_VALUE);
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif
#ifdef USE_EDGE_MODULES
    STRICT_EXPECTED_CALL(IoTHubClient_EdgeHandle_Destroy(IGNORED_PTR_ARG));
#endif

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_Destroys_unregisters_but_does_not_destroy_transport_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(TEST_HOSTNAME_VALUE);
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_CreateWithTransport(&TEST_DEVICE_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif
#ifdef USE_EDGE_MODULES
    STRICT_EXPECTED_CALL(IoTHubClient_EdgeHandle_Destroy(IGNORED_PTR_ARG));
#endif

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_Destroy(handle);

    //assert -uMock does it
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventAsync_with_NULL_iotHubClientHandle_fails)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventAsync(NULL, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, (void*)3);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventAsync_with_NULL_messageHandle_fails)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventAsync(handle, NULL, test_event_confirmation_callback, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventAsync_with_NULL_test_event_confirmation_callback_and_non_NULL_context_fails)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventAsync(handle, TEST_MESSAGE_HANDLE, NULL, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);

}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventAsync_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_sendeventasync_mocks(false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventAsync(handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_Destroy_after_sendEvent_succeeds)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    IoTHubClientCore_LL_SendEventAsync(handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); /*IOTHUBMESSAGE*/

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif
#ifdef USE_EDGE_MODULES
    STRICT_EXPECTED_CALL(IoTHubClient_EdgeHandle_Destroy(IGNORED_PTR_ARG));
#endif

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventAsync_fails)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t thisIsNotZero = 312984751;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &thisIsNotZero); /*this forces _SendEventAsync to query the currentTime. If that fails, _SendEvent should fail as well*/
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_sendeventasync_mocks(true);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "IoTHubClientCore_LL_Create failure in test %zu/%zu", index, count);

            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventAsync(handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, (void*)1);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
        }
    }

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_LL_SetConnectionStatusCallback_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetConnectionStatusCallback(NULL, connectionStatusCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_SetConnectionStatusCallback_with_non_NULL_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetConnectionStatusCallback(handle, connectionStatusCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback(NULL, messageCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_with_NULL_calls_Unsubscribe_and_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback(handle, test_message_callback_async, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_with_NULL_after_SetMessageCallbacEx_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_after_SetMessageCallbacEx_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback(handle, messageCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_Ex_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback_Ex(NULL, messageCallbackEx, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_Ex_with_NULL_not_subscribed_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback_Ex(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_Ex_with_NULL_after_SetMessageCallback_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback(handle, messageCallback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback_Ex(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_Ex_with_NULL_unsubscribe_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback_Ex(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_Ex_Subscribe_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(__LINE__);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_Ex_after_SetMessageCallback_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback(handle, messageCallback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_Ex_happy_path_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendMessageDisposition_with_first_NULL_fails)
{
    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendMessageDisposition(NULL, NULL, IOTHUBMESSAGE_ACCEPTED);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_SendMessageDisposition_with_second_NULL_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendMessageDisposition(NULL, NULL, IOTHUBMESSAGE_ACCEPTED);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendMessageDisposition_Succeeds)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendMessageDisposition(handle, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendComplete_with_1_items_with_callback_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);
    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = eventConfirmationCallback;
    one->context = (void*)1;
    DList_InsertTailList(&temp, &(one->entry));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    g_transport_cb_info.send_complete_cb(&temp, IOTHUB_CLIENT_CONFIRMATION_OK, g_transport_cb_ctx);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendComplete_with_3_items_with_callback_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);

    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = eventConfirmationCallback;
    one->context = (void*)1;
    DList_InsertTailList(&temp, &(one->entry));

    IOTHUB_MESSAGE_LIST* two = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    two->messageHandle = (IOTHUB_MESSAGE_HANDLE)2;
    two->callback = eventConfirmationCallback;
    two->context = (void*)2;
    DList_InsertTailList(&temp, &(two->entry));

    IOTHUB_MESSAGE_LIST* three = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    three->messageHandle = (IOTHUB_MESSAGE_HANDLE)3;
    three->callback = eventConfirmationCallback;
    three->context = (void*)3;
    DList_InsertTailList(&temp, &(three->entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)2));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)2));
    STRICT_EXPECTED_CALL(gballoc_free(two));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)3));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)3));
    STRICT_EXPECTED_CALL(gballoc_free(three));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    g_transport_cb_info.send_complete_cb(&temp, IOTHUB_CLIENT_CONFIRMATION_OK, g_transport_cb_ctx);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendComplete_with_3_items_with_callback_but_batch_failed_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);

    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = eventConfirmationCallback;
    one->context = (void*)1;
    DList_InsertTailList(&temp, &(one->entry));

    IOTHUB_MESSAGE_LIST* two = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    two->messageHandle = (IOTHUB_MESSAGE_HANDLE)2;
    two->callback = eventConfirmationCallback;
    two->context = (void*)2;
    DList_InsertTailList(&temp, &(two->entry));

    IOTHUB_MESSAGE_LIST* three = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    three->messageHandle = (IOTHUB_MESSAGE_HANDLE)3;
    three->callback = eventConfirmationCallback;
    three->context = (void*)3;
    DList_InsertTailList(&temp, &(three->entry));


    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_ERROR, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_ERROR, (void*)2));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)2));
    STRICT_EXPECTED_CALL(gballoc_free(two));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_ERROR, (void*)3));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)3));
    STRICT_EXPECTED_CALL(gballoc_free(three));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    g_transport_cb_info.send_complete_cb(&temp, IOTHUB_CLIENT_CONFIRMATION_ERROR, g_transport_cb_ctx);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}


TEST_FUNCTION(IoTHubClientCore_LL_SetRetryPolicy_with_NULL_iotHubClientHandle_fails)
{
    ///arrange
    umock_c_reset_all_calls();
    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetRetryPolicy(NULL, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

TEST_FUNCTION(IoTHubClientCore_LL_SetRetryPolicy_Retrytimeout_0_pass)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    for (int policy = 0; policy <= 6; policy++)
    {
        STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, (IOTHUB_CLIENT_RETRY_POLICY)policy, 0))
                .IgnoreArgument(1);

        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetRetryPolicy(handle, (IOTHUB_CLIENT_RETRY_POLICY)policy, 0);
        ///assert

        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    }

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetRetryPolicy_success)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetRetryPolicy(IGNORED_PTR_ARG, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS));

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetRetryPolicy(handle, TEST_RETRY_POLICY, TEST_RETRY_TIMEOUT_SECS);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_ConnectionStatusCallBack_calls_upper_layer_succeeds)
{
    ///arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetConnectionStatusCallback(handle, connectionStatusCallback, (void*)11);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(connectionStatusCallback((IOTHUB_CLIENT_CONNECTION_STATUS)IGNORED_NUM_ARG, (IOTHUB_CLIENT_CONNECTION_STATUS_REASON)IGNORED_NUM_ARG, (void*)11)).IgnoreAllArguments();

    ///act
    g_transport_cb_info.connection_status_cb(IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_ConnectionStatusCallBack_with_NULL_parameter_fails)
{
    ///arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetConnectionStatusCallback(handle, connectionStatusCallback, (void*)11);
    umock_c_reset_all_calls();

    ///act
    g_transport_cb_info.connection_status_cb(IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_ConnectionStatusCallBack_with_NULL_callback_fails)
{
    ///arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetConnectionStatusCallback(handle, NULL, (void*)11);
    umock_c_reset_all_calls();

    ///act
    g_transport_cb_info.connection_status_cb(IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_with_NULL_before_subscribe_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback(handle, NULL, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_with_non_NULL_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback(handle, test_message_callback_async, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetMessageCallback_with_non_fails_when_underlying_transport_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetMessageCallback(handle, test_message_callback_async, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_DoWork_with_NULL_does_nothing)
{
    //arrange

    //act
    IoTHubClientCore_LL_DoWork(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_DoWork_calls_underlying_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*_DoWork will ask "what's the time"*/
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendComplete_with_NULL_handle_shall_return)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    //arrange
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);
    umock_c_reset_all_calls();

    //act
    g_transport_cb_info.send_complete_cb(&temp, IOTHUB_CLIENT_CONFIRMATION_OK, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendComplete_with_NULL_completed_shall_return)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    g_transport_cb_info.send_complete_cb(NULL, IOTHUB_CLIENT_CONFIRMATION_OK, g_transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendComplete_with_empty_completed_shall_return)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(&temp));

    //act
    g_transport_cb_info.send_complete_cb(&temp, IOTHUB_CLIENT_CONFIRMATION_OK, g_transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendComplete_with_3_items_one_without_callback_succeeds) /*for fun*/
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);

    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = test_event_confirmation_callback;
    one->context = (void*)1;
    DList_InsertTailList(&temp, &(one->entry));

    IOTHUB_MESSAGE_LIST* two = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    two->messageHandle = (IOTHUB_MESSAGE_HANDLE)2;
    two->callback = NULL;
    two->context = NULL;
    DList_InsertTailList(&temp, &(two->entry));

    IOTHUB_MESSAGE_LIST* three = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    three->messageHandle = (IOTHUB_MESSAGE_HANDLE)3;
    three->callback = test_event_confirmation_callback;
    three->context = (void*)3;
    DList_InsertTailList(&temp, &(three->entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)1));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)2));
    STRICT_EXPECTED_CALL(gballoc_free(two));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_OK, (void*)3));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)3));
    STRICT_EXPECTED_CALL(gballoc_free(three));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    g_transport_cb_info.send_complete_cb(&temp, IOTHUB_CLIENT_CONFIRMATION_OK, g_transport_cb_ctx);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendComplete_with_3_items_one_with_callback_but_batch_failed_succeeds)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    DLIST_ENTRY temp;
    DList_InitializeListHead(&temp);

    IOTHUB_MESSAGE_LIST* one = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    one->messageHandle = (IOTHUB_MESSAGE_HANDLE)1;
    one->callback = NULL;
    one->context = NULL;
    DList_InsertTailList(&temp, &(one->entry));

    IOTHUB_MESSAGE_LIST* two = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    two->messageHandle = (IOTHUB_MESSAGE_HANDLE)2;
    two->callback = NULL;
    two->context = NULL;
    DList_InsertTailList(&temp, &(two->entry));

    IOTHUB_MESSAGE_LIST* three = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST)); /*this is SendEvent wannabe*/
    three->messageHandle = (IOTHUB_MESSAGE_HANDLE)3;
    three->callback = test_event_confirmation_callback;
    three->context = (void*)3;
    DList_InsertTailList(&temp, &(three->entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)1));
    STRICT_EXPECTED_CALL(gballoc_free(one));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)2));
    STRICT_EXPECTED_CALL(gballoc_free(two));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_ERROR, (void*)3));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)3));
    STRICT_EXPECTED_CALL(gballoc_free(three));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    g_transport_cb_info.send_complete_cb(&temp, IOTHUB_CLIENT_CONFIRMATION_ERROR, g_transport_cb_ctx);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_messageData_NULL_parameter_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    bool result = g_transport_cb_info.msg_cb(NULL, NULL);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_messageData_messageHandle_is_NULL_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    bool result = g_transport_cb_info.msg_cb(NULL, handle);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_unsubscribed_returns_false)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));

    //act
    bool result = g_transport_cb_info.msg_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_messageCallback_calls_client_layer_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback(handle, test_message_callback_async, (void*)11);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(test_message_callback_async(TEST_MESSAGE_HANDLE, (void*)11));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    //act
    bool result = g_transport_cb_info.msg_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_messageCallback_calls_client_layer_succeeds_report_disposition_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback(handle, test_message_callback_async, (void*)11);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(test_message_callback_async(TEST_MESSAGE_HANDLE, (void*)11));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED))
        .SetReturn(IOTHUB_CLIENT_ERROR);

    //act
    bool result = g_transport_cb_info.msg_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_messageCallbackEx_calls_client_layer_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)11);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallbackEx(TEST_MESSAGE_HANDLE, (void*)11));

    //act
    bool result = g_transport_cb_info.msg_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_messageCallbackEx_calls_client_layer_reports_true)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)11);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallbackEx(TEST_MESSAGE_HANDLE, (void*)11));

    //act
    bool result = g_transport_cb_info.msg_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_messageCallbackEx_calls_client_layer_reports_false)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)11);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallbackEx(TEST_MESSAGE_HANDLE, (void*)11))
        .SetReturn(false);

    //act
    bool result = g_transport_cb_info.msg_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallback_with_messageCallbackEx_calls_client_layer_fails_disposition_reporting_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)11);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallbackEx(TEST_MESSAGE_HANDLE, (void*)11))
        .SetReturn(false);

    //act
    bool result = g_transport_cb_info.msg_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

/*** IoTHubClientCore_LL_GetLastMessageReceiveTime ***/

TEST_FUNCTION(IoTHubClientCore_LL_GetLastMessageReceiveTime_InvalidClientHandleArg_fails)
{
    // arrange
    time_t lastMessageReceiveTime;

    IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetLastMessageReceiveTime(NULL, &lastMessageReceiveTime);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    IoTHubClientCore_LL_Destroy(iotHubClientHandle);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetLastMessageReceiveTime_InvalidTimeRefArg_fails)
{
    // arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetLastMessageReceiveTime(iotHubClientHandle, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());


    // Cleanup
    IoTHubClientCore_LL_Destroy(iotHubClientHandle);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetLastMessageReceiveTime_NoMessagesReceived_fails)
{
    // arrange
    time_t lastMessageReceiveTime = (time_t)0;

    IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetLastMessageReceiveTime(iotHubClientHandle, &lastMessageReceiveTime);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INDEFINITE_TIME, result);
    ASSERT_ARE_EQUAL(int, 0, (int)(lastMessageReceiveTime));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    IoTHubClientCore_LL_Destroy(iotHubClientHandle);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetLastMessageReceiveTime_MessagesReceived_succeeds)
{
    // arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetMessageCallback(handle, test_message_callback_async, (void*)11);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(test_message_callback_async(TEST_MESSAGE_HANDLE, (void*)11));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    (void)g_transport_cb_info.msg_cb(TEST_MESSAGE_HANDLE, handle);

    // act
    time_t lastMessageReceiveTime = (time_t)0;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetLastMessageReceiveTime(handle, &lastMessageReceiveTime);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, (int)TEST_TIME_VALUE, (int)lastMessageReceiveTime);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

/*** IoTHubClientCore_LL_GetSendStatus ***/

TEST_FUNCTION(IoTHubClientCore_LL_GetSendStatus_BadHandleArgument_fails)
{
    // arrange

    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetSendStatus(NULL, &status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_GetSendStatus_BadStatusArgument_fails)
{
    // arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetSendStatus(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetSendStatus_NoEventToSend_Succeeds)
{
    // arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS status;
    IOTHUB_CLIENT_STATUS desire_status = IOTHUB_CLIENT_SEND_STATUS_IDLE;

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSendStatus(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .CopyOutArgumentBuffer_iotHubClientStatus(&desire_status, sizeof(status))
        .SetReturn(IOTHUB_CLIENT_OK);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetSendStatus(handle, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_SEND_STATUS_IDLE, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetSendStatus_HasEventToSend_Succeeds)
{
    // arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS status;
    IOTHUB_CLIENT_STATUS desire_status = IOTHUB_CLIENT_SEND_STATUS_BUSY;

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSendStatus(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .CopyOutArgumentBuffer_iotHubClientStatus(&desire_status, sizeof(status))
        .SetReturn(IOTHUB_CLIENT_OK);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetSendStatus(handle, &status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_SEND_STATUS_BUSY, status);

    // cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_sas_token_lifetime_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Set_SasToken_Expiry(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    //act
    size_t sas_lifetime = 10;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, OPTION_SAS_TOKEN_LIFETIME, &sas_lifetime);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_sas_token_lifetime_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Set_SasToken_Expiry(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);

    //act
    size_t sas_lifetime = 255;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, OPTION_SAS_TOKEN_LIFETIME, &sas_lifetime);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_with_NULL_handle_fails)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(NULL, "a", "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_with_NULL_optionName_fails)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, NULL, "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);

}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_with_NULL_value_fails)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, "a", NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);

}

#ifndef DONT_USE_UPLOADTOBLOB
/*these tests are to be run when upload to blob functionality exists*/

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_blob_upload_timeout_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    .IgnoreArgument_handle()
    .IgnoreArgument_optionName()
    .IgnoreArgument_value()
    .SetReturn(IOTHUB_CLIENT_INDEFINITE_TIME)
    .CallCannotFail();

    //act
    long timeout = 10;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, OPTION_BLOB_UPLOAD_TIMEOUT_SECS, &timeout);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INDEFINITE_TIME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);

}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_fails_when_IoTHubTransport_SetOption_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    .IgnoreArgument_handle()
    .IgnoreArgument_optionName()
    .IgnoreArgument_value()
    .SetReturn(IOTHUB_CLIENT_INDEFINITE_TIME);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, "a", "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INDEFINITE_TIME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);

}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_succeeds_when_IoTHubClient_LL_UploadToBlob_SetOption_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    .IgnoreArgument_handle()
    .IgnoreArgument_optionName()
    .IgnoreArgument_value()
    .SetReturn(IOTHUB_CLIENT_OK);

    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
    .IgnoreArgument_handle()
    .IgnoreArgument_optionName()
    .IgnoreArgument_value()
    .SetReturn(IOTHUB_CLIENT_INVALID_ARG);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, "a", "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);

}
#else
/*these tests are to be run when uploadtoblob is not present*/
TEST_FUNCTION(IoTHubClientCore_LL_SetOption_fails_when_underlying_transport_fails)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(FAKE_IoTHubTransport_SetOption(IGNORED_PTR_ARG, "a", "b"))
        .SetReturn(IOTHUB_CLIENT_INDEFINITE_TIME);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, "a", "b");

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INDEFINITE_TIME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);

}
#endif

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_messageTimeout_to_zero_after_Create_succeeds)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    tickcounter_ms_t zero = 0;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &zero);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_messageTimeout_to_one_after_Create_succeeds)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    tickcounter_ms_t one = 1;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);

}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_messageTimeout_can_be_reverted_back_to_zero_succeeds)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);
    umock_c_reset_all_calls();

    //act
    tickcounter_ms_t zero = 0;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &zero);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_messageTimeout_calls_timeout_callback)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);

    /*send 1 messages that will expire*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);
    umock_c_reset_all_calls();

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    tickcounter_ms_t twelve = 12000; /*12 > 10 (receive time) + 1 (timeout) => timeout*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &twelve, sizeof(twelve));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)TEST_DEVICEMESSAGE_HANDLE)); /*calling the callback*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_defaults_to_zero)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    /*send 1 messages that will expire*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);
    umock_c_reset_all_calls();

    /*we don't care what happens in the Transport, so let's ignore all those calls*/

    tickcounter_ms_t twelve = 12000; /*12 > 10 (receive time) + 1 (timeout) => would result in timeout, except the fact that messageTimeout option has never been set, therefore no timeout shall be called*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &twelve, sizeof(twelve));
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_messageTimeout_only_removes_the_message_if_it_has_NULL_callback)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);

    /*send 1 messages that will expire*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, NULL, NULL); /*this is a message with a NULL callback*/
    umock_c_reset_all_calls();

    tickcounter_ms_t twelve = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &twelve, sizeof(twelve));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_messageTimeout_when_exactly_on_the_edge_does_not_call_the_callback) /*because "more"*/
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);

    /*send 1 messages that will expire*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);
    umock_c_reset_all_calls();

    tickcounter_ms_t eleven = 11; /*11 = 10 (receive time) + 1 (timeout) => NO timeout*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &eleven, sizeof(eleven));

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_2_messages_with_timeouts_at_11_and_12_calls_1_timeout) /*test wants to see that message that did not timeout yet do not have their callbacks called*/
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);

    /*send 2 messages that will expire at 12 and 13, both of these messages are send at time=10*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);

    tickcounter_ms_t two = 2;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &two);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)(TEST_DEVICEMESSAGE_HANDLE_2));
    umock_c_reset_all_calls();

    tickcounter_ms_t twelve = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout!!!*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &twelve, sizeof(twelve));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)); /*this is removing the item from waitingToSend*/
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)TEST_DEVICEMESSAGE_HANDLE)); /*calling the callback*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)); /*destroying the message clone*/
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); /*destroying the IOTHUB_MESSAGE_LIST*/

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    /*because we're at time = 12 in this test, the second message is untouched*/

    //act
    IoTHubClientCore_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_2_messages_with_timeouts_at_11_and_12_calls_2_timeouts) /*test wants to see that message that did not timeout yet do not have their callbacks called*/
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);

    /*send 2 messages that will expire at 12 and 13, both of these messages are send at time=10*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);

    tickcounter_ms_t two = 2;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &two);
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)(TEST_DEVICEMESSAGE_HANDLE_2));

    umock_c_reset_all_calls();

    /*we don't care what happens in the Transport, so let's ignore all those calls*/

    tickcounter_ms_t timeIsNow = 12; /*12 > 10 (receive time) + 1 (timeout) => timeout!!!*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)TEST_DEVICEMESSAGE_HANDLE)); /*calling the callback*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);

    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    timeIsNow = 13; /*13 > 10 (receive time) + 2 (timeout) => timeout!!!*/
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow));

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)(TEST_DEVICEMESSAGE_HANDLE_2))); /*calling the callback*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
        .IgnoreArgument(1);

    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));


    /*because we're at time = 13 in this test, the second message times out too*/

    //act
    IoTHubClientCore_LL_DoWork(handle);
    IoTHubClientCore_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_2_messages_one_with_timeout_one_without_call_1_callback) /*test wants to see that message that did not timeout yet do not have their callbacks called*/
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);

    /*send 2 messages that will expire at 12 and 13, both of these messages are send at time=10*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);

    tickcounter_ms_t zero = 0;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &zero); /*essentially no timeout*/
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)(TEST_DEVICEMESSAGE_HANDLE_2));

    umock_c_reset_all_calls();

    /*we don't care what happens in the Transport, so let's ignore all those calls*/

    {/*this scope happen in the first _DoWork call*/
        tickcounter_ms_t timeIsNow = 12000; /*12 > 10 (receive time) + 1 (timeout) => timeout!!!*/
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow));

        STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)) /*this is removing the item from waitingToSend*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, (void*)TEST_DEVICEMESSAGE_HANDLE)); /*calling the callback*/
        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG)) /*destroying the message clone*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)) /*destroying the IOTHUB_MESSAGE_LIST*/
            .IgnoreArgument(1);
    }

    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    {/*this scope happen in the second _DoWork call*/
        tickcounter_ms_t timeIsNow = 999999999UL; /*some very big number*/
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow));
    }
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_DoWork(handle);
    IoTHubClientCore_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_messageTimeout_when_tickcounter_fails_in_do_work_no_timeout_callbacks_are_called) /*test wants to see that message that did not timeout yet do not have their callbacks called*/
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t one = 1;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &one);

    /*send 2 messages that will expire at 12 and 13, both of these messages are send at time=10*/
    /*because sending messages stamps the message's timeout, the call to tickcounter_get_current_ms needs to be here, so the test can says
    "the message has been received at time=10*/
    tickcounter_ms_t ten = 10000;
    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .CopyOutArgumentBuffer(2, &ten, sizeof(ten));
    (void)IoTHubClientCore_LL_SendEventAsync(handle, TEST_DEVICEMESSAGE_HANDLE, test_event_confirmation_callback, (void*)TEST_DEVICEMESSAGE_HANDLE);

    umock_c_reset_all_calls();

    {/*this scope happen in the _DoWork call*/
        tickcounter_ms_t timeIsNow = 12000; /*12 > 10 (receive time) + 1 (timeout) => timeout!!! (well - normally - but here the code doesn't call any callbacks because time cannot be obtained*/
        STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &timeIsNow, sizeof(timeIsNow))
            .SetReturn(MU_FAILURE);
    }

    /*we don't care what happens in the Transport, so let's ignore all those calls*/
    EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG))
        .IgnoreAllCalls();

    //act
    IoTHubClientCore_LL_DoWork(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

#ifndef DONT_USE_UPLOADTOBLOB

TEST_FUNCTION(IoTHubClientCore_LL_UploadToBlob_OK)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Impl(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadToBlob(h, "irrelevantFileName", (const unsigned char*)"a", 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadToBlob_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Impl(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(IOTHUB_CLIENT_ERROR);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadToBlob(h, "irrelevantFileName", (const unsigned char*)"a", 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}


TEST_FUNCTION(IoTHubClientCore_LL_UploadToBlob_with_NULL_handle_fails)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadToBlob(NULL, "irrelevantFileName", (const unsigned char*)"a", 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadToBlob_with_NULL_fileName_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadToBlob(h, NULL, (const unsigned char*)"a", 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadToBlob_with_NULL_source_and_size_greater_than_0_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadToBlob(h, "someFileName.txt", NULL, 1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadMultipleBlocksToBlob_with_NULL_handle_fails)
{
    //arrange
    unsigned int context = 1;

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadMultipleBlocksToBlob(NULL, "irrelevantFileName", my_FileUpload_GetData_Callback, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadMultipleBlocksToBlob_with_NULL_filename_fails)
{
    //arrange
    unsigned int context = 1;
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadMultipleBlocksToBlob(h, NULL, my_FileUpload_GetData_Callback, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadMultipleBlocksToBlob_with_NULL_callback_fails)
{
    //arrange
    unsigned int context = 1;
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadMultipleBlocksToBlob(h, "irrelevantFileName", NULL, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx_succeeds)
{
    //arrange
    unsigned int context = 1;
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(h, "irrelevantFileName", my_FileUpload_GetData_CallbackEx, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx_fails)
{
    //arrange
    unsigned int context = 1;
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(IOTHUB_CLIENT_ERROR);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(h, "irrelevantFileName", my_FileUpload_GetData_CallbackEx, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}


TEST_FUNCTION(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx_with_NULL_handle_fails)
{
    //arrange
    unsigned int context = 1;

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(NULL, "irrelevantFileName", my_FileUpload_GetData_CallbackEx, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx_with_NULL_filename_fails)
{
    //arrange
    unsigned int context = 1;
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(h, NULL, my_FileUpload_GetData_CallbackEx, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx_with_NULL_callback_fails)
{
    //arrange
    unsigned int context = 1;
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(h, "irrelevantFileName", NULL, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

#endif

TEST_FUNCTION(IoTHubClientCore_LL_SendReportedState_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_sendreportedstate_mocks();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(handle, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_Destroy_with_pending_reported_state_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    setup_IoTHubClientCore_LL_sendreportedstate_mocks();
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(handle, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unregister(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/

    STRICT_EXPECTED_CALL(iothub_reported_state_callback(IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG)); /*because there is one item in the list*/

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tickcounter_destroy(IGNORED_PTR_ARG));

#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_Destroy(IGNORED_PTR_ARG));
#endif
#ifdef USE_EDGE_MODULES
    STRICT_EXPECTED_CALL(IoTHubClient_EdgeHandle_Destroy(IGNORED_PTR_ARG));
#endif

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    //act

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_SendReportedState_NULL_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(NULL, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_SendReportedState_reported_state_NULL_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(handle, NULL, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendReportedState_reported_size_0_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();


    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(handle, TEST_REPORTED_STATE, 0, iothub_reported_state_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendReportedState_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_sendreportedstate_mocks();

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_LL_SendReportedState failure in test %zu/%zu", index, count);

            //act
            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(handle, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
    umock_c_negative_tests_deinit();
}


TEST_FUNCTION(IoTHubClientCore_LL_DoWork_SendReportedState_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(h, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); /*_DoWork will ask "what's the time"*/
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_ProcessItem(IGNORED_PTR_ARG, IOTHUB_TYPE_DEVICE_TWIN, IGNORED_PTR_ARG))
        .IgnoreArgument_item_type();
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_DoWork(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DoWork_SendReportedState_continue_processing_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(h, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG)) /*_DoWork will ask "what's the time"*/
        .IgnoreArgument_tick_counter()
        .IgnoreArgument_current_ms();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_ProcessItem(IGNORED_PTR_ARG, IOTHUB_TYPE_DEVICE_TWIN, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument_item_type()
        .IgnoreArgument(3)
        .SetReturn(IOTHUB_PROCESS_CONTINUE);

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DoWork(IGNORED_PTR_ARG));

    //act
    IoTHubClientCore_LL_DoWork(h);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(NULL, deviceMethodCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_with_NULL_callback_Unsubscribe_and_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetDeviceMethodCallback(handle, deviceMethodCallback, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_with_NULL_callback_Unsubscribe_After_SetDeviceMethodCallback_Ex_fail)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(handle, iothub_client_inbound_device_method_callback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_unitialized_with_NULL_callback_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetDeviceMethodCallback(NULL, deviceMethodCallback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(handle, NULL, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_with_non_NULL_succeeds)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(handle, deviceMethodCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_After_SetDeviceMethodCallback_Ex_fail)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(handle, iothub_client_inbound_device_method_callback, (void*)1);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(handle, deviceMethodCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_fails_when_underlying_transport_fails)
{
    ///arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(1);

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(handle, deviceMethodCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_ReportedStateComplete_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendReportedState(h, TEST_REPORTED_STATE, TEST_REPORTED_SIZE, iothub_reported_state_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    IoTHubClientCore_LL_DoWork(h);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothub_reported_state_callback(IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    g_transport_cb_info.twin_rpt_state_complete_cb(2, TEST_DEVICE_STATUS_CODE, h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_ReportedStateComplete_NULL_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    g_transport_cb_info.twin_rpt_state_complete_cb(2, TEST_DEVICE_STATUS_CODE, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodComplete_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(h, deviceMethodCallback, (void*)1);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    size_t len = 1;
    unsigned char* resp = (unsigned char*)my_gballoc_malloc(len);
    resp[0] = 0xa;

    STRICT_EXPECTED_CALL(deviceMethodCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_response(&resp, sizeof(unsigned char**))
        .CopyOutArgumentBuffer_resp_size(&len, sizeof(size_t));

    EXPECTED_CALL(FAKE_IoTHubTransport_DeviceMethod_Response(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int status = g_transport_cb_info.method_complete_cb(TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID, h);

    //assert
    ASSERT_ARE_EQUAL(int, 0, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodComplete_payload_NULL_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback(h, deviceMethodCallback, (void*)1);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    size_t len = 0;
    unsigned char* resp = NULL;

    STRICT_EXPECTED_CALL(deviceMethodCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_method_name()
        .IgnoreArgument_payload()
        .IgnoreArgument_size()
        .IgnoreArgument_userContextCallback()
        .CopyOutArgumentBuffer_response(&resp, sizeof(unsigned char**))
        .CopyOutArgumentBuffer_resp_size(&len, sizeof(size_t));

    //act
    int status = g_transport_cb_info.method_complete_cb(TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID, h);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodComplete_Async_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothub_client_inbound_device_method_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    int status = g_transport_cb_info.method_complete_cb(TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID, h);

    //assert
    ASSERT_ARE_EQUAL(int, 0, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodComplete_No_callback_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    int status = g_transport_cb_info.method_complete_cb(TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID, h);

    //assert
    ASSERT_ARE_EQUAL(int, 0, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodComplete_handle_NULL_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    int status = g_transport_cb_info.method_complete_cb(TEST_METHOD_NAME, (const unsigned char*)TEST_STRING_VALUE, strlen(TEST_STRING_VALUE), TEST_METHOD_ID, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, status);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceTwinCallback_iothubclienthandle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceTwinCallback(NULL, iothub_device_twin_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_RetrievePropertyComplete_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothub_device_twin_callback(DEVICE_TWIN_UPDATE_COMPLETE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));

    //act
    g_transport_cb_info.twin_retrieve_prop_complete_cb(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_RetrievePropertyComplete_iothubclienthandle_NULL_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    g_transport_cb_info.twin_retrieve_prop_complete_cb(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_RetrievePropertyComplete_deviceTwincallback_NULL_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    umock_c_reset_all_calls();

    //act
    g_transport_cb_info.twin_retrieve_prop_complete_cb(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_RetrievePropertyComplete_update_partial_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    //act
    g_transport_cb_info.twin_retrieve_prop_complete_cb(DEVICE_TWIN_UPDATE_PARTIAL, NULL, 0, h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_RetrievePropertyComplete_update_partial_before_complete_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothub_device_twin_callback(DEVICE_TWIN_UPDATE_PARTIAL, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_update_state()
        .IgnoreArgument_userContextCallback()
        .IgnoreArgument_payLoad()
        .IgnoreArgument_size();

    //act
    g_transport_cb_info.twin_retrieve_prop_complete_cb(DEVICE_TWIN_UPDATE_PARTIAL, NULL, 0, h);
    g_transport_cb_info.twin_retrieve_prop_complete_cb(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceTwinCallback_unsubscribe_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe_DeviceTwin(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceTwinCallback(h, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceTwinCallback_subscribe_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceTwin(IGNORED_PTR_ARG))
        .IgnoreArgument_handle();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceTwinCallback_subscribe_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceTwin(IGNORED_PTR_ARG))
        .IgnoreArgument_handle()
        .SetReturn(MU_FAILURE);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceTwinCallback(h, iothub_device_twin_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetTwinAsync_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceTwin(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetTwinAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetTwinAsync(h, iothub_device_twin_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(my_FAKE_IoTHubTransport_GetTwinAsync_callbackContext);

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
    my_gballoc_free(my_FAKE_IoTHubTransport_GetTwinAsync_callbackContext);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetTwinAsync_fail)
{
    //arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceTwin(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetTwinAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetTwinAsync(h, iothub_device_twin_callback, (void*)1);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_LL_GetTwinAsync_NULL_handle_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetTwinAsync(NULL, iothub_device_twin_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_GetTwinAsync_NULL_callback_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetTwinAsync(h, NULL, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_handle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(NULL, iothub_client_inbound_device_method_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_subscribe_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn(MU_FAILURE);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_inbound_device_method_cb_NULL_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Unsubscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(h, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_inbound_device_method_cb_NULL_After_SetDeviceMethodCallback_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetDeviceMethodCallback(h, deviceMethodCallback, (void*)1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(h, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_uninitialized_inbound_device_method_cb_NULL_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(h, NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_After_SetDeviceMethodCallback_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    (void)IoTHubClientCore_LL_SetDeviceMethodCallback(h, deviceMethodCallback, (void*)1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_Subscribe_DeviceMethod(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(h, iothub_client_inbound_device_method_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodResponse_handle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_DeviceMethodResponse(NULL, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodResponse_method_id_NULL_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_DeviceMethodResponse(h, NULL, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodResponse_IoTHubTransport_DeviceMethod_Response_FAILS_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DeviceMethod_Response(IGNORED_PTR_ARG, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE)).SetReturn(MU_FAILURE);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_DeviceMethodResponse(h, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodResponse_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_DeviceMethod_Response(IGNORED_PTR_ARG, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE))
        .IgnoreArgument(1);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_DeviceMethodResponse(h, TEST_METHOD_ID, (const unsigned char*)TEST_DEVICE_METHOD_RESPONSE, strlen(TEST_DEVICE_METHOD_RESPONSE), TEST_DEVICE_STATUS_CODE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}


TEST_FUNCTION(IoTHubClientCore_LL_GetRetryPolicy_NULL_HANDLEParam_fail)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetRetryPolicy(NULL, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_GetRetryPolicy_NULL_RetryPolicyParam_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetRetryPolicy(h, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetRetryPolicy_NULL_SizeParam_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RETRY_POLICY r;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetRetryPolicy(h, &r, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_GetRetryPolicy_succeed)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RETRY_POLICY r;
    size_t s;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_GetRetryPolicy(h, &r, &s);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_product_info_twice_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    PLATFORM_INFO_OPTION supportedPlatformInfo = PLATFORM_INFO_OPTION_DEFAULT;
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_DEFAULT));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_DEFAULT));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");
    if (result == IOTHUB_CLIENT_OK)
    {
        result = IoTHubClientCore_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");
    }

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_product_info_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    PLATFORM_INFO_OPTION supportedPlatformInfo = PLATFORM_INFO_OPTION_DEFAULT;
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_DEFAULT));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_product_info_fails_case2)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    PLATFORM_INFO_OPTION supportedPlatformInfo = PLATFORM_INFO_OPTION_RETRIEVE_SQM;
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_RETRIEVE_SQM));

    //act
    g_fail_platform_get_platform_info = true;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_product_info_fails_case1)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    PLATFORM_INFO_OPTION supportedPlatformInfo = PLATFORM_INFO_OPTION_DEFAULT;
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_GetSupportedPlatformInfo(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(0)
        .CopyOutArgumentBuffer(2, &supportedPlatformInfo, sizeof(supportedPlatformInfo))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(platform_get_platform_info(PLATFORM_INFO_OPTION_DEFAULT));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    g_fail_string_construct_sprintf = true;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_PRODUCT_INFO, "Eight");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_model_id_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_MODEL_ID, "dtmi:YOUR_COMPANY_NAME_HERE:sample_device;1");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_model_id_string_construct_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));

    //act
    g_fail_string_construct = true;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_MODEL_ID, "dtmi:YOUR_COMPANY_NAME_HERE:sample_device;1");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_model_id_twice_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_MODEL_ID, "dtmi:YOUR_COMPANY_NAME_HERE:sample_device;1");
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //act
    result = IoTHubClientCore_LL_SetOption(h, OPTION_MODEL_ID, "dtmi:YOUR_COMPANY_NAME_HERE:sample_device;2");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_diag_sampling_percentage_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    uint32_t diagPercentage = 9;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_DIAGNOSTIC_SAMPLING_PERCENTAGE, &diagPercentage);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetOption_diag_sampling_percentage_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE h = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    uint32_t diagPercentage = 101;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetOption(h, OPTION_DIAGNOSTIC_SAMPLING_PERCENTAGE, &diagPercentage);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(h);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventToOutputAsync_with_NULL_iotHubClientHandle_fails)
{
    //arrange

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventToOutputAsync(NULL, TEST_MESSAGE_HANDLE, TEST_OUTPUT_NAME, test_event_confirmation_callback, (void*)3);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventToOutputAsync_with_NULL_messageHandle_fails)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventToOutputAsync(handle, NULL, TEST_OUTPUT_NAME, test_event_confirmation_callback, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventToOutputAsync_with_NULL_test_event_confirmation_callback_and_non_NULL_context_fails)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventToOutputAsync(handle, TEST_MESSAGE_HANDLE, TEST_OUTPUT_NAME, NULL, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventToOutputAsync_with_NULL_OUTPUT_NAME_fails)
{
    //arrange

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventToOutputAsync(handle, NULL, NULL, test_event_confirmation_callback, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventToOutputAsync_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_SetOutputName(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    setup_IoTHubClientCore_LL_sendeventasync_mocks(false);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventToOutputAsync(handle, TEST_MESSAGE_HANDLE, TEST_OUTPUT_NAME, test_event_confirmation_callback, (void*)1);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendEventToOutputAsync_fails)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    tickcounter_ms_t thisIsNotZero = 312984751;
    (void)IoTHubClientCore_LL_SetOption(handle, "messageTimeout", &thisIsNotZero); /*this forces _SendEventAsync to query the currentTime. If that fails, _SendEvent should fail as well*/
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_SetOutputName(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    setup_IoTHubClientCore_LL_sendeventasync_mocks(true);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "IoTHubClientCore_LL_Create failure in test %zu/%zu", index, count);

            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendEventToOutputAsync(handle, TEST_MESSAGE_HANDLE, TEST_OUTPUT_NAME, test_event_confirmation_callback, (void*)1);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
        }
    }

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
    umock_c_negative_tests_deinit();
}


TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_with_NULL_iotHubClientHandle_fails)
{
    ///arrange

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(NULL, TEST_INPUT_NAME, messageCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_with_NULL_inputName_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, NULL, messageCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

static void setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks()
{
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IotHubTransport_Subscribe_InputQueue(IGNORED_NUM_ARG));
}

static void setup_IoTHubClientCore_LL_SetInputMessageCallback_after_first_invocation_mocks(bool alreadyInList, int depthInList, const char *lastItemInputName)
{
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    while (depthInList > 0)
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn((depthInList == 1) ? lastItemInputName : TEST_STRING_VALUE);
        depthInList--;
    }

    if (alreadyInList == false)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
    }
}

static void setup_IoTHubClientCore_LL_SetInputMessageCallback_callback_null(bool unsubscribeExpected, const char* input_name)
{
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(input_name);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    if (unsubscribeExpected)
    {
        STRICT_EXPECTED_CALL(FAKE_IotHubTransport_Unsubscribe_InputQueue(IGNORED_PTR_ARG));
    }
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_one_item_success)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)1);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_same_input_queue_twice_success)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks();

    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)1);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_SetInputMessageCallback_after_first_invocation_mocks(true, 1, TEST_INPUT_NAME);

    ///act
    result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)2);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

static void setup_three_event_callbacks(IOTHUB_CLIENT_CORE_LL_HANDLE handle)
{
    // Arrange
    setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks();

    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)1);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    setup_IoTHubClientCore_LL_SetInputMessageCallback_after_first_invocation_mocks(false, 1, TEST_STRING_VALUE);
    setup_IoTHubClientCore_LL_SetInputMessageCallback_after_first_invocation_mocks(false, 2, TEST_STRING_VALUE);

    ///act
    result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME2, messageCallback, (void*)2);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME3, messageCallback, (void*)3);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_multiple_items_success)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_three_event_callbacks(handle);

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_one_item_and_unregister_success)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks();

    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)1);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_LL_SetInputMessageCallback_callback_null(true, TEST_INPUT_NAME);

    result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_three_items_and_unregister_each_success)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_three_event_callbacks(handle);
    umock_c_reset_all_calls();

    // Remove TEST_INPUT_NAME2
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME2);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));


    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME2, NULL, NULL);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Remove TEST_INPUT_NAME
    umock_c_reset_all_calls();
    setup_IoTHubClientCore_LL_SetInputMessageCallback_callback_null(false, TEST_INPUT_NAME);

    result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, NULL, NULL);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Remove TEST_INPUT_NAME3
    umock_c_reset_all_calls();
    setup_IoTHubClientCore_LL_SetInputMessageCallback_callback_null(true, TEST_INPUT_NAME3);

    result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME3, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_unregister_not_found_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    setup_three_event_callbacks(handle);
    umock_c_reset_all_calls();

    // Remove TEST_INPUT_NAME2
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG,IGNORED_PTR_ARG,IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME2);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME3);

    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME_NOTFOUND, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallback_one_item_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_LL_SetInputMessageCallback failure in test %zu/%zu", index, count);

            ///act
            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)1);

            ///assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    //
    // We need to de-init and then re-init the test framework due to how IoTHubClientCore_LL_SetInputMessageCallback is implemented,
    // namely after its list is created and non-NULL (but not before) it'll invoke singlylinkedlist_get_head_item.
    //
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();


    negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_NUM_ARG)).IgnoreArgument_psz();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IotHubTransport_Subscribe_InputQueue(IGNORED_NUM_ARG));

    umock_c_negative_tests_snapshot();

    count = umock_c_negative_tests_call_count();
    for (size_t index = 5; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_LL_SetInputMessageCallback failure in test %zu/%zu", index, count);

        ///act
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)1);

        ///assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_with_messageData_NULL_parameter_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();

    //act
    bool result = g_transport_cb_info.msg_input_cb(NULL, handle);
    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_with_unsubscribed_returns_false)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    umock_c_reset_all_calls();


    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_match_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)20);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallback(TEST_MESSAGE_HANDLE, (void*)20));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}


TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_no_match_one_item_in_list_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)21);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME_NOTFOUND);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_match_with_multiple_item_in_list_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)22);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME2, messageCallback, (void*)23);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME3, messageCallback, (void*)24);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME3);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME2);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME3);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallback(TEST_MESSAGE_HANDLE, (void*)24));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}


TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_no_match_with_multiple_item_in_list_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)32);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME2, messageCallback, (void*)33);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME3, messageCallback, (void*)34);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME_NOTFOUND);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME2);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME3);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME2);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME3);

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_overwrite_callback_match_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks();
    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)40);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);
    umock_c_reset_all_calls();

    // Set the same name as before but to a different callback context.
    setup_IoTHubClientCore_LL_SetInputMessageCallback_after_first_invocation_mocks(true, 1, TEST_INPUT_NAME);
    result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)41);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallback(TEST_MESSAGE_HANDLE, (void*)41));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_default_handler_no_registered_inputs_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks();

    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, NULL, messageCallback, (void*)61);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallback(TEST_MESSAGE_HANDLE, (void*)61));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}


TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_default_handler_no_match_on_registered_inputs_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks();
    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)60);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, NULL, messageCallback, (void*)61);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME_NOTFOUND);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME_NOTFOUND);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageCallback(TEST_MESSAGE_HANDLE, (void*)61));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_default_handler_ex_no_registered_inputs_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetMessageCallback_Ex(handle, messageCallbackEx, (void*)70);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_default_handler_ex_no_match_on_registered_inputs_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    setup_IoTHubClientCore_LL_SetInputMessageCallback_first_invocation_mocks();
    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)80);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);


    MESSAGE_INPUT_CALLBACK_CONTEXT messageInputCallbackContext;
    SetMessageInputContextValues(&messageInputCallbackContext, 1, 2, 3, true);
    SetMessageInputContextValues(&expectedContextData, 1, 2, 3, true);

    result_set = IoTHubClientCore_LL_SetInputMessageCallbackEx(handle, TEST_INPUT_NAME, messageInputCallbackEx, &messageInputCallbackContext, sizeof(messageInputCallbackContext));
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME_NOTFOUND);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME_NOTFOUND);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageInputCallbackEx(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG));

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_match_fail)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    IOTHUB_CLIENT_RESULT result_set = IoTHubClientCore_LL_SetInputMessageCallback(handle, TEST_INPUT_NAME, messageCallback, (void*)20);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result_set);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);

    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL)).CallCannotFail();
    STRICT_EXPECTED_CALL(messageCallback(TEST_MESSAGE_HANDLE, (void*)20)).CallCannotFail();
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED)).CallCannotFail();

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_LL_MessageCallbackFromInput failure in test %zu/%zu", index, count);

            bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

            //assert
            ASSERT_ARE_EQUAL(bool, false, result, tmp_msg);
        }
    }

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
    umock_c_negative_tests_deinit();
}

static void IoTHubClientCore_LL_MessageCallbackFromInput_with_messageCallbackEx_calls_client_layer_impl(bool callbackReturnValue)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    MESSAGE_INPUT_CALLBACK_CONTEXT messageInputCallbackContext;
    SetMessageInputContextValues(&messageInputCallbackContext, 1, 2, 3, callbackReturnValue);
    SetMessageInputContextValues(&expectedContextData, 1, 2, 3, callbackReturnValue);

    (void)IoTHubClientCore_LL_SetInputMessageCallbackEx(handle, TEST_INPUT_NAME, messageInputCallbackEx, &messageInputCallbackContext, sizeof(messageInputCallbackContext));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetInputName(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_find(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(TEST_INPUT_NAME);
    STRICT_EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(messageInputCallbackEx(TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG));

    //act
    bool result = g_transport_cb_info.msg_input_cb(TEST_MESSAGE_HANDLE, handle);

    //assert
    if (callbackReturnValue == true)
    {
        ASSERT_IS_TRUE(result);
    }
    else
    {
        ASSERT_IS_FALSE(result);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);

}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_with_messageCallbackEx_calls_client_layer_returns_true_succeeds)
{
    IoTHubClientCore_LL_MessageCallbackFromInput_with_messageCallbackEx_calls_client_layer_impl(true);
}

TEST_FUNCTION(IoTHubClientCore_LL_MessageCallbackFromInput_with_messageCallbackEx_calls_client_layer_returns_false_succeeds)
{
    IoTHubClientCore_LL_MessageCallbackFromInput_with_messageCallbackEx_calls_client_layer_impl(false);
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallbackEx_with_NULL_iotHubClientHandle_fails)
{
    //arrange
    MESSAGE_INPUT_CALLBACK_CONTEXT messageInputCallbackContext;
    SetMessageInputContextValues(&messageInputCallbackContext, 1, 2, 3, true);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallbackEx(NULL, TEST_INPUT_NAME, messageInputCallbackEx, &messageInputCallbackContext, sizeof(messageInputCallbackContext));

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_LL_SetInputMessageCallbackEx_with_NULL_inputName_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);
    MESSAGE_INPUT_CALLBACK_CONTEXT messageInputCallbackContext;
    SetMessageInputContextValues(&messageInputCallbackContext, 1, 2, 3, true);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SetInputMessageCallbackEx(handle, NULL, messageInputCallbackEx, &messageInputCallbackContext, sizeof(messageInputCallbackContext));

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendMessageDisposition_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendMessageDisposition(handle, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendMessageDisposition_negative_test_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED))
        .SetReturn(IOTHUB_CLIENT_ERROR);

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendMessageDisposition(handle, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendMessageDisposition_NULL_message_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_LL_HANDLE handle = IoTHubClientCore_LL_Create(&TEST_CONFIG);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendMessageDisposition(handle, NULL, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
    IoTHubClientCore_LL_Destroy(handle);
}

TEST_FUNCTION(IoTHubClientCore_LL_SendMessageDisposition_NULL_client_fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_LL_SendMessageDisposition(NULL, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

#ifdef USE_EDGE_MODULES

static void set_expected_calls_for_IoTHubClientCore_LL_CreateFromEnvironment_for_EdgeHsm()
{
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(NULL).CallCannotFail(); // don't failure test this path for HSM specifically.
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_SAS_TOKEN_AUTH);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_VAR_DEVICEID);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_VAR_EDGEHOSTNAME);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_VAR_EDGEGATEWAYHOST);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_VAR_MODULEID);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(iothub_security_init(IOTHUB_SECURITY_TYPE_HTTP_EDGE));
    setup_IoTHubClientCore_LL_create_mocks(true, true);

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_TrustBundle(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
#endif

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_IoTHubClientCore_LL_CreateFromEnvironment_for_ConnectionString()
{
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_EDGEHUB_CONNECTIONSTRING);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_EDGEHUB_CACERTIFICATEFILE);

    setup_IoTHubClientCore_LL_createfromconnectionstring_mocks(TEST_DEVICEKEY_TOKEN, TEST_STRING_VALUE, false);

    STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_TrustBundle(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(FAKE_IoTHubTransport_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
#ifndef DONT_USE_UPLOADTOBLOB
    STRICT_EXPECTED_CALL(IoTHubClient_LL_UploadToBlob_SetOption(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
#endif

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

// Tests IoTHubClientCore_LL_CreateFromEnvironment using environment variables when connectiong to an Edge HSM for authentication
TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_succeeds_for_hsm)
{
    //arrange
    set_expected_calls_for_IoTHubClientCore_LL_CreateFromEnvironment_for_EdgeHsm();

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(provideFAKE);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_for_hsm_fails)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_IoTHubClientCore_LL_CreateFromEnvironment_for_EdgeHsm();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(provideFAKE);

            //assert
            ASSERT_IS_NULL(result, "Test %lu failed", (unsigned long)i);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

// Tests IoTHubClientCore_LL_CreateFromEnvironment using environment variables when setting connection string in environment directly.
TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_succeeds_for_connection_string_env)
{
    //arrange
    set_expected_calls_for_IoTHubClientCore_LL_CreateFromEnvironment_for_ConnectionString();

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(provideFAKE);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_for_connection_string_env_fails)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_IoTHubClientCore_LL_CreateFromEnvironment_for_ConnectionString();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(provideFAKE);

            //assert
            ASSERT_IS_NULL(result, "Test %lu failed", (unsigned long)i);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
}


// Tests IoTHubClientCore_LL_CreateFromEnvironment when the requisite environment variables are missing
TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_no_env_set_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(provideFAKE);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}

// Tests IoTHubClientCore_LL_CreateFromEnvironment when connection string set, but the cert file isn't.
TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_set_connection_string_and_missing_certfile_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(TEST_EDGEHUB_CONNECTIONSTRING);
    STRICT_EXPECTED_CALL(environment_get_variable(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(provideFAKE);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_LL_Destroy(result);
}


#endif // USE_EDGE_MODULES


/* Partial refactoring has rendered the below tests invalid - These need to be restored in some capacity once iothub_client refactor is complete */

//
//
//
//TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_succeeds)
//{
//    //arrange
//    set_expected_calls_for_IoTHubClientCore_LL_CreateFromEnvironment();
//
//    //act
//    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(&TEST_CONFIG, TEST_MODULE_ID);
//
//    ///assert
//    ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
//    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
//
//    //cleanup
//    IoTHubClientCore_LL_Destroy(result);
//}
//
//
//TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_config_NULL_fails)
//{
//    //act
//    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(NULL, TEST_MODULE_ID);
//
//    ///assert
//    ASSERT_IS_NULL(result);
//}
//
//TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_module_id_NULL_fails)
//{
//    //act
//    IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(&TEST_CONFIG, NULL);
//
//    ///assert
//    ASSERT_IS_NULL(result);
//}
//
//TEST_FUNCTION(IoTHubClientCore_LL_CreateFromEnvironment_fails)
//{
//    int negativeTestsInitResult = umock_c_negative_tests_init();
//    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);
//
//    umock_c_reset_all_calls();
//
//    set_expected_calls_for_IoTHubClientCore_LL_CreateFromEnvironment();
//
//    umock_c_negative_tests_snapshot();
//
//    // act
//    size_t calls_cannot_fail[] = {
//        1, // STRING_c_str
//        2, // STRING_delete
//        8, // DList_InitializeListHead
//        9, // DList_InitializeListHead
//        10, // DList_InitializeListHead
//        15, // IoTHubClient_LL_UploadToBlob_SetOption
//        16, // gballoc_free
//    };
//    size_t count = umock_c_negative_tests_call_count();
//    for (size_t index = 0; index < count; index++)
//    {
//        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
//        {
//            continue;
//        }
//
//        umock_c_negative_tests_reset();
//        umock_c_negative_tests_fail_call(index);
//
//        char tmp_msg[128];
//        sprintf(tmp_msg, "IoTHubClientCore_LL_Create failure in test %zu/%zu", index, count);
//
//        IOTHUB_CLIENT_CORE_LL_HANDLE result = IoTHubClientCore_LL_CreateFromEnvironment(&TEST_CONFIG, TEST_MODULE_ID);
//
//        //assert
//        ASSERT_ARE_EQUAL(void_ptr, NULL, result, tmp_msg);
//    }
//
//    // cleanup
//    umock_c_negative_tests_deinit();
//}
//

//
//
//

END_TEST_SUITE(iothub_client_core_ll_ut)
