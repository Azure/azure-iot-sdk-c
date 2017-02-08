// Copyright (c) Texas Instruments. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "mico.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"
#include "StringUtils.h"

#include "httpapi.h"
#include "strings.h"
#include "xlogging.h"

static OSStatus onReceivedData( struct _HTTPHeader_t * httpHeader,
                                uint32_t pos,
                                uint8_t *data,
                                size_t len,
                                void * userContext );
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext );

typedef struct _http_context_t
{
    char *content;
    uint64_t content_length;
} http_context_t;

typedef struct HTTP_HANDLE_DATA_TAG
{
    /*working set*/
    int client_fd;
    HTTPHeader_t* httpHeader;
    /*options*/
    unsigned int timeout;
    const char* x509certificate;
    const char* x509privatekey;
} HTTP_HANDLE_DATA;

/*returns NULL if it failed to construct the headers*/
static const char* ConstructHeadersString(HTTP_HEADERS_HANDLE httpHeadersHandle)
{
    char* result;
    size_t headersCount;

    if (HTTPHeaders_GetHeaderCount(httpHeadersHandle, &headersCount) != HTTP_HEADERS_OK)
    {
        result = NULL;
        LogError("HTTPHeaders_GetHeaderCount failed.");
    }
    else
    {
        size_t i;

        /*the total size of all the headers is given by sumof(lengthof(everyheader)+2)*/
        size_t toAlloc = 0;
        for (i = 0; i < headersCount; i++)
        {
            char *temp;
            if (HTTPHeaders_GetHeader(httpHeadersHandle, i, &temp) == HTTP_HEADERS_OK)
            {
                toAlloc += strlen(temp);
                toAlloc += 2;
                free(temp);
            }
            else
            {
                LogError("HTTPHeaders_GetHeader failed");
                break;
            }
        }

        if (i < headersCount)
        {
            result = NULL;
        }
        else
        {
            result = malloc(toAlloc*sizeof(char) + 1 );

            if (result == NULL)
            {
                LogError("unable to malloc");
                /*let it be returned*/
            }
            else
            {
                result[0] = '\0';
                for (i = 0; i < headersCount; i++)
                {
                    char* temp;
                    if (HTTPHeaders_GetHeader(httpHeadersHandle, i, &temp) != HTTP_HEADERS_OK)
                    {
                        LogError("unable to HTTPHeaders_GetHeader");
                        break;
                    }
                    else
                    {
                        (void)strcat(result, temp);
                        (void)strcat(result, "\r\n");
                        free(temp);
                    }
                }

                if (i < headersCount)
                {
                    free(result);
                    result = NULL;
                }
                else
                {
                    /*all is good*/
                }
            }
        }
    }

    return result;
}

HTTPAPI_RESULT HTTPAPI_Init(void)
{
    return (HTTPAPI_OK);
}

void HTTPAPI_Deinit(void)
{
}

HTTP_HANDLE HTTPAPI_CreateConnection(const char* hostName)
{
    OSStatus err;
    HTTP_HANDLE_DATA* result = malloc(sizeof(HTTP_HANDLE_DATA));
    struct in_addr in_addr;
    struct sockaddr_in addr;
    char **pptr = NULL;
    char ipstr[16];
    http_context_t context = { NULL, 0 };
    struct hostent* hostent_content = NULL;

    hostent_content = gethostbyname( hostName );
    require_action_quiet( hostent_content != NULL, exit, err = kNotFoundErr);
    pptr=hostent_content->h_addr_list;
    in_addr.s_addr = *(uint32_t *)(*pptr);
    strcpy( ipstr, inet_ntoa(in_addr));
    LogInfo("HTTP server address: %s, host ip: %s", hostName, ipstr);

    result->httpHeader = HTTPHeaderCreateWithCallback( 1024, onReceivedData, onClearData, &context );
    require_action( result->httpHeader, exit, err = kNoMemoryErr );

    result->client_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    addr.sin_family = AF_INET;
    addr.sin_addr = in_addr;
    addr.sin_port = htons(443);
    err = connect( result->client_fd, (struct sockaddr *)&addr, sizeof(addr) );
    require_noerr_string( err, exit, "connect http server failed" );

    return (HTTP_HANDLE)result;

exit:
    return NULL;
}

void HTTPAPI_CloseConnection(HTTP_HANDLE handle)
{
    HTTP_HANDLE_DATA* handleData = (HTTP_HANDLE_DATA*) handle;

    if (handleData != NULL)
    {
        SocketClose( &handleData->client_fd );
        HTTPHeaderDestory( &handleData->httpHeader );
        free (handleData);
    }
}

HTTPAPI_RESULT HTTPAPI_ExecuteRequest(HTTP_HANDLE handle,
        HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
        HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
        size_t contentLength, unsigned int* statusCode,
        HTTP_HEADERS_HANDLE responseHeadersHandle,
        BUFFER_HANDLE responseContent)
{
    HTTPAPI_RESULT result;
    HTTP_HANDLE_DATA* handleData = (HTTP_HANDLE_DATA*)handle;
    size_t cnt;
    const char* headers2 = NULL;
    const char* method = NULL;

    switch (requestType)
    {
    case HTTPAPI_REQUEST_GET:
        method = "GET";
        break;

    case HTTPAPI_REQUEST_POST:
        method = "POST";
        break;

    case HTTPAPI_REQUEST_PUT:
        method = "PUT";
        break;

    case HTTPAPI_REQUEST_DELETE:
        method = "DELETE";
        break;

    case HTTPAPI_REQUEST_PATCH:
        method = "PATCH";
        break;

    default:
        break;
    }
    if ((handleData == NULL) || (method == NULL) || (relativePath == NULL)
         || (statusCode == NULL) || (responseHeadersHandle == NULL))
    {
        LogError("Invalid arguments: handle=%p, requestType=%d, relativePath=%p, statusCode=%p, responseHeadersHandle=%p",
                 handle, (int)requestType, relativePath, statusCode, responseHeadersHandle);
        result = HTTPAPI_INVALID_ARG;
        goto exit;
    }

    if (HTTPHeaders_GetHeaderCount(httpHeadersHandle, &cnt) != HTTP_HEADERS_OK)
    {
        LogError("Cannot get header count");
        result = HTTPAPI_QUERY_HEADERS_FAILED;
        goto exit;
    }

    /* Send HTTP Request */
    //send( client_fd, query, strlen( query ), 0 );

    /* Send the request line */
    /*ret = HTTPCli_sendRequest(cli, method, relativePath, true);
    if (ret < 0) {
        LogError("HTTPCli_sendRequest failed, ret=%d", ret);
        return (HTTPAPI_SEND_REQUEST_FAILED);
    }*/
    result = HTTPAPI_OK;

exit:
    if (headers2 != NULL)
    {
        free(headers2);
    }
    return result;
}

HTTPAPI_RESULT HTTPAPI_SetOption(HTTP_HANDLE handle, const char* optionName,
        const void* value)
{
    return (HTTPAPI_INVALID_ARG);
}

HTTPAPI_RESULT HTTPAPI_CloneOption(const char* optionName, const void* value,
        const void** savedValue)
{
    return (HTTPAPI_INVALID_ARG);
}

/*one request may receive multi reply*/
static OSStatus onReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData,
                                size_t inLen, void * inUserContext )
{
    OSStatus err = kNoErr;
    http_context_t *context = inUserContext;
    if ( inHeader->chunkedData == false )
    { //Extra data with a content length value
        if ( inPos == 0 && context->content == NULL )
        {
            context->content = calloc( inHeader->contentLength + 1, sizeof(uint8_t) );
            require_action( context->content, exit, err = kNoMemoryErr );
            context->content_length = inHeader->contentLength;

        }
        memcpy( context->content + inPos, inData, inLen );
    }
    else
    {
        //extra data use a chunked data protocol
        LogInfo("This is a chunked data, %d", inLen);
        if ( inPos == 0 )
        {
            context->content = calloc( inHeader->contentLength + 1, sizeof(uint8_t) );
            require_action( context->content, exit, err = kNoMemoryErr );
            context->content_length = inHeader->contentLength;
        }
        else
        {
            context->content_length += inLen;
            context->content = realloc( context->content, context->content_length + 1 );
            require_action( context->content, exit, err = kNoMemoryErr );
        }
        memcpy( context->content + inPos, inData, inLen );
    }

exit:
    return err;
}

/* Called when HTTPHeaderClear is called */
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext )
{
    UNUSED_PARAMETER( inHeader );
    http_context_t *context = inUserContext;
    if ( context->content )
    {
        free( context->content );
        context->content = NULL;
    }
}


