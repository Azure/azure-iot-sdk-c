// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include "azure_c_shared_utility/gballoc.h"
#include "internal/blob.h"
#include "internal/iothub_client_ll_uploadtoblob.h"

#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/shared_util_options.h"

BLOB_RESULT Blob_UploadBlock(
        HTTPAPIEX_HANDLE httpApiExHandle,
        const char* relativePath,
        BUFFER_HANDLE requestContent,
        unsigned int blockID,
        STRING_HANDLE blockIDList,
        unsigned int* httpStatus,
        BUFFER_HANDLE httpResponse)
{
    BLOB_RESULT result;

    if (requestContent == NULL ||
        blockIDList == NULL ||
        relativePath == NULL ||
        httpApiExHandle == NULL ||
        httpStatus == NULL ||
        httpResponse == NULL)
    {
        LogError("invalid argument detected requestContent=%p blockIDList=%p relativePath=%p httpApiExHandle=%p httpStatus=%p httpResponse=%p", requestContent, blockIDList, relativePath, httpApiExHandle, httpStatus, httpResponse);
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
            STRING_HANDLE blockIdString = Base64_Encode_Bytes((const unsigned char*)temp, 6);
            if (blockIdString == NULL)
            {
                /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                LogError("unable to Base64_Encode_Bytes");
                result = BLOB_ERROR;
            }
            else
            {
                /*add the blockId base64 encoded to the XML*/
                if (!(
                    (STRING_concat(blockIDList, "<Latest>") == 0) &&
                    (STRING_concat_with_STRING(blockIDList, blockIdString) == 0) &&
                    (STRING_concat(blockIDList, "</Latest>") == 0)
                    ))
                {
                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                    LogError("unable to STRING_concat");
                    result = BLOB_ERROR;
                }
                else
                {
                    /*Codes_SRS_BLOB_02_022: [ Blob_UploadMultipleBlocksFromSasUri shall construct a new relativePath from following string: base relativePath + "&comp=block&blockid=BASE64 encoded string of blockId" ]*/
                    STRING_HANDLE newRelativePath = STRING_construct(relativePath);
                    if (newRelativePath == NULL)
                    {
                        /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                        LogError("unable to STRING_construct");
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        if (!(
                            (STRING_concat(newRelativePath, "&comp=block&blockid=") == 0) &&
                            (STRING_concat_with_STRING(newRelativePath, blockIdString) == 0)
                            ))
                        {
                            /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                            LogError("unable to STRING concatenate");
                            result = BLOB_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_BLOB_02_024: [ Blob_UploadMultipleBlocksFromSasUri shall call HTTPAPIEX_ExecuteRequest with a PUT operation, passing httpStatus and httpResponse. ]*/
                            if (HTTPAPIEX_ExecuteRequest(
                                httpApiExHandle,
                                HTTPAPI_REQUEST_PUT,
                                STRING_c_str(newRelativePath),
                                NULL,
                                requestContent,
                                httpStatus,
                                NULL,
                                httpResponse) != HTTPAPIEX_OK
                                )
                            {
                                /*Codes_SRS_BLOB_02_025: [ If HTTPAPIEX_ExecuteRequest fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_HTTP_ERROR. ]*/
                                LogError("unable to HTTPAPIEX_ExecuteRequest");
                                result = BLOB_HTTP_ERROR;
                            }
                            else if (*httpStatus >= 300)
                            {
                                /*Codes_SRS_BLOB_02_026: [ Otherwise, if HTTP response code is >=300 then Blob_UploadMultipleBlocksFromSasUri shall succeed and return BLOB_OK. ]*/
                                LogError("HTTP status from storage does not indicate success (%d)", (int)*httpStatus);
                                result = BLOB_OK;
                            }
                            else
                            {
                                /*Codes_SRS_BLOB_02_027: [ Otherwise Blob_UploadMultipleBlocksFromSasUri shall continue execution. ]*/
                                result = BLOB_OK;
                            }
                        }
                        STRING_delete(newRelativePath);
                    }
                }
                STRING_delete(blockIdString);
            }
        }
    }
    return result;
}

BLOB_RESULT Blob_UploadMultipleBlocksFromSasUri(const char* SASURI, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx, void* context, unsigned int* httpStatus, BUFFER_HANDLE httpResponse, const char* certificates, HTTP_PROXY_OPTIONS *proxyOptions)
{
    BLOB_RESULT result;
    /*Codes_SRS_BLOB_02_001: [ If SASURI is NULL then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
    if (SASURI == NULL)
    {
        LogError("parameter SASURI is NULL");
        result = BLOB_INVALID_ARG;
    }
    else
    {
        /*Codes_SRS_BLOB_02_002: [ If getDataCallbackEx is NULL then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
        if (getDataCallbackEx == NULL)
        {
            LogError("IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx is NULL");
            result = BLOB_INVALID_ARG;
        }
        /*the below define avoid a "condition always false" on some compilers*/
        else
        {
            /*Codes_SRS_BLOB_02_017: [ Blob_UploadMultipleBlocksFromSasUri shall copy from SASURI the hostname to a new const char* ]*/
            /*to find the hostname, the following logic is applied:*/
            /*the hostname starts at the first character after "://"*/
            /*the hostname ends at the first character before the next "/" after "://"*/
            const char* hostnameBegin = strstr(SASURI, "://");
            if (hostnameBegin == NULL)
            {
                /*Codes_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
                LogError("hostname cannot be determined");
                result = BLOB_INVALID_ARG;
            }
            else
            {
                hostnameBegin += 3; /*have to skip 3 characters which are "://"*/
                const char* hostnameEnd = strchr(hostnameBegin, '/');
                if (hostnameEnd == NULL)
                {
                    /*Codes_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
                    LogError("hostname cannot be determined");
                    result = BLOB_INVALID_ARG;
                }
                else
                {
                    size_t hostnameSize = hostnameEnd - hostnameBegin;
                    char* hostname = (char*)malloc(hostnameSize + 1); /*+1 because of '\0' at the end*/
                    if (hostname == NULL)
                    {
                        /*Codes_SRS_BLOB_02_016: [ If the hostname copy cannot be made then then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                        LogError("oom - out of memory");
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        HTTPAPIEX_HANDLE httpApiExHandle;
                        (void)memcpy(hostname, hostnameBegin, hostnameSize);
                        hostname[hostnameSize] = '\0';

                        /*Codes_SRS_BLOB_02_018: [ Blob_UploadMultipleBlocksFromSasUri shall create a new HTTPAPI_EX_HANDLE by calling HTTPAPIEX_Create passing the hostname. ]*/
                        httpApiExHandle = HTTPAPIEX_Create(hostname);
                        if (httpApiExHandle == NULL)
                        {
                            /*Codes_SRS_BLOB_02_007: [ If HTTPAPIEX_Create fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR. ]*/
                            LogError("unable to create a HTTPAPIEX_HANDLE");
                            result = BLOB_ERROR;
                        }
                        else
                        {
                            if ((certificates != NULL)&& (HTTPAPIEX_SetOption(httpApiExHandle, "TrustedCerts", certificates) == HTTPAPIEX_ERROR))
                            {
                                LogError("failure in setting trusted certificates");
                                result = BLOB_ERROR;
                            }
                            else if ((proxyOptions != NULL && proxyOptions->host_address != NULL) && HTTPAPIEX_SetOption(httpApiExHandle, OPTION_HTTP_PROXY, proxyOptions) == HTTPAPIEX_ERROR)
                            {
                                LogError("failure in setting proxy options");
                                result = BLOB_ERROR;
                            }
                            else
                            {
                                /*Codes_SRS_BLOB_02_019: [ Blob_UploadMultipleBlocksFromSasUri shall compute the base relative path of the request from the SASURI parameter. ]*/
                                const char* relativePath = hostnameEnd; /*this is where the relative path begins in the SasUri*/

                                /*Codes_SRS_BLOB_02_028: [ Blob_UploadMultipleBlocksFromSasUri shall construct an XML string with the following content: ]*/
                                STRING_HANDLE blockIDList = STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>"); /*the XML "build as we go"*/
                                if (blockIDList == NULL)
                                {
                                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                                    LogError("failed to STRING_construct");
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
                                    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT getDataReturnValue;

                                    do
                                    {
                                        getDataReturnValue = getDataCallbackEx(FILE_UPLOAD_OK, &source, &size, context);
                                        if (getDataReturnValue == IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT)
                                        {
                                            /*Codes_SRS_BLOB_99_004: [ If `getDataCallbackEx` returns `IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_ABORT`, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop and return `BLOB_ABORTED`. ]*/
                                            LogInfo("Upload to blob has been aborted by the user");
                                            uploadOneMoreBlock = 0;
                                            result = BLOB_ABORTED;
                                        }
                                        else if (source == NULL || size == 0)
                                        {
                                            /*Codes_SRS_BLOB_99_002: [ If the size of the block returned by `getDataCallbackEx` is 0 or if the data is NULL, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop. ]*/
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
                                                LogError("unable to upload more than %zu blocks in one blob", MAX_BLOCK_COUNT);
                                                result = BLOB_INVALID_ARG;
                                                isError = 1;
                                            }
                                            else
                                            {
                                                /*Codes_SRS_BLOB_02_023: [ Blob_UploadMultipleBlocksFromSasUri shall create a BUFFER_HANDLE from source and size parameters. ]*/
                                                BUFFER_HANDLE requestContent = BUFFER_create(source, size);
                                                if (requestContent == NULL)
                                                {
                                                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                                                    LogError("unable to BUFFER_create");
                                                    result = BLOB_ERROR;
                                                    isError = 1;
                                                }
                                                else
                                                {
                                                    result = Blob_UploadBlock(
                                                            httpApiExHandle,
                                                            relativePath,
                                                            requestContent,
                                                            blockID,
                                                            blockIDList,
                                                            httpStatus,
                                                            httpResponse);

                                                    BUFFER_delete(requestContent);
                                                }

                                                /*Codes_SRS_BLOB_02_026: [ Otherwise, if HTTP response code is >=300 then Blob_UploadMultipleBlocksFromSasUri shall succeed and return BLOB_OK. ]*/
                                                if (result != BLOB_OK || *httpStatus >= 300)
                                                {
                                                    LogError("unable to Blob_UploadBlock. Returned value=%d, httpStatus=%u", result, httpStatus);
                                                    isError = 1;
                                                }
                                            }
                                            blockID++;
                                        }
                                    }
                                    while(uploadOneMoreBlock && !isError);

                                    if (isError || result != BLOB_OK)
                                    {
                                        /*do nothing, it will be reported "as is"*/
                                    }
                                    else
                                    {
                                        /*complete the XML*/
                                        if (STRING_concat(blockIDList, "</BlockList>") != 0)
                                        {
                                            /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                                            LogError("failed to STRING_concat");
                                            result = BLOB_ERROR;
                                        }
                                        else
                                        {
                                            /*Codes_SRS_BLOB_02_029: [Blob_UploadMultipleBlocksFromSasUri shall construct a new relativePath from following string : base relativePath + "&comp=blocklist"]*/
                                            STRING_HANDLE newRelativePath = STRING_construct(relativePath);
                                            if (newRelativePath == NULL)
                                            {
                                                /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                                                LogError("failed to STRING_construct");
                                                result = BLOB_ERROR;
                                            }
                                            else
                                            {
                                                if (STRING_concat(newRelativePath, "&comp=blocklist") != 0)
                                                {
                                                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                                                    LogError("failed to STRING_concat");
                                                    result = BLOB_ERROR;
                                                }
                                                else
                                                {
                                                    /*Codes_SRS_BLOB_02_030: [ Blob_UploadMultipleBlocksFromSasUri shall call HTTPAPIEX_ExecuteRequest with a PUT operation, passing the new relativePath, httpStatus and httpResponse and the XML string as content. ]*/
                                                    const char* s = STRING_c_str(blockIDList);
                                                    BUFFER_HANDLE blockIDListAsBuffer = BUFFER_create((const unsigned char*)s, strlen(s));
                                                    if (blockIDListAsBuffer == NULL)
                                                    {
                                                        /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_ERROR ]*/
                                                        LogError("failed to BUFFER_create");
                                                        result = BLOB_ERROR;
                                                    }
                                                    else
                                                    {
                                                        if (HTTPAPIEX_ExecuteRequest(
                                                            httpApiExHandle,
                                                            HTTPAPI_REQUEST_PUT,
                                                            STRING_c_str(newRelativePath),
                                                            NULL,
                                                            blockIDListAsBuffer,
                                                            httpStatus,
                                                            NULL,
                                                            httpResponse
                                                        ) != HTTPAPIEX_OK)
                                                        {
                                                            /*Codes_SRS_BLOB_02_031: [ If HTTPAPIEX_ExecuteRequest fails then Blob_UploadMultipleBlocksFromSasUri shall fail and return BLOB_HTTP_ERROR. ]*/
                                                            LogError("unable to HTTPAPIEX_ExecuteRequest");
                                                            result = BLOB_HTTP_ERROR;
                                                        }
                                                        else
                                                        {
                                                            /*Codes_SRS_BLOB_02_032: [ Otherwise, Blob_UploadMultipleBlocksFromSasUri shall succeed and return BLOB_OK. ]*/
                                                            result = BLOB_OK;
                                                        }
                                                        BUFFER_delete(blockIDListAsBuffer);
                                                    }
                                                }
                                                STRING_delete(newRelativePath);
                                            }
                                        }
                                    }
                                    STRING_delete(blockIDList);
                                }

                            }
                            HTTPAPIEX_Destroy(httpApiExHandle);
                        }
                        free(hostname);
                    }
                }
            }
        }
    }
    return result;
}
