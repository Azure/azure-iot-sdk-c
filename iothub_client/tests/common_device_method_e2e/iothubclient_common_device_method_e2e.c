// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#else
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#endif

#include "iothub.h"
#include "iothub_messaging_ll.h"

#include "umock_c/umock_c.h"
#include "testrunnerswitcher.h"

#include "iothub_account.h"
#include "iothubtest.h"

#include "iothub_client_options.h"

#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "iothub_service_client_auth.h"
#include "iothub_devicemethod.h"
#include "iothub_device_client.h"
#include "iothub_module_client.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#include "iothubclient_common_device_method_e2e.h"

#define DEVICE_METHOD_SUB_WAIT_TIME_MS    (5 * 1000)
#define COMMAND_SUB_WAIT_TIME_MS          DEVICE_METHOD_SUB_WAIT_TIME_MS

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(IOTHUB_DEVICE_METHOD_RESULT, IOTHUB_DEVICE_METHOD_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);

typedef struct IOTHUB_CONN_INFO_TAG
{
    IOTHUB_CLIENT_CONNECTION_STATUS conn_status;
    bool upload_status;
    LOCK_HANDLE lock;
} IOTHUB_CONN_INFO;

static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo = NULL;
static IOTHUB_CONN_INFO g_conn_info;

static IOTHUB_DEVICE_CLIENT_HANDLE iothub_deviceclient_handle = NULL;
static IOTHUB_MODULE_CLIENT_HANDLE iothub_moduleclient_handle = NULL;
static IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = NULL;
static IOTHUB_SERVICE_CLIENT_DEVICE_METHOD_HANDLE serviceClientDeviceMethodHandle = NULL;

void device_method_e2e_init(bool testing_modules)
{
    ASSERT_ARE_EQUAL(int, 0, IoTHub_Init() );

    IOTHUB_ACCOUNT_CONFIG config;
    config.number_of_sas_devices = 2;

    g_iothubAcctInfo = IoTHubAccount_Init_With_Config(&config, testing_modules);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo);

    // Initialize locking
    g_conn_info.lock = Lock_Init();
    g_conn_info.upload_status = false;
    ASSERT_IS_NOT_NULL(g_conn_info.lock);
}

void device_method_e2e_deinit(void)
{
    IoTHubAccount_deinit(g_iothubAcctInfo);

    // Need a double deinit
    IoTHub_Deinit();
    IoTHub_Deinit();

    Lock_Deinit(g_conn_info.lock);
}

static const char *TEST_METHOD_NAME = "MethodName";
#define TEST_COMMAND_COMPONENT_NAME "TestComponentName"
#define TEST_COMMAND_COMMAND_NAME "TestCommandName"
static const char TEST_METHOD_TO_INVOKE_FOR_COMMAND[] = TEST_COMMAND_COMPONENT_NAME "*" TEST_COMMAND_COMMAND_NAME;
static const int METHOD_RESPONSE_SUCCESS = 200;
static const int METHOD_RESPONSE_ERROR = 401;
static const unsigned int TIMEOUT = 60;
static const unsigned int IOTHUB_CONNECT_TIMEOUT_SEC = 120;
static const char TEST_COMMAND_PAYLOAD[] = "{\"foo\":41,\"bar\":42,\"baz\":\"boo\"}";
static const char TEST_MODEL_ID_FOR_COMMAND_TESTS[] = "dtmi:azure:c-sdk:command-e2e-test;1";

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    if (reason == IOTHUB_CLIENT_CONNECTION_OK)
    {
        IOTHUB_CONN_INFO* conn_info = (IOTHUB_CONN_INFO*)userContextCallback;
        if (Lock(conn_info->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            conn_info->conn_status = result;
            (void)Unlock(conn_info->lock);
        }
    }
}

static int MethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
    int responseCode;
    const char * expectedMethodPayload = (const char*)userContextCallback;

    LogInfo("MethodCallback invoked with method_name=%s", method_name);

    if (strcmp(TEST_METHOD_NAME, method_name))
    {
        LogError("Method name incorrect - expected %s but got %s", TEST_METHOD_NAME, method_name);
        responseCode = METHOD_RESPONSE_ERROR;
    }
    else if (size != strlen(expectedMethodPayload))
    {
        LogError("payload size incorrect - expected %zu but got %zu", strlen(expectedMethodPayload), size);
        responseCode = METHOD_RESPONSE_ERROR;
    }
    else if (memcmp(payload, expectedMethodPayload, size))
    {
        LogError("Payload strings do not match");
        responseCode = METHOD_RESPONSE_ERROR;
    }
    else
    {
        *resp_size = size;
        if (size == 0)
        {
            *response = NULL;
            responseCode = METHOD_RESPONSE_SUCCESS;
        }
        else
        {
            if ((*response = (unsigned char*)malloc(*resp_size)) == NULL)
            {
                LogError("allocation failure");
                responseCode = METHOD_RESPONSE_ERROR;
            }
            else
            {
                (void)memcpy(*response, payload, *resp_size);
                responseCode = METHOD_RESPONSE_SUCCESS;
            }
        }
    }
    return responseCode;
}

static void CommandCallback(const IOTHUB_CLIENT_COMMAND_REQUEST* commandRequest, IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse,  void* userContextCallback)
{
    const char * expectedCommandPayload = (const char*)userContextCallback;

    LogInfo("CommandCallback invoked with componentName=%s, commandName=%s", commandRequest->componentName, commandRequest->commandName);

    if ((commandRequest->componentName == NULL) || (strcmp(TEST_COMMAND_COMPONENT_NAME, commandRequest->componentName) != 0))
    {
        LogError("Component name incorrect - expected %s but got %s", TEST_COMMAND_COMPONENT_NAME, commandRequest->componentName);
        commandResponse->statusCode = METHOD_RESPONSE_ERROR;
    }
    else if ((commandRequest->commandName == NULL) || (strcmp(TEST_COMMAND_COMMAND_NAME, commandRequest->commandName) != 0))
    {
        LogError("Command name incorrect - expected %s but got %s", TEST_COMMAND_COMMAND_NAME, commandRequest->commandName);
        commandResponse->statusCode = METHOD_RESPONSE_ERROR;
    }
    else if (IOTHUB_CLIENT_COMMAND_REQUEST_STRUCT_VERSION_1 != commandRequest->structVersion)
    {
        LogError("Command request structVersion wrong - expected %d but got %d", IOTHUB_CLIENT_COMMAND_REQUEST_STRUCT_VERSION_1, commandRequest->structVersion);
        commandResponse->statusCode = METHOD_RESPONSE_ERROR;
    }
    else if (IOTHUB_CLIENT_COMMAND_RESPONSE_STRUCT_VERSION_1 != commandResponse->structVersion)
    {
        LogError("Command response structVersion wrong - expected %d but got %d", IOTHUB_CLIENT_COMMAND_REQUEST_STRUCT_VERSION_1, commandResponse->structVersion);
        commandResponse->statusCode = METHOD_RESPONSE_ERROR;
    }
    else if (commandRequest->payloadLength != strlen(expectedCommandPayload))
    {
        LogError("payload size incorrect - expected %zu but got %zu", strlen(expectedCommandPayload), commandRequest->payloadLength);
        commandResponse->statusCode = METHOD_RESPONSE_ERROR;
    }
    else if (memcmp(commandRequest->payload, expectedCommandPayload, commandRequest->payloadLength))
    {
        LogError("Payload strings do not match");
        commandResponse->statusCode = METHOD_RESPONSE_ERROR;
    }
    else
    {
        // Echo the payload we received from the request into the response payload.
        commandResponse->payloadLength = commandRequest->payloadLength;
        if (commandRequest->payloadLength == 0)
        {
            commandResponse->payload = NULL;
            commandResponse->payloadLength = 0;
            commandResponse->statusCode = METHOD_RESPONSE_SUCCESS;
        }
        else
        {
            if ((commandResponse->payload = (unsigned char*)malloc(commandResponse->payloadLength)) == NULL)
            {
                LogError("allocation failure");
                commandResponse->statusCode = METHOD_RESPONSE_ERROR;
            }
            else
            {
                (void)memcpy(commandResponse->payload, commandRequest->payload, commandResponse->payloadLength);
                commandResponse->statusCode = METHOD_RESPONSE_SUCCESS;
            }
        }
    }
}

void fileUploadCallback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, void* userContextCallback)
{
    LogInfo("fileUploadCallback(%s)", MU_ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result));

    IOTHUB_CONN_INFO* conn_info = (IOTHUB_CONN_INFO*)userContextCallback;
    if (Lock(conn_info->lock) != LOCK_OK)
    {
        ASSERT_FAIL("unable to lock");
    }
    else
    {
        conn_info->upload_status = result == FILE_UPLOAD_OK;
        (void)Unlock(conn_info->lock);
    }
}

#ifndef DONT_USE_UPLOADTOBLOB
#define HELLO_WORLD "Hello World from IoTHubClient_UploadToBlob"
static int DeviceMethodWithUploadCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
    (void)method_name;
    (void)userContextCallback;

    int responseCode;
    if (IoTHubDeviceClient_UploadToBlobAsync(iothub_deviceclient_handle, "hello_world.txt", (const unsigned char*)HELLO_WORLD, sizeof(HELLO_WORLD) - 1, fileUploadCallback, &g_conn_info) == IOTHUB_CLIENT_OK)
    {
        LogInfo("Upload succeeded");

        if ((*response = (unsigned char*)malloc(size)) == NULL)
        {
            responseCode = METHOD_RESPONSE_ERROR;
            *resp_size = 0;
        }
        else
        {
            (void)memcpy(*response, payload, size);
            responseCode = METHOD_RESPONSE_SUCCESS;
            *resp_size = size;
        }
    }
    else
    {
        LogError("Upload failed");
        responseCode = METHOD_RESPONSE_ERROR;
    }
    return responseCode;
}
#endif

// From IoT Hub / Service API perspective, there is no difference between a "legacy" device method and a command.
// Callers of this function decide method or PnP convention via methodOrCommandName parameter, but API calls remain same.
void test_invoke_device_method(const char* deviceId, const char* moduleId, const char* methodOrCommandName, const char *payload)
{
    int responseStatus = METHOD_RESPONSE_ERROR;
    unsigned char* responsePayload = NULL;
    size_t responsePayloadSize = 0;
    IOTHUB_DEVICE_METHOD_RESULT invokeResult = IOTHUB_DEVICE_METHOD_ERROR;

    if (moduleId != NULL)
    {
        for (int tryCounter = 0; tryCounter < TEST_METHOD_INVOKE_MAX_RETRIES; tryCounter++)
        {
            invokeResult = IoTHubDeviceMethod_InvokeModule(
                serviceClientDeviceMethodHandle, 
                deviceId,
                moduleId,
                methodOrCommandName,
                payload,
                TIMEOUT,
                &responseStatus,
                &responsePayload,
                &responsePayloadSize);

            if (invokeResult == IOTHUB_DEVICE_METHOD_OK)
            {
                break;
            }

            LogError(
                "(Try %d) IoTHubDeviceMethod_InvokeModule deviceId='%s', moduleId='%s' error=%d", 
                tryCounter + 1,
                deviceId,
                moduleId,
                invokeResult);

            ThreadAPI_Sleep(TEST_SLEEP_BETWEEN_METHOD_INVOKE_FAILURES_MSEC);
        }

        ASSERT_ARE_EQUAL(IOTHUB_DEVICE_METHOD_RESULT, IOTHUB_DEVICE_METHOD_OK, invokeResult, "Service Client IoTHubDeviceMethod_InvokeModule failed");
    }
    else
    {
        for (int tryCounter = 0; tryCounter < TEST_METHOD_INVOKE_MAX_RETRIES; tryCounter++)
        {
            invokeResult = IoTHubDeviceMethod_Invoke(
                serviceClientDeviceMethodHandle, 
                deviceId,
                methodOrCommandName,
                payload,
                TIMEOUT,
                &responseStatus,
                &responsePayload,
                &responsePayloadSize);

            if (invokeResult == IOTHUB_DEVICE_METHOD_OK)
            {
                break;
            }

            LogError("(Try %d) IoTHubDeviceMethod_Invoke deviceId='%s' error=%d", tryCounter + 1, deviceId, invokeResult);
            ThreadAPI_Sleep(TEST_SLEEP_BETWEEN_METHOD_INVOKE_FAILURES_MSEC);
        }

        ASSERT_ARE_EQUAL(IOTHUB_DEVICE_METHOD_RESULT, IOTHUB_DEVICE_METHOD_OK, invokeResult, "Service Client IoTHubDeviceMethod_Invoke failed");
    }

    // After a NULL payload is sent above (ie no payload), we now expect the device to send us back "{}" since underneath in the device
    // SDK, we rewrap a NULL payload to empty braces.
    if(payload == NULL)
    {
        payload = "{}";
    }

    ASSERT_ARE_EQUAL(int, METHOD_RESPONSE_SUCCESS, responseStatus, "response status is incorrect");
    ASSERT_ARE_EQUAL(size_t, strlen(payload), responsePayloadSize, "response size is incorrect");
    if (memcmp(payload, responsePayload, responsePayloadSize))
    {
        LogInfo("response string does not match.  Expected=<%s>, Actual=<%s>", payload, responsePayload);
        ASSERT_FAIL("response string does not match");
    }

    free(responsePayload);
}

void test_device_method_with_string_ex(IOTHUB_PROVISIONED_DEVICE** devicesToUse, size_t number_of_multiplexed_devices, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char *payload)
{
    IOTHUB_CLIENT_RESULT result;
    time_t beginOperation, nowTime;
    bool trace = true;
    size_t iterator;

    // Note: Device multiplexing is only supported by AMQP and HTTP protocols.
    // Note: device multiplexing is only supported if using CBS authentication.

    IOTHUB_DEVICE_CLIENT_HANDLE* registered_devices = (IOTHUB_DEVICE_CLIENT_HANDLE*)malloc(sizeof(IOTHUB_DEVICE_CLIENT_HANDLE) * number_of_multiplexed_devices);
    ASSERT_IS_NOT_NULL(registered_devices, "Failed allocating array of registered devices");

    IOTHUB_CONN_INFO** connection_infos = (IOTHUB_CONN_INFO**)malloc(sizeof(IOTHUB_CONN_INFO*) * number_of_multiplexed_devices);
    ASSERT_IS_NOT_NULL(connection_infos, "Failed creating the connection info array");

    TRANSPORT_HANDLE transport_handle = IoTHubTransport_Create(protocol, IoTHubAccount_GetIoTHubName(g_iothubAcctInfo), IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL(transport_handle, "Failed creating the transport");

    for (iterator = 0; iterator < number_of_multiplexed_devices; iterator++)
    {
        ASSERT_IS_FALSE(devicesToUse[iterator]->howToCreate == IOTHUB_ACCOUNT_AUTH_X509, "Device multiplexing not supported with authentication mode other than CBS");

        connection_infos[iterator] = (IOTHUB_CONN_INFO*)malloc(sizeof(IOTHUB_CONN_INFO));
        ASSERT_IS_NOT_NULL(connection_infos[iterator], "Failed creating the connection info instance");
        connection_infos[iterator]->conn_status = IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED;
        connection_infos[iterator]->lock = Lock_Init();
        connection_infos[iterator]->upload_status = false;
        ASSERT_IS_NOT_NULL(connection_infos[iterator]->lock, "Failed creating thread lock");

        IOTHUB_CLIENT_CONFIG client_config;
        client_config.deviceId = devicesToUse[iterator]->deviceId;
        client_config.deviceKey = devicesToUse[iterator]->primaryAuthentication;
        client_config.deviceSasToken = NULL;
        client_config.iotHubName = IoTHubAccount_GetIoTHubName(g_iothubAcctInfo);
        client_config.iotHubSuffix = IoTHubAccount_GetIoTHubSuffix(g_iothubAcctInfo);
        client_config.protocol = (const IOTHUB_CLIENT_TRANSPORT_PROVIDER)protocol;

        registered_devices[iterator] = IoTHubDeviceClient_CreateWithTransport(transport_handle, &client_config);
        ASSERT_IS_NOT_NULL(registered_devices[iterator], "Failed creating multiplexed client");

        result = IoTHubDeviceClient_SetConnectionStatusCallback(registered_devices[iterator], connection_status_callback, connection_infos[iterator]);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set connection Status Callback");

        result = IoTHubDeviceClient_SetOption(registered_devices[iterator], OPTION_LOG_TRACE, &trace);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set tracing option");

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        result = IoTHubDeviceClient_SetOption(registered_devices[iterator], OPTION_TRUSTED_CERT, certificates);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set trusted certificate");
#endif // SET_TRUSTED_CERT_IN_SAMPLES

        result = IoTHubDeviceClient_SetDeviceMethodCallback(registered_devices[iterator], MethodCallback, (void*)payload);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device method callback");
    }

    beginOperation = time(NULL);
    bool continue_running;
    do
    {
        continue_running = false;

        for (iterator = 0; iterator < number_of_multiplexed_devices; iterator++)
        {
            if (Lock(connection_infos[iterator]->lock) != LOCK_OK)
            {
                ASSERT_FAIL("unable to lock");
            }
            else
            {
                IOTHUB_CLIENT_CONNECTION_STATUS conn_status = connection_infos[iterator]->conn_status;
                (void)Unlock(connection_infos[iterator]->lock);

                if (conn_status != IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
                {
                    continue_running = true;
                    break;
                }
            }
        }

        ThreadAPI_Sleep(10);
    } while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < IOTHUB_CONNECT_TIMEOUT_SEC) &&
        (continue_running)
      );

    for (iterator = 0; iterator < number_of_multiplexed_devices; iterator++)
    {
        if (Lock(connection_infos[iterator]->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Failed locking to verify connection status");
        }
        else
        {
            char msg[1024];
            sprintf(msg, "Device %s not connected", devicesToUse[iterator]->deviceId);
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, connection_infos[iterator]->conn_status, msg);
            (void)Unlock(connection_infos[iterator]->lock);
        }
    }

    // Wait for the method to subscribe
    ThreadAPI_Sleep(DEVICE_METHOD_SUB_WAIT_TIME_MS);

    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "Could not create service client handle");

    serviceClientDeviceMethodHandle = IoTHubDeviceMethod_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL(serviceClientDeviceMethodHandle, "Could not create device method handle");

    for (iterator = 0; iterator < number_of_multiplexed_devices; iterator++)
    {
        test_invoke_device_method(devicesToUse[iterator]->deviceId, devicesToUse[iterator]->moduleId, TEST_METHOD_NAME, payload);
    }

    for (iterator = 0; iterator < number_of_multiplexed_devices; iterator++)
    {
        (void)IoTHubDeviceClient_SetDeviceMethodCallback(registered_devices[iterator], NULL, NULL);
        IoTHubDeviceClient_Destroy(registered_devices[iterator]);
    }

    IoTHubTransport_Destroy(transport_handle);
    IoTHubDeviceMethod_Destroy(serviceClientDeviceMethodHandle);
    serviceClientDeviceMethodHandle = NULL;
    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
    iotHubServiceClientHandle = NULL;

    for (iterator = 0; iterator < number_of_multiplexed_devices; iterator++)
    {
        Lock_Deinit(connection_infos[iterator]->lock);
        free(connection_infos[iterator]);
    }
    free(connection_infos);
    free(registered_devices);
}


static void setoption_on_device_or_module(const char* optionName, const void* optionData, const char* errorMessage)
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

static void setconnectionstatuscallback_on_device_or_module()
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SetConnectionStatusCallback(iothub_moduleclient_handle, connection_status_callback, &g_conn_info);
    }
    else
    {
        result = IoTHubDeviceClient_SetConnectionStatusCallback(iothub_deviceclient_handle, connection_status_callback, &g_conn_info);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set connection Status Callback");
}

static void setmethodcallback_on_device_or_module(const char* payload)
{
    IOTHUB_CLIENT_RESULT result;

    // If payload passed from the service is NULL, we give the user an empty JSON object payload
    // https://github.com/Azure/azure-iot-sdk-c/pull/2097
    if(payload == NULL)
    {
      payload = "{}";
    }

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SetModuleMethodCallback(iothub_moduleclient_handle, MethodCallback, (void*)payload);
    }
    else
    {
        result = IoTHubDeviceClient_SetDeviceMethodCallback(iothub_deviceclient_handle, MethodCallback, (void*)payload);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubDeviceClient_Set(Device|Module)MethodCallback");
}

static void setcommandcallback_on_device_or_module(const char* payload)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SubscribeToCommands(iothub_moduleclient_handle, CommandCallback, (void*)payload);
    }
    else
    {
        result = IoTHubDeviceClient_SubscribeToCommands(iothub_deviceclient_handle, CommandCallback, (void*)payload);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not IoTHubDeviceClient_Set(Device|Module)MethodCallback");
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

static void create_hub_client_from_provisioned_device(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char* modelId)
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

    bool trace = true;
    setoption_on_device_or_module(OPTION_LOG_TRACE, &trace, "Cannot enable tracing");

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    setoption_on_device_or_module(OPTION_TRUSTED_CERT, certificates, "Could not set the device trusted certificate");
#endif // SET_TRUSTED_CERT_IN_SAMPLES

    if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509)
    {
        setoption_on_device_or_module(OPTION_X509_CERT, deviceToUse->certificate, "Could not set the device x509 certificate");
        setoption_on_device_or_module(OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication, "Could not set the device x509 privateKey");
    }

    if (modelId != NULL)
    {
        setoption_on_device_or_module(OPTION_MODEL_ID, modelId, "Cannot specify modelId");
    }

    setconnectionstatuscallback_on_device_or_module();
}

static void test_wait_for_device_to_connect(void)
{
    time_t beginOperation, nowTime;
    bool continue_running = true;

    beginOperation = time(NULL);
    do
    {
        if (Lock(g_conn_info.lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            if (g_conn_info.conn_status == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
            {
                continue_running = false;
            }
            (void)Unlock(g_conn_info.lock);
        }

        ThreadAPI_Sleep(10);
    } while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < IOTHUB_CONNECT_TIMEOUT_SEC) &&
        (continue_running)
        );
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, g_conn_info.conn_status, "Device Not connected");
}

static void test_device_method_with_string(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char *payload)
{
    g_conn_info.conn_status = IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED;

    create_hub_client_from_provisioned_device(deviceToUse, protocol, NULL);
    setmethodcallback_on_device_or_module(payload);
    test_wait_for_device_to_connect();

    // Wait for the method to subscribe
    ThreadAPI_Sleep(DEVICE_METHOD_SUB_WAIT_TIME_MS);

    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "Could not create service client handle");

    serviceClientDeviceMethodHandle = IoTHubDeviceMethod_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL(serviceClientDeviceMethodHandle, "Could not create device method handle");

    test_invoke_device_method(deviceToUse->deviceId, deviceToUse->moduleId, TEST_METHOD_NAME, payload);
}

static void test_subscribe_to_command(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    g_conn_info.conn_status = IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED;

    create_hub_client_from_provisioned_device(deviceToUse, protocol, TEST_MODEL_ID_FOR_COMMAND_TESTS);
    setcommandcallback_on_device_or_module(TEST_COMMAND_PAYLOAD);
    test_wait_for_device_to_connect();

    // Wait for the command to subscribe
    ThreadAPI_Sleep(COMMAND_SUB_WAIT_TIME_MS);

    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "Could not create service client handle");

    serviceClientDeviceMethodHandle = IoTHubDeviceMethod_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL(serviceClientDeviceMethodHandle, "Could not create device method handle");

    test_invoke_device_method(deviceToUse->deviceId, deviceToUse->moduleId, TEST_METHOD_TO_INVOKE_FOR_COMMAND, TEST_COMMAND_PAYLOAD);
}

#ifndef DONT_USE_UPLOADTOBLOB
void test_device_method_calls_upload(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char *payload)
{
    time_t beginOperation, nowTime;
    bool continue_running;

    ASSERT_IS_NULL(deviceToUse->moduleConnectionString, "Modules do not support uploadToBlob");
    g_conn_info.conn_status = IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED;

    create_hub_client_from_provisioned_device(deviceToUse, protocol, NULL);
    setmethodcallback_on_device_or_module(payload);
    test_wait_for_device_to_connect();

    // Wait for the method to subscribe
    ThreadAPI_Sleep(DEVICE_METHOD_SUB_WAIT_TIME_MS);

    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "Could not create service client handle");

    serviceClientDeviceMethodHandle = IoTHubDeviceMethod_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL(serviceClientDeviceMethodHandle, "Could not create device method handle");

    test_invoke_device_method(deviceToUse->deviceId, deviceToUse->moduleId, TEST_METHOD_NAME, payload);

    // yield
    ThreadAPI_Sleep(DEVICE_METHOD_SUB_WAIT_TIME_MS);

    continue_running = true;
    beginOperation = time(NULL);
    do
    {
        if (Lock(g_conn_info.lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            continue_running = g_conn_info.upload_status != true;
            (void)Unlock(g_conn_info.lock);
        }

        ThreadAPI_Sleep(10);
    } while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < IOTHUB_CONNECT_TIMEOUT_SEC) &&
        (continue_running)
        );
}
#endif

static void client_create_with_properies_and_send_d2c(IOTHUB_PROVISIONED_DEVICE* deviceToUse, MAP_HANDLE mapHandle)
{
    IOTHUB_MESSAGE_HANDLE msgHandle;

    char messageStr[512];
    int len = snprintf(messageStr, sizeof(messageStr), "Happy little device message from device '%s'", deviceToUse->deviceId);
    if (len < 0 || len == sizeof(messageStr))
    {
        ASSERT_FAIL("messageStr is not large enough!");
        return;
    }

    msgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)messageStr, len);
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

static void test_device_method_with_string_svc_fault_ctrl(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char *payload, const char* faultOperationType, const char* faultOperationCloseReason, const char* faultOperationDelayInSecs)
{
    g_conn_info.conn_status = IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED;

    create_hub_client_from_provisioned_device(deviceToUse, protocol, NULL);
    setmethodcallback_on_device_or_module(payload);
    test_wait_for_device_to_connect();

    // Wait for the method to subscribe
    ThreadAPI_Sleep(DEVICE_METHOD_SUB_WAIT_TIME_MS);

    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL(iotHubServiceClientHandle, "Could not create service client handle");

    serviceClientDeviceMethodHandle = IoTHubDeviceMethod_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL(serviceClientDeviceMethodHandle, "Could not create device method handle");

    test_invoke_device_method(deviceToUse->deviceId, deviceToUse->moduleId, TEST_METHOD_NAME, payload);

    // Send the Event from the client
    MAP_HANDLE propMap = Map_Create(NULL);
    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationType", faultOperationType) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationCloseReason", faultOperationCloseReason) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationDelayInSecs", faultOperationDelayInSecs) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!");
    }
    (void)printf("Send fault control message...\r\n");
    client_create_with_properies_and_send_d2c(deviceToUse, propMap);
    Map_Destroy(propMap);

    ThreadAPI_Sleep(3000);

    test_invoke_device_method(deviceToUse->deviceId, deviceToUse->moduleId, TEST_METHOD_NAME, payload);
}

void device_method_function_cleanup()
{
    if (serviceClientDeviceMethodHandle != NULL)
    {
        IoTHubDeviceMethod_Destroy(serviceClientDeviceMethodHandle);
        serviceClientDeviceMethodHandle = NULL;
    }

    if (iotHubServiceClientHandle != NULL)
    {
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
        iotHubServiceClientHandle = NULL;
    }

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
}

void device_command_subscribe_e2e_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_subscribe_to_command(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol);
}

void device_command_subscribe_e2e_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_subscribe_to_command(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol);
}

void device_method_e2e_method_call_with_string_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, "\"I'm a happy little string\"");
}

void device_method_e2e_method_call_with_string_sas_multiplexed(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, size_t number_of_devices)
{
    test_device_method_with_string_ex(IoTHubAccount_GetSASDevices(g_iothubAcctInfo), number_of_devices, protocol, "\"I'm a happy little string from multiplexed clients\"");
}

void device_method_e2e_method_call_with_double_quoted_json_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, "{\"foo\":41,\"bar\":42,\"baz\":\"boo\"}");
}

void device_method_e2e_method_call_with_empty_json_object_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, "{}");
}

void device_method_e2e_method_call_with_null_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, "null");
}

extern void device_method_e2e_method_call_with_embedded_double_quote_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, "\"this string has a double quote \\\" in the middle\"");
}

extern void device_method_e2e_method_call_with_embedded_single_quote_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, "\"this string has a single quote ' in the middle\"");
}

#ifndef DONT_USE_UPLOADTOBLOB
void device_method_e2e_method_calls_upload_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_calls_upload(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, "{\"Response\":\"Nothing\"}");
}
#endif

void device_method_e2e_method_call_with_string_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol, "\"I'm a happy little string\"");
}

void device_method_e2e_method_call_with_double_quoted_json_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol, "{\"foo\":41,\"bar\":42,\"baz\":\"boo\"}");
}

void device_method_e2e_method_call_with_empty_json_object_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol, "{}");
}

void device_method_e2e_method_call_with_NULL_json_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol, NULL);
}

void device_method_e2e_method_call_with_null_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol, "null");
}

extern void device_method_e2e_method_call_with_embedded_double_quote_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol, "\"this string has a double quote \\\" in the middle\"");
}

extern void device_method_e2e_method_call_with_embedded_single_quote_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol, "\"this string has a single quote ' in the middle\"");
}

#ifndef DONT_USE_UPLOADTOBLOB
void device_method_e2e_method_calls_upload_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_calls_upload(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol, "{\"Response\":\"Nothing\"}");
}
#endif

void device_method_e2e_method_call_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    test_device_method_with_string_svc_fault_ctrl(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, "\"I'm a happy little string\"", "KillTcp", "boom", "1");
}
