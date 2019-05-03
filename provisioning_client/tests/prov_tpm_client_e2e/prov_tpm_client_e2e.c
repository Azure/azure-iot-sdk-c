// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include "testrunnerswitcher.h"

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/internal/prov_auth_client.h"

#include "azure_prov_client/prov_transport_http_client.h"
#include "azure_prov_client/prov_transport_amqp_client.h"
#include "azure_prov_client/prov_transport_amqp_ws_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"

#include "common_prov_e2e.h"

static const char* g_prov_conn_string = NULL;
static const char* g_dps_scope_id = NULL;
static const char* g_dps_uri = NULL;
static const char* g_desired_iothub = NULL;
static bool g_enable_tracing = true;

BEGIN_TEST_SUITE(prov_tpm_client_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        platform_init();
        prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);

        g_prov_conn_string = getenv(DPS_CONNECTION_STRING);
        ASSERT_IS_NOT_NULL(g_prov_conn_string, "PROV_CONNECTION_STRING is NULL");

        g_dps_uri = getenv(DPS_GLOBAL_ENDPOINT);
        ASSERT_IS_NOT_NULL(g_dps_uri, "DPS_GLOBAL_ENDPOINT is NULL");

        g_dps_scope_id = getenv(DPS_ID_SCOPE);
        ASSERT_IS_NOT_NULL(g_dps_scope_id, "DPS_ID_SCOPE is NULL");

        // Register device
        create_tpm_enrollment_device(g_prov_conn_string, g_enable_tracing);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        // Remove device
        remove_enrollment_device(g_prov_conn_string);

        prov_dev_security_deinit();
        platform_deinit();
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
    }

#if USE_HTTP
    TEST_FUNCTION(dps_register_tpm_device_http_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_HTTP_Protocol, g_enable_tracing);
    }
#endif

#if USE_AMQP
    TEST_FUNCTION(dps_register_tpm_device_amqp_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_AMQP_Protocol, g_enable_tracing);
    }

    TEST_FUNCTION(dps_register_tpm_device_amqp_ws_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_AMQP_WS_Protocol, g_enable_tracing);
    }
#endif
END_TEST_SUITE(prov_tpm_client_e2e)
