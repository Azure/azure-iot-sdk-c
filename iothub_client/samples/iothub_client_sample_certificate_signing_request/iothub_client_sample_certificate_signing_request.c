// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.
//
// This sample demonstrates certificate renewal via Certificate Signing Request (CSR) on IoT Hub.
// The device connects with an existing X.509 certificate, sends a CSR to obtain a renewed certificate,
// then reconnects using the renewed certificate to verify it works.
//
// For initial device provisioning with CSR via DPS, see the prov_dev_client_ll_x509_csr_sample.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "parson.h"

// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
    #include "iothubtransportmqtt.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    #include "iothubtransportmqtt_websockets.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    #include "iothubtransportamqp.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    #include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    #include "iothubtransporthttp.h"
#endif // SAMPLE_HTTP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    #include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/* Paste in your x509 iothub connection string  */
/*  "HostName=<host_name>;DeviceId=<device_id>;x509=true"  */
static const char* connectionString = "HostName=<host_name>;DeviceId=<device_id>;x509=true";

static const char* x509certificate =
"-----BEGIN CERTIFICATE-----""\n"
"MIICpDCCAYwCCQCfIjBnPxs5TzANBgkqhkiG9w0BAQsFADAUMRIwEAYDVQQDDAls""\n"
"...""\n"
"-----END CERTIFICATE-----";

static const char* x509privatekey =
"-----BEGIN PRIVATE KEY-----""\n"
"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg6s92gNq7JlPpI9Cu""\n"
"...""\n"
"-----END PRIVATE KEY-----";

// Certificate Signing Request for renewal (raw base64, without PEM headers/footers).
// See readme.md for generation instructions.
static const char* certificateSigningRequest =
"MIHoMIGPAgEAMBMxETAPBgNVBAMMCGRldmljZUlkMFkwEwYHKoZIzj0CAQYIKoZI"
"...";

// Private key corresponding to the CSR above (PKCS#8 PEM format).
static const char* csrPrivateKey =
"-----BEGIN PRIVATE KEY-----""\n"
"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg6s92gNq7JlPpI9Cu""\n"
"...""\n"
"-----END PRIVATE KEY-----";

#define CSR_TIMEOUT_SECS            60

static volatile bool g_connected = false;
static volatile size_t g_message_count_send_confirmations = 0;

typedef struct CSR_CALLBACK_CONTEXT_TAG
{
    bool response_received;
    IOTHUB_CLIENT_CONFIRMATION_RESULT result;
    int response_status_code;
    char* payload;
} CSR_CALLBACK_CONTEXT;

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    (void)user_context;
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        g_connected = true;
        (void)printf("The device client is connected to iothub\r\n");
    }
    else
    {
        g_connected = false;
        (void)printf("The device client has been disconnected\r\n");
    }
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %zu with result %s\r\n",
        g_message_count_send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void csr_response_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, int response_status_code, const char* responsePayload, void* userContextCallback)
{
    CSR_CALLBACK_CONTEXT* ctx = (CSR_CALLBACK_CONTEXT*)userContextCallback;

    (void)printf("CSR response received (result: %s, status: %d)\r\n",
        MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result), response_status_code);

    if (result == IOTHUB_CLIENT_CONFIRMATION_ACCEPTED)
    {
        (void)printf("CSR request accepted (202), waiting for final response...\r\n");
        if (responsePayload != NULL)
        {
            (void)printf("Intermediate payload: %s\r\n", responsePayload);
        }
        return;
    }

    ctx->result = result;
    ctx->response_status_code = response_status_code;
    ctx->response_received = true;

    if (result == IOTHUB_CLIENT_CONFIRMATION_OK && response_status_code == 200 && responsePayload != NULL)
    {
        (void)mallocAndStrcpy_s(&ctx->payload, responsePayload);
    }
    else if (responsePayload != NULL)
    {
        (void)printf("Error response: %s\r\n", responsePayload);
    }
}

// Converts a base64 string to PEM certificate format by adding header/footer and 64-char line breaks.
static char* base64_to_pem_certificate(const char* base64)
{
    size_t b64len = strlen(base64);
    size_t num_lines = (b64len + 63) / 64;
    size_t pem_size = 28 + b64len + num_lines + 26 + 1;
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

// Parses the IoT Hub CSR response JSON and extracts the certificate chain as PEM.
// Expected format: {"certificates": ["base64cert1", "base64cert2", ...]}
static char* parse_certificate_response(const char* responseJson)
{
    char* result = NULL;
    JSON_Value* root_value = json_parse_string(responseJson);

    if (root_value == NULL)
    {
        (void)printf("Failed to parse CSR response JSON\r\n");
    }
    else
    {
        JSON_Object* root_obj = json_value_get_object(root_value);
        JSON_Array* certs_array = json_object_get_array(root_obj, "certificates");

        if (certs_array == NULL || json_array_get_count(certs_array) == 0)
        {
            (void)printf("CSR response missing or empty certificates array\r\n");
        }
        else
        {
            size_t cert_count = json_array_get_count(certs_array);
            size_t total_size = 0;
            bool success = true;

            char** pem_certs = (char**)calloc(cert_count, sizeof(char*));
            if (pem_certs != NULL)
            {
                for (size_t i = 0; i < cert_count && success; i++)
                {
                    const char* base64_cert = json_array_get_string(certs_array, i);
                    if (base64_cert == NULL ||
                        (pem_certs[i] = base64_to_pem_certificate(base64_cert)) == NULL)
                    {
                        success = false;
                    }
                    else
                    {
                        total_size += strlen(pem_certs[i]);
                    }
                }

                if (success && (result = (char*)calloc(1, total_size + 1)) != NULL)
                {
                    char* p = result;
                    for (size_t i = 0; i < cert_count; i++)
                    {
                        size_t len = strlen(pem_certs[i]);
                        memcpy(p, pem_certs[i], len);
                        p += len;
                    }
                }

                for (size_t i = 0; i < cert_count; i++)
                {
                    free(pem_certs[i]);
                }
                free(pem_certs);
            }
        }

        json_value_free(root_value);
    }

    return result;
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
    protocol = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    protocol = HTTP_Protocol;
#endif // SAMPLE_HTTP

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    (void)printf("Creating IoTHub handle\r\n");
    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (device_ll_handle == NULL)
    {
        (void)printf("Failure creating IotHub device. Hint: Check your connection string.\r\n");
    }
    else
    {
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_OVER_WEBSOCKETS
        bool urlEncodeOn = true;
        (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif

        // Set the X509 certificates in the SDK
        if (
            (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_X509_CERT, x509certificate) != IOTHUB_CLIENT_OK) ||
            (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_X509_PRIVATE_KEY, x509privatekey) != IOTHUB_CLIENT_OK)
            )
        {
            (void)printf("failure to set options for x509, aborting\r\n");
        }
        else
        {
            int csr_timeout = CSR_TIMEOUT_SECS;
            (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_CSR_TIMEOUT_SECS, &csr_timeout);
            (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL);

            // Wait for connection
            (void)printf("Waiting for connection...\r\n");
            while (!g_connected)
            {
                IoTHubDeviceClient_LL_DoWork(device_ll_handle);
                ThreadAPI_Sleep(1);
            }

            // Send certificate signing request
            (void)printf("Sending certificate signing request...\r\n");

            CSR_CALLBACK_CONTEXT csr_ctx;
            memset(&csr_ctx, 0, sizeof(csr_ctx));

            if (IoTHubDeviceClient_LL_SendCertificateSigningRequestAsync(
                    device_ll_handle, certificateSigningRequest, NULL,
                    csr_response_callback, &csr_ctx) != IOTHUB_CLIENT_OK)
            {
                (void)printf("Failed to send CSR\r\n");
            }
            else
            {
                // Wait for CSR response
                while (!csr_ctx.response_received)
                {
                    IoTHubDeviceClient_LL_DoWork(device_ll_handle);
                    ThreadAPI_Sleep(1);
                }

                if (csr_ctx.result == IOTHUB_CLIENT_CONFIRMATION_OK && csr_ctx.response_status_code == 200 && csr_ctx.payload != NULL)
                {
                    char* renewed_cert = parse_certificate_response(csr_ctx.payload);

                    if (renewed_cert != NULL)
                    {
                        (void)printf("Renewed certificate received. Reconnecting...\r\n");

                        // Disconnect and reconnect with the renewed certificate
                        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
                        device_ll_handle = NULL;
                        g_connected = false;

                        device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
                        if (device_ll_handle != NULL)
                        {
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
                            IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif
#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_OVER_WEBSOCKETS
                            (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif
                            (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_X509_CERT, renewed_cert);
                            (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_X509_PRIVATE_KEY, csrPrivateKey);
                            (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL);

                            while (!g_connected)
                            {
                                IoTHubDeviceClient_LL_DoWork(device_ll_handle);
                                ThreadAPI_Sleep(1);
                            }

                            // Send a test message to verify the renewed certificate
                            IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString("CSR renewal verified");
                            (void)printf("Sending test message with renewed certificate...\r\n");
                            IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, send_confirm_callback, NULL);
                            IoTHubMessage_Destroy(message_handle);

                            while (g_message_count_send_confirmations < 1)
                            {
                                IoTHubDeviceClient_LL_DoWork(device_ll_handle);
                                ThreadAPI_Sleep(1);
                            }

                            (void)printf("Certificate renewal verified successfully.\r\n");
                        }

                        free(renewed_cert);
                    }
                }
                else
                {
                    (void)printf("CSR request failed (result=%s, status=%d)\r\n",
                        MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, csr_ctx.result), csr_ctx.response_status_code);
                }

                free(csr_ctx.payload);
            }
        }

        if (device_ll_handle != NULL)
        {
            IoTHubDeviceClient_LL_Destroy(device_ll_handle);
        }
    }

    // Free all the sdk subsystem
    IoTHub_Deinit();

    return 0;
}
