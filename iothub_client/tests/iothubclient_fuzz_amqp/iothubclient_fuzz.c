// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Fuzzing documentation:
// SDK fuzzing using afl-fuzz at https://lcamtuf.coredump.cx/afl/ using package https://packages.ubuntu.com/bionic/afl
// more details at https://expresslogic.visualstudio.com/X-Ware/_wiki/wikis/X-Ware.wiki/8/Azure-RTOS-Fuzz-Testing

// Linux OS setup:
//   sudo apt install afl++
//   sudo echo core > /proc/sys/kernel/core_pattern
//

// Source setup
//   git clone  https://github.com/Azure/azure-iot-sdk-c.git
//   cd azure-iot-sdk-c
//   git submodule update --init
//

// Build Linux
//   cd azure-iot-sdk-c
//   mkdir cmake
//   cd cmake
//   AFL_HARDEN=1
//   cmake -Duse_schannel=OFF -Duse_openssl=OFF -Duse_socketio=OFF -Dbuild_service_client=OFF -Dbuild_provisioning_service_client=OFF -Drun_unittests=ON -Dskip_samples=ON -Duse_wsio=OFF -Duse_http=OFF -Ddont_use_uploadtoblob=ON -Duse_wsio=OFF -DCMAKE_C_COMPILER=/usr/bin/afl-gcc -DcompileOption_C=-fsanitize=address ..
//   cmake --build . --target iothubclient_fuzz_amqp
//

// Run
//   cd ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_amqp
//   mkdir ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_amqp/findings_dir
//   afl-fuzz -m 230000000 -t 10000 -i ~/azure-iot-sdk-c/iothub_client/tests/iothubclient_fuzz_amqp/packets -o findings_dir ./iothubclient_fuzz_amqp @@

#include <stdio.h>
#include <stdlib.h>

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/xio.h"


// The protocol you wish to use should be uncommented
//
//#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
#define SAMPLE_AMQP
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

/* Paste in the your iothub connection string  */
static const char* connectionString = "HostName=ericwol-hub.azure-devices.net;DeviceId=eeqq;SharedAccessSignature=SharedAccessSignature sr=ericwol-hub.azure-devices.net%2Fdevices%2Feeqq&sig=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX%3D&se=5605002146";

#define MESSAGE_COUNT        5
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;
static size_t g_message_recv_count = 0;
static int g_packet_id_fuzz = -1;

char* test_filepath;
unsigned char fuzzpacket_buffer[4096];
size_t fuzzpacket_len;
int fuzzpacket_id = -1;


static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get invoked
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    (void)user_context;
    // This sample DOES NOT take into consideration network outages.
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        (void)printf("The device client is connected to iothub\r\n");
    }
    else
    {
        (void)printf("The device client has been disconnected\r\n");
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)message;
    (void)user_context;

    g_message_recv_count++;
    (void)printf("receive_msg_callback() count=%d\r\n", (int)g_message_recv_count);
    return IOTHUBMESSAGE_ACCEPTED;
}

static int device_method_callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    (void)method_name;
    (void)payload;
    (void)size;
    (void)userContextCallback;

    g_message_recv_count++;
    char method_status[] = "{\"status\":200}";
    *response_size = strlen(method_status);
    *response = malloc(*response_size);
    (void)memcpy(*response, method_status, *response_size);
    (void)printf("device_method_callback() method_name=%s\r\n", method_name);

    return 200;
}

static void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)payLoad;
    (void)size;
    (void)userContextCallback;
    (void)printf("deviceTwinCallback() update_state=%d\r\n", update_state);
}

int main(int argc, const char* argv[])
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_MESSAGE_HANDLE message_handle;
    size_t messages_sent = 0;
    const char* telemetry_msg = "test_message";

    if (argc == 2)
    {
        test_filepath = (char*)argv[1];

        FILE* fp = fopen(test_filepath, "rb");
        if (fp == NULL)
        {
            (void)printf("ERROR: Cant open file %s\r\n", test_filepath);
            return -1;
        }
        fuzzpacket_len = fread(fuzzpacket_buffer, sizeof(fuzzpacket_buffer[0]), sizeof(fuzzpacket_buffer), fp);
        fclose(fp);

        if (fuzzpacket_len == sizeof(fuzzpacket_buffer))
        {
            (void)printf("ERROR: test file is to big\r\n");
            return -1;
        }

        if (fuzzpacket_buffer[0] != (fuzzpacket_buffer[1] ^ 0xff))
        {
            // packte ID got fuzzed, ok to return success
            return 0;
        }

        fuzzpacket_id = fuzzpacket_buffer[0];
        (void)printf("fuzzing with packet id: %d\r\n", fuzzpacket_id);
    }
    else if (argc > 1)
    {
        (void)printf("Usage:\r\n");
        return -1;
    }


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

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    (void)printf("Creating IoTHub Device handle\r\n");
    // Create the iothub handle here
    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (device_ll_handle == NULL)
    {
        (void)printf("Failure creating IotHub device. Hint: Check your connection string.\r\n");
    }
    else
    {
        // Set any option that are necessary.
        // For available options please see the iothub_sdk_options.md documentation

#ifndef SAMPLE_HTTP
        // Can not set this options in HTTP
        bool traceOn = true;
        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_LOG_TRACE, &traceOn);
#endif

        // Setting connection status callback to get indication of connection to iothub
        (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL);
        (void)IoTHubDeviceClient_LL_SetMessageCallback(device_ll_handle, receive_msg_callback, NULL);
        (void)IoTHubDeviceClient_LL_GetTwinAsync(device_ll_handle, deviceTwinCallback, NULL);
        (void)IoTHubDeviceClient_LL_SetDeviceMethodCallback(device_ll_handle, device_method_callback, NULL);
        (void)IoTHubDeviceClient_LL_SetDeviceTwinCallback(device_ll_handle, deviceTwinCallback, NULL);

        int loop_count;
        for (loop_count = 0; g_continueRunning && loop_count < 300000; loop_count++)
        {
            if (messages_sent < MESSAGE_COUNT)
            {
                // Construct the iothub message from a string or a byte array
                message_handle = IoTHubMessage_CreateFromString(telemetry_msg);

                (void)printf("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
                IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, send_confirm_callback, NULL);

                // The message is copied to the sdk so then we can destroy it
                IoTHubMessage_Destroy(message_handle);

                messages_sent++;
            }
            else if (g_message_count_send_confirmations >= MESSAGE_COUNT && g_message_recv_count > 0)
            {
                // After all messages are all received stop running
                g_continueRunning = false;
            }

            IoTHubDeviceClient_LL_DoWork(device_ll_handle);
        }
        (void)printf("Exiting with IoTHubDeviceClient_LL_DoWork() count of: %d\r\n", loop_count);

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    }
    // Free all the sdk subsystem
    IoTHub_Deinit();

    (void)printf("Test exit: SUCCESS\r\n");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
typedef struct queue_item
{
    const unsigned char* data;
    size_t size;
    struct queue_item* next;
} QUEUE_ITEM;

static ON_IO_OPEN_COMPLETE on_io_open_complete_callback;
static void* on_io_open_complete_context_callback;

static ON_BYTES_RECEIVED on_bytes_received_callback;
static void* on_bytes_received_context_callback;

QUEUE_ITEM* received_queue = NULL;
void received_queue_add(const unsigned char* buffer, size_t size)
{
    QUEUE_ITEM* item = malloc(sizeof(QUEUE_ITEM));
    if (item == NULL) return;
    item->data = malloc(size);
    if (item->data == NULL)
    {
        free(item);
        return;
    }
    memcpy((void*)item->data, buffer, size);
    item->size = size;
    item->next = NULL;

    if (received_queue == NULL)
    {
        received_queue = item;
    }
    else
    {
        // append to end
        QUEUE_ITEM* node = received_queue;
        while (node->next != NULL)
        {
            node = node->next;
        }
        node->next = item;
    }
}

QUEUE_ITEM* received_queue_remove(void)
{
    QUEUE_ITEM* current = received_queue;
    if (received_queue != NULL)
    {
        received_queue = received_queue->next;
    }
    return current;
}

void received_queue_free(QUEUE_ITEM* removed)
{
    free((void*)removed->data);
    free(removed);
}


static void* tlsio_fuzz_CloneOption(const char* name, const void* value)
{
    (void)name;
    void* result;
    if (mallocAndStrcpy_s((char**)&result, (const char*)value) != 0)
    {
        result = NULL;
    }
    return result;
}

static void tlsio_fuzz_DestroyOption(const char* name, const void* value)
{
    (void)name;
    (void)value;
}

static int tlsio_fuzz_setoption(CONCRETE_IO_HANDLE tls_io, const char* optionName, const void* value)
{
    (void)tls_io;
    (void)optionName;
    (void)value;
    return 0;
}

static OPTIONHANDLER_HANDLE tlsio_fuzz_retrieveoptions(CONCRETE_IO_HANDLE handle)
{
    (void)handle;
    OPTIONHANDLER_HANDLE result;
    result = OptionHandler_Create(tlsio_fuzz_CloneOption, tlsio_fuzz_DestroyOption, tlsio_fuzz_setoption);
    return result;
}

CONCRETE_IO_HANDLE tlsio_fuzz_create(void* io_create_parameters)
{
    (void)io_create_parameters;
    return (CONCRETE_IO_HANDLE*)malloc(sizeof(CONCRETE_IO_HANDLE));
}

void tlsio_fuzz_destroy(CONCRETE_IO_HANDLE tls_io)
{
    if (tls_io)
    {
        free(tls_io);
    }
}

int tlsio_fuzz_open(CONCRETE_IO_HANDLE tls_io, ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_context, ON_BYTES_RECEIVED on_bytes_received,
    void* on_bytes_received_context, ON_IO_ERROR on_io_error, void* on_io_error_context)
{
    (void)tls_io;
    on_io_open_complete_callback = on_io_open_complete;
    on_io_open_complete_context_callback = on_io_open_complete_context;
    on_bytes_received_callback = on_bytes_received;
    on_bytes_received_context_callback = on_bytes_received_context;
    (void)on_io_error;
    (void)on_io_error_context;

    on_io_open_complete_callback(on_io_open_complete_context, IO_OPEN_OK);

    return 0;
}

int tlsio_fuzz_close(CONCRETE_IO_HANDLE tls_io, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context)
{
    (void)tls_io;
    (void)on_io_close_complete;
    (void)callback_context;
    while (received_queue != NULL)
    {
        QUEUE_ITEM* item = received_queue_remove();
        received_queue_free(item);
    }
    return 0;
}

size_t load_from_file(int i, unsigned char file_buffer[], size_t size)
{
    if (fuzzpacket_id == i)
    {
        // return the fuzz packets instead
        (void)printf("using fuzzed packet id: %d\r\n", fuzzpacket_id);
        size_t buf_max_len = (fuzzpacket_len > size ? size : fuzzpacket_len) - 2;
        memcpy(file_buffer, &fuzzpacket_buffer[2], buf_max_len);
        return buf_max_len;
    }

    size_t size_read = 0;
    char file_path[1024];
    sprintf(file_path, "%d.bin", i);
    FILE* fp = fopen(file_path, "rb");

    if (fp != NULL)
    {
        unsigned char count;
        size_read = fread(&count, 1, 1, fp);
        unsigned char count_xor = count ^ 255;
        size_read = fread(&count_xor, 1, 1, fp);
        size_read = fread(file_buffer, 1, size, fp);
        fclose(fp);
    }
    else
    {
        exit(-1);
    }
    return size_read;
}

int tlsio_fuzz_send(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context)
{
    (void)tls_io;
    static int iteration = 0;

    size_t size_read;
    unsigned char file_buffer[4096];

    switch (iteration)
    {
        case 0: // client SASL tunnel request
            // server SASL tunnel response
            size_read = load_from_file(0, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            // server SASL Mechanisms
            size_read = load_from_file(1, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 1: // client SASL Init
            // server SASL Challenge
            size_read = load_from_file(2, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 2: // client connection header
            // server connection header
            //-> Header (AMQP 0.1.0.0)
            //<- Header (AMQP 0.1.0.0)
            size_read = load_from_file(3, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 3: // send Connection Open
            //-> [OPEN]* {69fdf035-a758-4ec8-b7e1-3c949800d785,ericwol-hub.azure-devices.net,4294967295,65535,240000}
            //<- [OPEN]* {DeviceGateway_5ad507ef78724b7386c9c3309fb31014,localhost,65536,8191,240000,NULL,NULL,NULL,NULL,NULL}
            //  container-id: DeviceGateway_5ad507ef78724b7386c9c3309fb31014
            //  hostname: localhost
            //  max-frame-size: 65536
            //  channel-max: 8191
            //  idle-time-out: 240000ms
            size_read = load_from_file(4, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 4: // send CONNECTION begin
            //-> [BEGIN]* {NULL, 0, 4294967295, 100, 4294967295}
            //<- [BEGIN]* {0,1,5000,4294967295,262143,NULL,NULL,NULL}
            //  remote-channel: 0
            //  next-outgoing-id: 1
            //  incoming-window: 5000
            size_read = load_from_file(5, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 5: // send SESSION attach $cbs-sender
            //-> [ATTACH]* {$cbs-sender,0,false,0,0,* {$cbs},* {$cbs},NULL,NULL,0,0}
            //<- [ATTACH]* {$cbs-sender,0,true,0,0,* {$cbs,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},* {$cbs,NULL,NULL,NULL,NULL,NULL,NULL},NULL,NULL,NULL,1048576,NULL,NULL,NULL}
            //  name: $cbs-sender
            //  handle: 0
            //  role: true
            //  snd-settle-mode: 0
            //  rcv-settle-mode: 0
            //  source: {$cbs,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}
            //  target: {$cbs,NULL,NULL,NULL,NULL,NULL,NULL}
            //  unsettled: NULL
            //  incomplete-unsettled: NULL
            //  initial-delivery-count: NULL
            //  max-message-size: 1048576
            //  offered-capabilities: NULL
            //  desired-capabilities: NULL
            //  properties: NULL
            size_read = load_from_file(6, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 6: // send SESSION attach $cbs-receiver
            //-> [ATTACH]* {$cbs-receiver,1,true,0,0,* {$cbs},* {$cbs},NULL,NULL,NULL,0}
            //<- [ATTACH]* {$cbs-receiver,1,false,0,0,* {$cbs,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},*
            //  name: $cbs-receiver
            //  handle: 1
            //  role: false
            //  snd-settle-mode: 0
            //  rcv-settle-mode: 0
            //  source: {$cbs,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}
            //  target: 
            size_read = load_from_file(8, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            //< -[FLOW] * {0, 5000, 1, 4294967295, 0, 0, 100, 0, NULL, false, NULL}
            //  next-incoming-id: 0
            //  incoming-window: 5000
            //  next-outgoing-id: 1
            //  outgoing-window: 4294967295
            //  handle: 0
            //  delivery-count: 0
            //  link-credit: 100
            //  available: 0
            //  drain: NULL
            //  echo: false
            //  properties: NULL
            size_read = load_from_file(7, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 7: // send_flow
            //-> [FLOW]* {1,4294967295,0,100,1,0,10000}
            //<- [DISPOSITION]* {true,0,NULL,true,* {},NULL}
            //  role: true
            //  first: 0
            //  last: NULL
            //  settled: true
            //  state: {}
            //  batchable: NULL
            size_read = load_from_file(9, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 8: // session_send_transfer
            // send SAS token to hub
            //-> [TRANSFER]* {0,0,<01 00 00 00>,0,false,false}
            //<- [TRANSFER]* {1,0,<01 00 00 00>,0,NULL,false,NULL,NULL,NULL,NULL,false}
            size_read = load_from_file(10, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 9: // send_disposition
            break;

        case 10: // send_attach (events)
            //<- [ATTACH]* {link-snd-eeqq-6322c15e-2c82-4a42-a57d-aa203cf1f4dc,2,true,0,NULL,* {link-snd-eeqq-6322c15e-2c82-4a42-a57d-aa203cf1f4dc-source,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},* {amqps://ericwol-hub.azure-devices.net/devices/eeqq/messages/events,NULL,NULL,NULL,NULL,NULL,NULL},NULL,NULL,NULL,1048576,NULL,NULL,{[com.microsoft:client-version:iothubclient/1.3.9 (native; WindowsProduct:0x00000004 6.2; x64; {F9FA04EF-2602-43AB-8505-A1EDE028ADD8})]}}
            size_read = load_from_file(11, file_buffer, sizeof(file_buffer));
            if (size_read > 0x5f + 41 && size > 31 + 41)
            {
                memcpy(&file_buffer[0x1f], &((char*)buffer)[31], 41);
                memcpy(&file_buffer[0x5f], &((char*)buffer)[31], 41);
            }
            received_queue_add(file_buffer, size_read);
            //<- [FLOW]* {1,5000,2,4294967295,2,0,1000,0,NULL,false,NULL}
            size_read = load_from_file(12, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 11: // send_attach (twin)
            //<- [ATTACH]* {link-snd-eeqq-54c3e766-02f0-4057-8c58-9299e97f1bf0,3,true,1,0,* {link-snd-eeqq-54c3e766-02f0-4057-8c58-9299e97f1bf0-source,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},* {amqps://ericwol-hub.azure-devices.net/devices/eeqq/twin,NULL,NULL,NULL,NULL,NULL,NULL},NULL,NULL,NULL,1048576,NULL,NULL,{[com.microsoft:client-version:iothubclient/1.3.9 (native; WindowsProduct:0x00000004 6.2; x64; {F9FA04EF-2602-43AB-8505-A1EDE028ADD8})],[com.microsoft:channel-correlation-id:twin:d90e52ce-a264-4d74-b236-cad0b08029a8],[com.microsoft:api-version:2019-10-01]}}
            size_read = load_from_file(14, file_buffer, sizeof(file_buffer));
            if (size_read > 0x60 + 41 && size > 31 + 41)
            {
                memcpy(&file_buffer[0x1f], &((char*)buffer)[31], 41);
                memcpy(&file_buffer[0x60], &((char*)buffer)[31], 41);
            }
            received_queue_add(file_buffer, size_read);
            //<- [FLOW]* {1,5000,2,4294967295,3,0,1000,0,NULL,false,NULL}
            size_read = load_from_file(15, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 12: // send_attach (devicebound)
            //<- [ATTACH]* {link-rcv-eeqq-5faca423-9286-4cfb-97ed-b7e6c4452fb3,4,false,NULL,1,* {amqps://ericwol-hub.azure-devices.net/devices/eeqq/messages/devicebound,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},* {link-rcv-eeqq-5faca423-9286-4cfb-97ed-b7e6c4452fb3-target,NULL,NULL,NULL,NULL,NULL,NULL},NULL,NULL,0,65536,NULL,NULL,{[com.microsoft:client-version:iothubclient/1.3.9 (native; WindowsProduct:0x00000004 6.2; x64; {F9FA04EF-2602-43AB-8505-A1EDE028ADD8})]}}
            size_read = load_from_file(13, file_buffer, sizeof(file_buffer));
            if (size_read > 0xb8 + 41 && size > 31 + 41)
            {
                memcpy(&file_buffer[0x1f], &((char*)buffer)[31], 41);
                memcpy(&file_buffer[0xb8], &((char*)buffer)[31], 41);
            }
            received_queue_add(file_buffer, size_read);
            break;

        case 13: //
            //<- [ATTACH]* {link-snd-eeqq-54c3e766-02f0-4057-8c58-9299e97f1bf0,3,true,1,0,* {link-snd-eeqq-54c3e766-02f0-4057-8c58-9299e97f1bf0-source,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},* {amqps://ericwol-hub.azure-devices.net/devices/eeqq/twin,NULL,NULL,NULL,NULL,NULL,NULL},NULL,NULL,NULL,1048576,NULL,NULL,{[com.microsoft:client-version:iothubclient/1.3.9 (native; WindowsProduct:0x00000004 6.2; x64; {F9FA04EF-2602-43AB-8505-A1EDE028ADD8})],[com.microsoft:channel-correlation-id:twin:d90e52ce-a264-4d74-b236-cad0b08029a8],[com.microsoft:api-version:2019-10-01]}}
            size_read = load_from_file(16, file_buffer, sizeof(file_buffer));
            if (size_read > 0x60 + 41 && size > 31 + 41)
            {
                memcpy(&file_buffer[0x1f], &((char*)buffer)[31], 41);
                memcpy(&file_buffer[0x60], &((char*)buffer)[31], 41);
            }
            received_queue_add(file_buffer, size_read);
            break;

        case 14: // session_send_transfer
            //twin get
            size_read = load_from_file(22, file_buffer, sizeof(file_buffer));
            if (size_read > 0x4d + 36 && size > 0x3e + 36)
            {
                memcpy(&file_buffer[0x4d], &((char*)buffer)[0x3e], 36);
            }
            received_queue_add(file_buffer, size_read);
            break;

        case 15: // send_flow
        case 16: // send_flow
            break;

        case 17: // session_send_transfer (twin get)
            //twin get
            size_read = load_from_file(23, file_buffer, sizeof(file_buffer));
            if (size_read > 0x4d + 36 && size > 0x3e + 36)
            {
                memcpy(&file_buffer[0x4d], &((char*)buffer)[0x3e], 36);
            }
            received_queue_add(file_buffer, size_read);
            break;

        case 18: // session_send_disposition
            //<- [DISPOSITION]* {true,1,NULL,true,* {},NULL}
            size_read = load_from_file(17, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 19: // send_attach (methods_responses_link)
            //<- [ATTACH]* {methods_responses_link-eeqq,6,true,1,0,* {responses,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},* {amqps://ericwol-hub.azure-devices.net/devices/eeqq/methods/devicebound,NULL,NULL,NULL,NULL,NULL,NULL},NULL,NULL,NULL,1048576,NULL,NULL,{[com.microsoft:channel-correlation-id:eeqq],[com.microsoft:api-version:2019-10-01]}}
            size_read = load_from_file(19, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 20: // send_attach (methods_requests_link)
            //<- [ATTACH]* {methods_requests_link-eeqq,7,false,1,0,* {amqps://ericwol-hub.azure-devices.net/devices/eeqq/methods/devicebound,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},* {requests,NULL,NULL,NULL,NULL,NULL,NULL},NULL,NULL,0,1048576,NULL,NULL,{[com.microsoft:channel-correlation-id:eeqq],[com.microsoft:api-version:2019-10-01]}}
            size_read = load_from_file(21, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 21: // session_send_transfer (telem)
            size_read = load_from_file(26, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;
        
        case 22: // session_send_disposition
            size_read = load_from_file(18, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;

        case 23: // session_send_transfer (twin PUT)
            size_read = load_from_file(25, file_buffer, sizeof(file_buffer));
            if (size_read > 0x4c + 36 && size > 0x70 + 36)
            {
                memcpy(&file_buffer[0x4c], &((char*)buffer)[0x70], 36);
            }
            received_queue_add(file_buffer, size_read);
            break;

        case 24: // remaining packets
            size_read = load_from_file(20, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            size_read = load_from_file(24, file_buffer, sizeof(file_buffer));
            received_queue_add(file_buffer, size_read);
            break;
    }


    iteration++;
    on_send_complete(callback_context, IO_SEND_OK);
    return 0;
}

void tlsio_fuzz_dowork(CONCRETE_IO_HANDLE tls_io)
{
    (void)tls_io;
    static int iteration = 1;

    if (received_queue != NULL)
    {
        QUEUE_ITEM* item = received_queue_remove();
        on_bytes_received_callback(on_bytes_received_context_callback, item->data, item->size);
        received_queue_free(item);
    }
    
    iteration++;
}


static const IO_INTERFACE_DESCRIPTION tlsio_fuzz_interface_description =
{
    tlsio_fuzz_retrieveoptions,
    tlsio_fuzz_create,
    tlsio_fuzz_destroy,
    tlsio_fuzz_open,
    tlsio_fuzz_close,
    tlsio_fuzz_send,
    tlsio_fuzz_dowork,
    tlsio_fuzz_setoption
};


#ifdef WIN32
const IO_INTERFACE_DESCRIPTION* tlsio_schannel_get_interface_description(void)
{
    return &tlsio_fuzz_interface_description;
}
#else
const IO_INTERFACE_DESCRIPTION* tlsio_openssl_get_interface_description(void)
{
    return &tlsio_fuzz_interface_description;
}
#endif

