// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file blob.h
*    @brief Contains blob APIs needed for File Upload feature of IoTHub client.
*
*    @details IoTHub client needs to upload a byte array by using blob storage API
*             IoTHub service provides the complete SAS URI to execute a PUT request
*             that will upload the data.
*
*/

#ifndef BLOB_H
#define BLOB_H

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings_types.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/shared_util_options.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#include "umock_c/umock_c_prod.h"

/* Allow unit tests to override MAX_BLOCK_COUNT to something much smaller */
#ifndef MAX_BLOCK_COUNT
/* Maximum count of blocks uploaded is 50000, per server*/
#define MAX_BLOCK_COUNT 50000
#endif

#define BLOB_RESULT_VALUES \
    BLOB_OK,               \
    BLOB_ERROR,            \
    BLOB_NOT_IMPLEMENTED,  \
    BLOB_HTTP_ERROR,       \
    BLOB_INVALID_ARG,      \
    BLOB_ABORTED

MU_DEFINE_ENUM_WITHOUT_INVALID(BLOB_RESULT, BLOB_RESULT_VALUES)

MOCKABLE_FUNCTION(, HTTPAPIEX_HANDLE, Blob_CreateHttpConnection, const char*, blobStorageHostname, const char*, certificates, const HTTP_PROXY_OPTIONS*, proxyOptions, const char*, networkInterface, const size_t, timeoutInMilliseconds);

MOCKABLE_FUNCTION(, void, Blob_DestroyHttpConnection, HTTPAPIEX_HANDLE, httpApiExHandle);

MOCKABLE_FUNCTION(, void, Blob_ClearBlockIdList, SINGLYLINKEDLIST_HANDLE, blockIdList);

/**
* @brief  Synchronously uploads a byte array as a new block to blob storage (put block operation)
*
* @param  httpApiExHandle     The HTTP connection handle
* @param  relativePath        The destination path within the storage
* @param  blockID             The block id (from 00000 to 49999)
* @param  blockData           The data to upload
* @param  blockIDList         The list where to store the block IDs
* @param  httpStatus          A pointer to an out argument receiving the HTTP status (available only when the return value is BLOB_OK)
* @param  httpResponse        A BUFFER_HANDLE that receives the HTTP response from the server (available only when the return value is BLOB_OK)
*/
MOCKABLE_FUNCTION(, BLOB_RESULT, Blob_PutBlock,
        HTTPAPIEX_HANDLE, httpApiExHandle,
        const char*, relativePath,
        unsigned int, blockID,
        BUFFER_HANDLE, blockData,
        SINGLYLINKEDLIST_HANDLE, blockIDList,
        unsigned int*, httpStatus,
        BUFFER_HANDLE, httpResponse)

/**
* @brief  Synchronously sends a put block list request to Azure Storage
*
* @param  httpApiExHandle     The HTTP connection handle
* @param  relativePath        The destination path within the storage
* @param  blockIDList         The list containing the block IDs to report
* @param  httpStatus          A pointer to an out argument receiving the HTTP status (available only when the return value is BLOB_OK)
* @param  httpResponse        A BUFFER_HANDLE that receives the HTTP response from the server (available only when the return value is BLOB_OK)
*/
MOCKABLE_FUNCTION(, BLOB_RESULT, Blob_PutBlockList,
    HTTPAPIEX_HANDLE, httpApiExHandle,
    const char*, relativePath,
    SINGLYLINKEDLIST_HANDLE, blockIDList,
    unsigned int*, httpStatus,
    BUFFER_HANDLE, httpResponse)
#ifdef __cplusplus
}
#endif

#endif /* BLOB_H */
