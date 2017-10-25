// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file blob.h
*	@brief Contains blob APIs needed for File Upload feature of IoTHub client.
*
*	@details IoTHub client needs to upload a byte array by using blob storage API
*			 IoTHub service provides the complete SAS URI to execute a PUT request
*			 that will upload the data.
*			
*/

#ifndef BLOB_H
#define BLOB_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings_types.h"
#include "azure_c_shared_utility/httpapiex.h"

#include "iothub_client_ll.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#include "azure_c_shared_utility/umock_c_prod.h"

#define BLOB_RESULT_VALUES \
    BLOB_OK,               \
    BLOB_ERROR,            \
    BLOB_NOT_IMPLEMENTED,  \
    BLOB_HTTP_ERROR,       \
    BLOB_INVALID_ARG    

DEFINE_ENUM(BLOB_RESULT, BLOB_RESULT_VALUES)

/**
* @brief	Synchronously uploads a byte array to blob storage
*
* @param	SASURI	        The URI to use to upload data
* @param	getDataCallback	A callback to be invoked to acquire the file chunks to be uploaded, as well as to indicate the status of the upload of the previous block.
* @param	context		    Any data provided by the user to serve as context on getDataCallback.
* @param    httpStatus      A pointer to an out argument receiving the HTTP status (available only when the return value is BLOB_OK)
* @param    httpResponse    A BUFFER_HANDLE that receives the HTTP response from the server (available only when the return value is BLOB_OK)
* @param    certificates    A null terminated string containing CA certificates to be used
*
* @return	A @c BLOB_RESULT. BLOB_OK means the blob has been uploaded successfully. Any other value indicates an error
*/
MOCKABLE_FUNCTION(, BLOB_RESULT, Blob_UploadMultipleBlocksFromSasUri, const char*, SASURI, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK, getDataCallback, void*, context, unsigned int*, httpStatus, BUFFER_HANDLE, httpResponse, const char*, certificates)

/**
* @brief  Synchronously uploads a byte array as a new block to blob storage
*
* @param  requestContent      The data to upload
* @param  blockId             The block id (from 00000 to 49999)
* @param  isError             A pointer to an int stating if there was an error during upload. In case of error, is set to 1.
* @param  xml                 The XML file containing the blockId list
* @param  relativePath        The destination path within the storage
* @param  httpApiExHandle     The connection handle
* @param  httpStatus          A pointer to an out argument receiving the HTTP status (available only when the return value is BLOB_OK)
* @param  httpResponse        A BUFFER_HANDLE that receives the HTTP response from the server (available only when the return value is BLOB_OK)
*/
MOCKABLE_FUNCTION(, BLOB_RESULT, Blob_UploadNextBlock, BUFFER_HANDLE, requestContent, unsigned int, blockID, int*, isError, STRING_HANDLE, xml, const char*, relativePath, HTTPAPIEX_HANDLE, httpApiExHandle, unsigned int*, httpStatus, BUFFER_HANDLE, httpResponse)

#ifdef __cplusplus
}
#endif

#endif /* BLOB_H */
