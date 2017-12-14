#Blob Requirements

##Overview

References
[Operations on Block Blobs](https://msdn.microsoft.com/en-us/library/azure/ee691974.aspx)
[Put Block](https://msdn.microsoft.com/en-us/library/azure/dd135726.aspx)
[Put Block List](https://msdn.microsoft.com/en-us/library/azure/dd179467.aspx)

##Exposed API
```c
#define BLOB_RESULT_VALUES \
    BLOB_OK,               \
    BLOB_ERROR,            \
    BLOB_NOT_IMPLEMENTED,  \
    BLOB_HTTP_ERROR,       \
    BLOB_INVALID_ARG,      \
    BLOB_ABORTED

DEFINE_ENUM(BLOB_RESULT, BLOB_RESULT_VALUES)
    
/**
* @brief  Synchronously uploads a byte array to blob storage
*
* @param  SASURI            The URI to use to upload data
* @param  getDataCallback   A callback to be invoked to acquire the file chunks to be uploaded, as well as to indicate the status of the upload of the previous block.
* @param  context           Any data provided by the user to serve as context on getDataCallback.
* @param  httpStatus        A pointer to an out argument receiving the HTTP status (available only when the return value is BLOB_OK)
* @param  httpResponse      A BUFFER_HANDLE that receives the HTTP response from the server (available only when the return value is BLOB_OK)
* @param  certificates      A null terminated string containing CA certificates to be used
*
* @return	A @c BLOB_RESULT. BLOB_OK means the blob has been uploaded successfully. Any other value indicates an error
*/
extern BLOB_RESULT Blob_UploadMultipleBlocksFromSasUri(const char* SASURI, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context, unsigned int* httpStatus, BUFFER_HANDLE httpResponse, const char* certificates);
```

##Blob_UploadMultipleBlocksFromSasUri 
```c
BLOB_RESULT Blob_UploadMultipleBlocksFromSasUri(const char* SASURI, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context, unsigned int* httpStatus, BUFFER_HANDLE httpResponse, const char* certificates)

/**
*  @brief           Callback invoked to request the chunks of data to be uploaded.
*  @param result    The result of the upload of the previous block of data provided by the user.
*  @param data      Next block of data to be uploaded, to be provided by the user when this callback is invoked.
*  @param size      Size of the data parameter.
*  @param context   User context provided.
*  @remarks         If a NULL is provided for parameter "data" and/or zero is provided for "size", the user indicates to the client that the complete file has been uploaded.
*                   In such case this callback will be invoked only once more to indicate the status of the final block upload.
*                   If result is not FILE_UPLOAD_OK, the download is cancelled and this callback stops being invoked.
*                   When this callback is called for the last time, no data or size is expected, so data and size are set to NULL
*/
typedef IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_VALUES(*IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context);
```
`Blob_UploadMultipleBlocksFromSasUri` uploads as a Blob the blocks of data repetitively provided by `getDataCallback` by using HTTPAPI_EX module.


**SRS_BLOB_02_001: [** If `SASURI` is NULL then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. **]**
**SRS_BLOB_02_002: [** If `getDataCallback` is NULL then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. **]**
**SRS_BLOB_02_034: [** If size is bigger than 50000\*4\*1024\*1024 then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. **]**
**SRS_BLOB_02_005: [** If the hostname cannot be determined, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. **]**
**SRS_BLOB_02_016: [** If the hostname copy cannot be made then then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return ``BLOB_INVALID_ARG`` **]**
**SRS_BLOB_02_007: [** If `HTTPAPIEX_Create` fails then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_ERROR`. **]**


Design considerations: Blob_UploadMultipleBlocksFromSasUri will obtain blocks of data from `getDataCallback`.
These blocks must have a size equal or smaller than 4MB.
These blocks have the IDs starting from 000000 and ending with 049999 (potentially). 
    Note: the URL encoding of the BASE64 of these numbers is the same as the BASE64 representation (therefore no URL encoding needed)
Blocks are uploaded serially by "Put Block" REST API. After all the blocks have been uploaded, a "Put Block List" is executed.

**SRS_BLOB_02_017: [** `Blob_UploadMultipleBlocksFromSasUri` shall copy from `SASURI` the hostname to a new const char\* **]**

**SRS_BLOB_02_018: [** `Blob_UploadMultipleBlocksFromSasUri` shall create a new `HTTPAPI_EX_HANDLE` by calling `HTTPAPIEX_Create` passing the hostname. **]**

**SRS_BLOB_02_037: [** If `certificates` is non-NULL then `Blob_UploadMultipleBlocksFromSasUri` shall pass `certificates` to `HTTPAPI_EX_HANDLE` by calling `HTTPAPIEX_SetOption` with the option name "TrustedCerts". **]**

**SRS_BLOB_02_038: [** If `HTTPAPIEX_SetOption` fails then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_ERROR`. **]**

**SRS_BLOB_02_019: [** `Blob_UploadMultipleBlocksFromSasUri` shall compute the base relative path of the request from the `SASURI` parameter. **]**
 
**SRS_BLOB_02_021: [** For every block returned by `getDataCallback` the following operations shall happen: **]**
  
1. **SRS_BLOB_99_001: [** If the size of the block returned by `getDataCallback` is bigger than 4MB, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. **]**
2. **SRS_BLOB_99_002: [** If the size of the block returned by `getDataCallback` is 0 or if the data is NULL, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop. **]**
3. **SRS_BLOB_99_003: [** If `getDataCallback` returns more than 50000 blocks, then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_INVALID_ARG`. **]**
4. **SRS_BLOB_99_004: [** If `getDataCallback` returns `IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_ABORT`, then `Blob_UploadMultipleBlocksFromSasUri` shall exit the loop and return `BLOB_ABORTED`. **]**
5. **SRS_BLOB_02_020: [** `Blob_UploadMultipleBlocksFromSasUri` shall construct a BASE64 encoded string from the block ID (000000... 049999) **]**
6. **SRS_BLOB_02_022: [** `Blob_UploadMultipleBlocksFromSasUri` shall construct a new relativePath from following string: base relativePath + "&comp=block&blockid=BASE64 encoded string of blockId" **]**
7. **SRS_BLOB_02_023: [** `Blob_UploadMultipleBlocksFromSasUri` shall create a BUFFER_HANDLE from `source` and `size` parameters. **]**
8. **SRS_BLOB_02_024: [** `Blob_UploadMultipleBlocksFromSasUri` shall call `HTTPAPIEX_ExecuteRequest` with a PUT operation, passing `httpStatus` and `httpResponse`. **]**
9. **SRS_BLOB_02_025: [** If `HTTPAPIEX_ExecuteRequest` fails then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_HTTP_ERROR`. **]**
10. **SRS_BLOB_02_026: [** Otherwise, if HTTP response code is >=300 then `Blob_UploadMultipleBlocksFromSasUri` shall succeed and return `BLOB_OK`. **]**
11. **SRS_BLOB_02_027: [** Otherwise `Blob_UploadMultipleBlocksFromSasUri` shall continue execution. **]**

**SRS_BLOB_02_028: [** `Blob_UploadMultipleBlocksFromSasUri` shall construct an XML string with the following content: **]**
```xml
<?xml version="1.0" encoding="utf-8"?>
<BlockList>
  <Latest>BASE64 encoding of the first 4MB block ID (MDAwMDAw)</Latest>
  <Latest>BASE64 encoding of the second 4MB block ID (MDAwMDAx)</Latest>
  ...
</BlockList>
```

**SRS_BLOB_02_029: [** `Blob_UploadMultipleBlocksFromSasUri` shall construct a new relativePath from following string: base relativePath + "&comp=blocklist" **]**
**SRS_BLOB_02_030: [** `Blob_UploadMultipleBlocksFromSasUri` shall call `HTTPAPIEX_ExecuteRequest` with a PUT operation, passing the new relativePath, `httpStatus` and `httpResponse` and the XML string as content. **]**
**SRS_BLOB_02_031: [** If `HTTPAPIEX_ExecuteRequest` fails then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_HTTP_ERROR`. **]**
**SRS_BLOB_02_033: [** If any previous operation that doesn't have an explicit failure description fails then `Blob_UploadMultipleBlocksFromSasUri` shall fail and return `BLOB_ERROR` **]**  
**SRS_BLOB_02_032: [** Otherwise, `Blob_UploadMultipleBlocksFromSasUri` shall succeed and return `BLOB_OK`. **]**