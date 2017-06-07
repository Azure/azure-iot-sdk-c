// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* This is a template file used for porting */

/* This is a template for a TLS IO adapter for a TLS library that directly talks to the sockets
Go through each TODO item in this file and replace the called out function calls with your own TLS library calls.
Please refer to the porting guide for more details.

Make sure that you replace tlsio_template everywhere in this file with your own TLS library name (like tlsio_mytls) */

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/*mico library*/
#include "mico.h"
#include "SocketUtils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "gballoc.h"
#include "tlsio.h"
#include "tlsio_mico.h"
#include "xlogging.h"
#include "crt_abstractions.h"

/* TODO: If more states are needed, simply add them to the enum. Most of the implementation will not require additional states.
   Example of another state would be TLSIO_STATE_RENEGOTIATING, etc. */
typedef enum TLSIO_STATE_TAG
{
    TLSIO_STATE_NOT_OPEN,
    TLSIO_STATE_OPEN,
    TLSIO_STATE_ERROR
} TLSIO_STATE;

typedef struct TLS_MICO_INSTANCE_TAG
{
    ON_IO_OPEN_COMPLETE on_io_open_complete;
    void* on_io_open_complete_context;
    ON_BYTES_RECEIVED on_bytes_received;
    void* on_bytes_received_context;
    ON_IO_ERROR on_io_error;
    void* on_io_error_context;
    TLSIO_STATE tlsio_state;
    char* hostname;
    int port;
    char* certificates;
	
    int client_socket_id;
    mico_ssl_t client_ssl;

} TLS_MICO_INSTANCE;


typedef struct _http_context_t
{
    char *content;
    uint64_t content_length;
} http_context_t;


static int mico_open_secure_connection(TLS_MICO_INSTANCE* tls_io_instance)
{
    OSStatus err = kNoErr;
    int client_fd = -1;
    int ssl_errno = 0;
    mico_ssl_t client_ssl = NULL;
    char ipstr[16];
    char name[20];
    struct sockaddr_in addr;
    struct hostent* hostent_content = NULL;
    char **pptr = NULL;
    struct in_addr in_addr;

    // Set TLS 1.1 for Azure IoT hub
    ssl_set_client_version(TLS_V1_1_MODE);
    // Get host info
    hostent_content = gethostbyname(tls_io_instance->hostname);
    require_action_quiet(hostent_content != NULL, exit, err = kNotFoundErr);
    pptr = hostent_content->h_addr_list;
    strcpy(name,hostent_content->h_name);
    LogInfo("name is %s",name);
    in_addr.s_addr = *(uint32_t *)(*pptr);
    strcpy( ipstr, inet_ntoa(in_addr));
    LogInfo("HTTP server address: host:%s, ip: %s", tls_io_instance->hostname, ipstr);

    // Open connection
    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr.sin_family = AF_INET;
    addr.sin_addr = in_addr;
    addr.sin_port = htons(tls_io_instance->port);
    err = connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));
    require_noerr_string(err, exit, "connect http server failed");
    // Open SSL connection
    if (tls_io_instance->certificates != NULL)
    {
        client_ssl = ssl_connect(client_fd, strlen(tls_io_instance->certificates), tls_io_instance->certificates, &ssl_errno);
    }
    else
    {
        client_ssl = ssl_connect(client_fd, 0, NULL, &ssl_errno);
    }
    if (client_ssl == NULL)
    {
        goto exit;
    }

    // Done
    tls_io_instance->client_socket_id = client_fd;
    tls_io_instance->client_ssl = client_ssl;
    return 0;

exit:
    LogError("Exit: Client exit with err = %d, fd: %d, ssl: %d", err, client_fd, ssl_errno);
    if (client_ssl != NULL)
    {
        ssl_close(client_ssl);
    }
    if (client_fd != -1)
    {
        SocketClose(&client_fd);
    }
    return err;
}


static void mico_close_secure_connection(TLS_MICO_INSTANCE* tls_io_instance)
{
    if (tls_io_instance->client_ssl != NULL)
    {
        ssl_close(tls_io_instance->client_ssl);
        tls_io_instance->client_ssl = NULL;
    }
    if (tls_io_instance->client_socket_id != -1)
    {
        SocketClose(&tls_io_instance->client_socket_id);
        tls_io_instance->client_socket_id = -1;
    }
}


static int mico_secure_received_bytes(TLS_MICO_INSTANCE* tls_io_instance)
{
    int received = 0;
    fd_set readfds;
    struct timeval timeout;
    unsigned char buffer[64];

    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(tls_io_instance->client_socket_id, &readfds);
    select(tls_io_instance->client_socket_id + 1, &readfds, NULL, NULL, &timeout);
    if (FD_ISSET(tls_io_instance->client_socket_id, &readfds))
    {
        received = ssl_recv(tls_io_instance->client_ssl, buffer, sizeof(buffer));
        if (received > 0)
        {
            tls_io_instance->on_bytes_received(tls_io_instance->on_bytes_received_context, buffer, received);
        }
        else
        {
            LogError("Error received bytes");
            tls_io_instance->tlsio_state = TLSIO_STATE_ERROR;
            tls_io_instance->on_io_error(tls_io_instance->on_io_error_context);
        }
    }
    return received;
}


static int tlsio_mico_close(CONCRETE_IO_HANDLE tls_io, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* on_io_close_complete_context);

/*this function will clone an option given by name and value*/
static void* tlsio_mico_clone_option(const char* name, const void* value)
{
    void* result;

    if((name == NULL) || (value == NULL))
    {
        LogError("invalid parameter detected: const char* name=%p, const void* value=%p", name, value);
        result = NULL;
    }
    else
    {
		/* TODO: This only handles TrustedCerts, if you need to handle more options specific to your TLS library, fill the code in here

        if (strcmp(name, "...my_option...") == 0)
        {
			// ... copy the option and assign it to result to be returned.
		}
		else
		*/
        if (strcmp(name, "TrustedCerts") == 0)
        {
            if(mallocAndStrcpy_s((char**)&result, value) != 0)
            {
                LogError("unable to mallocAndStrcpy_s TrustedCerts value");
                result = NULL;
            }
            else
            {
                /*return as is*/
            }
        }
        else
        {
            LogError("not handled option : %s", name);
            result = NULL;
        }
    }
    return result;
}

/*this function destroys an option previously created*/
static void tlsio_mico_destroy_option(const char* name, const void* value)
{
    /*since all options for this layer are actually string copies., disposing of one is just calling free*/

    if ((name == NULL) || (value == NULL))
    {
        LogError("invalid parameter detected: const char* name=%p, const void* value=%p", name, value);
    }
	else
	{
		/* TODO: This only handles TrustedCerts, if you need to handle more options specific to your TLS library, fill the code in here

        if (strcmp(name, "...my_option...") == 0)
        {
			// ... free any resources for the option
		}
		else
		*/
		if (strcmp(name, "TrustedCerts") == 0)
		{
			free((void*)value);
		}
        else
        {
            LogError("not handled option : %s", name);
        }
    }
}

static CONCRETE_IO_HANDLE tlsio_mico_create(void* io_create_parameters)
{
    TLS_MICO_INSTANCE* result;

	/* check whether the argument is good */
    if (io_create_parameters == NULL)
    {
        result = NULL;
        LogError("NULL tls_io_config.");
    }
    else
    {
        TLSIO_CONFIG* tls_io_config = io_create_parameters;

		/* check if the hostname is good */
        if (tls_io_config->hostname == NULL)
        {
            result = NULL;
            LogError("NULL hostname in the TLS IO configuration.");
        }
        else
        {
			/* allocate */
            result = malloc(sizeof(TLS_MICO_INSTANCE));
            if (result == NULL)
            {
                LogError("Failed allocating TLSIO instance.");
            }
            else
            {
                /* copy the hostname for later use in open */
                if (mallocAndStrcpy_s(&result->hostname, tls_io_config->hostname) != 0)
                {
                    LogError("Failed to copy the hostname.");
                    free(result);
                    result = NULL;
                }
                else
                {
					/* copy port and initialize all the callback data */
                    result->port = tls_io_config->port;
                    result->certificates = NULL;
                    result->on_bytes_received = NULL;
                    result->on_bytes_received_context = NULL;
                    result->on_io_open_complete = NULL;
                    result->on_io_open_complete_context = NULL;
                    result->on_io_error = NULL;
                    result->on_io_error_context = NULL;
                    result->tlsio_state = TLSIO_STATE_NOT_OPEN;
                    result->client_socket_id = -1;
                    result->client_ssl = NULL;
                }
            }
        }
    }

    return result;
}

static void tlsio_mico_destroy(CONCRETE_IO_HANDLE tls_io)
{
    if (tls_io == NULL)
    {
        LogError("NULL tls_io.");
    }
    else
    {
        TLS_MICO_INSTANCE* tls_io_instance = (TLS_MICO_INSTANCE*)tls_io;

        /* force a close when destroying */
        tlsio_mico_close(tls_io, NULL, NULL);

        if (tls_io_instance->certificates != NULL)
        {
            free(tls_io_instance->certificates);
        }
        free(tls_io_instance->hostname);
        free(tls_io);
    }
}

static int tlsio_mico_open(CONCRETE_IO_HANDLE tls_io, ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_context, ON_BYTES_RECEIVED on_bytes_received, void* on_bytes_received_context, ON_IO_ERROR on_io_error, void* on_io_error_context)
{
    int result;

    /* check arguments */
    if ((tls_io == NULL) ||
        (on_io_open_complete == NULL) ||
        (on_bytes_received == NULL) ||
        (on_io_error == NULL))
    {
        result = __LINE__;
        LogError("NULL tls_io.");
    }
    else
    {
        TLS_MICO_INSTANCE* tls_io_instance = (TLS_MICO_INSTANCE*)tls_io;

        if (tls_io_instance->tlsio_state != TLSIO_STATE_NOT_OPEN)
        {
            result = __LINE__;
            LogError("Invalid tlsio_state. Expected state is TLSIO_STATE_NOT_OPEN.");
        }
        else
        {
            tls_io_instance->on_bytes_received = on_bytes_received;
            tls_io_instance->on_bytes_received_context = on_bytes_received_context;
            tls_io_instance->on_io_open_complete = on_io_open_complete;
            tls_io_instance->on_io_error = on_io_error;
            tls_io_instance->on_io_error_context = on_io_error_context;

            if (mico_open_secure_connection(tls_io_instance) != 0)
            {
                result = __LINE__;
            }
            else
            {
                /* setting the state to OPEN here is the way to go for a blocking connect. */
                tls_io_instance->tlsio_state = TLSIO_STATE_OPEN;
                tls_io_instance->on_io_open_complete(on_io_open_complete_context, IO_OPEN_OK);

                result = 0;
            }
        }
    }

    return result;
}

static int tlsio_mico_close(CONCRETE_IO_HANDLE tls_io, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* on_io_close_complete_context)
{
    int result = 0;

    if (tls_io == NULL)
    {
        result = __LINE__;
        LogError("NULL tls_io.");
    }
    else
    {
        TLS_MICO_INSTANCE* tls_io_instance = (TLS_MICO_INSTANCE*)tls_io;

        /* If we're not open do not try to close */
        if (tls_io_instance->tlsio_state == TLSIO_STATE_NOT_OPEN)
        {
            result = __LINE__;
            LogError("Invalid tlsio_state. Expected state is TLSIO_STATE_NOT_OPEN or TLSIO_STATE_CLOSING.");
        }
        else
        {
            /* TODO: Here you should call the function for your TLS library that shuts down the connection. */
            mico_close_secure_connection(tls_io_instance);

            tls_io_instance->tlsio_state = TLSIO_STATE_NOT_OPEN;

            if (on_io_close_complete != NULL)
            {
                on_io_close_complete(on_io_close_complete_context);
            }
            result = 0;
        }
    }

    return result;
}

static int tlsio_mico_send(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* on_send_complete_context)
{
    int result;

    if ((tls_io == NULL) ||
        (buffer == NULL) ||
        (size == 0))
    {
        result = __LINE__;
        LogError("NULL tls_io.");
    }
    else
    {
        TLS_MICO_INSTANCE* tls_io_instance = (TLS_MICO_INSTANCE*)tls_io;

		/* If we are not open, do not try to send */
        if (tls_io_instance->tlsio_state != TLSIO_STATE_OPEN)
        {
            result = __LINE__;
            LogError("Invalid tlsio_state. Expected state is TLSIO_STATE_OPEN.");
        }
        else
        {
            int sent = ssl_send(tls_io_instance->client_ssl, (void *)buffer, size);
            if (sent == -1)
            {
                result = __LINE__;
                LogError("TLS library failed to encrypt bytes.");
            }
            else
            {
                LogInfo("TLS library sending encrypt bytes.'%d'", sent);

				/* TODO: this assumes that all writes are blocking and no buffering is needed. If buffering is needed you would have to implement additional code here. */
                if (on_send_complete != NULL)
                {
                    on_send_complete(on_send_complete_context, IO_SEND_OK);
                }

                result = 0;
            }
        }
    }

    return result;
}

static void tlsio_mico_dowork(CONCRETE_IO_HANDLE tls_io)
{
	/* check arguments */
    if (tls_io == NULL)
    {
        LogError("NULL tls_io.");
    }
    else
    {
        TLS_MICO_INSTANCE* tls_io_instance = (TLS_MICO_INSTANCE*)tls_io;

		/* only perform work if we are not in error */
        if ((tls_io_instance->tlsio_state != TLSIO_STATE_NOT_OPEN) &&
            (tls_io_instance->tlsio_state != TLSIO_STATE_ERROR))
        {
            mico_secure_received_bytes(tls_io_instance);
        }
    }
}

static int tlsio_mico_setoption(CONCRETE_IO_HANDLE tls_io, const char* optionName, const void* value)
{
    int result;

	/* check arguments */
    if ((tls_io == NULL) || (optionName == NULL))
    {
        LogError("NULL tls_io");
        result = __LINE__;
    }
    else
    {
        TLS_MICO_INSTANCE* tls_io_instance = (TLS_MICO_INSTANCE*)tls_io;

		/* TODO: This only handles TrustedCerts, if you need to handle more options specific to your TLS library, fill the code in here

        if (strcmp(name, "...my_option...") == 0)
        {
			// ... store the option value as needed. Make sure that you make a copy when storing.
		}
		else
		*/
        if (strcmp("TrustedCerts", optionName) == 0)
        {
            const char* cert = (const char*)value;

            if (tls_io_instance->certificates != NULL)
            {
                // Free the memory if it has been previously allocated
                free(tls_io_instance->certificates);
                tls_io_instance->certificates = NULL;
            }

            if (cert == NULL)
            {
                result = 0;
            }
            else
            {
                // Store the certificate
                if (mallocAndStrcpy_s(&tls_io_instance->certificates, cert) != 0)
                {
                    LogError("Error allocating memory for certificates");
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                }
            }
        }
        else
        {
            LogError("Unrecognized option");
            result = __LINE__;
        }
    }

    return result;
}

static OPTIONHANDLER_HANDLE tlsio_mico_retrieve_options(CONCRETE_IO_HANDLE handle)
{
    OPTIONHANDLER_HANDLE result;

    /* Codes_SRS_tlsio_template_01_064: [ If parameter handle is `NULL` then `tlsio_template_retrieve_options` shall fail and return NULL. ]*/
    if (handle == NULL)
    {
        LogError("invalid parameter detected: CONCRETE_IO_HANDLE handle=%p", handle);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_tlsio_template_01_065: [ `tlsio_template_retrieve_options` shall produce an OPTIONHANDLER_HANDLE. ]*/
        result = OptionHandler_Create(tlsio_mico_clone_option, tlsio_mico_destroy_option, tlsio_mico_setoption);
        if (result == NULL)
        {
            /* Codes_SRS_tlsio_template_01_068: [ If producing the OPTIONHANDLER_HANDLE fails then tlsio_template_retrieve_options shall fail and return NULL. ]*/
            LogError("unable to OptionHandler_Create");
            /*return as is*/
        }
        else
        {
            TLS_MICO_INSTANCE* tls_io_instance = (TLS_MICO_INSTANCE*)handle;

            /* Codes_SRS_tlsio_template_01_066: [ `tlsio_template_retrieve_options` shall add to it the options: ]*/
            if (
                (tls_io_instance->certificates != NULL) &&
				/* TODO: This only handles TrustedCerts, if you need to handle more options specific to your TLS library, fill the code in here

                (OptionHandler_AddOption(result, "my_option", tls_io_instance->...) != 0) ||
				*/
                (OptionHandler_AddOption(result, "TrustedCerts", tls_io_instance->certificates) != 0)
                )
            {
                LogError("unable to save TrustedCerts option");
                OptionHandler_Destroy(result);
                result = NULL;
            }
            else
            {
                /*all is fine, all interesting options have been saved*/
                /*return as is*/
            }
        }
    }
    return result;
}

static const IO_INTERFACE_DESCRIPTION tlsio_mico_interface_description =
{
    tlsio_mico_retrieve_options,
    tlsio_mico_create,
    tlsio_mico_destroy,
    tlsio_mico_open,
    tlsio_mico_close,
    tlsio_mico_send,
    tlsio_mico_dowork,
    tlsio_mico_setoption
};

/* This simply returns the concrete implementations for the TLS adapter */
const IO_INTERFACE_DESCRIPTION* tlsio_mico_get_interface_description(void)
{
    return &tlsio_mico_interface_description;
}
