// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*
CAUTION: Checking of return codes and error values shall be omitted for brevity in this sample.
This sample is to demonstrate the certificate signing request (CSR) feature and is not a guide
for design principles or style. Please practice sound engineering practices when writing production code.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/bio.h>

#include "parson.h"

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"

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

/* Paste in your device connection string (SAS-based) */
static const char* connectionString = "[connectionString]";

#define MAX_PATH_LEN 512
#define CONNECT_TIMEOUT_MS 30000
#define TELEMETRY_SEND_TIMEOUT_MS 30000
#define CSR_RESPONSE_TIMEOUT_MS 60000

// Context passed to the CSR response callback.
typedef struct CSR_CALLBACK_CONTEXT_TAG
{
    bool responseReceived;
    int responseStatus;
    char* responsePayload;
} CSR_CALLBACK_CONTEXT;

static volatile bool g_connected = false;
static volatile size_t g_messages_confirmed = 0;

// ---------------------------------------------------------------------------
// Connection string parsing helper
// ---------------------------------------------------------------------------

// Extracts a value for a given key from an IoT Hub connection string.
// Connection string format: "key1=value1;key2=value2;..."
// Returns a malloc'd string or NULL if the key is not found.
static char* connection_string_get_value(const char* connStr, const char* key)
{
    char* result = NULL;
    size_t keyLen = strlen(key);
    const char* p = connStr;

    while (p != NULL && *p != '\0')
    {
        if (strncmp(p, key, keyLen) == 0 && p[keyLen] == '=')
        {
            const char* valueStart = p + keyLen + 1;
            const char* valueEnd = strchr(valueStart, ';');
            size_t valueLen = (valueEnd != NULL) ? (size_t)(valueEnd - valueStart) : strlen(valueStart);

            result = (char*)malloc(valueLen + 1);
            if (result != NULL)
            {
                memcpy(result, valueStart, valueLen);
                result[valueLen] = '\0';
            }
            break;
        }

        p = strchr(p, ';');
        if (p != NULL)
        {
            p++;
        }
    }

    return result;
}

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

// Converts an EVP_PKEY to a PEM-encoded private key string.
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
        }
        BIO_free(bio);
    }

    return result;
}

// Generates a PEM-encoded PKCS#10 CSR. Returns a malloc'd string or NULL on failure.
static char* generate_csr(EVP_PKEY* pkey, const char* commonName)
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
                    char* bioData = NULL;
                    long bioLen = BIO_get_mem_data(csrBio, &bioData);

                    if (bioLen > 0 && bioData != NULL)
                    {
                        result = (char*)calloc(1, (size_t)bioLen + 1);
                        if (result != NULL)
                        {
                            memcpy(result, bioData, (size_t)bioLen);
                        }
                    }
                }
                BIO_free(csrBio);
            }
        }

        X509_REQ_free(req);
    }

    return result;
}

// ---------------------------------------------------------------------------
// Certificate response parsing
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
                    char* bioData = NULL;
                    long bioLen = BIO_get_mem_data(pemBio, &bioData);

                    if (bioLen > 0 && bioData != NULL)
                    {
                        result = (char*)calloc(1, (size_t)bioLen + 1);
                        if (result != NULL)
                        {
                            memcpy(result, bioData, (size_t)bioLen);
                        }
                    }
                }
                BIO_free(pemBio);
            }
            X509_free(cert);
        }
        BUFFER_delete(decodedBuffer);
    }
    return result;
}

// Parses the CSR response JSON, extracts the certificates array,
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

static IOTHUB_DEVICE_CLIENT_HANDLE create_client(const char* connStr)
{
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
    char* hostName = NULL;
    char* deviceId = NULL;
    char* privateKeyPem = NULL;
    char* csrPem = NULL;
    char* certChainPem = NULL;
    EVP_PKEY* pkey = NULL;
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle = NULL;

    // Allow overriding output directory from command line.
    if (argc > 1)
    {
        outputDir = argv[1];
    }

    (void)printf("IoT Hub CSR Sample\r\n");
    (void)printf("==================\r\n\r\n");

    // Step 1: Extract HostName and DeviceId from connection string.
    hostName = connection_string_get_value(connectionString, "HostName");
    deviceId = connection_string_get_value(connectionString, "DeviceId");

    if (hostName == NULL || deviceId == NULL)
    {
        LogError("Failed to parse HostName or DeviceId from connection string");
        exitCode = 1;
        goto cleanup;
    }

    (void)printf("Host:     %s\r\n", hostName);
    (void)printf("Device:   %s\r\n", deviceId);
    (void)printf("Output:   %s\r\n\r\n", outputDir);

    // Initialize SDK.
    if (IoTHub_Init() != 0)
    {
        LogError("IoTHub_Init failed");
        exitCode = 1;
        goto cleanup;
    }

    // Step 2: Connect to IoT Hub with SAS authentication.
    (void)printf("Connecting with SAS authentication...\r\n");
    deviceHandle = create_client(connectionString);
    if (deviceHandle == NULL)
    {
        LogError("Failed to create IoT Hub client. Check your connection string.");
        exitCode = 1;
        goto cleanup_sdk;
    }

    if (!wait_for_connection(CONNECT_TIMEOUT_MS))
    {
        LogError("Timed out waiting for connection");
        exitCode = 1;
        goto cleanup_client;
    }

    // Step 3: Send telemetry to verify SAS connectivity.
    if (send_test_telemetry(deviceHandle, "pre-csr") != 0)
    {
        exitCode = 1;
        goto cleanup_client;
    }

    // Step 4: Generate ECC key pair and CSR.
    (void)printf("\r\nGenerating ECC key pair and CSR...\r\n");

    pkey = generate_ecc_key_pair();
    if (pkey == NULL)
    {
        LogError("Failed to generate key pair");
        exitCode = 1;
        goto cleanup_client;
    }

    csrPem = generate_csr(pkey, deviceId);
    if (csrPem == NULL)
    {
        LogError("Failed to generate CSR");
        exitCode = 1;
        goto cleanup_client;
    }
    (void)printf("CSR generated successfully.\r\n");

    // Step 5: Send CSR to IoT Hub.
    {
        CSR_CALLBACK_CONTEXT csrCtx;
        memset(&csrCtx, 0, sizeof(csrCtx));

        (void)printf("Sending CSR to IoT Hub...\r\n");

        if (IoTHubDeviceClient_SendCertificateSigningRequestAsync(
                deviceHandle, csrPem, NULL, on_csr_response, &csrCtx) != IOTHUB_CLIENT_OK)
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

        // Step 6: Parse the certificate response.
        certChainPem = parse_certificate_response(csrCtx.responsePayload);
        free(csrCtx.responsePayload);

        if (certChainPem == NULL)
        {
            LogError("Failed to parse certificate response");
            exitCode = 1;
            goto cleanup_client;
        }
    }

    // Save generated private key and certificate to files.
    privateKeyPem = pkey_to_pem_string(pkey);
    if (privateKeyPem == NULL)
    {
        LogError("Failed to convert private key to PEM");
        exitCode = 1;
        goto cleanup_client;
    }

    {
        char keyPath[MAX_PATH_LEN];
        char certPath[MAX_PATH_LEN];

        snprintf(keyPath, sizeof(keyPath), "%s/%s.key.pem", outputDir, deviceId);
        snprintf(certPath, sizeof(certPath), "%s/%s.cert.pem", outputDir, deviceId);

        if (write_string_to_file(keyPath, privateKeyPem) != 0 ||
            write_string_to_file(certPath, certChainPem) != 0)
        {
            LogError("Failed to save credentials to disk");
            exitCode = 1;
            goto cleanup_client;
        }

        (void)printf("Private key saved to:  %s\r\n", keyPath);
        (void)printf("Certificate saved to:  %s\r\n", certPath);
    }

    // Step 7: Reconnect with X.509 certificate to verify it works.
    (void)printf("\r\nReconnecting with X.509 certificate...\r\n");
    IoTHubDeviceClient_Destroy(deviceHandle);
    deviceHandle = NULL;
    g_connected = false;

    {
        char x509ConnStr[MAX_PATH_LEN];
        snprintf(x509ConnStr, sizeof(x509ConnStr), "HostName=%s;DeviceId=%s;x509=true", hostName, deviceId);

        deviceHandle = create_client(x509ConnStr);
        if (deviceHandle == NULL)
        {
            LogError("Failed to create X.509 client");
            exitCode = 1;
            goto cleanup_sdk;
        }

        if (IoTHubDeviceClient_SetOption(deviceHandle, OPTION_X509_CERT, certChainPem) != IOTHUB_CLIENT_OK ||
            IoTHubDeviceClient_SetOption(deviceHandle, OPTION_X509_PRIVATE_KEY, privateKeyPem) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed to set X.509 options");
            exitCode = 1;
            goto cleanup_client;
        }

        if (!wait_for_connection(CONNECT_TIMEOUT_MS))
        {
            LogError("Timed out waiting for X.509 connection");
            exitCode = 1;
            goto cleanup_client;
        }

        // Step 8: Send telemetry to verify X.509 connectivity.
        if (send_test_telemetry(deviceHandle, "post-csr") != 0)
        {
            exitCode = 1;
            goto cleanup_client;
        }
    }

    (void)printf("\r\n==================\r\n");
    (void)printf("CSR sample completed successfully!\r\n");

cleanup_client:
    if (deviceHandle != NULL)
    {
        IoTHubDeviceClient_Destroy(deviceHandle);
    }

cleanup_sdk:
    IoTHub_Deinit();

cleanup:
    if (pkey != NULL) EVP_PKEY_free(pkey);
    free(hostName);
    free(deviceId);
    free(privateKeyPem);
    free(csrPem);
    free(certChainPem);

    return exitCode;
}
