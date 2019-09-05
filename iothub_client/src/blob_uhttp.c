// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include "azure_c_shared_utility/gballoc.h"
#include "internal/blob.h"
#include "internal/iothub_client_ll_uploadtoblob.h"

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/tickcounter.h"

#include "azure_c_shared_utility/threadapi.h"

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "azure_uhttp_c/uhttp.h"

#define DEFAULT_HTTPS_PORT                   443
static const char* const AUTHORIZATION_HEADER = "Authorization";

typedef struct BLOB_CONTEXT_DATA_TAG
{
    int http_resp_recv;
    int http_error;
    unsigned int status_code;
    BUFFER_HANDLE http_data;
    HTTP_PROXY_OPTIONS http_proxy_options;
} BLOB_CONTEXT_DATA;

static void on_http_error(void* callback_ctx, HTTP_CALLBACK_REASON error_result)
{
    (void)error_result;
    if (callback_ctx != NULL)
    {
        BLOB_CONTEXT_DATA* blob_data = (BLOB_CONTEXT_DATA*)callback_ctx;
        blob_data->http_error = 1;
    }
}

static void on_http_recv(void* callback_ctx, HTTP_CALLBACK_REASON request_result, const unsigned char* content, size_t content_len, unsigned int statusCode, HTTP_HEADERS_HANDLE responseHeadersHandle)
{
    (void)responseHeadersHandle;
    if (callback_ctx != NULL)
    {
        BLOB_CONTEXT_DATA* blob_data = (BLOB_CONTEXT_DATA*)callback_ctx;
        if (request_result == HTTP_CALLBACK_REASON_OK)
        {
            if (content != NULL)
            {
                // Clear the buffer
                (void)BUFFER_unbuild(blob_data->http_data);

                if (BUFFER_build(blob_data->http_data, content, content_len) != 0)
                {
                    LogError("Failure unable to create blob response BUFFER");
                    blob_data->http_error = 1;
                }
            }
            blob_data->status_code = statusCode;
        }
        else
        {
            blob_data->http_error = 1;
        }
        blob_data->http_resp_recv = 1;
    }
    else
    {
        LogError("callback_ctx is NULL");
    }
}

static void close_http_handle(HTTP_CLIENT_HANDLE http_client)
{
    uhttp_client_close(http_client, NULL, NULL);
    uhttp_client_dowork(http_client);
}

static int send_http_handle(HTTP_CLIENT_HANDLE http_client, const char* relative_path, const unsigned char* content_data, size_t content_len, BLOB_CONTEXT_DATA* blob_data)
{
    int result;
    if (uhttp_client_execute_request(http_client, HTTP_CLIENT_REQUEST_PUT, relative_path, NULL, content_data, content_len, on_http_recv, blob_data) != HTTP_CLIENT_OK)
    {
        LogError("Failure executing http request");
        result = MU_FAILURE;
    }
    else
    {
        // loop until timeout of connected
        do
        {
            uhttp_client_dowork(http_client);
        } while (blob_data->http_resp_recv == 0 && blob_data->http_error == 0);
        result = 0;
    }
    return result;
}

static const char* extract_hostname(const char* sas_uri, char** hostname)
{
    const char* result;

    const char* hostnameBegin = strstr(sas_uri, "://");
    if (hostnameBegin == NULL)
    {
        /*Codes_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
        LogError("hostname cannot be determined");
        result = NULL;
    }
    else
    {
        hostnameBegin += 3; /*have to skip 3 characters which are "://"*/
        if ((result = strchr(hostnameBegin, '/')) == NULL)
        {
            /*Codes_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
            LogError("hostname cannot be determined");
            result = NULL;
        }
        else
        {
            size_t hostnameSize = result - hostnameBegin;
            if ((*hostname = (char*)malloc(hostnameSize + 1)) == NULL)
            {
                /*Codes_SRS_BLOB_02_016: [ If the hostname copy cannot be made then then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                LogError("Failed to allocate hostname");
            }
            else
            {
                memset(*hostname, 0, hostnameSize+1);
                (void)memcpy(*hostname, hostnameBegin, hostnameSize);
            }
        }
    }
    return result;
}

static BLOB_RESULT Blob_UploadBlock(HTTP_CLIENT_HANDLE http_client, BLOB_CONTEXT_DATA* blob_data, const char* relativePath,
        const unsigned char* requestContent, size_t content_len,
        unsigned int blockID,
        STRING_HANDLE blockIDList)
{
    BLOB_RESULT result;

    if (requestContent == NULL ||
        blockIDList == NULL ||
        relativePath == NULL)
    {
        LogError("invalid argument detected requestContent=%p blockIDList=%p relativePath=%p", requestContent, blockIDList, relativePath);
        result = BLOB_ERROR;
    }
    else
    {
        char temp[7]; /*this will contain 000000... 049999*/
        if (sprintf(temp, "%6u", (unsigned int)blockID) != 6) /*produces 000000... 049999*/
        {
            /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
            LogError("failed to sprintf");
            result = BLOB_ERROR;
        }
        else
        {
            STRING_HANDLE newRelativePath;
            STRING_HANDLE blockIdString = Azure_Base64_Encode_Bytes((const unsigned char*)temp, 6);
            if (blockIdString == NULL)
            {
                /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                LogError("unable to Base64_Encode_Bytes");
                result = BLOB_ERROR;
            }
            else
            {
                /*add the blockId base64 encoded to the XML*/
                if (STRING_sprintf(blockIDList, "<Latest>%s</Latest>", STRING_c_str(blockIdString)) != 0)
                {
                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                    LogError("Unable to append latest string to upload block");
                    result = BLOB_ERROR;
                }
                /*Codes_SRS_BLOB_02_022: [ Blob_UploadMultipleBlocksFromSasUri shall construct a new relativePath from following string: base relativePath + "&comp=block&blockid=BASE64 encoded string of blockId" ]*/
                else if ((newRelativePath = STRING_construct_sprintf("%s&comp=block&blockid=%s", relativePath, STRING_c_str(blockIdString))) == NULL)
                {
                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                    LogError("unable to STRING_construct");
                    result = BLOB_ERROR;
                }
                else
                {
                    if (send_http_handle(http_client, STRING_c_str(newRelativePath), requestContent, content_len, blob_data) != 0)
                    {
                        LogError("Failed sending blob data");
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        result = BLOB_OK;
                    }
                    STRING_delete(newRelativePath);
                }
                STRING_delete(blockIdString);
            }
        }
    }
    return result;
}

static HTTP_CLIENT_HANDLE create_http_client(const char* hostname, BLOB_CONTEXT_DATA* blob_data, const char* certificate, HTTP_PROXY_OPTIONS* proxyOptions)
{
    HTTP_CLIENT_HANDLE result;
    // Create uhttp
    TLSIO_CONFIG tls_io_config;
    HTTP_PROXY_IO_CONFIG http_proxy;
    memset(&tls_io_config, 0, sizeof(TLSIO_CONFIG));
    tls_io_config.hostname = hostname;
    tls_io_config.port = DEFAULT_HTTPS_PORT;

    // Setup proxy
    if (proxyOptions->host_address != NULL)
    {
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_IO_CONFIG));
        http_proxy.hostname = hostname;
        http_proxy.port = DEFAULT_HTTPS_PORT;
        http_proxy.proxy_hostname = proxyOptions->host_address;
        http_proxy.proxy_port = proxyOptions->port;
        http_proxy.username = proxyOptions->username;
        http_proxy.password = proxyOptions->password;

        tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();
        tls_io_config.underlying_io_parameters = &http_proxy;
    }

    const IO_INTERFACE_DESCRIPTION* interface_desc = platform_get_default_tlsio();
    if (interface_desc == NULL)
    {
        LogError("platform default tlsio is NULL");
        result = NULL;
    }
    else if ((result = uhttp_client_create(interface_desc, &tls_io_config, on_http_error, blob_data)) == NULL)
    {
        LogError("Failed creating http object");
    }
    else if (certificate != NULL && uhttp_client_set_trusted_cert(result, certificate) != HTTP_CLIENT_OK)
    {
        LogError("Failed setting trusted cert");
        uhttp_client_destroy(result);
        result = NULL;
    }
    else if (uhttp_client_open(result, hostname, DEFAULT_HTTPS_PORT, NULL, NULL) != HTTP_CLIENT_OK)
    {
        LogError("Failed opening http url %s", hostname);
        uhttp_client_destroy(result);
        result = NULL;
    }
    else
    {
        uhttp_client_set_trace(result, true, true);
    }
    return result;
}

BLOB_RESULT Blob_UploadMultipleBlocksFromSasUri(const char* sas_uri, const char* certificate, HTTP_PROXY_OPTIONS* proxyOptions, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx, void* context, unsigned int* httpStatus, BUFFER_HANDLE httpResponse)
{
    (void)sas_uri;
    (void)getDataCallbackEx;
    (void)context;
    (void)httpResponse;

    BLOB_RESULT result = 0;
    /*Codes_SRS_BLOB_02_001: [ If SASURI is NULL then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
    if (sas_uri == NULL || getDataCallbackEx == NULL)
    {
        LogError("parameter sas uri is NULL");
        result = BLOB_INVALID_ARG;
    }
    else
    {
        char* hostname;

        /*Codes_SRS_BLOB_02_017: [ Blob_UploadMultipleBlocksFromSasUri shall copy from SASURI the hostname to a new const char* ]*/
        /*to find the hostname, the following logic is applied:*/
        /*the hostname starts at the first character after "://"*/
        /*the hostname ends at the first character before the next "/" after "://"*/
        // Codes_SRS_BLOB_02_019: [ Blob_UploadMultipleBlocksFromSasUri shall compute the base relative path of the request from the SASURI parameter. ]
        const char* relativePath = extract_hostname(sas_uri, &hostname);
        if (relativePath == NULL || hostname == NULL)
        {
            LogError("parameter SASURI is NULL");
            result = BLOB_ERROR;
        }
        else
        {
            HTTP_CLIENT_HANDLE http_client;
            BLOB_CONTEXT_DATA blob_data = { 0 };

            // Codes_SRS_BLOB_02_028: [ Blob_UploadMultipleBlocksFromSasUri shall construct an XML string with the following content: ]
            STRING_HANDLE blockIDList = STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>"); // the XML "build as we go"
            if (blockIDList == NULL)
            {
                /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                LogError("failed to STRING_construct");
                result = BLOB_HTTP_ERROR;
            }
            else if ((http_client = create_http_client(hostname, &blob_data, certificate, proxyOptions)) == NULL)
            {
                LogError("Failed opening http url %s", hostname);
                STRING_delete(blockIDList);
                result = BLOB_HTTP_ERROR;
            }
            else if ((blob_data.http_data = BUFFER_new()) == NULL)
            {
                LogError("Failure allocating buffer data");
                STRING_delete(blockIDList);
                close_http_handle(http_client);
                result = BLOB_HTTP_ERROR;
            }
            else
            {
                /*Codes_SRS_BLOB_02_021: [ For every block returned by `getDataCallbackEx` the following operations shall happen: ]*/
                unsigned int blockID = 0; /* incremented for each new block */
                unsigned int isError = 0; /* set to 1 if a block upload fails or if getDataCallbackEx returns incorrect blocks to upload */
                unsigned int uploadOneMoreBlock = 1; /* set to 1 while getDataCallbackEx returns correct blocks to upload */
                unsigned char const * source; /* data set by getDataCallbackEx */
                size_t size; /* source size set by getDataCallbackEx */
                do
                {
                    if (getDataCallbackEx(FILE_UPLOAD_OK, &source, &size, context) == IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT)
                    {
                        // Codes_SRS_BLOB_99_004: [ If `getDataCallbackEx` returns `IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_ABORT`, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop and return `BLOB_ABORTED`. ]*/
                        //LogInfo("Upload to blob has been aborted by the user");
                        uploadOneMoreBlock = 0;
                        result = BLOB_ABORTED;
                    }
                    else if (source == NULL || size == 0)
                    {
                        // Codes_SRS_BLOB_99_002: [ If the size of the block returned by `getDataCallbackEx` is 0 or if the data is NULL, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop. ]*/
                        uploadOneMoreBlock = 0;
                        result = BLOB_OK;
                    }
                    else
                    {
                        if (size > BLOCK_SIZE)
                        {
                            /*Codes_SRS_BLOB_99_001: [ If the size of the block returned by `getDataCallbackEx` is bigger than 4MB, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. ]*/
                            LogError("tried to upload block of size %zu, max allowed size is %d", size, BLOCK_SIZE);
                            result = BLOB_INVALID_ARG;
                            isError = 1;
                        }
                        else if (blockID >= MAX_BLOCK_COUNT)
                        {
                            /*Codes_SRS_BLOB_99_003: [ If `getDataCallbackEx` returns more than 50000 blocks, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. ]*/
                            LogError("unable to upload more than %d blocks in one blob", MAX_BLOCK_COUNT);
                            result = BLOB_INVALID_ARG;
                            isError = 1;
                        }
                        else
                        {
                            /*Codes_SRS_BLOB_02_023: [ Blob_UploadMultipleBlocksFromSasUri shall create a BUFFER_HANDLE from source and size parameters. ]*/
                            result = Blob_UploadBlock(http_client, &blob_data, relativePath, source, size, blockID, blockIDList);

                            /*Codes_SRS_BLOB_02_026: [ Otherwise, if HTTP response code is >=300 then Blob_UploadMultipleBlocksFromSasUri shall succeed and return BLOB_OK. ]*/
                            if (result != BLOB_OK || blob_data.status_code >= 300)
                            {
                                LogError("unable to Blob_UploadBlock. Returned value=%d, status_code=%d", result, (int)blob_data.status_code);
                                isError = 1;
                            }
                            *httpStatus = blob_data.status_code;
                        }
                        blockID++;
                    }
                    ThreadAPI_Sleep(100);
                }
                while(uploadOneMoreBlock && !isError);


                if (isError == 0 && result == BLOB_OK)
                {
                    STRING_HANDLE newRelativePath;
                    /*complete the XML*/
                    if (STRING_concat(blockIDList, "</BlockList>") != 0)
                    {
                        // Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]
                        LogError("failed to STRING_concat");
                        result = BLOB_ERROR;
                    }
                    // Codes_SRS_BLOB_02_029: [Blob_UploadMultipleBlocksFromSasUri shall construct a new relativePath from following string : base relativePath + "&comp=blocklist"]
                    else if ((newRelativePath = STRING_construct_sprintf("%s&comp=blocklist", relativePath)) == NULL)
                    {
                        /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                        LogError("failed to STRING_construct");
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        const char* content = STRING_c_str(blockIDList);
                        size_t content_len = STRING_length(blockIDList);
                        if (send_http_handle(http_client, STRING_c_str(newRelativePath), (const unsigned char*)content, content_len, &blob_data) != 0)
                        {
                            LogError("Failed sending blob data");
                            result = BLOB_ERROR;
                        }
                        else
                        {
                            result = BLOB_OK;
                        }
                       STRING_delete(newRelativePath);
                    }
                }
                STRING_delete(blockIDList);

                close_http_handle(http_client);
            }
            // Close the http client
        }
        free(hostname);
    }
    return result;
}
