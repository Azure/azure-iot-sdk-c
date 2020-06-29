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

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_module_client.h"
#include "iothub_client_options.h"
#include "iothub_devicetwin.h"

#include "iothub_account.h"
#include "iothubtest.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "parson.h"
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#define MAX_CLOUD_TRAVEL_TIME  120.0    /* 2 minutes */
#define BUFFER_SIZE            37

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);

typedef struct DEVICE_REPORTED_DATA_TAG
{
    char *string_property;
    int   integer_property;
    bool receivedCallBack;   // true when device callback has been called
    int  status_code;        // status reported by the callback
    LOCK_HANDLE lock;
} DEVICE_REPORTED_DATA;


static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo = NULL;
static IOTHUB_DEVICE_CLIENT_HANDLE iothub_deviceclient_handle = NULL;
static IOTHUB_MODULE_CLIENT_HANDLE iothub_moduleclient_handle = NULL;


static void reportedStateCallback(int status_code, void* userContextCallback)
{
    DEVICE_REPORTED_DATA *device = (DEVICE_REPORTED_DATA *) userContextCallback;
    if (Lock(device->lock) == LOCK_ERROR)
    {
        LogError("Lock failed");
    }
    else
    {
        device->status_code = status_code;
        device->receivedCallBack = true;
        (void) Unlock(device->lock);
    }
}

static int generate_new_int(void)
{
    int retValue;
    time_t nowTime = time(NULL);

    retValue = (int) nowTime;
    return retValue;
}

static char *generate_unique_string(void)
{
    char *retValue;

    retValue = (char *) malloc(BUFFER_SIZE);
    if (retValue == NULL)
    {
        LogError("malloc failed");
    }
    else if (UniqueId_Generate(retValue, BUFFER_SIZE) != UNIQUEID_OK)
    {
        LogError("UniqueId_Generate failed");
        free(retValue);
        retValue = NULL;
    }
    return retValue;
}

static DEVICE_REPORTED_DATA *device_reported_init()
{
    DEVICE_REPORTED_DATA *retValue;

    if ((retValue = (DEVICE_REPORTED_DATA *) malloc(sizeof(DEVICE_REPORTED_DATA))) == NULL)
    {
        LogError("malloc failed");
    }
    else
    {
        retValue->lock = Lock_Init();
        if (retValue->lock == NULL)
        {
            LogError("Lock_Init failed");
            free(retValue);
            retValue = NULL;
        }
        else
        {
            retValue->string_property = generate_unique_string();
            if (retValue->string_property == NULL)
            {
                LogError("generate unique string failed");
                Lock_Deinit(retValue->lock);
                free(retValue);
                retValue = NULL;
            }
            else
            {
                retValue->receivedCallBack = false;
                retValue->integer_property = generate_new_int();
            }
        }
    }
    return retValue;
}

static void device_reported_deinit(DEVICE_REPORTED_DATA *device)
{
    if (device == NULL)
    {
        LogError("invalid parameter device");
    }
    else
    {
        free(device->string_property);
        Lock_Deinit(device->lock);
        free(device);
    }
}

void dt_e2e_init(bool testing_modules)
{
    int result = IoTHub_Init();
    ASSERT_ARE_EQUAL(int, 0, result, "IoTHub_Init failed");

    /* the return value from the second init is deliberatly ignored. */
    (void)IoTHub_Init();

    g_iothubAcctInfo = IoTHubAccount_Init(testing_modules);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo);
}

void dt_e2e_deinit(void)
{
    IoTHubAccount_deinit(g_iothubAcctInfo);

    // Need a double deinit
    IoTHub_Deinit();
    IoTHub_Deinit();
}

typedef struct DEVICE_DESIRED_DATA_TAG
{
    bool receivedCallBack;                     // true when device callback has been called
    DEVICE_TWIN_UPDATE_STATE update_state;     // status reported by the callback
    char *cb_payload;
    LOCK_HANDLE lock;
} DEVICE_DESIRED_DATA;


static const char *REPORTED_PAYLOAD_FORMAT = "{\"integer_property\": %d, \"string_property\": \"%s\", \"array\": [%d, \"%s\"] }";
static char *malloc_and_fill_reported_payload(const char *string, int aint)
{
    size_t  length = snprintf(NULL, 0, REPORTED_PAYLOAD_FORMAT, aint, string, aint, string);
    char   *retValue = (char *) malloc(length + 1);
    if (retValue == NULL)
    {
        LogError("malloc failed");
    }
    else
    {
        (void) sprintf(retValue, REPORTED_PAYLOAD_FORMAT, aint, string, aint, string);
    }
    return retValue;
}

static void setoption_on_device_or_module(const char * optionName, const void * optionData, const char * errorMessage)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SetOption(iothub_moduleclient_handle, optionName, optionData);
    }
    else
    {
        result = IoTHubDeviceClient_SetOption(iothub_deviceclient_handle, optionName, optionData);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, errorMessage);
}

static void dt_e2e_create_client_handle(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    ASSERT_IS_NULL(iothub_deviceclient_handle, "iothub_deviceclient_handle is non-NULL on test initialization");
    ASSERT_IS_NULL(iothub_moduleclient_handle, "iothub_moduleclient_handle is non-NULL on test initialization");

    if (deviceToUse->moduleConnectionString != NULL)
    {
        iothub_moduleclient_handle = IoTHubModuleClient_CreateFromConnectionString(deviceToUse->moduleConnectionString, protocol);
        ASSERT_IS_NOT_NULL(iothub_moduleclient_handle, "Could not invoke IoTHubModuleClient_CreateFromConnectionString");
    }
    else
    {
        iothub_deviceclient_handle = IoTHubDeviceClient_CreateFromConnectionString(deviceToUse->connectionString, protocol);
        ASSERT_IS_NOT_NULL(iothub_deviceclient_handle, "Could not invoke IoTHubDeviceClient_CreateFromConnectionString");
    }

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    setoption_on_device_or_module(OPTION_TRUSTED_CERT, certificates, "Cannot enable trusted cert");
#endif // SET_TRUSTED_CERT_IN_SAMPLES

    if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509)
    {
        setoption_on_device_or_module(OPTION_X509_CERT, deviceToUse->certificate, "Could not set the device x509 certificate");
        setoption_on_device_or_module(OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication, "Could not set the device x509 privateKey");
    }

    bool trace = true;
    setoption_on_device_or_module(OPTION_LOG_TRACE, &trace, "Cannot enable tracing");
}

static void destroy_on_device_or_module()
{
    LogInfo("Beginning to destroy IotHub client handle");
    if (iothub_deviceclient_handle != NULL)
    {
        IoTHubDeviceClient_Destroy(iothub_deviceclient_handle);
        iothub_deviceclient_handle = NULL;
    }

    if (iothub_moduleclient_handle != NULL)
    {
        IoTHubModuleClient_Destroy(iothub_moduleclient_handle);
        iothub_moduleclient_handle = NULL;
    }
    LogInfo("Completed destroy of IotHub client handle");
}

static void sendreportedstate_on_device_or_module(const char* buffer, DEVICE_REPORTED_DATA *device)
{
    IOTHUB_CLIENT_RESULT result;

    size_t bufferLen = strlen(buffer);

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SendReportedState(iothub_moduleclient_handle, (unsigned char *) buffer, bufferLen, reportedStateCallback, device);
    }
    else
    {
        result = IoTHubDeviceClient_SendReportedState(iothub_deviceclient_handle, (unsigned char *) buffer, bufferLen, reportedStateCallback, device);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHub(Device|Module)Client_SendReportedState failed");
}

static void setdevicetwincallback_on_device_or_module(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK twinCallback, DEVICE_DESIRED_DATA *device)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SetModuleTwinCallback(iothub_moduleclient_handle, twinCallback, device);
    }
    else
    {
        result = IoTHubDeviceClient_SetDeviceTwinCallback(iothub_deviceclient_handle, twinCallback, device);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "IoTHub(Device|Module)Client_SetDeviceTwinCallback failed");
}

static void sendeventasync_on_device_or_module(IOTHUB_MESSAGE_HANDLE msgHandle)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SendEventAsync(iothub_moduleclient_handle, msgHandle, NULL, NULL);
    }
    else
    {
        result = IoTHubDeviceClient_SendEventAsync(iothub_deviceclient_handle, msgHandle, NULL, NULL);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "SendEventAsync failed");
}



static void dt_e2e_update_twin(IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceClientDeviceTwinHandle, IOTHUB_PROVISIONED_DEVICE* deviceToUse, const char* twinJson)
{
    char* twinResponse;

    LogInfo("Beginning update of twin via Service SDK");

    if (deviceToUse->moduleId != NULL)
    {
        twinResponse = IoTHubDeviceTwin_UpdateModuleTwin(serviceClientDeviceTwinHandle, deviceToUse->deviceId, deviceToUse->moduleId, twinJson);
        ASSERT_IS_NOT_NULL(twinResponse, "IoTHubDeviceTwin_UpdateModuleTwin failed");
    }
    else
    {
        twinResponse = IoTHubDeviceTwin_UpdateTwin(serviceClientDeviceTwinHandle, deviceToUse->deviceId, twinJson);
        ASSERT_IS_NOT_NULL(twinResponse, "IoTHubDeviceTwin_UpdateTwin failed");
    }

    LogInfo("Twin response from Service SDK after update is <%s>\n", twinResponse);
    free(twinResponse);
}

static char* dt_e2e_get_twin(IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceClientDeviceTwinHandle, IOTHUB_PROVISIONED_DEVICE* deviceToUse)
{
    char *twinData;

    if (deviceToUse->moduleId != NULL)
    {
        twinData = IoTHubDeviceTwin_GetModuleTwin(serviceClientDeviceTwinHandle, deviceToUse->deviceId, deviceToUse->moduleId);
        ASSERT_IS_NOT_NULL(twinData, "IoTHubDeviceTwin_GetModuleTwin failed");
    }
    else
    {
        twinData = IoTHubDeviceTwin_GetTwin(serviceClientDeviceTwinHandle, deviceToUse->deviceId);
        ASSERT_IS_NOT_NULL(twinData, "IoTHubDeviceTwin_GetModuleTwin failed");
    }

    return twinData;
}


void dt_e2e_send_reported_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod)
{
    // arrange
    IOTHUB_PROVISIONED_DEVICE* deviceToUse;
    if (accountAuthMethod == IOTHUB_ACCOUNT_AUTH_X509)
    {
        deviceToUse = IoTHubAccount_GetX509Device(g_iothubAcctInfo);
    }
    else
    {
        deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    }

    DEVICE_REPORTED_DATA *device = device_reported_init();
    ASSERT_IS_NOT_NULL(device, "failed to create the device client data");

    // Create the IoT Hub Data
    dt_e2e_create_client_handle(deviceToUse, protocol);

    // generate the payload
    char *buffer = malloc_and_fill_reported_payload(device->string_property, device->integer_property);
    ASSERT_IS_NOT_NULL(buffer, "failed to allocate and prepare the payload for SendReportedState");

    // act
    sendreportedstate_on_device_or_module(buffer, device);

    int status_code = 400;
    time_t beginOperation, nowTime;
    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(device->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed");
        }
        else
        {
            if (device->receivedCallBack)
            {
                status_code = device->status_code;
                Unlock(device->lock);
                break;
            }
            Unlock(device->lock);
        }
        ThreadAPI_Sleep(1000);
    }

    if (Lock(device->lock) != LOCK_OK)
    {
        ASSERT_FAIL("Lock failed");
    }
    else
    {
        ASSERT_IS_TRUE(status_code < 300, "SendReported status_code is an error");

        const char *connectionString = IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo);
        IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
        ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "IoTHubServiceClientAuth_CreateFromConnectionString failed");

        IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceClientDeviceTwinHandle = IoTHubDeviceTwin_Create(iotHubServiceClientHandle);
        ASSERT_IS_NOT_NULL(serviceClientDeviceTwinHandle, "IoTHubDeviceTwin_Create failed");

        char *twinData = dt_e2e_get_twin(serviceClientDeviceTwinHandle, deviceToUse);

        JSON_Value *root_value = json_parse_string(twinData);
        ASSERT_IS_NOT_NULL(root_value, "json_parse_string failed");

        JSON_Object *root_object = json_value_get_object(root_value);
        const char *string_property = json_object_dotget_string(root_object, "properties.reported.string_property");
        ASSERT_ARE_EQUAL(char_ptr, device->string_property, string_property, "string data retrieved differs from reported");

        int integer_property = (int) json_object_dotget_number(root_object, "properties.reported.integer_property");
        ASSERT_ARE_EQUAL(int, device->integer_property, integer_property, "integer data retrieved differs from reported");

        (void) Unlock(device->lock);

        // cleanup
        json_value_free(root_value);
        free(twinData);
        free(buffer);
        IoTHubDeviceTwin_Destroy(serviceClientDeviceTwinHandle);
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
        destroy_on_device_or_module();
        device_reported_deinit(device);
    }
}

 


static const char *COMPLETE_DESIRED_PAYLOAD_FORMAT = "{\"properties\":{\"desired\":{\"integer_property\": %d, \"string_property\": \"%s\", \"array\": [%d, \"%s\"]}}}";
static char *malloc_and_fill_desired_payload(const char *string, int aint)
{
    size_t  length = snprintf(NULL, 0, COMPLETE_DESIRED_PAYLOAD_FORMAT, aint, string, aint, string);
    char   *retValue = (char *) malloc(length + 1);
    if (retValue == NULL)
    {
        LogError("malloc failed");
    }
    else
    {
        (void) sprintf(retValue, COMPLETE_DESIRED_PAYLOAD_FORMAT, aint, string, aint, string);
    }
    return retValue;
}

static char * malloc_and_copy_unsigned_char(const unsigned char* payload, size_t size)
{
    char *retValue;
    if (payload == NULL)
    {
        LogError("invalid parameter payload");
        retValue = NULL;
    }
    else if (size < 1)
    {
        LogError("invalid parameter size");
        retValue = NULL;
    }
    else
    {
        char *temp = (char *) malloc(size + 1);
        if (temp == NULL)
        {
            LogError("malloc failed");
            retValue = NULL;
        }
        else
        {
            retValue = (char *) memcpy(temp, payload, size);
            temp[size] = '\0';
        }
    }
    return retValue;
}

static void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payload, size_t size, void* userContextCallback)
{
    LogInfo("Callback:: Received payload len=<%lu>, data=<%.*s>\n", (unsigned long)size, (int)size, payload);
    DEVICE_DESIRED_DATA *device = (DEVICE_DESIRED_DATA *)userContextCallback;
    if (Lock(device->lock) == LOCK_ERROR)
    {
        LogError("Lock failed");
    }
    else
    {
        device->update_state = update_state;
        device->receivedCallBack = true;
        if (device->cb_payload != NULL)
        {
            free(device->cb_payload);
        }
        device->cb_payload = malloc_and_copy_unsigned_char(payload, size);
        (void) Unlock(device->lock);
    }
}

static DEVICE_DESIRED_DATA *device_desired_init()
{
    DEVICE_DESIRED_DATA *retValue;

    if ((retValue = (DEVICE_DESIRED_DATA *) malloc(sizeof(DEVICE_DESIRED_DATA))) == NULL)
    {
        LogError("malloc failed");
    }
    else
    {
        retValue->lock = Lock_Init();
        if (retValue->lock == NULL)
        {
            LogError("Lock_Init failed");
            free(retValue);
            retValue = NULL;
        }
        else
        {
            retValue->receivedCallBack = false;
            retValue->cb_payload = NULL;
        }
    }
    return retValue;
}

static void device_desired_deinit(DEVICE_DESIRED_DATA *device)
{
    if (device == NULL)
    {
        LogError("invalid parameter device");
    }
    else
    {
        free(device->cb_payload);
        Lock_Deinit(device->lock);
        free(device);
    }
}

void client_create_with_properies_and_send_d2c(MAP_HANDLE mapHandle)
{
    IOTHUB_MESSAGE_HANDLE msgHandle;

    const char* messageStr = "Happy little message";
    msgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)messageStr, strlen(messageStr));
    ASSERT_IS_NOT_NULL(msgHandle, "Could not create the D2C message to be sent");

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
                ASSERT_FAIL("Map_AddOrUpdate failed!");
            }
        }
    }

    // act
    sendeventasync_on_device_or_module(msgHandle);
    IoTHubMessage_Destroy(msgHandle);
}

static int dt_e2e_parse_twin_version(const char *twinData, bool jsonFromGetTwin)
{
    // The twin JSON we get depends on context (callback versus a GetTwin call)
    const char *jsonToQuery = jsonFromGetTwin ? "properties.desired.$version" : "desired.$version";

    JSON_Value *root_value = json_parse_string(twinData);
    ASSERT_IS_NOT_NULL(root_value, "json_parse_string failed");

    JSON_Object *root_object = json_value_get_object(root_value);
    double double_version = json_object_dotget_number(root_object, jsonToQuery);
    int int_version = (int)(double_version + 0.1); // Account for possible underflow by small increment and then int typecast.

    if ((int_version == 0) && (jsonFromGetTwin == false))
    {
        // Sometimes we're invoked after a patch which means different JSON
        double_version = json_object_dotget_number(root_object, "$version");
        int_version = (int)(double_version + 0.1);
    }

    ASSERT_ARE_NOT_EQUAL(int, 0, int_version);

    // cleanup
    json_value_free(root_value);
    return int_version;
}

static int dt_e2e_gettwin_version(IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceClientDeviceTwinHandle, IOTHUB_PROVISIONED_DEVICE* deviceToUse)
{
    char *twinData = dt_e2e_get_twin(serviceClientDeviceTwinHandle, deviceToUse);
    int version = dt_e2e_parse_twin_version(twinData, true);

    // cleanup
    free(twinData);
    return version;
}

void dt_e2e_get_complete_desired_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod)
{
    // arrange
    IOTHUB_PROVISIONED_DEVICE* deviceToUse;
    if (accountAuthMethod == IOTHUB_ACCOUNT_AUTH_X509)
    {
        deviceToUse = IoTHubAccount_GetX509Device(g_iothubAcctInfo);
    }
    else
    {
        deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    }

    DEVICE_DESIRED_DATA *device = device_desired_init();
    ASSERT_IS_NOT_NULL(device, "failed to create the device client data");

    dt_e2e_create_client_handle(deviceToUse, protocol);

    // subscribe
    setdevicetwincallback_on_device_or_module(deviceTwinCallback, device);

    // Twin registrations to the cloud happen asyncronously because we're using the convenience layer.  There is an (unavoidable)
    // race potential in tests like this where we create handles and immediately invoke the service SDK.  Namely without this
    // sleep, we could:
    // 1 - Register for the full twin (which happens via IoTHubDeviceClient_SetDeviceTwinCallback)
    // 2 - Have the service SDK update the twin (see dt_e2e_update_twin), but it takes a while
    // 3 - The client receives its full twin, which will just be empty data given (2) isn't completed
    // 4 - When the client receives full twin, it will register for PATCH changes
    // 5 - The server only now completes (2), setting the full twin.  However this has happened *before* it received
    //     the subscribe for PATCH and therefore it doesn't send down the PATCH of the full twin.
    // Apps in field will rarely hit this, as it requries service SDK & client handle to be invoked almost simultaneously.
    // And the client *is* registered for future twin updates on this handle, so it would get future changes.
    LogInfo("Sleeping for a few seconds as client-side registers with twin");
    ThreadAPI_Sleep(5000);

    const char *connectionString = IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "IoTHubServiceClientAuth_CreateFromConnectionString failed");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceClientDeviceTwinHandle = IoTHubDeviceTwin_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL(serviceClientDeviceTwinHandle, "IoTHubDeviceTwin_Create failed");

    int initial_twin_version = dt_e2e_gettwin_version(serviceClientDeviceTwinHandle, deviceToUse);

    char *expected_desired_string = generate_unique_string();
    int   expected_desired_integer = generate_new_int();
    char *buffer = malloc_and_fill_desired_payload(expected_desired_string, expected_desired_integer);
    ASSERT_IS_NOT_NULL(buffer, "failed to create the payload for IoTHubDeviceTwin_UpdateTwin");

    dt_e2e_update_twin(serviceClientDeviceTwinHandle, deviceToUse, buffer);

    JSON_Value *root_value = NULL;
    const char *string_property = NULL;
    int integer_property = 0;
    const char *string_property_from_array = NULL;
    int integer_property_from_array = 0;

    time_t beginOperation, nowTime;
    beginOperation = time(NULL);

    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(device->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed");
        }
        else
        {
            if ((device->receivedCallBack) && (device->cb_payload != NULL))
            {
                LogInfo("device->cb_payload: %s", device->cb_payload);
                int current_version = dt_e2e_parse_twin_version(device->cb_payload, false);

                if (current_version == initial_twin_version)
                {
                    // There is a potential race where we'll get the callback for deviceTwin availability on the initial twin, not on the
                    // updated one.  We determine this by looking at $version and if they're the same, it means we haven't got update yet.
                    LogInfo("The version of twin on callback is identical to initially set (%d).  Waiting for update\n", current_version);
                    Unlock(device->lock);
                    ThreadAPI_Sleep(1000);
                    continue;
                }

                root_value = json_parse_string(device->cb_payload);
                if (root_value != NULL)
                {
                    JSON_Object *root_object = json_value_get_object(root_value);
                    JSON_Array *array;

                    if (root_object != NULL)
                    {
                        switch (device->update_state)
                        {
                        case DEVICE_TWIN_UPDATE_COMPLETE:
                            string_property = json_object_dotget_string(root_object, "desired.string_property");
                            integer_property = (int)json_object_dotget_number(root_object, "desired.integer_property");
                            array = json_object_dotget_array(root_object, "desired.array");
                            ASSERT_IS_NOT_NULL(array, "Array not specified");
                            integer_property_from_array = (int)json_array_get_number(array, 0);
                            string_property_from_array = json_array_get_string(array, 1);

                            break;
                        case DEVICE_TWIN_UPDATE_PARTIAL:
                            string_property = json_object_get_string(root_object, "string_property");
                            integer_property = (int)json_object_get_number(root_object, "integer_property");
                            array = json_object_get_array(root_object, "array");
                            ASSERT_IS_NOT_NULL(array, "Array not specified");
                            integer_property_from_array = (int)json_array_get_number(array, 0);
                            string_property_from_array = json_array_get_string(array, 1);
                            break;
                        default: // invalid update state
                            ASSERT_FAIL("Invalid update_state reported");
                            break;
                        }
                        if ((string_property != NULL) && (integer_property != 0))
                        {
                            Unlock(device->lock);
                            break;
                        }
                    }
                }
                json_value_free(root_value);
                root_value = NULL;
            }
            Unlock(device->lock);
        }
        ThreadAPI_Sleep(1000);
    }

    ASSERT_IS_TRUE(difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME, "Timeout waiting for twin message");

    // unsubscribe
    setdevicetwincallback_on_device_or_module(NULL, NULL);

    if (Lock(device->lock) != LOCK_OK)
    {
        ASSERT_FAIL("Lock failed");
    }
    else
    {
        ASSERT_IS_NOT_NULL(root_value, "json_parse_string failed");
        ASSERT_ARE_EQUAL(char_ptr, expected_desired_string, string_property, "string data retrieved differs from expected");
        ASSERT_ARE_EQUAL(int, expected_desired_integer, integer_property, "integer data retrieved differs from expected");
        ASSERT_ARE_EQUAL(char_ptr, expected_desired_string, string_property_from_array, "string data (from array) retrieved differs from expected");
        ASSERT_ARE_EQUAL(int, expected_desired_integer, integer_property_from_array, "integer data (from array) retrieved differs from expected");

        (void)Unlock(device->lock);

        // cleanup
        json_value_free(root_value);
        IoTHubDeviceTwin_Destroy(serviceClientDeviceTwinHandle);
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
        free(expected_desired_string);
        free(buffer);
        destroy_on_device_or_module();
        device_desired_deinit(device);
    }
}

void dt_e2e_get_twin_async_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod)
{
    // arrange
    IOTHUB_PROVISIONED_DEVICE* deviceToUse;
    if (accountAuthMethod == IOTHUB_ACCOUNT_AUTH_X509)
    {
        deviceToUse = IoTHubAccount_GetX509Device(g_iothubAcctInfo);
    }
    else
    {
        deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    }

    DEVICE_DESIRED_DATA *device = device_desired_init();
    ASSERT_IS_NOT_NULL(device, "failed to create the device client data");

    dt_e2e_create_client_handle(deviceToUse, protocol);

    if (deviceToUse->moduleConnectionString != NULL)
    {
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IoTHubModuleClient_GetTwinAsync(iothub_moduleclient_handle, deviceTwinCallback, device), IOTHUB_CLIENT_OK);
    }
    else
    {
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_GetTwinAsync(iothub_deviceclient_handle, deviceTwinCallback, device), IOTHUB_CLIENT_OK);
    }

    bool callbackReceived = false;
    time_t beginOperation, nowTime;
    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(device->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed");
        }
        else
        {
            if (device->receivedCallBack)
            {
                ASSERT_IS_NOT_NULL(device->cb_payload);
                ASSERT_IS_TRUE(strlen(device->cb_payload) > 0);
                callbackReceived = device->receivedCallBack;
                Unlock(device->lock);
                break;
            }
            Unlock(device->lock);
        }
        ThreadAPI_Sleep(1000);
    }
    ASSERT_IS_TRUE(callbackReceived, "Did not receive the GetTwinAsync call back");    

    // cleanup
    destroy_on_device_or_module();
    device_desired_deinit(device);
}

void dt_e2e_send_reported_test_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod)
{
    // arrange
    IOTHUB_PROVISIONED_DEVICE* deviceToUse;
    if (accountAuthMethod == IOTHUB_ACCOUNT_AUTH_X509)
    {
        deviceToUse = IoTHubAccount_GetX509Device(g_iothubAcctInfo);
    }
    else
    {
        deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    }

    DEVICE_REPORTED_DATA *device = device_reported_init();
    ASSERT_IS_NOT_NULL(device, "failed to create the device client data");

    // Create the IoT Hub Data
    dt_e2e_create_client_handle(deviceToUse, protocol);

    // generate the payload
    char *buffer = malloc_and_fill_reported_payload(device->string_property, device->integer_property);
    ASSERT_IS_NOT_NULL(buffer, "failed to allocate and prepare the payload for SendReportedState");

    // act
    sendreportedstate_on_device_or_module(buffer, device);

    ThreadAPI_Sleep(3000);

    int status_code = 400;
    time_t beginOperation, nowTime;
    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(device->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed");
        }
        else
        {
            if (device->receivedCallBack)
            {
                status_code = device->status_code;
                Unlock(device->lock);
                break;
            }
            Unlock(device->lock);
        }
        ThreadAPI_Sleep(1000);
    }
    ASSERT_IS_TRUE(status_code < 300, "SendReported status_code is an error");

    // Send the Event from the client
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
    client_create_with_properies_and_send_d2c(propMap);
    Map_Destroy(propMap);

    ThreadAPI_Sleep(3000);

    // act
    sendreportedstate_on_device_or_module(buffer, device);

    ThreadAPI_Sleep(3000);

    status_code = 400;
    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(device->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed");
        }
        else
        {
            if (device->receivedCallBack)
            {
                status_code = device->status_code;
                Unlock(device->lock);
                break;
            }
            Unlock(device->lock);
        }
        ThreadAPI_Sleep(1000);
    }
    ASSERT_IS_TRUE(status_code < 300, "SendReported status_code is an error");

    free(buffer);
    destroy_on_device_or_module();
    device_reported_deinit(device);
}

void dt_e2e_get_complete_desired_test_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod)
{
    // arrange
    IOTHUB_PROVISIONED_DEVICE* deviceToUse;
    if (accountAuthMethod == IOTHUB_ACCOUNT_AUTH_X509)
    {
        deviceToUse = IoTHubAccount_GetX509Device(g_iothubAcctInfo);
    }
    else
    {
        deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);
    }

    DEVICE_DESIRED_DATA *device = device_desired_init();
    ASSERT_IS_NOT_NULL(device, "failed to create the device client data");

    // Create the IoT Hub Data
    dt_e2e_create_client_handle(deviceToUse, protocol);


    // subscribe
    setdevicetwincallback_on_device_or_module(deviceTwinCallback, device);

    const char *connectionString = IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo);
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "IoTHubServiceClientAuth_CreateFromConnectionString failed");

    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceClientDeviceTwinHandle = IoTHubDeviceTwin_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL(serviceClientDeviceTwinHandle, "IoTHubDeviceTwin_Create failed");

    char *expected_desired_string = generate_unique_string();
    int   expected_desired_integer = generate_new_int();
    char *buffer = malloc_and_fill_desired_payload(expected_desired_string, expected_desired_integer);
    ASSERT_IS_NOT_NULL(buffer, "failed to create the payload for IoTHubDeviceTwin_UpdateTwin");

    dt_e2e_update_twin(serviceClientDeviceTwinHandle, deviceToUse, buffer);

    ThreadAPI_Sleep(3000);
    int status_code = 400;
    time_t beginOperation, nowTime;
    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(device->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed");
        }
        else
        {
            if (device->receivedCallBack)
            {
                status_code = 0;
                Unlock(device->lock);
                break;
            }
            Unlock(device->lock);
        }
        ThreadAPI_Sleep(1000);
    }
    ASSERT_IS_TRUE(status_code == 0, "SendReported status_code is an error");

    // Send the Event from the client
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
    client_create_with_properies_and_send_d2c(propMap);
    Map_Destroy(propMap);

    ThreadAPI_Sleep(3000);

    dt_e2e_update_twin(serviceClientDeviceTwinHandle, deviceToUse, buffer);

    status_code = 400;
    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (Lock(device->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Lock failed");
        }
        else
        {
            if (device->receivedCallBack)
            {
                status_code = 0;
                Unlock(device->lock);
                break;
            }
            Unlock(device->lock);
        }
        ThreadAPI_Sleep(1000);
    }
    ASSERT_IS_TRUE(status_code == 0, "SendReported status_code is an error");

    // unsubscribe
    setdevicetwincallback_on_device_or_module(NULL, NULL);

    free(expected_desired_string);
    free(buffer);
    IoTHubDeviceTwin_Destroy(serviceClientDeviceTwinHandle);
    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
    destroy_on_device_or_module();
    device_desired_deinit(device);
}
