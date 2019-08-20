// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices 
// when writing production code.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "iothub.h"
#include "iothub_client_streaming.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/socketio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/ws_url.h"
#include "azure_c_shared_utility/uws_client.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/* This sample uses the _LL APIs of iothub_client for example purposes.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
    #include "iothubtransportmqtt.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    #include "iothubtransportmqtt_websockets.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    #include "iothubtransportamqp.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    #include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    #include "iothubtransporthttp.h"
#endif // SAMPLE_HTTP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

MU_DEFINE_ENUM_STRINGS(WS_OPEN_RESULT, WS_OPEN_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(WS_ERROR, WS_ERROR_VALUES)
MU_DEFINE_ENUM_STRINGS(WS_SEND_FRAME_RESULT, WS_SEND_FRAME_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(IO_SEND_RESULT, IO_SEND_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(IO_OPEN_RESULT, IO_OPEN_RESULT_VALUES)

/* Paste in the your iothub connection string  */
static const char* connectionString = "[device connection string]";
static const char* localHost = "127.0.0.1"; // Address of the local server to connect to.
static const int localPort = 22; // Port of the local server to connect to.

static const char* proxy_host = NULL;     // "Web proxy name here"
static int proxy_port = 0;                // Proxy port
static const char* proxy_username = NULL; // Proxy user name
static const char* proxy_password = NULL; // Proxy password

static bool g_continueRunning = true;
static UWS_CLIENT_HANDLE g_uws_client_handle = NULL;
static bool g_is_uws_client_ready = false;
static XIO_HANDLE g_io_handle = NULL; // connection to the local service.
static bool g_is_socket_connection_ready = false;

static size_t g_incomingByteCount = 0;
static size_t g_outgoingByteCount = 0;
static time_t g_lastTrafficPrintTime;

#define TRAFFIC_COUNTERS_PRINT_FREQ_IN_SECS 10
#define INDEFINITE_TIME ((time_t)-1)

// Functions for socket connection to local service:

static void on_ws_send_frame_complete(void* context, WS_SEND_FRAME_RESULT ws_send_frame_result)
{
    (void)context;

    if (ws_send_frame_result != WS_SEND_FRAME_OK)
    {
        g_continueRunning = false;
    }
}

static void on_bytes_received(void* context, const unsigned char* buffer, size_t size)
{
    (void)context;

    if (g_is_uws_client_ready)
    {
        if (uws_client_send_frame_async(g_uws_client_handle, 2, buffer, size, true, on_ws_send_frame_complete, NULL) != 0)
        {
            (void)printf("Failed sending data to the local service\r\n");
            g_continueRunning = false;
        }

        g_outgoingByteCount += size;
    }
}
static void on_io_open_complete(void* context, IO_OPEN_RESULT open_result)
{
    (void)context;

    if (open_result != IO_OPEN_OK)
    {
        (void)printf("Failed opening connection to the local service (%s)\r\n", MU_ENUM_TO_STRING(IO_OPEN_RESULT, open_result));
        g_continueRunning = false;
    }
    else
    {
        g_is_socket_connection_ready = true;
    }
}

static void on_io_error(void* context)
{
    (void)context;
    (void)printf("Connection to the local service has failed\r\n");
    g_continueRunning = false;
}

static XIO_HANDLE connect_to_local_service()
{
    XIO_HANDLE result;
    SOCKETIO_CONFIG io_config;
    io_config.hostname = localHost;
    io_config.port = localPort;

    const IO_INTERFACE_DESCRIPTION* io_interface_description = socketio_get_interface_description();

    result = xio_create(io_interface_description, &io_config);
    (void)xio_open(result, on_io_open_complete, NULL, on_bytes_received, NULL, on_io_error, NULL);

    return result;
}

// Functions for connection to streaming gateway (cloud):

static void on_ws_open_complete(void* context, WS_OPEN_RESULT ws_open_result)
{
    (void)context;
    (void)printf("Client connected to the streaming gateway (%s)\r\n", MU_ENUM_TO_STRING(WS_OPEN_RESULT, ws_open_result));
    
    if (ws_open_result == WS_OPEN_OK)
    {
        g_io_handle = connect_to_local_service();
        g_is_uws_client_ready = true;

        (void)printf("Reporting traffic statistics every 10 seconds.\r\n");
    }
    else
    {
        g_continueRunning = false;
    }
}

static void on_send_complete(void* context, IO_SEND_RESULT send_result) 
{
    (void)context;

    if (send_result != IO_SEND_OK)
    {
        g_continueRunning = false;
    }
}

static void on_ws_frame_received(void* context, unsigned char frame_type, const unsigned char* buffer, size_t size)
{
    (void)context;
    (void)frame_type;

    if (g_is_socket_connection_ready)
    {
        if (xio_send(g_io_handle, buffer, size, on_send_complete, NULL) != 0)
        {
            (void)printf("Failed sending data to the local service\r\n");
            g_continueRunning = false;
        }

        g_incomingByteCount += size;
    }
}

static void on_ws_peer_closed(void* context, uint16_t* close_code, const unsigned char* extra_data, size_t extra_data_length)
{
    (void)context;
    (void)extra_data_length;
    (void)printf("on_ws_peer_closed (Code: %d, Data: %.*s)\r\n", *close_code, (int)extra_data_length, extra_data);
    g_continueRunning = false;
}

static void on_ws_error(void* context, WS_ERROR error_code)
{
    (void)context;
    (void)printf("on_ws_error (%s)\r\n", MU_ENUM_TO_STRING(WS_ERROR, error_code));
    g_continueRunning = false;
}

static UWS_CLIENT_HANDLE create_websocket_client(DEVICE_STREAM_C2D_REQUEST* stream_request)
{
    UWS_CLIENT_HANDLE result;
    HTTP_PROXY_IO_CONFIG http_proxy_io_config;
    TLSIO_CONFIG tls_io_config;
    const IO_INTERFACE_DESCRIPTION* tlsio_interface;

    WS_URL_HANDLE ws_url;
    WS_PROTOCOL protocols;
    char auth_header_value[1024];

    const char* host;
    size_t host_length;
    const char* path;
    size_t path_length;
    int port;

    char host_address[1024];
    char resource_name[1024];

    ws_url = ws_url_create(stream_request->url);
    (void)ws_url_get_host(ws_url, &host, &host_length);
    (void)ws_url_get_path(ws_url, &path, &path_length);
    (void)ws_url_get_port(ws_url, (size_t*)&port);

    (void)memcpy(host_address, host, host_length);
    host_address[host_length] = '\0';

    (void)memcpy(resource_name + 1, path, path_length);
    resource_name[0] = '/';
    resource_name[path_length + 1] = '\0';

    // Establishing the connection to the streaming gateway
#if defined SAMPLE_AMQP || defined SAMPLE_AMQP_OVER_WEBSOCKETS
    protocols.protocol = "AMQP";
#elif defined SAMPLE_HTTP
    protocols.protocol = "HTTP";
#else
    protocols.protocol = "MQTT";
#endif

    (void)sprintf(auth_header_value, "Bearer %s", stream_request->authorization_token);

    // Setting up optional HTTP proxy configuration for connecting to streaming gateway:
    tlsio_interface = platform_get_default_tlsio();

    tls_io_config.hostname = host_address;
    tls_io_config.port = port;

    if (proxy_host != NULL)
    {
        http_proxy_io_config.proxy_hostname = proxy_host;
        http_proxy_io_config.proxy_port = proxy_port;
        http_proxy_io_config.username = proxy_username;
        http_proxy_io_config.password = proxy_password;
        http_proxy_io_config.hostname = host_address;
        http_proxy_io_config.port = port;

        tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();
        tls_io_config.underlying_io_parameters = &http_proxy_io_config;

        result = uws_client_create_with_io(tlsio_interface, &tls_io_config, host_address, port, resource_name, &protocols, 1);
    }
    else
    {
        result = uws_client_create(host_address, port, resource_name, true, &protocols, 1);
    }
    (void)uws_client_set_request_header(result, "Authorization", auth_header_value);
    (void)uws_client_open_async(result, on_ws_open_complete, NULL, on_ws_frame_received, NULL, on_ws_peer_closed, NULL, on_ws_error, NULL);

    ws_url_destroy(ws_url);

    return result;
}

static DEVICE_STREAM_C2D_RESPONSE* streamRequestCallback(DEVICE_STREAM_C2D_REQUEST* stream_request, void* context)
{
    (void)context;

    (void)printf("Received stream request (%s)\r\n", stream_request->name);

    g_uws_client_handle = create_websocket_client(stream_request);

    return stream_c2d_response_create(stream_request, true);
}

static void print_traffic_counters()
{
    if (g_lastTrafficPrintTime != INDEFINITE_TIME)
    {
        time_t current_time = time(NULL);

        if (current_time != INDEFINITE_TIME)
        {
            if ((g_incomingByteCount > 0 || g_outgoingByteCount > 0) && difftime(current_time, g_lastTrafficPrintTime) >= TRAFFIC_COUNTERS_PRINT_FREQ_IN_SECS)
            {
                char time_str[128];
                struct tm* current_time_tm = localtime(&current_time);
                strftime(time_str, 128, "%F %T UTC%z", current_time_tm);

                (void)printf("[%s] Network traffic (in bytes) (sent=%d; received=%d)\r\n", time_str, (int)g_outgoingByteCount, (int)g_incomingByteCount);
                g_incomingByteCount = 0;
                g_outgoingByteCount = 0;
                g_lastTrafficPrintTime = current_time;
            }
        }
    }
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

    // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
    protocol = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    protocol = HTTP_Protocol;
#endif // SAMPLE_HTTP

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    g_lastTrafficPrintTime = time(NULL);

    IOTHUB_DEVICE_CLIENT_HANDLE device_handle;

    // Create the iothub handle here
    device_handle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, protocol);

    if (device_handle == NULL)
    {
        (void)printf("Failure creating the IotHub device. Hint: Check you connection string.\r\n");
    }
    else
    {
        // Set any option that are neccessary.
        // For available options please see the iothub_sdk_options.md documentation

        bool traceOn = true;
        IoTHubDeviceClient_SetOption(device_handle, OPTION_LOG_TRACE, &traceOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate.  This is only necessary on system with without
        // built in certificate stores.
            IoTHubDeviceClient_SetOption(device_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_WS
        //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
        //you are URL Encoding inputs yourself.
        //ONLY valid for use with MQTT
        //bool urlEncodeOn = true;
        //IoTHubDeviceClient_SetOption(iothub_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif

        if (proxy_host)
        {
            HTTP_PROXY_OPTIONS http_proxy_options = { 0 };
            http_proxy_options.host_address = proxy_host;
            http_proxy_options.port = proxy_port;
            http_proxy_options.username = proxy_username;
            http_proxy_options.password = proxy_password;

            if (IoTHubDeviceClient_SetOption(device_handle, OPTION_HTTP_PROXY, &http_proxy_options) != IOTHUB_CLIENT_OK)
            {
                (void)printf("failure to set proxy\n");
            }
        }

        if (IoTHubDeviceClient_SetStreamRequestCallback(device_handle, streamRequestCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            (void)printf("Failed setting the stream request callback");
        }
        else
        {
            do
            {
                if (g_uws_client_handle != NULL)
                {
                    uws_client_dowork(g_uws_client_handle);
                }

                if (g_io_handle != NULL)
                {
                    xio_dowork(g_io_handle);

                    print_traffic_counters();
                }

                ThreadAPI_Sleep(100);

            } while (g_continueRunning);

        }
 
        // Clean up the iothub sdk handle
        IoTHubDeviceClient_Destroy(device_handle);

        if (g_uws_client_handle != NULL)
        {
            uws_client_destroy(g_uws_client_handle);
        }

        if (g_io_handle != NULL)
        {
            xio_destroy(g_io_handle);
        }
    }

    // Free all the sdk subsystem
    IoTHub_Deinit();

    (void)printf("Press any key to continue");
    getchar();

    return 0;
}
