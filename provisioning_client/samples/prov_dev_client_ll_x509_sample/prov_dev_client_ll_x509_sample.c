// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate Azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.
#include <stdio.h>
#include <stdlib.h>

#include "iothub.h"
#include "iothub_message.h"
#include "iothub_client_version.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"

// Provisioning information:
static const char* global_prov_uri = "global.azure-devices-provisioning.net";
static const char* id_scope = "<ID SCOPE>";
static const char* registration_id = "<registration-id>";   // Case sensitive.

//
// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

// If using an OpenSSL ENGINE uncomment and modify the line below. 
//#define SAMPLE_OPENSSL_ENGINE "pkcs11"

// For development-time only: configure a hardcoded Trusted Root Authorities store.
//#define SET_TRUSTED_CERT_IN_SAMPLES

static const char* x509certificate =
"-----BEGIN CERTIFICATE-----\n"
"MIIBVjCB/aADAgECAhRMqqc/rOqEW+Afbkw4XyMLu1PUaTAKBggqhkjOPQQDAjAT\n"
"..."
"dwjNGPacV5zzgA==\n"
"-----END CERTIFICATE-----\n";

#ifndef SAMPLE_OPENSSL_ENGINE
static const char* x509privatekey =
"-----BEGIN EC PARAMETERS-----\n"
"..."
"-----END EC PRIVATE KEY-----\n";
#else
// PKCS#11 Example. Other OpenSSL Engines will require a different key ID.
static const char* x509privatekey = "pkcs11:object=test-privkey;type=private?pin-value=<yourPin>";
#endif

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES
#ifdef SAMPLE_MQTT
#include "iothubtransportmqtt.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
#include "iothubtransportmqtt_websockets.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
#include "iothubtransportamqp.h"
#include "azure_prov_client/prov_transport_amqp_client.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
#include "iothubtransportamqp_websockets.h"
#include "azure_prov_client/prov_transport_amqp_ws_client.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
#include "iothubtransporthttp.h"
#include "azure_prov_client/prov_transport_http_client.h"
#endif // SAMPLE_HTTP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

#ifdef SAMPLE_OPENSSL_ENGINE
static const char* opensslEngine = SAMPLE_OPENSSL_ENGINE;
static const OPTION_OPENSSL_KEY_TYPE x509_key_from_engine = KEY_TYPE_ENGINE;
#endif

#define MESSAGES_TO_SEND                2
#define TIME_BETWEEN_MESSAGES_SECONDS   2

typedef struct CLIENT_SAMPLE_INFO_TAG
{
    PROV_DEVICE_LL_HANDLE handle;
    unsigned int sleep_time_msec;
    char* iothub_uri;
    char* device_id;
    bool registration_complete;
} CLIENT_SAMPLE_INFO;

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
    bool connected;
    bool stop_running;
} IOTHUB_CLIENT_SAMPLE_INFO;

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)message;
    IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
    (void)printf("Stop message received from IoTHub\r\n");
    iothub_info->stop_running = true;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void registration_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    (void)user_context;
    (void)printf("Provisioning Status: %s\r\n", MU_ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
}

static void iothub_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    if (user_context == NULL)
    {
        printf("iothub_connection_status user_context is NULL\r\n");
    }
    else
    {
        IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
        if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
        {
            iothub_info->connected = true;
        }
        else
        {
            iothub_info->connected = false;
            iothub_info->stop_running = true;
        }
    }
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
        CLIENT_SAMPLE_INFO* user_ctx = (CLIENT_SAMPLE_INFO*)user_context;
        user_ctx->registration_complete = true;
        if (register_result == PROV_DEVICE_RESULT_OK)
        {
            (void)printf("Registration Information received from service: %s\r\n", iothub_uri);
            (void)mallocAndStrcpy_s(&user_ctx->iothub_uri, iothub_uri);
            (void)mallocAndStrcpy_s(&user_ctx->device_id, device_id);
        }
        else
        {
            (void)printf("Failure encountered on registration %s\r\n", MU_ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result) );
        }
    }
}

#define SAMPLE_MAX_RETRY_COUNT          3
#define SAMPLE_RETRY_DELAY_MILLISECS    2000
static void run_upload_to_blob(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle)
{
    static const char* azureStorageBlobPath = "subdir/hello_world_mb_with_retry_dps.txt";
    static const char* data_to_upload_format = "Hello World from iothub_client_sample_upload_to_blob_with_retry: %d\n";
    static char data_to_upload[128];

    char* uploadCorrelationId;
    char* azureBlobSasUri;

    if (IoTHubDeviceClient_LL_AzureStorageInitializeBlobUpload(
            device_ll_handle, azureStorageBlobPath, &uploadCorrelationId, &azureBlobSasUri) != IOTHUB_CLIENT_OK)
    {
        printf("failed initializing upload in IoT Hub\n");
    }
    else
    {
        // The SAS URI obtained above (azureBlobSasUri) can be used with other tools
        // like az_copy or Azure Storage SDK instead of the API functions
        // built-in this SDK shown below.

        IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE azureStorageClientHandle = IoTHubDeviceClient_LL_AzureStorageCreateClient(device_ll_handle, azureBlobSasUri);

        if (azureStorageClientHandle == NULL)
        {
            (void)printf("failed to create upload context\n");
        }
        else
        {
            bool uploadSuccessful = true;
            int uploadResultCode = 200;
            int attemptCount;

            for (uint32_t block_number = 0; block_number < 10 && uploadSuccessful; block_number++)
            {
                int data_size = snprintf(data_to_upload, sizeof(data_to_upload), data_to_upload_format, block_number);

                attemptCount = 1;

                while (true)
                {
                    if (IoTHubDeviceClient_LL_AzureStoragePutBlock(
                            azureStorageClientHandle, block_number, (const uint8_t*)data_to_upload, data_size) == IOTHUB_CLIENT_OK)
                    {
                        // Block upload succeeded. Continuing with next block.
                        break;
                    }
                    else
                    {
                        if (attemptCount >= SAMPLE_MAX_RETRY_COUNT)
                        {
                            (void)printf("Failed uploading block number %u to blob (exhausted all retries).\n", block_number);
                            uploadSuccessful = false;
                            uploadResultCode = 300;
                            break;
                        }
                        else
                        {
                            (void)printf("Failed uploading block number %u. Retrying in %u seconds...\n", block_number, SAMPLE_RETRY_DELAY_MILLISECS / 1000);
                            ThreadAPI_Sleep(SAMPLE_RETRY_DELAY_MILLISECS);
                        }
                    }

                    attemptCount++;
                }
            }

            if (uploadSuccessful)
            {
                // This function can also be retried if any errors occur.
                if (IoTHubDeviceClient_LL_AzureStoragePutBlockList(azureStorageClientHandle) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("Failed performing Azure Storage Put Blob List.\n");
                    uploadSuccessful = false;
                    uploadResultCode = 400;
                }
            }

            IoTHubDeviceClient_LL_AzureStorageDestroyClient(azureStorageClientHandle);

            attemptCount = 1;

            while (true)
            {
                if (IoTHubDeviceClient_LL_AzureStorageNotifyBlobUploadCompletion(
                        device_ll_handle, uploadCorrelationId, uploadSuccessful, uploadResultCode, uploadSuccessful ? "OK" : "Aborted")
                    == IOTHUB_CLIENT_OK)
                {
                    // Notification succeeded.
                    if (uploadSuccessful)
                    {
                        (void)printf("hello world blob has been created\n");
                    }
                    break;
                }
                else
                {
                    if (attemptCount >= SAMPLE_MAX_RETRY_COUNT)
                    {
                        (void)printf("Failed notifying Azure IoT Hub of upload completion (exhausted all retries).\n");
                        break;
                    }
                    else
                    {
                        (void)printf("Failed notifying Azure IoT Hub of upload completion. Retrying in %u seconds...\n", SAMPLE_RETRY_DELAY_MILLISECS / 1000);
                        ThreadAPI_Sleep(SAMPLE_RETRY_DELAY_MILLISECS);
                    }
                }

                attemptCount++;
            }
        }

        free(uploadCorrelationId);
        free(azureBlobSasUri);
    }
}

int main(void)
{
    bool traceOn = true;

    (void)IoTHub_Init();
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

    PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport;
    CLIENT_SAMPLE_INFO user_ctx;

    memset(&user_ctx, 0, sizeof(CLIENT_SAMPLE_INFO));

    // Protocol to USE - HTTP, AMQP, AMQP_WS, MQTT, MQTT_WS
#ifdef SAMPLE_MQTT
    prov_transport = Prov_Device_MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    prov_transport = Prov_Device_MQTT_WS_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    prov_transport = Prov_Device_AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    prov_transport = Prov_Device_AMQP_WS_Protocol;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    prov_transport = Prov_Device_HTTP_Protocol;
#endif // SAMPLE_HTTP

    user_ctx.registration_complete = false;
    user_ctx.sleep_time_msec = 10;

    (void)printf("Provisioning API Version: %s\r\n", Prov_Device_LL_GetVersionString());
    (void)printf("Iothub API Version: %s\r\n", IoTHubClient_GetVersionString());

    if ((user_ctx.handle = Prov_Device_LL_Create(global_prov_uri, id_scope, prov_transport)) == NULL)
    {
        (void)printf("failed calling Prov_Device_LL_Create\r\n");
    }
    // Set the X509 certificates in the DPS client
    else if (
#ifdef SAMPLE_OPENSSL_ENGINE
        (Prov_Device_LL_SetOption(user_ctx.handle, OPTION_OPENSSL_ENGINE, opensslEngine) != PROV_DEVICE_RESULT_OK) ||
        (Prov_Device_LL_SetOption(user_ctx.handle, OPTION_OPENSSL_PRIVATE_KEY_TYPE, &x509_key_from_engine) != PROV_DEVICE_RESULT_OK) ||
#endif
        (Prov_Device_LL_SetOption(user_ctx.handle, OPTION_X509_CERT, x509certificate) != PROV_DEVICE_RESULT_OK) ||
        (Prov_Device_LL_SetOption(user_ctx.handle, OPTION_X509_PRIVATE_KEY, x509privatekey) != PROV_DEVICE_RESULT_OK) ||
        (Prov_Device_LL_SetOption(user_ctx.handle, PROV_REGISTRATION_ID, registration_id) != PROV_DEVICE_RESULT_OK)
        )
    {
        (void)printf("failure to set options for x509, aborting\r\n");
    }
    else
    {
        Prov_Device_LL_SetOption(user_ctx.handle, PROV_OPTION_LOG_TRACE, &traceOn);
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate. This is only necessary on systems without
        // built in certificate stores.
        Prov_Device_LL_SetOption(user_ctx.handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

        if (Prov_Device_LL_Register_Device(user_ctx.handle, register_device_callback, &user_ctx, registration_status_callback, &user_ctx) != PROV_DEVICE_RESULT_OK)
        {
            (void)printf("failed calling Prov_Device_LL_Register_Device\r\n");
        }
        else
        {
            do
            {
                Prov_Device_LL_DoWork(user_ctx.handle);
                ThreadAPI_Sleep(user_ctx.sleep_time_msec);
            } while (!user_ctx.registration_complete);
        }
        Prov_Device_LL_Destroy(user_ctx.handle);
    }

    if (user_ctx.iothub_uri == NULL || user_ctx.device_id == NULL)
    {
        (void)printf("registration failed!\r\n");
    }
    else
    {
        IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport;

        // Protocol to USE - HTTP, AMQP, AMQP_WS, MQTT, MQTT_WS
#if defined(SAMPLE_MQTT) || defined(SAMPLE_HTTP) // HTTP sample will use mqtt protocol
        iothub_transport = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
        iothub_transport = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS

#ifdef SAMPLE_AMQP
        iothub_transport = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
        iothub_transport = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
        IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

        (void)printf("Creating IoTHub Device handle\r\n");
        if ((device_ll_handle = IoTHubDeviceClient_LL_CreateFromDeviceAuth(user_ctx.iothub_uri, user_ctx.device_id, iothub_transport) ) == NULL)
        {
            (void)printf("failed create IoTHub client %s!\r\n", user_ctx.iothub_uri);
        }
        else
        {
            IOTHUB_CLIENT_SAMPLE_INFO iothub_info;
            iothub_info.stop_running = false;
            iothub_info.connected = false;

            (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, iothub_connection_status, &iothub_info);

            // Set any option that are necessary.
            // For available options please see the iothub_sdk_options.md documentation

            IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_LOG_TRACE, &traceOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
            // Setting the Trusted Certificate. This is only necessary on systems without
            // built in certificate stores.
            IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#ifdef SAMPLE_OPENSSL_ENGINE
            IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_OPENSSL_ENGINE, opensslEngine);
            IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_OPENSSL_PRIVATE_KEY_TYPE, &x509_key_from_engine);
#endif // SAMPLE_OPENSSL_ENGINE

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_OVER_WEBSOCKETS
            //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
            //you are URL Encoding inputs yourself.
            //ONLY valid for use with MQTT
            bool urlEncodeOn = true;
            (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif
            
            run_upload_to_blob(device_ll_handle);

            IoTHubDeviceClient_LL_Destroy(device_ll_handle);
        }
    }

    free(user_ctx.iothub_uri);
    free(user_ctx.device_id);
    prov_dev_security_deinit();

    // Free all the sdk subsystem
    IoTHub_Deinit();

    return 0;
}
