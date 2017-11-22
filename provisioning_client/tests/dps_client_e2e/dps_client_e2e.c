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

#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/prov_auth_client.h"

#include "azure_prov_client/prov_transport_http_client.h"
#include "azure_prov_client/prov_transport_amqp_client.h"
#include "azure_prov_client/prov_transport_amqp_ws_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"

#include "azure_c_shared_utility/connection_string_parser.h" 
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "provisioning_service_client.h"
#include "provisioning_sc_enrollment.h"

#include "platform_process.h"

static TEST_MUTEX_HANDLE g_dllByDll;

typedef enum REGISTRATION_RESULT_TAG
{
    REG_RESULT_BEGIN,
    REG_RESULT_COMPLETE,
    REG_RESULT_FAILED
} REGISTRATION_RESULT;

typedef struct PROV_CLIENT_E2E_INFO_TAG
{
    char* iothub_uri;
    char* device_id;
    REGISTRATION_RESULT reg_result;
    MAP_HANDLE conn_map;
} PROV_CLIENT_E2E_INFO;

static const char* g_prov_conn_string = NULL;
static const char* g_dps_scope_id = NULL;
static char* g_dps_uri = NULL;
static const char* g_desired_iothub = NULL;

PROV_CLIENT_E2E_INFO g_prov_info;
PLATFORM_PROCESS_HANDLE g_emulator_proc;
SECURE_DEVICE_TYPE g_hsm_device_type;

#define MAX_CLOUD_TRAVEL_TIME       60.0
#define DEVICE_GUID_SIZE            37

#ifdef WIN32
const char* FOLDER_SEPARATOR = "\\";
#else
const char* FOLDER_SEPARATOR = "/";
#endif

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);

static void iothub_prov_register_device(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    if (user_context == NULL)
    {
        ASSERT_FAIL("User Context is NULL iothub_prov_register_device");
    }
    else
    {
        PROV_CLIENT_E2E_INFO* prov_info = (PROV_CLIENT_E2E_INFO*)user_context;
        if (register_result == PROV_DEVICE_RESULT_OK)
        {
            (void)mallocAndStrcpy_s(&prov_info->iothub_uri, iothub_uri);
            (void)mallocAndStrcpy_s(&prov_info->device_id, device_id);
            prov_info->reg_result = REG_RESULT_COMPLETE;
        }
        else
        {
            prov_info->reg_result = REG_RESULT_FAILED;
        }
    }
}

static void dps_registation_status(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
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

static PROV_DEVICE_LL_HANDLE create_dps_handle(PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION dps_transport)
{
    PROV_DEVICE_LL_HANDLE result;
    result = Prov_Device_LL_Create(g_dps_uri, g_dps_scope_id, dps_transport);
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

static int construct_device_id(const char* prefix, char** device_name)
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
        *device_name = (char*)malloc(len + 1);
        if (*device_name == NULL)
        {
            LogError("Failure allocating device ID.\r\n");
            result = __FAILURE__;
        }
        else
        {
            if (sprintf_s(*device_name, len + 1, prefix, deviceGuid) <= 0)
            {
                LogError("Failure constructing device ID.\r\n");
                free(*device_name);
                result = __FAILURE__;
            }
            else
            {
                LogInfo("Created Device %s.", *device_name);
                result = 0;
            }
        }
    }
    return result;
}

static void create_enrollment_device()
{
    /*INDIVIDUAL_ENROLLMENT enrollment;
    char* registration_id = NULL;
    char* certificate = NULL;

    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_handle = prov_sc_create_from_connection_string(g_prov_conn_string);
    ASSERT_IS_NOT_NULL_WITH_MSG(prov_sc_handle, "Failure creating provisioning service client");

    char* device_id;

    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, construct_device_id("device_", &device_id), "Failure creating device name");

    PROV_AUTH_HANDLE auth_handle = prov_auth_create();
    ASSERT_IS_NOT_NULL_WITH_MSG(auth_handle, "Failure creating auth client");

    enrollment.device_id = device_id;

    if (prov_auth_get_type(auth_handle) == PROV_AUTH_TYPE_X509)
    {
        registration_id = prov_auth_get_registration_id(auth_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(registration_id, "Failure retrieving registration Id");
        certificate = prov_auth_get_certificate(auth_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(certificate, "Failure retrieving certificate");

        enrollment.registration_id = registration_id;
        //enrollment.attestation->attestation.x509.client_certificates->primary->certificate = ;
    }
    else
    {
    }
    //ASSERT_ARE_EQUAL_WITH_MSG(int, 0, prov_sc_create_or_update_individual_enrollment(prov_sc_handle, &enrollment), "Failure creating enrollment");

    free(registration_id);
    free(certificate);
    prov_auth_destroy(auth_handle);
    prov_sc_destroy(prov_sc_handle);*/
}

static void remove_enrollment_device()
{
    /*INDIVIDUAL_ENROLLMENT enrollment;
    char* registration_id = NULL;
    char* certificate = NULL;

    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_handle = prov_sc_create_from_connection_string(g_prov_conn_string);
    ASSERT_IS_NOT_NULL_WITH_MSG(prov_sc_handle, "Failure creating provisioning service client");

    PROV_AUTH_HANDLE auth_handle = prov_auth_create();
    ASSERT_IS_NOT_NULL_WITH_MSG(auth_handle, "Failure creating auth client");

    if (prov_auth_get_type(auth_handle) == PROV_AUTH_TYPE_X509)
    {
        registration_id = prov_auth_get_registration_id(auth_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(registration_id, "Failure retrieving registration Id");

        enrollment.registration_id = registration_id;
    }
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, prov_sc_delete_individual_enrollment(prov_sc_handle, &enrollment), "Failure deleting enrollment");

    free(registration_id);
    free(certificate);
    prov_auth_destroy(auth_handle);
    prov_sc_destroy(prov_sc_handle);*/
}

BEGIN_TEST_SUITE(dps_client_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);

        memset(&g_prov_info, 0, sizeof(PROV_CLIENT_E2E_INFO));

        platform_init();
        prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

        g_prov_conn_string = getenv("PROV_CONNECTION_STRING");
        ASSERT_IS_NOT_NULL_WITH_MSG(g_prov_conn_string, "PROV_CONNECTION_STRING is NULL");

        // Start Emulator
        /*if (g_dps_hsm_type == DPS_SEC_TYPE_TPM)
        {
            start_tpm_emulator();
        }*/

        // Register device
        create_enrollment_device();

        g_dps_scope_id = getenv("DPS_SCOPE_ID_VALUE");
        ASSERT_IS_NOT_NULL_WITH_MSG(g_dps_scope_id, "DPS_SCOPE_ID_VALUE is NULL");
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        // Stop Emulator
        /*if (g_dps_hsm_type == DPS_SEC_TYPE_TPM)
        {
            stop_tpm_emulator();
        }*/

        // Remove device
        remove_enrollment_device();

        prov_dev_security_deinit();
        platform_deinit();

        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
        free(g_prov_info.iothub_uri);
        free(g_prov_info.device_id);
    }

    TEST_FUNCTION(dps_register_device_http_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_HTTP_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &g_prov_info, dps_registation_status, &g_prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            Prov_Device_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ( (g_prov_info.reg_result == REG_RESULT_BEGIN) &&
            ( (now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME) ) );

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_prov_info.reg_result, "Failure calling registering device");

        Prov_Device_LL_Destroy(handle);
    }

#if USE_AMQP
    TEST_FUNCTION(dps_register_device_amqp_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_AMQP_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &g_prov_info, dps_registation_status, &g_prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            Prov_Device_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ((g_prov_info.reg_result == REG_RESULT_BEGIN) &&
            ((now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_prov_info.reg_result, "Failure calling registering device");

        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(dps_register_device_amqp_ws_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_AMQP_WS_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &g_prov_info, dps_registation_status, &g_prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            Prov_Device_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ((g_prov_info.reg_result == REG_RESULT_BEGIN) &&
            ((now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_prov_info.reg_result, "Failure calling registering device");

        Prov_Device_LL_Destroy(handle);
    }
#endif

#if USE_MQTT
    TEST_FUNCTION(dps_register_device_mqtt_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_MQTT_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &g_prov_info, dps_registation_status, &g_prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            Prov_Device_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ((g_prov_info.reg_result == REG_RESULT_BEGIN) &&
            ((now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_prov_info.reg_result, "Failure calling registering device");

        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(dps_register_device_mqtt_ws_success)
    {
        time_t begin_operation;
        time_t now_time;
        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_MQTT_WS_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &g_prov_info, dps_registation_status, &g_prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        begin_operation = time(NULL);
        do
        {
            Prov_Device_LL_DoWork(handle);
            ThreadAPI_Sleep(1);
        } while ((g_prov_info.reg_result == REG_RESULT_BEGIN) &&
            ((now_time = time(NULL)),
            (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, g_prov_info.reg_result, "Failure calling registering device");

        Prov_Device_LL_Destroy(handle);
    }
#endif

END_TEST_SUITE(dps_client_e2e)
