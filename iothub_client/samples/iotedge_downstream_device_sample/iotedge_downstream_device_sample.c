// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample demonstrates how a downstream device connects to an Edge device. For more details
// pertaining to this scenario please see:
// https://docs.microsoft.com/azure/iot-edge/how-to-create-transparent-gateway-linux OR
// https://docs.microsoft.com/azure/iot-edge/how-to-create-transparent-gateway-windows
//
// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity. Please practice sound engineering practices
// when writing production code.

#include <stdio.h>
#include <stdlib.h>

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"

/* This sample uses the convenience APIs of iothub_client for example purposes. */

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

/*
    1) Obtain the connection string for your downstream device and to it
       append this string GatewayHostName=<edge device hostname>;
    2) The edge device hostname is the hostname set in the config.yaml of the Edge device
       to which this sample will connect to.

    The resulting string should look like the following
    "HostName=<iothub_host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>;GatewayHostName=<edge device hostname>"
*/
static const char* connectionString = "[Downstream device IoT Edge connection string]";

// Path to the Edge "owner" root CA certificate
static const char* edge_ca_cert_path = "[Path to Edge CA certificate]";

#define MESSAGE_COUNT        20
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)user_context;
    const char* messageId;
    const char* correlationId;

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL)
    {
        messageId = "<unavailable>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL)
    {
        correlationId = "<unavailable>";
    }

    IOTHUBMESSAGE_CONTENT_TYPE content_type = IoTHubMessage_GetContentType(message);
    if (content_type == IOTHUBMESSAGE_BYTEARRAY)
    {
        const unsigned char* buff_msg;
        size_t buff_len;

        if (IoTHubMessage_GetByteArray(message, &buff_msg, &buff_len) != IOTHUB_MESSAGE_OK)
        {
            (void)printf("Failure retrieving byte array message\r\n");
        }
        else
        {
            (void)printf("Received Binary message\r\nMessage ID: %s\r\n Correlation ID: %s\r\n Data: <<<%.*s>>> & Size=%d\r\n", messageId, correlationId, (int)buff_len, buff_msg, (int)buff_len);
        }
    }
    else
    {
        const char* string_msg = IoTHubMessage_GetString(message);
        if (string_msg == NULL)
        {
            (void)printf("Failure retrieving byte array message\r\n");
        }
        else
        {
            (void)printf("Received String Message\r\nMessage ID: %s\r\n Correlation ID: %s\r\n Data: <<<%s>>>\r\n", messageId, correlationId, string_msg);
        }
    }
    return IOTHUBMESSAGE_ACCEPTED;
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    (void)user_context;
    // This sample DOES NOT take into consideration network outages.
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        (void)printf("The device client is connected to IoT Edge\r\n");
    }
    else
    {
        (void)printf("The device client has been disconnected\r\n");
    }
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get invoked
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %zu with result %s\r\n", g_message_count_send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

/**
    Read the certificate file and provide a null terminated string
    containing the certificate.
*/
static char *obtain_edge_ca_certificate(void)
{
    char *result = NULL;
    FILE *ca_file;

    ca_file = fopen(edge_ca_cert_path, "r");
    if (ca_file == NULL)
    {
        printf("Error could not open file for reading %s\r\n", edge_ca_cert_path);
    }
    else
    {
        size_t file_size;

        (void)fseek(ca_file, 0, SEEK_END);
        file_size = ftell(ca_file);
        (void)fseek(ca_file, 0, SEEK_SET);
        // increment size to hold the null term
        file_size += 1;

        if (file_size == 0) // check wrap around
        {
            printf("File size invalid for %s\r\n", edge_ca_cert_path);
        }
        else
        {
            result = (char*)calloc(file_size, 1);
            if (result == NULL)
            {
                printf("Could not allocate memory to hold the certificate\r\n");
            }
            else
            {
                // copy the file into the buffer
                size_t read_size = fread(result, 1, file_size - 1, ca_file);
                if (read_size != file_size - 1)
                {
                    printf("Error reading file %s\r\n", edge_ca_cert_path);
                    free(result);
                    result = NULL;
                }
            }
        }
        (void)fclose(ca_file);
    }

    return result;
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_MESSAGE_HANDLE message_handle;
    const char* telemetry_msg = "test_message";
    char *cert_string = NULL;

    printf("This sample will send %d messages and wait for any C2D messages.\r\nPress the enter key to end the sample\r\n\r\n", MESSAGE_COUNT);

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

    IOTHUB_DEVICE_CLIENT_HANDLE device_handle;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    (void)printf("Creating IoTHub handle\r\n");
    // Create the iothub handle here
    device_handle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, protocol);
    if (device_handle == NULL)
    {
        (void)printf("Failure creating device client handle. Hint: Check your connection string.\r\n");
    }
    else
    {
        // Setting message callback to get C2D messages
        (void)IoTHubDeviceClient_SetMessageCallback(device_handle, receive_msg_callback, NULL);
        // Setting connection status callback to get indication of connection to iothub
        (void)IoTHubDeviceClient_SetConnectionStatusCallback(device_handle, connection_status_callback, NULL);

        // Set any option that are necessary.
        // For available options please see the iothub_sdk_options.md documentation

        // Turn tracing on/off
        // bool traceOn = true;
        // (void)IoTHubDeviceClient_SetOption(device_handle, OPTION_LOG_TRACE, &traceOn);

        // Provide the Azure IoT device client with the Edge root
        // X509 CA certificate that was used to setup the Edge runtime
        cert_string = obtain_edge_ca_certificate();
        (void)IoTHubDeviceClient_SetOption(device_handle, OPTION_TRUSTED_CERT, cert_string);

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_OVER_WEBSOCKETS
        //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
        //you are URL Encoding inputs yourself.
        //ONLY valid for use with MQTT
        bool urlEncodeOn = true;
        (void)IoTHubDeviceClient_SetOption(device_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif

        for (size_t index = 0; index < MESSAGE_COUNT; index++)
        {
            // Construct the iothub message from a string
            message_handle = IoTHubMessage_CreateFromString(telemetry_msg);

            // Set Message property
            (void)IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
            (void)IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
            (void)IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application%2fjson");
            (void)IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");

            // Add custom properties to message
            (void)IoTHubMessage_SetProperty(message_handle, "property_key", "property_value");

            (void)printf("Sending message %d to Edge Hub\r\n", (int)(index + 1));
            IoTHubDeviceClient_SendEventAsync(device_handle, message_handle, send_confirm_callback, NULL);

            // The message is copied to the sdk so the we can destroy it
            IoTHubMessage_Destroy(message_handle);
        }

        printf("\r\nPress any key to continue\r\n");
        (void)getchar();

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_Destroy(device_handle);

        if (cert_string != NULL)
        {
            free(cert_string);
            cert_string = NULL;
        }
    }
    // Free all the sdk subsystem
    IoTHub_Deinit();

    return 0;
}
