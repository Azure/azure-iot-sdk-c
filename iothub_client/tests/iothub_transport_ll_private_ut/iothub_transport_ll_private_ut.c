// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

/**
 * The gballoc.h will replace the malloc, free, and realloc by the my_gballoc functions, in this case,
 *    if you define these mock functions after include the gballoc.h, you will create an infinity recursion,
 *    so, places the my_gballoc functions before the #include "azure_c_shared_utility/gballoc.h"
 */
void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

void my_gballoc_free(void* ptr)
{
    free(ptr);
}

/**
 * Include the C standards here.
 */
#ifdef __cplusplus
#include <cstddef>
#include <ctime>
#else
#include <stddef.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#include "internal/iothub_transport_ll_private.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"

MOCKABLE_FUNCTION(, bool, Transport_MessageCallbackFromInput, IOTHUB_MESSAGE_HANDLE, message, void*, ctx);
MOCKABLE_FUNCTION(, bool, Transport_MessageCallback, IOTHUB_MESSAGE_HANDLE, message, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_ConnectionStatusCallBack, IOTHUB_CLIENT_CONNECTION_STATUS, status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_SendComplete_Callback, PDLIST_ENTRY, completed, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, ctx);
MOCKABLE_FUNCTION(, const char*, Transport_GetOption_Product_Info_Callback, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_ReportedStateComplete_Callback, uint32_t, item_id, int, status_code, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_RetrievePropertyComplete_Callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, ctx);
MOCKABLE_FUNCTION(, int, Transport_DeviceMethod_Complete_Callback, const char*, method_name, const unsigned char*, payLoad, size_t, size, METHOD_HANDLE, response_id, void*, ctx);

#undef ENABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(iothub_transport_ll_private_ut)

    TEST_SUITE_INITIALIZE(suite_init)
    {
        int result;
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);

        result = umocktypes_charptr_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    TEST_SUITE_CLEANUP(suite_cleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
        TEST_MUTEX_ACQUIRE(g_testByTest);

        umock_c_reset_all_calls();
    }
    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    TEST_FUNCTION(IoTHub_Transport_ValidateCallbacks_success)
    {
        int result;
        //arrange
        TRANSPORT_CALLBACKS_INFO transport_cb_info;
        transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
        transport_cb_info.twin_retrieve_prop_complete_cb = Transport_Twin_RetrievePropertyComplete_Callback;
        transport_cb_info.twin_rpt_state_complete_cb = Transport_Twin_ReportedStateComplete_Callback;
        transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
        transport_cb_info.connection_status_cb = Transport_ConnectionStatusCallBack;
        transport_cb_info.prod_info_cb = Transport_GetOption_Product_Info_Callback;
        transport_cb_info.msg_input_cb = Transport_MessageCallbackFromInput;
        transport_cb_info.msg_cb = Transport_MessageCallback;
        transport_cb_info.method_complete_cb = Transport_DeviceMethod_Complete_Callback;

        //act
        result = IoTHub_Transport_ValidateCallbacks(&transport_cb_info);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(IoTHub_Transport_ValidateCallbacks_transport_NULL_fail)
    {
        int result;
        //arrange

        //act
        result = IoTHub_Transport_ValidateCallbacks(NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(IoTHub_Transport_ValidateCallbacks_transport_invalid_function_fail)
    {
        // arrange
        TRANSPORT_CALLBACKS_INFO fail_transport_cb;
        TRANSPORT_CALLBACKS_INFO transport_cb_info;
        transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
        transport_cb_info.twin_retrieve_prop_complete_cb = Transport_Twin_RetrievePropertyComplete_Callback;
        transport_cb_info.twin_rpt_state_complete_cb = Transport_Twin_ReportedStateComplete_Callback;
        transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
        transport_cb_info.connection_status_cb = Transport_ConnectionStatusCallBack;
        transport_cb_info.prod_info_cb = Transport_GetOption_Product_Info_Callback;
        transport_cb_info.msg_input_cb = Transport_MessageCallbackFromInput;
        transport_cb_info.msg_cb = Transport_MessageCallback;
        transport_cb_info.method_complete_cb = Transport_DeviceMethod_Complete_Callback;

        for (size_t index = 0; index < 8; index++)
        {
            memcpy(&fail_transport_cb, &transport_cb_info, sizeof(TRANSPORT_CALLBACKS_INFO));
            switch (index)
            {
                case 0:
                    fail_transport_cb.send_complete_cb = NULL;
                    break;
                case 1:
                    fail_transport_cb.twin_retrieve_prop_complete_cb = NULL;
                    break;
                case 2:
                    fail_transport_cb.twin_rpt_state_complete_cb = NULL;
                    break;
                case 3:
                    fail_transport_cb.send_complete_cb = NULL;
                    break;
                case 4:
                    fail_transport_cb.connection_status_cb = NULL;
                    break;
                case 5:
                    fail_transport_cb.prod_info_cb = NULL;
                    break;
                case 6:
                    fail_transport_cb.msg_input_cb = NULL;
                    break;
                case 7:
                    fail_transport_cb.msg_cb = NULL;
                    break;
                case 8:
                    fail_transport_cb.method_complete_cb = NULL;
            }

            // act
            int result = IoTHub_Transport_ValidateCallbacks(&fail_transport_cb);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }

        // cleanup
    }
END_TEST_SUITE(iothub_transport_ll_private_ut)
