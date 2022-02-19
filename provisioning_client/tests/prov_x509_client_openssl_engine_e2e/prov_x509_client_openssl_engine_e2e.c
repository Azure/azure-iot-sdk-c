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

BEGIN_TEST_SUITE(prov_x509_client_openssl_engine_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        platform_init();
        prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

        g_prov_conn_string = getenv(DPS_CONNECTION_STRING);
        ASSERT_IS_NOT_NULL(g_prov_conn_string, "DPS_CONNECTION_STRING is NULL");

        g_dps_uri = getenv(DPS_GLOBAL_ENDPOINT);
        ASSERT_IS_NOT_NULL(g_dps_uri, "DPS_GLOBAL_ENDPOINT is NULL");

        g_dps_scope_id = getenv(DPS_ID_SCOPE);
        ASSERT_IS_NOT_NULL(g_dps_scope_id, "DPS_ID_SCOPE is NULL");

        char* dps_x509_cert_individual_base64 = getenv(DPS_X509_INDIVIDUAL_CERT_BASE64);
        ASSERT_IS_NOT_NULL(dps_x509_cert_individual_base64, "DPS_X509_INDIVIDUAL_CERT_BASE64 is NULL");
        g_dps_x509_cert_individual = convert_base64_to_string(dps_x509_cert_individual_base64);

        g_dps_x509_key_individual = PKCS11_PRIVATE_KEY_URI;

        g_dps_regid_individual = getenv(DPS_X509_INDIVIDUAL_REGISTRATION_ID);
        ASSERT_IS_NOT_NULL(g_dps_regid_individual, "DPS_X509_INDIVIDUAL_REGISTRATION_ID is NULL");

        // Register device
        create_x509_individual_enrollment_device();
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        // For X509 Individual Enrollment we are using a single registration.
        // Removing it will cause issues with parallel test runs.

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
    TEST_FUNCTION(dps_register_x509_openssl_engine_device_http_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_HTTP_Protocol, g_enable_tracing);
    }
#endif

#if USE_AMQP
    TEST_FUNCTION(dps_register_x509_openssl_engine_device_amqp_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_AMQP_Protocol, g_enable_tracing);
    }

    TEST_FUNCTION(dps_register_x509_openssl_engine_device_amqp_ws_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_AMQP_WS_Protocol, g_enable_tracing);
    }
#endif

#if USE_MQTT
    TEST_FUNCTION(dps_register_x509_openssl_engine_device_mqtt_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_MQTT_Protocol, g_enable_tracing);
    }

    TEST_FUNCTION(dps_register_x509_openssl_engine_device_mqtt_ws_success)
    {
        send_dps_test_registration(g_dps_uri, g_dps_scope_id, Prov_Device_MQTT_WS_Protocol, g_enable_tracing);
    }
#endif
END_TEST_SUITE(prov_x509_client_openssl_engine_e2e)
