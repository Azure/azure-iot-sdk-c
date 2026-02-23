// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*
CAUTION: Checking of return codes and error values shall be omitted for brevity in this sample.
This sample is to demonstrate the certificate signing request (CSR) feature and is not a guide
for design principles or style. Please practice sound engineering practices when writing production code.

This sample demonstrates two CSR flows:

Phase 1 - DPS Provisioning with CSR (SAS-based):
    Generates an ECC P-256 key pair, creates a PKCS#10 CSR, and registers with the
    Azure Device Provisioning Service using symmetric key (SAS) authentication.
    DPS issues a signed certificate for the device.

Phase 2 - IoT Hub Certificate Renewal (X.509-based):
    Connects to IoT Hub using the DPS-issued certificate, generates a new key pair
    and CSR, sends it via the IoT Hub credentials MQTT topic, and verifies the
    renewed certificate by reconnecting.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/hmac.h>

#include "parson.h"

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"

#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "hsm_client_x509.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/strings.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

// DPS provisioning parameters (SAS-based symmetric key enrollment).
static const char* global_prov_uri = "global.azure-devices-provisioning.net";
static const char* id_scope = NULL;
static const char* registration_id = NULL;
static const char* symmetric_key = NULL;

#define MAX_PATH_LEN 512
#define CONNECT_TIMEOUT_MS 30000
#define TELEMETRY_SEND_TIMEOUT_MS 30000
#define CSR_RESPONSE_TIMEOUT_MS 60000

// ---------------------------------------------------------------------------
// DPS provisioning context
// ---------------------------------------------------------------------------

typedef struct DPS_CONTEXT_TAG
{
    bool registration_complete;
    char* iothub_uri;
    char* device_id;
} DPS_CONTEXT;

// ---------------------------------------------------------------------------
// IoT Hub CSR response context
// ---------------------------------------------------------------------------

typedef struct CSR_CALLBACK_CONTEXT_TAG
{
    bool responseReceived;
    int responseStatus;
    char* responsePayload;
} CSR_CALLBACK_CONTEXT;

static volatile bool g_connected = false;
static volatile size_t g_messages_confirmed = 0;

// ---------------------------------------------------------------------------
// File I/O helper
// ---------------------------------------------------------------------------

static int write_string_to_file(const char* filePath, const char* content)
{
    int result;
    FILE* fp = fopen(filePath, "w");

    if (fp == NULL)
    {
        LogError("Cannot open file %s for writing", filePath);
        result = 1;
    }
    else
    {
        size_t len = strlen(content);
        if (fwrite(content, 1, len, fp) != len)
        {
            LogError("Failed to write to file %s", filePath);
            result = 1;
        }
        else
        {
            result = 0;
        }
        fclose(fp);
    }
    return result;
}

// Saves a key/cert pair to disk. Files are named {dir}/{deviceId}{suffix}.key.pem
// and {dir}/{deviceId}{suffix}.cert.pem. Returns 0 on success, 1 on failure.
static int save_credentials(const char* dir, const char* deviceId, const char* suffix,
                            const char* keyPem, const char* certPem)
{
    char keyPath[MAX_PATH_LEN];
    char certPath[MAX_PATH_LEN];

    snprintf(keyPath, sizeof(keyPath), "%s/%s%s.key.pem", dir, deviceId, suffix);
    snprintf(certPath, sizeof(certPath), "%s/%s%s.cert.pem", dir, deviceId, suffix);

    if (write_string_to_file(keyPath, keyPem) != 0 ||
        write_string_to_file(certPath, certPem) != 0)
    {
        LogError("Failed to save credentials to disk");
        return 1;
    }

    (void)printf("Private key saved to:  %s\r\n", keyPath);
    (void)printf("Certificate saved to:  %s\r\n", certPath);
    return 0;
}

// ---------------------------------------------------------------------------
// Symmetric key derivation for DPS group enrollment
// ---------------------------------------------------------------------------

// Derives a device-specific symmetric key from a group enrollment key.
// derived_key = base64(HMAC-SHA256(base64_decode(group_key), registration_id))
// Returns a malloc'd base64 string or NULL on failure.
static char* derive_device_key(const char* groupKey, const char* regId)
{
    char* result = NULL;
    BUFFER_HANDLE decodedKey = Azure_Base64_Decode(groupKey);

    if (decodedKey == NULL)
    {
        LogError("Failed to base64-decode group enrollment key");
    }
    else
    {
        unsigned char hmacResult[EVP_MAX_MD_SIZE];
        unsigned int hmacLen = 0;

        if (HMAC(EVP_sha256(),
                 BUFFER_u_char(decodedKey), (int)BUFFER_length(decodedKey),
                 (const unsigned char*)regId, strlen(regId),
                 hmacResult, &hmacLen) == NULL)
        {
            LogError("HMAC-SHA256 failed");
        }
        else
        {
            STRING_HANDLE encoded = Azure_Base64_Encode_Bytes(hmacResult, hmacLen);
            if (encoded == NULL)
            {
                LogError("Failed to base64-encode derived key");
            }
            else
            {
                if (mallocAndStrcpy_s(&result, STRING_c_str(encoded)) != 0)
                {
                    result = NULL;
                }
                STRING_delete(encoded);
            }
        }
        BUFFER_delete(decodedKey);
    }
    return result;
}

// ---------------------------------------------------------------------------
// OpenSSL helpers
// ---------------------------------------------------------------------------

// Reads the contents of a memory BIO into a malloc'd null-terminated string.
// Returns a malloc'd string or NULL on failure.
static char* bio_to_string(BIO* bio)
{
    char* result = NULL;
    char* bioData = NULL;
    long bioLen = BIO_get_mem_data(bio, &bioData);

    if (bioLen > 0 && bioData != NULL)
    {
        result = (char*)calloc(1, (size_t)bioLen + 1);
        if (result != NULL)
        {
            memcpy(result, bioData, (size_t)bioLen);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// OpenSSL: Key pair and CSR generation
// ---------------------------------------------------------------------------

// Generates an ECC P-256 key pair. Returns a new EVP_PKEY* or NULL on failure.
static EVP_PKEY* generate_ecc_key_pair(void)
{
    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);

    if (ctx == NULL)
    {
        LogError("EVP_PKEY_CTX_new_id failed");
    }
    else if (EVP_PKEY_keygen_init(ctx) <= 0)
    {
        LogError("EVP_PKEY_keygen_init failed");
    }
    else if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) <= 0)
    {
        LogError("EVP_PKEY_CTX_set_ec_paramgen_curve_nid failed");
    }
    else if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    {
        LogError("EVP_PKEY_keygen failed");
        pkey = NULL;
    }

    if (ctx != NULL)
    {
        EVP_PKEY_CTX_free(ctx);
    }

    return pkey;
}

// Converts an EVP_PKEY to a PEM-encoded private key string (PKCS#8 format).
// Returns a malloc'd string or NULL on failure.
static char* pkey_to_pem_string(EVP_PKEY* pkey)
{
    char* result = NULL;
    BIO* bio = BIO_new(BIO_s_mem());

    if (bio == NULL)
    {
        LogError("BIO_new failed");
    }
    else
    {
        if (PEM_write_bio_PrivateKey(bio, pkey, NULL, NULL, 0, NULL, NULL) == 1)
        {
            result = bio_to_string(bio);
        }
        BIO_free(bio);
    }

    return result;
}

// Generates a PEM-encoded PKCS#10 CSR. Returns a malloc'd string or NULL on failure.
static char* generate_csr_pem(EVP_PKEY* pkey, const char* commonName)
{
    char* result = NULL;
    X509_REQ* req = X509_REQ_new();

    if (req == NULL)
    {
        LogError("X509_REQ_new failed");
    }
    else
    {
        bool ok = true;

        if (X509_REQ_set_version(req, 0) != 1)
        {
            LogError("X509_REQ_set_version failed");
            ok = false;
        }
        else if (X509_REQ_set_pubkey(req, pkey) != 1)
        {
            LogError("X509_REQ_set_pubkey failed");
            ok = false;
        }
        else
        {
            X509_NAME* name = X509_REQ_get_subject_name(req);
            if (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                (const unsigned char*)commonName, -1, -1, 0) != 1)
            {
                LogError("X509_NAME_add_entry_by_txt failed");
                ok = false;
            }
        }

        if (ok && X509_REQ_sign(req, pkey, EVP_sha256()) == 0)
        {
            LogError("X509_REQ_sign failed");
            ok = false;
        }

        if (ok)
        {
            BIO* csrBio = BIO_new(BIO_s_mem());
            if (csrBio == NULL)
            {
                LogError("BIO_new failed");
            }
            else
            {
                if (PEM_write_bio_X509_REQ(csrBio, req) == 1)
                {
                    result = bio_to_string(csrBio);
                }
                BIO_free(csrBio);
            }
        }

        X509_REQ_free(req);
    }

    return result;
}

// Strips PEM headers/footers and whitespace to produce raw base64
// (used for DPS CSR option which expects base64-encoded DER).
// Returns a malloc'd string or NULL on failure.
static char* pem_to_base64(const char* pem)
{
    char* result = NULL;
    size_t pemLen = strlen(pem);
    char* buf = (char*)malloc(pemLen + 1);

    if (buf == NULL)
    {
        LogError("Failed to allocate buffer");
    }
    else
    {
        size_t outIdx = 0;
        const char* p = pem;
        bool skipLine = false;

        while (*p != '\0')
        {
            if (*p == '-' && strncmp(p, "-----", 5) == 0)
            {
                skipLine = true;
            }

            if (skipLine)
            {
                if (*p == '\n')
                {
                    skipLine = false;
                }
            }
            else if (*p != '\n' && *p != '\r' && *p != ' ' && *p != '\t')
            {
                buf[outIdx++] = *p;
            }

            p++;
        }

        buf[outIdx] = '\0';
        result = buf;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Certificate response parsing (for IoT Hub CSR renewal)
// ---------------------------------------------------------------------------

// Converts a base64-encoded DER certificate to a PEM string.
// Returns a malloc'd string or NULL on failure.
static char* base64_der_cert_to_pem(const char* base64Der)
{
    char* result = NULL;
    BUFFER_HANDLE decodedBuffer = Azure_Base64_Decode(base64Der);

    if (decodedBuffer == NULL)
    {
        LogError("Azure_Base64_Decode failed");
    }
    else
    {
        const unsigned char* derData = BUFFER_u_char(decodedBuffer);
        size_t derLen = BUFFER_length(decodedBuffer);

        const unsigned char* p = derData;
        X509* cert = d2i_X509(NULL, &p, (long)derLen);

        if (cert == NULL)
        {
            LogError("d2i_X509 failed");
        }
        else
        {
            BIO* pemBio = BIO_new(BIO_s_mem());
            if (pemBio != NULL)
            {
                if (PEM_write_bio_X509(pemBio, cert) == 1)
                {
                    result = bio_to_string(pemBio);
                }
                BIO_free(pemBio);
            }
            X509_free(cert);
        }
        BUFFER_delete(decodedBuffer);
    }
    return result;
}

// Parses the IoT Hub CSR response JSON, extracts the certificates array,
// decodes each base64 DER cert to PEM, and returns the concatenated chain.
// Returns a malloc'd string or NULL on failure.
static char* parse_certificate_response(const char* responseJson)
{
    char* result = NULL;
    JSON_Value* rootValue = json_parse_string(responseJson);

    if (rootValue == NULL)
    {
        LogError("Failed to parse CSR response JSON");
    }
    else
    {
        JSON_Object* rootObj = json_value_get_object(rootValue);
        JSON_Array* certsArray = json_object_get_array(rootObj, "certificates");

        if (certsArray == NULL || json_array_get_count(certsArray) == 0)
        {
            LogError("CSR response missing or empty certificates array");
        }
        else
        {
            STRING_HANDLE pemChain = STRING_new();
            if (pemChain != NULL)
            {
                bool success = true;
                size_t certCount = json_array_get_count(certsArray);

                for (size_t i = 0; i < certCount; i++)
                {
                    const char* base64Cert = json_array_get_string(certsArray, i);
                    if (base64Cert == NULL)
                    {
                        LogError("Failed to get certificate at index %zu", i);
                        success = false;
                        break;
                    }

                    char* pemCert = base64_der_cert_to_pem(base64Cert);
                    if (pemCert == NULL)
                    {
                        LogError("Failed to convert certificate at index %zu", i);
                        success = false;
                        break;
                    }

                    if (STRING_concat(pemChain, pemCert) != 0)
                    {
                        free(pemCert);
                        success = false;
                        break;
                    }
                    free(pemCert);
                }

                if (success)
                {
                    if (mallocAndStrcpy_s(&result, STRING_c_str(pemChain)) != 0)
                    {
                        result = NULL;
                    }
                }
                STRING_delete(pemChain);
            }
        }
        json_value_free(rootValue);
    }
    return result;
}

// ---------------------------------------------------------------------------
// DPS callbacks
// ---------------------------------------------------------------------------

static void dps_registration_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* userContext)
{
    (void)userContext;
    (void)printf("DPS provisioning status: %s\r\n", MU_ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
}

static void dps_register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* userContext)
{
    DPS_CONTEXT* ctx = (DPS_CONTEXT*)userContext;
    ctx->registration_complete = true;

    if (register_result == PROV_DEVICE_RESULT_OK)
    {
        (void)printf("DPS registration succeeded. Hub: %s, Device: %s\r\n", iothub_uri, device_id);
        (void)mallocAndStrcpy_s(&ctx->iothub_uri, iothub_uri);
        (void)mallocAndStrcpy_s(&ctx->device_id, device_id);
    }
    else
    {
        (void)printf("DPS registration failed: %s\r\n", MU_ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result));
    }
}

// ---------------------------------------------------------------------------
// IoT Hub callbacks
// ---------------------------------------------------------------------------

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    (void)reason;
    (void)userContextCallback;

    if (status == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        g_connected = true;
        (void)printf("Device connected to IoT Hub.\r\n");
    }
    else
    {
        g_connected = false;
        (void)printf("Device disconnected (reason: %s).\r\n",
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
    }
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    g_messages_confirmed++;
    (void)printf("Telemetry confirmed #%zu (result: %s)\r\n",
        g_messages_confirmed, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void on_csr_response(int status, const char* responsePayload, void* userContextCallback)
{
    CSR_CALLBACK_CONTEXT* ctx = (CSR_CALLBACK_CONTEXT*)userContextCallback;

    (void)printf("CSR response received. Status: %d\r\n", status);

    ctx->responseStatus = status;
    ctx->responseReceived = true;

    if (status == 200 && responsePayload != NULL)
    {
        if (mallocAndStrcpy_s(&ctx->responsePayload, responsePayload) != 0)
        {
            LogError("Failed to copy CSR response payload");
        }
    }
    else if (responsePayload != NULL)
    {
        (void)printf("Error response: %s\r\n", responsePayload);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool wait_for_connection(int timeoutMs)
{
    int elapsed = 0;
    while (!g_connected && elapsed < timeoutMs)
    {
        ThreadAPI_Sleep(100);
        elapsed += 100;
    }
    return g_connected;
}

static int send_test_telemetry(IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle, const char* phase)
{
    int result = 0;
    size_t startCount = g_messages_confirmed;
    char msgBuffer[128];

    snprintf(msgBuffer, sizeof(msgBuffer), "{\"phase\":\"%s\"}", phase);

    IOTHUB_MESSAGE_HANDLE msg = IoTHubMessage_CreateFromString(msgBuffer);
    if (msg == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
        result = 1;
    }
    else
    {
        if (IoTHubDeviceClient_SendEventAsync(deviceHandle, msg, send_confirm_callback, NULL) != IOTHUB_CLIENT_OK)
        {
            LogError("IoTHubDeviceClient_SendEventAsync failed");
            result = 1;
        }
        else
        {
            (void)printf("Sent telemetry [%s]: %s\r\n", phase, msgBuffer);

            int elapsed = 0;
            while (g_messages_confirmed <= startCount && elapsed < TELEMETRY_SEND_TIMEOUT_MS)
            {
                ThreadAPI_Sleep(100);
                elapsed += 100;
            }

            if (g_messages_confirmed <= startCount)
            {
                LogError("Timed out waiting for telemetry confirmation");
                result = 1;
            }
        }
        IoTHubMessage_Destroy(msg);
    }
    return result;
}

static IOTHUB_DEVICE_CLIENT_HANDLE create_x509_client(const char* hubUri, const char* deviceId, const char* certPem, const char* keyPem)
{
    char connStr[MAX_PATH_LEN];
    snprintf(connStr, sizeof(connStr), "HostName=%s;DeviceId=%s;x509=true", hubUri, deviceId);

    IOTHUB_DEVICE_CLIENT_HANDLE handle = IoTHubDeviceClient_CreateFromConnectionString(connStr, MQTT_Protocol);

    if (handle != NULL)
    {
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        (void)IoTHubDeviceClient_SetOption(handle, OPTION_TRUSTED_CERT, certificates);
#endif
        bool traceOn = true;
        (void)IoTHubDeviceClient_SetOption(handle, OPTION_LOG_TRACE, &traceOn);

        bool urlEncodeOn = true;
        (void)IoTHubDeviceClient_SetOption(handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);

        (void)IoTHubDeviceClient_SetOption(handle, OPTION_X509_CERT, certPem);
        (void)IoTHubDeviceClient_SetOption(handle, OPTION_X509_PRIVATE_KEY, keyPem);

        (void)IoTHubDeviceClient_SetConnectionStatusCallback(handle, connection_status_callback, NULL);
    }
    return handle;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    int exitCode = 0;
    const char* outputDir = ".";
    EVP_PKEY* pkey = NULL;
    EVP_PKEY* renewPkey = NULL;
    char* privateKeyPem = NULL;
    char* csrPem = NULL;
    char* csrBase64 = NULL;
    char* dpsCertPem = NULL;
    char* renewKeyPem = NULL;
    char* renewCsrPem = NULL;
    char* renewedCertPem = NULL;
    char* derivedKey = NULL;
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle = NULL;
    DPS_CONTEXT dpsCtx;

    memset(&dpsCtx, 0, sizeof(dpsCtx));

    // Parse command line arguments.
    if (argc < 4)
    {
        (void)printf("Usage: %s <id_scope> <device_id> <enrollment_group_key> [output_dir] [dps_uri]\r\n", argv[0]);
        return 1;
    }

    id_scope = argv[1];
    registration_id = argv[2];

    if (argc > 4)
    {
        outputDir = argv[4];
    }

    if (argc > 5)
    {
        global_prov_uri = argv[5];
    }

    // Derive device-specific symmetric key from enrollment group key.
    derivedKey = derive_device_key(argv[3], registration_id);
    if (derivedKey == NULL)
    {
        LogError("Failed to derive device key");
        return 1;
    }
    symmetric_key = derivedKey;

    (void)printf("IoT Hub CSR Sample (DPS + Renewal)\r\n");
    (void)printf("==================================\r\n\r\n");
    (void)printf("DPS URI:          %s\r\n", global_prov_uri);
    (void)printf("ID Scope:         %s\r\n", id_scope);
    (void)printf("Device ID:        %s\r\n", registration_id);
    (void)printf("Output Dir:       %s\r\n\r\n", outputDir);

    // ======================================================================
    // Phase 1: DPS Provisioning with SAS + CSR
    // ======================================================================

    // Step 1: Generate ECC P-256 key pair and CSR.
    (void)printf("Generating ECC key pair and CSR for DPS provisioning...\r\n");

    pkey = generate_ecc_key_pair();
    if (pkey == NULL)
    {
        LogError("Failed to generate key pair");
        exitCode = 1;
        goto cleanup;
    }

    privateKeyPem = pkey_to_pem_string(pkey);
    csrPem = generate_csr_pem(pkey, registration_id);
    csrBase64 = (csrPem != NULL) ? pem_to_base64(csrPem) : NULL;

    if (privateKeyPem == NULL || csrPem == NULL || csrBase64 == NULL)
    {
        LogError("Failed to generate CSR or private key");
        exitCode = 1;
        goto cleanup;
    }

    (void)printf("CSR generated successfully.\r\n\r\n");

    // Step 2: Initialize DPS security with symmetric key.
    if (prov_dev_set_symmetric_key_info(registration_id, symmetric_key) != 0)
    {
        LogError("prov_dev_set_symmetric_key_info failed");
        exitCode = 1;
        goto cleanup;
    }

    if (prov_dev_security_init(SECURE_DEVICE_TYPE_SYMMETRIC_KEY) != 0)
    {
        LogError("prov_dev_security_init failed");
        exitCode = 1;
        goto cleanup;
    }

    if (IoTHub_Init() != 0)
    {
        LogError("IoTHub_Init failed");
        exitCode = 1;
        goto cleanup_security;
    }

    // Step 3: Create DPS client and set CSR options.
    {
        PROV_DEVICE_LL_HANDLE provHandle = Prov_Device_LL_Create(global_prov_uri, id_scope, Prov_Device_MQTT_Protocol);

        if (provHandle == NULL)
        {
            LogError("Prov_Device_LL_Create failed");
            exitCode = 1;
            goto cleanup_sdk;
        }

        bool traceOn = true;
        (void)Prov_Device_LL_SetOption(provHandle, PROV_OPTION_LOG_TRACE, &traceOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        (void)Prov_Device_LL_SetOption(provHandle, OPTION_TRUSTED_CERT, certificates);
#endif

        // Set the CSR (base64-encoded DER) and its corresponding private key (PEM).
        if (Prov_Device_LL_SetOption(provHandle, PROV_CERTIFICATE_SIGNING_REQUEST, csrBase64) != PROV_DEVICE_RESULT_OK)
        {
            LogError("Failed to set PROV_CERTIFICATE_SIGNING_REQUEST");
            Prov_Device_LL_Destroy(provHandle);
            exitCode = 1;
            goto cleanup_sdk;
        }

        if (Prov_Device_LL_SetOption(provHandle, PROV_CERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY, privateKeyPem) != PROV_DEVICE_RESULT_OK)
        {
            LogError("Failed to set PROV_CERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY");
            Prov_Device_LL_Destroy(provHandle);
            exitCode = 1;
            goto cleanup_sdk;
        }

        // Step 4: Register device with DPS.
        (void)printf("Registering device with DPS (SAS + CSR)...\r\n");

        if (Prov_Device_LL_Register_Device(provHandle, dps_register_device_callback, &dpsCtx, dps_registration_status_callback, &dpsCtx) != PROV_DEVICE_RESULT_OK)
        {
            LogError("Prov_Device_LL_Register_Device failed");
            Prov_Device_LL_Destroy(provHandle);
            exitCode = 1;
            goto cleanup_sdk;
        }

        while (!dpsCtx.registration_complete)
        {
            Prov_Device_LL_DoWork(provHandle);
            ThreadAPI_Sleep(10);
        }

        Prov_Device_LL_Destroy(provHandle);
    }

    if (dpsCtx.iothub_uri == NULL || dpsCtx.device_id == NULL)
    {
        LogError("DPS registration failed");
        exitCode = 1;
        goto cleanup_sdk;
    }

    // Step 5: Retrieve the DPS-issued certificate from the HSM singleton.
    {
        HSM_CLIENT_HANDLE hsmHandle = hsm_client_x509_create();
        dpsCertPem = hsm_client_x509_get_certificate(hsmHandle);
    }

    if (dpsCertPem == NULL)
    {
        LogError("Failed to retrieve DPS-issued certificate from HSM");
        exitCode = 1;
        goto cleanup_sdk;
    }

    (void)printf("\r\nDPS-issued certificate retrieved.\r\n");

    // Save initial credentials to disk.
    if (save_credentials(outputDir, dpsCtx.device_id, "", privateKeyPem, dpsCertPem) != 0)
    {
        exitCode = 1;
        goto cleanup_sdk;
    }

    // ======================================================================
    // Phase 2: IoT Hub connection with X.509 + Certificate Renewal via CSR
    // ======================================================================

    // Step 6: Connect to IoT Hub with the DPS-issued X.509 certificate.
    (void)printf("\r\nConnecting to IoT Hub with DPS-issued certificate...\r\n");

    deviceHandle = create_x509_client(dpsCtx.iothub_uri, dpsCtx.device_id, dpsCertPem, privateKeyPem);
    if (deviceHandle == NULL)
    {
        LogError("Failed to create IoT Hub client");
        exitCode = 1;
        goto cleanup_sdk;
    }

    if (!wait_for_connection(CONNECT_TIMEOUT_MS))
    {
        LogError("Timed out waiting for IoT Hub connection");
        exitCode = 1;
        goto cleanup_client;
    }

    // Step 7: Send telemetry to verify X.509 connectivity.
    if (send_test_telemetry(deviceHandle, "pre-renewal") != 0)
    {
        exitCode = 1;
        goto cleanup_client;
    }

    // Step 8: Generate new key pair and CSR for certificate renewal.
    // Configure the CSR timeout to match our application-level wait.
    {
        int csr_timeout_secs = CSR_RESPONSE_TIMEOUT_MS / 1000;
        (void)IoTHubDeviceClient_SetOption(deviceHandle, OPTION_CSR_TIMEOUT_SECS, &csr_timeout_secs);
    }

    (void)printf("\r\nGenerating new key pair and CSR for renewal...\r\n");

    renewPkey = generate_ecc_key_pair();
    if (renewPkey == NULL)
    {
        LogError("Failed to generate renewal key pair");
        exitCode = 1;
        goto cleanup_client;
    }

    renewKeyPem = pkey_to_pem_string(renewPkey);
    renewCsrPem = generate_csr_pem(renewPkey, dpsCtx.device_id);

    if (renewKeyPem == NULL || renewCsrPem == NULL)
    {
        LogError("Failed to generate renewal CSR");
        exitCode = 1;
        goto cleanup_client;
    }

    (void)printf("Renewal CSR generated. Sending to IoT Hub...\r\n");

    // Step 9: Send CSR to IoT Hub.
    {
        CSR_CALLBACK_CONTEXT csrCtx;
        memset(&csrCtx, 0, sizeof(csrCtx));

        if (IoTHubDeviceClient_SendCertificateSigningRequestAsync(
                deviceHandle, renewCsrPem, NULL, on_csr_response, &csrCtx) != IOTHUB_CLIENT_OK)
        {
            LogError("IoTHubDeviceClient_SendCertificateSigningRequestAsync failed");
            exitCode = 1;
            goto cleanup_client;
        }

        int elapsed = 0;
        while (!csrCtx.responseReceived && elapsed < CSR_RESPONSE_TIMEOUT_MS)
        {
            ThreadAPI_Sleep(100);
            elapsed += 100;
        }

        if (!csrCtx.responseReceived || csrCtx.responseStatus != 200 || csrCtx.responsePayload == NULL)
        {
            LogError("CSR request failed (received=%s, status=%d)",
                csrCtx.responseReceived ? "true" : "false", csrCtx.responseStatus);
            free(csrCtx.responsePayload);
            exitCode = 1;
            goto cleanup_client;
        }

        // Step 10: Parse the certificate response.
        renewedCertPem = parse_certificate_response(csrCtx.responsePayload);
        free(csrCtx.responsePayload);

        if (renewedCertPem == NULL)
        {
            LogError("Failed to parse certificate response");
            exitCode = 1;
            goto cleanup_client;
        }
    }

    // Save renewed credentials to disk.
    if (save_credentials(outputDir, dpsCtx.device_id, "_renewed", renewKeyPem, renewedCertPem) != 0)
    {
        exitCode = 1;
        goto cleanup_client;
    }

    // Step 11: Reconnect with renewed certificate to verify it works.
    (void)printf("\r\nReconnecting with renewed certificate...\r\n");
    IoTHubDeviceClient_Destroy(deviceHandle);
    deviceHandle = NULL;
    g_connected = false;

    deviceHandle = create_x509_client(dpsCtx.iothub_uri, dpsCtx.device_id, renewedCertPem, renewKeyPem);
    if (deviceHandle == NULL)
    {
        LogError("Failed to create IoT Hub client with renewed cert");
        exitCode = 1;
        goto cleanup_sdk;
    }

    if (!wait_for_connection(CONNECT_TIMEOUT_MS))
    {
        LogError("Timed out waiting for connection with renewed cert");
        exitCode = 1;
        goto cleanup_client;
    }

    // Step 12: Send telemetry to verify renewed certificate works.
    if (send_test_telemetry(deviceHandle, "post-renewal") != 0)
    {
        exitCode = 1;
        goto cleanup_client;
    }

    (void)printf("\r\n==================================\r\n");
    (void)printf("CSR sample completed successfully!\r\n");

cleanup_client:
    if (deviceHandle != NULL)
    {
        IoTHubDeviceClient_Destroy(deviceHandle);
    }

cleanup_sdk:
    IoTHub_Deinit();

cleanup_security:
    prov_dev_security_deinit();

cleanup:
    if (pkey != NULL) EVP_PKEY_free(pkey);
    if (renewPkey != NULL) EVP_PKEY_free(renewPkey);
    free(privateKeyPem);
    free(csrPem);
    free(csrBase64);
    free(dpsCertPem);
    free(renewKeyPem);
    free(renewCsrPem);
    free(renewedCertPem);
    free(derivedKey);
    free(dpsCtx.iothub_uri);
    free(dpsCtx.device_id);

    return exitCode;
}
