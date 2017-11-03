// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include "azure_c_shared_utility/gballoc.h"
#include "blob.h"

#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/base64.h"

BLOB_RESULT Blob_UploadNextBlock(
        BUFFER_HANDLE requestContent,
        unsigned int blockID,
        STRING_HANDLE xml,
        const char* relativePath,
        HTTPAPIEX_HANDLE httpApiExHandle,
        unsigned int* httpStatus,
        BUFFER_HANDLE httpResponse)
{
    BLOB_RESULT result;

    if (requestContent == NULL ||
        xml == NULL ||
        relativePath == NULL ||
        httpApiExHandle == NULL ||
        httpStatus == NULL ||
        httpResponse == NULL)
    {
        LogError("invalid argument detected requestContent=%p xml=%p relativePath=%p httpApiExHandle=%p httpStatus=%p httpResponse=%p", requestContent, xml, relativePath, httpApiExHandle, httpStatus, httpResponse);
        result = BLOB_ERROR;
    }
    else
    {
        char temp[7]; /*this will contain 000000... 049999*/
        if (sprintf(temp, "%6u", (unsigned int)blockID) != 6) /*produces 000000... 049999*/
        {
            /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
            LogError("failed to sprintf");
            result = BLOB_ERROR;
        }
        else
        {
            STRING_HANDLE blockIdString = Base64_Encode_Bytes((const unsigned char*)temp, 6);
            if (blockIdString == NULL)
            {
                /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                LogError("unable to Base64_Encode_Bytes");
                result = BLOB_ERROR;
            }
            else
            {
                /*add the blockId base64 encoded to the XML*/
                if (!(
                    (STRING_concat(xml, "<Latest>") == 0) &&
                    (STRING_concat_with_STRING(xml, blockIdString) == 0) &&
                    (STRING_concat(xml, "</Latest>") == 0)
                    ))
                {
                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                    LogError("unable to STRING_concat");
                    result = BLOB_ERROR;
                }
                else
                {
                    /*Codes_SRS_BLOB_02_022: [ Blob_UploadFromSasUri shall construct a new relativePath from following string: base relativePath + "&comp=block&blockid=BASE64 encoded string of blockId" ]*/
                    STRING_HANDLE newRelativePath = STRING_construct(relativePath);
                    if (newRelativePath == NULL)
                    {
                        /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
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
                            /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                            LogError("unable to STRING concatenate");
                            result = BLOB_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_BLOB_02_024: [ Blob_UploadFromSasUri shall call HTTPAPIEX_ExecuteRequest with a PUT operation, passing httpStatus and httpResponse. ]*/
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
                                /*Codes_SRS_BLOB_02_025: [ If HTTPAPIEX_ExecuteRequest fails then Blob_UploadFromSasUri shall fail and return BLOB_HTTP_ERROR. ]*/
                                LogError("unable to HTTPAPIEX_ExecuteRequest");
                                result = BLOB_HTTP_ERROR;
                            }
                            else if (*httpStatus >= 300)
                            {
                                /*Codes_SRS_BLOB_02_026: [ Otherwise, if HTTP response code is >=300 then Blob_UploadFromSasUri shall succeed and return BLOB_OK. ]*/
                                LogError("HTTP status from storage does not indicate success (%d)", (int)*httpStatus);
                                result = BLOB_OK;
                            }
                            else
                            {
                                //LogInfo("Blob_UploadNextBlock succeeds");
                                /*Codes_SRS_BLOB_02_027: [ Otherwise Blob_UploadFromSasUri shall continue execution. ]*/
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

BLOB_RESULT Blob_UploadMultipleBlocksFromSasUri(const char* SASURI, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context, unsigned int* httpStatus, BUFFER_HANDLE httpResponse, const char* certificates)
{
    BLOB_RESULT result;
    /*Codes_SRS_BLOB_02_001: [ If SASURI is NULL then Blob_UploadFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
    if (SASURI == NULL)
    {
        LogError("parameter SASURI is NULL");
        result = BLOB_INVALID_ARG;
    }
    else
    {
        /*Codes_SRS_BLOB_02_002: [ If source is NULL and size is not zero then Blob_UploadFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
        if (getDataCallback == NULL)
        {
            LogError("IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback = %p is invalid", getDataCallback);
            result = BLOB_INVALID_ARG;
        }
        /*the below define avoid a "condition always false" on some compilers*/
        else
        {
            /*Codes_SRS_BLOB_02_017: [ Blob_UploadFromSasUri shall copy from SASURI the hostname to a new const char* ]*/
            /*Codes_SRS_BLOB_02_004: [ Blob_UploadFromSasUri shall copy from SASURI the hostname to a new const char*. ]*/
            /*to find the hostname, the following logic is applied:*/
            /*the hostname starts at the first character after "://"*/
            /*the hostname ends at the first character before the next "/" after "://"*/
            const char* hostnameBegin = strstr(SASURI, "://");
            if (hostnameBegin == NULL)
            {
                /*Codes_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
                LogError("hostname cannot be determined");
                result = BLOB_INVALID_ARG;
            }
            else
            {
                hostnameBegin += 3; /*have to skip 3 characters which are "://"*/
                const char* hostnameEnd = strchr(hostnameBegin, '/');
                if (hostnameEnd == NULL)
                {
                    /*Codes_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
                    LogError("hostname cannot be determined");
                    result = BLOB_INVALID_ARG;
                }
                else
                {
                    size_t hostnameSize = hostnameEnd - hostnameBegin;
                    char* hostname = (char*)malloc(hostnameSize + 1); /*+1 because of '\0' at the end*/
                    if (hostname == NULL)
                    {
                        /*Codes_SRS_BLOB_02_016: [ If the hostname copy cannot be made then then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                        LogError("oom - out of memory");
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        HTTPAPIEX_HANDLE httpApiExHandle;
                        (void)memcpy(hostname, hostnameBegin, hostnameSize);
                        hostname[hostnameSize] = '\0';

                        /*Codes_SRS_BLOB_02_006: [ Blob_UploadFromSasUri shall create a new HTTPAPI_EX_HANDLE by calling HTTPAPIEX_Create passing the hostname. ]*/
                        /*Codes_SRS_BLOB_02_018: [ Blob_UploadFromSasUri shall create a new HTTPAPI_EX_HANDLE by calling HTTPAPIEX_Create passing the hostname. ]*/
                        httpApiExHandle = HTTPAPIEX_Create(hostname);
                        if (httpApiExHandle == NULL)
                        {
                            /*Codes_SRS_BLOB_02_007: [ If HTTPAPIEX_Create fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR. ]*/
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
                            else
                            {

                                /*Codes_SRS_BLOB_02_008: [ Blob_UploadFromSasUri shall compute the relative path of the request from the SASURI parameter. ]*/
                                /*Codes_SRS_BLOB_02_019: [ Blob_UploadFromSasUri shall compute the base relative path of the request from the SASURI parameter. ]*/
                                const char* relativePath = hostnameEnd; /*this is where the relative path begins in the SasUri*/

                                /*Codes_SRS_BLOB_02_028: [ Blob_UploadFromSasUri shall construct an XML string with the following content: ]*/
                                STRING_HANDLE xml = STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>"); /*the XML "build as we go"*/
                                if (xml == NULL)
                                {
                                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                                    LogError("failed to STRING_construct");
                                    result = BLOB_HTTP_ERROR;
                                }
                                else
                                {
                                    /*Codes_SRS_BLOB_02_021: [ For every block of 4MB the following operations shall happen: ]*/
                                    unsigned int blockID = 0; /* incremented for each new block */
                                    unsigned int isError = 0; /* set to 1 if a block upload fails or if getDataCallback returns incorrect blocks to upload */
                                    unsigned int uploadOneMoreBlock; /* set to 1 while getDataCallback returns correct blocks to upload */
                                    unsigned char const * source; /* data set by getDataCallback */
                                    size_t size; /* source size set by getDataCallback */

                                    /* get first block */
                                    getDataCallback(FILE_UPLOAD_OK, &source, &size, context);

                                    uploadOneMoreBlock = (source != NULL && size > 0) ? 1 : 0;

                                    if (size > BLOCK_SIZE)
                                    {
                                        LogError("tried to upload block of size %zu, max allowed size is %zu", size, BLOCK_SIZE);
                                        result = BLOB_INVALID_ARG;
                                        isError = 1;
                                    }
                                    else
                                    {
                                        result = BLOB_OK;
                                    }

                                    while(uploadOneMoreBlock && !isError)
                                    {
                                        /*Codes_SRS_BLOB_02_023: [ Blob_UploadFromSasUri shall create a BUFFER_HANDLE from source and size parameters. ]*/
                                        BUFFER_HANDLE requestContent = BUFFER_create(source, size);
                                        if (requestContent == NULL)
                                        {
                                            /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                                            LogError("unable to BUFFER_create");
                                            result = BLOB_ERROR;
                                            isError = 1;
                                        }
                                        else
                                        {
                                            result = Blob_UploadNextBlock(
                                                    requestContent,
                                                    blockID,
                                                    xml,
                                                    relativePath,
                                                    httpApiExHandle,
                                                    httpStatus,
                                                    httpResponse);

                                            BUFFER_delete(requestContent);
                                        }

                                        /*Codes_SRS_BLOB_02_026: [ Otherwise, if HTTP response code is >=300 then Blob_UploadFromSasUri shall succeed and return BLOB_OK. ]*/
                                        if (result != BLOB_OK || *httpStatus >= 300)
                                        {
                                            isError = 1;
                                        }
                                        else
                                        {
                                            // try to get next block
                                            getDataCallback(FILE_UPLOAD_OK, &source, &size, context);
                                            if (source == NULL || size == 0)
                                            {
                                                uploadOneMoreBlock = 0;
                                            }
                                            else
                                            {
                                                blockID++;
                                                if (size > BLOCK_SIZE)
                                                {
                                                    LogError("tried to upload block of size %zu, max allowed size is %zu", size, BLOCK_SIZE);
                                                    result = BLOB_INVALID_ARG;
                                                    isError = 1;
                                                }
                                                else if (blockID >= MAX_BLOCK_COUNT)
                                                {
                                                    LogError("unable to upload more than %zu blocks in one blob", MAX_BLOCK_COUNT);
                                                    result = BLOB_INVALID_ARG;
                                                    isError = 1;
                                                }
                                            }
                                        }
                                    }

                                    if (isError)
                                    {
                                        /*do nothing, it will be reported "as is"*/
                                    }
                                    else
                                    {
                                        /*complete the XML*/
                                        if (STRING_concat(xml, "</BlockList>") != 0)
                                        {
                                            /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                                            LogError("failed to STRING_concat");
                                            result = BLOB_ERROR;
                                        }
                                        else
                                        {
                                            /*Codes_SRS_BLOB_02_029: [Blob_UploadFromSasUri shall construct a new relativePath from following string : base relativePath + "&comp=blocklist"]*/
                                            STRING_HANDLE newRelativePath = STRING_construct(relativePath);
                                            if (newRelativePath == NULL)
                                            {
                                                /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                                                LogError("failed to STRING_construct");
                                                result = BLOB_ERROR;
                                            }
                                            else
                                            {
                                                if (STRING_concat(newRelativePath, "&comp=blocklist") != 0)
                                                {
                                                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                                                    LogError("failed to STRING_concat");
                                                    result = BLOB_ERROR;
                                                }
                                                else
                                                {
                                                    /*Codes_SRS_BLOB_02_030: [ Blob_UploadFromSasUri shall call HTTPAPIEX_ExecuteRequest with a PUT operation, passing the new relativePath, httpStatus and httpResponse and the XML string as content. ]*/
                                                    const char* s = STRING_c_str(xml);
                                                    BUFFER_HANDLE xmlAsBuffer = BUFFER_create((const unsigned char*)s, strlen(s));
                                                    if (xmlAsBuffer == NULL)
                                                    {
                                                        /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
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
                                                            xmlAsBuffer,
                                                            httpStatus,
                                                            NULL,
                                                            httpResponse
                                                        ) != HTTPAPIEX_OK)
                                                        {
                                                            /*Codes_SRS_BLOB_02_031: [ If HTTPAPIEX_ExecuteRequest fails then Blob_UploadFromSasUri shall fail and return BLOB_HTTP_ERROR. ]*/
                                                            LogError("unable to HTTPAPIEX_ExecuteRequest");
                                                            result = BLOB_HTTP_ERROR;
                                                        }
                                                        else
                                                        {
                                                            /*Codes_SRS_BLOB_02_032: [ Otherwise, Blob_UploadFromSasUri shall succeed and return BLOB_OK. ]*/
                                                            result = BLOB_OK;
                                                        }
                                                        BUFFER_delete(xmlAsBuffer);
                                                    }
                                                }
                                                STRING_delete(newRelativePath);
                                            }
                                        }
                                    }
                                    STRING_delete(xml);
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
