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

#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_message.h"
#include "iothubtransporthttp.h"
#include "iothub_client_options.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#define ITERATIONS    1
#define MAX_BLOCK_SIZE_BYTES    4194304

/*Optional string with http proxy host and integer for http proxy port. (Both Win and Lin when uhttp is used.)*/
static const char* proxyHost = NULL;
static int proxyPort = 0;

// Enables verbose logging for IoT Hub communications. 
// Use CMake flag -Duse_uhttp_upload_logging=ON to enable BLOB verbose logging.
static int enableVerboseLogging = 0;

static char data_to_upload[MAX_BLOCK_SIZE_BYTES] = { 0 };
static int blocks_sent = 0;
static int blocks_count = 0;
static int block_size = 0;
static const char* file_name = NULL;

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT getDataCallback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    (void)context;

    if (result == FILE_UPLOAD_OK)
    {
        if (data != NULL && size != NULL)
        {
            // "block_count" is used to simulate reading chunks from a larger data content, like a large file.

            if (blocks_sent < blocks_count)
            {
                *data = (const unsigned char*)data_to_upload;
                *size = block_size;
                blocks_sent++;
                (void)printf("."); fflush(stdout);
            }
            else
            {
                // This simulates reaching the end of the file. At this point all the data content has been uploaded to blob.
                // Setting data to NULL and/or passing size as zero indicates the upload is completed.

                *data = NULL;
                *size = 0;

                (void)printf("\nIndicating upload is complete (%d blocks uploaded)\r\n", blocks_sent);
            }
        }
        else
        {
            // The last call to this callback is to indicate the result of uploading the previous data block provided.
            // Note: In this last call, data and size pointers are NULL.

            (void)printf("\nLast call to getDataCallback (result for %dth block uploaded: %s)\r\n", blocks_sent, MU_ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result));
        }
    }
    else
    {
        (void)printf("\nReceived unexpected result %s\r\n", MU_ENUM_TO_STRING(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result));
    }

    // This callback returns IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK to indicate that the upload shall continue.
    // To abort the upload, it should return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

void construct_data_upload_string()
{
    srand((unsigned int)time(NULL));

    for (size_t index = 0; index < block_size; index++)
    {
        data_to_upload[index] = (char)rand();
    }
}

int write_local_file()
{
    FILE* fp;
    int failed = 0;

    blocks_sent = 0;
    
    if ((fp = fopen(file_name, "wb")) == NULL)
    {
        (void)printf("Failed to create local reference file.\n");
    }
    else
    {
        while (!failed && blocks_sent < blocks_count)
        {
            if (fwrite(data_to_upload, block_size, 1, fp) != 1)
            {
                (void)printf("Failed to create write to local reference file block (%d / %d).\n", blocks_sent, blocks_count);
                failed = 1;
            }
            else
            {
                blocks_sent++;
            }
        }

        fclose(fp);
    }

    return failed;
}

int upload_test_file(IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle, const char* fileName, int blockSize, int blockCount)
{
    int failed = 0;

    (void)printf("--- BLOB UPLOAD STARTED: [%s] block_size=[%d] block_count=[%d]\n", fileName, blockSize, blockCount);

    if (blockSize > MAX_BLOCK_SIZE_BYTES)
    {
        (void)printf("Block size exceeds maximum allowed block size.");
        failed = 1;
    }
    else
    {
        file_name = fileName;
        blocks_count = blockCount;
        block_size = blockSize;

        construct_data_upload_string();
        if (write_local_file() != 0)
        {
            (void)printf("Failed to create local reference file.\n");
            failed = 1;
        }
        else
        {
            blocks_sent = 0;

            if (IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob(iotHubClientHandle, fileName, getDataCallback, NULL) != IOTHUB_CLIENT_OK)
            {
                (void)printf("%s failed to upload\n", fileName);
                failed = 1;
            }
            else
            {
                (void)printf("%s has been created\n", fileName);
            }
        }
    }

    (void)printf("--- BLOB UPLOAD %s: [%s] \n", failed ? "FAILED": "SUCCESS", fileName);
    return failed;
}

int main(int argc, char* argv[])
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;


    (void)IoTHub_Init();
    (void)printf("Starting the IoTHub client sample upload to blob with multiple blocks...\r\n");

    if (argc < 2)
    {
        (void)printf("Connection string missing as argument.\r\n");
        return 1;
    }

    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(argv[1], HTTP_Protocol);
    if (device_ll_handle == NULL)
    {
        (void)printf("Failure creating IotHub device.  Hint: Check your connection string.\r\n");
    }
    else
    {
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
        else if (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_CURL_VERBOSE, &enableVerboseLogging) != IOTHUB_CLIENT_OK)
        {
            (void)printf("failure to enable logging\n");
        }
        else
        {
            upload_test_file(device_ll_handle, "blob1x1.bin", 1, 1);
            upload_test_file(device_ll_handle, "blob10x10.bin", 10, 10);
            upload_test_file(device_ll_handle, "blob123x1.bin", 123, 1);
            upload_test_file(device_ll_handle, "blob1x123.bin", 1, 123);
            upload_test_file(device_ll_handle, "blob100x2000.bin", 100, 2000);
            upload_test_file(device_ll_handle, "blob1000000x2.bin", 1000000, 2);
            upload_test_file(device_ll_handle, "blob100x500.bin", 100, 500);
            upload_test_file(device_ll_handle, "blob4194303x1.bin", 4194303, 1);
            upload_test_file(device_ll_handle, "blob4194303x10.bin", 4000000, 10); 
            upload_test_file(device_ll_handle, "blob4194304x1.bin", 4194304, 1);
            upload_test_file(device_ll_handle, "blob4194304x10.bin", 4194304, 10); 
        }

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    }
    IoTHub_Deinit();
    return 0;
}
#endif /*DONT_USE_UPLOADTOBLOB*/

