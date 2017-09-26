// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "testrunnerswitcher.h"

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/base64.h"

#include "azure_hub_modules/dps_client.h"
#include "azure_hub_modules/secure_device_factory.h"

#include "azure_hub_modules/dps_transport_http_client.h"
#include "azure_hub_modules/dps_transport_amqp_ws_client.h"
#include "azure_hub_modules/dps_transport_amqp_client.h"
#include "azure_hub_modules/dps_transport_mqtt_ws_client.h"
#include "azure_hub_modules/dps_transport_mqtt_client.h"

#include "azure_c_shared_utility/connection_string_parser.h" 
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "platform_process.h"
#include "dps_service_client.h"
#include "azure_hub_modules/dps_sec_client.h"

static TEST_MUTEX_HANDLE g_dllByDll;

typedef enum REGISTRATION_RESULT_TAG
{
    REG_RESULT_BEGIN,
    REG_RESULT_COMPLETE,
    REG_RESULT_FAILED
} REGISTRATION_RESULT;

typedef struct DPS_CLIENT_E2E_INFO_TAG
{
    char* iothub_uri;
    char* device_id;
    REGISTRATION_RESULT reg_result;
    MAP_HANDLE conn_map;
} DPS_CLIENT_E2E_INFO;

static const char* g_dps_conn_string = NULL;
static const char* g_dps_scope_id = NULL;
static char* g_dps_uri = NULL;
static const char* g_desired_iothub = NULL;

DPS_CLIENT_E2E_INFO g_dps_info;
PLATFORM_PROCESS_HANDLE g_emulator_proc;
DPS_SEC_TYPE g_dps_hsm_type;

#define MAX_CLOUD_TRAVEL_TIME       60.0
#define DEVICE_GUID_SIZE            37

#ifdef WIN32
const char* FOLDER_SEPARATOR = "\\";
#else
const char* FOLDER_SEPARATOR = "/";
#endif

TEST_DEFINE_ENUM_TYPE(DPS_RESULT, DPS_RESULT_VALUES);

static void on_dps_error_callback(DPS_ERROR error_type, void* user_context)
{
    (void)error_type;
    if (user_context == NULL)
    {
        ASSERT_FAIL("User Context is NULL in on_dps_error_callback");
    }
    else
    {
        DPS_CLIENT_E2E_INFO* dps_info = (DPS_CLIENT_E2E_INFO*)user_context;
        dps_info->reg_result = REG_RESULT_FAILED;
        ASSERT_FAIL("DPS On Error callback");
    }
}

static void iothub_dps_register_device(DPS_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    if (user_context == NULL)
    {
        ASSERT_FAIL("User Context is NULL iothub_dps_register_device");
    }
    else
    {
        DPS_CLIENT_E2E_INFO* dps_info = (DPS_CLIENT_E2E_INFO*)user_context;
        if (register_result == DPS_CLIENT_OK)
        {
            (void)mallocAndStrcpy_s(&dps_info->iothub_uri, iothub_uri);
            (void)mallocAndStrcpy_s(&dps_info->device_id, device_id);
            dps_info->reg_result = REG_RESULT_COMPLETE;
        }
        else
        {
            dps_info->reg_result = REG_RESULT_FAILED;
        }
    }
}

static void dps_registation_status(DPS_REGISTRATION_STATUS reg_status, void* user_context)
{
    (void)reg_status;
    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
    }
}

static bool provision_dps_device()
{
    bool result;
    result = true;
    return result;
}

static DPS_LL_HANDLE create_dps_handle(DPS_TRANSPORT_PROVIDER_FUNCTION dps_transport)
{
    DPS_LL_HANDLE result;
    result = DPS_LL_Create(g_dps_uri, g_dps_scope_id, dps_transport, on_dps_error_callback, &g_dps_info);
    ASSERT_IS_NOT_NULL_WITH_MSG(result, "Failure create a DPS HANDLE");
    return result;
}

static char* construct_simulator_path()
{
    const char* path_fmt = "..\\azure-iot-device-auth\\iothub_device_auth\\deps\\utpm\\tools\\tpm_simulator\\Simulator.exe";
    size_t path_len = strlen(path_fmt);
    // Need to get this done
    char* sim_path = (char*)malloc(path_len+1);
    if (sim_path != NULL)
    {
        strcpy(sim_path, path_fmt);
    }
    return sim_path;
}

static void start_tpm_emulator()
{
    char* simulator_path = construct_simulator_path();
    ASSERT_IS_NOT_NULL_WITH_MSG(simulator_path, "Unable to construct simulator path");

    g_emulator_proc = platform_process_create(simulator_path);
    ASSERT_IS_NOT_NULL_WITH_MSG(g_emulator_proc, "Failure starting TPM Emulator");

    free(simulator_path);
}

static void stop_tpm_emulator()
{
    platform_process_destroy(g_emulator_proc);
}

static int construct_device_id(const char* prefix, char** deviceName)
{
    int result;
    char deviceGuid[DEVICE_GUID_SIZE];
    if (UniqueId_Generate(deviceGuid, DEVICE_GUID_SIZE) != UNIQUEID_OK)
    {
        LogError("Unable to generate unique Id.\r\n");
        result = __FAILURE__;
    }
    else
    {
        size_t len = strlen(prefix) + DEVICE_GUID_SIZE;
        *deviceName = (char*)malloc(len + 1);
        if (*deviceName == NULL)
        {
            LogError("Failure allocating device ID.\r\n");
            result = __FAILURE__;
        }
        else
        {
            if (sprintf_s(*deviceName, len + 1, prefix, deviceGuid) <= 0)
            {
                LogError("Failure constructing device ID.\r\n");
                free(*deviceName);
                result = __FAILURE__;
            }
            else
            {
                LogInfo("Created Device %s.", *deviceName);
                result = 0;
            }
        }
    }
    return result;
}

static void create_enrollment_device(const char* dps_uri, const char* key, const char* keyname)
{
    int result;
    DPS_SEC_HANDLE dps_sec_handle;
    DPS_SERVICE_CLIENT_HANDLE svc_client;
    ENROLLMENT_INFO enroll_info;

    memset(&enroll_info, 0, sizeof(ENROLLMENT_INFO));
    svc_client = dps_service_create(dps_uri, key, keyname);
    ASSERT_IS_NOT_NULL_WITH_MSG(svc_client, "Failure creating service client");

    dps_sec_handle = dps_sec_create();
    ASSERT_IS_NOT_NULL_WITH_MSG(dps_sec_handle, "Failure creating dps sec create");

    enroll_info.registration_id = dps_sec_get_registration_id(dps_sec_handle);
    ASSERT_IS_NOT_NULL_WITH_MSG(enroll_info.registration_id, "Failure retrieving registration id");

    if (g_desired_iothub != NULL)
    {
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, mallocAndStrcpy_s(&enroll_info.desired_iothub, g_desired_iothub), "failed allocating desired iothub");
    }

    enroll_info.enable_entry = true;

    g_dps_hsm_type = dps_sec_get_type(dps_sec_handle);
    if (g_dps_hsm_type == DPS_SEC_TYPE_X509)
    {
        enroll_info.attestation_value = dps_sec_get_certificate(dps_sec_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(enroll_info.attestation_value, "Failure retrieving certificate");

        enroll_info.type = SECURE_DEVICE_TYPE_X509;
    }
    else
    {
        BUFFER_HANDLE ek;
        STRING_HANDLE encoded_ek;

        ek = dps_sec_get_endorsement_key(dps_sec_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(ek, "Failure creating endorsement key");

        encoded_ek = Base64_Encoder(ek);
        ASSERT_IS_NOT_NULL_WITH_MSG(encoded_ek, "Failure base64 encoding endorsement key");

        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, mallocAndStrcpy_s(&enroll_info.attestation_value, STRING_c_str(encoded_ek)), "failed allocating endorsement key");

        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, construct_device_id("device_", &enroll_info.device_id), "Failed allocating device name");

        enroll_info.type = SECURE_DEVICE_TYPE_TPM;

        BUFFER_delete(ek);
        STRING_delete(encoded_ek);
    }

    result = dps_service_create_enrollment(svc_client, &enroll_info);
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "Failed enrolling device");

    dps_serivce_destroy(svc_client);
}

static void remove_enrollment_device(const char* dps_uri, const char* key, const char* keyname)
{
    DPS_SEC_HANDLE dps_sec_handle;
    DPS_SERVICE_CLIENT_HANDLE svc_client;
    char* registration_id;

    svc_client = dps_service_create(dps_uri, key, keyname);
    ASSERT_IS_NOT_NULL_WITH_MSG(svc_client, "Failure creating service client");

    dps_sec_handle = dps_sec_create();
    ASSERT_IS_NOT_NULL_WITH_MSG(dps_sec_handle, "Failure creating dps sec create");

    registration_id = dps_sec_get_registration_id(dps_sec_handle);
    ASSERT_IS_NOT_NULL_WITH_MSG(registration_id, "Failure retrieving registration id");


    dps_serivce_destroy(svc_client);
}

BEGIN_TEST_SUITE(dps_client_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);

        memset(&g_dps_info, 0, sizeof(DPS_CLIENT_E2E_INFO));

        platform_init();
        dps_secure_device_init();

        g_dps_conn_string = getenv("DPS_CONNECTION_STRING");
        ASSERT_IS_NOT_NULL_WITH_MSG(g_dps_conn_string, "DPS_CONNECTION_STRING is NULL");

        g_desired_iothub = getenv("DPS_DESIRED_IOTHUB");

        // Parse connection string
        g_dps_info.conn_map = connectionstringparser_parse_from_char(g_dps_conn_string);
        ASSERT_IS_NOT_NULL_WITH_MSG(g_dps_conn_string, "DPS_CONNECTION_STRING is NULL");

        const char* host_name = Map_GetValueFromKey(g_dps_info.conn_map, "HostName");
        ASSERT_IS_NOT_NULL_WITH_MSG(host_name, "Failed retrieving connection hostname");

        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, mallocAndStrcpy_s(&g_dps_uri, host_name), "Failed allocating connection hostname");

        const char* key_name = Map_GetValueFromKey(g_dps_info.conn_map, "SharedAccessKeyName");
        ASSERT_IS_NOT_NULL_WITH_MSG(key_name, "Failed retrieving primary keyname");

        const char* access_key = Map_GetValueFromKey(g_dps_info.conn_map, "SharedAccessKey");
        ASSERT_IS_NOT_NULL_WITH_MSG(access_key, "Failed retrieving key");

        // Register device
        create_enrollment_device(host_name, key_name, access_key);

        // Start Emulator
        if (g_dps_hsm_type == DPS_SEC_TYPE_TPM)
        {
            start_tpm_emulator();
        }

        g_dps_scope_id = getenv("DPS_SCOPE_ID_VALUE");
        ASSERT_IS_NOT_NULL_WITH_MSG(g_dps_scope_id, "DPS_SCOPE_ID_VALUE is NULL");
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        // Stop Emulator
        if (g_dps_hsm_type == DPS_SEC_TYPE_TPM)
        {
            stop_tpm_emulator();
        }
        dps_secure_device_init();
        platform_deinit();

        const char* host_name = Map_GetValueFromKey(g_dps_info.conn_map, "HostName");
        ASSERT_IS_NOT_NULL_WITH_MSG(host_name, "Failed retrieving connection hostname");

        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, mallocAndStrcpy_s(&g_dps_uri, host_name), "Failed allocating connection hostname");

        const char* key_name = Map_GetValueFromKey(g_dps_info.conn_map, "SharedAccessKeyName");
        ASSERT_IS_NOT_NULL_WITH_MSG(key_name, "Failed retrieving primary keyname");

        const char* access_key = Map_GetValueFromKey(g_dps_info.conn_map, "SharedAccessKey");
        ASSERT_IS_NOT_NULL_WITH_MSG(access_key, "Failed retrieving key");

        // Remove device
        remove_enrollment_device(host_name, key_name, access_key);

        Map_Destroy(g_dps_info.conn_map);

        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
        free(g_dps_info.iothub_uri);
        free(g_dps_info.device_id);
    }

    TEST_FUNCTION(dps_register_device_http_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        DPS_LL_HANDLE handle;
        handle = create_dps_handle(DPS_HTTP_Protocol);

        // act
        DPS_RESULT dps_result = DPS_LL_Register_Device(handle, iothub_dps_register_device, &g_dps_info, dps_registation_status, &g_dps_info);
        ASSERT_ARE_EQUAL_WITH_MSG(DPS_RESULT, DPS_CLIENT_OK, dps_result, "Failure calling DPS_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            DPS_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ( (g_dps_info.reg_result == REG_RESULT_BEGIN) &&
            ( (now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME) ) );

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_dps_info.reg_result, "Failure calling registering device");

        DPS_LL_Destroy(handle);
    }

#if USE_AMQP
    TEST_FUNCTION(dps_register_device_amqp_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        DPS_LL_HANDLE handle;
        handle = create_dps_handle(DPS_AMQP_Protocol);

        // act
        DPS_RESULT dps_result = DPS_LL_Register_Device(handle, iothub_dps_register_device, &g_dps_info, dps_registation_status, &g_dps_info);
        ASSERT_ARE_EQUAL_WITH_MSG(DPS_RESULT, DPS_CLIENT_OK, dps_result, "Failure calling DPS_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            DPS_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ((g_dps_info.reg_result == REG_RESULT_BEGIN) &&
            ((now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_dps_info.reg_result, "Failure calling registering device");

        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(dps_register_device_amqp_ws_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        DPS_LL_HANDLE handle;
        handle = create_dps_handle(DPS_AMQP_WS_Protocol);

        // act
        DPS_RESULT dps_result = DPS_LL_Register_Device(handle, iothub_dps_register_device, &g_dps_info, dps_registation_status, &g_dps_info);
        ASSERT_ARE_EQUAL_WITH_MSG(DPS_RESULT, DPS_CLIENT_OK, dps_result, "Failure calling DPS_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            DPS_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ((g_dps_info.reg_result == REG_RESULT_BEGIN) &&
            ((now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_dps_info.reg_result, "Failure calling registering device");

        DPS_LL_Destroy(handle);
    }
#endif

#if USE_MQTT
    TEST_FUNCTION(dps_register_device_mqtt_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        DPS_LL_HANDLE handle;
        handle = create_dps_handle(DPS_MQTT_Protocol);

        // act
        DPS_RESULT dps_result = DPS_LL_Register_Device(handle, iothub_dps_register_device, &g_dps_info, dps_registation_status, &g_dps_info);
        ASSERT_ARE_EQUAL_WITH_MSG(DPS_RESULT, DPS_CLIENT_OK, dps_result, "Failure calling DPS_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            DPS_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ((g_dps_info.reg_result == REG_RESULT_BEGIN) &&
            ((now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_dps_info.reg_result, "Failure calling registering device");

        DPS_LL_Destroy(handle);
    }

    TEST_FUNCTION(dps_register_device_mqtt_ws_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        DPS_LL_HANDLE handle;
        handle = create_dps_handle(DPS_MQTT_WS_Protocol);

        // act
        DPS_RESULT dps_result = DPS_LL_Register_Device(handle, iothub_dps_register_device, &g_dps_info, dps_registation_status, &g_dps_info);
        ASSERT_ARE_EQUAL_WITH_MSG(DPS_RESULT, DPS_CLIENT_OK, dps_result, "Failure calling DPS_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            DPS_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ((g_dps_info.reg_result == REG_RESULT_BEGIN) &&
            ((now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_dps_info.reg_result, "Failure calling registering device");

        DPS_LL_Destroy(handle);
    }
#endif

END_TEST_SUITE(dps_client_e2e)
