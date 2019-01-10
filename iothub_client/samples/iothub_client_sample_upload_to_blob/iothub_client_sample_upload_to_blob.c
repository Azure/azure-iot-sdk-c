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
#include "iothubtransporthttp.h"

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

#define HELLO_WORLD "Hello World from IoTHubClient_LL_UploadToBlob"

int main(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();
    (void)printf("Starting the IoTHub client sample upload to blob...\r\n");

    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);
    if (device_ll_handle == NULL)
    {
        (void)printf("Failure createing Iothub device.  Hint: Check you connection string.\r\n");
    }
    else
    {
#ifndef WIN32
        size_t log = 1;
        (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_CURL_VERBOSE, &log);
#endif // !WIN32

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate.  This is only necessary on system with without
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
            if (IoTHubDeviceClient_LL_UploadToBlob(device_ll_handle, "subdir/hello_world.txt", (const unsigned char*)HELLO_WORLD, sizeof(HELLO_WORLD) - 1) != IOTHUB_CLIENT_OK)
            {
                (void)printf("hello world failed to upload\n");
            }
            else
            {
                (void)printf("hello world has been created\n");
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
