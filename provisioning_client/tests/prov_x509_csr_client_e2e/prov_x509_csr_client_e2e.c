// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CSR (Certificate Signing Request) E2E test for the Azure IoT C SDK.
// Tests CSR scenarios using the convenience layer API:
//   - X509 individual enrollment: full 4-phase lifecycle
//   - Symmetric key group enrollment: full 4-phase lifecycle

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "testrunnerswitcher.h"

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/csr_gen.h"

#include "azure_prov_client/prov_device_client.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_prov_client/internal/prov_auth_client.h"

#include "azure_prov_client/prov_transport_mqtt_client.h"

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"

#include "common_prov_e2e.h"

#include "hsm_client_x509.h"

// Test filter support — when main.c receives a test name argument,
// only the matching TEST_FUNCTION runs; the rest are skipped.
extern const char* csr_test_get_filter(void);
#define SKIP_IF_FILTERED(testName) \
    do { \
        const char* _f = csr_test_get_filter(); \
        if (_f != NULL && strcmp(_f, #testName) != 0) { return; } \
    } while (0)

TEST_DEFINE_ENUM_TYPE(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

// Globals required by common_prov_e2e.h
const char* g_prov_conn_string = NULL;
const char* g_dps_scope_id = NULL;
const char* g_dps_uri = NULL;
const char* g_desired_iothub = NULL;
char* g_dps_x509_cert_individual = NULL;
char* g_dps_x509_key_individual = NULL;
char* g_dps_regid_individual = NULL;
const bool g_enable_tracing = true;

// CSR-specific env var names
static const char* ENV_ADR_CERT_MGMT_POLICY_NAME = "ADR_CERT_MGMT_POLICY_NAME";
static const char* ENV_SYMM_KEY_GROUP_ENROLLMENT_ID = "IOT_DPS_SYMM_KEY_GROUP_ENROLLMENT_ID";
static const char* ENV_SYMM_KEY_GROUP_PRIMARY_KEY = "IOT_DPS_SYMM_KEY_GROUP_PRIMARY_KEY";

// Timeouts
#define MAX_WAIT_TIME_SECS          180
#define CSR_TIMEOUT_SECS            60
#define GUID_SIZE                   37

// CSR callback context for IoT Hub CSR
typedef struct CSR_CALLBACK_CONTEXT_TAG
{
    volatile bool response_received;
    IOTHUB_CLIENT_CONFIRMATION_RESULT result;
    int response_status_code;
    char* payload;
} CSR_CALLBACK_CONTEXT;

// Connection status context
typedef struct CONNECTION_STATUS_CONTEXT_TAG
{
    volatile bool connected;
    volatile bool connection_failed;
} CONNECTION_STATUS_CONTEXT;

// DPS registration context (convenience layer)
typedef struct DPS_CONV_REGISTRATION_CONTEXT_TAG
{
    volatile bool registration_complete;
    PROV_DEVICE_RESULT reg_result;
    char* iothub_uri;
    char* device_id;
} DPS_CONV_REGISTRATION_CONTEXT;

// ----- DPS callbacks -----

static void dps_registration_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    (void)user_context;
    LogInfo("DPS status: %d", (int)reg_status);
}

static void dps_conv_register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    if (user_context == NULL)
    {
        LogError("DPS conv register callback: user_context is NULL");
        return;
    }

    DPS_CONV_REGISTRATION_CONTEXT* ctx = (DPS_CONV_REGISTRATION_CONTEXT*)user_context;

    if (register_result == PROV_DEVICE_RESULT_OK)
    {
        (void)mallocAndStrcpy_s(&ctx->iothub_uri, iothub_uri);
        (void)mallocAndStrcpy_s(&ctx->device_id, device_id);
        ctx->reg_result = PROV_DEVICE_RESULT_OK;
    }
    else
    {
        ctx->reg_result = register_result;
    }

    ctx->registration_complete = true;
}

// ----- IoT Hub callbacks -----

static void iothub_connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    CONNECTION_STATUS_CONTEXT* ctx = (CONNECTION_STATUS_CONTEXT*)user_context;
    if (ctx == NULL) return;

    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        ctx->connected = true;
        ctx->connection_failed = false;
        LogInfo("IoT Hub connected");
    }
    else
    {
        ctx->connected = false;
        ctx->connection_failed = true;
        LogInfo("IoT Hub disconnected (reason=%d)", (int)reason);
    }
}

static void hub_csr_response_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, int response_status_code, const char* responsePayload, void* userContextCallback)
{
    CSR_CALLBACK_CONTEXT* ctx = (CSR_CALLBACK_CONTEXT*)userContextCallback;

    LogInfo("CSR response (result=%d, status=%d)", (int)result, response_status_code);

    if (result == IOTHUB_CLIENT_CONFIRMATION_ACCEPTED)
    {
        // Intermediate 202 response — request accepted, final response pending
        LogInfo("CSR intermediate response (%d), waiting...", response_status_code);
        return;
    }

    ctx->result = result;
    ctx->response_status_code = response_status_code;

    if (result == IOTHUB_CLIENT_CONFIRMATION_OK && response_status_code == 200 && responsePayload != NULL)
    {
        (void)mallocAndStrcpy_s(&ctx->payload, responsePayload);
    }

    ctx->response_received = true;
}

// ----- Certificate / CSR helpers -----
// Certificate format helpers and CSR/key generation live outside this file:
//   - base64-DER <-> PEM and CSR response parsing are in common_prov_e2e.c
//   - ECDSA P-256 keygen + CSR signing lives in c-utility/adapters/csr_gen_*
// so this test file has zero direct dependency on OpenSSL / Schannel.

// Context holding a generated CSR (base64 DER) and its private key (PKCS#8 PEM).
typedef struct GENERATED_CSR_TAG
{
    char* csr_base64;
    char* private_key_pem;
} GENERATED_CSR;

static GENERATED_CSR generate_ec_csr(const char* common_name)
{
    GENERATED_CSR result;
    memset(&result, 0, sizeof(result));
    ASSERT_ARE_EQUAL(int, 0,
        csr_gen_ec_p256(common_name, &result.csr_base64, &result.private_key_pem),
        "csr_gen_ec_p256 failed");
    ASSERT_IS_NOT_NULL(result.csr_base64, "csr_gen_ec_p256 returned NULL csr_base64");
    ASSERT_IS_NOT_NULL(result.private_key_pem, "csr_gen_ec_p256 returned NULL private_key_pem");
    return result;
}

// ----- Wait helpers -----

static bool wait_for_connection(CONNECTION_STATUS_CONTEXT* ctx, int timeout_secs)
{
    time_t start = time(NULL);
    while (!ctx->connected && !ctx->connection_failed)
    {
        ThreadAPI_Sleep(500);
        if (difftime(time(NULL), start) > timeout_secs)
        {
            return false;
        }
    }
    return ctx->connected;
}

static bool wait_for_csr_response(CSR_CALLBACK_CONTEXT* ctx, int timeout_secs)
{
    time_t start = time(NULL);
    while (!ctx->response_received)
    {
        ThreadAPI_Sleep(500);
        if (difftime(time(NULL), start) > timeout_secs)
        {
            return false;
        }
    }
    return true;
}

// Generate a unique request ID string
static char* generate_request_id(void)
{
    char guid[GUID_SIZE];
    ASSERT_ARE_EQUAL(int, UNIQUEID_OK, (int)UniqueId_Generate(guid, GUID_SIZE), "Failed to generate GUID");

    char* request_id = NULL;
    ASSERT_ARE_EQUAL(int, 0, mallocAndStrcpy_s(&request_id, guid), "Failed to copy request ID");
    return request_id;
}


BEGIN_TEST_SUITE(prov_x509_csr_client_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        platform_init();
        prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

        // Check if CSR/ADR tests should run
        const char* adr_policy = getenv(ENV_ADR_CERT_MGMT_POLICY_NAME);
        if (adr_policy == NULL || adr_policy[0] == '\0')
        {
            LogInfo("*** SKIPPING CSR E2E tests: %s not set ***", ENV_ADR_CERT_MGMT_POLICY_NAME);
            // We do not ASSERT here; test functions will skip individually.
        }

        g_prov_conn_string = getenv(DPS_CONNECTION_STRING);
        ASSERT_IS_NOT_NULL(g_prov_conn_string, "DPS_CONNECTION_STRING not set");

        g_dps_uri = getenv(DPS_GLOBAL_ENDPOINT);
        ASSERT_IS_NOT_NULL(g_dps_uri, "DPS_GLOBAL_ENDPOINT not set");

        g_dps_scope_id = getenv(DPS_ID_SCOPE);
        ASSERT_IS_NOT_NULL(g_dps_scope_id, "DPS_ID_SCOPE not set");

#ifdef HSM_TYPE_X509
        {
            char* cert_b64 = getenv(DPS_X509_INDIVIDUAL_CERT_BASE64);
            ASSERT_IS_NOT_NULL(cert_b64, "DPS_X509_INDIVIDUAL_CERT_BASE64 not set");
            g_dps_x509_cert_individual = convert_base64_to_string(cert_b64);

            char* key_b64 = getenv(DPS_X509_INDIVIDUAL_KEY_BASE64);
            ASSERT_IS_NOT_NULL(key_b64, "DPS_X509_INDIVIDUAL_KEY_BASE64 not set");
            g_dps_x509_key_individual = convert_base64_to_string(key_b64);

            g_dps_regid_individual = getenv(DPS_X509_INDIVIDUAL_REGISTRATION_ID);
            ASSERT_IS_NOT_NULL(g_dps_regid_individual, "DPS_X509_INDIVIDUAL_REGISTRATION_ID not set");
        }
#endif

        // Randomized back-off to avoid DPS collision from parallel test runs
        srand((unsigned int)time(NULL));
        int random_back_off_sec = rand() % TEST_PROV_RANDOMIZED_BACK_OFF_SEC;
        LogInfo("prov_x509_csr_client_e2e: Random back-off = %ds", random_back_off_sec);
        ThreadAPI_Sleep(random_back_off_sec * 1000);

        create_x509_individual_enrollment_device();
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        prov_dev_security_deinit();
        platform_deinit();
    }

    TEST_FUNCTION_INITIALIZE(method_init)
    {
        // Re-init HSM security to reset cert/key state between tests.
        // Each CSR test modifies the HSM (installs issued certs),
        // so subsequent tests need a clean slate with the original bootstrap certs.
        prov_dev_security_deinit();
        iothub_security_deinit();
        prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

        // Re-init IoT Hub SDK to clear any leftover transport state
        IoTHub_Deinit();
        IoTHub_Init();
    }

    TEST_FUNCTION_CLEANUP(method_cleanup)
    {
    }

    // Full 4-phase CSR lifecycle test (X509 individual enrollment, convenience layer):
    //   Phase 1: DPS registration with CSR
    //   Phase 2: Connect to IoT Hub with DPS-issued cert
    //   Phase 3: IoT Hub CSR for certificate re-issuance
    //   Phase 4: Reconnect with re-issued cert
    TEST_FUNCTION(dps_x509_csr_full_lifecycle_mqtt)
    {
        SKIP_IF_FILTERED(dps_x509_csr_full_lifecycle_mqtt);

        // Gate on ADR policy being configured
        const char* adr_policy = getenv(ENV_ADR_CERT_MGMT_POLICY_NAME);
        if (adr_policy == NULL || adr_policy[0] == '\0')
        {
            LogInfo("SKIPPED: %s not set", ENV_ADR_CERT_MGMT_POLICY_NAME);
            return;
        }

        // ================================================================
        // Phase 1: DPS Registration with CSR (convenience layer)
        // ================================================================
        LogInfo("=== Phase 1: DPS Registration with CSR ===");

        DPS_CONV_REGISTRATION_CONTEXT dps_ctx;
        memset(&dps_ctx, 0, sizeof(dps_ctx));

        PROV_DEVICE_HANDLE prov_handle = Prov_Device_Create(g_dps_uri, g_dps_scope_id, Prov_Device_MQTT_Protocol);
        ASSERT_IS_NOT_NULL(prov_handle, "Failed to create DPS handle");

        bool trace_on = true;
        Prov_Device_SetOption(prov_handle, PROV_OPTION_LOG_TRACE, &trace_on);

#ifdef HSM_TYPE_X509
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK,
            Prov_Device_SetOption(prov_handle, OPTION_X509_CERT, g_dps_x509_cert_individual),
            "Failed to set DPS X509 cert");
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK,
            Prov_Device_SetOption(prov_handle, OPTION_X509_PRIVATE_KEY, g_dps_x509_key_individual),
            "Failed to set DPS X509 key");
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK,
            Prov_Device_SetOption(prov_handle, PROV_REGISTRATION_ID, g_dps_regid_individual),
            "Failed to set DPS registration ID");
#endif

        GENERATED_CSR dps_csr = generate_ec_csr(g_dps_regid_individual ? g_dps_regid_individual : "csr-e2e-device");

        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK,
            Prov_Device_SetOption(prov_handle, PROV_CERTIFICATE_SIGNING_REQUEST, dps_csr.csr_base64),
            "Failed to set DPS CSR");
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK,
            Prov_Device_SetOption(prov_handle, PROV_CERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY, dps_csr.private_key_pem),
            "Failed to set DPS CSR private key");

        PROV_DEVICE_RESULT reg_result = Prov_Device_Register_Device(prov_handle,
            dps_conv_register_device_callback, &dps_ctx,
            dps_registration_status_callback, NULL);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, reg_result, "Failed to start DPS registration");

        {
            time_t start = time(NULL);
            while (!dps_ctx.registration_complete)
            {
                ThreadAPI_Sleep(500);
                ASSERT_IS_TRUE(difftime(time(NULL), start) < MAX_WAIT_TIME_SECS, "DPS registration timed out");
            }
        }

        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, dps_ctx.reg_result, "DPS registration failed");
        ASSERT_IS_NOT_NULL(dps_ctx.iothub_uri, "DPS did not return iothub_uri");
        ASSERT_IS_NOT_NULL(dps_ctx.device_id, "DPS did not return device_id");

        LogInfo("Phase 1 complete: Registered device '%s' on '%s'", dps_ctx.device_id, dps_ctx.iothub_uri);

        Prov_Device_Destroy(prov_handle);
        prov_handle = NULL;

        // ================================================================
        // Phase 2: Connect to IoT Hub with DPS-issued certificate
        // ================================================================
        LogInfo("=== Phase 2: Connect to IoT Hub with DPS-issued certificate ===");

        CONNECTION_STATUS_CONTEXT conn_ctx;
        memset(&conn_ctx, 0, sizeof(conn_ctx));

        IOTHUB_DEVICE_CLIENT_HANDLE hub_handle = IoTHubDeviceClient_CreateFromDeviceAuth(
            dps_ctx.iothub_uri, dps_ctx.device_id, MQTT_Protocol);
        ASSERT_IS_NOT_NULL(hub_handle, "Failed to create IoT Hub client from device auth");

        IoTHubDeviceClient_SetOption(hub_handle, OPTION_LOG_TRACE, &trace_on);
        IoTHubDeviceClient_SetConnectionStatusCallback(hub_handle, iothub_connection_status_callback, &conn_ctx);

        int csr_timeout = CSR_TIMEOUT_SECS;
        IoTHubDeviceClient_SetOption(hub_handle, OPTION_CSR_TIMEOUT_SECS, &csr_timeout);

        ASSERT_IS_TRUE(wait_for_connection(&conn_ctx, MAX_WAIT_TIME_SECS), "Phase 2: IoT Hub connection timed out");
        LogInfo("Phase 2 complete: Connected to IoT Hub");

        // Brief settle delay — let the convenience layer's worker thread
        // finish processing the CONNACK before we queue the CSR subscribe
        ThreadAPI_Sleep(2000);

        // ================================================================
        // Phase 3: IoT Hub CSR for certificate re-issuance
        // ================================================================
        LogInfo("=== Phase 3: IoT Hub CSR for certificate re-issuance ===");

        char* request_id = generate_request_id();

        CSR_CALLBACK_CONTEXT csr_ctx;
        memset(&csr_ctx, 0, sizeof(csr_ctx));

        GENERATED_CSR hub_csr = generate_ec_csr(dps_ctx.device_id);

        IOTHUB_CLIENT_RESULT hub_result = IoTHubDeviceClient_SendCertificateSigningRequestAsync(
            hub_handle, hub_csr.csr_base64, request_id, NULL,
            hub_csr_response_callback, &csr_ctx);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, hub_result, "Failed to send Hub CSR");

        ASSERT_IS_TRUE(wait_for_csr_response(&csr_ctx, MAX_WAIT_TIME_SECS), "Phase 3: Hub CSR response timed out");

        ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_CONFIRMATION_OK, (int)csr_ctx.result, "Hub CSR failed");
        ASSERT_ARE_EQUAL(int, 200, csr_ctx.response_status_code, "Hub CSR response status != 200");
        ASSERT_IS_NOT_NULL(csr_ctx.payload, "Hub CSR response payload is NULL");

        size_t cert_count = 0;
        char* hub_issued_cert_chain = csr_e2e_parse_hub_csr_response(csr_ctx.payload, &cert_count);
        ASSERT_IS_NOT_NULL(hub_issued_cert_chain, "Failed to parse Hub CSR response certificates");
        LogInfo("Phase 3 complete: Hub CSR returned %zu certificate(s)", cert_count);

        IoTHubDeviceClient_Destroy(hub_handle);
        hub_handle = NULL;

        // ================================================================
        // Phase 4: Reconnect with re-issued certificate
        // ================================================================
        LogInfo("=== Phase 4: Reconnect with re-issued certificate ===");

        // Inject the new certificate and private key into the HSM
        PROV_AUTH_HANDLE auth_handle = prov_auth_create();
        ASSERT_IS_NOT_NULL(auth_handle, "Failed to create auth handle for Phase 4");

        ASSERT_ARE_EQUAL(int, 0, prov_auth_set_certificate(auth_handle, hub_issued_cert_chain),
            "Failed to set re-issued certificate in HSM");
        ASSERT_ARE_EQUAL(int, 0, prov_auth_set_key(auth_handle, hub_csr.private_key_pem),
            "Failed to set Hub CSR private key in HSM");

        memset(&conn_ctx, 0, sizeof(conn_ctx));

        hub_handle = IoTHubDeviceClient_CreateFromDeviceAuth(
            dps_ctx.iothub_uri, dps_ctx.device_id, MQTT_Protocol);
        ASSERT_IS_NOT_NULL(hub_handle, "Failed to create IoT Hub client with re-issued cert");

        IoTHubDeviceClient_SetOption(hub_handle, OPTION_LOG_TRACE, &trace_on);
        IoTHubDeviceClient_SetConnectionStatusCallback(hub_handle, iothub_connection_status_callback, &conn_ctx);

        ASSERT_IS_TRUE(wait_for_connection(&conn_ctx, MAX_WAIT_TIME_SECS), "Phase 4: IoT Hub reconnection timed out");
        LogInfo("Phase 4 complete: Reconnected to IoT Hub with re-issued certificate");

        // ================================================================
        // Cleanup
        // ================================================================
        IoTHubDeviceClient_Destroy(hub_handle);

        prov_auth_destroy(auth_handle);
        free(hub_csr.csr_base64);
        free(hub_csr.private_key_pem);
        free(dps_csr.csr_base64);
        free(dps_csr.private_key_pem);
        free(hub_issued_cert_chain);
        free(csr_ctx.payload);
        free(request_id);
        free(dps_ctx.iothub_uri);
        free(dps_ctx.device_id);
    }

    // Full 4-phase CSR lifecycle test (symmetric key group enrollment, convenience layer):
    //   Phase 1: DPS registration with derived symmetric key + CSR
    //   Phase 2: Connect to IoT Hub with DPS-issued cert
    //   Phase 3: IoT Hub CSR for certificate re-issuance
    //   Phase 4: Reconnect with re-issued cert
    TEST_FUNCTION(dps_symkey_csr_full_lifecycle_mqtt)
    {
        SKIP_IF_FILTERED(dps_symkey_csr_full_lifecycle_mqtt);

        // Gate on ADR policy + symmetric key group env vars
        const char* adr_policy = getenv(ENV_ADR_CERT_MGMT_POLICY_NAME);
        if (adr_policy == NULL || adr_policy[0] == '\0')
        {
            LogInfo("SKIPPED: %s not set", ENV_ADR_CERT_MGMT_POLICY_NAME);
            return;
        }

        const char* group_primary_key = getenv(ENV_SYMM_KEY_GROUP_PRIMARY_KEY);
        if (group_primary_key == NULL || group_primary_key[0] == '\0')
        {
            LogInfo("SKIPPED: %s not set", ENV_SYMM_KEY_GROUP_PRIMARY_KEY);
            return;
        }

        // ================================================================
        // Phase 1: DPS Registration with symmetric key + CSR
        // ================================================================
        LogInfo("=== [SymKey] Phase 1: DPS Registration with CSR ===");

        // Generate a unique device/registration ID
        char reg_id[GUID_SIZE + 16];
        {
            char guid[GUID_SIZE];
            ASSERT_ARE_EQUAL(int, UNIQUEID_OK, (int)UniqueId_Generate(guid, GUID_SIZE));
            snprintf(reg_id, sizeof(reg_id), "csr-sk-%s", guid);
        }

        // Derive per-device key from group primary key
        char* derived_key = csr_e2e_derive_device_symmetric_key(group_primary_key, reg_id);
        ASSERT_IS_NOT_NULL(derived_key, "Failed to derive per-device key");
        LogInfo("Derived device key for '%s'", reg_id);

        // Switch HSM to symmetric key mode for this test
        // Must deinit both layers so cached security type is cleared
        prov_dev_security_deinit();
        iothub_security_deinit();
        prov_dev_security_init(SECURE_DEVICE_TYPE_SYMMETRIC_KEY);
        prov_dev_set_symmetric_key_info(reg_id, derived_key);

        DPS_CONV_REGISTRATION_CONTEXT dps_ctx;
        memset(&dps_ctx, 0, sizeof(dps_ctx));

        PROV_DEVICE_HANDLE prov_handle = Prov_Device_Create(g_dps_uri, g_dps_scope_id, Prov_Device_MQTT_Protocol);
        ASSERT_IS_NOT_NULL(prov_handle, "Failed to create DPS handle");

        bool trace_on = true;
        Prov_Device_SetOption(prov_handle, PROV_OPTION_LOG_TRACE, &trace_on);

        GENERATED_CSR dps_csr = generate_ec_csr(reg_id);

        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK,
            Prov_Device_SetOption(prov_handle, PROV_CERTIFICATE_SIGNING_REQUEST, dps_csr.csr_base64),
            "Failed to set DPS CSR");
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK,
            Prov_Device_SetOption(prov_handle, PROV_CERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY, dps_csr.private_key_pem),
            "Failed to set DPS CSR private key");

        PROV_DEVICE_RESULT reg_result = Prov_Device_Register_Device(prov_handle,
            dps_conv_register_device_callback, &dps_ctx,
            dps_registration_status_callback, NULL);
        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, reg_result, "Failed to start DPS registration");

        {
            time_t start = time(NULL);
            while (!dps_ctx.registration_complete)
            {
                ThreadAPI_Sleep(500);
                ASSERT_IS_TRUE(difftime(time(NULL), start) < MAX_WAIT_TIME_SECS, "DPS registration timed out");
            }
        }

        ASSERT_ARE_EQUAL(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_OK, dps_ctx.reg_result, "DPS registration failed");
        ASSERT_IS_NOT_NULL(dps_ctx.iothub_uri, "DPS did not return iothub_uri");
        ASSERT_IS_NOT_NULL(dps_ctx.device_id, "DPS did not return device_id");

        LogInfo("[SymKey] Phase 1 complete: Registered device '%s' on '%s'", dps_ctx.device_id, dps_ctx.iothub_uri);

        Prov_Device_Destroy(prov_handle);
        prov_handle = NULL;

        // After DPS registration with CSR, the X509 HSM singleton has the issued cert.
        // Save cert/key before deinit (which clears the X509 singleton),
        // then restore after switching HSM back to X509 mode.
        HSM_CLIENT_HANDLE x509_hsm = hsm_client_x509_create();
        char* saved_cert = hsm_client_x509_get_certificate(x509_hsm);
        char* saved_key = hsm_client_x509_get_key(x509_hsm);
        ASSERT_IS_NOT_NULL(saved_cert, "DPS did not store X509 certificate in HSM");
        ASSERT_IS_NOT_NULL(saved_key, "DPS did not store X509 key in HSM");

        prov_dev_security_deinit();
        iothub_security_deinit();
        prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

        // Restore the DPS-issued cert and key into the freshly initialized X509 HSM
        ASSERT_ARE_EQUAL(int, 0, hsm_client_x509_set_certificate(hsm_client_x509_create(), saved_cert));
        ASSERT_ARE_EQUAL(int, 0, hsm_client_x509_set_key(hsm_client_x509_create(), saved_key));
        free(saved_cert);
        free(saved_key);

        // ================================================================
        // Phase 2: Connect to IoT Hub with DPS-issued certificate
        // ================================================================
        LogInfo("=== [SymKey] Phase 2: Connect to IoT Hub with DPS-issued certificate ===");

        CONNECTION_STATUS_CONTEXT conn_ctx;
        memset(&conn_ctx, 0, sizeof(conn_ctx));

        IOTHUB_DEVICE_CLIENT_HANDLE hub_handle = IoTHubDeviceClient_CreateFromDeviceAuth(
            dps_ctx.iothub_uri, dps_ctx.device_id, MQTT_Protocol);
        ASSERT_IS_NOT_NULL(hub_handle, "Failed to create IoT Hub client from device auth");

        IoTHubDeviceClient_SetOption(hub_handle, OPTION_LOG_TRACE, &trace_on);
        IoTHubDeviceClient_SetConnectionStatusCallback(hub_handle, iothub_connection_status_callback, &conn_ctx);

        int csr_timeout = CSR_TIMEOUT_SECS;
        IoTHubDeviceClient_SetOption(hub_handle, OPTION_CSR_TIMEOUT_SECS, &csr_timeout);

        ASSERT_IS_TRUE(wait_for_connection(&conn_ctx, MAX_WAIT_TIME_SECS), "[SymKey] Phase 2: IoT Hub connection timed out");
        LogInfo("[SymKey] Phase 2 complete: Connected to IoT Hub");

        ThreadAPI_Sleep(2000);

        // ================================================================
        // Phase 3: IoT Hub CSR for certificate re-issuance
        // ================================================================
        LogInfo("=== [SymKey] Phase 3: IoT Hub CSR for certificate re-issuance ===");

        char* request_id = generate_request_id();

        CSR_CALLBACK_CONTEXT csr_ctx;
        memset(&csr_ctx, 0, sizeof(csr_ctx));

        GENERATED_CSR hub_csr = generate_ec_csr(dps_ctx.device_id);

        IOTHUB_CLIENT_RESULT hub_result = IoTHubDeviceClient_SendCertificateSigningRequestAsync(
            hub_handle, hub_csr.csr_base64, request_id, NULL,
            hub_csr_response_callback, &csr_ctx);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, hub_result, "Failed to send Hub CSR");

        ASSERT_IS_TRUE(wait_for_csr_response(&csr_ctx, MAX_WAIT_TIME_SECS), "[SymKey] Phase 3: Hub CSR response timed out");

        ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_CONFIRMATION_OK, (int)csr_ctx.result, "Hub CSR failed");
        ASSERT_ARE_EQUAL(int, 200, csr_ctx.response_status_code, "Hub CSR response status != 200");
        ASSERT_IS_NOT_NULL(csr_ctx.payload, "Hub CSR response payload is NULL");

        size_t cert_count = 0;
        char* hub_issued_cert_chain = csr_e2e_parse_hub_csr_response(csr_ctx.payload, &cert_count);
        ASSERT_IS_NOT_NULL(hub_issued_cert_chain, "Failed to parse Hub CSR response certificates");
        LogInfo("[SymKey] Phase 3 complete: Hub CSR returned %zu certificate(s)", cert_count);

        IoTHubDeviceClient_Destroy(hub_handle);
        hub_handle = NULL;

        // ================================================================
        // Phase 4: Reconnect with re-issued certificate
        // ================================================================
        LogInfo("=== [SymKey] Phase 4: Reconnect with re-issued certificate ===");

        PROV_AUTH_HANDLE auth_handle = prov_auth_create();
        ASSERT_IS_NOT_NULL(auth_handle, "Failed to create auth handle for Phase 4");

        ASSERT_ARE_EQUAL(int, 0, prov_auth_set_certificate(auth_handle, hub_issued_cert_chain),
            "Failed to set re-issued certificate in HSM");
        ASSERT_ARE_EQUAL(int, 0, prov_auth_set_key(auth_handle, hub_csr.private_key_pem),
            "Failed to set Hub CSR private key in HSM");

        memset(&conn_ctx, 0, sizeof(conn_ctx));

        hub_handle = IoTHubDeviceClient_CreateFromDeviceAuth(
            dps_ctx.iothub_uri, dps_ctx.device_id, MQTT_Protocol);
        ASSERT_IS_NOT_NULL(hub_handle, "Failed to create IoT Hub client with re-issued cert");

        IoTHubDeviceClient_SetOption(hub_handle, OPTION_LOG_TRACE, &trace_on);
        IoTHubDeviceClient_SetConnectionStatusCallback(hub_handle, iothub_connection_status_callback, &conn_ctx);

        ASSERT_IS_TRUE(wait_for_connection(&conn_ctx, MAX_WAIT_TIME_SECS), "[SymKey] Phase 4: IoT Hub reconnection timed out");
        LogInfo("[SymKey] Phase 4 complete: Reconnected to IoT Hub with re-issued certificate");

        // ================================================================
        // Cleanup
        // ================================================================
        IoTHubDeviceClient_Destroy(hub_handle);

        prov_auth_destroy(auth_handle);
        free(hub_csr.csr_base64);
        free(hub_csr.private_key_pem);
        free(dps_csr.csr_base64);
        free(dps_csr.private_key_pem);
        free(hub_issued_cert_chain);
        free(csr_ctx.payload);
        free(request_id);
        free(derived_key);
        free(dps_ctx.iothub_uri);
        free(dps_ctx.device_id);
    }

END_TEST_SUITE(prov_x509_csr_client_e2e)
