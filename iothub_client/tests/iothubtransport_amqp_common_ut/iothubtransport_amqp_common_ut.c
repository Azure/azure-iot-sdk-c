// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <ctime>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
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
#include "azure_c_shared_utility/shared_util_options.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_c.h"

#ifdef __cplusplus
extern "C"
{
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
            if (saved_malloc_returns[i] == ptr)
            {
                real_free(ptr);
                j++;
            }

            saved_malloc_returns[i] = saved_malloc_returns[j];
        }

        if (i != j) saved_malloc_returns_count--;
    }

    static int TEST_mallocAndStrcpy_s_return;
    static int TEST_mallocAndStrcpy_s(char** destination, const char* source)
    {
        *destination = (char*)source;
        return TEST_mallocAndStrcpy_s_return;
    }

    void* my_gballoc_realloc(void* ptr, size_t size)
    {
        return realloc(ptr, size);
    }

#ifdef __cplusplus
}
#endif


#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "azure_c_shared_utility/socketio.h"

#include "iothub_client_core_ll.h"
#include "iothub_client_options.h"
#include "internal/iothub_client_private.h"
#include "iothub_client_version.h"
#include "internal/iothub_client_retry_control.h"

#undef ENABLE_MOCK_FILTERING_SWITCH
#define ENABLE_MOCK_FILTERING

#define please_mock_amqp_connection_create MOCK_ENABLED
#define please_mock_amqp_connection_destroy MOCK_ENABLED
#define please_mock_amqp_connection_do_work MOCK_ENABLED
#define please_mock_amqp_connection_get_cbs_handle MOCK_ENABLED
#define please_mock_amqp_connection_get_session_handle MOCK_ENABLED
#define please_mock_amqp_connection_set_logging MOCK_ENABLED
#define please_mock_amqp_device_clone_message_disposition_info MOCK_ENABLED
#define please_mock_amqp_device_create MOCK_ENABLED
#define please_mock_amqp_device_delayed_stop MOCK_ENABLED
#define please_mock_amqp_device_destroy MOCK_ENABLED
#define please_mock_amqp_device_destroy_message_disposition_info MOCK_ENABLED
#define please_mock_amqp_device_do_work MOCK_ENABLED
#define please_mock_amqp_device_get_send_status MOCK_ENABLED
#define please_mock_amqp_device_get_twin_async MOCK_ENABLED
#define please_mock_amqp_device_send_event_async MOCK_ENABLED
#define please_mock_amqp_device_send_message_disposition MOCK_ENABLED
#define please_mock_amqp_device_send_twin_update_async MOCK_ENABLED
#define please_mock_amqp_device_set_option MOCK_ENABLED
#define please_mock_amqp_device_start_async MOCK_ENABLED
#define please_mock_amqp_device_stop MOCK_ENABLED
#define please_mock_amqp_device_subscribe_for_twin_updates MOCK_ENABLED
#define please_mock_amqp_device_subscribe_message MOCK_ENABLED
#define please_mock_amqp_device_unsubscribe_for_twin_updates MOCK_ENABLED
#define please_mock_amqp_device_unsubscribe_message MOCK_ENABLED
#define please_mock_amqpvalue_create_map MOCK_ENABLED
#define please_mock_amqpvalue_create_string MOCK_ENABLED
#define please_mock_amqpvalue_create_symbol MOCK_ENABLED
#define please_mock_cbs_create MOCK_ENABLED
#define please_mock_connection_create2 MOCK_ENABLED
#define please_mock_iothubtransportamqp_methods_create MOCK_ENABLED
#define please_mock_iothubtransportamqp_methods_destroy MOCK_ENABLED
#define please_mock_iothubtransportamqp_methods_respond MOCK_ENABLED
#define please_mock_iothubtransportamqp_methods_subscribe MOCK_ENABLED
#define please_mock_iothubtransportamqp_methods_unsubscribe MOCK_ENABLED
#define please_mock_mqp_connection_do_work MOCK_ENABLED
#define please_mock_mqp_connection_get_session_handle MOCK_ENABLED
#define please_mock_mqp_device_destroy MOCK_ENABLED
#define please_mock_mqp_device_get_send_status MOCK_ENABLED
#define please_mock_mqp_device_send_event_async MOCK_ENABLED
#define please_mock_mqp_device_set_option MOCK_ENABLED
#define please_mock_mqp_device_stop MOCK_ENABLED
#define please_mock_mqp_device_unsubscribe_message MOCK_ENABLED
#define please_mock_othubtransportamqp_methods_respond MOCK_ENABLED
#define please_mock_session_create MOCK_ENABLED

#include "azure_uamqp_c/cbs.h"
#include "azure_uamqp_c/amqpvalue.h"
#include "internal/iothubtransportamqp_methods.h"
#include "internal/iothubtransport_amqp_connection.h"
#include "internal/iothubtransport_amqp_device.h"

#undef ENABLE_MOCK_FILTERING_SWITCH
#undef ENABLE_MOCK_FILTERING

#include "internal/iothub_message_private.h"
#include "internal/iothub_transport_ll_private.h"

MOCKABLE_FUNCTION(, bool, Transport_MessageCallbackFromInput, IOTHUB_MESSAGE_HANDLE, message, void*, ctx);
MOCKABLE_FUNCTION(, bool, Transport_MessageCallback, IOTHUB_MESSAGE_HANDLE, message, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_ConnectionStatusCallBack, IOTHUB_CLIENT_CONNECTION_STATUS, status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_SendComplete_Callback, PDLIST_ENTRY, completed, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, ctx);
MOCKABLE_FUNCTION(, const char*, Transport_GetOption_Product_Info_Callback, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_ReportedStateComplete_Callback, uint32_t, item_id, int, status_code, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_RetrievePropertyComplete_Callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, ctx);
MOCKABLE_FUNCTION(, int, Transport_DeviceMethod_Complete_Callback, const char*, method_name, const unsigned char*, payLoad, size_t, size, METHOD_HANDLE, response_id, void*, ctx);

#undef ENABLE_MOCKS

#include "internal/iothubtransport_amqp_common.h"

TEST_DEFINE_ENUM_TYPE(AMQP_CONNECTION_STATE, AMQP_CONNECTION_STATE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(AMQP_CONNECTION_STATE, AMQP_CONNECTION_STATE_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(AMQP_CONNECTION_STATE, AMQP_CONNECTION_STATE_VALUES);

static bool g_failDispositionMake;
static bool g_failDispositionSend;

#ifdef __cplusplus
extern "C"
{
#endif

    extern int real_DList_RemoveEntryList(PDLIST_ENTRY Entry);
    extern void real_DList_InsertTailList(PDLIST_ENTRY ListHead, PDLIST_ENTRY Entry);
    extern int real_DList_IsListEmpty(const PDLIST_ENTRY ListHead);
    extern void real_DList_InitializeListHead(PDLIST_ENTRY ListHead);

    int my_DList_RemoveEntryList(PDLIST_ENTRY Entry)
    {
        return real_DList_RemoveEntryList(Entry);
    }

    void my_DList_InsertTailList(PDLIST_ENTRY ListHead, PDLIST_ENTRY Entry)
    {
        real_DList_InsertTailList(ListHead, Entry);
    }

    int my_DList_IsListEmpty(const PDLIST_ENTRY ListHead)
    {
        return real_DList_IsListEmpty(ListHead);
    }

    void my_DList_InitializeListHead(PDLIST_ENTRY ListHead)
    {
        real_DList_InitializeListHead(ListHead);
    }


    // singlylinkedlist
    static int saved_registered_devices_list_count;
    static const void* saved_registered_devices_list[20];

    static bool TEST_singlylinkedlist_add_fail_return = false;
    static LIST_ITEM_HANDLE TEST_singlylinkedlist_add(SINGLYLINKEDLIST_HANDLE list, const void* item)
    {
        (void)list;
        saved_registered_devices_list[saved_registered_devices_list_count++] = item;

        return TEST_singlylinkedlist_add_fail_return ? NULL : (LIST_ITEM_HANDLE)item;
    }

    static int TEST_singlylinkedlist_remove_return = 0;
    static int TEST_singlylinkedlist_remove(SINGLYLINKEDLIST_HANDLE list, LIST_ITEM_HANDLE item)
    {
        (void)list;
        const void** TEST_list = NULL;
        int* TEST_list_count = NULL;

        TEST_list = saved_registered_devices_list;
        TEST_list_count = &saved_registered_devices_list_count;

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

    static SINGLYLINKEDLIST_HANDLE TEST_singlylinkedlist_foreach_list;
    static LIST_ACTION_FUNCTION TEST_singlylinkedlist_foreach_action_function;
    static const void* TEST_singlylinkedlist_foreach_context;
    static int TEST_singlylinkedlist_foreach(SINGLYLINKEDLIST_HANDLE list, LIST_ACTION_FUNCTION action_function, const void* action_context)
    {
        TEST_singlylinkedlist_foreach_list = list;
        TEST_singlylinkedlist_foreach_action_function = action_function;
        TEST_singlylinkedlist_foreach_context = action_context;
        return 0;
    }

    static const void* TEST_singlylinkedlist_item_get_value(LIST_ITEM_HANDLE item_handle)
    {
        return (const void*)item_handle;
    }

    static LIST_ITEM_HANDLE TEST_singlylinkedlist_get_head_item(SINGLYLINKEDLIST_HANDLE list)
    {
        (void)list;
        LIST_ITEM_HANDLE list_item;

        if (saved_registered_devices_list_count <= 0)
        {
            list_item = NULL;
        }
        else
        {
            list_item = (LIST_ITEM_HANDLE)saved_registered_devices_list[0];
        }

        return list_item;
    }

    static LIST_ITEM_HANDLE TEST_singlylinkedlist_get_next_item(LIST_ITEM_HANDLE item_handle)
    {
        LIST_ITEM_HANDLE next_item = NULL;

        int i;
        int item_found = 0;
        for (i = 0; i < saved_registered_devices_list_count; i++)
        {
            if (item_found)
            {
                next_item = (LIST_ITEM_HANDLE)saved_registered_devices_list[i];
                break;
            }
            else if (saved_registered_devices_list[i] == (void*)item_handle)
            {
                item_found = 1;
            }
        }

        return next_item;
    }


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
            result = __LINE__;
        }
        else
        {
            result = 0;
        }

        return result;
    }

    static STRING_HANDLE STRING_construct_sprintf_result;
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        return STRING_construct_sprintf_result;
    }

    static ON_DEVICE_C2D_MESSAGE_RECEIVED TEST_device_subscribe_message_saved_callback;
    static void* TEST_device_subscribe_message_saved_context;
    static int TEST_device_subscribe_message_return;
    static int TEST_device_subscribe_message(AMQP_DEVICE_HANDLE handle, ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback, void* context)
    {
        (void)handle;
        TEST_device_subscribe_message_saved_callback = on_message_received_callback;
        TEST_device_subscribe_message_saved_context = context;
        return TEST_device_subscribe_message_return;
    }

    static IOTHUB_CLIENT_RESULT TEST_IoTHubClientCore_LL_GetOption(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* optionName, void** value)
    {
        (void)iotHubClientHandle;
        (void)optionName;

        *value = (STRING_HANDLE)0x87;
        return IOTHUB_CLIENT_OK;
    }

#ifdef __cplusplus
}
#endif

MOCKABLE_FUNCTION(, time_t, get_time, time_t*, currentTime);
MOCKABLE_FUNCTION(, double, get_difftime, time_t, stopTime, time_t, startTime);


typedef struct DEVICE_DATA_TAG
{
    AMQP_DEVICE_HANDLE device_handle;
} DEVICE_DATA;


static DEVICE_DATA g_device_state;


typedef struct MESSAGE_DISPOSITION_CONTEXT_TAG
{
    unsigned long message_id;
    char* source;
} MESSAGE_DISPOSITION_CONTEXT;


static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}


// ---------- Control parameters ---------- //

#define INDEFINITE_TIME                            ((time_t)-1)
#define TEST_DEVICE_ID_CHAR_PTR                    "deviceid"
#define TEST_PRODUCT_INFO_CHAR_PTR                 "product info"
#define TEST_DEVICE_ID_2_CHAR_PTR                  "deviceid2"
#define TEST_DEVICE_KEY                            "devicekey"
#define TEST_DEVICE_SAS_TOKEN                      "deviceSas"
#define TEST_IOT_HUB_NAME                          "servername"
#define TEST_IOT_HUB_SUFFIX                        "domainname"
#define TEST_IOT_HUB_PORT                          5671
#define TEST_PROT_GW_HOSTNAME_NULL                 NULL
#define TEST_PROT_GW_HOSTNAME                      "gw"
#define TEST_DEVICE_STATUS_CODE                    200
#define TEST_DEFAULT_SVC2CL_KEEP_ALIVE_FREQ_SECS   240
#define TEST_USER_SVC2CL_KEEP_ALIVE_FREQ_SECS      123
#define TEST_DEFAULT_REMOTE_IDLE_TIMEOUT_RATIO     0.50
#define TEST_USER_REMOTE_IDLE_TIMEOUT_RATIO        0.875
#define DEFAULT_RETRY_POLICY                      IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define DEFAULT_MAX_RETRY_TIME_IN_SECS            0

#define TEST_STRING_HANDLE                         (STRING_HANDLE)0x4240
#define TEST_IOTHUBTRANSPORTAMQP_METHODS           ((IOTHUBTRANSPORT_AMQP_METHODS_HANDLE)0x4244)
#define TEST_SESSION_HANDLE                        ((SESSION_HANDLE)0x4245)
#define TEST_METHOD_HANDLE                         ((IOTHUBTRANSPORT_AMQP_METHOD_HANDLE)0x4246)
#define TEST_XIO_INTERFACE                         ((const IO_INTERFACE_DESCRIPTION*)0x4247)
#define TEST_XIO_HANDLE                            ((XIO_HANDLE)0x4248)
#define TEST_SASL_MECHANISM                        ((SASL_MECHANISM_HANDLE)0x4249)
#define TEST_SASL_MSSBCBS_INTERFACE                ((const SASL_MECHANISM_INTERFACE_DESCRIPTION*)0x4250)
#define TEST_CONNECTION_HANDLE                     ((CONNECTION_HANDLE)0x4251)
#define TEST_CBS_HANDLE                            ((CBS_HANDLE)0x4253)
#define TEST_MESSAGING_SOURCE                      ((AMQP_VALUE)0x4254)
#define TEST_MESSAGING_TARGET                      ((AMQP_VALUE)0x4255)
#define TEST_BUFFER_HANDLE                         ((BUFFER_HANDLE)0x4256)
#define TEST_AMQP_MAP                              ((AMQP_VALUE)0x4258)
#define TEST_AMQP_VALUE                            ((AMQP_VALUE)0x4260)

#define TEST_UNDERLYING_IO_TRANSPORT               ((XIO_HANDLE)0x4261)
#define TEST_TRANSPORT_PROVIDER                    ((TRANSPORT_PROVIDER*)0x4263)
#define TEST_IOTHUB_HOST_FQDN_CHAR_PTR             "servername.domainname"
#define TEST_IOTHUB_HOST_FQDN_STRING_HANDLE        (STRING_HANDLE)0x4264
#define TEST_IOTHUB_HOST_FQDN_CLONE_STRING_HANDLE  (STRING_HANDLE)0x4265
#define TEST_PROTOCOL_PROVIDER                     (IOTHUB_CLIENT_TRANSPORT_PROVIDER)0x4266
#define TEST_REGISTERED_DEVICES_LIST               (SINGLYLINKEDLIST_HANDLE)0x4267
#define TEST_DEVICE_ID_STRING_HANDLE               (STRING_HANDLE)0x4268
#define TEST_DEVICE_HANDLE                         (AMQP_DEVICE_HANDLE)0x4269
#define TEST_LIST_ITEM_HANDLE                      (LIST_ITEM_HANDLE)0x4270
#define TEST_AMQP_CONNECTION_HANDLE                (AMQP_CONNECTION_HANDLE)0x4271
#define TEST_IOTHUB_MESSAGE_LIST_HANDLE            (IOTHUB_MESSAGE_LIST*)0x4272
#define TEST_IOTHUB_DEVICE_HANDLE                  (IOTHUB_DEVICE_HANDLE)0x4273
#define TEST_OPTIONHANDLER_HANDLE                  (OPTIONHANDLER_HANDLE)0x4274
#define TEST_IOTHUB_MESSAGE_HANDLE                 (IOTHUB_MESSAGE_HANDLE)0x4275
#define TEST_X509_CERTIFICATE                      "Ariano Suassuna"
#define TEST_X509_PRIVATE_KEY                      "Raphael Rabello"
#define TEST_MESSAGE_SOURCE_CHAR_PTR               "messagereceiver_link_name"
#define TEST_RETRY_CONTROL_HANDLE                  (RETRY_CONTROL_HANDLE)0x4276

static TRANSPORT_CALLBACKS_INFO transport_cb_info;
static void* transport_cb_ctx = (void*)0x499922;

static const unsigned char* TEST_DEVICE_METHOD_RESPONSE = (const unsigned char*)0x62;
static size_t TEST_DEVICE_RESP_LENGTH = 1;

static const IOTHUB_CLIENT_CORE_LL_HANDLE TEST_IOTHUB_CLIENT_CORE_LL_HANDLE = (IOTHUB_CLIENT_CORE_LL_HANDLE)0x4343;

static time_t TEST_current_time;
static DLIST_ENTRY TEST_waitingToSend;

static unsigned long TEST_MESSAGE_ID;


// ---------- Helpers for Expected Calls ---------- //

// @remarks
//     IHTAC = IoTHubTransport_AMQP_Common
static void set_expected_calls_for_Create(IOTHUBTRANSPORT_CONFIG* transport_config)
{
    STRICT_EXPECTED_CALL(IoTHub_Transport_ValidateCallbacks(IGNORED_PTR_ARG) );
    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(retry_control_create(DEFAULT_RETRY_POLICY, DEFAULT_MAX_RETRY_TIME_IN_SECS));

    if (transport_config->upperConfig->protocolGatewayHostName != NULL)
    {
        STRICT_EXPECTED_CALL(STRING_construct(transport_config->upperConfig->protocolGatewayHostName))
            .SetReturn(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE);
    }
    else if (transport_config->upperConfig->iotHubName != NULL)
    {
        STRING_construct_sprintf_result = TEST_IOTHUB_HOST_FQDN_STRING_HANDLE;
    }

    STRICT_EXPECTED_CALL(singlylinkedlist_create())
        .SetReturn(TEST_REGISTERED_DEVICES_LIST);
}

static void set_expected_calls_for_GetSendStatus(bool is_waiting_to_send_list_empty, DEVICE_SEND_STATUS send_status)
{
    if (!is_waiting_to_send_list_empty)
    {
        STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
            .SetReturn(0)
            .CallCannotFail();
    }
    else
    {
        STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_PTR_ARG))
            .SetReturn(1)
            .CallCannotFail();
        STRICT_EXPECTED_CALL(amqp_device_get_send_status(TEST_DEVICE_HANDLE, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .CopyOutArgumentBuffer(2, &send_status, sizeof(DEVICE_SEND_STATUS))
            .SetReturn(0);
    }
}

static void set_expected_calls_for_GetHostname()
{
    STRICT_EXPECTED_CALL(STRING_clone(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE)).SetReturn(TEST_IOTHUB_HOST_FQDN_CLONE_STRING_HANDLE);
}

static void set_expected_calls_for_is_device_registered_ex(IOTHUB_DEVICE_CONFIG* device_config, IOTHUB_DEVICE_HANDLE registered_device)
{
    (void)device_config;

    STRICT_EXPECTED_CALL(singlylinkedlist_find(TEST_REGISTERED_DEVICES_LIST, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn((LIST_ITEM_HANDLE)registered_device).CallCannotFail();
}

static void set_expected_calls_for_SendMessageDisposition(IOTHUBMESSAGE_DISPOSITION_RESULT iothc_disposition_result, MESSAGE_DISPOSITION_CONTEXT* disposition_info)
{
    DEVICE_MESSAGE_DISPOSITION_RESULT device_disposition_result;

    if (iothc_disposition_result == IOTHUBMESSAGE_ACCEPTED)
    {
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED;
    }
    else if (iothc_disposition_result == IOTHUBMESSAGE_ABANDONED)
    {
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED;
    }
    else if (iothc_disposition_result == IOTHUBMESSAGE_REJECTED)
    {
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED;
    }
    else // unexpected.
    {
        device_disposition_result = DEVICE_MESSAGE_DISPOSITION_RESULT_NONE;
    }

    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_dispositionContext(&disposition_info, sizeof(&disposition_info));
    STRICT_EXPECTED_CALL(amqp_device_send_message_disposition(TEST_DEVICE_HANDLE, IGNORED_PTR_ARG, device_disposition_result));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
}

// @param registered_device
//     provide the handle to the registered device if the expected call is supposed to return an item,
//     or NULL if the intent is to return "not registered".
static void set_expected_calls_for_is_device_registered(IOTHUB_DEVICE_CONFIG* device_config, IOTHUB_DEVICE_HANDLE registered_device)
{
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);

    set_expected_calls_for_is_device_registered_ex(device_config, registered_device);
}

static void set_expected_calls_for_Register(IOTHUB_DEVICE_CONFIG* device_config, bool is_using_cbs)
{
    set_expected_calls_for_is_device_registered_ex(device_config, NULL);

    // is_device_credential_acceptable
    // Nothing to expect.

    EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_construct(device_config->deviceId))
        .SetReturn(TEST_DEVICE_ID_STRING_HANDLE);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE))
        .SetReturn(TEST_IOTHUB_HOST_FQDN_CHAR_PTR).CallCannotFail();
    EXPECTED_CALL(amqp_device_create(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_REGISTERED_DEVICES_LIST))
        .SetReturn(NULL).CallCannotFail();

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE)).SetReturn(TEST_IOTHUB_HOST_FQDN_CHAR_PTR).CallCannotFail();
    EXPECTED_CALL(iothubtransportamqp_methods_create(TEST_IOTHUB_HOST_FQDN_CHAR_PTR, device_config->deviceId, NULL));

    // replicate_device_options_to
    STRICT_EXPECTED_CALL(amqp_device_set_option(TEST_DEVICE_HANDLE, DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, IGNORED_PTR_ARG));

    if (is_using_cbs)
    {
        STRICT_EXPECTED_CALL(amqp_device_set_option(TEST_DEVICE_HANDLE, DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(singlylinkedlist_add(TEST_REGISTERED_DEVICES_LIST, IGNORED_PTR_ARG));
}

static void set_expected_calls_for_Unregister(IOTHUB_DEVICE_HANDLE iothub_device_handle)
{
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(TEST_REGISTERED_DEVICES_LIST, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn((LIST_ITEM_HANDLE)iothub_device_handle);

    STRICT_EXPECTED_CALL(singlylinkedlist_remove(TEST_REGISTERED_DEVICES_LIST, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(iothubtransportamqp_methods_destroy(TEST_IOTHUBTRANSPORTAMQP_METHODS));

    STRICT_EXPECTED_CALL(amqp_device_destroy(TEST_DEVICE_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_DEVICE_ID_STRING_HANDLE));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_establish_amqp_connection()
{
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE))
        .SetReturn(TEST_IOTHUB_HOST_FQDN_CHAR_PTR);
    EXPECTED_CALL(amqp_connection_create(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_subscribe_methods()
{
    STRICT_EXPECTED_CALL(amqp_connection_get_session_handle(TEST_AMQP_CONNECTION_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument_session_handle();

    STRICT_EXPECTED_CALL(iothubtransportamqp_methods_subscribe(TEST_IOTHUBTRANSPORTAMQP_METHODS, TEST_SESSION_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_on_methods_error()
        .IgnoreArgument_on_methods_error_context()
        .IgnoreArgument_on_method_request_received()
        .IgnoreArgument_on_method_request_received_context()
        .IgnoreArgument_on_methods_unsubscribed()
        .IgnoreArgument_on_methods_unsubscribed_context();
}

static void set_expected_calls_for_on_methods_unsubscribed()
{
    STRICT_EXPECTED_CALL(iothubtransportamqp_methods_unsubscribe(TEST_IOTHUBTRANSPORTAMQP_METHODS));
}

static void set_expected_calls_for_send_pending_events(PDLIST_ENTRY wts, int expected_number_of_events)
{
    int i;
    for (i = 0; i < expected_number_of_events; i++)
    {
        STRICT_EXPECTED_CALL(DList_IsListEmpty(wts));
        EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(amqp_device_send_event_async(TEST_DEVICE_HANDLE, TEST_IOTHUB_MESSAGE_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
    }

    STRICT_EXPECTED_CALL(DList_IsListEmpty(wts))
        .SetReturn(1);
}

static void set_expected_calls_for_Device_DoWork(PDLIST_ENTRY wts, int wts_length, DEVICE_STATE current_device_state, bool is_using_cbs, time_t current_time, bool subscribe_for_methods)
{
    if (current_device_state == DEVICE_STATE_STOPPED)
    {
        STRICT_EXPECTED_CALL(amqp_connection_get_session_handle(TEST_AMQP_CONNECTION_HANDLE, IGNORED_PTR_ARG))
            .IgnoreArgument_session_handle();

        if (is_using_cbs)
        {
            STRICT_EXPECTED_CALL(amqp_connection_get_cbs_handle(TEST_AMQP_CONNECTION_HANDLE, IGNORED_PTR_ARG))
                .IgnoreArgument_cbs_handle();

            STRICT_EXPECTED_CALL(amqp_device_start_async(TEST_DEVICE_HANDLE, TEST_SESSION_HANDLE, TEST_CBS_HANDLE));
        }
        else
        {
            STRICT_EXPECTED_CALL(amqp_device_start_async(TEST_DEVICE_HANDLE, TEST_SESSION_HANDLE, IGNORED_PTR_ARG));
        }
    }
    else if (current_device_state == DEVICE_STATE_STARTING ||
        current_device_state == DEVICE_STATE_STOPPING)
    {
        // is_timeout_reached
        STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(current_time);
        EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    }
    else if (current_device_state == DEVICE_STATE_ERROR_AUTH || current_device_state == DEVICE_STATE_ERROR_MSG)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE))
            .SetReturn(TEST_DEVICE_ID_CHAR_PTR);

        STRICT_EXPECTED_CALL(amqp_device_stop(TEST_DEVICE_HANDLE));
    }
    else if (current_device_state == DEVICE_STATE_STARTED)
    {
        if (subscribe_for_methods)
        {
            set_expected_calls_for_subscribe_methods();
        }

        set_expected_calls_for_send_pending_events(wts, wts_length);
    }

    STRICT_EXPECTED_CALL(amqp_device_do_work(TEST_DEVICE_HANDLE));
}

static void set_expected_calls_for_get_new_underlying_io_transport(bool feed_options)
{
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE))
        .SetReturn(TEST_IOTHUB_HOST_FQDN_CHAR_PTR);
    //STRICT_EXPECTED_CALL(IoTHubClient_Auth_Get_Credential_Type(IGNORED_PTR_ARG));

    if (feed_options)
    {
        STRICT_EXPECTED_CALL(OptionHandler_FeedOptions(TEST_OPTIONHANDLER_HANDLE, TEST_UNDERLYING_IO_TRANSPORT))
            .SetReturn(OPTIONHANDLER_OK);
    }
}

static void set_expected_calls_for_DoWork2(PDLIST_ENTRY wts, int wts_length, DEVICE_STATE current_device_state, bool is_tls_io_acquired, bool feed_options, bool is_using_cbs, bool is_connection_created, bool is_connection_open, int number_of_registered_devices, time_t current_time, bool subscribe_for_methods)
{
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_REGISTERED_DEVICES_LIST));

    if (!is_tls_io_acquired)
    {
        set_expected_calls_for_get_new_underlying_io_transport(feed_options);
    }

    if (!is_connection_created)
    {
        set_expected_calls_for_establish_amqp_connection();

        is_connection_created = true;
    }

    if (is_connection_open)
    {
        int i;
        for (i = 0; i < number_of_registered_devices; i++)
        {
            EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
            set_expected_calls_for_Device_DoWork(wts, wts_length, current_device_state, is_using_cbs, current_time, subscribe_for_methods);
            EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));
        }
    }

    if (is_connection_created)
    {
        STRICT_EXPECTED_CALL(amqp_connection_do_work(TEST_AMQP_CONNECTION_HANDLE));
    }
}

static void set_expected_calls_for_DoWork(PDLIST_ENTRY wts, int wts_length, DEVICE_STATE current_device_state, bool is_tls_io_acquired, bool is_using_cbs, bool is_connection_created, bool is_connection_open, int number_of_registered_devices, time_t current_time, bool subscribe_for_methods)
{
    set_expected_calls_for_DoWork2(wts, wts_length, current_device_state, is_tls_io_acquired, false /* feed_options */, is_using_cbs, is_connection_created, is_connection_open, number_of_registered_devices, current_time, subscribe_for_methods);
}

static void set_expected_calls_for_Destroy(int number_of_registered_devices, IOTHUB_DEVICE_HANDLE* registered_devices)
{
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_REGISTERED_DEVICES_LIST));

    int i;
    for (i = 0; i < number_of_registered_devices; i++)
    {
        EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));
        set_expected_calls_for_Unregister(registered_devices[i]);
    }

    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_REGISTERED_DEVICES_LIST));
    STRICT_EXPECTED_CALL(amqp_connection_destroy(TEST_AMQP_CONNECTION_HANDLE));
    STRICT_EXPECTED_CALL(xio_destroy(TEST_UNDERLYING_IO_TRANSPORT));
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));
    EXPECTED_CALL(free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_Subscribe(IOTHUB_DEVICE_CONFIG* device_config, IOTHUB_DEVICE_HANDLE registered_device)
{
    set_expected_calls_for_is_device_registered(device_config, registered_device);

    STRICT_EXPECTED_CALL(amqp_device_subscribe_message(TEST_DEVICE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);
}

static void set_expected_calls_for_Unsubscribe(IOTHUB_DEVICE_CONFIG* device_config, IOTHUB_DEVICE_HANDLE registered_device)
{
    set_expected_calls_for_is_device_registered(device_config, registered_device);

    STRICT_EXPECTED_CALL(amqp_device_unsubscribe_message(TEST_DEVICE_HANDLE));
}

static void set_expected_calls_for_prepare_device_for_connection_retry(DEVICE_STATE current_device_state)
{
    set_expected_calls_for_on_methods_unsubscribed();

    if (current_device_state != DEVICE_STATE_STOPPED)
    {
        STRICT_EXPECTED_CALL(amqp_device_stop(TEST_DEVICE_HANDLE));
    }
}

static void set_expected_calls_for_prepare_for_connection_retry(int number_of_registered_devices, DEVICE_STATE current_device_state)
{
    RETRY_ACTION retry_action = RETRY_ACTION_RETRY_NOW;
    STRICT_EXPECTED_CALL(retry_control_should_retry(TEST_RETRY_CONTROL_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(RETRY_ACTION));

    STRICT_EXPECTED_CALL(xio_retrieveoptions(TEST_UNDERLYING_IO_TRANSPORT))
        .SetReturn(TEST_OPTIONHANDLER_HANDLE);

    EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_PTR_ARG));

    int i;
    for (i = 0; i < number_of_registered_devices; i++)
    {
        EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
        set_expected_calls_for_prepare_device_for_connection_retry(current_device_state);
        EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(amqp_connection_destroy(TEST_AMQP_CONNECTION_HANDLE));
    STRICT_EXPECTED_CALL(xio_destroy(TEST_UNDERLYING_IO_TRANSPORT));
}

// ---------- Test Hooks ---------- //
static STRING_HANDLE TEST_STRING_construct_sprintf(const char* format, ...)
{
    (void)format;
    return NULL;
}

static ON_METHODS_ERROR g_on_methods_error;
static void* g_on_methods_error_context;
static ON_METHOD_REQUEST_RECEIVED g_on_method_request_received;
static void* g_on_method_request_received_context;
static ON_METHODS_UNSUBSCRIBED g_on_methods_unsubscribed;
static void* g_on_methods_unsubscribed_context;

static int my_iothubtransportamqp_methods_subscribe(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle,
    SESSION_HANDLE session_handle, ON_METHODS_ERROR on_methods_error, void* on_methods_error_context,
    ON_METHOD_REQUEST_RECEIVED on_method_request_received, void* on_method_request_received_context,
    ON_METHODS_UNSUBSCRIBED on_methods_unsubscribed, void* on_methods_unsubscribed_context)
{
    (void)iothubtransport_amqp_methods_handle, (void)session_handle;
    g_on_methods_error = on_methods_error;
    g_on_methods_error_context = on_methods_error_context;
    g_on_method_request_received = on_method_request_received;
    g_on_method_request_received_context = on_method_request_received_context;
    g_on_methods_unsubscribed = on_methods_unsubscribed;
    g_on_methods_unsubscribed_context = on_methods_unsubscribed_context;
    return 0;
}

static bool TEST_amqp_connection_create_saved_create_sasl_io;
static bool TEST_amqp_connection_create_saved_create_cbs_connection;
static ON_AMQP_CONNECTION_STATE_CHANGED TEST_amqp_connection_create_saved_on_state_changed_callback;
static const void* TEST_amqp_connection_create_saved_on_state_changed_context;
static size_t TEST_amqp_connection_create_saved_c2d_keep_alive_freq_secs;
static double TEST_amqp_connection_create_saved_cl2svc_keep_alive_send_ratio;
static AMQP_CONNECTION_HANDLE TEST_amqp_connection_create_return;
static AMQP_CONNECTION_HANDLE TEST_amqp_connection_create(AMQP_CONNECTION_CONFIG* config)
{
    TEST_amqp_connection_create_saved_create_sasl_io = config->create_sasl_io;
    TEST_amqp_connection_create_saved_create_cbs_connection = config->create_cbs_connection;
    TEST_amqp_connection_create_saved_on_state_changed_callback = config->on_state_changed_callback;
    TEST_amqp_connection_create_saved_on_state_changed_context = config->on_state_changed_context;
    TEST_amqp_connection_create_saved_c2d_keep_alive_freq_secs = config->svc2cl_keep_alive_timeout_secs;
    TEST_amqp_connection_create_saved_cl2svc_keep_alive_send_ratio = config->cl2svc_keep_alive_send_ratio;

    return TEST_amqp_connection_create_return;
}

static SESSION_HANDLE TEST_amqp_connection_get_session_handle_session_handle;
static int TEST_amqp_connection_get_session_handle_return;
static int TEST_amqp_connection_get_session_handle(AMQP_CONNECTION_HANDLE conn_handle, SESSION_HANDLE* session_handle)
{
    (void)conn_handle;
    *session_handle = TEST_amqp_connection_get_session_handle_session_handle;

    return TEST_amqp_connection_get_session_handle_return;
}

static CBS_HANDLE TEST_amqp_connection_get_cbs_handle_cbs_handle;
static int TEST_amqp_connection_get_cbs_handle_return;
static int TEST_amqp_connection_get_cbs_handle(AMQP_CONNECTION_HANDLE conn_handle, CBS_HANDLE* cbs_handle)
{
    (void)conn_handle;
    *cbs_handle = TEST_amqp_connection_get_cbs_handle_cbs_handle;

    return TEST_amqp_connection_get_cbs_handle_return;
}

static double TEST_get_difftime(time_t t1, time_t t0)
{
    return difftime(t1, t0);
}

static ON_DEVICE_STATE_CHANGED TEST_device_create_saved_on_state_changed_callback;
static void* TEST_device_create_saved_on_state_changed_context;
static AMQP_DEVICE_HANDLE TEST_device_create_return;
static AMQP_DEVICE_HANDLE TEST_device_create(AMQP_DEVICE_CONFIG* config)
{
    TEST_device_create_saved_on_state_changed_callback = config->on_state_changed_callback;
    TEST_device_create_saved_on_state_changed_context = config->on_state_changed_context;
    return TEST_device_create_return;
}


// ---------- Test Helpers ---------- //
static const TRANSPORT_PROVIDER* TEST_get_iothub_client_transport_provider(void)
{
    return TEST_TRANSPORT_PROVIDER;
}

static XIO_HANDLE TEST_amqp_get_io_transport_result;
static AMQP_TRANSPORT_PROXY_OPTIONS* expected_AMQP_TRANSPORT_PROXY_OPTIONS;
static int error_proxy_options;
static XIO_HANDLE TEST_amqp_get_io_transport(const char* target_fqdn, const AMQP_TRANSPORT_PROXY_OPTIONS* amqp_transport_proxy_options)
{
    (void)target_fqdn;
    (void)amqp_transport_proxy_options;
    error_proxy_options = 0;
    if (((expected_AMQP_TRANSPORT_PROXY_OPTIONS == NULL) && (amqp_transport_proxy_options != NULL)) ||
        ((expected_AMQP_TRANSPORT_PROXY_OPTIONS != NULL) && (amqp_transport_proxy_options == NULL)))
    {
        error_proxy_options = 1;
    }
    else
    {
        if (expected_AMQP_TRANSPORT_PROXY_OPTIONS != NULL)
        {
            if ((strcmp(expected_AMQP_TRANSPORT_PROXY_OPTIONS->host_address, amqp_transport_proxy_options->host_address) != 0) ||
                ((expected_AMQP_TRANSPORT_PROXY_OPTIONS->username != NULL) && (amqp_transport_proxy_options->username != NULL) && (strcmp(expected_AMQP_TRANSPORT_PROXY_OPTIONS->username, amqp_transport_proxy_options->username) != 0)) ||
                ((expected_AMQP_TRANSPORT_PROXY_OPTIONS->password != NULL) && (amqp_transport_proxy_options->password != NULL) && (strcmp(expected_AMQP_TRANSPORT_PROXY_OPTIONS->password, amqp_transport_proxy_options->password) != 0)))
            {
                error_proxy_options = 1;
            }
        }
    }
    return TEST_amqp_get_io_transport_result;
}

static IOTHUB_CLIENT_CONFIG* create_client_config()
{
    static IOTHUB_CLIENT_CONFIG client_config;
    client_config.protocol = TEST_PROTOCOL_PROVIDER;
    client_config.deviceId = TEST_DEVICE_ID_CHAR_PTR;
    client_config.deviceKey = TEST_DEVICE_KEY;
    client_config.deviceSasToken = TEST_DEVICE_SAS_TOKEN;
    client_config.iotHubName = TEST_IOT_HUB_NAME;
    client_config.iotHubSuffix = TEST_IOT_HUB_SUFFIX;
    client_config.protocolGatewayHostName = TEST_PROT_GW_HOSTNAME;
    return &client_config;
}

static IOTHUBTRANSPORT_CONFIG* create_transport_config()
{
    static IOTHUBTRANSPORT_CONFIG config;
    config.upperConfig = create_client_config();
    config.waitingToSend = NULL;
    return &config;
}

static IOTHUB_DEVICE_CONFIG* create_device_config(const char* device_id, bool use_device_key)
{
    static IOTHUB_DEVICE_CONFIG device_config;
    device_config.deviceId = device_id;

    if (use_device_key)
    {
        device_config.deviceKey = TEST_DEVICE_KEY;
        device_config.deviceSasToken = NULL;
    }
    else
    {
        device_config.deviceKey = NULL;
        device_config.deviceSasToken = TEST_DEVICE_SAS_TOKEN;
    }

    device_config.moduleId = NULL;

    return &device_config;
}

static IOTHUB_DEVICE_CONFIG* create_device_config_for_x509(const char* device_id)
{
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(device_id, false);
    device_config->deviceSasToken = NULL;
    return device_config;
}

static TRANSPORT_LL_HANDLE create_transport()
{
    IOTHUBTRANSPORT_CONFIG* config = create_transport_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_Create(config);
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_AMQP_Common_Create(config, TEST_amqp_get_io_transport, &transport_cb_info, transport_cb_ctx);

    return handle;
}

static void crank_transport(void* handle, PDLIST_ENTRY wts, int wts_length, DEVICE_STATE current_device_state, bool is_tls_io_acquired, bool is_using_cbs, bool is_connection_created, bool is_connection_open, int number_of_registered_devices, time_t current_time, bool subscribe_for_methods)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork(wts, wts_length, current_device_state, is_tls_io_acquired, is_using_cbs, is_connection_created, is_connection_open, number_of_registered_devices, current_time, subscribe_for_methods);
    (void)IoTHubTransport_AMQP_Common_DoWork(handle);
}

static void crank_transport_ready_after_create(void* handle, PDLIST_ENTRY wts, int wts_length, bool is_tls_io_acquired, bool is_using_cbs, int number_of_registered_devices, time_t current_time, bool subscribe_for_methods)
{
    crank_transport(handle, wts, wts_length, DEVICE_STATE_STOPPED, is_tls_io_acquired, is_using_cbs, false, false, number_of_registered_devices, current_time, false);

    TEST_amqp_connection_create_saved_on_state_changed_callback(
        TEST_amqp_connection_create_saved_on_state_changed_context,
        AMQP_CONNECTION_STATE_CLOSED, AMQP_CONNECTION_STATE_OPENED);

    crank_transport(handle, wts, wts_length, DEVICE_STATE_STOPPED, true, is_using_cbs, true, true, number_of_registered_devices, current_time, false);

    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    STRICT_EXPECTED_CALL(retry_control_reset(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, IGNORED_PTR_ARG));

    TEST_device_create_saved_on_state_changed_callback(TEST_device_create_saved_on_state_changed_context,
        DEVICE_STATE_STOPPED, DEVICE_STATE_STARTED);

    crank_transport(handle, wts, wts_length, DEVICE_STATE_STARTED, true, is_using_cbs, true, true, number_of_registered_devices, current_time, subscribe_for_methods);
}

static IOTHUB_DEVICE_HANDLE register_device(TRANSPORT_LL_HANDLE handle, IOTHUB_DEVICE_CONFIG* device_config, PDLIST_ENTRY wts, bool is_using_cbs)
{
    umock_c_reset_all_calls();
    set_expected_calls_for_Register(device_config, is_using_cbs);
    return IoTHubTransport_AMQP_Common_Register(handle, device_config, wts);
}

static void destroy_transport(TRANSPORT_LL_HANDLE handle, IOTHUB_DEVICE_HANDLE registered_device0, IOTHUB_DEVICE_HANDLE registered_device1)
{
    int number_of_registered_devices = (registered_device1 != NULL ? 2 : (registered_device0 != NULL ? 1 : 0));

    IOTHUB_DEVICE_HANDLE registered_devices[2];
    registered_devices[0] = registered_device0;
    registered_devices[1] = registered_device1;

    umock_c_reset_all_calls();
    set_expected_calls_for_Destroy(number_of_registered_devices, registered_devices);
    IoTHubTransport_AMQP_Common_Destroy(handle);
}

static DEVICE_TWIN_UPDATE_STATE get_twin_update_type;
static const unsigned char* get_twin_message;
static size_t get_twin_length;
static void* get_twin_context;
static void on_device_get_twin_completed_callback(DEVICE_TWIN_UPDATE_STATE update_type, const unsigned char* message, size_t length, void* context)
{
    get_twin_update_type = update_type;
    get_twin_message = message;
    get_twin_length = length;
    get_twin_context = context;
}

// ---------- Test Initialization Helpers ---------- //
static void register_umock_alias_types()
{
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_CONNECTION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CBS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CONNECTION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_DEVICE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_MESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_SEND_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_CONTENT_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBTRANSPORT_AMQP_METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_DISPOSITION_CONTEXT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ACTION_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_MATCH_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_AMQP_CONNECTION_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_CONNECTION_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DEVICE_C2D_MESSAGE_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DEVICE_D2C_EVENT_SEND_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DEVICE_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_IO_ERROR, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_LINK_ATTACHED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_METHODS_UNSUBSCRIBED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_NEW_ENDPOINT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(OPTIONHANDLER_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(RETRY_CONTROL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(fields, void*);
    REGISTER_UMOCK_ALIAS_TYPE(role, bool);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_METHOD_REQUEST_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_METHODS_ERROR, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_TWIN_UPDATE_RECEIVED_CALLBACK, void*);
}

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, TEST_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, TEST_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_realloc, my_gballoc_realloc);
    REGISTER_GLOBAL_MOCK_HOOK(iothubtransportamqp_methods_subscribe, my_iothubtransportamqp_methods_subscribe);

    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_add, TEST_singlylinkedlist_add);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_remove, TEST_singlylinkedlist_remove);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_head_item, TEST_singlylinkedlist_get_head_item);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_get_next_item, TEST_singlylinkedlist_get_next_item);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_find, TEST_singlylinkedlist_find);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_foreach, TEST_singlylinkedlist_foreach);
    REGISTER_GLOBAL_MOCK_HOOK(singlylinkedlist_item_get_value, TEST_singlylinkedlist_item_get_value);

    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveEntryList, my_DList_RemoveEntryList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertTailList, my_DList_InsertTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_IsListEmpty, my_DList_IsListEmpty);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InitializeListHead, my_DList_InitializeListHead);

    REGISTER_GLOBAL_MOCK_HOOK(amqp_connection_create, TEST_amqp_connection_create);
    REGISTER_GLOBAL_MOCK_HOOK(amqp_connection_get_session_handle, TEST_amqp_connection_get_session_handle);
    REGISTER_GLOBAL_MOCK_HOOK(amqp_connection_get_cbs_handle, TEST_amqp_connection_get_cbs_handle);

    REGISTER_GLOBAL_MOCK_HOOK(get_difftime, TEST_get_difftime);

    REGISTER_GLOBAL_MOCK_HOOK(amqp_device_create, TEST_device_create);
    REGISTER_GLOBAL_MOCK_HOOK(amqp_device_subscribe_message, TEST_device_subscribe_message);

    REGISTER_GLOBAL_MOCK_RETURN(Transport_GetOption_Product_Info_Callback, TEST_PRODUCT_INFO_CHAR_PTR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_GetOption_Product_Info_Callback, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, TEST_mallocAndStrcpy_s);
}

static void register_global_mock_returns()
{
    REGISTER_GLOBAL_MOCK_RETURN(iothubtransportamqp_methods_create, TEST_IOTHUBTRANSPORTAMQP_METHODS);
    REGISTER_GLOBAL_MOCK_RETURN(session_create, TEST_SESSION_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_XIO_INTERFACE);
    REGISTER_GLOBAL_MOCK_RETURN(xio_create, TEST_XIO_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(connection_create2, TEST_CONNECTION_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(cbs_create, TEST_CBS_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_new, TEST_BUFFER_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_map, TEST_AMQP_MAP);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_symbol, TEST_AMQP_VALUE);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_string, TEST_AMQP_VALUE);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_device_start_async, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_device_start_async, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_device_stop, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_device_stop, 1);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHub_Transport_ValidateCallbacks, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHub_Transport_ValidateCallbacks, __LINE__);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_device_set_option, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_device_set_option, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_device_subscribe_message, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_device_subscribe_message, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_device_unsubscribe_message, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_device_unsubscribe_message, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_device_get_send_status, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_device_get_send_status, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_device_send_message_disposition, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_device_send_message_disposition, 1);

    REGISTER_GLOBAL_MOCK_RETURN(amqp_device_get_twin_async, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqp_device_get_twin_async, 1);

    REGISTER_GLOBAL_MOCK_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(OptionHandler_FeedOptions, OPTIONHANDLER_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_create, TEST_RETRY_CONTROL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_create, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(retry_control_set_option, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(retry_control_set_option, 1);
}

static void reset_test_data()
{
    g_STRING_sprintf_call_count = 0;
    g_STRING_sprintf_fail_on_count = 0;

    STRING_construct_sprintf_result = NULL;
    saved_STRING_sprintf_handle = NULL;

    TEST_amqp_get_io_transport_result = TEST_UNDERLYING_IO_TRANSPORT;

    TEST_amqp_connection_create_saved_on_state_changed_callback = NULL;
    TEST_amqp_connection_create_saved_on_state_changed_context = NULL;
    TEST_amqp_connection_create_return = TEST_AMQP_CONNECTION_HANDLE;

    TEST_amqp_connection_get_session_handle_session_handle = TEST_SESSION_HANDLE;
    TEST_amqp_connection_get_session_handle_return = 0;

    TEST_amqp_connection_get_cbs_handle_cbs_handle = TEST_CBS_HANDLE;
    TEST_amqp_connection_get_cbs_handle_return = 0;

    TEST_device_create_saved_on_state_changed_callback = NULL;
    TEST_device_create_saved_on_state_changed_context = NULL;
    TEST_device_create_return = TEST_DEVICE_HANDLE;

    saved_registered_devices_list_count = 0;

    TEST_device_subscribe_message_saved_callback = NULL;
    TEST_device_subscribe_message_saved_context = NULL;
    TEST_device_subscribe_message_return = 0;

    TEST_MESSAGE_ID = 1234;
    TEST_mallocAndStrcpy_s_return = 0;

    TEST_singlylinkedlist_foreach_list = NULL;
    TEST_singlylinkedlist_foreach_action_function = NULL;
    TEST_singlylinkedlist_foreach_context = NULL;

    memset(&TEST_waitingToSend, 0, sizeof(TEST_waitingToSend));

    g_on_methods_error_context = NULL;
    g_on_method_request_received_context = NULL;
    g_on_methods_unsubscribed_context = NULL;
    expected_AMQP_TRANSPORT_PROXY_OPTIONS = NULL;
}

static void initialize_test_variables()
{
    TEST_current_time = time(NULL);
    ASSERT_IS_TRUE(INDEFINITE_TIME != TEST_current_time, "Failed setting TEST_current_time");

    real_DList_InitializeListHead(&TEST_waitingToSend);
}


BEGIN_TEST_SUITE(iothubtransport_amqp_common_ut)

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

    register_umock_alias_types();
    register_global_mock_hooks();
    register_global_mock_returns();

    transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
    transport_cb_info.twin_retrieve_prop_complete_cb = Transport_Twin_RetrievePropertyComplete_Callback;
    transport_cb_info.twin_rpt_state_complete_cb = Transport_Twin_ReportedStateComplete_Callback;
    transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
    transport_cb_info.connection_status_cb = Transport_ConnectionStatusCallBack;
    transport_cb_info.prod_info_cb = Transport_GetOption_Product_Info_Callback;
    transport_cb_info.msg_input_cb = Transport_MessageCallbackFromInput;
    transport_cb_info.msg_cb = Transport_MessageCallback;
    transport_cb_info.method_complete_cb = Transport_DeviceMethod_Complete_Callback;
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

/* IoTHubTransport_AMQP_Common_Register */

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Register_creates_a_new_methods_handler)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    initialize_test_variables();

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.moduleId = NULL;
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;

    umock_c_reset_all_calls();
    set_expected_calls_for_Register(&device_config, true);

    // act
    device_handle = IoTHubTransport_AMQP_Common_Register(handle, &device_config, &TEST_waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(device_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(when_creating_the_methods_handler_fails_then_IoTHubTransport_AMQP_Common_Register_fails)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle = NULL;

    initialize_test_variables();

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.moduleId = NULL;
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;

    umock_c_reset_all_calls();
    set_expected_calls_for_Register(&device_config, true);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    size_t n = umock_c_negative_tests_call_count();
    for (i = 0; i < n; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            // arrange
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            device_handle = IoTHubTransport_AMQP_Common_Register(handle, &device_config, &TEST_waitingToSend);

            // assert
            ASSERT_IS_NULL(device_handle, "On failed call %lu", (unsigned long)i);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Unregister_destroys_the_methods_handler)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    initialize_test_variables();

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_Unregister(device_handle);

    // act
    IoTHubTransport_AMQP_Common_Unregister(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

/* IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod */

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod_with_NULL_handle_fails)
{
    // arrange
    int result;

    // act
    result = IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod_with_valid_handle_succeeds)
{
    // arrange
    int result;

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    initialize_test_variables();

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    umock_c_reset_all_calls();

    // act
    result = IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod_After_Subscribe_succeeds)
{
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(on_methods_unsubscribed_CALLS_iothubtransportamqp_methods_unsubscribe)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    ASSERT_IS_NOT_NULL(g_on_methods_unsubscribed);
    ASSERT_IS_NOT_NULL(g_on_methods_unsubscribed_context);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_methods_unsubscribed();

    // act
    g_on_methods_unsubscribed(g_on_methods_unsubscribed_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(on_methods_unsubscribed_re_subscribes)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);
    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_on_methods_unsubscribed();
    g_on_methods_unsubscribed(g_on_methods_unsubscribed_context);

    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STARTED, true, true, true, true, 1, TEST_current_time, true /* here lies the difference */);

    // act
    (void)IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

/* IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod */

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Unsubscribe_With_NULL_handle_does_nothing)
{
    // arrange
    // act
    IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Unsubscribe_unsubscribes_from_receiving_methods)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(iothubtransportamqp_methods_unsubscribe(TEST_IOTHUBTRANSPORTAMQP_METHODS));

    // act
    IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Unsubscribe_without_subscribe_does_nothing)
{
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothubtransportamqp_methods_unsubscribe(TEST_IOTHUBTRANSPORTAMQP_METHODS));

    // act
    IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

/* IoTHubTransport_AMQP_Common_DoWork */

TEST_FUNCTION(IoTHubTransport_AMQP_Common_DoWork_does_not_subscribe_if_SubScribe_for_methods_was_not_Called_by_the_user)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STARTED, true, true, true, true, 1, TEST_current_time, false);

    // act
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_DoWork_does_not_subscribe_if_already_subscribed)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STARTED, true, true, true, true, 1, TEST_current_time, false);

    // act
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

/* on_methods_request_received */

TEST_FUNCTION(on_methods_request_received_responds_to_the_method_request)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    static unsigned char test_request_payload[] = { 0x42, 0x43 };
    static unsigned char test_method_response[] = { 0x44, 0x45 };
    int result;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    umock_c_reset_all_calls();
    set_expected_calls_for_Register(&device_config, true);
    device_handle = IoTHubTransport_AMQP_Common_Register(handle, &device_config, &TEST_waitingToSend);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Transport_DeviceMethod_Complete_Callback("test_method", IGNORED_PTR_ARG, sizeof(test_method_response), IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    result = g_on_method_request_received(g_on_method_request_received_context, "test_method", test_request_payload, sizeof(test_request_payload), TEST_METHOD_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_DeviceMethod_Response_succeed)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;

    initialize_test_variables();

    handle = create_transport();

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(iothubtransportamqp_methods_respond(TEST_METHOD_HANDLE, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE));

    // act
    int result = IoTHubTransport_AMQP_Common_DeviceMethod_Response(handle, (METHOD_HANDLE)TEST_METHOD_HANDLE, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_DeviceMethod_Response_fail)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;

    initialize_test_variables();

    handle = create_transport();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(iothubtransportamqp_methods_respond(TEST_METHOD_HANDLE, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE))
        .SetReturn(1);

    // act
    int result = IoTHubTransport_AMQP_Common_DeviceMethod_Response(handle, TEST_METHOD_HANDLE, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_DEVICE_STATUS_CODE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

/* on_methods_error */

TEST_FUNCTION(on_methods_error_does_nothing)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUB_DEVICE_CONFIG device_config;
    IOTHUB_DEVICE_HANDLE device_handle;

    handle = create_transport();

    device_config.deviceId = "blah";
    device_config.deviceKey = "cucu";
    device_config.deviceSasToken = NULL;
    device_config.moduleId = NULL;

    device_handle = register_device(handle, &device_config, &TEST_waitingToSend, true);

    (void)IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(device_handle);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, true);

    umock_c_reset_all_calls();

    // act
    g_on_methods_error(g_on_methods_error_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(AMQP_Create_NULL_config)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    TRANSPORT_LL_HANDLE tr_hdl = IoTHubTransport_AMQP_Common_Create(NULL, NULL, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(tr_hdl, "Expected NULL transport handle");

    // cleanup
}

TEST_FUNCTION(AMQP_Create_NULL_upperConfig)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG config;
    config.upperConfig = NULL;

    umock_c_reset_all_calls();

    // act
    TRANSPORT_LL_HANDLE tr_hdl = IoTHubTransport_AMQP_Common_Create(&config, TEST_amqp_get_io_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(tr_hdl);
}

TEST_FUNCTION(AMQP_Create_upperConfig_protocol_NULL)
{
    // arrange
    IOTHUBTRANSPORT_CONFIG* config = create_transport_config();

    umock_c_reset_all_calls();

    // act
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_AMQP_Common_Create(config, TEST_amqp_get_io_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(handle);
}

TEST_FUNCTION(AMQP_Create_upperConfig_iotHubName_NULL)
{
    // arrange
    IOTHUB_CLIENT_CONFIG client_config;
    client_config.protocol = NULL;
    client_config.deviceId = TEST_DEVICE_ID_CHAR_PTR;
    client_config.deviceKey = TEST_DEVICE_KEY;
    client_config.deviceSasToken = TEST_DEVICE_SAS_TOKEN;
    client_config.iotHubName = NULL;
    client_config.iotHubSuffix = TEST_IOT_HUB_SUFFIX;
    client_config.protocolGatewayHostName = TEST_PROT_GW_HOSTNAME;

    IOTHUBTRANSPORT_CONFIG config;
    config.upperConfig = &client_config;
    config.waitingToSend = NULL;

    umock_c_reset_all_calls();

    // act
    TRANSPORT_LL_HANDLE handle = IoTHubTransport_AMQP_Common_Create(&config, TEST_amqp_get_io_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(handle);
}

TEST_FUNCTION(GetHostName_NULL_handle)
{
    // arrange

    // act
    STRING_HANDLE result = IoTHubTransport_AMQP_Common_GetHostname(NULL);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);

    // cleanup
}

TEST_FUNCTION(GetHostName_success)
{
    // arrange
    TRANSPORT_LL_HANDLE handle = create_transport();

    umock_c_reset_all_calls();
    set_expected_calls_for_GetHostname();

    // act
    STRING_HANDLE result = IoTHubTransport_AMQP_Common_GetHostname(handle);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_IOTHUB_HOST_FQDN_CLONE_STRING_HANDLE, result);

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(Create_success)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUBTRANSPORT_CONFIG* config = create_transport_config();

    set_expected_calls_for_Create(config);

    // act
    handle = IoTHubTransport_AMQP_Common_Create(config, TEST_amqp_get_io_transport, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Create_callbacks_NULL_fail)
{
    // arrange
    initialize_test_variables();

    TRANSPORT_LL_HANDLE handle;
    IOTHUBTRANSPORT_CONFIG* config = create_transport_config();

    // act
    handle = IoTHubTransport_AMQP_Common_Create(config, TEST_amqp_get_io_transport, NULL, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(Create_failure_checks)
{
    // arrange
    initialize_test_variables();

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    TRANSPORT_LL_HANDLE handle;
    IOTHUBTRANSPORT_CONFIG* config = create_transport_config();

    umock_c_reset_all_calls();
    set_expected_calls_for_Create(config);
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
        handle = IoTHubTransport_AMQP_Common_Create(config, TEST_amqp_get_io_transport, &transport_cb_info, transport_cb_ctx);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(handle, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Destroy_NULL_handle)
{
    // arrange
    initialize_test_variables();
    umock_c_reset_all_calls();

    // act
    IoTHubTransport_AMQP_Common_Destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(Destroy_success)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE registered_devices[1];
    registered_devices[0] = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    set_expected_calls_for_Destroy(1, registered_devices);

    // act
    IoTHubTransport_AMQP_Common_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(Register_NULL_wts)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    // act
    IOTHUB_DEVICE_HANDLE device_handle = IoTHubTransport_AMQP_Common_Register(handle, device_config, NULL);

    // assert
    ASSERT_IS_NULL(device_handle);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Register_NULL_device_config)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    // act
    IOTHUB_DEVICE_HANDLE device_handle = IoTHubTransport_AMQP_Common_Register(handle, NULL, &TEST_waitingToSend);

    // assert
    ASSERT_IS_NULL(device_handle);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Register_NULL_transport_handle)
{
    // arrange
    initialize_test_variables();
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    // act
    IOTHUB_DEVICE_HANDLE device_handle = IoTHubTransport_AMQP_Common_Register(NULL, device_config, &TEST_waitingToSend);

    // assert
    ASSERT_IS_NULL(device_handle);

    // cleanup
}

TEST_FUNCTION(Register_NULL_config_device_id)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    device_config->deviceId = NULL;

    // act
    IOTHUB_DEVICE_HANDLE device_handle = IoTHubTransport_AMQP_Common_Register(handle, device_config, &TEST_waitingToSend);

    // assert
    ASSERT_IS_NULL(device_handle);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Register_device_already_registered)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    STRICT_EXPECTED_CALL(singlylinkedlist_find(TEST_REGISTERED_DEVICES_LIST, IGNORED_PTR_ARG, device_config->deviceId))
        .IgnoreArgument(2)
        .SetReturn(TEST_LIST_ITEM_HANDLE);

    // act
    IOTHUB_DEVICE_HANDLE device_handle = IoTHubTransport_AMQP_Common_Register(handle, device_config, &TEST_waitingToSend);

    // assert
    ASSERT_IS_NULL(device_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Register_CBS_transport_X509_credentials)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config1 = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE device_handle1 = register_device(handle, device_config1, &TEST_waitingToSend, true);

    IOTHUB_DEVICE_CONFIG* device_config2 = create_device_config(TEST_DEVICE_ID_2_CHAR_PTR, true);
    device_config2->deviceKey = NULL;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(singlylinkedlist_find(TEST_REGISTERED_DEVICES_LIST, IGNORED_PTR_ARG, device_config2->deviceId))
        .IgnoreArgument(2)
        .SetReturn(NULL);

    // act
    IOTHUB_DEVICE_HANDLE device_handle2 = IoTHubTransport_AMQP_Common_Register(handle, device_config2, &TEST_waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(device_handle1);
    ASSERT_IS_NULL(device_handle2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle1, NULL);
}

TEST_FUNCTION(Register_X509_transport_CBS_credentials)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config1 = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    device_config1->deviceKey = NULL;

    IOTHUB_DEVICE_HANDLE device_handle1 = register_device(handle, device_config1, &TEST_waitingToSend, false);

    IOTHUB_DEVICE_CONFIG* device_config2 = create_device_config(TEST_DEVICE_ID_2_CHAR_PTR, true);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(singlylinkedlist_find(TEST_REGISTERED_DEVICES_LIST, IGNORED_PTR_ARG, device_config2->deviceId))
        .IgnoreArgument(2)
        .SetReturn(NULL);

    // act
    IOTHUB_DEVICE_HANDLE device_handle2 = IoTHubTransport_AMQP_Common_Register(handle, device_config2, &TEST_waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(device_handle1);
    ASSERT_IS_NULL(device_handle2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle1, NULL);
}

TEST_FUNCTION(Register_failure_checks)
{
    // arrange
    initialize_test_variables();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_Register(device_config, true);
    umock_c_negative_tests_snapshot();

    // act
    size_t i, n = umock_c_negative_tests_call_count();
    for (i = 0; i < n; i++)
    {
        if (i == 0 || i == 2 || i == 3 || i == 4 || i >= 5)
        {
            // These expected calls do not cause the API to fail.
            continue;
        }

        // arrange
        char error_msg[64];

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        IOTHUB_DEVICE_HANDLE device_handle = IoTHubTransport_AMQP_Common_Register(handle, device_config, &TEST_waitingToSend);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_IS_NULL(device_handle, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    destroy_transport(handle, NULL, NULL);
}


TEST_FUNCTION(Register_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_Register(device_config, true);

    // act
    IOTHUB_DEVICE_HANDLE device_handle = IoTHubTransport_AMQP_Common_Register(handle, device_config, &TEST_waitingToSend);

    // assert
    ASSERT_IS_NOT_NULL(device_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Subscribe_NULL_handle)
{
    // arrange
    initialize_test_variables();
    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_AMQP_Common_Subscribe(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(Subscribe_device_not_registered)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_is_device_registered(device_config, NULL);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);

    // act
    int result = IoTHubTransport_AMQP_Common_Subscribe(device_handle);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Subscribe_messages_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_Subscribe(device_config, device_handle);

    // act
    int result = IoTHubTransport_AMQP_Common_Subscribe(device_handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Subscribe_messages_failure_checks)
{
    // arrange
    initialize_test_variables();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_Subscribe(device_config, device_handle);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (i == 0 || i == 1)
        {
            continue;
        }

        // arrange
        char error_msg[64];
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        int result = IoTHubTransport_AMQP_Common_Subscribe(device_handle);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }

    // cleanup
    destroy_transport(handle, device_handle, NULL);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(Unsubscribe_NULL_handle)
{
    // arrange
    initialize_test_variables();
    umock_c_reset_all_calls();

    // act
    IoTHubTransport_AMQP_Common_Unsubscribe(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(Unsubscribe_messages_device_not_registered)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_is_device_registered(device_config, NULL);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);

    // act
    IoTHubTransport_AMQP_Common_Unsubscribe(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Unsubscribe_messages_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_Subscribe(device_config, device_handle);
    (void)IoTHubTransport_AMQP_Common_Subscribe(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_Unsubscribe(device_config, device_handle);

    // act
    IoTHubTransport_AMQP_Common_Unsubscribe(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(GetSendStatus_NULL_handle)
{
    // arrange
    initialize_test_variables();
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS iotHubClientStatus;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetSendStatus(NULL, &iotHubClientStatus);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(GetSendStatus_NULL_iotHubClientStatus)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetSendStatus(device_handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(GetSendStatus_failure_checks)
{
    // arrange
    initialize_test_variables();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_GetSendStatus(true, DEVICE_SEND_STATUS_IDLE);
    umock_c_negative_tests_snapshot();

    // act
    size_t i;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (!umock_c_negative_tests_can_call_fail(i))
        {
            continue;
        }

        // arrange
        char error_msg[64];
        IOTHUB_CLIENT_STATUS iotHubClientStatus;

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetSendStatus(device_handle, &iotHubClientStatus);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(GetSendStatus_IDLE_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_GetSendStatus(true, DEVICE_SEND_STATUS_IDLE);

    IOTHUB_CLIENT_STATUS iotHubClientStatus;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetSendStatus(device_handle, &iotHubClientStatus);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_SEND_STATUS_IDLE, iotHubClientStatus);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(GetSendStatus_BUSY_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_GetSendStatus(true, DEVICE_SEND_STATUS_BUSY);

    IOTHUB_CLIENT_STATUS iotHubClientStatus;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetSendStatus(device_handle, &iotHubClientStatus);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_SEND_STATUS_BUSY, iotHubClientStatus);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(GetSendStatus_waiting_to_send_not_empty_BUSY)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_GetSendStatus(false, DEVICE_SEND_STATUS_BUSY);

    IOTHUB_CLIENT_STATUS iotHubClientStatus;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetSendStatus(device_handle, &iotHubClientStatus);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_SEND_STATUS_BUSY, iotHubClientStatus);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_NULL_handle)
{
    // arrange
    initialize_test_variables();
    umock_c_reset_all_calls();

    // act
    size_t value = 10;
    int result = IoTHubTransport_AMQP_Common_SetOption(NULL, OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(SetOption_NULL_name)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    // act
    size_t value = 10;
    int result = IoTHubTransport_AMQP_Common_SetOption(handle, NULL, &value);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_NULL_value)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_EVENT_SEND_TIMEOUT_SECS, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_device_specific_failure_check)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    size_t value = 10;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_REGISTERED_DEVICES_LIST));
    EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG)).SetReturn(device_handle);
    STRICT_EXPECTED_CALL(amqp_device_set_option(TEST_DEVICE_HANDLE, DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, &value))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_EVENT_SEND_TIMEOUT_SECS, &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_CBS_transport_option_x509certificate)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    const char* value = "cert info";

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_X509_CERT, value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_CBS_transport_option_x509privatekey)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    const char* value = "cert info";

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_X509_PRIVATE_KEY, value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_log_trace)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    // This creates the amqp_connection_handle
    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    bool value = true;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(amqp_connection_set_logging(TEST_AMQP_CONNECTION_HANDLE, value))
        .IgnoreArgument(2)
        .SetReturn(0);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_LOG_TRACE, &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}
TEST_FUNCTION(SetOption_xio_option_success)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    bool value = true;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE))
        .SetReturn(TEST_IOTHUB_HOST_FQDN_CHAR_PTR);
    STRICT_EXPECTED_CALL(xio_setoption(TEST_UNDERLYING_IO_TRANSPORT, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(0);
    STRICT_EXPECTED_CALL(xio_retrieveoptions(TEST_UNDERLYING_IO_TRANSPORT))
        .SetReturn(TEST_OPTIONHANDLER_HANDLE);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "Some XIO option name", &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_xio_option_get_underlying_TLS_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    bool value = true;
    TEST_amqp_get_io_transport_result = NULL;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE))
        .SetReturn(TEST_IOTHUB_HOST_FQDN_CHAR_PTR);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "Some XIO option name", &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_xio_option_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    bool value = true;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE))
        .SetReturn(TEST_IOTHUB_HOST_FQDN_CHAR_PTR);
    STRICT_EXPECTED_CALL(xio_setoption(TEST_UNDERLYING_IO_TRANSPORT, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "Some XIO option name", &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_cl2svc_keep_alive_send_ratio_fail_for_zero)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    // This creates the amqp_connection_handle
    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    double value = 0.0;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_REMOTE_IDLE_TIMEOUT_RATIO, &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_cl2svc_keep_alive_send_ratio_fail_for_1)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    // This creates the amqp_connection_handle
    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    double value = 1.0;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_REMOTE_IDLE_TIMEOUT_RATIO, &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_cl2svc_keep_alive_send_ratio_success_for_0875)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    // This creates the amqp_connection_handle
    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    double value = 0.875;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_REMOTE_IDLE_TIMEOUT_RATIO, &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_with_proxy_data_copies_the_options_for_later_use)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "me";
    http_proxy_options.password = "shhhh";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "me"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "shhhh"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_with_NULL_host_address_fails)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = NULL;
    http_proxy_options.port = 2222;
    http_proxy_options.username = "me";
    http_proxy_options.password = "shhhh";

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_with_NULL_option_value_fails)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", NULL);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_and_password_saves_only_the_hostname)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = NULL;
    http_proxy_options.password = NULL;

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_but_non_NULL_password_fails)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "the_other_me";
    http_proxy_options.password = NULL;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_with_NULL_password_but_non_NULL_username_fails)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = NULL;
    http_proxy_options.password = "bleah";

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_frees_previously_Saved_proxy_options)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "me"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "shhhh"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "me";
    http_proxy_options.password = "shhhh";

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(when_allocating_proxy_name_fails_SetOption_proxy_data_fails)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .SetReturn(1);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(when_allocating_username_fails_SetOption_proxy_data_fails)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(when_allocating_password_fails_SetOption_proxy_data_fails)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(when_allocating_proxy_name_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .SetReturn(1);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(when_allocating_username_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(when_allocating_password_fails_SetOption_proxy_data_does_not_free_previous_proxy_options)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_with_NULL_hostname_does_not_free_previous_proxy_options)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = NULL;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_with_NULL_username_and_non_NULL_password_does_not_free_previous_proxy_options)
{
    // arrange
    TRANSPORT_LL_HANDLE handle;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    umock_c_reset_all_calls();

    http_proxy_options.host_address = "baba";
    http_proxy_options.username = NULL;
    http_proxy_options.password = "cloanta";

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_xio_option_get_underlying_TLS_when_proxy_data_was_not_set_passes_down_NULL)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    bool value = true;
    TEST_amqp_get_io_transport_result = NULL;
    expected_AMQP_TRANSPORT_PROXY_OPTIONS = NULL;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "Some XIO option name", &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(int, 0, error_proxy_options);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_xio_option_get_underlying_TLS_when_proxy_data_was_set_passes_down_the_proxy_options)
{
    // arrange
    AMQP_TRANSPORT_PROXY_OPTIONS amqp_transport_proxy_options;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    amqp_transport_proxy_options.host_address = "test_proxy";
    amqp_transport_proxy_options.port = 2222;
    amqp_transport_proxy_options.username = "haha";
    amqp_transport_proxy_options.password = "bleah";

    bool value = true;
    TEST_amqp_get_io_transport_result = NULL;
    expected_AMQP_TRANSPORT_PROXY_OPTIONS = &amqp_transport_proxy_options;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "Some XIO option name", &value);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(int, 0, error_proxy_options);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_proxy_data_when_underlying_IO_is_already_created_fails)
{
    // arrange
    AMQP_TRANSPORT_PROXY_OPTIONS amqp_transport_proxy_options;
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    amqp_transport_proxy_options.host_address = "test_proxy";
    amqp_transport_proxy_options.port = 2222;
    amqp_transport_proxy_options.username = "haha";
    amqp_transport_proxy_options.password = "bleah";

    bool value = true;
    expected_AMQP_TRANSPORT_PROXY_OPTIONS = &amqp_transport_proxy_options;

    // act
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "Some XIO option name", &value);
    umock_c_reset_all_calls();

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);

    // assert
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_retry_interval_succeed)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    // act
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(retry_control_set_option(IGNORED_PTR_ARG, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, IGNORED_PTR_ARG));

    int retry_interval = 10;
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_RETRY_INTERVAL_SEC, &retry_interval);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_retry_max_delay_succeed)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    // act
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(retry_control_set_option(IGNORED_PTR_ARG, RETRY_CONTROL_OPTION_MAX_DELAY_IN_SECS, IGNORED_PTR_ARG));

    int retry_interval = 10;
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_RETRY_MAX_DELAY_SECS, &retry_interval);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(SetOption_retry_interval_fail)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    // act
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(retry_control_set_option(IGNORED_PTR_ARG, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, IGNORED_PTR_ARG)).SetReturn(__LINE__);

    int retry_interval = 10;
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_RETRY_INTERVAL_SEC, &retry_interval);

    // assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_Destroy_frees_proxy_options)
{
    // arrange
    HTTP_PROXY_OPTIONS http_proxy_options;
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    http_proxy_options.host_address = "test_proxy";
    http_proxy_options.port = 2222;
    http_proxy_options.username = "haha";
    http_proxy_options.password = "bleah";

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "test_proxy"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.host_address, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "haha"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.username, sizeof(char**));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, "bleah"))
        .CopyOutArgumentBuffer_destination(&http_proxy_options.password, sizeof(char**));

    (void)IoTHubTransport_AMQP_Common_SetOption(handle, "proxy_data", &http_proxy_options);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_REGISTERED_DEVICES_LIST));

    EXPECTED_CALL(singlylinkedlist_item_get_value(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_PTR_ARG));

    set_expected_calls_for_Unregister(device_handle);

    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_REGISTERED_DEVICES_LIST));
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(STRING_delete(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_PTR_ARG));

    // act
    IoTHubTransport_AMQP_Common_Destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(Unregister_NULL_handle)
{
    // arrange
    initialize_test_variables();
    umock_c_reset_all_calls();

    // act
    IoTHubTransport_AMQP_Common_Unregister(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(Unregister_device_not_registered)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_DEVICE_ID_STRING_HANDLE))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);
    set_expected_calls_for_is_device_registered_ex(device_config, NULL);

    // act
    IoTHubTransport_AMQP_Common_Unregister(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(Unregister_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    umock_c_reset_all_calls();
    set_expected_calls_for_Unregister(device_handle);

    // act
    IoTHubTransport_AMQP_Common_Unregister(device_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(on_message_received_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_Subscribe(device_config, device_handle);
    (void)IoTHubTransport_AMQP_Common_Subscribe(device_handle);

    DEVICE_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = "some link source name";
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(amqp_device_clone_message_disposition_info(IGNORED_PTR_ARG))
        .SetReturn(&disposition_info);
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDispositionContext(IGNORED_PTR_ARG, (MESSAGE_DISPOSITION_CONTEXT_HANDLE)&disposition_info, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(true);

    // act
    DEVICE_MESSAGE_DISPOSITION_RESULT result = TEST_device_subscribe_message_saved_callback(
        TEST_IOTHUB_MESSAGE_HANDLE, &disposition_info, TEST_device_subscribe_message_saved_context);

    // assert
    ASSERT_ARE_EQUAL(int, DEVICE_MESSAGE_DISPOSITION_RESULT_NONE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(on_message_received_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_Subscribe(device_config, device_handle);
    (void)IoTHubTransport_AMQP_Common_Subscribe(device_handle);

    DEVICE_MESSAGE_DISPOSITION_INFO disposition_info;
    disposition_info.source = "some link source name";
    disposition_info.message_id = TEST_MESSAGE_ID;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(amqp_device_clone_message_disposition_info(IGNORED_PTR_ARG))
        .SetReturn(&disposition_info);
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDispositionContext(IGNORED_PTR_ARG, (MESSAGE_DISPOSITION_CONTEXT_HANDLE)&disposition_info, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    DEVICE_MESSAGE_DISPOSITION_RESULT result = TEST_device_subscribe_message_saved_callback(
        TEST_IOTHUB_MESSAGE_HANDLE, &disposition_info, TEST_device_subscribe_message_saved_context);

    // assert
    ASSERT_ARE_EQUAL(int, DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(DoWork_success)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    // act
    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, false, true, false, false, 1, TEST_current_time, false);
    IoTHubTransport_AMQP_Common_DoWork(handle);

    ASSERT_IS_NOT_NULL(TEST_amqp_connection_create_saved_on_state_changed_callback);

    TEST_amqp_connection_create_saved_on_state_changed_callback(
        TEST_amqp_connection_create_saved_on_state_changed_context,
        AMQP_CONNECTION_STATE_CLOSED, AMQP_CONNECTION_STATE_OPENED);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, true, true, true, true, 1, TEST_current_time, false);
    IoTHubTransport_AMQP_Common_DoWork(handle);

    ASSERT_IS_NOT_NULL(TEST_device_create_saved_on_state_changed_callback);

    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    STRICT_EXPECTED_CALL(retry_control_reset(TEST_RETRY_CONTROL_HANDLE));
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, IGNORED_PTR_ARG));

    TEST_device_create_saved_on_state_changed_callback(TEST_device_create_saved_on_state_changed_context,
        DEVICE_STATE_STOPPED, DEVICE_STATE_STARTED);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STARTED, true, true, true, true, 1, TEST_current_time, false);
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(on_amqp_connection_state_changed_CLOSED_unexpectedly)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();

    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, false, true, false, false, 1, TEST_current_time, false);
    IoTHubTransport_AMQP_Common_DoWork(handle);

    TEST_amqp_connection_create_saved_on_state_changed_callback(
        TEST_amqp_connection_create_saved_on_state_changed_context,
        AMQP_CONNECTION_STATE_CLOSED, AMQP_CONNECTION_STATE_OPENED);

    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, true, true, true, true, 1, TEST_current_time, false);
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // act
    TEST_amqp_connection_create_saved_on_state_changed_callback(
        TEST_amqp_connection_create_saved_on_state_changed_context,
        AMQP_CONNECTION_STATE_OPENED, AMQP_CONNECTION_STATE_CLOSED);

    set_expected_calls_for_prepare_for_connection_retry(1, DEVICE_STATE_STOPPED);
    IoTHubTransport_AMQP_Common_DoWork(handle);

    set_expected_calls_for_DoWork2(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, false, true, true, false, false, 1, TEST_current_time, false);
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(DoWork_NULL_handle)
{
    // arrange
    initialize_test_variables();
    umock_c_reset_all_calls();

    // act
    IoTHubTransport_AMQP_Common_DoWork(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(DoWork_get_underlying_io_transport_provider_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_REGISTERED_DEVICES_LIST));
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_IOTHUB_HOST_FQDN_STRING_HANDLE))
        .SetReturn(TEST_IOTHUB_HOST_FQDN_CHAR_PTR);
    TEST_amqp_get_io_transport_result = NULL;

    // act
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(DoWork_sets_amqp_connection_for_X509)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    const char* certificate = TEST_X509_CERTIFICATE;
    const char* private_key = TEST_X509_PRIVATE_KEY;
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_X509_CERT, certificate);
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_X509_PRIVATE_KEY, private_key);

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config_for_x509(TEST_DEVICE_ID_CHAR_PTR);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, false);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, true, true, false, false, 1, TEST_current_time, false);

    // act
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_IS_FALSE(TEST_amqp_connection_create_saved_create_sasl_io);
    ASSERT_IS_FALSE(TEST_amqp_connection_create_saved_create_cbs_connection);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(size_t, TEST_amqp_connection_create_saved_c2d_keep_alive_freq_secs, TEST_DEFAULT_SVC2CL_KEEP_ALIVE_FREQ_SECS);
    ASSERT_ARE_EQUAL(double, TEST_amqp_connection_create_saved_cl2svc_keep_alive_send_ratio, TEST_DEFAULT_REMOTE_IDLE_TIMEOUT_RATIO);
    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(DoWork_sets_amqp_connection_for_CBS)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, false, true, false, false, 1, TEST_current_time, false);

    // act
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_IS_TRUE(TEST_amqp_connection_create_saved_create_sasl_io);
    ASSERT_IS_TRUE(TEST_amqp_connection_create_saved_create_cbs_connection);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(size_t, TEST_amqp_connection_create_saved_c2d_keep_alive_freq_secs, TEST_DEFAULT_SVC2CL_KEEP_ALIVE_FREQ_SECS);
    ASSERT_ARE_EQUAL(double, TEST_amqp_connection_create_saved_cl2svc_keep_alive_send_ratio, TEST_DEFAULT_REMOTE_IDLE_TIMEOUT_RATIO);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(DoWork_configures_AMQP_connection_using_c2d_keep_alive_freq_secs)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    const char* certificate = TEST_X509_CERTIFICATE;
    const char* private_key = TEST_X509_PRIVATE_KEY;
    size_t c2d_secs = TEST_USER_SVC2CL_KEEP_ALIVE_FREQ_SECS;
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_X509_CERT, certificate);
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_X509_PRIVATE_KEY, private_key);
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_SERVICE_SIDE_KEEP_ALIVE_FREQ_SECS, &c2d_secs);

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config_for_x509(TEST_DEVICE_ID_CHAR_PTR);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, false);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, true, true, false, false, 1, TEST_current_time, false);

    // act
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_IS_FALSE(TEST_amqp_connection_create_saved_create_sasl_io);
    ASSERT_IS_FALSE(TEST_amqp_connection_create_saved_create_cbs_connection);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(size_t, TEST_amqp_connection_create_saved_c2d_keep_alive_freq_secs, TEST_USER_SVC2CL_KEEP_ALIVE_FREQ_SECS);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}
TEST_FUNCTION(DoWork_configures_AMQP_connection_using_cl2svc_keep_alive_send_ratio)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    const char* certificate = TEST_X509_CERTIFICATE;
    const char* private_key = TEST_X509_PRIVATE_KEY;
    double cl2svc_ratio = TEST_USER_REMOTE_IDLE_TIMEOUT_RATIO;
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_X509_CERT, certificate);
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_X509_PRIVATE_KEY, private_key);
    (void)IoTHubTransport_AMQP_Common_SetOption(handle, OPTION_REMOTE_IDLE_TIMEOUT_RATIO, &cl2svc_ratio);

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config_for_x509(TEST_DEVICE_ID_CHAR_PTR);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, false);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    set_expected_calls_for_DoWork(&TEST_waitingToSend, 0, DEVICE_STATE_STOPPED, true, true, false, false, 1, TEST_current_time, false);

    // act
    IoTHubTransport_AMQP_Common_DoWork(handle);

    // assert
    ASSERT_IS_FALSE(TEST_amqp_connection_create_saved_create_sasl_io);
    ASSERT_IS_FALSE(TEST_amqp_connection_create_saved_create_cbs_connection);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(double, TEST_amqp_connection_create_saved_cl2svc_keep_alive_send_ratio, TEST_USER_REMOTE_IDLE_TIMEOUT_RATIO);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}


TEST_FUNCTION(ConnectionStatusCallBack_UNAUTH_OK)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE device_handle;
    device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, IGNORED_PTR_ARG));

    // act
    TEST_device_create_saved_on_state_changed_callback(TEST_device_create_saved_on_state_changed_context,
        DEVICE_STATE_STARTED, DEVICE_STATE_STOPPED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(ConnectionStatusCallBack_UNAUTH_auth_error)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE device_handle;
    device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL, IGNORED_PTR_ARG));

    // act
    TEST_device_create_saved_on_state_changed_callback(TEST_device_create_saved_on_state_changed_context,
        DEVICE_STATE_STARTED, DEVICE_STATE_ERROR_AUTH);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(ConnectionStatusCallBack_UNAUTH_auth_communication_error)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE device_handle;
    device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, IGNORED_PTR_ARG));

    // act
    TEST_device_create_saved_on_state_changed_callback(TEST_device_create_saved_on_state_changed_context,
        DEVICE_STATE_STARTED, DEVICE_STATE_ERROR_AUTH_TIMEOUT);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(ConnectionStatusCallBack_UNAUTH_msg_communication_error)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE device_handle;
    device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR, IGNORED_PTR_ARG));

    // act
    TEST_device_create_saved_on_state_changed_callback(TEST_device_create_saved_on_state_changed_context,
        DEVICE_STATE_STARTED, DEVICE_STATE_ERROR_MSG);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(ConnectionStatusCallBack_UNAUTH_no_network)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE device_handle;
    device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_NETWORK, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(get_time(NULL)).SetReturn(TEST_current_time);

    // act

    TEST_amqp_connection_create_saved_on_state_changed_callback(
        TEST_amqp_connection_create_saved_on_state_changed_context,
        AMQP_CONNECTION_STATE_OPENED, AMQP_CONNECTION_STATE_ERROR);

    TEST_device_create_saved_on_state_changed_callback(TEST_device_create_saved_on_state_changed_context,
        DEVICE_STATE_STARTED, DEVICE_STATE_STOPPED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(ConnectionStatusCallBack_UNAUTH_retry_expired)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();
    int result_set_retry_policy = IoTHubTransport_AMQP_Common_SetRetryPolicy(handle, IOTHUB_CLIENT_RETRY_IMMEDIATE, 1);
    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE device_handle;
    device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_NETWORK, IGNORED_PTR_ARG));
    RETRY_ACTION retry_action = RETRY_ACTION_STOP_RETRYING;
    STRICT_EXPECTED_CALL(retry_control_should_retry(TEST_RETRY_CONTROL_HANDLE, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_retry_action(&retry_action, sizeof(RETRY_ACTION));

    STRICT_EXPECTED_CALL(singlylinkedlist_foreach(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Transport_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED, IGNORED_PTR_ARG));

    // act
    TEST_amqp_connection_create_saved_on_state_changed_callback(
        TEST_amqp_connection_create_saved_on_state_changed_context,
        AMQP_CONNECTION_STATE_OPENED, AMQP_CONNECTION_STATE_ERROR);

    (void)IoTHubTransport_AMQP_Common_DoWork(handle);

    bool continue_processing;
    TEST_singlylinkedlist_foreach_action_function(
        saved_registered_devices_list[saved_registered_devices_list_count - 1],
        TEST_singlylinkedlist_foreach_context,
        &continue_processing);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result_set_retry_policy);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_NULL_data_fails)
{
    // arrange
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE registered_devices[1];
    registered_devices[0] = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(registered_devices[0], NULL, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_NULL_MESSAGE_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, NULL, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_NULL_disposition_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    umock_c_reset_all_calls();
    
    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(IOTHUB_MESSAGE_INVALID_ARG);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, TEST_IOTHUB_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_ACCEPTED_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    MESSAGE_DISPOSITION_CONTEXT disposition_context;
    disposition_context.message_id = 123;
    disposition_context.source = "my_link";

    umock_c_reset_all_calls();
    set_expected_calls_for_SendMessageDisposition(IOTHUBMESSAGE_ACCEPTED, &disposition_context);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, TEST_IOTHUB_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_ACCEPTED_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    MESSAGE_DISPOSITION_CONTEXT disposition_context;
    disposition_context.message_id = 123;
    disposition_context.source = "my_link";

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_dispositionContext(&disposition_context, sizeof(&disposition_context));
    STRICT_EXPECTED_CALL(amqp_device_send_message_disposition(TEST_DEVICE_HANDLE, IGNORED_PTR_ARG, DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED))
        .IgnoreArgument(2)
        .SetReturn(1);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, TEST_IOTHUB_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_DEVICE_MESSAGE_DISPOSITION_INFO_create_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    MESSAGE_DISPOSITION_CONTEXT* disposition_context = NULL;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_dispositionContext(&disposition_context, sizeof(&disposition_context));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, TEST_IOTHUB_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_ABANDONED_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    MESSAGE_DISPOSITION_CONTEXT disposition_context;
    disposition_context.message_id = 123;
    disposition_context.source = "my_link";

    umock_c_reset_all_calls();
    set_expected_calls_for_SendMessageDisposition(IOTHUBMESSAGE_ABANDONED, &disposition_context);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, TEST_IOTHUB_MESSAGE_HANDLE, IOTHUBMESSAGE_ABANDONED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_ABANDONED_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    MESSAGE_DISPOSITION_CONTEXT disposition_context;
    disposition_context.message_id = 123;
    disposition_context.source = "my_link";

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_dispositionContext(&disposition_context, sizeof(&disposition_context));
    STRICT_EXPECTED_CALL(amqp_device_send_message_disposition(TEST_DEVICE_HANDLE, IGNORED_PTR_ARG, DEVICE_MESSAGE_DISPOSITION_RESULT_RELEASED))
        .IgnoreArgument(2)
        .SetReturn(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, TEST_IOTHUB_MESSAGE_HANDLE, IOTHUBMESSAGE_ABANDONED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_REJECTED_succeeds)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    MESSAGE_DISPOSITION_CONTEXT disposition_context;
    disposition_context.message_id = 123;
    disposition_context.source = "my_link";

    umock_c_reset_all_calls();
    set_expected_calls_for_SendMessageDisposition(IOTHUBMESSAGE_REJECTED, &disposition_context);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, TEST_IOTHUB_MESSAGE_HANDLE, IOTHUBMESSAGE_REJECTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SendMessageDisposition_REJECTED_fails)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, false);
    IOTHUB_DEVICE_HANDLE device_handle = register_device(handle, device_config, &TEST_waitingToSend, true);
    ASSERT_IS_NOT_NULL(device_handle);

    MESSAGE_DISPOSITION_CONTEXT disposition_context;
    disposition_context.message_id = 123;
    disposition_context.source = (char*)malloc(12);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_dispositionContext(&disposition_context, sizeof(&disposition_context));
    STRICT_EXPECTED_CALL(amqp_device_send_message_disposition(TEST_DEVICE_HANDLE, IGNORED_PTR_ARG, DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED))
        .IgnoreArgument(2)
        .SetReturn(1);
    EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn(TEST_DEVICE_ID_CHAR_PTR);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_SendMessageDisposition(device_handle, TEST_IOTHUB_MESSAGE_HANDLE, IOTHUBMESSAGE_REJECTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);

    // cleanup
    destroy_transport(handle, device_handle, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SetRetryPolicy_NULL_handle)
{
    // arrange
    initialize_test_variables();

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_AMQP_Common_SetRetryPolicy(NULL, IOTHUB_CLIENT_RETRY_IMMEDIATE, 1600);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SetRetryPolicy_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(retry_control_create(IOTHUB_CLIENT_RETRY_IMMEDIATE, 1600));
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
        int result = IoTHubTransport_AMQP_Common_SetRetryPolicy(handle, IOTHUB_CLIENT_RETRY_IMMEDIATE, 1600);

        // assert
        sprintf(error_msg, "On failed call %lu", (unsigned long)i);
        ASSERT_ARE_NOT_EQUAL(int, 0, result, error_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_SetRetryPolicy_success)
{
    // arrange
    initialize_test_variables();
    TRANSPORT_LL_HANDLE handle = create_transport();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(retry_control_create(IOTHUB_CLIENT_RETRY_IMMEDIATE, 1600));
    STRICT_EXPECTED_CALL(retry_control_destroy(TEST_RETRY_CONTROL_HANDLE));

    // act
    int result = IoTHubTransport_AMQP_Common_SetRetryPolicy(handle, IOTHUB_CLIENT_RETRY_IMMEDIATE, 1600);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_SetCallbackContext_success)
{
    // arrange
    TRANSPORT_LL_HANDLE handle = create_transport();
    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_AMQP_SetCallbackContext(handle, transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_SetCallbackContext_fail)
{
    // arrange

    // act
    int result = IoTHubTransport_AMQP_SetCallbackContext(NULL, transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_GetTwinAsync_success)
{
    // arrange
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE registered_devices[1];
    registered_devices[0] = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(amqp_device_get_twin_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetTwinAsync(handle, on_device_get_twin_completed_callback, (void*)0x5566);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_OK, result);

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_GetTwinAsync_NULL_handle)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetTwinAsync(NULL, on_device_get_twin_completed_callback, (void*)0x5566);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_GetTwinAsync_NULL_callback)
{
    // arrange
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE registered_devices[1];
    registered_devices[0] = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetTwinAsync(handle, NULL, (void*)0x5566);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_GetTwinAsync_failure_checks)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
    TRANSPORT_LL_HANDLE handle = create_transport();

    IOTHUB_DEVICE_CONFIG* device_config = create_device_config(TEST_DEVICE_ID_CHAR_PTR, true);

    IOTHUB_DEVICE_HANDLE registered_devices[1];
    registered_devices[0] = register_device(handle, device_config, &TEST_waitingToSend, true);

    crank_transport_ready_after_create(handle, &TEST_waitingToSend, 0, false, true, 1, TEST_current_time, false);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(IGNORED_NUM_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(singlylinkedlist_get_next_item(IGNORED_NUM_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(amqp_device_get_twin_async(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (!umock_c_negative_tests_can_call_fail(index))
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubTransport_AMQP_Common_GetTwinAsync(handle, on_device_get_twin_completed_callback, (void*)0x5566);

        // assert
        ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, result, tmp_msg);
    }

    // cleanup
    destroy_transport(handle, NULL, NULL);
    umock_c_negative_tests_deinit();
}


TEST_FUNCTION(IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo_success)
{
    // arrange
    TRANSPORT_LL_HANDLE handle = create_transport();

    PLATFORM_INFO_OPTION info;

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(handle, &info);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, info, PLATFORM_INFO_OPTION_RETRIEVE_SQM);

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo_NULL_handle)
{
    // arrange
    PLATFORM_INFO_OPTION info;

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(NULL, &info);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo_NULL_info)
{
    // arrange
    TRANSPORT_LL_HANDLE handle = create_transport();

    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    destroy_transport(handle, NULL, NULL);
}

END_TEST_SUITE(iothubtransport_amqp_common_ut)
