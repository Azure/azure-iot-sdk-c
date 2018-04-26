// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef DONT_USE_UPLOADTOBLOB
#error "trying to compile iothub_client_sample_upload_to_blob_mb.c while DONT_USE_UPLOADTOBLOB is #define'd"
#else

#include <stdio.h>
#include <stdlib.h>

/* This sample uses the _LL APIs of iothub_client for example purposes.
That does not mean that HTTP only works with the _LL APIs.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_client_ll.h"
#include "iothub_message.h"
#include "iothubtransporthttp.h"
#include "certs.h"

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = "[device connection string]";

/*Optional string with http proxy host and integer for http proxy port (Linux only)         */
static const char* proxyHost = NULL; 
static int proxyPort = 0;
static const char* data_to_upload = "Hello World from IoTHubClient_LL_UploadToBlob\n";
static int block_count = 0;

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT getDataCallback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)

{
    (void)context;

    if (result == FILE_UPLOAD_OK)
    {
        if (data != NULL && size != NULL)
        {
            // "block_count" is used to simulate reading chunks from a larger data content, like a large file.

            if (block_count < 100)
            {
                *data = (const unsigned char*)data_to_upload;
                *size = strlen(data_to_upload);
                block_count++;
            }
            else
            {
                // This simulates reaching the end of the file. At this point all the data content has been uploaded to blob.
                // Setting data to NULL and/or passing size as zero indicates the upload is completed.

                *data = NULL;
                *size = 0;

                (void)printf("Indicating upload is complete (%d blocks uploaded)\r\n", block_count);
            }
        }
        else
        {
            // The last call to this callback is to indicate the result of uploading the previous data block provided.
            // Note: In this last call, data and size pointers are NULL.

            (void)printf("Last call to getDataCallback (result for %dth block uploaded: %s)\r\n", block_count, ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result));
        }
    }
    else
    {
        (void)printf("Received unexpected result %s\r\n", ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result));
    }

    // This callback returns IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK to indicate that the upload shall continue.
    // To abort the upload, it should return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

void iothub_client_sample_upload_to_blob_multi_block_run(void)
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    if (platform_init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        (void)printf("Starting the IoTHub client sample upload to blob with multiple blocks...\r\n");

        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
            (void)IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES
            HTTP_PROXY_OPTIONS http_proxy_options = {0};
            http_proxy_options.host_address = proxyHost;
            http_proxy_options.port = proxyPort;

            if (proxyHost != NULL && IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_HTTP_PROXY, &http_proxy_options) != IOTHUB_CLIENT_OK)
            {
                (void)printf("failure to set proxy\n");
            }
            else
            {
                if (IoTHubClient_LL_UploadMultipleBlocksToBlobEx(iotHubClientHandle, "subdir/hello_world_mb.txt", getDataCallback, NULL) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("hello world failed to upload\n");
                }
                else
                {
                    (void)printf("hello world has been created\n");
                }
            }
            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }
}
#endif /*DONT_USE_UPLOADTOBLOB*/
