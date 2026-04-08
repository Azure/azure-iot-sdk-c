// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CSR (Certificate Signing Request) E2E test for the Azure IoT C SDK.
// Tests CSR scenarios using the convenience layer API:
//   - DPS CSR-only registration (X509 individual enrollment)
//   - Full 4-phase lifecycle: DPS CSR → Hub connect → Hub CSR → Reconnect
//   - Symmetric key group enrollment with CSR → full lifecycle

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include "testrunnerswitcher.h"

#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/hmacsha256.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/shared_util_options.h"

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

#include "parson.h"

#include "common_prov_e2e.h"

#include "hsm_client_x509.h"

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

// ----- Certificate helpers -----

// Convert base64 DER to PEM certificate
static char* base64_to_pem_certificate(const char* base64)
{
    size_t b64len = strlen(base64);
    size_t num_lines = (b64len + 63) / 64;
    size_t pem_size = 28 + b64len + num_lines + 26 + 1; // header + data + newlines + footer + null
    char* pem = (char*)calloc(1, pem_size);

    if (pem != NULL)
    {
        char* p = pem;
        p += sprintf(p, "-----BEGIN CERTIFICATE-----\n");

        for (size_t i = 0; i < b64len; i += 64)
        {
            size_t chunk = (b64len - i < 64) ? b64len - i : 64;
            memcpy(p, base64 + i, chunk);
            p += chunk;
            *p++ = '\n';
        }

        (void)sprintf(p, "-----END CERTIFICATE-----\n");
    }

    return pem;
}

// Parse IoT Hub CSR response JSON: {"certificates": ["b64cert1", ...]}
// Returns concatenated PEM chain and sets cert_count.
static char* parse_hub_csr_response(const char* responseJson, size_t* cert_count)
{
    char* result = NULL;
    *cert_count = 0;

    JSON_Value* root_value = json_parse_string(responseJson);
    ASSERT_IS_NOT_NULL(root_value, "Failed to parse CSR response JSON");

    JSON_Object* root_obj = json_value_get_object(root_value);
    ASSERT_IS_NOT_NULL(root_obj, "CSR response is not a JSON object");

    JSON_Array* certs_array = json_object_get_array(root_obj, "certificates");
    ASSERT_IS_NOT_NULL(certs_array, "CSR response missing 'certificates' array");

    size_t count = json_array_get_count(certs_array);
    ASSERT_IS_TRUE(count > 0, "CSR response certificates array is empty");

    size_t total_size = 0;
    bool success = true;
    char** pem_certs = (char**)calloc(count, sizeof(char*));
    ASSERT_IS_NOT_NULL(pem_certs, "Failed to allocate pem_certs array");

    for (size_t i = 0; i < count && success; i++)
    {
        const char* base64_cert = json_array_get_string(certs_array, i);
        if (base64_cert == NULL || (pem_certs[i] = base64_to_pem_certificate(base64_cert)) == NULL)
        {
            success = false;
        }
        else
        {
            total_size += strlen(pem_certs[i]);
        }
    }

    ASSERT_IS_TRUE(success, "Failed to convert one or more certificates to PEM");

    result = (char*)calloc(1, total_size + 1);
    ASSERT_IS_NOT_NULL(result, "Failed to allocate certificate chain buffer");

    char* p = result;
    for (size_t i = 0; i < count; i++)
    {
        size_t len = strlen(pem_certs[i]);
        memcpy(p, pem_certs[i], len);
        p += len;
        free(pem_certs[i]);
    }
    free(pem_certs);

    *cert_count = count;

    json_value_free(root_value);
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

// ----- OpenSSL CSR generation -----

// Context holding a generated CSR (base64 DER) and its private key (PKCS#8 PEM).
typedef struct GENERATED_CSR_TAG
{
    char* csr_base64;       // base64(DER(CSR)), no PEM headers
    char* private_key_pem;  // PKCS#8 PEM string
} GENERATED_CSR;

// Generate a P-256 EC key pair, create a CSR signed with it, and return
// the base64-encoded DER CSR plus the PKCS#8 PEM private key.
static GENERATED_CSR generate_ec_csr(const char* common_name)
{
    GENERATED_CSR result;
    memset(&result, 0, sizeof(result));

    // Generate EC P-256 key
    EVP_PKEY* pkey = EVP_EC_gen("P-256");
    ASSERT_IS_NOT_NULL(pkey, "Failed to generate EC P-256 key");

    // Create CSR
    X509_REQ* req = X509_REQ_new();
    ASSERT_IS_NOT_NULL(req, "Failed to create X509_REQ");

    X509_REQ_set_version(req, 0);

    X509_NAME* name = X509_REQ_get_subject_name(req);
    ASSERT_IS_NOT_NULL(name, "Failed to get CSR subject name");
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)common_name, -1, -1, 0);

    X509_REQ_set_pubkey(req, pkey);
    ASSERT_IS_TRUE(X509_REQ_sign(req, pkey, EVP_sha256()) > 0, "Failed to sign CSR");

    // Encode CSR to DER -> base64
    {
        unsigned char* der_buf = NULL;
        int der_len = i2d_X509_REQ(req, &der_buf);
        ASSERT_IS_TRUE(der_len > 0, "Failed to encode CSR to DER");

        BUFFER_HANDLE der_buffer = BUFFER_create(der_buf, (size_t)der_len);
        ASSERT_IS_NOT_NULL(der_buffer, "Failed to create BUFFER from DER");

        STRING_HANDLE b64 = Azure_Base64_Encode(der_buffer);
        ASSERT_IS_NOT_NULL(b64, "Failed to base64 encode CSR DER");

        ASSERT_ARE_EQUAL(int, 0, mallocAndStrcpy_s(&result.csr_base64, STRING_c_str(b64)),
            "Failed to copy base64 CSR");

        STRING_delete(b64);
        BUFFER_delete(der_buffer);
        OPENSSL_free(der_buf);
    }

    // Encode private key to PKCS#8 PEM string
    {
        BIO* bio = BIO_new(BIO_s_mem());
        ASSERT_IS_NOT_NULL(bio, "Failed to create BIO");

        ASSERT_IS_TRUE(PEM_write_bio_PrivateKey(bio, pkey, NULL, NULL, 0, NULL, NULL) == 1,
            "Failed to write private key PEM");

        char* pem_data = NULL;
        long pem_len = BIO_get_mem_data(bio, &pem_data);
        ASSERT_IS_TRUE(pem_len > 0, "PEM data is empty");

        result.private_key_pem = (char*)malloc((size_t)pem_len + 1);
        ASSERT_IS_NOT_NULL(result.private_key_pem, "Failed to allocate private key PEM");
        memcpy(result.private_key_pem, pem_data, (size_t)pem_len);
        result.private_key_pem[pem_len] = '\0';

        BIO_free(bio);
    }

    X509_REQ_free(req);
    EVP_PKEY_free(pkey);

    return result;
}

// Derive per-device symmetric key from group key via HMAC-SHA256.
// derived_key = Base64Encode(HMAC-SHA256(Base64Decode(group_key), registration_id))
static char* derive_device_key(const char* group_key, const char* registration_id)
{
    BUFFER_HANDLE decoded_key = Azure_Base64_Decode(group_key);
    ASSERT_IS_NOT_NULL(decoded_key, "Failed to base64-decode group key");

    BUFFER_HANDLE hash = BUFFER_new();
    ASSERT_IS_NOT_NULL(hash, "Failed to allocate HMAC hash buffer");

    ASSERT_ARE_EQUAL(int, HMACSHA256_OK,
        (int)HMACSHA256_ComputeHash(
            BUFFER_u_char(decoded_key), BUFFER_length(decoded_key),
            (const unsigned char*)registration_id, strlen(registration_id),
            hash),
        "Failed to compute HMAC-SHA256 for device key derivation");

    STRING_HANDLE derived_b64 = Azure_Base64_Encode(hash);
    ASSERT_IS_NOT_NULL(derived_b64, "Failed to base64-encode derived key");

    char* result = NULL;
    ASSERT_ARE_EQUAL(int, 0, mallocAndStrcpy_s(&result, STRING_c_str(derived_b64)),
        "Failed to copy derived key");

    STRING_delete(derived_b64);
    BUFFER_delete(hash);
    BUFFER_delete(decoded_key);

    return result;
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
        char* hub_issued_cert_chain = parse_hub_csr_response(csr_ctx.payload, &cert_count);
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
        char* derived_key = derive_device_key(group_primary_key, reg_id);
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
        char* hub_issued_cert_chain = parse_hub_csr_response(csr_ctx.payload, &cert_count);
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
