// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "gballoc.h"
#include "tlsio_arduino.h"
#include "sslClient_arduino.h"
#include "socketio.h"
#include "xlogging.h"
#include "platform_mico.h"
#include "threadapi.h"
#include "optionhandler.h"

/* Codes_SRS_TLSIO_ARDUINO_21_001: [ The tlsio_arduino shall implement and export all the Concrete functions in the VTable IO_INTERFACE_DESCRIPTION defined in the `xio.h`. ]*/
/* Codes_SRS_TLSIO_ARDUINO_21_002: [ The tlsio_arduino shall report the open operation status using the IO_OPEN_RESULT enumerator defined in the `xio.h`.]*/
/* Codes_SRS_TLSIO_ARDUINO_21_003: [ The tlsio_arduino shall report the send operation status using the IO_SEND_RESULT enumerator defined in the `xio.h`. ]*/
/* Codes_SRS_TLSIO_ARDUINO_21_004: [ The tlsio_arduino shall call the callbacks functions defined in the `xio.h`. ]*/
#include "azure_c_shared_utility/xio.h"

/* Codes_SRS_TLSIO_ARDUINO_21_005: [ The tlsio_arduino shall received the connection information using the TLSIO_CONFIG structure defined in `tlsio.h`. ]*/
#include "azure_c_shared_utility/tlsio.h"


/* Codes_SRS_TLSIO_ARDUINO_21_001: [ The tlsio_arduino shall implement and export all the Concrete functions in the VTable IO_INTERFACE_DESCRIPTION defined in the `xio.h`. ]*/
CONCRETE_IO_HANDLE tlsio_arduino_create(void* io_create_parameters);
void tlsio_arduino_destroy(CONCRETE_IO_HANDLE tls_io);
int tlsio_arduino_open(CONCRETE_IO_HANDLE tls_io, ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_context, ON_BYTES_RECEIVED on_bytes_received, void* on_bytes_received_context, ON_IO_ERROR on_io_error, void* on_io_error_context);
int tlsio_arduino_close(CONCRETE_IO_HANDLE tls_io, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context);
int tlsio_arduino_send(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context);
void tlsio_arduino_dowork(CONCRETE_IO_HANDLE tls_io);
int tlsio_arduino_setoption(CONCRETE_IO_HANDLE tls_io, const char* optionName, const void* value);
OPTIONHANDLER_HANDLE tlsio_arduino_retrieveoptions(CONCRETE_IO_HANDLE tls_io);

static const IO_INTERFACE_DESCRIPTION tlsio_handle_interface_description =
{
    tlsio_arduino_retrieveoptions,
    tlsio_arduino_create,
    tlsio_arduino_destroy,
    tlsio_arduino_open,
    tlsio_arduino_close,
    tlsio_arduino_send,
    tlsio_arduino_dowork,
    tlsio_arduino_setoption
};

/* Codes_SRS_TLSIO_ARDUINO_21_027: [ The tlsio_arduino_open shall set the tlsio to try to open the connection for 10 times before assuming that connection failed. ]*/
#define MAX_TLS_OPENING_RETRY  10
/* Codes_SRS_TLSIO_ARDUINO_21_044: [ The tlsio_arduino_close shall set the tlsio to try to close the connection for 10 times before assuming that close connection failed. ]*/
#define MAX_TLS_CLOSING_RETRY  10
#define RECEIVE_BUFFER_SIZE    128

#define CallErrorCallback() do { if (tlsio_instance->on_io_error != NULL) (void)tlsio_instance->on_io_error(tlsio_instance->on_io_error_context); } while(0)
#define CallOpenCallback(status) do { if (tlsio_instance->on_io_open_complete != NULL) (void)tlsio_instance->on_io_open_complete(tlsio_instance->on_io_open_complete_context, status); } while(0)
#define CallCloseCallback() do { if (tlsio_instance->on_io_close_complete != NULL) (void)tlsio_instance->on_io_close_complete(tlsio_instance->on_io_close_complete_context); } while(0)


typedef struct ArduinoTLS_tag
{
    ON_IO_OPEN_COMPLETE on_io_open_complete;
    void* on_io_open_complete_context;

    ON_BYTES_RECEIVED on_bytes_received;
    void* on_bytes_received_context;

    ON_IO_ERROR on_io_error;
    void* on_io_error_context;

    ON_IO_CLOSE_COMPLETE on_io_close_complete;
    void* on_io_close_complete_context;

    uint32_t remote_addr;
    uint16_t port;
    TLSIO_ARDUINO_STATE state;
    int countTry;
} ArduinoTLS;

/* Codes_SRS_TLSIO_ARDUINO_21_008: [ The tlsio_arduino_get_interface_description shall return the VTable IO_INTERFACE_DESCRIPTION. ]*/
const IO_INTERFACE_DESCRIPTION* tlsio_arduino_get_interface_description(void)
{
    return &tlsio_handle_interface_description;
}


/* Codes_SRS_TLSIO_ARDUINO_21_020: [ If tlsio_arduino_create get success to create the tlsio instance, it shall set the tlsio state as TLSIO_ARDUINO_STATE_CLOSED. ]*/
TLSIO_ARDUINO_STATE tlsio_arduino_get_state(CONCRETE_IO_HANDLE tlsio_handle)
{
    TLSIO_ARDUINO_STATE result;
    ArduinoTLS* tlsio_instance = (ArduinoTLS*)tlsio_handle;

    if (tlsio_handle == NULL)
        result = TLSIO_ARDUINO_STATE_NULL;
    else
        result = tlsio_instance->state;

    return result;
}


/* Codes_SRS_TLSIO_ARDUINO_21_005: [ The tlsio_arduino shall received the connection information using the TLSIO_CONFIG structure defined in `tlsio.h`. ]*/
/* Codes_SRS_TLSIO_ARDUINO_21_009: [ The tlsio_arduino_create shall create a new instance of the tlsio for Arduino. ]*/
/* Codes_SRS_TLSIO_ARDUINO_21_017: [ The tlsio_arduino_create shall receive the connection configuration (TLSIO_CONFIG). ]*/
CONCRETE_IO_HANDLE tlsio_arduino_create(void* io_create_parameters)
{
    ArduinoTLS* tlsio_instance;
    if (io_create_parameters == NULL)
    {
        LogError("Invalid TLS parameters.");
        tlsio_instance = NULL;
    }
    else
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_011: [ The tlsio_arduino_create shall allocate memory to control the tlsio instance. ]*/
        tlsio_instance = (ArduinoTLS*)malloc(sizeof(ArduinoTLS));
        if (tlsio_instance == NULL)
        {
            /* Codes_SRS_TLSIO_ARDUINO_21_012: [ If there is not enough memory to control the tlsio, the tlsio_arduino_create shall return NULL as the handle. ]*/
            LogError("There is not enough memory to create the TLS instance.");
            /* return as is */
        }
        else
        {
            /* Codes_SRS_TLSIO_ARDUINO_21_005: [ The tlsio_arduino shall received the connection information using the TLSIO_CONFIG structure defined in `tlsio.h`. ]*/
            /* Codes_SRS_TLSIO_ARDUINO_21_017: [ The tlsio_arduino_create shall receive the connection configuration (TLSIO_CONFIG). ]*/
            TLSIO_CONFIG* tlsio_config = (TLSIO_CONFIG*)io_create_parameters;

            /* Codes_SRS_TLSIO_ARDUINO_21_015: [ The tlsio_arduino_create shall set 10 seconds for the sslClient timeout. ]*/
            sslClient_setTimeout(10000);

            /* Codes_SRS_TLSIO_ARDUINO_21_016: [ The tlsio_arduino_create shall initialize all callback pointers as NULL. ]*/
            tlsio_instance->on_io_open_complete = NULL;
            tlsio_instance->on_io_open_complete_context = NULL;
            tlsio_instance->on_bytes_received = NULL;
            tlsio_instance->on_bytes_received_context = NULL;
            tlsio_instance->on_io_error = NULL;
            tlsio_instance->on_io_error_context = NULL;
            tlsio_instance->on_io_close_complete = NULL;
            tlsio_instance->on_io_close_complete_context = NULL;

            /* Codes_SRS_TLSIO_ARDUINO_21_020: [ If tlsio_arduino_create get success to create the tlsio instance, it shall set the tlsio state as TLSIO_ARDUINO_STATE_CLOSED. ]*/
            tlsio_instance->state = TLSIO_ARDUINO_STATE_CLOSED;

            /* Codes_SRS_TLSIO_ARDUINO_21_018: [ The tlsio_arduino_create shall convert the provide hostName to an IP address. ]*/
            if (sslClient_hostByName(tlsio_config->hostname, &(tlsio_instance->remote_addr)))
            {
                tlsio_instance->port = (uint16_t)tlsio_config->port;
            }
            else
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_019: [ If the WiFi cannot find the IP for the hostName, the tlsio_arduino_create shall destroy the sslClient and tlsio instances and return NULL as the handle. ]*/
                LogError("Host %s not found.", tlsio_config->hostname);
                free(tlsio_instance);
                tlsio_instance = NULL;
            }
        }
    }

    /* Codes_SRS_TLSIO_ARDUINO_21_010: [ The tlsio_arduino_create shall return a non-NULL handle on success. ]*/
    return (CONCRETE_IO_HANDLE)tlsio_instance;
}

/* Codes_SRS_TLSIO_ARDUINO_21_021: [ The tlsio_arduino_destroy shall destroy a created instance of the tlsio for Arduino identified by the CONCRETE_IO_HANDLE. ]*/
void tlsio_arduino_destroy(CONCRETE_IO_HANDLE tlsio_handle)
{
    ArduinoTLS* tlsio_instance = (ArduinoTLS*)tlsio_handle;

    if (tlsio_handle == NULL)
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_024: [ If the tlsio_handle is NULL, the tlsio_arduino_destroy shall not do anything. ]*/
        LogError("NULL TLS handle.");
    }
    else
    {
        if ((tlsio_instance->state == TLSIO_ARDUINO_STATE_OPENING) ||
            (tlsio_instance->state == TLSIO_ARDUINO_STATE_OPEN) ||
            (tlsio_instance->state == TLSIO_ARDUINO_STATE_CLOSING))
        {
            /* Codes_SRS_TLSIO_ARDUINO_21_026: [ If the tlsio state is TLSIO_ARDUINO_STATE_OPENING, TLSIO_ARDUINO_STATE_OPEN, or TLSIO_ARDUINO_STATE_CLOSING, the tlsio_arduino_destroy shall close destroy the tlsio, but log an error. ]*/
            LogError("TLS destroyed with a SSL connection still active.");
        }

        /* Codes_SRS_TLSIO_ARDUINO_21_022: [ The tlsio_arduino_destroy shall free the memory allocated for tlsio_instance. ]*/
        free(tlsio_instance);
    }
}

/* Codes_SRS_TLSIO_ARDUINO_21_026: [ The tlsio_arduino_open shall star the process to open the ssl connection with the host provided in the tlsio_arduino_create. ]*/
int tlsio_arduino_open(
    CONCRETE_IO_HANDLE tlsio_handle,
    ON_IO_OPEN_COMPLETE on_io_open_complete,
    void* on_io_open_complete_context,
    ON_BYTES_RECEIVED on_bytes_received,
    void* on_bytes_received_context,
    ON_IO_ERROR on_io_error,
    void* on_io_error_context)
{
    int result;
    ArduinoTLS* tlsio_instance = (ArduinoTLS*)tlsio_handle;

    if (tlsio_handle == NULL)
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_036: [ If the tlsio_handle is NULL, the tlsio_arduino_open shall not do anything, and return _LINE_. ]*/
        LogError("NULL TLS handle.");
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_004: [ The tlsio_arduino shall call the callbacks functions defined in the `xio.h`. ]*/
        /* Codes_SRS_TLSIO_ARDUINO_21_006: [ The tlsio_arduino shall return the status of all async operations using the callbacks. ]*/
        /* Codes_SRS_TLSIO_ARDUINO_21_007: [ If the callback function is set as NULL. The tlsio_arduino shall not call anything. ]*/
        /* Codes_SRS_TLSIO_ARDUINO_21_028: [ The tlsio_arduino_open shall store the provided on_io_open_complete callback function address. ]*/
        tlsio_instance->on_io_open_complete = on_io_open_complete;
        /* Codes_SRS_TLSIO_ARDUINO_21_029: [ The tlsio_arduino_open shall store the provided on_io_open_complete_context handle. ]*/
        tlsio_instance->on_io_open_complete_context = on_io_open_complete_context;

        /* Codes_SRS_TLSIO_ARDUINO_21_030: [ The tlsio_arduino_open shall store the provided on_bytes_received callback function address. ]*/
        tlsio_instance->on_bytes_received = on_bytes_received;
        /* Codes_SRS_TLSIO_ARDUINO_21_031: [ The tlsio_arduino_open shall store the provided on_bytes_received_context handle. ]*/
        tlsio_instance->on_bytes_received_context = on_bytes_received_context;

        /* Codes_SRS_TLSIO_ARDUINO_21_032: [ The tlsio_arduino_open shall store the provided on_io_error callback function address. ]*/
        tlsio_instance->on_io_error = on_io_error;
        /* Codes_SRS_TLSIO_ARDUINO_21_033: [ The tlsio_arduino_open shall store the provided on_io_error_context handle. ]*/
        tlsio_instance->on_io_error_context = on_io_error_context;

        if ((tlsio_instance->state == TLSIO_ARDUINO_STATE_ERROR) ||
            (tlsio_instance->state == TLSIO_ARDUINO_STATE_OPENING) ||
            (tlsio_instance->state == TLSIO_ARDUINO_STATE_OPEN) ||
            (tlsio_instance->state == TLSIO_ARDUINO_STATE_CLOSING))
        {
            /* Codes_SRS_TLSIO_ARDUINO_21_035: [ If the tlsio state is TLSIO_ARDUINO_STATE_ERROR, TLSIO_ARDUINO_STATE_OPENING, TLSIO_ARDUINO_STATE_OPEN, or TLSIO_ARDUINO_STATE_CLOSING, the tlsio_arduino_open shall set the tlsio state as TLSIO_ARDUINO_STATE_ERROR, and return _LINE_. ]*/
            LogError("Try to open a connection with an already opened TLS.");
            tlsio_instance->state = TLSIO_ARDUINO_STATE_ERROR;
            result = __LINE__;
        }
        else
        {
            if (sslClient_connected())
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_037: [ If the ssl client is connected, the tlsio_arduino_open shall change the state to TLSIO_ARDUINO_STATE_ERROR, log the error, and return _LINE_. ]*/
                LogError("No SSL clients available.");
                tlsio_instance->state = TLSIO_ARDUINO_STATE_ERROR;
                result = __LINE__;
            }
            /* Codes_SRS_TLSIO_ARDUINO_21_027: [ The tlsio_arduino_open shall set the tlsio to try to open the connection for 10 times before assuming that connection failed. ]*/
            else if (sslClient_connect(tlsio_instance->remote_addr, tlsio_instance->port))
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_034: [ If tlsio_arduino_open get success to start the process to open the ssl connection, it shall set the tlsio state as TLSIO_ARDUINO_STATE_OPENING, and return 0. ]*/
                tlsio_instance->state = TLSIO_ARDUINO_STATE_OPENING;
                tlsio_instance->countTry = MAX_TLS_OPENING_RETRY;
                result = 0;
            }
            else
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_038: [ If tlsio_arduino_open failed to start the process to open the ssl connection, it shall set the tlsio state as TLSIO_ARDUINO_STATE_ERROR, and return _LINE_. ]*/
                LogError("TLS failed to start the connection process.");
                tlsio_instance->state = TLSIO_ARDUINO_STATE_ERROR;
                result = __LINE__;
            }
        }
    }

    if (result != 0)
    {
        if (on_io_open_complete != NULL)
        {
            /* Codes_SRS_TLSIO_ARDUINO_21_002: [ The tlsio_arduino shall report the open operation status using the IO_OPEN_RESULT enumerator defined in the `xio.h`.]*/
            /* Codes_SRS_TLSIO_ARDUINO_21_039: [ If the tlsio_arduino_open failed to open the tls connection, and the on_io_open_complete callback was provided, it shall call the on_io_open_complete with IO_OPEN_ERROR. ]*/
            (void)on_io_open_complete(on_io_open_complete_context, IO_OPEN_ERROR);
        }
        if (on_io_error != NULL)
        {
            /* Codes_SRS_TLSIO_ARDUINO_21_040: [ If the tlsio_arduino_open failed to open the tls connection, and the on_io_error callback was provided, it shall call the on_io_error. ]*/
            (void)on_io_error(on_io_error_context);
        }
    }
    else
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_041: [ If the tlsio_arduino_open get success opening the tls connection, it shall call the tlsio_arduino_dowork. ]*/
        tlsio_arduino_dowork(tlsio_handle);
    }
    return result;
}

/* Codes_SRS_TLSIO_ARDUINO_21_043: [ The tlsio_arduino_close shall start the process to close the ssl connection. ]*/
int tlsio_arduino_close(CONCRETE_IO_HANDLE tlsio_handle, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* on_io_close_complete_context)
{
    int result;
    ArduinoTLS* tlsio_instance = (ArduinoTLS*)tlsio_handle;

    if (tlsio_handle == NULL)
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_049: [ If the tlsio_handle is NULL, the tlsio_arduino_close shall not do anything, and return _LINE_. ]*/
        LogError("NULL TLS handle.");
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_045: [ The tlsio_arduino_close shall store the provided on_io_close_complete callback function address. ]*/
        tlsio_instance->on_io_close_complete = on_io_close_complete;
        /* Codes_SRS_TLSIO_ARDUINO_21_046: [ The tlsio_arduino_close shall store the provided on_io_close_complete_context handle. ]*/
        tlsio_instance->on_io_close_complete_context = on_io_close_complete_context;

        if ((tlsio_instance->state == TLSIO_ARDUINO_STATE_CLOSED) || (tlsio_instance->state == TLSIO_ARDUINO_STATE_ERROR))
        {
            /* Codes_SRS_TLSIO_ARDUINO_21_079: [ If the tlsio state is TLSIO_ARDUINO_STATE_ERROR, or TLSIO_ARDUINO_STATE_CLOSED, the tlsio_arduino_close shall set the tlsio state as TLSIO_ARDUINO_STATE_CLOSE, and return 0. ]*/
            tlsio_instance->state = TLSIO_ARDUINO_STATE_CLOSED;
            result = 0;
        }
        else if ((tlsio_instance->state == TLSIO_ARDUINO_STATE_OPENING) || (tlsio_instance->state == TLSIO_ARDUINO_STATE_CLOSING))
        {
            /* Codes_SRS_TLSIO_ARDUINO_21_048: [ If the tlsio state is TLSIO_ARDUINO_STATE_OPENING, or TLSIO_ARDUINO_STATE_CLOSING, the tlsio_arduino_close shall set the tlsio state as TLSIO_ARDUINO_STATE_CLOSE, and return _LINE_. ]*/
            LogError("Try to close the connection with an already closed TLS.");
            tlsio_instance->state = TLSIO_ARDUINO_STATE_ERROR;
            result = __LINE__;
        }
        else
        {
            sslClient_stop();
            /* Codes_SRS_TLSIO_ARDUINO_21_047: [ If tlsio_arduino_close get success to start the process to close the ssl connection, it shall set the tlsio state as TLSIO_ARDUINO_STATE_CLOSING, and return 0. ]*/
            tlsio_instance->state = TLSIO_ARDUINO_STATE_CLOSING;
            result = 0;
            /* Codes_SRS_TLSIO_ARDUINO_21_044: [ The tlsio_arduino_close shall set the tlsio to try to close the connection for 10 times before assuming that close connection failed. ]*/
            tlsio_instance->countTry = MAX_TLS_CLOSING_RETRY;
            /* Codes_SRS_TLSIO_ARDUINO_21_050: [ If the tlsio_arduino_close get success closing the tls connection, it shall call the tlsio_arduino_dowork. ]*/
            tlsio_arduino_dowork(tlsio_handle);
        }
    }

    return result;
}

/* Codes_SRS_TLSIO_ARDUINO_21_052: [ The tlsio_arduino_send shall send all bytes in a buffer to the ssl connectio. ]*/
int tlsio_arduino_send(CONCRETE_IO_HANDLE tlsio_handle, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context)
{
    int result;
    ArduinoTLS* tlsio_instance = (ArduinoTLS*)tlsio_handle;

    if ((tlsio_handle == NULL) || (buffer == NULL) || (size == 0))
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_059: [ If the tlsio_handle is NULL, the tlsio_arduino_send shall not do anything, and return _LINE_. ]*/
        /* Codes_SRS_TLSIO_ARDUINO_21_060: [ If the buffer is NULL, the tlsio_arduino_send shall not do anything, and return _LINE_. ]*/
        /* Codes_SRS_TLSIO_ARDUINO_21_061: [ If the size is 0, the tlsio_arduino_send shall not do anything, and return _LINE_. ]*/
        LogError("Invalid parameter");
        result = __LINE__;
    }
    else if (tlsio_instance->state != TLSIO_ARDUINO_STATE_OPEN)
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_058: [ If the tlsio state is TLSIO_ARDUINO_STATE_ERROR, TLSIO_ARDUINO_STATE_OPENING, TLSIO_ARDUINO_STATE_CLOSING, or TLSIO_ARDUINO_STATE_CLOSED, the tlsio_arduino_send shall call the on_send_complete with IO_SEND_ERROR, and return _LINE_. ]*/
        LogError("TLS is not ready to send data");
        result = __LINE__;
    }
    else
    {
        size_t send_result;
        size_t send_size = size;
        const uint8_t* runBuffer = (const uint8_t *)buffer;
        result = __LINE__;
        /* Codes_SRS_TLSIO_ARDUINO_21_055: [ if the ssl was not able to send all data in the buffer, the tlsio_arduino_send shall call the ssl again to send the remaining bytes. ]*/
        while (send_size > 0)
        {
            send_result = sslClient_write(runBuffer, send_size);

            if (send_result == 0) /* Didn't transmit anything! Failed. */
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_056: [ if the ssl was not able to send any byte in the buffer, the tlsio_arduino_send shall call the on_send_complete with IO_SEND_ERROR, and return _LINE_. ]*/
                LogError("TLS failed sending data");
                /* Codes_SRS_TLSIO_ARDUINO_21_053: [ The tlsio_arduino_send shall use the provided on_io_send_complete callback function address. ]*/
                /* Codes_SRS_TLSIO_ARDUINO_21_054: [ The tlsio_arduino_send shall use the provided on_io_send_complete_context handle. ]*/
                if (on_send_complete != NULL)
                {
                    /* Codes_SRS_TLSIO_ARDUINO_21_003: [ The tlsio_arduino shall report the send operation status using the IO_SEND_RESULT enumerator defined in the `xio.h`. ]*/
                    on_send_complete(callback_context, IO_SEND_ERROR);
                }
                send_size = 0;
            }
            else if (send_result >= send_size) /* Transmit it all. */
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_057: [ if the ssl finish to send all bytes in the buffer, the tlsio_arduino_send shall call the on_send_complete with IO_SEND_OK, and return 0 ]*/
                /* Codes_SRS_TLSIO_ARDUINO_21_053: [ The tlsio_arduino_send shall use the provided on_io_send_complete callback function address. ]*/
                /* Codes_SRS_TLSIO_ARDUINO_21_054: [ The tlsio_arduino_send shall use the provided on_io_send_complete_context handle. ]*/
                if (on_send_complete != NULL)
                {
                    /* Codes_SRS_TLSIO_ARDUINO_21_003: [ The tlsio_arduino shall report the send operation status using the IO_SEND_RESULT enumerator defined in the `xio.h`. ]*/
                    on_send_complete(callback_context, IO_SEND_OK);
                }
                result = 0;
                send_size = 0;
            }
            else /* Still have buffer to transmit. */
            {
                runBuffer += send_result;
                send_size -= send_result;
            }
        }
    }
    return result;
}

/* Codes_SRS_TLSIO_ARDUINO_21_062: [ The tlsio_arduino_dowork shall execute the async jobs for the tlsio. ]*/
void tlsio_arduino_dowork(CONCRETE_IO_HANDLE tlsio_handle)
{
    if (tlsio_handle == NULL)
    {
        /* Codes_SRS_TLSIO_ARDUINO_21_074: [ If the tlsio_handle is NULL, the tlsio_arduino_dowork shall not do anything. ]*/
        LogError("Invalid parameter");
    }
    else
    {
        int received;
        ArduinoTLS* tlsio_instance = (ArduinoTLS*)tlsio_handle;
        /* Codes_SRS_TLSIO_ARDUINO_21_075: [ The tlsio_arduino_dowork shall create a buffer to store the data received from the ssl client. ]*/
        /* Codes_SRS_TLSIO_ARDUINO_21_076: [ The tlsio_arduino_dowork shall delete the buffer to store the data received from the ssl client. ]*/
        uint8_t RecvBuffer[RECEIVE_BUFFER_SIZE];

        switch (tlsio_instance->state)
        {
        case TLSIO_ARDUINO_STATE_OPENING:
            if (sslClient_connected())
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_063: [ If the tlsio state is TLSIO_ARDUINO_STATE_OPENING, and ssl client is connected, the tlsio_arduino_dowork shall change the tlsio state to TLSIO_ARDUINO_STATE_OPEN, and call the on_io_open_complete with IO_OPEN_OK. ]*/
                tlsio_instance->state = TLSIO_ARDUINO_STATE_OPEN;
                /* Codes_SRS_TLSIO_ARDUINO_21_002: [ The tlsio_arduino shall report the open operation status using the IO_OPEN_RESULT enumerator defined in the `xio.h`.]*/
                CallOpenCallback(IO_OPEN_OK);
            }
            /* Codes_SRS_TLSIO_ARDUINO_21_064: [ If the tlsio state is TLSIO_ARDUINO_STATE_OPENING, and ssl client is not connected, the tlsio_arduino_dowork shall decrement the counter of trys for opening. ]*/
            else if ((tlsio_instance->countTry--) <= 0)
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_065: [ If the tlsio state is TLSIO_ARDUINO_STATE_OPENING, ssl client is not connected, and the counter to try becomes 0, the tlsio_arduino_dowork shall change the tlsio state to TLSIO_ARDUINO_STATE_ERROR, call on_io_open_complete with IO_OPEN_CANCELLED, call on_io_error. ]*/
                tlsio_instance->state = TLSIO_ARDUINO_STATE_ERROR;
                LogError("Timeout for TLS connect.");
                /* Codes_SRS_TLSIO_ARDUINO_21_002: [ The tlsio_arduino shall report the open operation status using the IO_OPEN_RESULT enumerator defined in the `xio.h`.]*/
                /* Codes_SRS_TLSIO_ARDUINO_21_042: [ If the tlsio_arduino_open retry to open more than 10 times without success, it shall call the on_io_open_complete with IO_OPEN_CANCELED. ]*/
                CallOpenCallback(IO_OPEN_CANCELLED);
                CallErrorCallback();
            }
            break;
        case TLSIO_ARDUINO_STATE_OPEN:
            if (!sslClient_connected())
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_071: [ If the tlsio state is TLSIO_ARDUINO_STATE_OPEN, and ssl client is not connected, the tlsio_arduino_dowork shall change the state to TLSIO_ARDUINO_STATE_ERROR, call on_io_error. ]*/
                tlsio_instance->state = TLSIO_ARDUINO_STATE_ERROR;
                LogError("SSL closed the connection.");
                CallErrorCallback();
            }
            else
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_069: [ If the tlsio state is TLSIO_ARDUINO_STATE_OPEN, the tlsio_arduino_dowork shall read data from the ssl client. ]*/
                received = sslClient_read((uint8_t*)RecvBuffer, RECEIVE_BUFFER_SIZE);
                if (received > 0)
                {
                    /* Codes_SRS_TLSIO_ARDUINO_21_070: [ If the tlsio state is TLSIO_ARDUINO_STATE_OPEN, and there are received data in the ssl client, the tlsio_arduino_dowork shall read this data and call the on_bytes_received with the pointer to the buffer with the data. ]*/
                    if (tlsio_instance->on_bytes_received != NULL)
                    {
                        // explictly ignoring here the result of the callback
                        (void)tlsio_instance->on_bytes_received(tlsio_instance->on_bytes_received_context, (const unsigned char*)RecvBuffer, received);
                    }
                }
            }
            break;
        case TLSIO_ARDUINO_STATE_CLOSING:
            if (!sslClient_connected())
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_066: [ If the tlsio state is TLSIO_ARDUINO_STATE_CLOSING, and ssl client is not connected, the tlsio_arduino_dowork shall change the tlsio state to TLSIO_ARDUINO_STATE_CLOSE, and call the on_io_close_complete. ]*/
                tlsio_instance->state = TLSIO_ARDUINO_STATE_CLOSED;
                CallCloseCallback();
            }
            /* Codes_SRS_TLSIO_ARDUINO_21_067: [ If the tlsio state is TLSIO_ARDUINO_STATE_CLOSING, and ssl client is connected, the tlsio_arduino_dowork shall decrement the counter of trys for closing. ]*/
            else if ((tlsio_instance->countTry--) <= 0)
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_051: [ If the tlsio_arduino_close retry to close more than 10 times without success, it shall call the on_io_error. ]*/
                /* Codes_SRS_TLSIO_ARDUINO_21_068: [ If the tlsio state is TLSIO_ARDUINO_STATE_CLOSING, ssl client is connected, and the counter to try becomes 0, the tlsio_arduino_dowork shall change the tlsio state to TLSIO_ARDUINO_STATE_ERROR, call on_io_error. ]*/
                tlsio_instance->state = TLSIO_ARDUINO_STATE_ERROR;
                LogError("Timeout for close TLS");
                CallErrorCallback();

            }
            break;
        case TLSIO_ARDUINO_STATE_CLOSED:
            if (sslClient_connected())
            {
                /* Codes_SRS_TLSIO_ARDUINO_21_072: [ If the tlsio state is TLSIO_ARDUINO_STATE_CLOSED, and ssl client is connected, the tlsio_arduino_dowork shall change the state to TLSIO_ARDUINO_STATE_ERROR, call on_io_error. ]*/
                tlsio_instance->state = TLSIO_ARDUINO_STATE_ERROR;
                LogError("SSL keep the connection open with TLS closed.");
                CallErrorCallback();
            }
        case TLSIO_ARDUINO_STATE_ERROR:
            /* Codes_SRS_TLSIO_ARDUINO_21_073: [ If the tlsio state is TLSIO_ARDUINO_STATE_ERROR, the tlsio_arduino_dowork shall not do anything. ]*/
        default:
            break;
        }
    }
}

int tlsio_arduino_setoption(CONCRETE_IO_HANDLE tlsio_handle, const char* optionName, const void* value)
{
    /* Codes_SRS_TLSIO_ARDUINO_21_077: [ The tlsio_arduino_setoption shall not do anything, and return 0. ]*/
    (void)(tlsio_handle, optionName, value);

    /* Not implementing any options */
    return 0;
}

OPTIONHANDLER_HANDLE tlsio_arduino_retrieveoptions(CONCRETE_IO_HANDLE tlsio_handle)
{
    /* Codes_SRS_TLSIO_ARDUINO_21_078: [ The tlsio_arduino_retrieveoptions shall not do anything, and return NULL. ]*/
    (void)(tlsio_handle);
        
    /* Not implementing any options */
    return NULL;
}


