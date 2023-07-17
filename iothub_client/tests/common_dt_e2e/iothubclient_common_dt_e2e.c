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

#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/xlogging.h"

#include "iothub.h"
#include "iothub_account.h"
#include "iothub_client_options.h"
#include "iothub_device_client.h"
#include "iothub_devicetwin.h"
#include "iothub_module_client.h"
#include "iothubtest.h"

#include "parson.h"
#include "testrunnerswitcher.h"


#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#define MAX_CLOUD_TRAVEL_TIME  120.0    // 2 minutes
#define BUFFER_SIZE            37
#define SLEEP_MS               5000
#define LONG_SLEEP_MS          30000
#define RETRY_COUNT            5

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);
TEST_DEFINE_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);

static IOTHUB_ACCOUNT_INFO_HANDLE iothub_accountinfo_handle = NULL;
static IOTHUB_DEVICE_CLIENT_HANDLE iothub_deviceclient_handle = NULL;
static IOTHUB_MODULE_CLIENT_HANDLE iothub_moduleclient_handle = NULL;

static const char* COMPLETE_DESIRED_PAYLOAD_FORMAT = // vs. PARTIAL DESIRED PAYLOAD FORMAT
    "{\"properties\":{\"desired\":{\"integer_property\": %d, \"string_property\": \"%s\","
        "\"array\": [%d, \"%s\"]}}}";
static const char* REPORTED_PAYLOAD_FORMAT =
    "{\"integer_property\": %d, \"string_property\": \"%s\", \"array\": [%d, \"%s\"] }";

//
// Test structures and parsing functions
//
typedef struct RECEIVED_TWIN_DATA_TAG
{
    bool received_callback;                     // True when device callback has been called
    bool update_state_set;                      // There is no "default" update_state to reset to
    DEVICE_TWIN_UPDATE_STATE update_state;      // Status reported by the callback
    unsigned char* cb_payload;
    size_t cb_payload_size;
    bool wasDisconnected;
    LOCK_HANDLE lock;
} RECEIVED_TWIN_DATA;

typedef struct REPORTED_TWIN_DATA_TAG
{
    char* string_property;
    int integer_property;
    bool received_callback;   // True when device callback has been called
    int status_code;         // Status reported by the callback
    LOCK_HANDLE lock;
} REPORTED_TWIN_DATA;

static int generate_new_int(void)
{
    return (int)time(NULL);
}

static char* generate_unique_string(void)
{
    char* return_value = (char*)calloc(BUFFER_SIZE, sizeof(char));
    ASSERT_IS_NOT_NULL(return_value);
    ASSERT_IS_TRUE(UniqueId_Generate(return_value, BUFFER_SIZE) == UNIQUEID_OK);
    return return_value;
}

static RECEIVED_TWIN_DATA* received_twin_data_init()
{
    RECEIVED_TWIN_DATA* twin_data = (RECEIVED_TWIN_DATA*)calloc(1, sizeof(RECEIVED_TWIN_DATA));
    ASSERT_IS_NOT_NULL(twin_data);

    twin_data->lock = Lock_Init();
    ASSERT_IS_NOT_NULL(twin_data->lock);

    return twin_data;
}

static void received_twin_data_reset(RECEIVED_TWIN_DATA* twin_data)
{
    ASSERT_IS_TRUE(Lock(twin_data->lock) == LOCK_OK);

    LogInfo("received_twin_data_reset(): Resetting data.");
    twin_data->received_callback = false;
    twin_data->update_state_set = false;
    free(twin_data->cb_payload);
    twin_data->cb_payload = NULL;
    twin_data->cb_payload_size = 0;
    twin_data->wasDisconnected = false;  

    Unlock(twin_data->lock);
}

static void received_twin_data_deinit(RECEIVED_TWIN_DATA* twin_data)
{
    free(twin_data->cb_payload);
    Lock_Deinit(twin_data->lock);
    free(twin_data);
}

static char* calloc_and_fill_service_client_desired_payload(const char* astring, int aint)
{
    size_t char_length = snprintf(NULL, 0, COMPLETE_DESIRED_PAYLOAD_FORMAT, aint, astring, aint, astring);
    ASSERT_IS_TRUE(char_length > 0);

    char* result = (char*)calloc(char_length + 1, sizeof(char));
    ASSERT_IS_NOT_NULL(result);

    size_t result_length = snprintf(result, char_length + 1, COMPLETE_DESIRED_PAYLOAD_FORMAT, aint, astring, aint, astring);
    ASSERT_IS_TRUE(result_length > 0);
    ASSERT_IS_TRUE(result_length <= char_length);

    return result;
}

static unsigned char* calloc_and_copy_unsigned_char(const unsigned char* payload, size_t size)
{
    unsigned char* result = (unsigned char*)calloc(1, size + 1);
    ASSERT_IS_NOT_NULL(result);
    memcpy(result, payload, size);
    return result;
}

static REPORTED_TWIN_DATA* reported_twin_data_init()
{
    REPORTED_TWIN_DATA* twin_data = (REPORTED_TWIN_DATA*)calloc(1, sizeof(REPORTED_TWIN_DATA));
    ASSERT_IS_NOT_NULL(twin_data);

    twin_data->lock = Lock_Init();
    ASSERT_IS_NOT_NULL(twin_data->lock);

    twin_data->string_property = generate_unique_string();
    ASSERT_IS_NOT_NULL(twin_data->string_property);

    twin_data->integer_property = generate_new_int();

    return twin_data;
}

static void reported_twin_data_reset(REPORTED_TWIN_DATA* twin_data)
{
    ASSERT_IS_TRUE(Lock(twin_data->lock) == LOCK_OK);

    LogInfo("reported_twin_data_reset(): Resetting data.");
    twin_data->received_callback = false;
    twin_data->status_code = 0;
    free(twin_data->string_property);
    twin_data->string_property = generate_unique_string();
    twin_data->integer_property = generate_new_int();

    Unlock(twin_data->lock);
}

static void reported_twin_data_deinit(REPORTED_TWIN_DATA* twin_data)
{
    free(twin_data->string_property);
    Lock_Deinit(twin_data->lock);
    free(twin_data);
}

static char* calloc_and_fill_reported_payload(const char* string, int num)
{
    size_t char_length = snprintf(NULL, 0, REPORTED_PAYLOAD_FORMAT, num, string, num, string);
    ASSERT_IS_TRUE(char_length > 0);

    char* result = (char*)calloc(char_length + 1, sizeof(char));
    ASSERT_IS_NOT_NULL(result);

    size_t result_length = snprintf(result, char_length + 1, REPORTED_PAYLOAD_FORMAT, num, string, num, string);
    ASSERT_IS_TRUE(result_length > 0);
    ASSERT_IS_TRUE(result_length <= char_length);

    return result;
}

static void verify_json_expected_string(const char* json,
                                        const char* full_property_name,
                                        const char* expected_str)
{
    JSON_Value* root_value = json_parse_string(json);
    ASSERT_IS_NOT_NULL(root_value);
    JSON_Object* root_object = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_object);

    const char* json_str = json_object_dotget_string(root_object, full_property_name);

    ASSERT_ARE_EQUAL(char_ptr, expected_str, json_str,
                     "String data in JSON differs from expected. Expected=<%s>, actual=<%s>", expected_str, json_str);

    json_value_free(root_value);
}

static void verify_json_expected_int(const char* json,
                                     const char* full_property_name,
                                     int expected_int)
{
    JSON_Value* root_value = json_parse_string(json);
    ASSERT_IS_NOT_NULL(root_value);
    JSON_Object* root_object = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_object);

    double double_value = json_object_dotget_number(root_object, full_property_name);
    double json_double = double_value + 0.1; // Account for possible underflow by small increment.

    ASSERT_ARE_EQUAL(int, expected_int, json_double,
                     "Integer data in JSON differs from expected. Expected=<%d>, actual=<%f>", expected_int, json_double);

    json_value_free(root_value);
}

static void verify_json_expected_string_array(const char* json,
                                              const char* full_property_name,
                                              size_t index,
                                              const char* expected_str)
{
    JSON_Value* root_value = json_parse_string(json);
    ASSERT_IS_NOT_NULL(root_value);
    JSON_Object* root_object = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_object);

    JSON_Array* array = json_object_dotget_array(root_object, full_property_name);
    ASSERT_IS_NOT_NULL(array, "Array not specified.");

    const char* json_str = json_array_get_string(array, index);

    ASSERT_ARE_EQUAL(char_ptr, expected_str, json_str,
                     "Array string data in JSON differs from expected. Expected=<%s>, actual=<%s>", expected_str, json_str);

    json_value_free(root_value);
}

static void verify_json_expected_int_array(const char* json,
                                           const char* full_property_name,
                                           size_t index,
                                           int expected_int)
{
    JSON_Value* root_value = json_parse_string(json);
    ASSERT_IS_NOT_NULL(root_value);
    JSON_Object* root_object = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_object);

    JSON_Array* array = json_object_dotget_array(root_object, full_property_name);
    ASSERT_IS_NOT_NULL(array, "Array not specified.");

    double double_value = json_array_get_number(array, index);
    double json_double = double_value + 0.1; // Account for possible underflow by small increment.

    ASSERT_ARE_EQUAL(int, expected_int, json_double,
                     "Array integer data in JSON differs from expected. Expected=<%d>, actual=<%f>", expected_int, json_double);

    json_value_free(root_value);
}

//
// Service Client APIs
//
static bool service_client_update_twin(IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle,
                                        IOTHUB_PROVISIONED_DEVICE* device_to_use,
                                        const char* twin_json)
{
    char* twin_response;
    bool result;

    LogInfo("service_client_update_twin(): Beginning update of twin via Service SDK.");

    if (device_to_use->moduleId != NULL)
    {
        twin_response = IoTHubDeviceTwin_UpdateModuleTwin(serviceclient_devicetwin_handle,
                                                          device_to_use->deviceId,
                                                          device_to_use->moduleId, twin_json);
    }
    else
    {
        twin_response = IoTHubDeviceTwin_UpdateTwin(serviceclient_devicetwin_handle,
                                                    device_to_use->deviceId, twin_json);
    }

    if (twin_response == NULL)
    {
        LogInfo("IoTHubDeviceTwin_Update(Module)Twin failed.");
        result = false;
    }
    else
    {
        LogInfo("service_client_update_twin(): Twin response from Service SDK after update is <%s>.",
            twin_response);
        free(twin_response);
        result = true;
    }

    return result;
}

static char* service_client_get_twin(IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle,
                                      IOTHUB_PROVISIONED_DEVICE* device_to_use)
{
    char* twin_data;

    if (device_to_use->moduleId != NULL)
    {
        twin_data = IoTHubDeviceTwin_GetModuleTwin(serviceclient_devicetwin_handle,
                                                   device_to_use->deviceId,
                                                   device_to_use->moduleId);
    }
    else
    {
        twin_data = IoTHubDeviceTwin_GetTwin(serviceclient_devicetwin_handle,
                                             device_to_use->deviceId);
    }

    ASSERT_IS_NOT_NULL(twin_data, "IoTHubDeviceTwin_Get(Module)Twin failed.");

    LogInfo("Twin data retrieved from Service SDK is <%s>.", twin_data);
    return twin_data;
}

//
// Device Client APIs & callbacks
//
// Invoked when a connection status changes.  Tests poll the status in the connection_status_info to make sure expected transitions occur.
static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    LogInfo("connection_status_callback: status=<%s>, reason=<%s>", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, status), MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));

    RECEIVED_TWIN_DATA* received_twin_data = (RECEIVED_TWIN_DATA*)userContextCallback;
    ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_OK, Lock(received_twin_data->lock));

    if (status == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED)
    {
        received_twin_data->wasDisconnected = true;
    }

    (void)Unlock(received_twin_data->lock);
}

static void set_client_option(const char* option_name,
                               const void* option_data,
                               const char* error_message)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle)
    {
        result = IoTHubModuleClient_SetOption(iothub_moduleclient_handle, option_name, option_data);
    }
    else
    {
        result = IoTHubDeviceClient_SetOption(iothub_deviceclient_handle, option_name, option_data);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, error_message);
}

static void create_client_handle(IOTHUB_PROVISIONED_DEVICE* device_to_use,
                        IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    ASSERT_IS_NULL(iothub_deviceclient_handle,
                   "iothub_deviceclient_handle is non-NULL on test initialization.");
    ASSERT_IS_NULL(iothub_moduleclient_handle,
                   "iothub_moduleclient_handle is non-NULL on test initialization.");

    if (device_to_use->moduleConnectionString)
    {
        iothub_moduleclient_handle = IoTHubModuleClient_CreateFromConnectionString(device_to_use->moduleConnectionString,
                                                                                   protocol);
        ASSERT_IS_NOT_NULL(iothub_moduleclient_handle,
                           "Could not invoke IoTHubModuleClient_CreateFromconnection_string.");
    }
    else
    {
        iothub_deviceclient_handle = IoTHubDeviceClient_CreateFromConnectionString(device_to_use->connectionString,
                                                                                   protocol);
        ASSERT_IS_NOT_NULL(iothub_deviceclient_handle,
                           "Could not invoke IoTHubDeviceClient_CreateFromconnection_string.");
    }

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    set_client_option(OPTION_TRUSTED_CERT, certificates, "Cannot enable trusted cert.");
#endif // SET_TRUSTED_CERT_IN_SAMPLES

    if (device_to_use->howToCreate == IOTHUB_ACCOUNT_AUTH_X509)
    {
        set_client_option(OPTION_X509_CERT, device_to_use->certificate,
                    "Could not set the device x509 certificate.");
        set_client_option(OPTION_X509_PRIVATE_KEY, device_to_use->primaryAuthentication,
                    "Could not set the device x509 privateKey.");
    }

    bool trace = true;
    set_client_option(OPTION_LOG_TRACE, &trace, "Cannot enable tracing.");
}

static void destroy_client_handle()
{
    LogInfo("destroy_client_handle(): Beginning to destroy IoTHub client handle.");
    if (iothub_moduleclient_handle)
    {
        IoTHubModuleClient_Destroy(iothub_moduleclient_handle);
        iothub_moduleclient_handle = NULL;
    }

    if (iothub_deviceclient_handle)
    {
        IoTHubDeviceClient_Destroy(iothub_deviceclient_handle);
        iothub_deviceclient_handle = NULL;
    }
    LogInfo("destroy_client_handle(): Completed destroy of IoTHub client handle.");
}

static void twin_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payload,
                           size_t size, void* user_context_callback)
{
    RECEIVED_TWIN_DATA* received_twin_data = (RECEIVED_TWIN_DATA*)user_context_callback;
    if (Lock(received_twin_data->lock) == LOCK_ERROR)
    {
        LogError("Lock failed.");
    }
    else
    {
        LogInfo("twin_callback():: Received JSON payload: len=<%lu>, data=<%.*s>. Updating state.",
                (unsigned long)size, (int)size, payload);

        received_twin_data->update_state = update_state;
        received_twin_data->update_state_set = true;
        if (received_twin_data->cb_payload != NULL)
        {
            free(received_twin_data->cb_payload);
        }

        received_twin_data->cb_payload = calloc_and_copy_unsigned_char(payload, size);
        received_twin_data->cb_payload_size = size;
        received_twin_data->received_callback = true;
        Unlock(received_twin_data->lock);
    }
}

static void set_twin_callback(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK twin_callback,
                               RECEIVED_TWIN_DATA* twin_data_context)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle)
    {
        result = IoTHubModuleClient_SetModuleTwinCallback(iothub_moduleclient_handle, twin_callback,
                                                          twin_data_context);
    }
    else
    {
        result = IoTHubDeviceClient_SetDeviceTwinCallback(iothub_deviceclient_handle, twin_callback,
                                                          twin_data_context);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result,
                     "IoTHub(Device|Module)Client_SetDeviceTwinCallback failed.");
}

static void get_twin_async(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK twin_callback,
                            RECEIVED_TWIN_DATA* received_twin_data)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle)
    {
        result = IoTHubModuleClient_GetTwinAsync(iothub_moduleclient_handle, twin_callback,
                                                 received_twin_data);
    }
    else
    {
        result = IoTHubDeviceClient_GetTwinAsync(iothub_deviceclient_handle, twin_callback,
                                                 received_twin_data);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result,
                     "IoTHub(Device|Module)Client_GetTwinAsync failed.");
}

static void receive_twin_loop(RECEIVED_TWIN_DATA* received_twin_data, DEVICE_TWIN_UPDATE_STATE expected_update_state)
{
    time_t begin_operation = time(NULL);
    time_t now_time;

    LogInfo("receive_twin_loop(): Entering loop with expected state %s.", MU_ENUM_TO_STRING(DEVICE_TWIN_UPDATE_STATE, expected_update_state));

    while (now_time = time(NULL), (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME))
    {
        ASSERT_IS_TRUE(Lock(received_twin_data->lock) == LOCK_OK);

        if ((received_twin_data->received_callback) &&
            (received_twin_data->cb_payload != NULL) &&
            (received_twin_data->update_state_set == true))
        {
            if (received_twin_data->wasDisconnected == false)
            {
                ASSERT_ARE_EQUAL(DEVICE_TWIN_UPDATE_STATE, expected_update_state,
                    received_twin_data->update_state);
            }

            Unlock(received_twin_data->lock);
            break;
        }
        Unlock(received_twin_data->lock);

        ThreadAPI_Sleep(1000);
    }

    ASSERT_IS_TRUE(difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME,
                   "Timeout waiting for twin message.");

    LogInfo("receive_twin_loop(): Exiting loop with state %s.", MU_ENUM_TO_STRING(DEVICE_TWIN_UPDATE_STATE, received_twin_data->update_state));
}

static void reported_state_callback(int status_code, void* user_context_callback)
{

    REPORTED_TWIN_DATA* reported_twin_data = (REPORTED_TWIN_DATA*) user_context_callback;
    if (Lock(reported_twin_data->lock) == LOCK_ERROR)
    {
        LogError("Lock failed.");
    }
    else
    {
        LogInfo("reported_state_callback():: Received status=<%d>", status_code);
        reported_twin_data->status_code = status_code;
        reported_twin_data->received_callback = true;
        Unlock(reported_twin_data->lock);
    }
}

static void send_reported_state(IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reported_state_callback,
                                 const char* buffer,
                                 REPORTED_TWIN_DATA* twin_data_context)
{
    IOTHUB_CLIENT_RESULT result;

    size_t bufferLen = strlen(buffer);

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SendReportedState(iothub_moduleclient_handle,
                                                      (unsigned char*) buffer, bufferLen,
                                                      reported_state_callback, twin_data_context);
    }
    else
    {
        result = IoTHubDeviceClient_SendReportedState(iothub_deviceclient_handle,
                                                      (unsigned char*) buffer, bufferLen,
                                                      reported_state_callback, twin_data_context);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result,
                     "IoTHub(Device|Module)Client_SendReportedState failed.");
}

static void receive_reported_state_acknowledgement_loop(REPORTED_TWIN_DATA* reported_twin_data)
{
    time_t begin_operation = time(NULL);
    time_t now_time;

    LogInfo("receive_reported_state_acknowledgement_loop(): Entering loop.");

    while (now_time = time(NULL), (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME))
    {
        // Receive IoT Hub response.
        ASSERT_IS_TRUE(Lock(reported_twin_data->lock) == LOCK_OK);

        if (reported_twin_data->received_callback)
        {
            ASSERT_IS_TRUE(reported_twin_data->status_code < 300,
                           "SendReported status_code (%d) is an error.",
                           reported_twin_data->status_code);

            Unlock(reported_twin_data->lock);
            break;
        }
        Unlock(reported_twin_data->lock);

        ThreadAPI_Sleep(1000);
    }
    ASSERT_IS_TRUE(difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME,
                   "Timeout waiting for twin message.");

    LogInfo("receive_reported_state_acknowledgement_loop(): Exiting loop.");
}

static void send_event_async(IOTHUB_MESSAGE_HANDLE msgHandle)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SendEventAsync(iothub_moduleclient_handle, msgHandle, NULL,
                                                   NULL);
    }
    else
    {
        result = IoTHubDeviceClient_SendEventAsync(iothub_deviceclient_handle, msgHandle, NULL,
                                                   NULL);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result,
                     "IoTHub(Device|Module)Client_SendEventAsync failed.");
}


void client_send_tcp_kill_via_d2c(IOTHUB_PROVISIONED_DEVICE* device)
{
    // IoT Hubs can be configured so that a special device telemetry message will cause them to
    // close the TCP connection. This is used to simulate network failures and check the SDK's
    // reconnection logic.

    char messageStr[512];
    int len = snprintf(messageStr, sizeof(messageStr), "Happy little message from device '%s'.",
                       device->deviceId);
    if (len < 0 || len == sizeof(messageStr))
    {
        ASSERT_FAIL("messageStr is not large enough.");
        return;
    }

    IOTHUB_MESSAGE_HANDLE msgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)messageStr, len);
    ASSERT_IS_NOT_NULL(msgHandle, "Could not create the D2C message to be sent.");
    MAP_HANDLE msgMapHandle = IoTHubMessage_Properties(msgHandle);
    ASSERT_IS_NOT_NULL(msgMapHandle);

    if (Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationType", "KillTcp") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!");
    }
    if (Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationCloseReason", "boom") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!");
    }
    if (Map_AddOrUpdate(msgMapHandle, "AzIoTHub_FaultOperationDelayInSecs", "1") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!");
    }

    LogInfo("Send fault control message...");
    send_event_async(msgHandle);

    IoTHubMessage_Destroy(msgHandle);
}

//
// dt_e2e Tests
//
void dt_e2e_init(bool testing_modules)
{
    int result = IoTHub_Init();
    ASSERT_ARE_EQUAL(int, 0, result, "IoTHub_Init failed");

    /* the return value from the second init is deliberatly ignored. */
    (void)IoTHub_Init();

    iothub_accountinfo_handle = IoTHubAccount_Init(testing_modules);
    ASSERT_IS_NOT_NULL(iothub_accountinfo_handle);
}

void dt_e2e_deinit(void)
{
    IoTHubAccount_deinit(iothub_accountinfo_handle);

    // Need a double deinit
    IoTHub_Deinit();
    IoTHub_Deinit();
}

void dt_e2e_send_reported_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                               IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    REPORTED_TWIN_DATA* reported_twin_data = reported_twin_data_init();

    create_client_handle(device_to_use, protocol);

    // Connect device client to IoT Hub.
    // Send reported payload. Register callback. Receive reported state message.
    char* reported_payload = calloc_and_fill_reported_payload(reported_twin_data->string_property,
                                                              reported_twin_data->integer_property);
    ASSERT_IS_NOT_NULL(reported_payload, "Failed to allocate and prepare the payload for SendReportedState.");

    send_reported_state(reported_state_callback, reported_payload, reported_twin_data);
    receive_reported_state_acknowledgement_loop(reported_twin_data);

    // Connect service client to IoT Hub to get twin.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                        "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    char* service_client_twin = service_client_get_twin(serviceclient_devicetwin_handle, device_to_use);

    // Check results.
    ASSERT_IS_TRUE(Lock(reported_twin_data->lock) == LOCK_OK);
    verify_json_expected_int(service_client_twin, "properties.reported.integer_property",
                                reported_twin_data->integer_property);
    verify_json_expected_string(service_client_twin, "properties.reported.string_property",
                                reported_twin_data->string_property);
    Unlock(reported_twin_data->lock);

    // Cleanup
    free(service_client_twin);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    free(reported_payload);
    destroy_client_handle();
    reported_twin_data_deinit(reported_twin_data);
}

void dt_e2e_get_complete_desired_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                      IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    RECEIVED_TWIN_DATA* received_twin_data = received_twin_data_init();

    create_client_handle(device_to_use, protocol);
    if (iothub_deviceclient_handle != NULL)
    {
        IOTHUB_CLIENT_RESULT result = IoTHubDeviceClient_SetConnectionStatusCallback(iothub_deviceclient_handle, connection_status_callback, received_twin_data);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHubDeviceClient_SetConnectionStatusCallback failed.");
    }
    else
    {
        IOTHUB_CLIENT_RESULT result = IoTHubModuleClient_SetConnectionStatusCallback(iothub_moduleclient_handle, connection_status_callback, received_twin_data);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHubModuleClient_SetConnectionStatusCallback failed.");
    }

    // There is a race condition between the service SDK updating the twin and the device
    // subscribing to the twin PATCH topic:
    // 1 - Register for the full twin (see set_twin_callback).
    // 2 - The client receives its full twin (see receive_twin_loop).
    // 3 - When the client receives full twin, it will register for PATCH changes
    // 4 - The service SDK updates the twin (see service_client_update_twin)
    //
    // If the server completes (4) *before* it receives the subscribe for PATCH (3), it will not
    // send down the PATCH of the full twin. On the server side, it seems that the MQTT subscribe
    // process completes faster than for AMQP.  To ensure the test passes for AMQP, add a sleep so
    // the server can receive the subscribe for twin PATCH before the service sdk updates the twin
    // on the server.

    // Connect device client to IoT Hub. Register callback. Receive full twin.
    set_twin_callback(twin_callback, received_twin_data);
    receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE);

    ThreadAPI_Sleep(LONG_SLEEP_MS);

    // Connect service client to IoT Hub to update twin.
    LogInfo("dt_e2e_get_complete_desired_test: Connecting to the hub service client.");
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                       "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    int expected_desired_integer;
    char* expected_desired_string = NULL;
    char* desired_payload = NULL;
    for (int i = 0; i < RETRY_COUNT; i++)
    {
        received_twin_data_reset(received_twin_data);
        if (desired_payload != NULL)
        {
            free(desired_payload);
        }
        if (expected_desired_string != NULL)
        {
            free(expected_desired_string);
        }

        expected_desired_integer = generate_new_int();
        expected_desired_string = generate_unique_string();
        desired_payload = calloc_and_fill_service_client_desired_payload(expected_desired_string,
                                                                               expected_desired_integer);
        ASSERT_IS_NOT_NULL(desired_payload,
                           "Failed to create the payload for IoTHubDeviceTwin_UpdateTwin.");

        LogInfo("dt_e2e_get_complete_desired_test: Updating device twin from service client.");
        if (service_client_update_twin(serviceclient_devicetwin_handle, device_to_use, desired_payload))
        {
            receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_PARTIAL);
            if (received_twin_data->wasDisconnected == false)
            {
                break;
            }
        }

        LogInfo("dt_e2e_get_complete_desired_test: Network was disconnected during twin update, retrying (%d).", i + 1);
        ThreadAPI_Sleep(LONG_SLEEP_MS);
    }

    // Check results.
    ASSERT_IS_TRUE(Lock(received_twin_data->lock) == LOCK_OK);
    verify_json_expected_int((char*)received_twin_data->cb_payload, "integer_property",
                              expected_desired_integer);
    verify_json_expected_string((char*)received_twin_data->cb_payload, "string_property",
                                expected_desired_string);
    verify_json_expected_int_array((char*)received_twin_data->cb_payload, "array", 0,
                                   expected_desired_integer);
    verify_json_expected_string_array((char*)received_twin_data->cb_payload, "array", 1,
                                      expected_desired_string);
    Unlock(received_twin_data->lock);

    // Cleanup
    free(desired_payload);
    free(expected_desired_string);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    destroy_client_handle();
    received_twin_data_deinit(received_twin_data);
}

void dt_e2e_send_reported_test_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                                       IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    REPORTED_TWIN_DATA* reported_twin_data = reported_twin_data_init();

    create_client_handle(device_to_use, protocol);

    // Connect device client to IoT Hub.
    // Send reported payload. Register callback. Receive reported state message.
    char* reported_payload = calloc_and_fill_reported_payload(reported_twin_data->string_property,
                                                              reported_twin_data->integer_property);
    ASSERT_IS_NOT_NULL(reported_payload,
                       "Failed to allocate and prepare the payload for SendReportedState.");

    send_reported_state(reported_state_callback, reported_payload, reported_twin_data);
    receive_reported_state_acknowledgement_loop(reported_twin_data);
    reported_twin_data_reset(reported_twin_data);

    // Send the Event from the device client
    client_send_tcp_kill_via_d2c(device_to_use);
    free(reported_payload);

    // Send a new reported payload. Register callback. Receive reported state message.
    reported_payload = calloc_and_fill_reported_payload(reported_twin_data->string_property,
                                                        reported_twin_data->integer_property);
    ASSERT_IS_NOT_NULL(reported_payload,
                       "Failed to allocate and prepare the payload for SendReportedState.");

    // Because the fault control message was sent asynchronously, the reported state call will
    // be PUBLISHed first unless there is a delay.
    ThreadAPI_Sleep(SLEEP_MS);

    send_reported_state(reported_state_callback, reported_payload, reported_twin_data);
    receive_reported_state_acknowledgement_loop(reported_twin_data);

    // Connect service client to IoT Hub to get twin.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                        "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    char* service_client_twin = service_client_get_twin(serviceclient_devicetwin_handle, device_to_use);

    // Check results.
    ASSERT_IS_TRUE(Lock(reported_twin_data->lock) == LOCK_OK);
    verify_json_expected_int(service_client_twin, "properties.reported.integer_property",
                                reported_twin_data->integer_property);
    verify_json_expected_string(service_client_twin, "properties.reported.string_property",
                                reported_twin_data->string_property);
    Unlock(reported_twin_data->lock);

    // Cleanup
    free(service_client_twin);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    free(reported_payload);
    destroy_client_handle();
    reported_twin_data_deinit(reported_twin_data);
}

void dt_e2e_get_complete_desired_test_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                                              IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    RECEIVED_TWIN_DATA* received_twin_data = received_twin_data_init();

    create_client_handle(device_to_use, protocol);

    // Connect device client to IoT Hub. Register callback. Receive full twin.
    set_twin_callback(twin_callback, received_twin_data);
    receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE);
    received_twin_data_reset(received_twin_data);

    // To ensure the test passes for AMQP, add a sleep so the server can receive the subscribe to
    // the twin PATCH topic before the service sdk updates the twin on the server.
    ThreadAPI_Sleep(SLEEP_MS);

    // Connect service client to IoT Hub to update twin.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                       "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    int expected_desired_integer = generate_new_int();
    char* expected_desired_string = generate_unique_string();
    char* desired_payload = calloc_and_fill_service_client_desired_payload(expected_desired_string,
                                                                           expected_desired_integer);
    ASSERT_IS_NOT_NULL(desired_payload,
                       "Failed to create the payload for IoTHubDeviceTwin_UpdateTwin.");

    service_client_update_twin(serviceclient_devicetwin_handle, device_to_use, desired_payload);
    receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_PARTIAL);
    received_twin_data_reset(received_twin_data);

    // Send the Event from the device client
    client_send_tcp_kill_via_d2c(device_to_use);
    receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE); // Sent when connection re-established.
    received_twin_data_reset(received_twin_data);

    // Sleep to allow for resubscribe to twin PATCH topic before updating service client twin.
    ThreadAPI_Sleep(SLEEP_MS);

    // Update service client twin again.
    service_client_update_twin(serviceclient_devicetwin_handle, device_to_use, desired_payload);
    receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_PARTIAL);

    // Check results.
    ASSERT_IS_TRUE(Lock(received_twin_data->lock) == LOCK_OK);
    verify_json_expected_int((char*)received_twin_data->cb_payload, "integer_property",
                              expected_desired_integer);
    verify_json_expected_string((char*)received_twin_data->cb_payload, "string_property",
                                expected_desired_string);
    verify_json_expected_int_array((char*)received_twin_data->cb_payload, "array", 0,
                                   expected_desired_integer);
    verify_json_expected_string_array((char*)received_twin_data->cb_payload, "array", 1,
                                      expected_desired_string);
    Unlock(received_twin_data->lock);

    // Cleanup
    free(desired_payload);
    free(expected_desired_string);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    destroy_client_handle();
    received_twin_data_deinit(received_twin_data);
}

void dt_e2e_get_twin_async_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    RECEIVED_TWIN_DATA* received_twin_data = received_twin_data_init();

    create_client_handle(device_to_use, protocol);

    // Connect device client to IoT Hub. Register callback. Receive full twin.
    get_twin_async(twin_callback, received_twin_data);
    receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE);

    // Check results.
    ASSERT_IS_TRUE(received_twin_data->cb_payload_size > 0);

    // Cleanup
    destroy_client_handle();
    received_twin_data_deinit(received_twin_data);
}

void dt_e2e_get_twin_async_destroy_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
    IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
        account_auth_method);
    RECEIVED_TWIN_DATA* received_twin_data = received_twin_data_init();

    create_client_handle(device_to_use, protocol);

    // Connect device client to IoT Hub. Register callback. 
    get_twin_async(twin_callback, received_twin_data);
    // destroy client handle, should get twin_callback
    destroy_client_handle();

    // Check results.
    ASSERT_IS_TRUE(received_twin_data->received_callback);
    ASSERT_IS_TRUE(received_twin_data->cb_payload_size == 0);

    // Cleanup
    received_twin_data_deinit(received_twin_data);
}

// dt_e2e_send_module_id_test makes sure that when OPTION_MODEL_ID is specified at creation time,
// that the Service Twin has it specified.
void dt_e2e_send_module_id_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method,
                                const char* model_id)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    RECEIVED_TWIN_DATA* received_twin_data = received_twin_data_init();

    create_client_handle(device_to_use, protocol);
    set_client_option(OPTION_MODEL_ID, model_id, "Cannot specify model_id."); // Set prior to network I/O.

    // Connect device client to IoT Hub. Register callback. Receive full twin.
    // We do not use the returned device twin, which doesn't contain the device's model_id.
    get_twin_async(twin_callback, received_twin_data);
    receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE);

    // Connect service client to IoT Hub to retrieve twin.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                       "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    char* service_client_twin_data = service_client_get_twin(serviceclient_devicetwin_handle, device_to_use);

    // Check results.
    verify_json_expected_string(service_client_twin_data, "modelId", model_id);

    // Cleanup
    free(service_client_twin_data);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    destroy_client_handle();
    received_twin_data_deinit(received_twin_data);
}
