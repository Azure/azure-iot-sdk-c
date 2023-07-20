// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>

#include "azure_c_shared_utility/gballoc.h"
#include "internal/blob.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/shared_util_options.h"

static const char blockListXmlBegin[]  = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>";
static const char blockListXmlEnd[] = "</BlockList>";
static const char blockListUriMarker[] = "&comp=blocklist";
static const char blockListLatestTagXmlBegin[] = "<Latest>";
static const char blockListLatestTagXmlEnd[] = "</Latest>";

// Size of the string containing an Azure Blob Block ID.
// For more details please see
// https://learn.microsoft.com/en-us/rest/api/storageservices/put-block?tabs=azure-ad#uri-parameters
#define AZURE_BLOB_BLOCK_ID_SIZE 7

// Length of the string representation of an Azure Blob Block ID (enough to represent "049999").
#define AZURE_BLOB_BLOCK_ID_LENGTH 6

#define IS_HTTP_STATUS_CODE_SUCCESS(x)      ((x) >= 100 && (x) < 300)


static STRING_HANDLE createBlockIdListXml(SINGLYLINKEDLIST_HANDLE blockIDList)
{
    STRING_HANDLE blockIdListXml;

    if ((blockIdListXml = STRING_construct(blockListXmlBegin)) == NULL)
    {
        LogError("failed opening Block ID List XML");
    }
    else
    {
        LIST_ITEM_HANDLE listNode = singlylinkedlist_get_head_item(blockIDList);

        while (listNode != NULL)
        {
            STRING_HANDLE blockIdEncodedString = (STRING_HANDLE)singlylinkedlist_item_get_value(listNode);

            if (!(
                (STRING_concat(blockIdListXml, blockListLatestTagXmlBegin) == 0) &&
                (STRING_concat_with_STRING(blockIdListXml, blockIdEncodedString) == 0) &&
                (STRING_concat(blockIdListXml, blockListLatestTagXmlEnd) == 0)
                ))
            {
                LogError("failed to concatenate Block ID to XML");
                STRING_delete(blockIdListXml);
                blockIdListXml = NULL;
                break;
            }

            listNode = singlylinkedlist_get_next_item(listNode);
        }

        if (blockIdListXml != NULL &&
            STRING_concat(blockIdListXml, blockListXmlEnd) != 0)
        {
            LogError("failed closing Block ID List XML");
            STRING_delete(blockIdListXml);
            blockIdListXml = NULL;
        }
    }

    return blockIdListXml;
}

static bool removeAndDestroyBlockIdInList(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    STRING_HANDLE blockId = (STRING_HANDLE)item;
    STRING_delete(blockId);
    *continue_processing = true;
    return true;
}

HTTPAPIEX_HANDLE Blob_CreateHttpConnection(const char* blobStorageHostname, const char* certificates, const HTTP_PROXY_OPTIONS *proxyOptions, const char* networkInterface, const size_t timeoutInMilliseconds)
{
    HTTPAPIEX_HANDLE httpApiExHandle;
    
    if (blobStorageHostname == NULL)
    {
        LogError("Required storage hostname is NULL, blobStorageHostname=%p", blobStorageHostname);
        httpApiExHandle = NULL;
    }
    else
    {
        if ((httpApiExHandle = HTTPAPIEX_Create(blobStorageHostname)) == NULL)
        {
            LogError("unable to create a HTTPAPIEX_HANDLE");
        }
        else if ((timeoutInMilliseconds != 0) && (HTTPAPIEX_SetOption(httpApiExHandle, OPTION_HTTP_TIMEOUT, &timeoutInMilliseconds) == HTTPAPIEX_ERROR))
        {
            LogError("unable to set blob transfer timeout");
            HTTPAPIEX_Destroy(httpApiExHandle);
            httpApiExHandle = NULL;
        }
        else if ((certificates != NULL) && (HTTPAPIEX_SetOption(httpApiExHandle, OPTION_TRUSTED_CERT, certificates) == HTTPAPIEX_ERROR))
        {
            LogError("failure in setting trusted certificates");
            HTTPAPIEX_Destroy(httpApiExHandle);
            httpApiExHandle = NULL;
        }
        else if ((proxyOptions != NULL && proxyOptions->host_address != NULL) && HTTPAPIEX_SetOption(httpApiExHandle, OPTION_HTTP_PROXY, proxyOptions) == HTTPAPIEX_ERROR)
        {
            LogError("failure in setting proxy options");
            HTTPAPIEX_Destroy(httpApiExHandle);
            httpApiExHandle = NULL;
        }
        else if ((networkInterface != NULL) && HTTPAPIEX_SetOption(httpApiExHandle, OPTION_CURL_INTERFACE, networkInterface) == HTTPAPIEX_ERROR)
        {
            LogError("failure in setting network interface");
            HTTPAPIEX_Destroy(httpApiExHandle);
            httpApiExHandle = NULL;
        }
    }

    return httpApiExHandle;
}

void Blob_DestroyHttpConnection(HTTPAPIEX_HANDLE httpApiExHandle)
{
    if (httpApiExHandle != NULL)
    {
        HTTPAPIEX_Destroy(httpApiExHandle);
    }
}

BLOB_RESULT Blob_PutBlock(
        HTTPAPIEX_HANDLE httpApiExHandle,
        const char* relativePath,
        unsigned int blockID,
        BUFFER_HANDLE blockData,
        SINGLYLINKEDLIST_HANDLE blockIDList,
        unsigned int* httpStatus,
        BUFFER_HANDLE httpResponse)
{
    BLOB_RESULT result;

    if (httpApiExHandle == NULL ||
        relativePath == NULL ||
        blockData == NULL ||
        blockIDList == NULL)
    {
        LogError("invalid argument detected blockData=%p blockIDList=%p relativePath=%p httpApiExHandle=%p", blockData, blockIDList, relativePath, httpApiExHandle);
        result = BLOB_ERROR;
    }
    else if (blockID > 49999) /*outside the expected range of 000000... 049999*/
    {
        LogError("block ID too large");
        result = BLOB_ERROR;
    }
    else
    {   
        char blockIdString[AZURE_BLOB_BLOCK_ID_SIZE]; /*this will contain 000000... 049999*/
        if (sprintf(blockIdString, "%6u", (unsigned int)blockID) != AZURE_BLOB_BLOCK_ID_LENGTH) /*produces 000000... 049999*/
        {
            LogError("failed to sprintf");
            result = BLOB_ERROR;
        }
        else
        {
            STRING_HANDLE blockIdEncodedString = Azure_Base64_Encode_Bytes((const unsigned char*)blockIdString, AZURE_BLOB_BLOCK_ID_LENGTH);

            if (blockIdEncodedString == NULL)
            {
                LogError("unable to Azure_Base64_Encode_Bytes");
                result = BLOB_ERROR;
            }
            else
            {
                LIST_ITEM_HANDLE newListItem = singlylinkedlist_add(blockIDList, blockIdEncodedString);

                /*add the blockId base64 encoded to the XML*/
                if (newListItem == NULL)
                {
                    LogError("unable to store block ID");
                    STRING_delete(blockIdEncodedString);
                    result = BLOB_ERROR;
                }
                else
                {
                    STRING_HANDLE newRelativePath = STRING_construct(relativePath);

                    if (newRelativePath == NULL)
                    {
                        LogError("unable to STRING_construct");
                        (void)singlylinkedlist_remove(blockIDList, newListItem);
                        STRING_delete(blockIdEncodedString);
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        if (!(
                            (STRING_concat(newRelativePath, "&comp=block&blockid=") == 0) &&
                            (STRING_concat_with_STRING(newRelativePath, blockIdEncodedString) == 0)
                            ))
                        {
                            LogError("unable to STRING concatenate");
                            (void)singlylinkedlist_remove(blockIDList, newListItem);
                            STRING_delete(blockIdEncodedString);
                            result = BLOB_ERROR;
                        }
                        else
                        {
                            unsigned int httpResponseStatusCode = 0;

                            if (HTTPAPIEX_ExecuteRequest(
                                    httpApiExHandle,
                                    HTTPAPI_REQUEST_PUT,
                                    STRING_c_str(newRelativePath),
                                    NULL,
                                    blockData,
                                    &httpResponseStatusCode,
                                    NULL,
                                    httpResponse) != HTTPAPIEX_OK ||
                                    !IS_HTTP_STATUS_CODE_SUCCESS(httpResponseStatusCode))
                            {
                                LogError("unable to HTTPAPIEX_ExecuteRequest (HTTP response status %u)", httpResponseStatusCode);
                                (void)singlylinkedlist_remove(blockIDList, newListItem);
                                STRING_delete(blockIdEncodedString);
                                result = BLOB_HTTP_ERROR;
                            }
                            else
                            {
                                result = BLOB_OK;
                            }

                            if (httpStatus != NULL)
                            {
                                *httpStatus = httpResponseStatusCode;
                            }
                        }

                        STRING_delete(newRelativePath);
                    }
                }
            }
        }
    }
    return result;
}

void Blob_ClearBlockIdList(SINGLYLINKEDLIST_HANDLE blockIdList)
{
    if (blockIdList != NULL)
    {
        if (singlylinkedlist_remove_if(blockIdList, removeAndDestroyBlockIdInList, NULL) != 0)
        {
            LogError("Failed clearing block ID list");
        }
    }
}

// Blob_PutBlockList to send an XML of uploaded blockIds to the server after the application's payload block(s) have been transferred.
BLOB_RESULT Blob_PutBlockList(
    HTTPAPIEX_HANDLE httpApiExHandle,
    const char* relativePath,
    SINGLYLINKEDLIST_HANDLE blockIDList,
    unsigned int* httpStatus,
    BUFFER_HANDLE httpResponse)
{
    BLOB_RESULT result;

    if (httpApiExHandle == NULL || relativePath == NULL || blockIDList == NULL || httpStatus == NULL)
    {
        LogError("Invalid argument (httpApiExHandle=%p, relativePath=%p, blockIDList=%p, httpStatus=%p)",
            httpApiExHandle, relativePath, blockIDList, httpStatus);
            result = BLOB_INVALID_ARG;
    }
    else if (singlylinkedlist_get_head_item(blockIDList) == NULL)
    {
        // Block ID List is empty, there is nothing to be be sent to Azure Blob Storage.
        // In this case we better return error, forcing the caller to call this function only
        // if at least one call to Blob_PutBlock() succeeds.
        LogError("Block ID list is empty");
        result = BLOB_ERROR;
    }
    else
    {
        STRING_HANDLE blockIdListXml = createBlockIdListXml(blockIDList);
        
        /*complete the XML*/
        if (blockIdListXml == NULL)
        {
            LogError("failed creating Block ID list XML");
            result = BLOB_ERROR;
        }
        else
        {
            STRING_HANDLE newRelativePath = STRING_construct(relativePath);

            if (newRelativePath == NULL)
            {
                LogError("failed to STRING_construct");
                result = BLOB_ERROR;
            }
            else
            {
                if (STRING_concat(newRelativePath, blockListUriMarker) != 0)
                {
                    LogError("failed to STRING_concat");
                    result = BLOB_ERROR;
                }
                else
                {
                    const char* blockIdListXmlCharPtr = STRING_c_str(blockIdListXml);
                    size_t blockIdListXmlLength = STRING_length(blockIdListXml);
                    BUFFER_HANDLE blockIDListAsBuffer = BUFFER_create((const unsigned char*)blockIdListXmlCharPtr, blockIdListXmlLength);

                    if (blockIDListAsBuffer == NULL)
                    {
                        LogError("failed to BUFFER_create");
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        unsigned int httpResponseStatusCode = 0;

                        if (HTTPAPIEX_ExecuteRequest(
                                httpApiExHandle,
                                HTTPAPI_REQUEST_PUT,
                                STRING_c_str(newRelativePath),
                                NULL,
                                blockIDListAsBuffer,
                                &httpResponseStatusCode,
                                NULL,
                                httpResponse) != HTTPAPIEX_OK ||
                                !IS_HTTP_STATUS_CODE_SUCCESS(httpResponseStatusCode))
                        {
                            LogError("unable to HTTPAPIEX_ExecuteRequest (HTTP response status %u)", httpResponseStatusCode);
                            result = BLOB_HTTP_ERROR;
                        }
                        else
                        {
                            (void)singlylinkedlist_remove_if(blockIDList, removeAndDestroyBlockIdInList, NULL);
                            result = BLOB_OK;
                        }

                        if (httpStatus != NULL)
                        {
                            *httpStatus = httpResponseStatusCode;
                        }
                        
                        BUFFER_delete(blockIDListAsBuffer);
                    }
                }
                STRING_delete(newRelativePath);
            }

            STRING_delete(blockIdListXml);
        }
    }

    return result;
}
