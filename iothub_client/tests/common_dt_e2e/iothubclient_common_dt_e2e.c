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

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);

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
static int _generate_new_int(void)
{
    return (int)time(NULL);
}

static char* _generate_unique_string(void)
{
    char* return_value = (char*)malloc(BUFFER_SIZE);

    if (return_value == NULL)
    {
        LogError("malloc failed.");
    }
    else if (UniqueId_Generate(return_value, BUFFER_SIZE) != UNIQUEID_OK)
    {
        LogError("UniqueId_Generate failed.");
        free(return_value);
        return_value = NULL;
    }

    return return_value;
}

typedef struct RECEIVED_TWIN_DATA_TAG
{
    bool received_callback;                     // True when device callback has been called
    DEVICE_TWIN_UPDATE_STATE update_state;      // Status reported by the callback
    unsigned char* cb_payload;
    size_t cb_payload_size;
    LOCK_HANDLE lock;
} RECEIVED_TWIN_DATA;

static RECEIVED_TWIN_DATA* _received_twin_data_init()
{
    RECEIVED_TWIN_DATA* twin_data = (RECEIVED_TWIN_DATA*)malloc(sizeof(RECEIVED_TWIN_DATA));

    if (twin_data == NULL)
    {
        LogError("malloc failed.");
    }
    else
    {
        twin_data->lock = Lock_Init();
        if (twin_data->lock == NULL)
        {
            LogError("Lock_Init failed.");
            free(twin_data);
            twin_data = NULL;
        }
        else
        {
            twin_data->received_callback = false;
            twin_data->cb_payload = NULL;
            twin_data->cb_payload_size = 0;
        }
    }

    ASSERT_IS_NOT_NULL(twin_data, "Failed to create the twin_data.");

    return twin_data;
}

static void _received_twin_data_reset(RECEIVED_TWIN_DATA* twin_data)
{
    if (twin_data == NULL)
    {
        LogError("Invalid parameter `twin_data`.");
    }
    else if (Lock(twin_data->lock) == LOCK_ERROR)
    {
        LogError("Lock failed.");
    }
    else
    {
        twin_data->received_callback = false;
        free(twin_data->cb_payload);
        twin_data->cb_payload = NULL;
        twin_data->cb_payload_size = 0;

        (void) Unlock(twin_data->lock);
    }
}

static void _received_twin_data_deinit(RECEIVED_TWIN_DATA* twin_data)
{
    if (twin_data == NULL)
    {
        LogError("Invalid parameter `twin_data`.");
    }
    else
    {
        free(twin_data->cb_payload);
        Lock_Deinit(twin_data->lock);
        free(twin_data);
    }
}

static char* _malloc_and_fill_service_client_desired_payload(const char* astring, int aint)
{
    size_t length = snprintf(NULL, 0, COMPLETE_DESIRED_PAYLOAD_FORMAT, aint, astring, aint, astring);
    char* return_value = (char*) malloc(length + 1);

    if (return_value == NULL)
    {
        LogError("malloc failed.");
    }
    else
    {
        (void)sprintf(return_value, COMPLETE_DESIRED_PAYLOAD_FORMAT, aint, astring, aint, astring);
    }

    return return_value;
}

static unsigned char* _malloc_and_copy_unsigned_char(const unsigned char* payload, size_t size)
{
    unsigned char* return_value;

    if (payload == NULL)
    {
        LogError("Invalid parameter `payload`.");
        return_value = NULL;
    }
    else if (size < 1)
    {
        LogError("Invalid parameter `size`.");
        return_value = NULL;
    }
    else
    {
        unsigned char* temp = (unsigned char*)malloc(size + 1);
        if (temp == NULL)
        {
            LogError("malloc failed.");
            return_value = NULL;
        }
        else
        {
            return_value = (unsigned char*)memcpy(temp, payload, size);
            return_value[size] = '\0';
        }
    }

    return return_value;
}

typedef struct REPORTED_TWIN_DATA_TAG
{
    char* string_property;
    int integer_property;
    bool received_callback;   // True when device callback has been called
    int status_code;         // Status reported by the callback
    LOCK_HANDLE lock;
} REPORTED_TWIN_DATA;

static REPORTED_TWIN_DATA* _reported_twin_data_init()
{
    REPORTED_TWIN_DATA* twin_data = (REPORTED_TWIN_DATA*)malloc(sizeof(REPORTED_TWIN_DATA));

    if (twin_data == NULL)
    {
        LogError("malloc failed.");
    }
    else
    {
        twin_data->lock = Lock_Init();
        if (twin_data->lock == NULL)
        {
            LogError("Lock_Init failed.");
            free(twin_data);
            twin_data = NULL;
        }
        else
        {
            twin_data->string_property = _generate_unique_string();
            if (twin_data->string_property == NULL)
            {
                LogError("_generate_unique_string failed.");
                Lock_Deinit(twin_data->lock);
                free(twin_data);
                twin_data = NULL;
            }
            else
            {
                twin_data->integer_property = _generate_new_int();
                twin_data->received_callback = false;
                twin_data->status_code = 0;
            }
        }
    }

    ASSERT_IS_NOT_NULL(twin_data, "Failed to create the twin_data.");

    return twin_data;
}

static void _reported_twin_data_reset(REPORTED_TWIN_DATA* twin_data)
{
    if (twin_data == NULL)
    {
        LogError("Invalid parameter `twin_data`.");
    }
    else if (Lock(twin_data->lock) == LOCK_ERROR)
    {
        LogError("Lock failed.");
    }
    else
    {
        twin_data->received_callback = false;
        free(twin_data->string_property);
        twin_data->string_property = _generate_unique_string();
        twin_data->integer_property = _generate_new_int();

        (void) Unlock(twin_data->lock);
    }
}

static void _reported_twin_data_deinit(REPORTED_TWIN_DATA* twin_data)
{
    if (twin_data == NULL)
    {
        LogError("Invalid parameter `twin_data`.");
    }
    else
    {
        free(twin_data->string_property);
        Lock_Deinit(twin_data->lock);
        free(twin_data);
    }
}

static char* _malloc_and_fill_reported_payload(const char* string, int num)
{
    size_t length = snprintf(NULL, 0, REPORTED_PAYLOAD_FORMAT, num, string, num, string);
    char* return_value = (char*) malloc(length + 1);

    if (return_value == NULL)
    {
        LogError("malloc failed.");
    }
    else
    {
        (void)sprintf(return_value, REPORTED_PAYLOAD_FORMAT, num, string, num, string);
    }

    return return_value;
}

static char* _parse_json_twin_char(const char* twin_payload, const char* full_property_name)
{
    JSON_Value* root_value = json_parse_string(twin_payload);
    ASSERT_IS_NOT_NULL(root_value);
    JSON_Object* root_object = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_object);

    const unsigned char* value =  (unsigned char*)json_object_dotget_string(root_object,
                                                                            full_property_name);
    size_t length = json_object_dotget_string_len(root_object, full_property_name);
    char* return_value = (char*)_malloc_and_copy_unsigned_char(value, length);

    json_value_free(root_value);

    return return_value;
}

static int _parse_json_twin_int(const char* twin_payload,
                                const char* full_property_name,
                                bool allow_for_zero)
{
    JSON_Value* root_value = json_parse_string(twin_payload);
    ASSERT_IS_NOT_NULL(root_value);
    JSON_Object* root_object = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_object);

    double double_value = json_object_dotget_number(root_object, full_property_name);
    int int_value = (int)(double_value + 0.1); // Account for possible underflow by small increment.

    if (!allow_for_zero)
    {
        ASSERT_ARE_NOT_EQUAL(int, 0, int_value, "Failed to parse `%s`.", full_property_name);
    }

    json_value_free(root_value);

    return int_value;
}

static char* _parse_json_twin_char_from_array(const char* twin_payload,
                                              const char* full_property_name,
                                              size_t index)
{
    JSON_Value* root_value = json_parse_string(twin_payload);
    ASSERT_IS_NOT_NULL(root_value);
    JSON_Object* root_object = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_object);

    JSON_Array* array = json_object_dotget_array(root_object, full_property_name);
    ASSERT_IS_NOT_NULL(array, "Array not specified.");

    const unsigned char* value =  (unsigned char*)json_array_get_string(array, index);
    size_t length = json_array_get_string_len(array, index);
    char* return_value = (char*)_malloc_and_copy_unsigned_char(value, length);

    json_value_free(root_value);

    return return_value;
}

static int _parse_json_twin_int_from_array(const char* twin_payload,
                                           const char* full_property_name,
                                           size_t index,
                                           bool allow_for_zero)
{
    JSON_Value* root_value = json_parse_string(twin_payload);
    ASSERT_IS_NOT_NULL(root_value);
    JSON_Object* root_object = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_object);

    JSON_Array* array = json_object_dotget_array(root_object, full_property_name);
    ASSERT_IS_NOT_NULL(array, "Array not specified.");

    double double_value = json_array_get_number(array, index);
    int int_value = (int)(double_value + 0.1); // Account for possible underflow by small increment.

    if (!allow_for_zero)
    {
        ASSERT_ARE_NOT_EQUAL(int, 0, int_value, "Failed to parse `%s`.", full_property_name);
    }

    json_value_free(root_value);

    return int_value;
}

//
// Device Client APIs & callbacks
//
static void _set_client_option(const char* option_name,
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

static void _create_client_handle(IOTHUB_PROVISIONED_DEVICE* device_to_use,
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
    _set_client_option(OPTION_TRUSTED_CERT, certificates, "Cannot enable trusted cert.");
#endif // SET_TRUSTED_CERT_IN_SAMPLES

    if (device_to_use->howToCreate == IOTHUB_ACCOUNT_AUTH_X509)
    {
        _set_client_option(OPTION_X509_CERT, device_to_use->certificate,
                    "Could not set the device x509 certificate.");
        _set_client_option(OPTION_X509_PRIVATE_KEY, device_to_use->primaryAuthentication,
                    "Could not set the device x509 privateKey.");
    }

    bool trace = true;
    _set_client_option(OPTION_LOG_TRACE, &trace, "Cannot enable tracing.");
}

static void _destroy_client_handle()
{
    LogInfo("Beginning to destroy IoTHub client handle.");
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
    LogInfo("Completed destroy of IoTHub client handle.");
}

static void _twin_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payload,
                           size_t size, void* user_context_callback)
{
    LogInfo("Device Twin Callback:: Received JSON payload: len=<%lu>, data=<%.*s>.\n",
            (unsigned long)size, (int)size, payload);

    RECEIVED_TWIN_DATA* received_twin_data = (RECEIVED_TWIN_DATA*)user_context_callback;
    if (Lock(received_twin_data->lock) == LOCK_ERROR)
    {
        LogError("Lock failed.");
    }
    else
    {
        received_twin_data->update_state = update_state;
        if (received_twin_data->cb_payload != NULL)
        {
            free(received_twin_data->cb_payload);
        }
        received_twin_data->cb_payload = _malloc_and_copy_unsigned_char(payload, size);
        received_twin_data->cb_payload_size = size;
        received_twin_data->received_callback = true;
        Unlock(received_twin_data->lock);
    }
}

static void _receive_twin_loop(RECEIVED_TWIN_DATA* received_twin_data, DEVICE_TWIN_UPDATE_STATE expected_update_state)
{
    time_t begin_operation = time(NULL);
    time_t now_time;

    while (now_time = time(NULL), difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)
    {
        if (Lock(received_twin_data->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed");
        }
        else
        {
            if ((received_twin_data->received_callback) && (received_twin_data->cb_payload != NULL))
            {
                ASSERT_ARE_EQUAL(DEVICE_TWIN_UPDATE_STATE, received_twin_data->update_state,
                                 expected_update_state);

                Unlock(received_twin_data->lock);
                break;
            }
            Unlock(received_twin_data->lock);
        }
        ThreadAPI_Sleep(1000);
    }

    ASSERT_IS_TRUE(difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME,
                   "Timeout waiting for twin message.");
}

static void _set_twin_callback(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK twin_callback,
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

static void _get_twin_async(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK twin_callback,
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

static void _reported_state_callback(int status_code, void* user_context_callback)
{
    LogInfo("Reported State Callback:: Received status=<%d>\n", status_code);

    REPORTED_TWIN_DATA* reported_twin_data = (REPORTED_TWIN_DATA*) user_context_callback;
    if (Lock(reported_twin_data->lock) == LOCK_ERROR)
    {
        LogError("Lock failed.");
    }
    else
    {
        reported_twin_data->status_code = status_code;
        reported_twin_data->received_callback = true;
        Unlock(reported_twin_data->lock);
    }
}

static void _receive_reported_state_loop(REPORTED_TWIN_DATA* reported_twin_data)
{
    time_t begin_operation = time(NULL);
    time_t now_time;

    while (now_time = time(NULL), difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)
    {
        if (Lock(reported_twin_data->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed.");
        }
        else
        {
            if (reported_twin_data->received_callback)
            {
                Unlock(reported_twin_data->lock);
                break;
            }
            Unlock(reported_twin_data->lock);
        }
        ThreadAPI_Sleep(1000);
    }
    ASSERT_IS_TRUE(difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME,
                   "Timeout waiting for twin message.");
}

static void _send_reported_state(IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reported_state_callback,
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

static void _send_event_async(IOTHUB_MESSAGE_HANDLE msgHandle)
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


void _client_create_with_properties_and_send_d2c(IOTHUB_PROVISIONED_DEVICE* device_to_use,
                                                 MAP_HANDLE mapHandle)
{
    char messageStr[512];
    int len = snprintf(messageStr, sizeof(messageStr), "Happy little message from device '%s'.",
                       device_to_use->deviceId);
    if (len < 0 || len == sizeof(messageStr))
    {
        ASSERT_FAIL("messageStr is not large enough.");
        return;
    }

    IOTHUB_MESSAGE_HANDLE msgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)messageStr,
                                                                        len);
    ASSERT_IS_NOT_NULL(msgHandle, "Could not create the D2C message to be sent.");

    MAP_HANDLE msgMapHandle = IoTHubMessage_Properties(msgHandle);

    const char*const* keys;
    const char*const* values;
    size_t propCount;

    MAP_RESULT mapResult = Map_GetInternals(mapHandle, &keys, &values, &propCount);
    if (mapResult == MAP_OK)
    {
        for (size_t i = 0; i < propCount; i++)
        {
            if (Map_AddOrUpdate(msgMapHandle, keys[i], values[i]) != MAP_OK)
            {
                ASSERT_FAIL("Map_AddOrUpdate failed.");
            }
        }
    }

    _send_event_async(msgHandle);

    IoTHubMessage_Destroy(msgHandle);
}

//
// Service Client APIs
//
static void _service_client_update_twin(IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle,
                                        IOTHUB_PROVISIONED_DEVICE* device_to_use,
                                        const char* twin_json)
{
    char* twin_response;

    LogInfo("Beginning update of twin via Service SDK.");

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

    ASSERT_IS_NOT_NULL(twin_response, "IoTHubDeviceTwin_Update(Module)Twin failed.");

    LogInfo("Twin response from Service SDK after update is <%s>.\n", twin_response);
    free(twin_response);
}

static char* _service_client_get_twin(IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle,
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

    LogInfo("Twin data retrieved from Service SDK is <%s>.\n", twin_data);
    return twin_data;
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
    REPORTED_TWIN_DATA* reported_twin_data = _reported_twin_data_init();

    // Create device client (global)
    _create_client_handle(device_to_use, protocol);

    // Generate the reported payload.
    char* buffer = _malloc_and_fill_reported_payload(reported_twin_data->string_property,
                                                     reported_twin_data->integer_property);
    ASSERT_IS_NOT_NULL(buffer, "Failed to allocate and prepare the payload for SendReportedState.");

    // Connect device client to IoT Hub. Send reported payload. Register callback.
    _send_reported_state(_reported_state_callback, buffer, reported_twin_data);

    // Receive IoT Hub response.
    _receive_reported_state_loop(reported_twin_data);

    // Connect service client to IoT Hub.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                        "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    // Retrieve service client twin and compare with reported data sent.
    char* twin_data = _service_client_get_twin(serviceclient_devicetwin_handle, device_to_use);

    // Check results.
    if (Lock(reported_twin_data->lock) != LOCK_OK)
    {
        ASSERT_FAIL("Lock failed");
    }
    else
    {
        int status_code = 400;
        status_code = reported_twin_data->status_code;

        bool allow_for_zero = true;
        int integer_property = _parse_json_twin_int(twin_data,
                                                    "properties.reported.integer_property",
                                                    allow_for_zero);
        char* string_property = _parse_json_twin_char(twin_data,
                                                      "properties.reported.string_property");

        ASSERT_IS_TRUE(status_code < 300, "SendReported status_code is an error.");
        ASSERT_ARE_EQUAL(char_ptr, reported_twin_data->string_property, string_property,
                         "String data retrieved differs from reported.");
        ASSERT_ARE_EQUAL(int, reported_twin_data->integer_property, integer_property,
                         "Integer data retrieved differs from reported.");

        Unlock(reported_twin_data->lock);

        // Cleanup
        free(string_property);
    }

    // Cleanup
    free(twin_data);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    free(buffer);
    _destroy_client_handle();
    _reported_twin_data_deinit(reported_twin_data);
}

void dt_e2e_get_complete_desired_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                      IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    RECEIVED_TWIN_DATA* received_twin_data = _received_twin_data_init();

    // Create device client (global)
    _create_client_handle(device_to_use, protocol);

    // Connect device client to IoT Hub. Register callback.
    _set_twin_callback(_twin_callback, received_twin_data);

    // Twin registrations to the cloud happen asyncronously because we're using the convenience
    // layer. There is an (unavoidable) race potential in tests like this where we create handles
    // and immediately invoke the service SDK. Namely we could:
    // 1 - Register for the full twin (which happens via IoTHubDeviceClient_SetDeviceTwinCallback)
    // 2 - Have the service SDK update the twin (see _service_client_update_twin), but it takes a while
    // 3 - The client receives its full twin, which will just be empty data given (2) isn't completed
    // 4 - When the client receives full twin, it will register for PATCH changes
    // 5 - The server only now completes (2), setting the full twin.  However this has happened
    //     *after* it received the subscribe for PATCH and therefore it doesn't send down the PATCH
    //     of the full twin.
    // Apps in field will rarely hit this, as it requries service SDK & client handle to be invoked
    // almost simultaneously.  And the client *is* registered for future twin updates on this handle,
    // so it would get future changes.
    //
    // To ensure the test passes: Receive first full twin in the test app. Then sleep so the server
    // can receive the subscribe for PATCH. Then have the service sdk update the twin on the Hub.

    // Connect service client to IoT Hub.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                       "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    // Receive first full twin before having service client update it.
    _receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE);

    // Reset received_twin_data for update.
    _received_twin_data_reset(received_twin_data);

    // Sleep still needed. Twin callback has been recorded in test app, but the IoT Hub is still
    // completing the twin PATCH subscription.
    ThreadAPI_Sleep(5000);

    // Update service client twin to prompt a desired property PATCH message to device.
    char* expected_desired_string = _generate_unique_string();
    int expected_desired_integer = _generate_new_int();
    char* buffer = _malloc_and_fill_service_client_desired_payload(expected_desired_string,
                                                                   expected_desired_integer);
    ASSERT_IS_NOT_NULL(buffer, "Failed to create the payload for IoTHubDeviceTwin_UpdateTwin.");
    _service_client_update_twin(serviceclient_devicetwin_handle, device_to_use, buffer);

    // Receive IoT Hub response.
    _receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_PARTIAL);

    // Unsubscribe
    _set_twin_callback(NULL, NULL);

    // Check results.
    // Format:: {"$version":[value]}
    if (Lock(received_twin_data->lock) != LOCK_OK)
    {
        ASSERT_FAIL("Lock failed.");
    }
    else
    {
        bool allow_for_zero = true;
        int integer_property = _parse_json_twin_int((char*)received_twin_data->cb_payload,
                                                    "integer_property", allow_for_zero);
        char* string_property = _parse_json_twin_char((char*)received_twin_data->cb_payload,
                                                      "string_property");
        int integer_property_from_array = _parse_json_twin_int_from_array((char*)received_twin_data->cb_payload,
                                                                          "array", 0,
                                                                          allow_for_zero);
        char* string_property_from_array = _parse_json_twin_char_from_array((char*)received_twin_data->cb_payload,
                                                                            "array", 1);

        ASSERT_ARE_EQUAL(char_ptr, expected_desired_string, string_property,
                         "string data retrieved differs from expected");
        ASSERT_ARE_EQUAL(int, expected_desired_integer, integer_property,
                         "integer data retrieved differs from expected");
        ASSERT_ARE_EQUAL(char_ptr, expected_desired_string, string_property_from_array,
                         "string data (from array) retrieved differs from expected");
        ASSERT_ARE_EQUAL(int, expected_desired_integer, integer_property_from_array,
                         "integer data (from array) retrieved differs from expected");

        Unlock(received_twin_data->lock);

        // Cleanup
        free(string_property_from_array);
        free(string_property);
    }

    // Cleanup
    free(buffer);
    free(expected_desired_string);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    _destroy_client_handle();
    _received_twin_data_deinit(received_twin_data);
}

void dt_e2e_send_reported_test_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                                       IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    REPORTED_TWIN_DATA* reported_twin_data = _reported_twin_data_init();

    // Create device client (global)
    _create_client_handle(device_to_use, protocol);

    // Generate the reported payload.
    char* buffer = _malloc_and_fill_reported_payload(reported_twin_data->string_property,
                                                     reported_twin_data->integer_property);
    ASSERT_IS_NOT_NULL(buffer, "failed to allocate and prepare the payload for SendReportedState");

    // Connect device client to IoT Hub. Send reported payload. Register callback.
    _send_reported_state(_reported_state_callback, buffer, reported_twin_data);

    // Receive IoT Hub response.
    _receive_reported_state_loop(reported_twin_data);

    // Check results.
    if (Lock(reported_twin_data->lock) != LOCK_OK)
    {
        ASSERT_FAIL("Lock failed");
    }
    else
    {
        int status_code = 400;
        status_code = reported_twin_data->status_code;
        ASSERT_IS_TRUE(status_code < 300, "SendReported status_code is an error.");
    }

    // Send the Event from the device client
    MAP_HANDLE propMap = Map_Create(NULL);
    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationType", "KillTcp") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationCloseReason", "boom") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationDelayInSecs", "1") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!");
    }
    (void)printf("Send fault control message...\r\n");
    _client_create_with_properties_and_send_d2c(device_to_use, propMap);
    Map_Destroy(propMap);

    // Reset reported_twin_data for update.
    _reported_twin_data_reset(reported_twin_data);

    // Send reported payload (buffer) to IoT Hub again.
    _send_reported_state(_reported_state_callback, buffer, reported_twin_data);

    // Receive IoT Hub response.
    _receive_reported_state_loop(reported_twin_data);

    // Connect service client to IoT Hub.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                        "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    // Retrieve service client twin and compare with reported data sent.
    char* twin_data = _service_client_get_twin(serviceclient_devicetwin_handle, device_to_use);

    // Check results.
    if (Lock(reported_twin_data->lock) != LOCK_OK)
    {
        ASSERT_FAIL("Lock failed");
    }
    else
    {
        int status_code = 400;
        status_code = reported_twin_data->status_code;

        bool allow_for_zero = true;
        int integer_property = _parse_json_twin_int(twin_data,
                                                    "properties.reported.integer_property",
                                                    allow_for_zero);
        char* string_property = _parse_json_twin_char(twin_data,
                                                      "properties.reported.string_property");

        ASSERT_IS_TRUE(status_code < 300, "SendReported status_code is an error.");
        ASSERT_ARE_EQUAL(char_ptr, reported_twin_data->string_property, string_property,
                         "String data retrieved differs from reported.");
        ASSERT_ARE_EQUAL(int, reported_twin_data->integer_property, integer_property,
                         "Integer data retrieved differs from reported.");

        Unlock(reported_twin_data->lock);

        // Cleanup
        free(string_property);
    }

    // Cleanup
    free(twin_data);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    free(buffer);
    _destroy_client_handle();
    _reported_twin_data_deinit(reported_twin_data);
}

void dt_e2e_get_complete_desired_test_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                                              IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    RECEIVED_TWIN_DATA* received_twin_data = _received_twin_data_init();

    // Create device client (global)
    _create_client_handle(device_to_use, protocol);

    // Connect device client to IoT Hub. Register callback.
    _set_twin_callback(_twin_callback, received_twin_data);

    // Connect service client to IoT Hub.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                       "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    // Receive first full twin before having service client update it.
    _receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE);

    // Reset received_twin_data for update.
    _received_twin_data_reset(received_twin_data);

    // Sleep still needed. Twin callback has been recorded in test app, but the IoT Hub is still
    // completing the twin PATCH subscription.
    ThreadAPI_Sleep(5000);

    // Update service client twin to prompt a desired property PATCH message to device.
    char* expected_desired_string = _generate_unique_string();
    int expected_desired_integer = _generate_new_int();
    char* buffer = _malloc_and_fill_service_client_desired_payload(expected_desired_string,
                                                                   expected_desired_integer);
    ASSERT_IS_NOT_NULL(buffer, "Failed to create the payload for IoTHubDeviceTwin_UpdateTwin.");
    _service_client_update_twin(serviceclient_devicetwin_handle, device_to_use, buffer);

    // Receive IoT Hub response.
    _receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_PARTIAL);

    // Send the Event from the device client
    MAP_HANDLE propMap = Map_Create(NULL);
    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationType", "KillTcp") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationCloseReason", "boom") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationDelayInSecs", "1") != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!");
    }
    (void)printf("Send fault control message...\r\n");
    _client_create_with_properties_and_send_d2c(device_to_use, propMap);
    Map_Destroy(propMap);

    // Reset received_twin_data for update.
    _received_twin_data_reset(received_twin_data);

    ThreadAPI_Sleep(3000);

    // Update service client twin again.
    _service_client_update_twin(serviceclient_devicetwin_handle, device_to_use, buffer);

    // Receive IoT Hub response.
    _receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_PARTIAL);

    // Unsubscribe
    _set_twin_callback(NULL, NULL);

    // Check results.
    // Format:: {"$version":[value]}
    if (Lock(received_twin_data->lock) != LOCK_OK)
    {
        ASSERT_FAIL("Lock failed.");
    }
    else
    {
        bool allow_for_zero = true;
        int integer_property = _parse_json_twin_int((char*)received_twin_data->cb_payload,
                                                    "integer_property", allow_for_zero);
        char* string_property = _parse_json_twin_char((char*)received_twin_data->cb_payload,
                                                      "string_property");
        int integer_property_from_array = _parse_json_twin_int_from_array((char*)received_twin_data->cb_payload,
                                                                          "array", 0,
                                                                          allow_for_zero);
        char* string_property_from_array = _parse_json_twin_char_from_array((char*)received_twin_data->cb_payload,
                                                                            "array", 1);

        ASSERT_ARE_EQUAL(char_ptr, expected_desired_string, string_property,
                         "string data retrieved differs from expected");
        ASSERT_ARE_EQUAL(int, expected_desired_integer, integer_property,
                         "integer data retrieved differs from expected");
        ASSERT_ARE_EQUAL(char_ptr, expected_desired_string, string_property_from_array,
                         "string data (from array) retrieved differs from expected");
        ASSERT_ARE_EQUAL(int, expected_desired_integer, integer_property_from_array,
                         "integer data (from array) retrieved differs from expected");
        Unlock(received_twin_data->lock);

        // Cleanup
        free(string_property_from_array);
        free(string_property);
    }

    // Cleanup
    free(buffer);
    free(expected_desired_string);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    _destroy_client_handle();
    _received_twin_data_deinit(received_twin_data);
}

void dt_e2e_get_twin_async_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol,
                                IOTHUB_ACCOUNT_AUTH_METHOD account_auth_method)
{
    // Test setup
    IOTHUB_PROVISIONED_DEVICE* device_to_use = IoTHubAccount_GetDevice(iothub_accountinfo_handle,
                                                                       account_auth_method);
    RECEIVED_TWIN_DATA* received_twin_data = _received_twin_data_init();

    // Create device client (global)
    _create_client_handle(device_to_use, protocol);

    // Connect device client to IoT Hub. Register callback.
    _get_twin_async(_twin_callback, received_twin_data);

    // Receive first full twin.
    _receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE);

   // Check results.
    ASSERT_IS_TRUE(received_twin_data->cb_payload_size > 0);

    // Cleanup
    _destroy_client_handle();
    _received_twin_data_deinit(received_twin_data);
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
    RECEIVED_TWIN_DATA* received_twin_data = _received_twin_data_init();

    // Create device client (global)
    _create_client_handle(device_to_use, protocol);
    _set_client_option(OPTION_MODEL_ID, model_id, "Cannot specify model_id."); // Set prior to network I/O.

    // Connect device to IoT Hub.
    // We do not use the returned device twin, which doesn't contain the device's model_id.
    _get_twin_async(_twin_callback, received_twin_data);

    // Receive IoT Hub response before continuing with test.
    _receive_twin_loop(received_twin_data, DEVICE_TWIN_UPDATE_COMPLETE);

    // Connect service client to IoT Hub.
    const char* connection_string = IoTHubAccount_GetIoTHubConnString(iothub_accountinfo_handle);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_serviceclient_handle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
    ASSERT_IS_NOT_NULL(iothub_serviceclient_handle,
                       "IoTHubServiceClientAuth_CreateFromConnectionString failed.");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceclient_devicetwin_handle = IoTHubDeviceTwin_Create(iothub_serviceclient_handle);
    ASSERT_IS_NOT_NULL(serviceclient_devicetwin_handle, "IoTHubDeviceTwin_Create failed.");

    // Get Twin data viasService client and compare model_id.
    char* twin_data = _service_client_get_twin(serviceclient_devicetwin_handle, device_to_use);
    char* parsed_model_id = _parse_json_twin_char(twin_data, "modelId");
    ASSERT_ARE_EQUAL(char_ptr, model_id, parsed_model_id);

    // Cleanup
    free(parsed_model_id);
    free(twin_data);
    IoTHubDeviceTwin_Destroy(serviceclient_devicetwin_handle);
    IoTHubServiceClientAuth_Destroy(iothub_serviceclient_handle);
    _destroy_client_handle();
    _received_twin_data_deinit(received_twin_data);
}
