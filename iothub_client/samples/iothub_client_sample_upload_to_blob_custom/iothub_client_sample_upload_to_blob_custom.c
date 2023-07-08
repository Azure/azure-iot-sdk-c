// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

#ifndef DONT_USE_UPLOADTOBLOB

#include <stdio.h>
#include <stdlib.h>

/* This sample uses the _LL APIs of iothub_client for example purposes.
That does not mean that HTTP only works with the _LL APIs.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

#include "iothub.h"
#include "iothub_device_client.h"

#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = "[device connection string]";

/*Optional string with http proxy host and integer for http proxy port (Linux only)         */
static const char* proxyHost = NULL;
static int proxyPort = 0;

static const char* data_to_upload_format = "Hello World from iothub_client_sample_upload_to_blob_custom: %d\n";
static char data_to_upload[128];

int main(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();
    (void)printf("Starting the IoTHub client sample upload to blob...\r\n");

    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (device_ll_handle == NULL)
    {
        (void)printf("Failure creating IotHub device. Hint: Check your connection string.\r\n");
    }
    else
    {
#ifndef WIN32
        size_t log = 1;
        (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_CURL_VERBOSE, &log);
#endif // !WIN32

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate. This is only necessary on systems without
        // built in certificate stores.
        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

        HTTP_PROXY_OPTIONS http_proxy_options = { 0 };
        http_proxy_options.host_address = proxyHost;
        http_proxy_options.port = proxyPort;

        if (proxyHost != NULL && IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_HTTP_PROXY, &http_proxy_options) != IOTHUB_CLIENT_OK)
        {
            (void)printf("failure to set proxy\n");
        }
        else
        {
            char* uploadCorrelationId;
            char* azureBlobSasUri;
            const char* bla = "subdir/hello_world_custom_mb.txt";

            if (IoTHubDeviceClient_LL_InitializeUpload(
                    device_ll_handle, bla, &uploadCorrelationId, &azureBlobSasUri) != IOTHUB_CLIENT_OK)
            {
                printf("failed initializing upload in IoT Hub\n");
            }
            else
            {
                IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContextHandle = IoTHubDeviceClient_LL_CreateUploadContext(device_ll_handle, azureBlobSasUri);

                if (uploadContextHandle == NULL)
                {
                    (void)printf("failed to create upload context\n");
                }
                else
                {
                    bool uploadSuccessful = true;
                    int uploadResultCode = 200;

                    for (uint32_t block_number = 0; block_number < 100; block_number++)
                    {
                        int data_size = snprintf(data_to_upload, sizeof(data_to_upload), data_to_upload_format, block_number);

                        if (IoTHubDeviceClient_LL_AzureStoragePutBlock(
                                uploadContextHandle, block_number, (const uint8_t*)data_to_upload, data_size) != IOTHUB_CLIENT_OK)
                        {
                            (void)printf("Failed uploading block number %u to blob. Aborting upload.\n", block_number);
                            uploadSuccessful = false;
                            uploadResultCode = 300;
                            break;
                        }
                    }

                    if (uploadSuccessful)
                    {
                        if (IoTHubDeviceClient_LL_AzureStoragePutBlockList(uploadContextHandle) != IOTHUB_CLIENT_OK)
                        {
                            (void)printf("failed performing Azure Storage Put Blob List.\n");
                            uploadSuccessful = false;
                            uploadResultCode = 400;
                        }
                    }

                    IoTHubDeviceClient_LL_DestroyUploadContext(uploadContextHandle);

                    // Hint: here if there is a failure (e.g., in HTTP transport)
                    //       this function can be called again in a retry loop if desired.
                    if (IoTHubDeviceClient_LL_NotifyUploadCompletion(
                            device_ll_handle, uploadCorrelationId, uploadSuccessful, uploadResultCode, uploadSuccessful ? "OK" : "Aborted")
                        != IOTHUB_CLIENT_OK)
                    {
                        (void)printf("Failed notifying Azure IoT Hub of upload completion.\n");
                    }
                    else
                    {
                        (void)printf("hello world has been created\n");
                    }
                }

                free(uploadCorrelationId);
                free(azureBlobSasUri);
            }
        }
        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    }
    // Free all the sdk subsystem
    IoTHub_Deinit();

    printf("Press any key to continue");
    (void)getchar();

    return 0;
}
#endif /*DONT_USE_UPLOADTOBLOB*/
