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
#include "azure_macro_utils/macro_utils.h"

#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "iothub_client_core_ll.h"
#include "internal/iothub_client_private.h"
#undef ENABLE_MOCKS

#include "iothub_device_client_ll.h"

static const IOTHUB_DEVICE_CLIENT_LL_HANDLE TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE = (IOTHUB_DEVICE_CLIENT_LL_HANDLE)0x0001;
static const IOTHUB_CLIENT_CORE_LL_HANDLE TEST_IOTHUB_CLIENT_CORE_LL_HANDLE = (IOTHUB_CLIENT_CORE_LL_HANDLE)0x0001; //this has to be same as above
static const IOTHUB_CLIENT_TRANSPORT_PROVIDER TEST_TRANSPORT_PROVIDER = (IOTHUB_CLIENT_TRANSPORT_PROVIDER)0x1110;
static const IOTHUB_CLIENT_CONFIG* TEST_CLIENT_CONFIG = (IOTHUB_CLIENT_CONFIG*)0x1115;
static const IOTHUB_CLIENT_DEVICE_CONFIG* TEST_CLIENT_DEVICE_CONFIG = (IOTHUB_CLIENT_DEVICE_CONFIG*)0x000D;
static IOTHUB_MESSAGE_HANDLE TEST_MESSAGE_HANDLE = (IOTHUB_MESSAGE_HANDLE)0x1116;
static IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK TEST_EVENT_CONFIRMATION_CALLBACK = (IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK)0x0002;
static IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC TEST_MESSAGE_CALLBACK_ASYNC = (IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC)0x0003;
static IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK TEST_CONNECTION_STATUS_CALLBACK = (IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK)0x0004;
static IOTHUB_CLIENT_RETRY_POLICY TEST_RETRY_POLICY = (IOTHUB_CLIENT_RETRY_POLICY)0x0005;
static IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK TEST_TWIN_CALLBACK = (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x0006;
static IOTHUB_CLIENT_REPORTED_STATE_CALLBACK TEST_REPORTED_STATE_CALLBACK = (IOTHUB_CLIENT_REPORTED_STATE_CALLBACK)0x0007;
static IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC TEST_DEVICE_METHOD_CALLBACK = (IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC)0x0008;
static IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK TEST_INBOUND_DEVICE_METHOD_CALLBACK = (IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK)0x000C;
static METHOD_HANDLE TEST_METHOD_HANDLE = (METHOD_HANDLE)0x0009;
#ifndef DONT_USE_UPLOADTOBLOB
static IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK TEST_FILE_UPLOAD_CALLBACK = (IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK)0x0009;
static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK TEST_FILE_UPLOAD_GET_DATA_CALLBACK = (IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK)0x000A;
static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX TEST_FILE_UPLOAD_GET_DATA_CALLBACK_EX = (IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX)0x000B;
#endif // !DONT_USE_UPLOADTOBLOB


static int TEST_INT = 0;
static const char* TEST_CONNECTION_STRING = "Test_connection_string";
static const char* TEST_DEVICE_ID = "theidofTheDevice";
static const char* TEST_IOTHUB_URI = "iothub_uri";
static const char* TEST_OPTION = "option";
static const char* TEST_VALUE = "value";
static const unsigned char* TEST_UNSIGNED_CHAR = (const unsigned char*)0x1234;
static size_t TEST_SIZE_T = 10;
static const char* TEST_CHAR_PTR = "char ptr";

static TEST_MUTEX_HANDLE test_serialize_mutex;
MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(iothubdeviceclient_ll_ut)

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

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_DEVICE_CLIENT_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_TRANSPORT_PROVIDER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIG, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_CONFIG, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, void*);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_CreateFromConnectionString, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_Create, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_CreateWithTransport, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_CreateFromDeviceAuth, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SendEventAsync, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_GetSendStatus, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetMessageCallback, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetConnectionStatusCallback, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetRetryPolicy, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_GetRetryPolicy, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_GetLastMessageReceiveTime, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetDeviceTwinCallback, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SendReportedState, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetDeviceMethodCallback, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_DeviceMethodResponse, IOTHUB_CLIENT_OK);
#ifndef DONT_USE_UPLOADTOBLOB
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_UploadToBlob, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_UploadMultipleBlocksToBlob, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx, IOTHUB_CLIENT_OK);
#endif
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION(IoTHubDeviceClient_LL_CreateFromConnectionString_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateFromConnectionString(TEST_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER));

    //act
    IOTHUB_DEVICE_CLIENT_LL_HANDLE result = IoTHubDeviceClient_LL_CreateFromConnectionString(TEST_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER);

    //assert
    ASSERT_IS_TRUE(result == TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_Create_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_Create(TEST_CLIENT_CONFIG));

    //act
    IOTHUB_DEVICE_CLIENT_LL_HANDLE result = IoTHubDeviceClient_LL_Create(TEST_CLIENT_CONFIG);

    //assert
    ASSERT_IS_TRUE(result == TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_CreateWithTransport_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateWithTransport(TEST_CLIENT_DEVICE_CONFIG));

    //act
    IOTHUB_DEVICE_CLIENT_LL_HANDLE result = IoTHubDeviceClient_LL_CreateWithTransport(TEST_CLIENT_DEVICE_CONFIG);

    //assert
    ASSERT_IS_TRUE(result == TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

#ifdef USE_PROV_MODULE
TEST_FUNCTION(IoTHubDeviceClient_LL_CreateFromDeviceAuth_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateFromDeviceAuth(TEST_IOTHUB_URI, TEST_DEVICE_ID, TEST_TRANSPORT_PROVIDER));

    //act
    IOTHUB_DEVICE_CLIENT_LL_HANDLE result = IoTHubDeviceClient_LL_CreateFromDeviceAuth(TEST_IOTHUB_URI, TEST_DEVICE_ID, TEST_TRANSPORT_PROVIDER);

    //assert
    ASSERT_IS_TRUE(result == TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}
#endif

TEST_FUNCTION(IoTHubDeviceClient_LL_Destroy_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_Destroy(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE));

    //act
    IoTHubDeviceClient_LL_Destroy(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_SendEventAsync_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendEventAsync(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_MESSAGE_HANDLE, TEST_EVENT_CONFIRMATION_CALLBACK, NULL));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SendEventAsync(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_MESSAGE_HANDLE, TEST_EVENT_CONFIRMATION_CALLBACK, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_GetSendStatus_Test)
{
    //arrange
    IOTHUB_CLIENT_STATUS iothub_status;
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetSendStatus(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, &iothub_status));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_GetSendStatus(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, &iothub_status);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_SetMessageCallback_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetMessageCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_MESSAGE_CALLBACK_ASYNC, NULL));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SetMessageCallback(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_MESSAGE_CALLBACK_ASYNC, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_SetConnectionStatusCallback_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetConnectionStatusCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_CONNECTION_STATUS_CALLBACK, NULL));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SetConnectionStatusCallback(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_CONNECTION_STATUS_CALLBACK, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_SetRetryPolicy_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetRetryPolicy(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_RETRY_POLICY, TEST_SIZE_T));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SetRetryPolicy(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_RETRY_POLICY, TEST_SIZE_T);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_GetRetryPolicy_Test)
{
    //arrange
    IOTHUB_CLIENT_RETRY_POLICY retry;
    size_t timeout;
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetRetryPolicy(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, &retry, &timeout));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_GetRetryPolicy(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, &retry, &timeout);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_GetLastMessageReceiveTime_Test)
{
    //arrange
    time_t last_msg_time;
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetLastMessageReceiveTime(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, &last_msg_time));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_GetLastMessageReceiveTime(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, &last_msg_time);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_DoWork_Test)
{
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DoWork(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE));

    //act
    IoTHubDeviceClient_LL_DoWork(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_SetOption_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetOption(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_OPTION, TEST_VALUE));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SetOption(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_OPTION, TEST_VALUE);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_SetDeviceTwinCallback_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_TWIN_CALLBACK, NULL));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SetDeviceTwinCallback(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_TWIN_CALLBACK, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_SendReportedState_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendReportedState(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_UNSIGNED_CHAR, TEST_SIZE_T, TEST_REPORTED_STATE_CALLBACK, NULL));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SendReportedState(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_UNSIGNED_CHAR, TEST_SIZE_T, TEST_REPORTED_STATE_CALLBACK, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_GetTwinAsync_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetTwinAsync(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_TWIN_CALLBACK, NULL));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_GetTwinAsync(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_TWIN_CALLBACK, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
}

TEST_FUNCTION(IoTHubDeviceClient_LL_SetDeviceMethodCallback_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_DEVICE_METHOD_CALLBACK, NULL));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_SetDeviceMethodCallback(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_DEVICE_METHOD_CALLBACK, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_LL_DeviceMethodResponse_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_METHOD_HANDLE, TEST_UNSIGNED_CHAR, TEST_SIZE_T, TEST_INT));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_DeviceMethodResponse(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_METHOD_HANDLE, TEST_UNSIGNED_CHAR, TEST_SIZE_T, TEST_INT);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

#ifndef DONT_USE_UPLOADTOBLOB

TEST_FUNCTION(IoTHubDeviceClient_LL_UploadToBlob_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_UploadToBlob(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_CHAR_PTR, TEST_UNSIGNED_CHAR, TEST_SIZE_T));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_UploadToBlob(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_CHAR_PTR, TEST_UNSIGNED_CHAR, TEST_SIZE_T);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob_Test)
{
    //arrange
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_CHAR_PTR, TEST_FILE_UPLOAD_GET_DATA_CALLBACK_EX, NULL));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob(TEST_IOTHUB_DEVICE_CLIENT_LL_HANDLE, TEST_CHAR_PTR, TEST_FILE_UPLOAD_GET_DATA_CALLBACK_EX, NULL);

    //assert
    ASSERT_IS_TRUE(result == IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

#endif // !DONT_USE_UPLOADTOBLOB


END_TEST_SUITE(iothubdeviceclient_ll_ut)