// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef DONT_USE_UPLOADTOBLOB
#error "trying to compile iothub_client_sample_upload_to_blob.c while DONT_USE_UPLOADTOBLOB is #define'd"
#else

#include <stdio.h>
#include <stdlib.h>

/* This sample uses the _LL APIs of iothub_client for example purposes.
That does not mean that HTTP only works with the _LL APIs.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

#ifdef ARDUINO
#include "AzureIoT.h"
#else
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_client_ll.h"
#include "iothub_message.h"
#include "iothubtransporthttp.h"
#include "certs.h"
#endif

#ifdef MBED_BUILD_TIMESTAMP
#include "certs.h"
#endif // MBED_BUILD_TIMESTAMP

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = "<<device connection string>>";

/*Optional string with http proxy host and integer for http proxy port (Linux only)         */
static const char* proxyHost = NULL; 
static int proxyPort = 0;

#define HELLO_WORLD "Hello World from IoTHubClient_LL_UploadToBlob"

void iothub_client_sample_upload_to_blob_run(void)
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    if (platform_init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        (void)printf("Starting the IoTHub client sample upload to blob...\r\n");

        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            if (IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_TRUSTED_CERT, certificates) != IOTHUB_CLIENT_OK)
            {
                (void)printf("failure in IoTHubClient_LL_SetOption (TrustedCerts)");
            }
            else
            {
                HTTP_PROXY_OPTIONS http_proxy_options = {0};
                http_proxy_options.host_address = proxyHost;
                http_proxy_options.port = proxyPort;

                if (proxyHost != NULL && IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_HTTP_PROXY, &http_proxy_options) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("failure to set proxy\n");
                }
                else
                {
                    if (IoTHubClient_LL_UploadToBlob(iotHubClientHandle, "subdir/hello_world.txt", (const unsigned char*)HELLO_WORLD, sizeof(HELLO_WORLD) - 1) != IOTHUB_CLIENT_OK)
                    {
                        (void)printf("hello world failed to upload\n");
                    }
                    else
                    {
                        (void)printf("hello world has been created\n");
                    }
                }
            }
            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }
}
#endif /*DONT_USE_UPLOADTOBLOB*/
