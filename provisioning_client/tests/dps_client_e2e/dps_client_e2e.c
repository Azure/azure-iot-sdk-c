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
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/azure_base64.h"

#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/internal/prov_auth_client.h"

#include "azure_prov_client/prov_transport_http_client.h"
#include "azure_prov_client/prov_transport_amqp_client.h"
#include "azure_prov_client/prov_transport_amqp_ws_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"

#include "azure_c_shared_utility/connection_string_parser.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "prov_service_client/provisioning_service_client.h"
#include "prov_service_client/provisioning_sc_enrollment.h"

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
} PROV_CLIENT_E2E_INFO;

static const char* g_prov_conn_string = NULL;
static const char* g_dps_scope_id = NULL;
static const char* const g_dps_uri = "global.azure-devices-provisioning.net";
static const char* g_desired_iothub = NULL;

#define MAX_CLOUD_TRAVEL_TIME       60.0
#define DEVICE_GUID_SIZE            37

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

static PROV_DEVICE_LL_HANDLE create_dps_handle(PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION dps_transport)
{
    PROV_DEVICE_LL_HANDLE result;
    result = Prov_Device_LL_Create(g_dps_uri, g_dps_scope_id, dps_transport);
    ASSERT_IS_NOT_NULL(result, "Failure create a DPS HANDLE");
    return result;
}

static void wait_for_dps_result(PROV_DEVICE_LL_HANDLE handle, PROV_CLIENT_E2E_INFO* prov_info)
{
    time_t begin_operation;
    time_t now_time;

    begin_operation = time(NULL);
    do
    {
        Prov_Device_LL_DoWork(handle);
        ThreadAPI_Sleep(1);
    } while ((prov_info->reg_result == REG_RESULT_BEGIN) &&
        ((now_time = time(NULL)),
        (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));
}

static int construct_device_id(const char* prefix, char** device_name)
{
    int result;
    char deviceGuid[DEVICE_GUID_SIZE];
    if (UniqueId_Generate(deviceGuid, DEVICE_GUID_SIZE) != UNIQUEID_OK)
    {
        LogError("Unable to generate unique Id.\r\n");
        result = MU_FAILURE;
    }
    else
    {
        size_t len = strlen(prefix) + DEVICE_GUID_SIZE;
        *device_name = (char*)malloc(len + 1);
        if (*device_name == NULL)
        {
            LogError("Failure allocating device ID.\r\n");
            result = MU_FAILURE;
        }
        else
        {
            if (sprintf_s(*device_name, len + 1, prefix, deviceGuid) <= 0)
            {
                LogError("Failure constructing device ID.\r\n");
                free(*device_name);
                result = MU_FAILURE;
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

static void create_tpm_enrollment_device()
{
    INDIVIDUAL_ENROLLMENT_HANDLE indiv_enrollment = NULL;

    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_handle = prov_sc_create_from_connection_string(g_prov_conn_string);
    ASSERT_IS_NOT_NULL(prov_sc_handle, "Failure creating provisioning service client");

    prov_sc_set_trace(prov_sc_handle, TRACING_STATUS_ON);

    PROV_AUTH_HANDLE auth_handle = prov_auth_create();
    ASSERT_IS_NOT_NULL(auth_handle, "Failure creating auth client");

    char* registration_id = prov_auth_get_registration_id(auth_handle);
    ASSERT_IS_NOT_NULL(registration_id, "Failure prov_auth_get_common_name");

    if (prov_sc_get_individual_enrollment(prov_sc_handle, registration_id, &indiv_enrollment) != 0)
    {
        BUFFER_HANDLE ek_handle = prov_auth_get_endorsement_key(auth_handle);
        ASSERT_IS_NOT_NULL(ek_handle, "Failure prov_auth_get_endorsement_key");

        STRING_HANDLE ek_value = Azure_Base64_Encode(ek_handle);
        ASSERT_IS_NOT_NULL(ek_value, "Failure Azure_Base64_Encode Endorsement key");

        ATTESTATION_MECHANISM_HANDLE attest_handle = attestationMechanism_createWithTpm(STRING_c_str(ek_value), NULL);
        ASSERT_IS_NOT_NULL(attest_handle, "Failure attestationMechanism_createWithTpm");

        indiv_enrollment = individualEnrollment_create(registration_id, attest_handle);
        ASSERT_IS_NOT_NULL(indiv_enrollment, "Failure hsm_client_riot_get_certificate ");

        BUFFER_delete(ek_handle);
        STRING_delete(ek_value);
    }
    free(registration_id);
    individualEnrollment_destroy(indiv_enrollment);
    prov_auth_destroy(auth_handle);
    prov_sc_destroy(prov_sc_handle);
}

static void create_x509_enrollment_device()
{
    INDIVIDUAL_ENROLLMENT_HANDLE indiv_enrollment = NULL;

    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_handle = prov_sc_create_from_connection_string(g_prov_conn_string);
    ASSERT_IS_NOT_NULL(prov_sc_handle, "Failure creating provisioning service client");

    prov_sc_set_trace(prov_sc_handle, TRACING_STATUS_ON);

    PROV_AUTH_HANDLE auth_handle = prov_auth_create();
    ASSERT_IS_NOT_NULL(auth_handle, "Failure creating auth client");

    char* registration_id = prov_auth_get_registration_id(auth_handle);
    ASSERT_IS_NOT_NULL(registration_id, "Failure prov_auth_get_common_name");

    if (prov_sc_get_individual_enrollment(prov_sc_handle, registration_id, &indiv_enrollment) != 0)
    {
        char* x509_cert = prov_auth_get_certificate(auth_handle);
        ASSERT_IS_NOT_NULL(x509_cert, "Failure prov_auth_get_certificate");

        STRING_HANDLE base64_cert = Azure_Base64_Encode_Bytes((const unsigned char*)x509_cert, strlen(x509_cert));
        ASSERT_IS_NOT_NULL(base64_cert, "Failure Azure_Base64_Encode_Bytes");

        ATTESTATION_MECHANISM_HANDLE attest_handle = attestationMechanism_createWithX509ClientCert(STRING_c_str(base64_cert), NULL);
        ASSERT_IS_NOT_NULL(attest_handle, "Failure hsm_client_riot_get_certificate ");

        indiv_enrollment = individualEnrollment_create(registration_id, attest_handle);
        ASSERT_IS_NOT_NULL(indiv_enrollment, "Failure hsm_client_riot_get_certificate ");

        ASSERT_ARE_EQUAL(int, 0, prov_sc_create_or_update_individual_enrollment(prov_sc_handle, &indiv_enrollment), "Failure prov_sc_create_or_update_individual_enrollment");

        STRING_delete(base64_cert);
        free(x509_cert);
    }
    free(registration_id);
    individualEnrollment_destroy(indiv_enrollment);
    prov_auth_destroy(auth_handle);
    prov_sc_destroy(prov_sc_handle);
}

static void remove_x509_enrollment_device()
{
    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_handle = prov_sc_create_from_connection_string(g_prov_conn_string);
    ASSERT_IS_NOT_NULL(prov_sc_handle, "Failure creating provisioning service client");

    PROV_AUTH_HANDLE auth_handle = prov_auth_create();
    ASSERT_IS_NOT_NULL(auth_handle, "Failure creating auth client");

    char* registration_id = prov_auth_get_registration_id(auth_handle);
    ASSERT_IS_NOT_NULL(registration_id, "Failure retrieving registration Id");

    ASSERT_ARE_EQUAL(int, 0, prov_sc_delete_individual_enrollment_by_param(prov_sc_handle, registration_id, NULL), "Failure deleting enrollment");

    free(registration_id);
    prov_auth_destroy(auth_handle);
    prov_sc_destroy(prov_sc_handle);
}

BEGIN_TEST_SUITE(dps_client_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        platform_init();

        prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);

        g_prov_conn_string = getenv("DPS_C_CONNECTION_STRING");
        ASSERT_IS_NOT_NULL(g_prov_conn_string, "PROV_CONNECTION_STRING is NULL");

        // Register device
        create_tpm_enrollment_device();
        //create_x509_enrollment_device();

        g_dps_scope_id = getenv("DPS_C_SCOPE_ID_VALUE");
        ASSERT_IS_NOT_NULL(g_dps_scope_id, "DPS_SCOPE_ID_VALUE is NULL");
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        // Remove device
        remove_x509_enrollment_device();

        prov_dev_security_deinit();
        platform_deinit();
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
    }

    TEST_FUNCTION(dps_register_device_http_success)
    {
        PROV_CLIENT_E2E_INFO prov_info;
        memset(&prov_info, 0, sizeof(PROV_CLIENT_E2E_INFO));

        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_HTTP_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device x509 http");

        free(prov_info.iothub_uri);
        free(prov_info.device_id);

        Prov_Device_LL_Destroy(handle);
    }

#if USE_AMQP
    TEST_FUNCTION(dps_register_device_amqp_success)
    {
        PROV_CLIENT_E2E_INFO prov_info;
        memset(&prov_info, 0, sizeof(PROV_CLIENT_E2E_INFO));

        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_AMQP_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device x509 amqp");

        free(prov_info.iothub_uri);
        free(prov_info.device_id);

        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(dps_register_device_amqp_ws_success)
    {
        PROV_CLIENT_E2E_INFO prov_info;
        memset(&prov_info, 0, sizeof(PROV_CLIENT_E2E_INFO));

        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_AMQP_WS_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device x509 amqp ws");

        free(prov_info.iothub_uri);
        free(prov_info.device_id);

        Prov_Device_LL_Destroy(handle);
    }
#endif

#if USE_MQTT
    TEST_FUNCTION(dps_register_device_mqtt_success)
    {
        PROV_CLIENT_E2E_INFO prov_info;
        memset(&prov_info, 0, sizeof(PROV_CLIENT_E2E_INFO));

        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_MQTT_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device with x509 mqtt");

        free(prov_info.iothub_uri);
        free(prov_info.device_id);

        Prov_Device_LL_Destroy(handle);
    }

    TEST_FUNCTION(dps_register_device_mqtt_ws_success)
    {
        PROV_CLIENT_E2E_INFO prov_info;
        memset(&prov_info, 0, sizeof(PROV_CLIENT_E2E_INFO));

        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = create_dps_handle(Prov_Device_MQTT_WS_Protocol);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device x509 mqtt ws");

        free(prov_info.iothub_uri);
        free(prov_info.device_id);

        Prov_Device_LL_Destroy(handle);
    }
#endif
END_TEST_SUITE(dps_client_e2e)
