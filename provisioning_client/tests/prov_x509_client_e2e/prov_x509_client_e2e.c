// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include "testrunnerswitcher.h"

#include "azure_c_shared_utility/azure_base64.h"
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

const char* g_prov_conn_string = NULL;
const char* g_dps_scope_id = NULL;
const char* g_dps_uri = NULL;
const char* g_desired_iothub = NULL;
char* g_dps_x509_cert_individual = NULL;
char* g_dps_x509_key_individual = NULL;
char* g_dps_regid_individual = NULL;
const bool g_enable_tracing = true;

static const int TEST_PROV_RANDOMIZED_BACK_OFF_SEC = 60;

BEGIN_TEST_SUITE(prov_x509_client_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        platform_init();
        prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

        g_prov_conn_string = getenv(DPS_CONNECTION_STRING);
        ASSERT_IS_NOT_NULL(g_prov_conn_string, "Environment variable DPS_CONNECTION_STRING is not set or empty.");

        g_dps_uri = getenv(DPS_GLOBAL_ENDPOINT);
        ASSERT_IS_NOT_NULL(g_dps_uri, "Environment variable DPS_GLOBAL_ENDPOINT is not set or empty.");

        g_dps_scope_id = getenv(DPS_ID_SCOPE);
        ASSERT_IS_NOT_NULL(g_dps_scope_id, "Environment variable DPS_ID_SCOPE is not set or empty.");

#ifdef HSM_TYPE_X509
        char* dps_x509_cert_individual_base64 = getenv(DPS_X509_INDIVIDUAL_CERT_BASE64);
        ASSERT_IS_NOT_NULL(dps_x509_cert_individual_base64, "Environment variable DPS_X509_INDIVIDUAL_CERT_BASE64 is not set or empty.");
        g_dps_x509_cert_individual = convert_base64_to_string(dps_x509_cert_individual_base64);

        char* dps_x509_key_individual = getenv(DPS_X509_INDIVIDUAL_KEY_BASE64);
        ASSERT_IS_NOT_NULL(dps_x509_key_individual, "Environment variable DPS_X509_INDIVIDUAL_KEY_BASE64 is not set or empty.");
        g_dps_x509_key_individual = convert_base64_to_string(dps_x509_key_individual);

        g_dps_regid_individual = getenv(DPS_X509_INDIVIDUAL_REGISTRATION_ID);
        ASSERT_IS_NOT_NULL(g_dps_regid_individual, "Environment variable DPS_X509_INDIVIDUAL_REGISTRATION_ID is not set or empty.");
#endif

        // DPS fails when having multiple enrollments at the same time. 
        // Since we are running these tests on multiple machines with each test taking about 1 second, 
        // we randomize their start time to avoid collisions.
        srand(time(0));
        int random_back_off_sec = rand() % TEST_PROV_RANDOMIZED_BACK_OFF_SEC;
        LogInfo("prov_x509_client_e2e: Random back-off = %ds", random_back_off_sec);
        ThreadAPI_Sleep(random_back_off_sec * 1000);

        // Register device
        create_x509_individual_enrollment_device();
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        // Remove device
#ifndef HSM_TYPE_X509
        // For X509 Individual Enrollment we are using a single registration.
        // Removing it will cause issues with parallel test runs.
        remove_enrollment_device(g_prov_conn_string);
#endif

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
    TEST_FUNCTION(dps_register_x509_device_http_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_HTTP_Protocol, g_enable_tracing);
    }
#endif

#if USE_AMQP
    TEST_FUNCTION(dps_register_x509_device_amqp_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_AMQP_Protocol, g_enable_tracing);
    }

    TEST_FUNCTION(dps_register_x509_device_amqp_ws_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_AMQP_WS_Protocol, g_enable_tracing);
    }
#endif

#if USE_MQTT
    TEST_FUNCTION(dps_register_x509_device_mqtt_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_MQTT_Protocol, g_enable_tracing);
    }

    TEST_FUNCTION(dps_register_x509_device_mqtt_ws_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_MQTT_WS_Protocol, g_enable_tracing);
    }
#endif
END_TEST_SUITE(prov_x509_client_e2e)
