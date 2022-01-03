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
//   cmake --build . --target iothubclient_fuzz_mqtt
//

// Run
//   cd ~/azure-iot-sdk-c/iothub_client/tests/iothubclient_fuzz_mqtt
//   mkdir ~/azure-iot-sdk-c/iothub_client/tests/iothubclient_fuzz_mqtt/findings_dir
//   afl-fuzz -m 230000000 -t 10000 -i conack -o findings_dir ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_mqtt/iothubclient_fuzz_mqtt CONACK @@
//   afl-fuzz -m 230000000 -t 10000 -i suback -o findings_dir_suback ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_mqtt/iothubclient_fuzz_mqtt SUBACK @@
//   afl-fuzz -m 230000000 -t 10000 -i twinget -o findings_dir_twinget ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_mqtt/iothubclient_fuzz_mqtt TWINGET @@
//   afl-fuzz -m 230000000 -t 10000 -i puback -o findings_dir_puback ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_mqtt/iothubclient_fuzz_mqtt PUBACK @@
//   afl-fuzz -m 230000000 -t 10000 -i c2d -o findings_dir_c2d ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_mqtt/iothubclient_fuzz_mqtt C2D @@
//   afl-fuzz -m 230000000 -t 10000 -i devicemethod -o findings_dir_method ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_mqtt/iothubclient_fuzz_mqtt DEVICEMETHOD @@
//   afl-fuzz -m 230000000 -t 10000 -i twinupdate -o findings_dir_twinupdate ~/azure-iot-sdk-c/cmake/iothub_client/tests/iothubclient_fuzz_mqtt/iothubclient_fuzz_mqtt TWINUPDATE @@

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


/* Paste in the your iothub connection string  */
static const char* connectionString = "HostName=fuzz-hub.azure-devices.net;DeviceId=fuzzdevice;SharedAccessKey=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX=";
#define MESSAGE_COUNT        5
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;
static size_t g_message_recv_count = 0;
static size_t conack_count = 0;

// MQTT PACKETS
static unsigned char PINGRESP[] = { 0xd0, 0x00 };
static unsigned char CONACK[] = { 0x20, 0x02, 0x00, 0x00 };
static unsigned char PUBACK[] = { 0x40, 0x02, 0x00, 0x05 };
static unsigned char SUBACK[] = { 0x90, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static unsigned char C2D[] = { 0x32, 0x95, 0x01, 0x00, 0x84, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x73, 0x2f, 0x73, 0x6e, 0x6f, 0x6f, 0x70, 0x79, 0x2f, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x73, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x62, 0x6f, 0x75, 0x6e, 0x64, 0x2f, 0x25, 0x32, 0x34, 0x2e, 0x6d, 0x69, 0x64, 0x3d, 0x39, 0x39, 0x32, 0x33, 0x64, 0x35, 0x65, 0x31, 0x2d, 0x32, 0x37, 0x35, 0x39, 0x2d, 0x34, 0x37,
                               0x30, 0x36, 0x2d, 0x39, 0x35, 0x37, 0x61, 0x2d, 0x38, 0x63, 0x38, 0x37, 0x32, 0x38, 0x64, 0x36, 0x63, 0x37, 0x65, 0x61, 0x26, 0x25, 0x32, 0x34, 0x2e, 0x74, 0x6f, 0x3d, 0x25, 0x32, 0x46, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x73, 0x25, 0x32, 0x46, 0x73, 0x6e, 0x6f, 0x6f, 0x70, 0x79, 0x25, 0x32, 0x46, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x73, 0x25, 0x32, 0x46, 0x64, 0x65, 0x76, 0x69,
                               0x63, 0x65, 0x62, 0x6f, 0x75, 0x6e, 0x64, 0x00, 0x04, 0x7b, 0x22, 0x66, 0x6f, 0x6f, 0x22, 0x3a, 0x22, 0x62, 0x61, 0x72, 0x22, 0x7d };
static unsigned char DEVICE_METHOD[] = { 0x30, 0x31, 0x00, 0x22, 0x24, 0x69, 0x6f, 0x74, 0x68, 0x75, 0x62, 0x2f, 0x6d, 0x65, 0x74, 0x68, 0x6f, 0x64, 0x73, 0x2f, 0x50, 0x4f, 0x53, 0x54, 0x2f, 0x62, 0x6c, 0x69, 0x6e, 0x6b, 0x2f, 0x3f, 0x24, 0x72, 0x69, 0x64, 0x3d, 0x31, 0x7b, 0x22, 0x66, 0x6f, 0x6f, 0x22, 0x3a, 0x22, 0x62, 0x61, 0x72, 0x22, 0x7d };
static unsigned char TWIN_UPDATE[] = { 0x30, 0x4a, 0x00, 0x31, 0x24, 0x69, 0x6f, 0x74, 0x68, 0x75, 0x62, 0x2f, 0x74, 0x77, 0x69, 0x6e, 0x2f, 0x50, 0x41, 0x54, 0x43, 0x48, 0x2f, 0x70, 0x72, 0x6f, 0x70, 0x65, 0x72, 0x74, 0x69, 0x65, 0x73, 0x2f, 0x64, 0x65, 0x73, 0x69, 0x72, 0x65, 0x64, 0x2f, 0x3f, 0x24, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x3d, 0x34, 0x7b, 0x22, 0x64, 0x64, 0x64, 0x64, 0x22, 0x3a, 0x34, 0x2c, 0x22, 0x24, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x34, 0x7d };
static unsigned char TWIN_GET[] = { 0x32, 0x5b, 0x00, 0x1c, 0x24, 0x69, 0x6f, 0x74, 0x68, 0x75, 0x62, 0x2f, 0x74, 0x77, 0x69, 0x6e, 0x2f, 0x72, 0x65, 0x73, 0x2f, 0x32, 0x30, 0x30, 0x2f, 0x3f, 0x24, 0x72, 0x69, 0x64, 0x3d, 0x39, 0x7b, 0x22, 0x64, 0x65, 0x73, 0x69, 0x72, 0x65, 0x64, 0x22, 0x3a, 0x7b, 0x22, 0x64, 0x64, 0x64, 0x64, 0x22, 0x3a, 0x34, 0x2c, 0x22, 0x24, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x34, 0x7d, 0x2c, 0x22, 0x72, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x64, 0x22, 0x3a, 0x7b, 0x22, 0x24, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x31, 0x7d, 0x7d };
static const char topic_twin_get[] = "$iothub/twin/GET";

enum TestCase {
    TESTCASE_NONE,
    TESTCASE_CONACK,
    TESTCASE_PUBACK,
    TESTCASE_SUBACK,
    TESTCASE_C2D,
    TESTCASE_DEVICEMETHOD,
    TESTCASE_TWINUPDATE,
    TESTCASE_TWINGET
};
enum TestCase test_case = TESTCASE_NONE;
char* test_filepath;
unsigned char filebuffer[2048];
size_t filebuffer_len;


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

    if (argc == 3)
    {
        test_filepath = (char*)argv[2];
        if (strcmp(argv[1], "CONACK") == 0)
        {
            test_case = TESTCASE_CONACK;
        }
        else if (strcmp(argv[1], "PUBACK") == 0)
        {
            test_case = TESTCASE_PUBACK;
        }
        else if (strcmp(argv[1], "SUBACK") == 0)
        {
            test_case = TESTCASE_SUBACK;
        }
        else if (strcmp(argv[1], "TWINGET") == 0)
        {
            test_case = TESTCASE_TWINGET;
        }
        else if (strcmp(argv[1], "C2D") == 0)
        {
            test_case = TESTCASE_C2D;
        }
        else if (strcmp(argv[1], "DEVICEMETHOD") == 0)
        {
            test_case = TESTCASE_DEVICEMETHOD;
        }
        else if (strcmp(argv[1], "TWINUPDATE") == 0)
        {
            test_case = TESTCASE_TWINUPDATE;
        }

        FILE* fp = fopen(test_filepath, "r");
        if (fp == NULL)
        {
            (void)printf("ERROR: Cant open file %s\r\n", test_filepath);
            return -1;
        }
        filebuffer_len = fread(filebuffer, sizeof(filebuffer[0]), sizeof(filebuffer), fp);
        fclose(fp);

        if (filebuffer_len == sizeof(filebuffer))
        {
            (void)printf("ERROR: test file is too big\r\n");
            return -1;
        }
    }
    else if (argc > 1)
    {
        (void)printf("Usage:\r\n");
        (void)printf("   iothubclient_fuzz CONACK <filepath>\r\n");
        (void)printf("   iothubclient_fuzz PUBACK <filepath>\r\n");
        (void)printf("   iothubclient_fuzz SUBACK <filepath>\r\n");
        (void)printf("   iothubclient_fuzz C2D <filepath>\r\n");
        (void)printf("   iothubclient_fuzz DEVICEMETHOD <filepath>\r\n");
        (void)printf("   iothubclient_fuzz TWINUPDATE <filepath>\r\n");
        (void)printf("   iothubclient_fuzz TWINGET <filepath>\r\n");
        return 0;
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

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_WS
        //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
        //you are URL Encoding inputs yourself.
        //ONLY valid for use with MQTT
        //bool urlEncodeOn = true;
        //IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif

        // Setting connection status callback to get indication of connection to iothub
        (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL);
        (void)IoTHubDeviceClient_LL_SetMessageCallback(device_ll_handle, receive_msg_callback, NULL);
        (void)IoTHubDeviceClient_LL_GetTwinAsync(device_ll_handle, deviceTwinCallback, NULL);
        (void)IoTHubDeviceClient_LL_SetDeviceMethodCallback(device_ll_handle, device_method_callback, NULL);
        (void)IoTHubDeviceClient_LL_SetDeviceTwinCallback(device_ll_handle, deviceTwinCallback, NULL);

        int loop_count;
        for (loop_count = 0; g_continueRunning && loop_count < 100; loop_count++)
        {
            if (messages_sent < MESSAGE_COUNT)
            {
                // Construct the iothub message from a string or a byte array
                message_handle = IoTHubMessage_CreateFromString(telemetry_msg);

                (void)printf("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
                IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, send_confirm_callback, NULL);

                // The message is copied to the sdk so the we can destroy it
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

size_t mqtt_parse_packet_length(const unsigned char* buffer, size_t* idx)
{
    char encodedByte;
    size_t multiplier = 1;
    size_t value = 0;
    do
    {
        encodedByte = buffer[*idx];
        *idx += 1;
        value += (encodedByte & 127) * multiplier;
        multiplier *= 128;
    } while ((encodedByte & 128) != 0);
    return value;
}

void mqtt_parse_packet(const unsigned char* buffer, size_t size)
{
    (void)size;
    int qos;
    int subscribe_topic_num;
    size_t topic_len;
    char topic_name[512];

    size_t idx = 0;
    unsigned char mqtt_control_packet_type = buffer[idx] >> 4;
    
    idx++;
    size_t packet_len = mqtt_parse_packet_length(buffer, &idx);

    switch (mqtt_control_packet_type)
    {
        case 1: //CONNECT
            if (test_case == TESTCASE_CONACK && conack_count == 0)
            {
                received_queue_add(filebuffer, filebuffer_len);
                conack_count++;
            }
            else
            { 
                received_queue_add(CONACK, sizeof(CONACK));
            }
            break;

        case 3: //PUBLISH
            topic_len = ((size_t)buffer[idx] << 8) + ((size_t)buffer[idx + 1]);
            idx += 2;
            qos = (buffer[0] >> 1) & 0x03;
            if (qos > 0)
            {
                PUBACK[2] = buffer[idx + topic_len];      // packet id
                PUBACK[3] = buffer[idx + topic_len + 1];

                if (test_case == TESTCASE_PUBACK &&
                    PUBACK[2] == filebuffer[2] && PUBACK[3] == filebuffer[3])
                {
                    received_queue_add(filebuffer, filebuffer_len);
                }
                else
                {
                    received_queue_add(PUBACK, sizeof(PUBACK));
                }
            }

            memset(topic_name, 0, sizeof(topic_name));
            memcpy(topic_name, &buffer[idx], topic_len);

            if (memcmp(topic_twin_get, topic_name, sizeof(topic_twin_get) - 1) == 0)
            {
                if (test_case == TESTCASE_TWINGET)
                {
                    received_queue_add(filebuffer, filebuffer_len);
                }
                else
                {
                    TWIN_GET[31] = topic_name[strlen(topic_name) - 1];
                    received_queue_add(TWIN_GET, sizeof(TWIN_GET));
                }
            }
            break;

        case 8: //SUBSCRIBE
            subscribe_topic_num = 0;
            SUBACK[1] = 0x02;
            SUBACK[2] = buffer[idx];      // packet id
            SUBACK[3] = buffer[idx + 1];
            packet_len -= 2;
            idx += 2;
            while (packet_len)
            {
                // subscribe topic
                topic_len = ((size_t)buffer[idx] << 8) + ((size_t)buffer[idx + 1]);
                idx += topic_len + 2;
                packet_len -= topic_len + 2;
                
                // subscribe qos
                idx++;
                packet_len--;

                SUBACK[1] += 1;
                SUBACK[4 + subscribe_topic_num] = 0x01;
                subscribe_topic_num++;
            }

            if (test_case == TESTCASE_SUBACK &&
                SUBACK[2] == filebuffer[2] && SUBACK[3] == filebuffer[3])
            {
                received_queue_add(filebuffer, filebuffer_len);
            }
            else
            {
                received_queue_add(SUBACK, 4 + (size_t)subscribe_topic_num);
            }
            break;

        case 12: //PINGREQ
            received_queue_add(PINGRESP, sizeof(PINGRESP));
            break;
    }
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

int tlsio_fuzz_send(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context)
{
    (void)tls_io;
    (void)buffer;
    (void)size;

    mqtt_parse_packet(buffer, size);

    on_send_complete(callback_context, IO_SEND_OK);
    return 0;
}

void tlsio_fuzz_dowork(CONCRETE_IO_HANDLE tls_io)
{
    (void)tls_io;
    static int iteration;

    if (received_queue != NULL)
    {
        QUEUE_ITEM* item = received_queue_remove();
        on_bytes_received_callback(on_bytes_received_context_callback, item->data, item->size);
        received_queue_free(item);
    }

    iteration++;
    if (iteration == 20)
    {
        if (test_case == TESTCASE_DEVICEMETHOD)
        {
            received_queue_add(filebuffer, filebuffer_len);
        }
        else if (test_case == TESTCASE_C2D)
        {
            received_queue_add(filebuffer, filebuffer_len);     
        }
        else if (test_case == TESTCASE_TWINUPDATE)
        {
            received_queue_add(filebuffer, filebuffer_len);
        }
    }


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