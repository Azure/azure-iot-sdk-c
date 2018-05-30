// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include "testrunnerswitcher.h"

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/internal/prov_auth_client.h"

#include "azure_prov_client/prov_transport_http_client.h"
#include "azure_prov_client/prov_transport_amqp_client.h"
#include "azure_prov_client/prov_transport_amqp_ws_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"

#include "common_prov_e2e.h"

static TEST_MUTEX_HANDLE g_dllByDll;

static const char* g_prov_conn_string = NULL;
static const char* g_dps_scope_id = NULL;
static const char* g_dps_uri = NULL;
static const char* g_desired_iothub = NULL;
static bool g_enable_tracing = true;

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
    ASSERT_IS_NOT_NULL_WITH_MSG(user_context, "user_context is NULL");
}

BEGIN_TEST_SUITE(prov_x509_client_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);

        platform_init();
        prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

        g_prov_conn_string = getenv(DPS_CONNECTION_STRING);
        ASSERT_IS_NOT_NULL_WITH_MSG(g_prov_conn_string, "DPS_CONNECTION_STRING is NULL");

        g_dps_uri = getenv(DPS_GLOBAL_ENDPOINT);
        ASSERT_IS_NOT_NULL_WITH_MSG(g_prov_conn_string, "DPS_GLOBAL_ENDPOINT is NULL");

        g_dps_scope_id = getenv(DPS_ID_SCOPE);
        ASSERT_IS_NOT_NULL_WITH_MSG(g_dps_scope_id, "DPS_ID_SCOPE is NULL");

        // Register device
        create_x509_enrollment_device(g_prov_conn_string, g_enable_tracing);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        // Remove device
        remove_enrollment_device(g_prov_conn_string);

        prov_dev_security_deinit();
        platform_deinit();

        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
    }

#if USE_HTTP
    TEST_FUNCTION(dps_register_device_http_success)
    {
        PROV_CLIENT_E2E_INFO prov_info;
        memset(&prov_info, 0, sizeof(PROV_CLIENT_E2E_INFO));

        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = Prov_Device_LL_Create(g_dps_uri, g_dps_scope_id, Prov_Device_HTTP_Protocol);
        ASSERT_IS_NOT_NULL_WITH_MSG(handle, "Failure create a DPS HANDLE");

        Prov_Device_LL_SetOption(handle, PROV_OPTION_LOG_TRACE, &g_enable_tracing);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device x509 http");

        free(prov_info.iothub_uri);
        free(prov_info.device_id);

        Prov_Device_LL_Destroy(handle);
    }
#endif

#if USE_AMQP
    TEST_FUNCTION(dps_register_device_amqp_success)
    {
        PROV_CLIENT_E2E_INFO prov_info;
        memset(&prov_info, 0, sizeof(PROV_CLIENT_E2E_INFO));

        // arrange
        PROV_DEVICE_LL_HANDLE handle;
        handle = Prov_Device_LL_Create(g_dps_uri, g_dps_scope_id, Prov_Device_HTTP_Protocol);
        ASSERT_IS_NOT_NULL_WITH_MSG(handle, "Failure create a DPS HANDLE");

        Prov_Device_LL_SetOption(handle, PROV_OPTION_LOG_TRACE, &g_enable_tracing);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device x509 amqp");

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
        handle = Prov_Device_LL_Create(g_dps_uri, g_dps_scope_id, Prov_Device_HTTP_Protocol);
        ASSERT_IS_NOT_NULL_WITH_MSG(handle, "Failure create a DPS HANDLE");

        Prov_Device_LL_SetOption(handle, PROV_OPTION_LOG_TRACE, &g_enable_tracing);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device x509 amqp ws");

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
        handle = Prov_Device_LL_Create(g_dps_uri, g_dps_scope_id, Prov_Device_HTTP_Protocol);
        ASSERT_IS_NOT_NULL_WITH_MSG(handle, "Failure create a DPS HANDLE");

        Prov_Device_LL_SetOption(handle, PROV_OPTION_LOG_TRACE, &g_enable_tracing);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device with x509 mqtt");

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
        handle = Prov_Device_LL_Create(g_dps_uri, g_dps_scope_id, Prov_Device_HTTP_Protocol);
        ASSERT_IS_NOT_NULL_WITH_MSG(handle, "Failure create a DPS HANDLE");

        Prov_Device_LL_SetOption(handle, PROV_OPTION_LOG_TRACE, &g_enable_tracing);

        // act
        PROV_DEVICE_RESULT prov_result = Prov_Device_LL_Register_Device(handle, iothub_prov_register_device, &prov_info, dps_registation_status, &prov_info);
        ASSERT_ARE_EQUAL_WITH_MSG(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, prov_result, "Failure calling Prov_Device_LL_Register_Device");

        wait_for_dps_result(handle, &prov_info);

        // Assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, REG_RESULT_COMPLETE, prov_info.reg_result, "Failure calling registering device x509 mqtt ws");

        free(prov_info.iothub_uri);
        free(prov_info.device_id);
        
        Prov_Device_LL_Destroy(handle);
    }
#endif
END_TEST_SUITE(prov_x509_client_e2e)
